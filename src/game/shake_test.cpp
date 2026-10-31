// hotd_shaketest: shake to reload (game/shake.h) against synthetic sensor traces, offline (no pad, no SDL). A DualSense
// reports at 250 Hz: each case is a gyro (rad/s) + linear acceleration (m/s^2) trace at that rate, run at the levels
// Low / Medium / High. A flick, a wrist roll, a jerk up / down and side to side must reload at Medium (the default) and
// High, a hard shake at every level; slow and fast aim sweeps, a hand's tremor and rumble never reload. Exit code 0: all
// passed.
#include "game/shake.h"

#include <cmath>
#include <cstdio>
#include <functional>
#include <random>

using hotd::game::ShakeDetector;

namespace {

constexpr float kPi = 3.14159265f;
constexpr float kHz = 250.0f;

// one reading: the gyro's rates and the linear acceleration at time t (s)
using Trace = std::function<void(float t, float r[3], float a[3])>;

// a half sine of height h from t0 lasting d (0 outside)
float bump(float t, float t0, float d, float h)
{
    return t >= t0 && t <= t0 + d ? h * std::sin(kPi * (t - t0) / d) : 0.0f;
}

int reloads(const Trace& tr, float seconds, int level)
{
    ShakeDetector s;
    int n = 0;
    const float dt = 1.0f / kHz;
    for (int i = 0; i < int(seconds * kHz); ++i) {
        const float t = float(i) * dt;
        const uint64_t ts = 1000000000ull + uint64_t(double(t) * 1e9);
        float r[3] = {}, a[3] = {};
        tr(t, r, a);
        n += s.gyro(r, ts, level);
        n += s.accel(a, dt, ts, level);
    }
    return n;
}

} // namespace

int main()
{
    std::mt19937 rng(1998);
    std::normal_distribution<float> noise(0.0f, 1.0f);
    struct Case {
        const char* name;
        int from;        // reloads expected from this level up (1 Low, 2 Medium, 3 High; 0: none at any level)
        float seconds;
        Trace tr;
    };
    const Case cases[] = {
        // a quick jerk on the lap: 3 cm forward and stopped in 0.12 s (10 m/s^2 then -10), the pad tipping a little
        {"flick / jerk forward on the lap (1.0 g)", 2, 1.0f,
         [](float t, float r[3], float a[3]) {
             a[2] = bump(t, 0.3f, 0.06f, -10.0f) + bump(t, 0.36f, 0.06f, 10.0f);
             r[0] = bump(t, 0.3f, 0.12f, 1.0f);
         }},
        // a wrist flick: rolled 20 degrees and back in 0.16 s (a 350 degrees / s peak), little linear motion
        {"wrist roll flick (350 deg/s)", 2, 1.0f,
         [](float t, float r[3], float a[3]) {
             r[2] = bump(t, 0.3f, 0.08f, 6.2f) + bump(t, 0.38f, 0.08f, -6.2f);
             a[0] = bump(t, 0.3f, 0.08f, 2.0f) + bump(t, 0.38f, 0.08f, -2.0f);
         }},
        // a wrist flick up (pitch), the pad held flat: 15 degrees in 0.1 s and back
        {"wrist pitch flick (300 deg/s)", 2, 1.0f,
         [](float t, float r[3], float a[3]) {
             r[0] = bump(t, 0.3f, 0.09f, 5.3f) + bump(t, 0.39f, 0.09f, -5.3f);
             a[1] = bump(t, 0.3f, 0.09f, 1.5f);
         }},
        // the pad flat (parallel to the ground), moved up and down: 3 cm up in 0.1 s and stopped (0.9 g each way)
        {"up / down, held flat (0.9 g)", 2, 1.0f,
         [](float t, float*, float a[3]) {
             a[1] = bump(t, 0.3f, 0.05f, 9.5f) + bump(t, 0.35f, 0.05f, -9.5f) + bump(t, 0.4f, 0.05f, -9.5f) +
                    bump(t, 0.45f, 0.05f, 9.5f);
         }},
        // side to side at 4 Hz, 0.8 g, turning a little with it
        {"side to side (4 Hz, 0.8 g)", 2, 1.0f,
         [](float t, float r[3], float a[3]) {
             if (t > 0.3f && t < 0.8f) {
                 a[0] = 8.0f * std::sin(2 * kPi * 4.0f * (t - 0.3f));
                 r[1] = 1.5f * std::cos(2 * kPi * 4.0f * (t - 0.3f));
             }
         }},
        // a hard shake (every level): 1.6 g each way and a 500 degrees / s turn
        {"hard shake (1.6 g, 500 deg/s)", 1, 1.0f,
         [](float t, float r[3], float a[3]) {
             a[0] = bump(t, 0.3f, 0.06f, 16.0f) + bump(t, 0.36f, 0.06f, -16.0f);
             r[2] = bump(t, 0.3f, 0.06f, 8.7f) + bump(t, 0.36f, 0.06f, -8.7f);
         }},
        // a slow aim sweep: 60 degrees in 0.6 s (a 175 degrees / s peak: almost two screen widths), the arm moving
        {"slow aim sweep (175 deg/s)", 0, 1.5f,
         [](float t, float r[3], float a[3]) {
             r[1] = bump(t, 0.3f, 0.6f, 3.05f);
             a[0] = bump(t, 0.3f, 0.3f, 1.5f) + bump(t, 0.6f, 0.3f, -1.5f);
         }},
        // a fast aim snap: 30 degrees in 0.2 s (a 235 degrees / s peak), with its own stop
        {"fast aim snap (235 deg/s)", 0, 1.0f,
         [](float t, float r[3], float a[3]) {
             r[1] = bump(t, 0.3f, 0.22f, 4.1f);
             r[0] = bump(t, 0.3f, 0.22f, 1.0f);
             a[0] = bump(t, 0.3f, 0.11f, 3.0f) + bump(t, 0.41f, 0.11f, -3.0f);
         }},
        // a hand's tremor while aiming: 9 Hz, 6 degrees / s, 0.3 m/s^2, plus sensor noise, 3 s
        {"tremor + noise while aiming", 0, 3.0f,
         [&](float t, float r[3], float a[3]) {
             for (int k = 0; k < 3; ++k) {
                 r[k] = 0.1f * std::sin(2 * kPi * 9.0f * t + float(k)) + 0.02f * noise(rng);
                 a[k] = 0.3f * std::sin(2 * kPi * 9.0f * t + float(k)) + 0.15f * noise(rng);
             }
         }},
        // rumble (a hit's vibration): 0.4 g of noise at the sensor's rate, 2 s
        {"rumble (0.4 g of noise)", 0, 2.0f,
         [&](float, float r[3], float a[3]) {
             for (int k = 0; k < 3; ++k) {
                 r[k] = 0.15f * noise(rng);
                 a[k] = 4.0f * noise(rng);
             }
         }},
    };
    int failed = 0, n = 0;
    std::printf("%-42s %6s %6s %6s  %s\n", "case (reloads at)", "Low", "Medium", "High", "result");
    for (const Case& c : cases) {
        int r[4] = {};
        for (int l = 1; l <= 3; ++l) r[l] = reloads(c.tr, c.seconds, l);
        const bool off = reloads(c.tr, c.seconds, 0) == 0;
        bool ok = off;
        for (int l = 1; l <= 3; ++l) {
            if (!c.from) ok = ok && r[l] == 0;
            else if (l >= c.from) ok = ok && r[l] >= 1;
        }
        std::printf("%-42s %6d %6d %6d  %s\n", c.name, r[1], r[2], r[3], ok ? "ok" : "FAILED");
        failed += !ok;
        ++n;
    }
    // a long shake: one reload per cooldown at most (4 s at 5 Hz)
    {
        const Trace tr = [](float t, float r[3], float a[3]) {
            a[0] = 12.0f * std::sin(2 * kPi * 5.0f * t);
            r[2] = 7.0f * std::sin(2 * kPi * 5.0f * t);
        };
        const int k = reloads(tr, 4.0f, 2);
        const bool ok = k >= 4 && k <= int(4.0f / 0.4f) + 1;
        std::printf("%-42s %6s %6d %6s  %s\n", "shaking 4 s (cooldown 0.4 s)", "", k, "", ok ? "ok" : "FAILED");
        failed += !ok;
        ++n;
    }
    std::printf("shake test: %d passed, %d failed\n", n - failed, failed);
    return failed ? 1 : 0;
}
