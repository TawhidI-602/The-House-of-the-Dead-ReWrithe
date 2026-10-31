#!/usr/bin/env python3
"""dump_kind.py - compact disassembly of a kind's routines, arcade and PC, for porting (address + instruction only).

Usage:
  py tools/dump_kind.py <arcade start> <arcade end> [--pc <start> <end>] [--name <name>]

Writes private/dumps/<name>_a.txt (tools/i960dis.py over [start, end), a header per index routine) and
private/dumps/<name>_pc.txt (tools/x86dis.py: each PC index function in the PC range, cut at its index size).
Without --pc the PC range is the span of the arcade routines' PC pairs (tools/ghidra/names.tsv). <name> defaults to
the first arcade routine's name. PC notes: `+n overrun` = the last instruction runs n bytes past the index size;
`+n to the next start` = n non-padding bytes between the index end and the next index function (the size may be short).
"""
import argparse
import bisect
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import i960dis  # noqa: E402
from capture_lists import A_INDEX, PC_INDEX, MAINCPU, THOTD, read_index, read_names  # noqa: E402


FILLERS = (bytes.fromhex("8d742600"), bytes.fromhex("8d4000"), bytes.fromhex("8d7600"), bytes.fromhex("8bc0"),
           bytes.fromhex("8bff"), b"\x90", b"\x00", b"\xcc")   # Watcom / MSVC alignment fillers


def gap_lines(gap, at, f, nxt, nxt2):
    """the bytes between a PC function's index end and the next index start: fillers (skipped), jump tables whose
    entries point into this function or the next one (Watcom puts a switch's table before its function; listed),
    anything else (reported: the index size may be short)."""
    out = []
    i = 0
    while i < len(gap):
        fill = next((x for x in FILLERS if gap.startswith(x, i)), None)
        if fill:
            i += len(fill)
            continue
        entries = []
        while i + 4 <= len(gap) and f <= int.from_bytes(gap[i:i + 4], "little") < nxt2:
            entries.append(int.from_bytes(gap[i:i + 4], "little"))
            i += 4
        if len(entries) >= 2:
            whose = "the next function's" if min(entries) >= nxt else "this function's"
            out.append(f"{at + i - 4 * len(entries):08x} jump table ({whose}): " + ",".join(f"{e:x}" for e in entries))
            continue
        i -= 4 * len(entries)
        rest = len(gap) - i
        out.append(f"; +{rest} to the next start {nxt:x} (not padding or a jump table at {at + i:x}: the index "
                   f"size may be short)")
        break
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("start", type=lambda s: int(s, 16))
    ap.add_argument("end", type=lambda s: int(s, 16))
    ap.add_argument("--pc", nargs=2, type=lambda s: int(s, 16), metavar=("START", "END"))
    ap.add_argument("--name")
    args = ap.parse_args()

    aidx = read_index(A_INDEX)
    pidx = read_index(PC_INDEX)
    a2pc, pcname, aname = read_names()
    astarts = sorted(a for a in aidx if args.start <= a < args.end)
    name = args.name or (aname.get(astarts[0]) or aidx[astarts[0]][0] if astarts else f"{args.start:x}")
    out_dir = os.path.join(ROOT, "private", "dumps")
    os.makedirs(out_dir, exist_ok=True)

    # arcade
    code = open(MAINCPU, "rb").read()
    lines = [f"# arcade {args.start:#x}-{args.end:#x} (tools/i960dis.py)"]
    for a, n, txt in i960dis.dis(code, 0, args.start, args.end):
        if a in aidx:
            lines.append(f"\n{a:06x} {aname.get(a, aidx[a][0])}  ; index size {aidx[a][1]:#x}"
                         + (f", PC {a2pc[a]:x}" if a in a2pc else ""))
        lines.append(f"{a:06x} {txt}")
    pa = os.path.join(out_dir, f"{name}_a.txt")
    open(pa, "w", encoding="utf-8").write("\n".join(lines) + "\n")

    # PC
    if args.pc:
        p0, p1 = args.pc
    else:
        pcs = [a2pc[a] for a in astarts if a in a2pc]
        if not pcs:
            print(f"wrote {pa}; no PC pairs in the range (give --pc)")
            return
        p0, p1 = min(pcs), max(pcs) + max(pidx.get(max(pcs), ("", 1))[1], 1)
    import capstone
    import x86dis
    d, secs = x86dis.load(THOTD)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    pstarts_all = sorted(pidx)
    lines = [f"# PC {p0:#x}-{p1:#x} (tools/x86dis.py, each index function cut at its index size)"]
    for f in [a for a in pstarts_all if p0 <= a < p1]:
        size = pidx[f][1]
        k = bisect.bisect_right(pstarts_all, f)
        nxt = pstarts_all[k] if k < len(pstarts_all) else None
        lines.append(f"\n{f:08x} {pcname.get(f, pidx[f][0])}  ; index size {size:#x}")
        last_end = f
        for i in md.disasm(x86dis.read(d, secs, f, size + 16), f):
            if i.address >= f + size:
                break
            lines.append(f"{i.address:08x} {i.mnemonic} {i.op_str}".rstrip())
            last_end = i.address + i.size
        if last_end > f + size:
            lines.append(f"; +{last_end - f - size} overrun (the last instruction ends past the index size)")
        if nxt and nxt > last_end:
            nxt2 = pstarts_all[k + 1] if k + 1 < len(pstarts_all) else nxt + 0x10000
            lines += gap_lines(x86dis.read(d, secs, last_end, nxt - last_end), last_end, f, nxt, nxt2)
    pp = os.path.join(out_dir, f"{name}_pc.txt")
    open(pp, "w", encoding="utf-8").write("\n".join(lines) + "\n")
    print(f"wrote {pa}\nwrote {pp}")


if __name__ == "__main__":
    main()
