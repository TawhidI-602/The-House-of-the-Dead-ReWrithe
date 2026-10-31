#!/usr/bin/env python3
"""
gap_pairs.py - proposes PC pairs for unpaired arcade routines by neighbour order: when an unpaired arcade routine sits
between two paired ones and exactly one unpaired PC routine sits between their PC pairs, that PC routine is the
candidate (the PC kept the arcade's source order in most files; 2026-10-12 found severed_part_free this way).
A candidate is accepted when the sizes are close (PC / arcade 0.5 .. 2.0, or both <= 8 bytes) and, if the arcade routine has paired
callers, at least half of their PC counterparts call the candidate.

  py tools/gap_pairs.py           list (accepted ones marked "ok")
  py tools/gap_pairs.py --apply   writes the accepted ones into names.tsv (only routines that already have a
                                  "func  -  <arcade>" line; unnamed FUN_ routines are left to the naming pass)
"""
import os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import check_pairs as c

ROOT = c.ROOT
NAMES = os.path.join(c.HERE, "ghidra", "names.tsv")


def main():
    A = c.index(os.path.join(ROOT, "private", "decomp", "arcade", "maincpu_index.tsv"))
    P = c.index(os.path.join(ROOT, "private", "decomp", "pc", "thotd_index.tsv"))
    arc = c.callees(os.path.join(ROOT, "private", "decomp", "arcade", "maincpu_index.tsv"))
    pc = c.callees(os.path.join(ROOT, "private", "decomp", "pc", "thotd_index.tsv"))
    allp = {}
    for line in open(NAMES, encoding="utf-8"):
        f = line.rstrip("\n").split("\t")
        if len(f) > 3 and f[0] == "func" and "-" not in (f[1], f[2]):
            allp[int(f[2], 16)] = int(f[1], 16)
    taken = set(allp.values())
    al, pl = sorted(A), sorted(P)
    accepted = {}
    for i in range(1, len(al) - 1):
        a, lo, hi = al[i], al[i - 1], al[i + 1]
        if a in allp or lo not in allp or hi not in allp:
            continue
        mid = [p for p in pl if allp[lo] < p < allp[hi]]
        if len(mid) != 1 or mid[0] in taken:
            continue
        p = mid[0]
        r = P[p][1] / max(1, A[a][1])
        cs = [x for x, cl in arc.items() if a in cl and x in allp and allp[x] in pc]
        agree = sum(p in pc[allp[x]] for x in cs)
        close = 0.5 <= r <= 2.0 or max(A[a][1], P[p][1]) <= 8  # empty states: 4-byte i960 ret vs 1-byte ret
        ok = close and (not cs or agree * 2 >= len(cs))
        print(f"{'ok ' if ok else '   '} {A[a][0]:36s} 0x{a:X} ({A[a][1]}) -> PC 0x{p:X} ({P[p][1]}) "
              f"r={r:.2f} callers {agree}/{len(cs)}")
        if ok:
            accepted[a] = p
    print(f"{len(accepted)} accepted")
    if "--apply" in sys.argv:
        lines, n = open(NAMES, encoding="utf-8").read().split("\n"), 0
        for i, line in enumerate(lines):
            f = line.split("\t")
            if len(f) > 3 and f[0] == "func" and f[1] == "-" and re.fullmatch("[0-9a-f]+", f[2]):
                a = int(f[2], 16)
                if a in accepted:
                    f[1] = f"{accepted[a]:x}"
                    if len(f) > 4:
                        f[4] += " (PC pair by neighbour order, gap_pairs.py)"
                    else:
                        f.append("(PC pair by neighbour order, gap_pairs.py)")
                    lines[i] = "\t".join(f)
                    n += 1
        open(NAMES, "w", encoding="utf-8").write("\n".join(lines))
        print(f"{n} written to names.tsv")


if __name__ == "__main__":
    main()
