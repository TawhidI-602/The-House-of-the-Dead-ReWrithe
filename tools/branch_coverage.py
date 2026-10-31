#!/usr/bin/env python3
"""
branch_coverage.py - which conditional branches of the proven routines ran both ways in the captures (docs/proof.md
"Branch coverage"). Input: the coverage.txt files that tools\\run_capture.ps1 writes next to each capture (one line per
conditional branch of every -Ported routine: whether its target edge and its fall-through edge ran while a capture
was open). Several files are merged (an edge counts if it ran in any of them).

Usage:
  py tools/branch_coverage.py <coverage.txt or capture folder> [...]            summary per routine
  py tools/branch_coverage.py --edges <...>                                     plus every edge that never ran
  py tools/branch_coverage.py --routine 1ca50 <...>                             only these routines (comma list)

"Proven" for a routine means the paths that ran: an edge that never ran is unproven even when the capture matched.
"""
import os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NAMES = os.path.join(ROOT, "tools", "ghidra", "names.tsv")
COBR = {0x00: "jcc (PC)", 0x10: "bno", 0x11: "bg", 0x12: "be", 0x13: "bge", 0x14: "bl", 0x15: "bne", 0x16: "ble", 0x17: "bo",
        0x30: "bbc", 0x31: "cmpobg", 0x32: "cmpobe", 0x33: "cmpobge", 0x34: "cmpobl", 0x35: "cmpobne", 0x36: "cmpoble",
        0x37: "bbs", 0x38: "cmpibno", 0x39: "cmpibg", 0x3A: "cmpibe", 0x3B: "cmpibge", 0x3C: "cmpibl", 0x3D: "cmpibne",
        0x3E: "cmpible", 0x3F: "cmpibo"}


def names():
    n = {}
    for line in open(NAMES, encoding="utf-8"):
        f = line.rstrip("\n").split("\t")
        if len(f) >= 4 and f[0] == "func":
            for col in (2, 1):   # arcade and PC addresses (PC coverage files name PC routines)
                try:
                    n.setdefault(int(f[col], 16), f[3])
                except ValueError:
                    pass
    return n


def main():
    args = sys.argv[1:]
    edges = "--edges" in args
    only = None
    if "--routine" in args:
        i = args.index("--routine")
        only = {int(x, 16) for x in args[i + 1].split(",")}
        del args[i:i + 2]
    files = [a for a in args if not a.startswith("--")]
    if not files:
        print(__doc__)
        return 2
    br = {}   # branch address -> [routine, op, target, fall, taken, fell]
    for p in files:
        if os.path.isdir(p):
            p = os.path.join(p, "coverage.txt")
        for line in open(p):
            if line.startswith("#"):
                continue
            r, at, op, t, fa, tk, fl = line.split()
            at = int(at, 16)
            e = br.setdefault(at, [int(r, 16), int(op, 16), int(t, 16), int(fa, 16), 0, 0])
            e[4] |= int(tk)
            e[5] |= int(fl)
    nm = names()
    per = {}
    for at, e in sorted(br.items()):
        if only and e[0] not in only:
            continue
        per.setdefault(e[0], []).append((at, e))
    total = [0, 0, 0, 0]
    for r, lst in sorted(per.items()):
        both = sum(1 for _, e in lst if e[4] and e[5])
        one = sum(1 for _, e in lst if e[4] != e[5])
        none = sum(1 for _, e in lst if not e[4] and not e[5])
        total = [total[0] + len(lst), total[1] + both, total[2] + one, total[3] + none]
        print(f"{r:x} {nm.get(r, '?')}: {len(lst)} branches, {both} both ways, {one} one way, {none} never reached")
        if edges:
            for at, e in lst:
                if e[4] and e[5]:
                    continue
                miss = "neither edge" if not e[4] and not e[5] else \
                       f"never to {e[2]:#x} (taken)" if not e[4] else f"never to {e[3]:#x} (not taken)"
                print(f"    {at:#x} {COBR.get(e[1], hex(e[1]))}: {miss}")
    print(f"total: {total[0]} branches, {total[1]} both ways, {total[2]} one way, {total[3]} never reached")
    return 0


if __name__ == "__main__":
    sys.exit(main())
