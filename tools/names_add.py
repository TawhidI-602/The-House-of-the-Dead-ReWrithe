#!/usr/bin/env python3
"""
names_add.py - add or update hand names in tools/ghidra/names.tsv from a small batch file, with the PC address taken
from the matcher's pairs (private/decomp/match_functions.tsv) when the arcade function is paired.

Batch file lines (tab-separated; '#' lines become a section header above the new entries):
  func  <arcade hex>  <name>  [comment]
  data  <pc hex or ->  <arcade hex>  <name>  [comment]          (data: give both addresses)
  pcfunc  <pc hex>  <name>  [comment]                              (PC-only function)
An existing hand line for the same arcade address is replaced in place; new ones go at the end of the hand block.
Then run tools/name_routines.py to refresh the structural names.

  py tools/names_add.py <batch.tsv>
"""
import os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
NAMES = os.path.join(HERE, "ghidra", "names.tsv")
MATCH = os.path.join(HERE, "..", "private", "decomp", "match_functions.tsv")
BEGIN = "# BEGIN name_routines.py"


def main():
    pc_of = {}
    for line in open(MATCH, encoding="utf-8").read().splitlines()[1:]:
        f = line.split("\t")
        pc_of[int(f[1], 16)] = f[0].lstrip("0") or "0"
    new, header = [], None
    for line in open(sys.argv[1], encoding="utf-8").read().splitlines():
        if not line.strip():
            continue
        if line.startswith("#"):
            header = line
            new.append((None, line))
            continue
        f = line.split("\t")
        if f[0] == "func":
            a = int(f[1], 16)
            pc = pc_of.get(a, "-")
            new.append((("func", a), "\t".join(["func", pc, f"{a:x}", f[2]] + f[3:4])))
        elif f[0] == "pcfunc":
            new.append((("pcfunc", int(f[1], 16)), "\t".join(["func", f"{int(f[1], 16):x}", "-", f[2]] + f[3:4])))
        elif f[0] == "data":
            new.append((("data", int(f[2], 16)), "\t".join(["data", f[1], f"{int(f[2], 16):x}", f[3]] + f[4:5])))
        else:
            sys.exit(f"bad line: {line}")
    lines = open(NAMES, encoding="utf-8").read().splitlines()
    end = next(i for i, l in enumerate(lines) if l.startswith(BEGIN))
    where = {}
    for i, l in enumerate(lines[:end]):
        c = l.split("\t")
        if c[0] in ("func", "data") and len(c) > 3 and c[2] != "-":
            where[(c[0], int(c[2], 16))] = i
        elif c[0] == "func" and len(c) > 3 and c[1] != "-":
            where[("pcfunc", int(c[1], 16))] = i
    append, replaced = [], 0
    for key, text in new:
        if key is not None and key in where:
            lines[where[key]] = text
            replaced += 1
        else:
            append.append(text)
    # drop headers with nothing under them
    out = [t for k, t in enumerate(append) if not (t.startswith("#") and (k + 1 == len(append) or append[k + 1].startswith("#")))]
    lines[end:end] = out
    open(NAMES, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")
    print(f"replaced {replaced}, added {sum(1 for t in out if not t.startswith('#'))}")


if __name__ == "__main__":
    main()
