#!/usr/bin/env python3
"""
tgp_trace.py - replays a full in-order coprocessor call trace (tools/run_copro_probe.ps1 -Funcs all) through the
models of tools/tgp_model.py, keeping the coprocessor's state the way its program does, and checks every output word.

State kept (from the program, private/copro/tgp_program.dis):
  PR[0..11]  the current 3x4 matrix in the P registers: rows (PR0..2), (PR3..5), (PR6..8), translation PR9..11
  B-RAM      matrix stack (depth B[0], pointer B[1]; base 0x20, 12 words per entry, 15 entries; function 0x08
             resets it), stored matrices at B[0xF8 + 12 n] (functions 0x73 / 0x74 / 0x75 / 0x76 / 0x77)
  MA0        the multiplier register an arctangent of 0 / 0 returns (see tgp_model.f0f_angles); tracked where known

  py tools/tgp_trace.py <probe log> [max lines]
Prints per function: calls, outputs checked, differences; the first differences in detail; functions not modelled
(their state effects make the matrix unknown until the next full matrix load).
"""
import os
import struct
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tgp_model as M

f32 = np.float32
UNK = None


def hf(h):
    return f32(struct.unpack("<f", struct.pack("<I", h))[0])


def bits(x):
    if isinstance(x, (int, np.integer)):
        return int(x) & 0xFFFFFFFF
    return struct.unpack("<I", struct.pack("<f", float(x)))[0]


IDENTITY = [f32(1), f32(0), f32(0), f32(0), f32(1), f32(0), f32(0), f32(0), f32(1), f32(0), f32(0), f32(0)]


_DATA = None


def data_rom():
    """the coprocessor's data ROM (MAME region copro_data: epr-19707 = low 16 bits, epr-19706 = high 16 bits of each
    32-bit word), read from the user's own hotd.zip (private/, never committed); seen by the program at 0x800000"""
    global _DATA
    if _DATA is None:
        import zipfile
        z = zipfile.ZipFile(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "private", "hotd.zip"))
        lo = np.frombuffer(z.read("epr-19707.29"), "<u2").astype(np.uint32)
        hi = np.frombuffer(z.read("epr-19706.30"), "<u2").astype(np.uint32)
        _DATA = lo | (hi << 16)
    return _DATA


def ray_test(k, p, d):
    """function 0x62 (0x7A9): ray p + s d against polygon list k of the data ROM.
    List pointer = ROM[0x100 + k]; at it: count, 6 words (not read here), then per polygon 18 words: plane (nx, ny,
    nz, dd), type (0: test in y-z, 1: x-z, 2: x-y), 4 vertices (x, y, z), attribute.
    Plane (routine 0x89C): nD = (nx dx + ny dy) + nz dz; |nD| <= 1e-4 (0x38D1B717) -> miss; t = ((nx px + ny py) +
    nz pz) + dd) / nD (division 0x1CD); t >= 0 -> miss; s = 0 - t; hit = p + d s (each p + d*s). Nearest: s < best
    (best starts at FLT_MAX). Inside (0x7F9): relative vectors of the 4 vertices in the type's plane, taken in the
    order v0 v1 v2 v3, or v0 v3 v2 v1 when the dropped axis' normal component is >= 0 (types 0, 2) / < 0 (type 1);
    the four cross products a.u b.v - b.u a.v (each product rounded, then the difference) must all have the sign bit
    clear (so -0.0 counts as outside). Output: -1.0, or 1.0, hit x y z, s^2, plane normal, attribute."""
    rom = data_rom()
    ptr = int(rom[0x100 + k])
    cnt = int(rom[ptr])
    eo = ptr + 7
    best, found = f32(np.finfo(np.float32).max), None
    sgn = lambda q: bits(q) >> 31
    for _ in range(cnt):
        nx, ny, nz, dd = (hf(int(rom[eo + j])) for j in range(4))
        eo += 4
        nd = f32(f32(f32(nx * d[0]) + f32(ny * d[1])) + f32(nz * d[2]))
        hit = None
        if not (f32(hf(0x38D1B717) - f32(abs(nd))) >= 0):
            npd = f32(f32(f32(f32(nx * p[0]) + f32(ny * p[1])) + f32(nz * p[2])) + dd)
            t = M.tgp_div(npd, nd)
            if t < 0:
                s = f32(f32(0) - t)
                if f32(s - best) < 0:
                    hit = [f32(f32(d[j] * s) + p[j]) for j in range(3)], s
        if hit is None:
            eo += 14
            continue
        (hx, hy, hz), s = hit
        q = eo + 1
        typ = int(rom[eo])
        v = [[hf(int(rom[q + 3 * i + j])) for j in range(3)] for i in range(4)]
        if typ == 0:
            ax, h, rev = (1, 2), (hy, hz), sgn(nx) == 0
        elif typ == 1:
            ax, h, rev = (0, 2), (hx, hz), sgn(ny) == 1
        else:
            ax, h, rev = (0, 1), (hx, hy), sgn(nz) == 0
        order = (0, 3, 2, 1) if rev else (0, 1, 2, 3)
        rel = [(f32(v[i][ax[0]] - h[0]), f32(v[i][ax[1]] - h[1])) for i in order]
        signs = 0
        for i in range(4):
            a, b = rel[i], rel[(i + 1) % 4]
            signs |= sgn(f32(f32(a[0] * b[1]) - f32(b[0] * a[1])))
        attr = int(rom[q + 12])
        eo += 14
        if signs == 0:
            best, found = s, ([hx, hy, hz], (nx, ny, nz), attr)
    if found is None:
        return [f32(-1.0)]
    (hx, hy, hz), nrm, attr = found
    return [f32(1.0), hx, hy, hz, f32(best * best), nrm[0], nrm[1], nrm[2], attr]


def _rcp2(x):
    """1 / x as routines 0x542 / 0x90D write it: r = FRCP(x); t = x r; e = 2 - t; t1 = e t; result (2 - t1)(r e)"""
    r = M.frcp(x)
    t = f32(x * r)
    e = f32(f32(2.0) - t)
    t1 = f32(e * t)
    return f32(f32(f32(2.0) - t1) * f32(r * e))


def curve_eval(c, t):
    """routine 0x8E3: keyframed curve at ROM word c, time t. Header word: hi16 = search steps + 1, lo16 = first step;
    keys of 4 words (time, value, out-tangent, in-tangent) from c + 1. Binary search: pos = c + 1 + 2 lo, step = lo;
    hi - 1 times: s = step, step >>= 1, t < key[pos] -> s = -s, pos += s; then t < key[pos] -> pos -= 2 step.
    Segment pos .. pos + 4: dt = t1 - t0, u = t - t0, 1 / dt, 1 / dt^2, 1 / dt^3 (each _rcp2), s2 = u^2 / dt^2,
    s3 = u^3 / dt^3: h00 = (2 s3 - 3 s2) + 1, h01 = 3 s2 - 2 s3, h10 = ((u^3 / dt^2 - u^2 / dt) - u^2 / dt) + u,
    h11 = u^3 / dt^2 - u^2 / dt; result ((h01 v1 + h00 v0) + h10 out0) + h11 in1"""
    rom = data_rom()
    w = int(rom[c])
    hi, lo = w >> 16, w & 0xFFFF
    pos, step, cnt = c + 1 + 2 * lo, lo, hi - 1
    while True:
        key = hf(int(rom[pos]))
        s = step
        step >>= 1
        if f32(t - key) < 0:
            s = -s
        cnt -= 1
        pos += s
        if cnt == 0:
            break
    if f32(t - hf(int(rom[pos]))) < 0:
        pos -= step << 1
    t0, t1 = hf(int(rom[pos])), hf(int(rom[pos + 4]))
    v0, v1 = hf(int(rom[pos + 1])), hf(int(rom[pos + 5]))
    m0, m1 = hf(int(rom[pos + 2])), hf(int(rom[pos + 7]))
    dt = f32(t1 - t0)
    u = f32(t - t0)
    r1 = _rcp2(dt)
    dt2 = f32(dt * dt)
    r2 = _rcp2(dt2)
    r3 = _rcp2(f32(dt2 * dt))
    u2 = f32(u * u)
    u3 = f32(u * u2)
    a2 = f32(f32(r3 * u3) * f32(2.0))
    a4 = f32(f32(r2 * u2) * f32(3.0))
    h00 = f32(f32(a2 - a4) + f32(1.0))
    h01 = f32(a4 - a2)
    a3 = f32(r2 * u3)
    a1 = f32(r1 * u2)
    h10 = f32(f32(f32(a3 - a1) - a1) + u)
    h11 = f32(a3 - a1)
    return f32(f32(f32(f32(h01 * v1) + f32(h00 * v0)) + f32(h10 * m0)) + f32(h11 * m1))


def path_table(i, t):
    """function 0x6E (0x8C1): base = ROM[i]; the 8 curves whose pointers are at ROM[base .. base + 7], all at time t
    (routine 0x8E3); outputs 6 floats, CFI(7th) & 0xFFFF (an angle), the 8th float"""
    rom = data_rom()
    base = int(rom[i])
    out = [curve_eval(int(rom[base + j]), t) for j in range(8)]
    out[6] = M._cfi(out[6]) & 0xFFFF
    return out


class Tgp:
    def __init__(self, from_boot=True):
        self.boot = from_boot
        self.pr = [f32(0)] * 12 if from_boot else [UNK] * 12
        self.b = {0: 0, 1: 0x20} if from_boot else {0: UNK, 1: UNK}   # B-RAM words (floats or ints)
        self.ma0 = UNK

    def bget(self, a):
        return self.b.get(a, f32(0) if self.boot else UNK)

    @staticmethod
    def mul(n, m):
        """the program's matrix product (functions 0x09 / 0x76 / 0x77, routine 0x6C0): row i of the result =
        (n[3i] row0 + n[3i+1] row1) + n[3i+2] row2 of m; translation = ((n9 row0 + n10 row1) + n11 row2) + m9..11"""
        if any(v is UNK for v in n) or any(v is UNK for v in m):
            return [UNK] * 12
        out = []
        for i in range(4):
            a, b, c = n[3 * i], n[3 * i + 1], n[3 * i + 2]
            for k in range(3):
                v = f32(f32(f32(a * m[k]) + f32(b * m[3 + k])) + f32(c * m[6 + k]))
                if i == 3:
                    v = f32(v + m[9 + k])
                out.append(v)
        return out

    def push(self):
        """0x05 (routine 0x020): depth + 1 always; the copy only while depth < 15"""
        d, p = self.b[0], self.b[1]
        if d is UNK or p is UNK:
            return
        if d - 15 < 0:
            for k in range(12):
                self.b[p + k] = self.pr[k]
            self.b[1] = p + 12
        self.b[0] = d + 1
        self.ma0 = UNK if (self.ma0 is UNK or self.pr[0] is UNK) else f32(self.ma0 * self.pr[0])

    def pop(self):
        """0x06 (0x02A): depth <= 0: nothing; depth > 15: depth - 1 only; else depth - 1, pointer - 12, load"""
        d, p = self.b[0], self.b[1]
        if d is UNK or p is UNK:
            self.pr = [UNK] * 12
            return
        if 0 - d >= 0:
            return
        self.b[0] = d - 1
        if 15 - d < 0:
            return
        p -= 12
        self.pr = [self.bget(p + k) for k in range(12)]
        self.b[1] = p

    def rot(self, angle, r1, r2, sign):
        """0x14 / 0x16 (0x0C9 / 0x0F4, rows 1, 2 / rows 0, 1): r1' = c r1 - s r2, r2' = s r1 + c r2;
        0x15 (0x0DE, rows 0, 2): r0' = c r0 + s r2, r2' = c r2 - s r0. sin / cos of the 16-bit angle by 0x21D."""
        s, c = M.tgp_sincos(angle)
        self.ma0 = c
        pr = list(self.pr)
        if any(v is UNK for v in pr):
            return
        for k in range(3):
            a, b = pr[3 * r1 + k], pr[3 * r2 + k]
            if sign > 0:
                pr[3 * r1 + k] = f32(f32(c * a) - f32(s * b))
                pr[3 * r2 + k] = f32(f32(s * a) + f32(c * b))
            else:
                pr[3 * r1 + k] = f32(f32(c * a) + f32(s * b))
                pr[3 * r2 + k] = f32(f32(c * b) - f32(s * a))
        self.pr = pr

    def xform(self, x, y, z):
        """0x1A (0x10A) / 0x12 (0x0A8): ((x m0 + y m3) + z m6) + m9 per column"""
        m = self.pr
        if any(v is UNK for v in m):
            return [UNK] * 3
        return [f32(f32(f32(f32(x * m[k]) + f32(y * m[3 + k])) + f32(z * m[6 + k])) + m[9 + k]) for k in range(3)]

    def inverse(self):
        """0x7A (0x4D1): the current matrix inverted. a..i = PR0..8, t = PR9..11; cofactors (each product rounded,
        then the difference); det = (a (ei - hf) + d (hc - bi)) + g (bf - ec); invdet = FRCP + two Newton steps in
        the 0x4EF order; rows: (ei-hf, hc-bi, bf-ec), (gf-di, ai-gc, dc-af), (dh-ge, gb-ah, ae-db) x invdet;
        translation ((g(e tz - f ty) - d(h tz - i ty)) - tx(ei - hf)) x invdet, ((a(h tz - i ty) - g(b tz - c ty))
        - tx(hc - bi)) x invdet, ((d(b tz - c ty) - a(e tz - f ty)) - tx(bf - ec)) x invdet"""
        m = self.pr
        if any(v is UNK for v in m):
            self.ma0 = UNK
            return []
        a, b, c, d, e, f, g, h, i, tx, ty, tz = m
        dif = lambda p, q: f32(f32(p) - f32(q))
        ab0 = dif(b * tz, ty * c)
        ab1 = dif(e * tz, ty * f)
        ab2 = dif(h * tz, ty * i)
        ab3 = dif(e * i, h * f)
        ab4 = dif(h * c, b * i)
        ab5 = dif(b * f, e * c)
        det = f32(f32(f32(a * ab3) + f32(d * ab4)) + f32(g * ab5))
        r = M.frcp(det)
        t = f32(det * r)
        e1 = f32(f32(2.0) - t)
        t1 = f32(e1 * t)
        inv = f32(f32(f32(2.0) - t1) * f32(r * e1))
        out = [f32(ab3 * inv), f32(ab4 * inv), f32(ab5 * inv),
               f32(dif(g * f, d * i) * inv), f32(dif(a * i, g * c) * inv), f32(dif(d * c, a * f) * inv),
               f32(dif(d * h, g * e) * inv), f32(dif(g * b, a * h) * inv)]
        aedb = dif(a * e, d * b)
        out.append(f32(aedb * inv))
        out.append(f32(dif(dif(g * ab1, d * ab2), tx * ab3) * inv))
        out.append(f32(dif(dif(a * ab2, g * ab0), tx * ab4) * inv))
        out.append(f32(dif(dif(d * ab0, a * ab1), tx * ab5) * inv))
        self.pr = out
        self.ma0 = aedb
        return []

    def sphere_visible(self, x, y, z, rad):
        """0x7B (0x51E): push; load stored matrix 0 (the camera); v = transform (x, y, z); pop (through 0x02A);
        output +1.0 / -1.0. v2 > 0: w = (rad B4 + v2); the point is in when -248 w / a < v0 < 248 w / a and
        -192 w / a < v1 < 192 w / a (tests by the sign bits of (W - v0) xor (-v0 - W) etc.; 1 / a by FRCP + two
        Newton steps); else in when |v|^2 < (rad B5)^2. a, B4, B5 from 0x7D."""
        self.push()
        self.pr = [self.bget(0xF8 + k) for k in range(12)]
        v0, v1, v2 = self.xform(x, y, z)
        res = UNK
        if v0 is not UNK:
            sb = lambda q: bits(q) >> 31
            if f32(f32(0) - v2) < 0:
                a = self.bget(2)
                r = M.frcp(a)
                t = f32(a * r)
                e1 = f32(f32(2.0) - t)
                t1 = f32(e1 * t)
                ia = f32(f32(f32(2.0) - t1) * f32(r * e1))
                w = f32(f32(rad * self.bget(4)) + v2)
                aa3 = f32(f32(f32(248.0) * w) * ia)
                aa4 = f32(f32(f32(192.0) * w) * ia)
                n0, n1 = f32(v0 * f32(-1.0)), f32(v1 * f32(-1.0))
                in0 = sb(f32(aa3 + n0)) ^ sb(f32(n0 - aa3))
                in1 = sb(f32(aa4 + n1)) ^ sb(f32(n1 - aa4))
                res = f32(1.0) if (in0 & in1) else f32(-1.0)
            else:
                q = f32(rad * self.bget(5))
                d2 = f32(f32(f32(v0 * v0) + f32(v1 * v1)) + f32(v2 * v2))
                res = f32(1.0) if f32(d2 - f32(q * q)) < 0 else f32(-1.0)
        self.ma0 = v0
        self.pop()
        return [res]

    def matrix_angles(self):
        """0x6F (0x368): push; (yaw, pitch) = routine 0x713 of row 2; identity; rotate X by -pitch, Y by -yaw;
        v = row 0 (saved before) transformed; roll = CFI(atan2(-v1, v0) x 65536 / 2 pi); pop; outputs yaw, pitch,
        roll (int32)"""
        self.push()
        m = list(self.pr)
        if any(v is UNK for v in m):
            self.pop()
            self.ma0 = UNK
            return [UNK] * 3
        yp = M.f0f_angles(m[6], m[7], m[8], self.ma0)
        yaw, pitch = yp
        if yaw is None:
            self.pop()
            self.ma0 = UNK
            return [UNK, pitch, UNK]
        self.pr = list(IDENTITY)
        self.rot((0 - pitch) & 0xFFFFFFFF, 1, 2, +1)
        self.rot((0 - yaw) & 0xFFFFFFFF, 0, 2, -1)
        v = self.xform(m[0], m[1], m[2])
        a = M.tgp_atan2(f32(v[1] * f32(-1.0)), v[0], self.ma0)
        roll = UNK if a is None else M._cfi(f32(a * M._ANGLE))
        self.ma0 = UNK
        self.pop()
        return [yaw, pitch, roll]

    @staticmethod
    def rsq(x):
        """1 / sqrt(x): FRSQ seed + two Newton steps y = (3 - x (y y)) (y 0.5) (routine 0x6A0, no zero test)"""
        with np.errstate(divide="ignore", invalid="ignore", over="ignore"):
            y = M.frsq(x)
            for _ in range(2):
                y = f32(f32(f32(3.0) - f32(x * f32(y * y))) * f32(y * f32(0.5)))
        return y

    def matrix_angles_x(self):
        """0x71 (0x40A): like 0x6F with X forward: push; (yaw, pitch) = routine 0x6F2 (0x713 without the last test)
        of (m1, m2, m0); identity; rotate Y by -pitch (as 0x15), then Z by -yaw (as 0x16); v = row 1 (saved)
        transformed; roll = CFI(atan2(-v2, v1) x 65536 / 2 pi); pop; outputs yaw, pitch, roll"""
        self.push()
        m = list(self.pr)
        if any(v is UNK for v in m):
            self.pop()
            self.ma0 = UNK
            return [UNK] * 3
        yaw, pitch = M.f0f_angles(m[1], m[2], m[0], self.ma0, fix=False)
        if yaw is None:
            self.pop()
            self.ma0 = UNK
            return [UNK, pitch, UNK]
        self.pr = list(IDENTITY)
        self.rot((0 - pitch) & 0xFFFFFFFF, 0, 2, -1)
        self.rot((0 - yaw) & 0xFFFFFFFF, 0, 1, +1)
        v = self.xform(m[3], m[4], m[5])
        a = M.tgp_atan2(f32(v[2] * f32(-1.0)), v[1], self.ma0)
        roll = UNK if a is None else M._cfi(f32(a * M._ANGLE))
        self.ma0 = UNK
        self.pop()
        return [yaw, pitch, roll]

    def call(self, n, ins):
        """runs function n on the FIFO input words; returns the output words (None = unknown) or 'unmodelled'"""
        fl = [hf(w) for w in ins]
        if n == 0x05:
            self.push()
            return []
        if n == 0x06:
            self.pop()
            return []
        if n == 0x07:
            self.pr = fl[:12]
            return []
        if n == 0x08:
            self.b[1] = 0x20
            self.b[0] = 0
            return []
        if n == 0x09:
            self.ma0 = UNK
            self.pr = self.mul(fl[:12], self.pr)
            return []
        if n == 0x0A:
            a = M.tgp_atan2(fl[1], fl[0], self.ma0)
            return [UNK if a is None else M._cfi(f32(a * M._ANGLE))]
        if n == 0x0F:
            return list(M.f0f_angles(fl[0], fl[1], fl[2], self.ma0))
        if n == 0x10:
            self.pr = list(IDENTITY)
            return []
        if n == 0x11:
            return list(self.pr)
        if n == 0x12:
            self.pr = self.pr[:9] + self.xform(fl[0], fl[1], fl[2])
            self.ma0 = fl[2]
            return []
        if n == 0x13:
            pr = self.pr
            if not any(v is UNK for v in pr):
                self.pr = [f32(pr[k] * fl[k // 3]) for k in range(9)] + pr[9:]
            self.ma0 = fl[2]
            return []
        if n == 0x14:
            self.rot(ins[0], 1, 2, +1)
            return []
        if n == 0x15:
            self.rot(ins[0], 0, 2, -1)
            return []
        if n == 0x16:
            self.rot(ins[0], 0, 1, +1)
            return []
        if n == 0x19:
            return [self.b[0]]   # None while unknown
        if n == 0x1A:
            self.ma0 = fl[2]
            return self.xform(fl[0], fl[1], fl[2])
        if n in (0x1B, 0x1C):
            s, c = M.tgp_sincos(ins[0])
            self.ma0 = c
            return [s if n == 0x1B else c]
        if n == 0x1D:
            s, c = M.tgp_sincos(ins[0])
            self.ma0 = fl[1]
            return [f32(fl[1] * s)]
        if n == 0x1E:
            s, c = M.tgp_sincos(ins[0])
            self.ma0 = f32(c * fl[1])
            return [self.ma0]
        if n == 0x1F:
            self.ma0 = UNK
            return [M.f1f_distance(*fl[:6])]
        if n == 0x0C:
            self.ma0 = UNK
            return list(M.f0c_normalize(*fl[:3]))
        if n == 0x20:
            return list(self.pr[9:12])
        if n == 0x21:
            self.push()
            self.pr = list(IDENTITY)
            return []
        if n in (0x73, 0x74, 0x75, 0x76, 0x77):
            slot = 0xF8 + 12 * ins[0]
            self.ma0 = UNK
            if n == 0x74:
                self.push()
            if n in (0x73, 0x74):
                self.pr = [self.bget(slot + k) for k in range(12)]
            elif n == 0x75:
                for k in range(12):
                    self.b[slot + k] = self.pr[k]
            elif n == 0x76:
                self.pr = self.mul([self.bget(slot + k) for k in range(12)], self.pr)
            else:
                self.pr = self.mul(self.pr, [self.bget(slot + k) for k in range(12)])
            return []
        if n == 0x7F:
            self.push()
            self.pr = fl[:12]
            self.ma0 = UNK
            return []
        if n == 0x7A:
            return self.inverse()
        if n == 0x7B:
            return self.sphere_visible(fl[0], fl[1], fl[2], fl[3])
        if n == 0x7D:
            # 0x5A3: projection constants: B[2] = a, B[3] = b, B[4] = sqrt(248^2 + a^2) / 248, B[5] = that / a
            a = fl[0]
            s = M.tgp_sqrt(f32(f32(61504.0) + f32(a * a)))
            self.b[2], self.b[3] = a, fl[1]
            self.b[4], self.b[5] = M.tgp_div(s, f32(248.0)), M.tgp_div(s, a)
            self.ma0 = a
            return []
        if n == 0x86:
            # 0x645: rotate Z (as 0x16), then Y (as 0x15), then X (as 0x14)
            self.rot(ins[0], 0, 1, +1)
            self.rot(ins[1], 0, 2, -1)
            self.rot(ins[2], 1, 2, +1)
            return []
        if n == 0x6F:
            return self.matrix_angles()
        if n == 0x6E:
            r = path_table(ins[0], fl[1])
            self.ma0 = UNK
            return r
        if n == 0x62:
            self.ma0 = UNK
            return ray_test(ins[0], fl[1:4], fl[4:7])
        if n == 0x71:
            return self.matrix_angles_x()
        if n == 0x89:
            # 0x692: cosine of the angle between a and b: dot = (a0 b0 + a1 b1) + a2 b2; |a|^2 = (a0^2 + a1^2) +
            # a2^2; 1 / |a| = FRSQ + two Newton steps (no zero test); result (dot / |a|) / |b| as two products
            a, b = fl[:3], fl[3:6]
            dot = f32(f32(f32(a[0] * b[0]) + f32(a[1] * b[1])) + f32(a[2] * b[2]))
            ra, rb = (self.rsq(f32(f32(f32(v[0] * v[0]) + f32(v[1] * v[1])) + f32(v[2] * v[2]))) for v in (a, b))
            self.ma0 = ra
            return [f32(f32(dot * ra) * rb)]
        if n == 0x87:
            self.pr = fl[:9] + self.pr[9:]
            return []
        if n == 0x88:
            self.pr = self.pr[:9] + fl[:3]
            return []
        return "unmodelled"


def match(want, outs):
    """the model's words must appear in order in the logged reads; extra logged words may only be 0 (reads of an
    empty FIFO that the i960 repeats); an unknown model word matches any one read"""
    want = [UNK if w is UNK else bits(w) for w in want]

    def m(i, j):
        if i == len(want):
            return all(o == 0 for o in outs[j:])
        if j == len(outs):
            return False
        if want[i] is UNK or outs[j] == want[i]:
            if m(i + 1, j + 1):
                return True
        return outs[j] == 0 and m(i, j + 1)
    return m(0, 0)


def main(path, maxlines=None):
    hdr = os.path.join(os.path.dirname(os.path.abspath(path)), "recording.txt")
    late = os.path.exists(hdr) and any("HOTD_PROBE_START=" in l and
                                       l.split("=")[-1].strip() not in ("", "0") for l in open(hdr))
    t = Tgp(from_boot=not late)
    stats, shown, unmod = {}, 0, {}
    for lineno, line in enumerate(open(path), 1):
        if maxlines and lineno > maxlines:
            break
        p = line.split()
        if "in" not in p or "out" not in p:
            continue
        n = int(p[0], 16) & 0xFF
        ins = [int(h, 16) for h in p[p.index("in") + 1:p.index("out")]]
        outs = [int(h, 16) for h in p[p.index("out") + 1:]]
        r = t.call(n, ins)
        if isinstance(r, str):
            unmod[n] = unmod.get(n, 0) + 1
            t.pr = [UNK] * 12   # conservative: these may change the matrix
            t.ma0 = UNK
            continue
        s = stats.setdefault(n, [0, 0, 0, 0])
        s[0] += 1
        if not r:
            continue
        if any(v is UNK for v in r):
            s[3] += 1
            if all(v is UNK for v in r):
                continue
        s[1] += 1
        if not match(r, outs):
            s[2] += 1
            if shown < 8:
                shown += 1
                print("line %d differs: %s\n   model %s" % (lineno, line.strip()[:200],
                      " ".join("????????" if v is UNK else "%08x" % bits(v) for v in r)))
    for n, (calls, checked, bad, unk) in sorted(stats.items()):
        print("0x%02X: %7d calls, %7d with outputs checked, %d differ%s" % (n, calls, checked, bad,
              ", %d with unknown words" % unk if unk else ""))
    if unmod:
        print("not modelled:", " ".join("0x%02X:%d" % kv for kv in sorted(unmod.items())))


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
    else:
        main(sys.argv[1], int(sys.argv[2]) if len(sys.argv) > 2 else None)
