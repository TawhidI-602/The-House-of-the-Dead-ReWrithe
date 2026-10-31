#include "util/recolour.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>

namespace hotd {

namespace {

bool parseHex(const std::string& s, uint8_t rgb[3])
{
    size_t i = 0;
    while (i < s.size() && std::isspace(uint8_t(s[i]))) ++i;
    if (i < s.size() && s[i] == '#') ++i;
    std::string h;
    while (i < s.size() && std::isxdigit(uint8_t(s[i]))) h += s[i++];
    if (h.size() != 6) return false;
    const unsigned long v = std::strtoul(h.c_str(), nullptr, 16);
    rgb[0] = uint8_t(v >> 16);
    rgb[1] = uint8_t(v >> 8);
    rgb[2] = uint8_t(v);
    return true;
}

} // namespace

void recolourRgba(uint8_t* px, size_t pixels, const Recolour& r)
{
    if (r.op == Recolour::None) return;
    for (size_t i = 0; i < pixels; ++i, px += 4) {
        if (px[3] == 0) continue;
        if (r.op == Recolour::Ramp) {
            const int R = px[0], G = px[1], B = px[2];
            if (r.select == Recolour::BlueDominant && !(B > R + 16 && B >= G))
                continue;
            if (r.select == Recolour::RedDominant && !(R > B + 16 && R >= G))
                continue;
            const float y = (0.299f * float(R) + 0.587f * float(G) + 0.114f * float(B)) / 255.0f;
            const float t = std::clamp(0.5f * y + 0.5f * float(std::max({R, G, B})) / 255.0f, 0.0f, 1.0f) * 4.0f;
            const int k = std::min(3, int(t));
            const float f = t - float(k);
            for (int c = 0; c < 3; ++c)
                px[c] = uint8_t(std::lround(float(r.ramp[k][c]) * (1.0f - f) + float(r.ramp[k + 1][c]) * f));
            continue;
        }
        if (r.op == Recolour::Tint) {
            const int v = std::max({px[0], px[1], px[2]});
            for (int c = 0; c < 3; ++c) px[c] = uint8_t(r.rgb[c] * v / 255);
        } else if (r.op == Recolour::Multiply) {
            for (int c = 0; c < 3; ++c) px[c] = uint8_t(px[c] * r.rgb[c] / 255);
        } else {   // Hue: RGB -> HSV, h + hueDeg, back
            const float R = px[0] / 255.0f, G = px[1] / 255.0f, B = px[2] / 255.0f;
            const float mx = std::max({R, G, B}), mn = std::min({R, G, B}), d = mx - mn;
            if (d <= 0.0f) continue;
            float h = mx == R ? std::fmod((G - B) / d, 6.0f) : mx == G ? (B - R) / d + 2.0f : (R - G) / d + 4.0f;
            h = std::fmod(h * 60.0f + r.hueDeg, 360.0f);
            if (h < 0) h += 360.0f;
            const float s = d / mx, v = mx, c = v * s, x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f)), m = v - c;
            float o[3];
            const int k = int(h / 60.0f) % 6;
            const float t[6][3] = {{c, x, 0}, {x, c, 0}, {0, c, x}, {0, x, c}, {x, 0, c}, {c, 0, x}};
            for (int q = 0; q < 3; ++q) o[q] = t[k][q] + m;
            for (int q = 0; q < 3; ++q) px[q] = uint8_t(std::clamp(int(std::lround(o[q] * 255.0f)), 0, 255));
        }
    }
}

bool parseRecolour(const std::string& text, Recolour& out)
{
    std::string s = text;
    size_t a = s.find_first_not_of(" \t");
    if (a == std::string::npos) return false;
    s = s.substr(a);
    std::string word;
    size_t i = 0;
    while (i < s.size() && std::isalpha(uint8_t(s[i]))) word += char(std::tolower(uint8_t(s[i++])));
    const std::string rest = s.substr(i);
    Recolour r;
    if (word.empty() || word == "tint") {
        r.op = Recolour::Tint;
        if (!parseHex(word.empty() ? s : rest, r.rgb)) return false;
    } else if (word == "multiply" || word == "mul") {
        r.op = Recolour::Multiply;
        if (!parseHex(rest, r.rgb)) return false;
    } else if (word == "hue") {
        char* end = nullptr;
        r.op = Recolour::Hue;
        r.hueDeg = std::strtof(rest.c_str(), &end);
        if (end == rest.c_str()) return false;
    } else {
        return false;
    }
    out = r;
    return true;
}

} // namespace hotd
