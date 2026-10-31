#!/usr/bin/env python3
"""
compare_switch.py - compares a paired routine's switch statement in the arcade and PC Ghidra exports, case by case:
the routines each case installs (`routine = X`, `pcVarN = X`) and the motion numbers it sets (`set_motion*(obj, N)`,
`uVarN = N`). A case whose routine names differ points at a wrong or missing pair in tools/ghidra/names.tsv (found
4 among the guard actions on 2026-10-13); differing motion lists are often only the decompiler's way of writing
the same code, so read those cases.

  py tools/compare_switch.py <arcade name> [<PC name>]      (the PC name defaults to the arcade name)
Exports: private/decomp/arcade/maincpu.c and private/decomp/pc/thotd.c (tools\\ghidra\\run_*.ps1 -Reexport).
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ARCADE = os.path.join(HERE, "..", "private", "decomp", "arcade", "maincpu.c")
PC = os.path.join(HERE, "..", "private", "decomp", "pc", "thotd.c")


def body(path, name):
    t = open(path, encoding="utf-8", errors="replace").read()
    i = t.index("// ==== " + name + " @")
    j = t.find("\n// ==== ", i + 10)
    return t[i:j if j > 0 else len(t)]


def cases(src):
    out, cur = {}, None
    for line in src.split("\n"):
        m = re.match(r"\s*case (0x[0-9a-f]+|\d+):", line)
        if m:
            cur = int(m.group(1), 0)
            out.setdefault(cur, set())
            continue
        if cur is None:
            continue
        for r in re.findall(r"routine = (\w+)|pcVar\d+ = (\w+)", line):
            out[cur].add("R:" + next(x for x in r if x))
        for mo in re.findall(r"set_motion\w*\(\(Obj \*\)\w+,(0x[0-9a-f]+|\d+)", line):
            out[cur].add("M:%#x" % int(mo, 0))
        for u in re.findall(r"uVar\d+ = (0x[0-9a-f]+|\d+);", line):
            out[cur].add("M?:%#x" % int(u, 0))
    return out


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return
    an = sys.argv[1]
    pn = sys.argv[2] if len(sys.argv) > 2 else an
    a, p = cases(body(ARCADE, an)), cases(body(PC, pn))
    for k in sorted(set(a) | set(p)):
        ra = sorted(x for x in a.get(k, ()) if x.startswith("R:"))
        rp = sorted(x for x in p.get(k, ()) if x.startswith("R:"))
        ma = sorted(x for x in a.get(k, ()) if x.startswith("M"))
        mp = sorted(x for x in p.get(k, ()) if x.startswith("M"))
        note = "" if ra == rp and ma == mp else ("  <-- routines differ" if ra != rp else "  (motions written differently?)")
        print("case %d: A %s %s | PC %s %s%s" % (k, ra, ma, rp, mp, note))


if __name__ == "__main__":
    main()
