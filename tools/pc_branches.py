#!/usr/bin/env python3
"""
pc_branches.py - the conditional jumps of PC routines, for the PC harness's branch coverage (docs/proof.md "Branch
coverage"): one line per jcc / jcxz / loop* instruction: routine, address, target, fall-through (hex).
Sizes come from the PC index (private/decomp/pc/thotd_index.tsv) unless given as addr:size.

Usage: py tools/pc_branches.py <THOTD.EXE> <out.txt> <addr[:size]>,...
"""
import os, sys
import capstone

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from x86dis import load, read   # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INDEX = os.path.join(ROOT, "private", "decomp", "pc", "thotd_index.tsv")


def main():
    exe, out, spec = sys.argv[1], sys.argv[2], sys.argv[3]
    sizes = {}
    if os.path.exists(INDEX):
        for line in open(INDEX, encoding="utf-8", errors="replace"):
            c = line.split("\t")
            if len(c) > 2 and c[2].isdigit():
                sizes[int(c[0], 16)] = int(c[2])
    d, secs = load(exe)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    lines = []
    for item in spec.split(","):
        if not item:
            continue
        a, _, sz = item.partition(":")
        a = int(a, 16)
        n = int(sz, 16) if sz else sizes.get(a, 0)
        for i in md.disasm(read(d, secs, a, n), a):
            if capstone.x86.X86_GRP_JUMP in i.groups and i.mnemonic != "jmp" and i.operands and \
                    i.operands[0].type == capstone.x86.X86_OP_IMM:
                lines.append("%x\t%x\t%x\t%x" % (a, i.address, i.operands[0].imm, i.address + i.size))
    open(out, "w").write("\n".join(lines) + "\n")
    print("pc_branches.py: %d conditional jumps" % len(lines))


if __name__ == "__main__":
    main()
