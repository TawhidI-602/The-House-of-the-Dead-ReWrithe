#!/usr/bin/env python3
"""
palanim_check.py - the game's palette animation (src/game/palette_anim.cpp) against MAME's palette RAM.

The animation depends only on the frame counter (period 43200 = lcm(135, 96, 100, 64)) and the chapter byte / event flags,
so a MAME dump with frame counter C is compared with the game's logged line for counter c, c = C - 1 (mod 43200) (the
palette RAM shows the previous frame's values): entries 959 and 984..1021 that the game sets (non-zero) must be equal.

Usage:
  hotd ... with HOTD_PALANIM_LOG=<log>   (one line per frame: counter chapter e959 e984..e1021)
  python palanim_check.py <log> <dumps folder>...
"""
import sys, os, glob, struct

P = 43200


def main():
    lines = {}
    for ln in open(sys.argv[1]):
        f = ln.split()
        if len(f) < 41:
            continue
        lines[int(f[0]) % P] = (int(f[1]), [int(x, 16) for x in f[2:]])
    total = exact = compared = 0
    for d in sys.argv[2:]:
        for wf in sorted(glob.glob(os.path.join(d, "*_workram.bin"))):
            pf = wf.replace("_workram", "_palette")
            if not os.path.exists(pf):
                continue
            w, pal = open(wf, "rb").read(), open(pf, "rb").read()
            c = struct.unpack_from("<i", w, 0x1F428)[0]
            if c == 0 or w[0x1F9D1] != 0:
                continue   # attract mode / not Chapter 1 (the game runs Chapter 1 only)
            ours = lines.get((c - 1) % P)
            if ours is None:
                continue
            total += 1
            entries = [959] + list(range(984, 1022))
            miss = []
            for e, v in zip(entries, ours[1]):
                if v == 0:
                    continue
                compared += 1
                m = struct.unpack_from("<H", pal, (0x1000 + e) * 2)[0]
                if m != v:
                    miss.append((e, hex(v), hex(m)))
            exact += not miss
            if miss and total - exact <= 10:
                print(f"{os.path.basename(wf)}: counter {c}: {miss[:6]}")
    print(f"{len(lines)} logged frames; {total} MAME dumps matched by counter, {exact} exact ({compared} entries compared)")
    return 0 if total and exact == total else 1


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    sys.exit(main())
