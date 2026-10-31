#!/usr/bin/env python3
"""
gen_cpp_types.py - generates the C++ view of the original data for the port (docs/port.md), for both layouts:
  src/port/types.h    per version (namespace a = arcade, pc = PC port) one struct per tools/ghidra/types.tsv struct
                      at that version's offsets (padding kept; pointers as 32-bit addresses: `Addr<T>`), static_assert
                      on every size; a struct missing in a version is only declared there. Then the layout traits
                      `A` and `Pc` (port/game.h: Game<P>): the struct types, G (data addresses), F (routine addresses),
                      off<Struct>(arcade offset) = the version's offset of an unnamed field (the struct's pcshift)
  src/port/globals.h  per version: struct G (constexpr addresses of the named data of names.tsv that exist in it) and
                      struct F (every named routine; 0 where the version has none)
Run after editing types.tsv / names.tsv (part of the START HERE item 5 chain).
"""
import os
import re

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
TYPES = os.path.join(ROOT, "tools", "ghidra", "types.tsv")
NAMES = os.path.join(ROOT, "tools", "ghidra", "names.tsv")
OUT_T = os.path.join(ROOT, "src", "port", "types.h")
OUT_G = os.path.join(ROOT, "src", "port", "globals.h")

BASE = {"int": ("int32_t", 4), "uint": ("uint32_t", 4), "short": ("int16_t", 2), "ushort": ("uint16_t", 2),
        "char": ("int8_t", 1), "byte": ("uint8_t", 1), "float": ("float", 4), "code": ("uint32_t", 4)}
KEYWORDS = {"delete", "new", "default", "switch", "case", "this", "class", "template", "register", "auto", "union"}


def ident(s):
    s = re.sub(r"\W", "_", s)
    if s in KEYWORDS:
        s += "_"
    return "_" + s if s[:1].isdigit() else s


def read_types():
    structs, order, extends, shift = {}, [], {}, {}
    for line in open(TYPES, encoding="utf-8"):
        if line.startswith("#") or not line.strip():
            continue
        c = line.rstrip("\n").split("\t")
        if c[0] == "struct":
            pcs = None if c[3] == "-" else int(c[3], 16)
            structs[c[1]] = {"size": (int(c[2], 16), pcs), "fields": [], "doc": c[4] if len(c) > 4 else ""}
            order.append(c[1])
        elif c[0] == "extends":
            extends[c[1]] = c[2]
        elif c[0] == "pcshift":
            shift[c[1]] = (int(c[2], 16), int(c[3], 16))
        elif c[0] == "field":
            structs[c[1]]["fields"].append((c[2], c[3], c[4], c[5], c[6] if len(c) > 6 else ""))
    for s, b in extends.items():
        # the struct's own fields replace the base's at the same arcade offset (PC-only fields: by name)
        mine = {f[0] for f in structs[s]["fields"] if f[0] != "-"} | {f[3] for f in structs[s]["fields"]}
        structs[s]["fields"] += [f for f in structs[b]["fields"] if (f[0] if f[0] != "-" else f[3]) not in mine]
        if s not in shift and b in shift:
            shift[s] = shift[b]
    return structs, order, shift


def offsets(f, sname, shift, pc):
    """the field's offset in this version, or None"""
    a, p = f[0], f[1]
    if not pc:
        return None if a == "-" else int(a, 16)
    if p == "-":
        return None
    if p == "=":
        o = int(a, 16)
        fr, by = shift.get(sname, (0, 0))
        return o + by if o >= fr and sname in shift else o
    return int(p, 16)


def gen_structs(structs, order, shift, pc):
    def ctype(t):
        m = re.match(r"^(\w+)(\**)(?:\[(\d+)\])?$", t)
        base, ptr, n = m.group(1), m.group(2), int(m.group(3) or 1)
        if ptr:
            inner = base + ptr[:-1]
            target = BASE[inner][0] if inner in BASE else (inner if inner in structs and not ptr[1:] else "void")
            return "Addr<%s>" % target, 4, n
        if base in BASE:
            return BASE[base][0], BASE[base][1], n
        if base in structs:
            return base, structs[base]["size"][1 if pc else 0], n
        raise SystemExit("unknown type " + t)

    ns = "pc" if pc else "a"
    out = ["namespace %s {" % ns, "", "#pragma pack(push, 1)"]
    for s in order:
        out.append("struct %s;" % s)
    out.append("")
    for s in order:
        st = structs[s]
        size = st["size"][1 if pc else 0]
        if size is None:
            continue
        out.append("// %s" % st["doc"][:110])
        out.append("struct %s {" % s)
        pos = 0
        fl = []
        for f in st["fields"]:
            o = offsets(f, s, shift, pc)
            if o is not None:
                fl.append((o, f[2], f[3], f[4]))
        for off, t, name, doc in sorted(fl):
            ty, es, n = ctype(t)
            if off < pos:
                out.append("    // (overlaps) +0x%X %s %s" % (off, t, name))
                continue
            if off > pos:
                out.append("    uint8_t pad_%x[0x%X];" % (pos, off - pos))
            arr = "[%d]" % n if n > 1 else ""
            out.append("    %s %s%s;   // +0x%X%s" % (ty, ident(name), arr, off, (" " + doc[:90]) if doc else ""))
            pos = off + es * n
        if pos < size:
            out.append("    uint8_t pad_%x[0x%X];" % (pos, size - pos))
        out.append("};")
        out.append("static_assert(sizeof(%s) == 0x%X);" % (s, size))
        out.append("")
    out += ["#pragma pack(pop)", "", "} // namespace %s" % ns, ""]
    return out


def gen_traits(structs, order, shift, pc):
    ns = "pc" if pc else "a"
    t = "Pc" if pc else "A"
    out = ["// layout traits of the %s" % ("PC port (PA / PM)" if pc else "arcade (A)"), "struct %s {" % t,
           "    static constexpr bool pc = %s;" % ("true" if pc else "false")]
    for s in order:
        out.append("    using %s = %s::%s;" % (s, ns, s))
    out += ["    using G = %s::G;" % ns, "    using F = %s::F;" % ns,
            "    // the version's offset of a field given by its arcade offset (unnamed fields; named ones: the structs)",
            "    template <class S> static constexpr uint32_t off(uint32_t a);", "};"]
    for s in order:
        fr, by = shift.get(s, (0, 0))
        body = "return a >= 0x%X ? a + 0x%X : a;" % (fr, by) if pc and s in shift else "return a;"
        out.append("template <> constexpr uint32_t %s::off<%s::%s>(uint32_t a) { %s }" % (t, ns, s, body))
    out.append("")
    return out


def main():
    structs, order, shift = read_types()
    out = ["// GENERATED by tools/gen_cpp_types.py from tools/ghidra/types.tsv: do not edit.",
           "// The original data layouts byte for byte, arcade (namespace a) and PC port (namespace pc); pointers are",
           "// 32-bit addresses (Addr<T>). A / Pc: the layout traits Game<P> is instantiated with (docs/port.md).",
           "#pragma once", "", "#include \"port/globals.h\"", "#include \"port/mem.h\"", "", "#include <cstdint>", "",
           "namespace hotd::port {", ""]
    out += gen_structs(structs, order, shift, False)
    out += gen_structs(structs, order, shift, True)
    out += gen_traits(structs, order, shift, False)
    out += gen_traits(structs, order, shift, True)
    out += ["// the struct types of the layout P inside Game<P>",
            "#define HOTD_LAYOUT_TYPES \\"]
    out += ["    using %s = typename P::%s; \\" % (s, s) for s in order]
    out += ["    using G = typename P::G; \\", "    using F = typename P::F;", "",
            "} // namespace hotd::port", ""]
    open(OUT_T, "w", encoding="utf-8", newline="\n").write("\n".join(out))

    data = {"a": [], "pc": []}
    funcs = {}
    seen = {"a": set(), "pc": set()}
    for line in open(NAMES, encoding="utf-8"):
        c = line.rstrip("\n").split("\t")
        if len(c) < 4 or c[0] not in ("data", "func"):
            continue
        n = ident(c[3])
        if c[0] == "func":
            f = funcs.setdefault(n, [0, 0])
            if c[2] not in ("-", "") and not f[0]:
                f[0] = int(c[2], 16)
            if c[1] not in ("-", "") and not f[1]:
                f[1] = int(c[1], 16)
            continue
        for ns, col in (("a", 2), ("pc", 1)):
            if c[col] in ("-", "") or n in seen[ns]:
                continue
            seen[ns].add(n)
            data[ns].append("    static constexpr uint32_t %s = 0x%X;" % (n, int(c[col], 16)))
    g = ["// GENERATED by tools/gen_cpp_types.py from tools/ghidra/names.tsv: do not edit.",
         "// Addresses of the named data (G: only what exists in that version) and routines (F: 0 = not in that",
         "// version), arcade (namespace a) and PC port (namespace pc).", "#pragma once", "", "#include <cstdint>", "",
         "namespace hotd::port {", ""]
    for ns, i in (("a", 0), ("pc", 1)):
        g += ["namespace %s {" % ns, "struct G {"] + data[ns] + ["};", "struct F {"]
        g += ["    static constexpr uint32_t %s = 0x%X;" % (n, v[i]) for n, v in funcs.items()]
        g += ["};", "} // namespace %s" % ns, ""]
    g += ["} // namespace hotd::port", ""]
    open(OUT_G, "w", encoding="utf-8", newline="\n").write("\n".join(g))
    print("types.h: %d structs; globals.h: %d / %d data, %d routines" % (len(order), len(seen["a"]), len(seen["pc"]),
                                                                       len(funcs)))


if __name__ == "__main__":
    main()
