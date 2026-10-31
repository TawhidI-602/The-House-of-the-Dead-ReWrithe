#!/usr/bin/env python3
"""
check_pairs.py - sanity check of the hand pairs in tools/ghidra/names.tsv: lists the pairs whose PC function is much
smaller or much larger than the arcade one (by the sizes in the Ghidra indexes). A wrong pair shows up like this
(2026-10-21: the arcade enemy_ai, 4132 bytes, was paired with a 43-byte PC matrix push). Not every hit is wrong:
tiny PC sizes can be Ghidra boundaries that need a look, and the PC sometimes inlines what the arcade calls.

  py tools/check_pairs.py [low ratio, default 0.3] [high ratio, default 3.5]
  py tools/check_pairs.py --callers   caller agreement instead: for each hand pair, how many of the arcade routine's
                                       paired callers have a PC counterpart that calls the PC routine (2026-10-12: found
                                       return_false paired with the wrong PC routine, 0 of 17). Lists pairs with at
                                       least 3 paired callers and under half agreeing.
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")


def index(path):
    d = {}
    for line in open(path, encoding="utf-8", errors="replace"):
        f = line.split("\t")
        if len(f) > 2 and re.fullmatch("[0-9a-f]{8}", f[0]):
            d[int(f[0], 16)] = (f[1], int(f[2]))
    return d


def callees(path):
    d = {}
    for line in open(path, encoding="utf-8", errors="replace"):
        f = line.rstrip("\n").split("\t")
        if len(f) > 5 and re.fullmatch("[0-9a-f]{8}", f[0]):
            d[int(f[0], 16)] = {int(x, 16) for x in f[5].split(",") if re.fullmatch("[0-9a-f]{8}", x)}
    return d


def hand_pairs():
    gen, pairs = False, {}
    for line in open(os.path.join(HERE, "ghidra", "names.tsv"), encoding="utf-8"):
        if line.startswith("# BEGIN name_routines"):
            gen = True
        elif line.startswith("# END name_routines"):
            gen = False
        c = line.rstrip("\n").split("\t")
        if not gen and c[0] == "func" and len(c) >= 4 and "-" not in (c[1], c[2]):
            pairs[int(c[2], 16)] = (int(c[1], 16), c[3])
    return pairs


def caller_check():
    arc = callees(os.path.join(ROOT, "private", "decomp", "arcade", "maincpu_index.tsv"))
    pc = callees(os.path.join(ROOT, "private", "decomp", "pc", "thotd_index.tsv"))
    pairs, out = hand_pairs(), []
    taken = {p for p, _ in pairs.values()}
    # pairs read by hand and found right despite few agreeing callers (PC inlining etc.): "pair checked" in the comment
    checked = set()
    for line in open(os.path.join(HERE, "ghidra", "names.tsv"), encoding="utf-8"):
        c = line.rstrip("\n").split("\t")
        if c[0] == "func" and len(c) > 4 and "pair checked" in c[4] and c[2] != "-":
            checked.add(int(c[2], 16))
    for a, (p, name) in pairs.items():
        cs = [c for c, cl in arc.items() if a in cl and c in pairs and pairs[c][0] in pc]
        if len(cs) < 3:
            continue
        ok = sum(p in pc[pairs[c][0]] for c in cs)
        if ok * 2 < len(cs) and a not in checked:
            # candidate: the PC routine called by most of the callers' PC counterparts (not one paired elsewhere)
            votes = {}
            for c in cs:
                for x in pc[pairs[c][0]] - taken:
                    votes[x] = votes.get(x, 0) + 1
            best = max(votes, key=votes.get) if votes else 0
            out.append((ok / len(cs), name, a, p, ok, len(cs), best, votes.get(best, 0)))
    for r, n, a, p, ok, tot, best, bv in sorted(out):
        print(f"{ok:3d}/{tot:<3d} {n:40s} arcade 0x{a:X}  PC 0x{p:X}  most called: PC 0x{best:X} ({bv})")
    print(f"{len(out)} pairs with under half of the paired callers agreeing")


def main():
    if "--callers" in sys.argv:
        return caller_check()
    lo = float(sys.argv[1]) if len(sys.argv) > 1 else 0.3
    hi = float(sys.argv[2]) if len(sys.argv) > 2 else 3.5
    arc = index(os.path.join(ROOT, "private", "decomp", "arcade", "maincpu_index.tsv"))
    pc = index(os.path.join(ROOT, "private", "decomp", "pc", "thotd_index.tsv"))
    gen, out = False, []
    for line in open(os.path.join(HERE, "ghidra", "names.tsv"), encoding="utf-8"):
        if line.startswith("# BEGIN name_routines"):
            gen = True
        elif line.startswith("# END name_routines"):
            gen = False
        c = line.rstrip("\n").split("\t")
        if gen or c[0] != "func" or len(c) < 4 or "-" in (c[1], c[2]):
            continue
        a, p = int(c[2], 16), int(c[1], 16)
        if a in arc and p in pc:
            sa, sp = arc[a][1], pc[p][1]
            if max(sa, sp) >= 48 and not lo <= sp / sa <= hi:
                out.append((sp / sa, c[3], a, sa, p, sp))
    for r, n, a, sa, p, sp in sorted(out):
        print(f"{r:6.2f}  {n:40s} arcade 0x{a:X} ({sa})  PC 0x{p:X} ({sp})")
    print(f"{len(out)} pairs to look at")


if __name__ == "__main__":
    main()
