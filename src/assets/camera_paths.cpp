#include "assets/camera_paths.h"

#include "assets/assets.h"

#include <cstring>

namespace hotd {

namespace {

template <typename T> T rd(const std::vector<uint8_t>& b, size_t o)
{
    T v{};
    if (o + sizeof(T) <= b.size()) std::memcpy(&v, b.data() + o, sizeof(T));
    return v;
}

} // namespace

// "PIFF" container, "BODY" chunk at 0x20, data at 0x28: a u32 offset per path (the first offset also gives the count), each
// path = 6 u32 channel offsets; a channel = u16 slots, u16 (unknown), then slots x {frame, value, in, out} floats, unused
// slots repeating the last frame. All offsets are relative to the BODY data.
bool CameraPaths::load(const std::string& camallPath, std::string& err)
{
    std::vector<uint8_t> d;
    if (!readFile(camallPath, d, err)) return false;
    if (d.size() < 0x2C || std::memcmp(d.data(), "PIFF", 4) != 0 || std::memcmp(d.data() + 0x20, "BODY", 4) != 0) {
        err = camallPath + ": not a CAMALL.RBH";
        return false;
    }
    const size_t base = 0x28;
    const uint32_t first = rd<uint32_t>(d, base);
    const size_t n = first / 4;
    paths_.assign(n, {});
    for (size_t i = 0; i < n; ++i) {
        const size_t rec = base + rd<uint32_t>(d, base + 4 * i);
        for (int c = 0; c < 6; ++c) {
            const size_t o = base + rd<uint32_t>(d, rec + 4 * c);
            const uint16_t slots = rd<uint16_t>(d, o);
            Channel& ch = paths_[i].ch[c];
            for (uint16_t k = 0; k < slots && o + 4 + 16 * (k + 1) <= d.size(); ++k) {
                Key key{rd<float>(d, o + 4 + 16 * k), rd<float>(d, o + 8 + 16 * k), rd<float>(d, o + 12 + 16 * k),
                        rd<float>(d, o + 16 + 16 * k)};
                if (!ch.empty() && key.t <= ch.back().t) break;
                ch.push_back(key);
            }
        }
    }
    return true;
}

float CameraPaths::length(int path) const
{
    const Channel& c = paths_[path].ch[0];
    return c.empty() ? 0.0f : c.back().t;
}

// cubic Hermite between keys; tangents are per frame
float CameraPaths::eval(const Channel& c, float t)
{
    if (c.empty()) return 0;
    if (t <= c.front().t) return c.front().v;
    for (size_t i = 0; i + 1 < c.size(); ++i) {
        const Key& a = c[i];
        const Key& b = c[i + 1];
        if (t > b.t) continue;
        const float h = b.t - a.t, s = (t - a.t) / h, s2 = s * s, s3 = s2 * s;
        return (2 * s3 - 3 * s2 + 1) * a.v + (s3 - 2 * s2 + s) * h * a.out + (-2 * s3 + 3 * s2) * b.v + (s3 - s2) * h * b.in;
    }
    return c.back().v;
}

void CameraPaths::evaluate(int path, float t, float eye[3], float target[3]) const
{
    const Path& p = paths_[path];
    for (int i = 0; i < 3; ++i) {
        eye[i] = eval(p.ch[i], t);
        target[i] = eval(p.ch[3 + i], t);
    }
}

} // namespace hotd
