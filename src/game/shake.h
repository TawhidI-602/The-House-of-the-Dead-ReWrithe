// Shake to reload (pads.h), ours: neither original had pads. relpad (user 2026-10-30, a DualSense on the lap: "a quick
// jerk or flick of the wrist should reload"; it took a real shake before): two short-window peak detectors, any axis,
// with no SDL in them (the offline test, src/game/shake_test.cpp, feeds them synthetic sensor traces):
//   - the turn: the gyro's rate (rad/s, the bias off) past kRate within kOnset of the pad being slow (below 40 % of
//     it): a flick or roll of the wrist. An aim sweep never gets there: it is slower, or speeds up gradually
//   - the jerk: the pad's linear acceleration (m/s^2, gravity off; smoothed over ~10 ms against rumble and noise) past
//     kAcc, then past 70 % of it the other way within kWindow: a jerk up / down, forward / back or side to side
// Then a cooldown (kCool). holdTo: a candidate is building, the gyro's motion until then is held back by Pads (and
// dropped on a reload, so the crosshair stays).
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace hotd::game {

struct ShakeDetector {
    // the levels 1 Low, 2 Medium (the default), 3 High (the most generous); 0 off
    static constexpr float kRate[4] = {0.0f, 7.0f, 5.0f, 4.5f};    // rad/s (Medium: 286 degrees / s)
    static constexpr float kAcc[4] = {0.0f, 11.0f, 7.0f, 5.0f};    // m/s^2 (Medium: 0.7 g)
    static constexpr uint64_t kMs = 1000000ull;
    static constexpr uint64_t kOnset = 100 * kMs, kWindow = 300 * kMs, kCool = 400 * kMs, kMinGap = 30 * kMs;
    static constexpr float kTau = 0.015f;   // the acceleration's smoothing (s)

    float f[3] = {};        // the smoothed linear acceleration
    float dir[3] = {};      // the jerk's first peak's direction
    uint64_t slowAt = 0, t1 = 0, cool = 0, holdTo = 0;
    int state = 0;          // 1: a jerk's first peak seen at t1
    bool seen = false;      // a gyro reading seen (slowAt valid)

    void reset() { *this = ShakeDetector{}; }

    // a gyro reading (r: rad/s, the bias off) at ts (ns): true = a reload
    bool gyro(const float r[3], uint64_t ts, int level)
    {
        const float T = kRate[std::clamp(level, 0, 3)];
        const float w = std::sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]);
        if (!seen || w < 0.4f * T) {
            slowAt = ts;
            seen = true;
        }
        if (T <= 0 || ts < cool || ts - slowAt > kOnset)
            return false;
        if (w > T)
            return fire(ts);
        if (w > 0.7f * T)   // maybe a flick: held back at most until the onset window's end
            holdTo = std::max(holdTo, slowAt + kOnset + 20 * kMs);
        return false;
    }

    // an accelerometer reading (lin: m/s^2, gravity off) dt seconds after the last one, at ts (ns): true = a reload
    bool accel(const float lin[3], float dt, uint64_t ts, int level)
    {
        const float T = kAcc[std::clamp(level, 0, 3)];
        const float a = 1.0f - std::exp(-std::max(dt, 0.0f) / kTau);
        for (int k = 0; k < 3; ++k) f[k] += a * (lin[k] - f[k]);
        const float m = std::sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
        if (T <= 0 || ts < cool) {
            state = 0;
            return false;
        }
        if (m > 0.6f * T)   // maybe a jerk: the gyro's motion is held back for a moment
            holdTo = std::max(holdTo, ts + 120 * kMs);
        if (state == 1 && ts - t1 > kWindow)
            state = 0;
        if (state == 0) {
            if (m > T) {
                state = 1;
                t1 = ts;
                for (int k = 0; k < 3; ++k) dir[k] = f[k] / m;
            }
        } else if (f[0] * dir[0] + f[1] * dir[1] + f[2] * dir[2] < -0.7f * T && ts - t1 >= kMinGap) {
            return fire(ts);
        }
        return false;
    }

private:
    bool fire(uint64_t ts)
    {
        state = 0;
        cool = ts + kCool;
        holdTo = 0;
        return true;
    }
};

} // namespace hotd::game
