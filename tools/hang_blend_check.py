"""hang_blend_check.py (hangcheck): the Hangedman as drawn with Smooth Motion (hotd --render with HOTD_INTERP_CLIP and
HOTD_TRACE_BLENDED=<file>,<his object>): per in-between picture, how far his wings are from the joint they hang on.

  py -I tools/hang_blend_check.py <blend trace> [...]

The wings (0x1401..0x1464) are drawn at joint 1's matrix just before that part's own model, so in every picture both
must have the same position; a gap means the wings were drawn from another moment than the body (not blended).
"""
import sys, math


def check(path):
    pics, cur, key = [], [], None
    for line in open(path):
        f = line.split()
        if len(f) < 7:
            continue
        k = (f[0], f[1])
        if k != key:
            if cur:
                pics.append((key, cur))
            cur, key = [], k
        cur.append((int(f[3], 16), tuple(float(x) for x in f[4:7])))
    if cur:
        pics.append((key, cur))
    gaps = []
    for (frame, alpha), items in pics:
        if alpha == "1.000":   # the tick's own picture (no blend)
            continue
        for i, (m, p) in enumerate(items):
            if 0x1401 <= m <= 0x1464 and i + 1 < len(items):
                gaps.append(math.dist(p, items[i + 1][1]))
    if not gaps:
        print(f"{path}: no in-between pictures with wings")
        return
    big = sum(1 for g in gaps if g > 0.05)
    print(f"{path}: {len(gaps)} in-between pictures with wings; wing-joint gap mean {sum(gaps)/len(gaps):.3f}, "
          f"max {max(gaps):.3f} units; {big} pictures ({100.0*big/len(gaps):.1f} %) over 0.05")


for p in sys.argv[1:]:
    check(p)
