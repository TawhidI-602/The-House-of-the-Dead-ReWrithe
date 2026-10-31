// Geometry coprocessor (MB86235 "TGP"), A behaviour. Every arithmetic step is one float32 operation in the program's
// order (MSVC /fp:precise on x64 = SSE, no contraction); see tgp.h and docs/coprocessor.md.
#include "port/tgp.h"

#include <cfloat>
#include <cmath>
#include <cstring>

namespace hotd::port {

namespace {

float hf(uint32_t h) { float f; std::memcpy(&f, &h, 4); return f; }
uint32_t bits(float f) { uint32_t h; std::memcpy(&h, &f, 4); return h; }
uint32_t sb(float f) { return bits(f) >> 31; }

// MB86235 CFI: float -> int by truncation
uint32_t cfi(float x) { return uint32_t(int64_t(double(x))); }

const float kIdentity[12] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};

// start-up constants (A-RAM 0x90..): 1 / pi, pi in two parts, the small-argument limit 2^-20, the odd polynomial
// r^13 .. r^3
const uint32_t kSerBits[11] = {0x3EA2F983, 0x40491000, 0xB715777A, 0x35800000, 0xAB4F7739, 0x2F3072AB, 0xB2D731A6,
                               0x3638EF1C, 0xB9500D01, 0x3C088889, 0xBE2AAAAB};
// arctangent constants (A-RAM 0x9B..): offsets 0, pi/6, pi/2, pi/3; tan 15; sqrt 3; 2^-12; p1, p0, q1, q0; pi
const uint32_t kAtBits[12] = {0x00000000, 0x3F060A92, 0x3FC90FDB, 0x3F860A92, 0x3E8930A3, 0x3FDDB3D7, 0x39800000,
                              0xBF3853AE, 0xBFB854A8, 0x4098123C, 0x408A3F7E, 0x40490FDB};
float ser(int i) { return hf(kSerBits[i]); }
float at(int i) { return hf(kAtBits[i]); }
const uint32_t kHalfPi = 0x3FC90FDB, kStep = 0x38C90FDB;   // pi / 2; 2 pi / 65536
const uint32_t kTiny = 0x993C56F6;                          // A-RAM 0x238
const uint32_t kAngle = 0x4622F83D;                         // 65536 / 2 pi

struct Tables {
    float coarse[129], fsin[128], fcos[128];
    Tables()
    {
        // start-up loop 0x238
        for (int i = 0; i < 128; ++i) {
            coarse[i] = Tgp::sin_series(float(i * 128) * hf(kStep), false);
            fsin[i] = Tgp::sin_series(float(i) * hf(kStep), false);
            fcos[i] = Tgp::sin_series(float(i) * hf(kStep), true);
        }
        coarse[128] = fcos[0];
    }
};
const Tables& tables() { static const Tables t; return t; }

// 1 / x as routines 0x542 / 0x90D / 0x4EF write it
float rcp2(float x)
{
    float r = Tgp::frcp(x);
    float t = x * r;
    float e = 2.0f - t;
    float t1 = e * t;
    return (2.0f - t1) * (r * e);
}

} // namespace

void Tgp::Out::putf(float f, bool known) { put(bits(f), known); }

Tgp::Tgp(const uint32_t* data, size_t words) : data_(data), words_(words) {}

float Tgp::frsq(float x) { return 1.0f / std::sqrt(x); }
float Tgp::frcp(float x) { return 1.0f / x; }

float Tgp::sqrt_1d9(float x)
{
    if (x == 0.0f)
        return 0.0f;
    float y = frsq(x);
    for (int i = 0; i < 2; ++i) {
        float a = x * (y * y);
        y = (3.0f - a) * (y * 0.5f);
    }
    return x * y;
}

float Tgp::distance_12d(const float* p)
{
    float d0 = p[3] - p[0], d1 = p[4] - p[1], d2 = p[5] - p[2];
    return sqrt_1d9((d0 * d0 + d1 * d1) + d2 * d2);
}

void Tgp::normalize_75b(const float* v, float* out)
{
    float s = (v[0] * v[0] + v[1] * v[1]) + v[2] * v[2];
    float r = frsq(s);
    for (int i = 0; i < 2; ++i) {
        float a = s * (r * r);
        r = (3.0f - a) * (r * 0.5f);
    }
    for (int k = 0; k < 3; ++k)
        out[k] = v[k] * r;
}

float Tgp::sin_series(float x, bool cos)
{
    float ax = std::fabs(x);
    float sign = 1.0f, q, off;
    if (cos) {
        q = ax + hf(kHalfPi);
        off = 0.5f;
    } else {
        q = ax;
        off = 0.0f;
        if (x < 0)
            sign = -1.0f;
    }
    int64_t n = int64_t(double(q * ser(0)));
    if (n & 1)
        sign = sign * -1.0f;
    float m = float(n) - off;
    float r = (ax - ser(1) * m) - ser(2) * m;
    if (std::fabs(r) < ser(3))
        return r * sign;
    float r2 = r * r;
    float p = ser(4);
    for (int i = 5; i < 11; ++i)
        p = r2 * p + ser(i);
    return (((r2 * p) * r) + r) * sign;
}

void Tgp::sincos_21d(uint32_t angle, float& so, float& co)
{
    const Tables& t = tables();
    uint32_t a = angle & 0xFFFF;
    uint32_t i = a & 0x7F, j = (a >> 7) & 0x7F, q = a >> 14;
    float sF = t.fsin[i], cF = t.fcos[i], sC = t.coarse[j], cC = t.coarse[128 - j];
    float s = cF * sC + sF * cC;
    float c = cF * cC - sF * sC;
    switch (q) {
    case 0: so = s; co = c; break;
    case 1: so = c; co = -s; break;
    case 2: so = -s; co = -c; break;
    default: so = -c; co = s; break;
    }
}

float Tgp::div_1cd(float a, float b)
{
    float r = frcp(b);
    float t = b * r;
    float e1 = 2.0f - t;
    float q = (r * a) * e1;
    float t1 = e1 * t;
    float e2 = 2.0f - t1;
    return e2 * q;
}

float Tgp::div_1cd_ma0(float, float b)
{
    float r = frcp(b);
    float t = b * r;
    float e1 = 2.0f - t;
    return 2.0f - e1 * t;
}

float Tgp::rcp3_342(float x)
{
    float r = frcp(x);
    float t = x * r;
    float e = 2.0f - t;
    for (int i = 0; i < 2; ++i) {
        r = e * r;
        t = e * t;
        e = 2.0f - t;
    }
    return e * r;
}

float Tgp::atan2_31a(float num, float den, float ma0, bool ma0Known, bool& known)
{
    known = true;
    if (std::fabs(den) < std::fabs(hf(kTiny))) {
        if (num == 0.0f) {
            known = ma0Known;   // 0 / 0: MA0 unchanged
            return ma0;
        }
        float a = at(2);
        return (0.0f - num) > 0 ? -a : a;
    }
    float q = std::fabs(div_1cd(num, den));
    int n = 0;
    if (1.0f - q < 0) {
        q = rcp3_342(q);
        n = 2;
    }
    if (q - at(4) >= 0) {
        n += 1;
        q = div_1cd(q * at(5) + -1.0f, q + at(5));
    }
    float f = q;
    if (std::fabs(at(6)) - std::fabs(f) < 0) {
        float g = f * f;
        float p = (g * at(7) + at(8)) * g;
        float d = (g + at(9)) * g + at(10);
        float r = div_1cd(p, d);
        f = r * f + f;
    }
    if (n >= 2)
        f = f * -1.0f;
    float a = f + at(n);
    if (!(0.0f - den < 0))
        a = at(11) - a;
    if (num - 0.0f < 0)
        a = a * -1.0f;
    return a;
}

void Tgp::angles_713(float x, float y, float z, float ma0, bool ma0Known, bool fix,
                     uint32_t& yaw, bool& yawKnown, uint32_t& pitch)
{
    float nx = 0.0f - x;
    bool known;
    float a = atan2_31a(nx, z, ma0, ma0Known, known);
    float h, m;
    yaw = cfi(a * hf(kAngle));
    yawKnown = known;
    if (!known) {
        // x = z = 0: h = +-0 whatever the yaw; MA0 after 0 / (cos or sin) is 1.0 for any normal divisor
        h = 0.0f;
        m = 1.0f;
    } else {
        float s, c;
        sincos_21d(yaw, s, c);
        if (((yaw + 0x2000) & 0x4000) == 0) {
            h = div_1cd(z, c);
            m = div_1cd_ma0(z, c);
        } else {
            h = div_1cd(nx, s);
            m = div_1cd_ma0(nx, s);
        }
    }
    a = atan2_31a(y, h, m, true, known);
    pitch = cfi(a * hf(kAngle));
    if (fix && (int32_t(pitch + pitch) ^ int32_t(pitch)) < 0) {
        yaw = yaw + 0x8000;
        pitch = 0x8000 - pitch;
    }
}

float Tgp::rsq_6a0(float x)
{
    float y = frsq(x);
    for (int i = 0; i < 2; ++i)
        y = (3.0f - x * (y * y)) * (y * 0.5f);
    return y;
}

float Tgp::bget(int a) const { return a >= 0 && a < int(b_.size()) ? b_[a] : 0.0f; }
void Tgp::bset(int a, float v) { if (a >= 0 && a < int(b_.size())) b_[a] = v; }

void Tgp::push()
{
    if (depth_ - 15 < 0) {
        for (int k = 0; k < 12; ++k)
            bset(sp_ + k, pr_[k]);
        sp_ += 12;
    }
    depth_ += 1;
    ma0_ = ma0_ * pr_[0];
}

void Tgp::pop()
{
    if (0 - depth_ >= 0)
        return;
    depth_ -= 1;
    if (15 - (depth_ + 1) < 0)
        return;
    sp_ -= 12;
    for (int k = 0; k < 12; ++k)
        pr_[k] = bget(sp_ + k);
}

void Tgp::mul(const float* n, const float* m, float* out)
{
    for (int i = 0; i < 4; ++i) {
        float a = n[3 * i], b = n[3 * i + 1], c = n[3 * i + 2];
        for (int k = 0; k < 3; ++k) {
            float v = (a * m[k] + b * m[3 + k]) + c * m[6 + k];
            if (i == 3)
                v = v + m[9 + k];
            out[3 * i + k] = v;
        }
    }
}

void Tgp::rot(uint32_t angle, int r1, int r2, int sign)
{
    float s, c;
    sincos_21d(angle, s, c);
    ma0_ = c;
    ma0Known_ = true;
    for (int k = 0; k < 3; ++k) {
        float a = pr_[3 * r1 + k], b = pr_[3 * r2 + k];
        if (sign > 0) {
            pr_[3 * r1 + k] = c * a - s * b;
            pr_[3 * r2 + k] = s * a + c * b;
        } else {
            pr_[3 * r1 + k] = c * a + s * b;
            pr_[3 * r2 + k] = c * b - s * a;
        }
    }
}

void Tgp::xform(float x, float y, float z, float* v) const
{
    const float* m = pr_;
    for (int k = 0; k < 3; ++k)
        v[k] = ((x * m[k] + y * m[3 + k]) + z * m[6 + k]) + m[9 + k];
}

void Tgp::inverse()
{
    const float a = pr_[0], b = pr_[1], c = pr_[2], d = pr_[3], e = pr_[4], f = pr_[5], g = pr_[6], h = pr_[7],
                i = pr_[8], tx = pr_[9], ty = pr_[10], tz = pr_[11];
    auto dif = [](float p, float q) { return p - q; };
    float ab0 = dif(b * tz, ty * c), ab1 = dif(e * tz, ty * f), ab2 = dif(h * tz, ty * i);
    float ab3 = dif(e * i, h * f), ab4 = dif(h * c, b * i), ab5 = dif(b * f, e * c);
    float det = (a * ab3 + d * ab4) + g * ab5;
    float r = frcp(det);
    float t = det * r;
    float e1 = 2.0f - t;
    float t1 = e1 * t;
    float inv = (2.0f - t1) * (r * e1);
    float aedb = dif(a * e, d * b);
    float out[12] = {ab3 * inv, ab4 * inv, ab5 * inv,
                     dif(g * f, d * i) * inv, dif(a * i, g * c) * inv, dif(d * c, a * f) * inv,
                     dif(d * h, g * e) * inv, dif(g * b, a * h) * inv, aedb * inv,
                     dif(dif(g * ab1, d * ab2), tx * ab3) * inv,
                     dif(dif(a * ab2, g * ab0), tx * ab4) * inv,
                     dif(dif(d * ab0, a * ab1), tx * ab5) * inv};
    std::memcpy(pr_, out, sizeof out);
    ma0_ = aedb;
    ma0Known_ = true;
}

float Tgp::sphereVisible(float x, float y, float z, float rad)
{
    push();
    for (int k = 0; k < 12; ++k)
        pr_[k] = bget(0xF8 + k);
    float v[3];
    xform(x, y, z, v);
    float res;
    if (0.0f - v[2] < 0) {
        float a = bget(2);
        float r = frcp(a);
        float t = a * r;
        float e1 = 2.0f - t;
        float t1 = e1 * t;
        float ia = (2.0f - t1) * (r * e1);
        float w = rad * bget(4) + v[2];
        float aa3 = (248.0f * w) * ia;
        float aa4 = (192.0f * w) * ia;
        float n0 = v[0] * -1.0f, n1 = v[1] * -1.0f;
        uint32_t in0 = sb(aa3 + n0) ^ sb(n0 - aa3);
        uint32_t in1 = sb(aa4 + n1) ^ sb(n1 - aa4);
        res = (in0 & in1) ? 1.0f : -1.0f;
    } else {
        float q = rad * bget(5);
        float d2 = (v[0] * v[0] + v[1] * v[1]) + v[2] * v[2];
        res = d2 - q * q < 0 ? 1.0f : -1.0f;
    }
    ma0_ = v[0];
    ma0Known_ = true;
    pop();
    return res;
}

void Tgp::matrixAngles(bool xForward, Out& out)
{
    push();
    float m[12];
    std::memcpy(m, pr_, sizeof m);
    uint32_t yaw, pitch;
    bool yawKnown;
    if (!xForward)
        angles_713(m[6], m[7], m[8], ma0_, ma0Known_, true, yaw, yawKnown, pitch);
    else
        angles_713(m[1], m[2], m[0], ma0_, ma0Known_, false, yaw, yawKnown, pitch);
    if (!yawKnown) {
        pop();
        ma0Known_ = false;
        out.put(0, false);
        out.put(pitch);
        out.put(0, false);
        return;
    }
    std::memcpy(pr_, kIdentity, sizeof pr_);
    float v[3];
    float a;
    bool known;
    if (!xForward) {
        rot(0 - pitch, 1, 2, +1);
        rot(0 - yaw, 0, 2, -1);
        xform(m[0], m[1], m[2], v);
        a = atan2_31a(v[1] * -1.0f, v[0], ma0_, ma0Known_, known);
    } else {
        rot(0 - pitch, 0, 2, -1);
        rot(0 - yaw, 0, 1, +1);
        xform(m[3], m[4], m[5], v);
        a = atan2_31a(v[2] * -1.0f, v[1], ma0_, ma0Known_, known);
    }
    uint32_t roll = cfi(a * hf(kAngle));
    ma0Known_ = false;
    pop();
    out.put(yaw);
    out.put(pitch);
    out.put(roll, known);
}

void Tgp::rayTest(int k, const float* p, const float* d, Out& out) const
{
    uint32_t ptr = rom(0x100 + k);
    uint32_t cnt = rom(ptr);
    uint32_t eo = ptr + 7;
    float best = FLT_MAX;
    bool found = false;
    float fh[3] = {}, fn[3] = {};
    uint32_t fattr = 0;
    for (uint32_t pi = 0; pi < cnt; ++pi) {
        float nx = hf(rom(eo)), ny = hf(rom(eo + 1)), nz = hf(rom(eo + 2)), dd = hf(rom(eo + 3));
        eo += 4;
        float nd = (nx * d[0] + ny * d[1]) + nz * d[2];
        bool hit = false;
        float h[3], s = 0;
        if (!(hf(0x38D1B717) - std::fabs(nd) >= 0)) {
            float npd = ((nx * p[0] + ny * p[1]) + nz * p[2]) + dd;
            float t = div_1cd(npd, nd);
            if (t < 0) {
                s = 0.0f - t;
                if (s - best < 0) {
                    hit = true;
                    for (int j = 0; j < 3; ++j)
                        h[j] = d[j] * s + p[j];
                }
            }
        }
        if (!hit) {
            eo += 14;
            continue;
        }
        uint32_t q = eo + 1;
        uint32_t typ = rom(eo);
        float v[4][3];
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 3; ++j)
                v[i][j] = hf(rom(q + 3 * i + j));
        int ax0, ax1;
        float h0, h1;
        bool rev;
        if (typ == 0) {
            ax0 = 1; ax1 = 2; h0 = h[1]; h1 = h[2]; rev = sb(nx) == 0;
        } else if (typ == 1) {
            ax0 = 0; ax1 = 2; h0 = h[0]; h1 = h[2]; rev = sb(ny) == 1;
        } else {
            ax0 = 0; ax1 = 1; h0 = h[0]; h1 = h[1]; rev = sb(nz) == 0;
        }
        static const int kFwd[4] = {0, 1, 2, 3}, kRev[4] = {0, 3, 2, 1};
        const int* order = rev ? kRev : kFwd;
        float rel[4][2];
        for (int i = 0; i < 4; ++i) {
            rel[i][0] = v[order[i]][ax0] - h0;
            rel[i][1] = v[order[i]][ax1] - h1;
        }
        uint32_t signs = 0;
        for (int i = 0; i < 4; ++i) {
            const float* a = rel[i];
            const float* b = rel[(i + 1) % 4];
            signs |= sb(a[0] * b[1] - b[0] * a[1]);
        }
        uint32_t attr = rom(q + 12);
        eo += 14;
        if (signs == 0) {
            best = s;
            found = true;
            std::memcpy(fh, h, sizeof fh);
            fn[0] = nx; fn[1] = ny; fn[2] = nz;
            fattr = attr;
        }
    }
    if (!found) {
        out.putf(-1.0f);
        return;
    }
    out.putf(1.0f);
    for (float f : fh)
        out.putf(f);
    out.putf(best * best);
    for (float f : fn)
        out.putf(f);
    out.put(fattr);
}

float Tgp::curve(uint32_t c, float t) const
{
    uint32_t w = rom(c);
    uint32_t hi = w >> 16, lo = w & 0xFFFF;
    int64_t pos = int64_t(c) + 1 + 2 * lo, step = lo, cnt = int64_t(hi) - 1;
    for (;;) {
        float key = hf(rom(uint32_t(pos)));
        int64_t s = step;
        step >>= 1;
        if (t - key < 0)
            s = -s;
        cnt -= 1;
        pos += s;
        if (cnt == 0)
            break;
    }
    if (t - hf(rom(uint32_t(pos))) < 0)
        pos -= step << 1;
    uint32_t ps = uint32_t(pos);
    float t0 = hf(rom(ps)), t1 = hf(rom(ps + 4));
    float v0 = hf(rom(ps + 1)), v1 = hf(rom(ps + 5));
    float m0 = hf(rom(ps + 2)), m1 = hf(rom(ps + 7));
    float dt = t1 - t0;
    float u = t - t0;
    float r1 = rcp2(dt);
    float dt2 = dt * dt;
    float r2 = rcp2(dt2);
    float r3 = rcp2(dt2 * dt);
    float u2 = u * u;
    float u3 = u * u2;
    float a2 = (r3 * u3) * 2.0f;
    float a4 = (r2 * u2) * 3.0f;
    float h00 = (a2 - a4) + 1.0f;
    float h01 = a4 - a2;
    float a3 = r2 * u3;
    float a1 = r1 * u2;
    float h10 = ((a3 - a1) - a1) + u;
    float h11 = a3 - a1;
    return ((h01 * v1 + h00 * v0) + h10 * m0) + h11 * m1;
}

void Tgp::pathTable(uint32_t i, float t, Out& out) const
{
    uint32_t base = rom(i);
    for (uint32_t j = 0; j < 8; ++j) {
        float v = curve(rom(base + j), t);
        if (j == 6)
            out.put(cfi(v) & 0xFFFF);
        else
            out.putf(v);
    }
}

bool Tgp::call(int n, const uint32_t* in, int count, Out& out)
{
    float fl[16] = {};
    for (int k = 0; k < count && k < 16; ++k)
        fl[k] = hf(in[k]);
    out = Out();
    switch (n) {
    case 0x05: push(); return true;
    case 0x06: pop(); return true;
    case 0x07: std::memcpy(pr_, fl, sizeof pr_); return true;
    case 0x08: sp_ = 0x20; depth_ = 0; return true;
    case 0x09: {
        float r[12];
        mul(fl, pr_, r);
        std::memcpy(pr_, r, sizeof r);
        ma0Known_ = false;
        return true;
    }
    case 0x0A: {
        bool known;
        float a = atan2_31a(fl[1], fl[0], ma0_, ma0Known_, known);
        out.put(cfi(a * hf(kAngle)), known);
        return true;
    }
    case 0x0C: {
        float v[3];
        normalize_75b(fl, v);
        for (float f : v)
            out.putf(f);
        ma0Known_ = false;
        return true;
    }
    case 0x0F: {
        uint32_t yaw, pitch;
        bool known;
        angles_713(fl[0], fl[1], fl[2], ma0_, ma0Known_, true, yaw, known, pitch);
        out.put(yaw, known);
        out.put(pitch);
        return true;
    }
    case 0x83: {
        // the vector's angles about z then y (held objects, kind 32): 0x0F's routine with the inputs (y, z, x):
        // atan2(-y, x), then atan2(z, |xy|) (checked against 1963 logged calls, 2026-10-15; the pitch never needs
        // the final fix, |xy| > 0)
        uint32_t yaw, pitch;
        bool known;
        angles_713(fl[1], fl[2], fl[0], ma0_, ma0Known_, true, yaw, known, pitch);
        out.put(yaw, known);
        out.put(pitch);
        return true;
    }
    case 0x84: {
        // 0x60B (the iron ball's chain links, worker "ch3own"): the length of (x, y, z) (s x rsq_6a0(s); 0: x),
        // then 0x83's angles (0x73A) with MA0 = that length; outputs angle, angle, length. Proven (hotd_proof tgp, ch4p_full: 22,465 calls)
        const float s = (fl[1] * fl[1] + fl[0] * fl[0]) + fl[2] * fl[2];
        const float len = s == 0.0f ? fl[0] : s * rsq_6a0(s);
        uint32_t yaw, pitch;
        bool known;
        angles_713(fl[1], fl[2], fl[0], len, true, true, yaw, known, pitch);
        out.put(yaw, known);
        out.put(pitch);
        out.putf(len);
        ma0Known_ = false;
        return true;
    }
    case 0x52: {
        // 0x151 (the chain's set-up, worker "ch3own"): (x, y, z) -> yaw = atan2(-x, z), h = its horizontal length,
        // pitch = atan2(y, h) (as 0x713 without the fix), then the length (h / cos or y / sin of the pitch by its
        // octant). Proven (hotd_proof tgp, ch4p_full: 8 calls)
        const float nx = 0.0f - fl[0];
        bool known;
        float a = atan2_31a(nx, fl[2], ma0_, ma0Known_, known);
        const uint32_t yaw = cfi(a * hf(kAngle));
        float h, m;
        if (!known) {
            h = 0.0f;
            m = 1.0f;
        } else {
            float s, c;
            sincos_21d(yaw, s, c);
            if (((yaw + 0x2000) & 0x4000) == 0) {
                h = div_1cd(fl[2], c);
                m = div_1cd_ma0(fl[2], c);
            } else {
                h = div_1cd(nx, s);
                m = div_1cd_ma0(nx, s);
            }
        }
        bool k2;
        a = atan2_31a(fl[1], h, m, true, k2);
        const uint32_t pitch = cfi(a * hf(kAngle));
        float s2, c2;
        sincos_21d(pitch, s2, c2);
        const float len = ((pitch + 0x2000) & 0x4000) == 0 ? div_1cd(h, c2) : div_1cd(fl[1], s2);
        out.put(yaw, known);
        out.put(pitch, k2);
        out.putf(len);
        ma0Known_ = false;
        return true;
    }
    case 0x10: std::memcpy(pr_, kIdentity, sizeof pr_); return true;
    case 0x11:
        for (float f : pr_)
            out.putf(f);
        return true;
    case 0x12: {
        float v[3];
        xform(fl[0], fl[1], fl[2], v);
        std::memcpy(pr_ + 9, v, sizeof v);
        ma0_ = fl[2];
        ma0Known_ = true;
        return true;
    }
    case 0x13:
        for (int k = 0; k < 9; ++k)
            pr_[k] = pr_[k] * fl[k / 3];
        ma0_ = fl[2];
        ma0Known_ = true;
        return true;
    case 0x14: rot(in[0], 1, 2, +1); return true;
    case 0x15: rot(in[0], 0, 2, -1); return true;
    case 0x16: rot(in[0], 0, 1, +1); return true;
    case 0x19: out.put(uint32_t(depth_)); return true;
    case 0x1A: {
        float v[3];
        xform(fl[0], fl[1], fl[2], v);
        ma0_ = fl[2];
        ma0Known_ = true;
        for (float f : v)
            out.putf(f);
        return true;
    }
    case 0x1B:
    case 0x1C: {
        float s, c;
        sincos_21d(in[0], s, c);
        ma0_ = c;
        ma0Known_ = true;
        out.putf(n == 0x1B ? s : c);
        return true;
    }
    case 0x1D: {
        float s, c;
        sincos_21d(in[0], s, c);
        ma0_ = fl[1];
        ma0Known_ = true;
        out.putf(fl[1] * s);
        return true;
    }
    case 0x1E: {
        float s, c;
        sincos_21d(in[0], s, c);
        ma0_ = c * fl[1];
        ma0Known_ = true;
        out.putf(ma0_);
        return true;
    }
    case 0x1F:
        ma0Known_ = false;
        out.putf(distance_12d(fl));
        return true;
    case 0x2D:
        // 0x144: the square root routine 0x1D9 of one input (dog_set3_fall)
        ma0Known_ = false;
        out.putf(sqrt_1d9(fl[0]));
        return true;
    case 0x20:
        for (int k = 9; k < 12; ++k)
            out.putf(pr_[k]);
        return true;
    case 0x21: push(); std::memcpy(pr_, kIdentity, sizeof pr_); return true;
    case 0x62:
        ma0Known_ = false;
        rayTest(int(in[0]), fl + 1, fl + 4, out);
        return true;
    case 0x6E:
        ma0Known_ = false;
        pathTable(in[0], fl[1], out);
        return true;
    case 0x6F: matrixAngles(false, out); return true;
    case 0x71: matrixAngles(true, out); return true;
    case 0x73: case 0x74: case 0x75: case 0x76: case 0x77: {
        int slot = 0xF8 + 12 * int(in[0]);
        ma0Known_ = false;
        if (n == 0x74)
            push();
        float s[12];
        for (int k = 0; k < 12; ++k)
            s[k] = bget(slot + k);
        if (n == 0x73 || n == 0x74) {
            std::memcpy(pr_, s, sizeof pr_);
        } else if (n == 0x75) {
            for (int k = 0; k < 12; ++k)
                bset(slot + k, pr_[k]);
        } else {
            float r[12];
            // MA0 afterwards (program 0x6B8 / 0x6C0): the multiply loads the left matrix's words into MA0 / MA1 / MA2
            // in turn, the last into MA0 is word 9 (0x76: the stored matrix, 0x77: the current one)
            const float last = n == 0x76 ? s[9] : pr_[9];
            if (n == 0x76)
                mul(s, pr_, r);
            else
                mul(pr_, s, r);
            std::memcpy(pr_, r, sizeof r);
            ma0_ = last;
            ma0Known_ = true;
            return true;
        }
        // 0x73 / 0x74 / 0x75 (program 0x4B1 / 0x4BA): MA0 = the input word (the slot number, raw)
        std::memcpy(&ma0_, &in[0], 4);
        ma0Known_ = true;
        return true;
    }
    case 0x7A: inverse(); return true;
    case 0x7B: out.putf(sphereVisible(fl[0], fl[1], fl[2], fl[3])); return true;
    case 0x7D: {
        // 0x5A3: projection constants B[2] = a, B[3] = b, B[4] = sqrt(248^2 + a^2) / 248, B[5] = that / a
        float a = fl[0];
        float s = sqrt_1d9(61504.0f + a * a);
        bset(2, a);
        bset(3, fl[1]);
        bset(4, div_1cd(s, 248.0f));
        bset(5, div_1cd(s, a));
        ma0_ = a;
        ma0Known_ = true;
        return true;
    }
    case 0x7F: push(); std::memcpy(pr_, fl, sizeof pr_); ma0Known_ = false; return true;
    case 0x86:
        // 0x645: rotate Z (as 0x16), then Y (as 0x15), then X (as 0x14)
        rot(in[0], 0, 1, +1);
        rot(in[1], 0, 2, -1);
        rot(in[2], 1, 2, +1);
        return true;
    case 0x87: std::memcpy(pr_, fl, 9 * sizeof(float)); return true;
    case 0x88: std::memcpy(pr_ + 9, fl, 3 * sizeof(float)); return true;
    case 0x89: {
        // 0x692: cosine of the angle between a and b
        const float* a = fl;
        const float* b = fl + 3;
        float dot = (a[0] * b[0] + a[1] * b[1]) + a[2] * b[2];
        float ra = rsq_6a0((a[0] * a[0] + a[1] * a[1]) + a[2] * a[2]);
        float rb = rsq_6a0((b[0] * b[0] + b[1] * b[1]) + b[2] * b[2]);
        ma0_ = ra;
        ma0Known_ = true;
        out.putf((dot * ra) * rb);
        return true;
    }
    default:
        return false;
    }
}

} // namespace hotd::port
