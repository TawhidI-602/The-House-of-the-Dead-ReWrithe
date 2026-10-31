// Small helpers of the port (docs/port.md): float <-> bit patterns, the i960's truncating conversion.
#pragma once

#include <cstdint>
#include <cstring>

namespace hotd::port {

inline uint32_t fbits(float f) { uint32_t u; std::memcpy(&u, &f, 4); return u; }
inline float ffrom(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }
inline double ffrom64(uint64_t u) { double d; std::memcpy(&d, &u, 8); return d; }
// cvtzril: float -> integer, truncated toward zero (64-bit result; the game keeps the low word)
inline int32_t cvtzril(float f) { return int32_t(int64_t(double(f))); }

// the angle pair fix of shot_ray_setup: when bits 15 and 14 of the second angle differ (it leaves -0x4000..0x3FFF),
// a1 = 0x8000 - a1, a0 += 0x8000 (full words)
inline void fixAngles(uint32_t& a0, uint32_t& a1)
{
    uint32_t s = uint32_t(int32_t(int16_t(a1 & 0xFFFF)));
    if (((s << 1) ^ s) & 0x8000) {
        a1 = 0x8000 - a1;
        a0 = a0 + 0x8000;
    }
}

} // namespace hotd::port
