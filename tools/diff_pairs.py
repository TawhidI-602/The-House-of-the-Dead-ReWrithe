#!/usr/bin/env python3
"""
diff_pairs.py - where do the PC port and the arcade program differ? For every pair from match_functions.py, compares
the numeric constants of both functions (arcade object offsets may be 8 lower) and lists the pairs whose constants
differ, most different first: candidates for gameplay changes (timers, damage, scores, flags) and so for the
"best of both" choices in docs/decompilation.md.

  py tools/diff_pairs.py [min differing constants, default 2]
Output: private/decomp/diff_pairs.txt
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import match_functions as mf   # noqa: E402


def main():
    minimum = int(sys.argv[1]) if len(sys.argv) > 1 else 2
    pc = mf.parse(os.path.join(mf.ROOT, "pc", "thotd.c"), 1)
    arc = mf.parse(os.path.join(mf.ROOT, "arcade", "maincpu.c"), 2)
    rows = []
    for line in open(os.path.join(mf.ROOT, "match_functions.tsv"), encoding="utf-8").read().splitlines()[1:]:
        p, a = (int(x, 16) for x in line.split("\t")[:2])
        if p not in pc or a not in arc:
            continue
        pcs, acs = pc[p]["consts"], arc[a]["consts"]
        aexp = acs | {c + 8 for c in acs if 0x50 <= c < 0x400}
        pexp = pcs | {c - 8 for c in pcs if 0x58 <= c < 0x408}
        only_pc = sorted(c for c in pcs if c not in aexp)
        only_arc = sorted(c for c in acs if c not in pexp)
        n = len(only_pc) + len(only_arc)
        if n >= minimum:
            rows.append((n, p, a, only_pc, only_arc, len(pc[p]["calls"]), len(arc[a]["calls"])))
    rows.sort(key=lambda r: -r[0])
    out = os.path.join(mf.ROOT, "diff_pairs.txt")
    with open(out, "w", encoding="utf-8") as f:
        f.write("diff  PC        arcade    calls pc/arc  constants only in PC | only in arcade\n")
        for n, p, a, op, oa, cp, ca in rows:
            f.write(f"{n:4d}  {p:08x}  {a:08x}  {cp:3d}/{ca:<3d}  {' '.join(hex(c) for c in op[:12])} | "
                    f"{' '.join(hex(c) for c in oa[:12])}\n")
    print(f"{len(rows)} pairs with >= {minimum} differing constants -> {out}")


if __name__ == "__main__":
    main()
