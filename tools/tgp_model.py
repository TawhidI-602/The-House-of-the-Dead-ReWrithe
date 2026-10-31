#!/usr/bin/env python3
"""
tgp_model.py - bit-exact models of the arcade's geometry coprocessor functions (Model 2C: Fujitsu MB86235 "TGP"),
for the A (arcade) behaviour. The i960 uploads the coprocessor's program at boot (copro_program_load 0xA36A0: 0x2000
words from ROM 0x2F48000 = main_data +0xF48000, two words per 64-bit instruction); `unidasm -arch mb86235` disassembles
it (private/copro/tgp_program.dis, ROM-derived: not committed). The dispatcher (0x198) checks the three copies of the
function number in the command word and calls the routine whose address the start-up code (0x250..) stored at A-RAM
[function]: private/copro/tgp_functions.txt.

Every model here is checked against MAME (tools/mame_copro_probe.lua logs a function's FIFO inputs / outputs):
  py tools/tgp_model.py check <probe log>          compares every modelled function in the log, bit for bit

MB86235 notes used: FSUB a, b, d = b - a; parallel operations in one instruction read the old register values;
the instruction after a DCALL / DRET runs before the jump (delay slot). FRSQ x = 1.0f / sqrtf(x) in float32 (as MAME
emulates it: verified on 5000 calls of function 0x1F, 2026-10-13). Real hardware's FRSQ is a table seed and may differ:
"A" here means the arcade as MAME runs it.
"""
import struct, sys
import numpy as np

f32 = np.float32


def frsq(x):
    """MB86235 FRSQ as MAME runs it"""
    return f32(f32(1.0) / np.sqrt(f32(x)))


def tgp_sqrt(x):
    """coprocessor routine 0x1D9: 0 stays 0; else FRSQ seed, two Newton steps y = (3 - x*y*y) * (0.5*y), result x*y"""
    x = f32(x)
    if x == 0:
        return f32(0)
    y = frsq(x)
    for _ in range(2):
        a = f32(x * f32(y * y))
        y = f32(f32(f32(3.0) - a) * f32(y * f32(0.5)))
    return f32(x * y)


def f1f_distance(x1, y1, z1, x2, y2, z2):
    """function 0x1F (code 0x12D): distance of two points; sum order (dx^2 + dy^2) + dz^2, d = second - first.
    The game passes y = 0 for both points (82 call sites: a distance on the floor plane)."""
    d0, d1, d2 = f32(f32(x2) - f32(x1)), f32(f32(y2) - f32(y1)), f32(f32(z2) - f32(z1))
    return tgp_sqrt(f32(f32(f32(d0 * d0) + f32(d1 * d1)) + f32(d2 * d2)))


def f0c_normalize(x, y, z):
    """function 0x0C (code 0x085 -> 0x75B): unit vector; s = (x*x + y*y) + z*z, r = FRSQ seed + two Newton steps
    (as tgp_sqrt, without the final multiply), result (x*r, y*r, z*r). Checked: 1272 of 1272 attract-loop calls."""
    x, y, z = f32(x), f32(y), f32(z)
    s = f32(f32(f32(x * x) + f32(y * y)) + f32(z * z))
    r = frsq(s)
    for _ in range(2):
        a = f32(s * f32(r * r))
        r = f32(f32(f32(3.0) - a) * f32(r * f32(0.5)))
    return f32(x * r), f32(y * r), f32(z * r)


def _hf(h):
    return f32(struct.unpack("<f", struct.pack("<I", h))[0])


# start-up constants (A-RAM 0x90.., written by immediates at 0x2E1): 1 / pi, pi in two parts (Cody-Waite), the
# "small argument" limit 2^-20, then the odd polynomial's coefficients from r^13 down to r^3
_SER = [_hf(h) for h in (0x3EA2F983, 0x40491000, 0xB715777A, 0x35800000, 0xAB4F7739, 0x2F3072AB, 0xB2D731A6,
                         0x3638EF1C, 0xB9500D01, 0x3C088889, 0xBE2AAAAB)]
_HALF_PI = _hf(0x3FC90FDB)
_STEP = _hf(0x38C90FDB)   # 2 pi / 65536: one angle unit in radians
_tables = None


def tgp_sin_series(x, cos=False):
    """routines 0x1F6 (sin x) / 0x1F0 (cos x = sin(|x| + pi / 2) with the period shifted by a half): float32 x in
    radians; n = (|x| (+ pi / 2)) / pi converted by CFI = **truncation**, odd n flips the sign (sin: also x < 0);
    r = (|x| - pi_hi * m) - pi_lo * m with m = n (- 0.5 for cos); |r| < 2^-20 -> r; else r + (r^2 * P(r^2)) * r."""
    x = f32(x)
    ax = f32(abs(x))
    sign = f32(1.0)
    if cos:
        q, off = f32(ax + _HALF_PI), f32(0.5)
    else:
        q, off = ax, f32(0.0)
        if x < 0:
            sign = f32(-1.0)
    n = int(float(f32(q * _SER[0])))
    if n & 1:
        sign = f32(sign * f32(-1.0))
    m = f32(f32(n) - off)
    r = f32(f32(ax - f32(_SER[1] * m)) - f32(_SER[2] * m))
    if abs(r) < _SER[3]:
        return f32(r * sign)
    r2 = f32(r * r)
    p = _SER[4]
    for c in _SER[5:]:
        p = f32(f32(r2 * p) + c)
    return f32(f32(f32(f32(r2 * p) * r) + r) * sign)


def _build_tables():
    """start-up loop 0x238: coarse sin(i x 128 units) at A-RAM 0xAD, fine cos(i units) at 0x12D (so the coarse
    table's entry 128 is cos(0)), fine sin(i units) at 0x1AD, i = 0..127"""
    global _tables
    if _tables is None:
        coarse = [tgp_sin_series(f32(f32(i * 128) * _STEP)) for i in range(128)]
        fsin = [tgp_sin_series(f32(f32(i) * _STEP)) for i in range(128)]
        fcos = [tgp_sin_series(f32(f32(i) * _STEP), cos=True) for i in range(128)]
        _tables = (coarse + fcos[:1], fsin, fcos)
    return _tables


def tgp_sincos(angle):
    """routine 0x21D: (sin, cos) of a 16-bit angle (0x10000 = 360 degrees). a = angle & 0xFFFF: fine i = a & 0x7F,
    coarse j = (a >> 7) & 0x7F, quadrant a >> 14; sC = coarse[j], cC = coarse[128 - j], sF / cF the fine pair;
    sin' = cF*sC + sF*cC, cos' = cF*cC - sF*sC (each product rounded to float32); quadrant 1: (cos', -sin'),
    2: (-sin', -cos'), 3: (-cos', sin'). Checked: 532 / 532 calls of each. Note cos(0) = 0x3F7FFFFE (0.99999988)."""
    coarse, fsin, fcos = _build_tables()
    a = angle & 0xFFFF
    i, j, q = a & 0x7F, (a >> 7) & 0x7F, a >> 14
    sF, cF, sC, cC = fsin[i], fcos[i], coarse[j], coarse[128 - j]
    s = f32(f32(cF * sC) + f32(sF * cC))
    c = f32(f32(cF * cC) - f32(sF * sC))
    if q == 0:
        return s, c
    if q == 1:
        return c, f32(-s)
    if q == 2:
        return f32(-s), f32(-c)
    return f32(-c), s


def f1b_sin(angle):
    """function 0x1B (code 0x11B): sin of a 16-bit angle"""
    return tgp_sincos(angle)[0]


def f1c_cos(angle):
    """function 0x1C (code 0x11F): cos of a 16-bit angle"""
    return tgp_sincos(angle)[1]


def frcp(x):
    """MB86235 FRCP as MAME runs it (assumed 1.0f / x, checked through every function that divides)"""
    return f32(f32(1.0) / f32(x))


def tgp_div(a, b):
    """routine 0x1CD (and 0x1D3, same maths, result in MA0): a / b as FRCP seed r + two Newton steps written in the
    program's order: t = b*r, e1 = 2 - t, q = (r*a)*e1, t1 = e1*t, e2 = 2 - t1, result e2*q"""
    a, b = f32(a), f32(b)
    r = frcp(b)
    t = f32(b * r)
    e1 = f32(f32(2.0) - t)
    q = f32(f32(r * a) * e1)
    t1 = f32(e1 * t)
    e2 = f32(f32(2.0) - t1)
    return f32(e2 * q)


def tgp_div_ma0_after(a, b):
    """what routine 0x1CD leaves in MA0: e2 (about 1.0); read back by an arctangent of 0 / 0"""
    a, b = f32(a), f32(b)
    r = frcp(b)
    t = f32(b * r)
    e1 = f32(f32(2.0) - t)
    return f32(f32(2.0) - f32(e1 * t))


def _rcp3(x):
    """routine 0x342: 1 / x, FRCP seed + three Newton steps: t = x*r, e1 = 2 - t, r1 = e1*r, t1 = e1*t, e2 = 2 - t1,
    r2 = e2*r1, t2 = e2*t1, e3 = 2 - t2, result e3*r2"""
    r = frcp(x)
    t = f32(x * r)
    e = f32(f32(2.0) - t)
    for _ in range(2):
        r = f32(e * r)
        t = f32(e * t)
        e = f32(f32(2.0) - t)
    return f32(e * r)


# arctangent constants (A-RAM 0x9B.., immediates at 0x2EE): offsets 0, pi/6, pi/2, pi/3; tan 15 deg; sqrt 3; the
# "small argument" limit 2^-12; Cody & Waite's rational coefficients p1, p0, q1, q0; pi
_AT = [_hf(h) for h in (0x00000000, 0x3F060A92, 0x3FC90FDB, 0x3F860A92, 0x3E8930A3, 0x3FDDB3D7, 0x39800000,
                        0xBF3853AE, 0xBFB854A8, 0x4098123C, 0x408A3F7E, 0x40490FDB)]
_TINY = _hf(0x993C56F6)   # A-RAM 0x238: |denominator| below this counts as 0
_ANGLE = _hf(0x4622F83D)  # 65536 / 2 pi


def tgp_atan2(num, den, ma0=None):
    """routine 0x31A: atan2(num, den) in radians, Cody & Waite style. |den| < |_TINY| -> +-pi/2 (num = 0: see below);
    q = |num / den| (0x320: FRCP + two Newton steps in the 1CD order); q > 1 -> q = 1/q (0x342), n = 2;
    q >= tan 15 -> f = (q*sqrt3 - 1) / (q + sqrt3) (0x1CD), n + 1; |f| > 2^-12 -> f = f + f * (g*(p1*g + p0)) /
    ((g + q1)*g + q0), g = f*f; n >= 2 -> -f; + offset[n]; den <= 0 -> pi - a; num < 0 -> -a.
    Multiplier flag exits (0x326..: divide / overflow / underflow after the first division) are not modelled yet
    (never seen in the probe logs)."""
    num, den = f32(num), f32(den)
    if abs(den) < abs(_TINY):           # 0x31D FABC, branch 0x363
        if num == 0:
            return ma0                   # 0 / 0: returns MA0 unchanged (whatever the code before left there)
        a = _AT[2]
        return f32(-a) if f32(f32(0) - num) > 0 else a
    q = f32(abs(tgp_div(num, den)))
    n = 0
    if f32(f32(1.0) - q) < 0:            # 0x32B FCMP q, 1.0 -> call 0x342
        q = _rcp3(q)
        n = 2
    if f32(q - _AT[4]) >= 0:             # 0x32E FCMP tan15, q -> call 0x34A
        n += 1
        q = tgp_div(f32(f32(q * _AT[5]) + f32(-1.0)), f32(q + _AT[5]))
    f = q
    if f32(abs(_AT[6]) - abs(f)) < 0:    # 0x332 FABC f, 2^-12 -> call 0x34E
        g = f32(f * f)
        p = f32(f32(f32(g * _AT[7]) + _AT[8]) * g)
        d = f32(f32(f32(g + _AT[9]) * g) + _AT[10])
        r = tgp_div_ma0(p, d)
        f = f32(f32(r * f) + f)
    if n >= 2:                           # 0x335 CMP n, 1 -> call 0x359 (negate)
        f = f32(f * f32(-1.0))
    a = f32(f + _AT[n])
    if not (f32(f32(0) - den) < 0):      # 0x339 FCMP den, 0: den <= 0 -> pi - a
        a = f32(_AT[11] - a)
    if f32(num - f32(0)) < 0:            # 0x33D: num < 0 -> -a
        a = f32(a * f32(-1.0))
    return a


def tgp_div_ma0(a, b):
    """routine 0x351..0x356 inside 0x34E: the same division as tgp_div"""
    return tgp_div(a, b)


def _cfi(x):
    """MB86235 CFI: float -> int32 by truncation (as the sin / cos tables showed)"""
    return int(float(x)) & 0xFFFFFFFF


def _s32(v):
    v &= 0xFFFFFFFF
    return v - (1 << 32) if v & 0x80000000 else v


def f0f_angles(x, y, z, ma0=None, fix=True):
    """function 0x0F (code 0x08D -> 0x713): direction (x, y, z) -> (yaw, pitch) as 16-bit-unit integers
    (0x10000 = 360 degrees). yaw = CFI(atan2(-x, z) * 65536 / 2 pi); h = z / cos(yaw) when (yaw + 0x2000) & 0x4000
    is 0, else -x / sin(yaw) (sin / cos from routine 0x21D of the integer yaw, divisions 0x1CD); pitch =
    CFI(atan2(y, h) * 65536 / 2 pi); if bit 31 xor bit 30 of pitch: yaw += 0x8000, pitch = 0x8000 - pitch.
    Outputs yaw then pitch (FIFO words, int32). fix=False: routine 0x6F2 (used by 0x71), the same without the
    final bit 31 / bit 30 test.
    Quirk: atan2(0, 0) returns the multiplier register MA0 unchanged. For x = z = 0 the yaw is therefore CFI(MA0 x
    65536 / 2 pi) with MA0 left by the previous coprocessor call (`ma0`; None = unknown -> yaw None); then h = 0 and,
    for y = 0, the pitch comes from the MA0 the division z / cos left (about 1.0: pitch 0x28BE).
    In the attract loop MA0 was ~0 (yaw 0) or 496.0 (yaw 0x4EF03D) at such calls."""
    x, y, z = f32(x), f32(y), f32(z)
    nx = f32(f32(0) - x)
    a = tgp_atan2(nx, z, ma0)
    if a is None:
        # yaw unknown; x = z = 0 makes h = +-0 whatever the yaw, and MA0 after 0 / (cos or sin) is 1.0 for any
        # normal divisor: model the pitch with that (checked against the log)
        yaw = None
        h, m = f32(0), f32(1.0)
    else:
        yaw = _cfi(f32(a * _ANGLE))
        s, c = tgp_sincos(yaw)
        if ((yaw + 0x2000) & 0x4000) == 0:
            h, m = tgp_div(z, c), tgp_div_ma0_after(z, c)
        else:
            h, m = tgp_div(nx, s), tgp_div_ma0_after(nx, s)
    a = tgp_atan2(y, h, m)
    pitch = _cfi(f32(a * _ANGLE))
    if fix and (_s32(pitch + pitch) ^ _s32(pitch)) < 0:
        yaw = None if yaw is None else (yaw + 0x8000) & 0xFFFFFFFF
        pitch = (0x8000 - pitch) & 0xFFFFFFFF
    return yaw, pitch


# Not modelled yet (meaning only): 0x1A = point x the current matrix (((m0*x + m3*y) + m6*z) + m9 per row), 0x06 =
# pop the matrix stack, 0x07 = load a matrix, 0x74 = load stored matrix n, 0x88 = set the translation, 0x0F = vector
# -> two angles (0x713: arctangent routine 0x31A, scale 65536 / 2 pi = 0x4622F83D, CFI), 0x1D / 0x1E = x * sin / cos.


def _f(h):
    return struct.unpack("<f", struct.pack("<I", int(h, 16)))[0]


def _bits(x):
    if isinstance(x, int):
        return x & 0xFFFFFFFF
    return struct.unpack("<I", struct.pack("<f", float(x)))[0]


def check(path):
    """compares every modelled function found in a probe log (tools/run_copro_probe.ps1) with the model"""
    models = {
        0x1F: lambda w: f1f_distance(*[_f(h) for h in w[:6]]),
        0x2D: lambda w: tgp_sqrt(_f(w[0])),   # 0x144: routine 0x1D9 on one input
        0x0C: lambda w: f0c_normalize(*[_f(h) for h in w[:3]]),
        0x1B: lambda w: f1b_sin(int(w[0], 16)),
        0x1C: lambda w: f1c_cos(int(w[0], 16)),
        0x0F: lambda w: f0f_angles(*[_f(h) for h in w[:3]]),
        0x83: lambda w: f0f_angles(_f(w[1]), _f(w[2]), _f(w[0])),   # 0x0F's routine on (y, z, x)
        0x1D: lambda w: f32(_f(w[1]) * tgp_sincos(int(w[0], 16))[0]),   # 0x123: r x sin (MA0 = r, MA1 = sin)
        0x1E: lambda w: f32(tgp_sincos(int(w[0], 16))[1] * _f(w[1])),   # 0x128: cos x r
    }
    stats = {}
    for line in open(path):
        p = line.split()
        if "in" not in p or "out" not in p:
            continue
        n = int(p[0], 16) & 0xFF
        if n not in models:
            continue
        ins = p[p.index("in") + 1:p.index("out")]
        outs = [int(h, 16) for h in p[p.index("out") + 1:]]
        want = models[n](ins)
        s = stats.setdefault(n, [0, 0, 0])
        want = want if isinstance(want, tuple) else (want,)
        if any(v is None for v in want):
            s[2] += 1
        ok = outs[-1] == _bits(want[-1]) and all(_bits(v) in outs for v in want if v is not None)
        s[0] += 1
        if not ok:
            s[1] += 1
            if s[1] <= 3:
                print("differs:", line.strip())
    for n, (calls, bad, skip) in sorted(stats.items()):
        print("function 0x%02X: %d calls, %d differ%s" % (n, calls, bad, ", %d only partly checked (input from an earlier call)" % skip if skip else ""))


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "check":
        check(sys.argv[2])
    else:
        print(__doc__)
