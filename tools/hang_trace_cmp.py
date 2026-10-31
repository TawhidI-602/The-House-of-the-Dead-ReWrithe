"""hang_trace_cmp.py (hangcheck): the Hangedman's per-frame state from MAME (tools/mame_hang_trace.lua) against ours
(hotd --frames ... with HOTD_TRACE_HANG=<his object>), aligned at the first frame of the fight (state 3, +0x50 = 4).

  py -I tools/hang_trace_cmp.py <mame trace.txt> <ours log>

Prints how many frames match exactly (state, position, rotation words, +0x50, +0x54, motion, motion frame, flags,
camera), the first difference, then per state: frames, mean / max movement per frame in both (the motion's pace), and
MAME's lag frames (vblanks without a game frame) inside the fight.
"""
import sys, math
from collections import defaultdict


def load_mame(p):
    rows = []
    for line in open(p):
        f = line.split()
        if not f or f[0] == "snap" or len(f) < 19:
            continue
        rows.append({"frame": int(f[0]), "screen": int(f[1]), "v": f[4:19]})
    return rows


def load_ours(p):
    rows = []
    for line in open(p, errors="replace"):
        f = line.split()
        if len(f) >= 18 and f[0] == "hang" and f[2] == "4FAA0":
            rows.append({"frame": int(f[1]), "v": f[3:18]})
    return rows


NAMES = "state x y z rot2c rot30 rot34 t50 wing54 motion mframe flags camx camy camz".split()


def start(rows):
    for i, r in enumerate(rows):
        if r["v"][0] == "3" and r["v"][7] == "4":
            return i
    raise SystemExit("no fight start")


def pace(rows):
    by = defaultdict(list)
    for a, b in zip(rows, rows[1:]):
        if a["v"][0] != b["v"][0]:
            continue
        d = math.dist([float(x) for x in a["v"][1:4]], [float(x) for x in b["v"][1:4]])
        by[a["v"][0]].append(d)
    return by


def main():
    m, o = load_mame(sys.argv[1]), load_ours(sys.argv[2])
    i, j = start(m), start(o)
    m, o = m[i:], o[j:]
    n = 0
    first = None
    for a, b in zip(m, o):
        if a["v"] != b["v"]:
            first = (a, b)
            break
        n += 1
    print(f"aligned: MAME frame {m[0]['frame']} = ours {o[0]['frame']}; {n} frames identical (all 15 fields)")
    if first:
        a, b = first
        diff = [f"{NAMES[k]} {a['v'][k]} / {b['v'][k]}" for k in range(15) if a["v"][k] != b["v"][k]]
        print(f"first difference at fight frame {n} (MAME {a['frame']}, ours {b['frame']}): " + ", ".join(diff))
    lag = sum(max(0, b["screen"] - a["screen"] - 1) for a, b in zip(m, m[1:]))
    print(f"MAME: {len(m)} fight frames, {lag} lag frames (vblanks without a game frame)")
    pm, po = pace(m), pace(o)
    print("state  frames(MAME/ours)  mean step  (MAME/ours)  max step (MAME/ours)")
    for s in sorted(set(pm) | set(po), key=int):
        a, b = pm.get(s, [0]), po.get(s, [0])
        print(f"{s:>5}  {len(a):6}/{len(b):<6}  {sum(a)/len(a):7.3f}/{sum(b)/len(b):<7.3f}  {max(a):7.3f}/{max(b):<7.3f}")


main()
