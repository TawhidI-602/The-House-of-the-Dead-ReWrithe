#!/usr/bin/env python3
"""
ghidra_to_cpp.py - first C++ drafts of arcade routines for the port (docs/port.md), from the typed Ghidra C
(private/decomp/arcade/maincpu.c). A draft is a starting point, never the proof: it is fixed by hand against the
disassembly and then proven with captures (docs/proof.md).

What it does: the signature becomes `template <class P> RET Game<P>::name(uint32_t obj, ...)` (struct pointers become
32-bit addresses); `p->field` of a typed struct parameter becomes `mem.at<Struct>(p).field`; named globals become
`gv<T>(G::name)` (T from types.tsv / names.tsv when known, else int32_t: check it); a routine name used as a value
becomes `F::name`; `DAT_00xxxxxx` stays as a raw address with a TODO; casts to struct pointers are dropped; the rest is
copied with a `// TODO draft` marker where it could not be translated (table expressions, ternaries of pointers ...).

Usage: py tools/ghidra_to_cpp.py <name>[,<name>...] [--out file]     (names as in tools/ghidra/names.tsv)
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
C = os.path.join(ROOT, "private", "decomp", "arcade", "maincpu.c")
NAMES = os.path.join(ROOT, "tools", "ghidra", "names.tsv")
TYPES = os.path.join(ROOT, "tools", "ghidra", "types.tsv")
STRUCTS = {"Obj", "Enemy", "PlayerTask", "Player", "BodyPart", "ShotRay", "SpawnRecord", "AttackSlot"}
CT = {"int": "int32_t", "uint": "uint32_t", "short": "int16_t", "ushort": "uint16_t", "char": "int8_t",
      "byte": "uint8_t", "float": "float", "undefined4": "uint32_t", "bool": "bool"}


def load_names():
    funcs, data = set(), set()
    for line in open(NAMES, encoding="utf-8"):
        c = line.rstrip("\n").split("\t")
        if len(c) >= 4 and c[2] not in ("-", ""):
            (funcs if c[0] == "func" else data if c[0] == "data" else set()).add(c[3])
    gtypes = {}
    # global types from types.tsv ("global <pc> <arcade> <type>") matched by the arcade address of the names
    addr_name = {}
    for line in open(NAMES, encoding="utf-8"):
        c = line.rstrip("\n").split("\t")
        if len(c) >= 4 and c[0] == "data" and c[2] not in ("-", ""):
            addr_name[c[2].lower()] = c[3]
    for line in open(TYPES, encoding="utf-8"):
        c = line.rstrip("\n").split("\t")
        if len(c) >= 4 and c[0] == "global" and c[2].lower() in addr_name:
            t = re.sub(r"\[.*", "", c[3])
            gtypes[addr_name[c[2].lower()]] = CT.get(t, "uint32_t" if t.endswith("*") else t)
    return funcs, data, gtypes


def bodies(names):
    src = open(C, encoding="utf-8", errors="replace").read()
    out = {}
    for n in names:
        m = re.search(r"^// ==== %s @ ([0-9a-f]+) \((\d+) bytes\)\n(.*?)(?=^// ==== |\Z)" % re.escape(n), src, re.M | re.S)
        if m:
            out[n] = (int(m.group(1), 16), int(m.group(2)), m.group(3))
    return out


def convert(name, addr, size, text, funcs, data, gtypes):
    lines = text.strip("\n").split("\n")
    doc = [l for l in lines if l.startswith("/*")]
    sig_i = next(i for i, l in enumerate(lines) if re.match(r"^\w[\w\s\*]*\b%s\s*\(" % re.escape(name), l))
    sig = lines[sig_i]
    m = re.match(r"^(\w+)\s+(\*?)\s*%s\s*\((.*)\)" % re.escape(name), sig)
    ret = "void" if not m or m.group(1) == "void" else CT.get(m.group(1), "uint32_t")
    params, ptypes = [], {}
    if m and m.group(3).strip() not in ("", "void"):
        for p in m.group(3).split(","):
            p = p.strip()
            pm = re.match(r"^(\w+)\s*(\*?)\s*(\w+)$", p)
            if not pm:
                params.append("uint32_t /* TODO %s */" % p)
                continue
            t, ptr, pn = pm.groups()
            if ptr:
                params.append("uint32_t " + pn)
                if t in STRUCTS:
                    ptypes[pn] = t
            else:
                params.append("%s %s" % (CT.get(t, t), pn))
    body = "\n".join(lines[sig_i + 1:])
    # struct field access of typed parameters
    for pn, t in ptypes.items():
        body = re.sub(r"\b%s->" % pn, "mem.at<%s>(%s)." % (t, pn), body)
        body = re.sub(r"\(\((Enemy|Obj) \*\)%s\)->" % pn, "mem.at<\\1>(%s)." % pn, body)
    body = re.sub(r"\((?:Enemy|Obj|BodyPart|Player|PlayerTask) \*\)", "", body)
    # routine names used as values
    def fval(mm):
        n = mm.group(1)
        return "F::" + n if n in funcs else n
    body = re.sub(r"(?<![\w.>])(\w+)(?=\s*;)", fval, body)
    body = re.sub(r"= (\w+);", lambda mm: "= F::%s;" % mm.group(1) if mm.group(1) in funcs else mm.group(0), body)
    # globals
    def gval(mm):
        n = mm.group(1)
        if n in data:
            return "gv<%s>(G::%s)" % (gtypes.get(n, "int32_t"), n)
        return n
    body = re.sub(r"(?<![\w.>:])([a-z_]\w*)(?![\w(])", gval, body)
    body = re.sub(r"\bDAT_00([0-9a-f]{6})\b", lambda mm: "/* TODO name */ 0x%s" % mm.group(1).upper(), body)
    head = "\n".join("// " + d.strip("/* ").rstrip(" */") for d in doc)
    return "%s\n// 0x%X (%d bytes): draft from the Ghidra C (tools/ghidra_to_cpp.py), to fix and prove\ntemplate <class P> %s Game<P>::%s(%s)\n%s\n" % (
        head, addr, size, ret, name, ", ".join(params), body.strip("\n"))


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    names = sys.argv[1].split(",")
    out = sys.argv[sys.argv.index("--out") + 1] if "--out" in sys.argv else None
    funcs, data, gtypes = load_names()
    b = bodies(names)
    text = []
    for n in names:
        if n not in b:
            text.append("// %s: not found in maincpu.c\n" % n)
            continue
        text.append(convert(n, *b[n], funcs, data, gtypes))
    s = "\n".join(text)
    if out:
        open(out, "w", encoding="utf-8", newline="\n").write(s)
        print("%d drafts -> %s" % (len(b), out))
    else:
        print(s)
    return 0


if __name__ == "__main__":
    sys.exit(main())
