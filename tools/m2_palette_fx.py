#!/usr/bin/env python3
"""
m2_palette_fx.py - the arcade's palette animation (formats.md §24), a Python copy of maincpu 0x25310 + 0x24E90.

0x25310 runs once per game frame: from the frame counter (work RAM 0x51F428), the stage byte (0x51F9D1) and four event
flags (0x51E982, 0x51E9A1, 0x51E9B2, 0x51E9B3) it computes pulsing colors into work RAM 0x52066C-0x5206E0; 0x24E90 copies
work RAM 0x520664 + 2n to polygon palette entry 1021 - n (n = 0..37, skipping 0x520668, 0x520688, 0x52068A) and 0x5206E0
to entry 959. The palette RAM shows the values of the previous frame. The engine's copy: src/assets/arcade_palette_fx.cpp.

Usage:
  python m2_palette_fx.py <regions folder> check <dumps folder>...   compare with every <tag>_workram.bin / _palette.bin
"""
import sys, os, glob, struct

cpu = b""
h = lambda a: struct.unpack_from("<H", cpu, a)[0]


def dec(c):
    return [c & 31, (c >> 5) & 31, (c >> 10) & 31]


def enc(p):
    p = [v if v == 31 else v & 30 for v in p]
    return (0x8000 | p[2] << 10 | p[1] << 5 | p[0]) & 0xFFFF


def tdiv(a, b):   # integer division toward zero
    q = abs(a) // abs(b)
    return q if (a >= 0) == (b >= 0) else -q


def trem(a, b):
    return a - tdiv(a, b) * b


def scale(c, r4):
    k = trem(r4, 32)
    return enc([tdiv(v * k, 32) for v in dec(c)])


def tri(frame, period, half):
    r = trem(frame, period)
    return r if r <= half else period - 1 - r


def palfx(frame, area, b982, b9a1, b9b2, b9b3, old):
    """work RAM address -> 16-bit value, as maincpu 0x25310 writes them (old: current values, for the entries it keeps)."""
    out = {}
    if area == 5 or (area == 2 and b982 != 0):
        return out, b9b3
    for i in range(9):
        out[0x52066C + 2 * i] = h(0xAB120 + 2 * (int((frame + 3 * i) / 1.5) % 90))
    r4 = int(tri(frame, 96, 47) / 1.5)
    for i in range(3):
        out[0x52067E + 2 * i] = scale(h(0xA35E8 + 2 * i), r4)
    out[0x520684] = 0 if area == 6 else scale(h(0xA35EE), r4)
    out[0x52068C] = scale(h(0xA35EE), r4)
    for i in range(14):
        a = 0x520690 + 2 * i
        if a == 0x520690 and area == 3 and b9b3:
            out[a] = 0
            continue
        if a == 0x520698:
            if area == 6:
                out[a] = 0
            continue
        v = int(tri(frame + 0x150 + 14 * i, 100, 49) / 2.5 + 12.0)
        if a == 0x520690 and area == 3 and b9b2 and v == 12:
            b9b3 = 1
            continue
        out[a] = scale(h(0xA35FA + 2 * i), v)
    r4 = tri(frame, 64, 31)
    out[0x52068E] = scale(h(0xA35F8), r4)
    A, B = dec(0x801F), dec(0x800C)
    k = trem(r4, 32)
    out[0x5206AE] = enc([a + tdiv((b - a) * k, 32) for a, b in zip(A, B)])
    out[0x5206E0] = scale(0x8012 if area == 3 and b9a1 else 0x800E, r4)
    return out, b9b3



def entry_of(addr):
    if addr == 0x5206E0:
        return 959
    if addr < 0x520664 or addr > 0x5206AE or addr in (0x520668, 0x520688, 0x52068A):
        return None
    return 1021 - (addr - 0x520664) // 2


def check(folders):
    total = exact = 0
    for d in folders:
        for f in sorted(glob.glob(os.path.join(d, "*_workram.bin"))):
            p = f.replace("_workram", "_palette")
            if not os.path.exists(p):
                continue
            w, pal = open(f, "rb").read(), open(p, "rb").read()
            frame = struct.unpack_from("<i", w, 0x1F428)[0]
            if frame == 0:
                continue   # attract mode: the routine doesn't run
            total += 1
            out, _ = palfx(frame - 1, w[0x1F9D1], w[0x1E982], w[0x1E9A1], w[0x1E9B2], w[0x1E9B3], None)
            miss = [entry_of(a) for a, v in out.items()
                    if entry_of(a) is not None and struct.unpack_from("<H", pal, (0x1000 + entry_of(a)) * 2)[0] != v]
            exact += not miss
            if miss and total - exact <= 10:
                print(f"{f}: frame {frame}, stage {w[0x1F9D1]}: entries differ {miss[:8]}")
    print(f"{total} dumps, {exact} exact")


if __name__ == "__main__":
    if len(sys.argv) < 4 or sys.argv[2] != "check":
        print(__doc__)
        sys.exit(1)
    cpu = open(os.path.join(sys.argv[1], "maincpu.bin"), "rb").read()
    check(sys.argv[3:])
