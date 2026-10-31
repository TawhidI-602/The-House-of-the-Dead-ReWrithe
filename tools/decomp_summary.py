#!/usr/bin/env python3
"""
decomp_summary.py - a quick digest of decompiled arcade functions, for reading state routines in bulk: motions set
(from the machine code: the literal in g1 before each set_motion call, with the developers' MO_ names), sound cues,
writes to chosen object fields (default +0x5C state, +0x60 sub-state, +0x64 action), float constants, and the
functions called (named and unnamed).

  py tools/decomp_summary.py <arcade address hex> [...] [--fields 5c,60]
"""
import contextlib, io, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import decomp_query
import i960dis

MAINCPU = os.path.join(HERE, "..", "private", "arcade_exports", "regions", "maincpu.bin")
MOTION_CALLS = {0x39180: "set_motion", 0x381D0: "set_motion_blend", 0x38A20: "set_motion_blend_hold"}
SKIP = ("FUN_", "SUB", "CONCAT", "ZEXT", "SEXT", "if", "while", "switch", "for", "return", "sizeof", "code", "float",
        "int", "uint", "short", "char", "double", "undefined", "byte", "ushort", "ulonglong", "longlong", "bool",
        "POPCOUNT", "CARRY", "SBORROW", "NAN", "ABS", "SQRT")

MOT = {}
_p = os.path.join(HERE, "..", "private", "decomp", "names_bodies_motions.txt")
if os.path.exists(_p):
    for _line in open(_p, encoding="utf-8"):
        _m = re.match(r"motion\s+(\d+) 0x[0-9a-f]+ (\S+)", _line)
        if _m:
            MOT[int(_m.group(1))] = _m.group(2)


def disasm(start, size):
    buf = io.StringIO()
    old = sys.argv
    sys.argv = ["i960dis.py", MAINCPU, f"{start:x}", f"{start + size:x}"]
    try:
        with contextlib.redirect_stdout(buf):
            i960dis.main()
    finally:
        sys.argv = old
    return buf.getvalue().splitlines()


def motion_calls(start, size):
    """The last literal moved into g1 before each call to a set_motion routine (Ghidra often drops the arguments)."""
    out, g1, src = [], None, None
    for line in disasm(start, size):
        if line.endswith(", g1"):
            m = re.search(r"(?:mov|lda) (0x[0-9a-f]+|\d+), g1$", line)
            g1 = int(m.group(1), 0) if m else None
            m2 = re.search(r"addo (\d+), (\d+), g1$", line)          # literals above 31: addo 31, n, g1
            m3 = re.search(r"setbit (\d+), 0, g1$", line)
            if m2:
                g1 = int(m2.group(1)) + int(m2.group(2))
            elif m3:
                g1 = 1 << int(m3.group(1))
            src = None if g1 is not None else line.split("  ", 1)[-1].strip()
        m = re.search(r"call 0x([0-9a-f]+)", line)
        if m:
            t = int(m.group(1), 16)
            if t in MOTION_CALLS:
                if g1 is not None:
                    what = f"{hex(g1)} = {MOT.get(g1, '?')}"
                else:
                    what = f"from `{src}`" if src else "?"
                out.append(f"  {line.split(':')[0]} {MOTION_CALLS[t]} {what}")
            g1 = src = None
    return out


def digest(text, fields):
    out = []
    cues = sorted(set(re.findall(r"sound_cue\((0x[0-9a-f]+)\)", text)))
    if cues:
        out.append("  sound: " + " ".join(cues))
    for f in fields:
        vals = re.findall(r"\(param_1 \+ 0x%s\)\s*=\s*([^;{]+);" % f, text)
        if vals:
            out.append(f"  +0x{f} = " + " | ".join(sorted(set(v.strip() for v in vals))))
    calls = sorted(set(c for c in re.findall(r"\b([A-Za-z_]\w+)\(", text) if not c.startswith(SKIP)))
    funs = sorted(set(re.findall(r"\b(FUN_[0-9a-f]+)\(", text)))
    if calls:
        out.append("  calls: " + " ".join(calls))
    if funs:
        out.append("  unnamed: " + " ".join(funs))
    fl = sorted(set(re.findall(r"(?<![\w.])(\d+\.\d+)(?![\w.])", text)), key=float)
    if fl:
        out.append("  floats: " + " ".join(fl[:30]))
    return out


def main():
    args = sys.argv[1:]
    fields = ["5c", "60", "64"]
    if "--fields" in args:
        i = args.index("--fields")
        fields = args[i + 1].split(",")
        del args[i:i + 2]
    funcs, index = decomp_query.load("arcade")
    for a in args:
        a = int(a, 16)
        for f in funcs:
            if f[0] <= a < f[0] + max(f[2], 1):
                text = "".join(f[3])
                cmt = re.search(r"/\* ((?!WARNING).*?) \*/", text)
                print(f"{f[1]} @ 0x{f[0]:x} ({f[2]} bytes){' - ' + cmt.group(1) if cmt else ''}")
                print("\n".join(digest(text, fields) + motion_calls(f[0], f[2])))
                break
        else:
            print(f"0x{a:x}: no function")


if __name__ == "__main__":
    main()
