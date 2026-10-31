#!/usr/bin/env python3
"""
name_routines.py - structural names for the arcade routines that hang off named code (and their PC pairs).

Most of the arcade program is per-frame routines that objects install for themselves (object +0x00, docs/decompilation.md
"Arcade object structure") or that a dispatcher picks from a table (state tables indexed by an object field, e.g. the
action code of an enemy record at +0x64). Starting from the hand-named functions in tools/ghidra/names.tsv, this tool
follows
  - function-pointer tables in maincpu.bin that a function indexes (name: <owner>_act<i> when indexed by the record's
    action code +0x64, else <owner>_st<i>),
  - code pointers a function stores (not calls): into the object's per-frame slot (*obj = routine: <root>_n<k>) or into
    another object field (<root>_f<offset hex>), or anywhere else (<root>_p<k>),
  - functions that only this function calls (helpers: <owner>_sub<k>),
breadth first. The prefix is the nearest hand-named owner (the root), so all states of e.g. obj_kind37 (the Chariot) are
obj_kind37_*. A routine reached from more than one root is left alone (shared code). The PC address comes from
private/decomp/match_functions.tsv when the routine is paired.

These are **structural** names (who installs the routine, in which slot), not semantic ones: hand names (anything
outside the generated block) always win and should replace them as systems are read. The generated block in
tools/ghidra/names.tsv (between the "# BEGIN name_routines.py" / "# END" lines) is rewritten on each run.

  py tools/name_routines.py            # rewrite the generated block, print a summary
  py tools/name_routines.py --dry      # summary only
"""
import os, re, struct, sys, collections

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
NAMES = os.path.join(HERE, "ghidra", "names.tsv")
BEGIN, END = "# BEGIN name_routines.py (generated: structural names, see the tool)", "# END name_routines.py"
# tables whose slots are roots of their own (structural names, not hand names): (address, entries, name pattern, comment)
TABLE_ROOTS = [(0xAFDE0, 192, "obj_kind{:03d}", "object kind {} (kind table 0xAFDE0; not read yet)"),
               (0xAF960, 125, "prop_type{:03d}", "prop sub-type {} (kind 128 prop_object, table 0xAF960; not read yet)")]


def load_c(path):
    funcs, cur = {}, None
    for line in open(path, encoding="utf-8", errors="replace"):
        m = re.match(r"// ==== (\S+) @ ([0-9a-f]+) \((\d+) bytes\)", line)
        if m:
            cur = int(m.group(2), 16)
            funcs[cur] = [m.group(1), int(m.group(3)), []]
        elif cur is not None:
            funcs[cur][2].append(line)
    return {a: (n, s, "".join(b[1:])) for a, (n, s, b) in funcs.items()}


def read_names():
    lines = open(NAMES, encoding="utf-8").read().splitlines()
    if BEGIN in lines:
        i, j = lines.index(BEGIN), lines.index(END)
        lines = lines[:i] + lines[j + 1:]
    hand = {}
    for l in lines:
        c = l.split("\t")
        if c[0] == "func" and len(c) > 3 and c[2] != "-":
            hand[int(c[2], 16)] = c[3]
    return lines, hand


def pointer_tables(rom, funcs):
    """Runs of function-pointer words (zero words allowed inside): start -> list of entries (address or 0)."""
    fa = set(funcs)
    tables, o, n = {}, 0, len(rom) - 3
    while o < n:
        w = struct.unpack_from("<I", rom, o)[0]
        if w in fa and w:
            s, ents, zeros = o, [], 0
            while o < n:
                w = struct.unpack_from("<I", rom, o)[0]
                if w in fa and w:
                    ents += [0] * zeros + [w]
                    zeros = 0
                elif w == 0 and zeros < 1:
                    zeros += 1
                else:
                    break
                o += 4
            if len(ents) >= 2:
                tables[s] = ents
            o = s + 4 * (len(ents) + 1)
            continue
        o += 4
    return tables


def main(dry):
    funcs = load_c(os.path.join(ROOT, "private", "decomp", "arcade", "maincpu.c"))
    rom = open(os.path.join(ROOT, "private", "arcade_exports", "regions", "maincpu.bin"), "rb").read()
    lines, hand = read_names()
    pc_of = {}
    for l in open(os.path.join(ROOT, "private", "decomp", "match_functions.tsv")).read().splitlines()[1:]:
        c = l.split("\t")
        pc_of[int(c[1], 16)] = c[0].lstrip("0")
    used_pc = set()
    for l in lines:
        c = l.split("\t")
        if c[0] == "func" and len(c) > 3 and c[1] != "-":
            used_pc.add(c[1].lstrip("0"))
    # named data (names.tsv "data" lines) appears under its name in the export: turn it back into DAT_<address>
    data = {}
    for l in lines:
        c = l.split("\t")
        if c[0] == "data" and len(c) > 3 and c[2] != "-":
            data[c[3]] = int(c[2], 16)
    if data:
        dre = re.compile(r"\b(" + "|".join(sorted(map(re.escape, data), key=len, reverse=True)) + r")\b")
        funcs = {a: (n, s, dre.sub(lambda m: f"DAT_{data[m.group(1)]:08x}", t)) for a, (n, s, t) in funcs.items()}
    by_name = {n: a for a, (n, s, t) in funcs.items()}
    tables = pointer_tables(rom, funcs)
    # labels as Ghidra prints them: DAT_000c0c60, PTR_FUN_000c0c60, or PTR_<name of the first entry>_000c0c60
    ref = re.compile(r"\b(?:PTR_\w*?_|DAT_|FUN_|LAB_)([0-9a-f]{8})\b")
    # a run of pointers can hold several tables back to back: split it where the code refers into it
    cited = {int(m.group(1), 16) for a, (n, sz, t) in funcs.items() for m in ref.finditer(t)}
    table_at = {}
    for s, ents in tables.items():
        cuts = sorted(c for c in cited if s <= c < s + 4 * len(ents) and (c - s) % 4 == 0) or [s]
        cuts.append(s + 4 * len(ents))
        for c0, c1 in zip(cuts, cuts[1:]):
            table_at[c0] = (c0, ents[(c0 - s) // 4:(c1 - s) // 4])
    fnref = re.compile(r"\b(FUN_[0-9a-f]{8}|[A-Za-z_][A-Za-z0-9_]*)\b(\s*\()?")
    store = re.compile(r"(\*param_1|\*\(code \*\*\)\(param_1 \+ (0x[0-9a-f]+|\d+)\)|param_1\[(0x[0-9a-f]+|\d+)\])\s*=\s*$")

    case_re = re.compile(r"\b(case (0x[0-9a-f]+|\d+)|default|switch)\b")
    callers = collections.defaultdict(set)
    for a, (n, s, t) in funcs.items():
        for m in fnref.finditer(t):
            if m.group(2) and m.group(1) in by_name and by_name[m.group(1)] != a:
                callers[by_name[m.group(1)]].add(a)

    def edges(addr):
        """(kind, label, target, how) for the routines this function hands out."""
        name, size, text = funcs[addr]
        out = []
        for m in ref.finditer(text):
            t = int(m.group(1), 16)
            if t in table_at and t != addr:
                ctx = text[m.end():m.end() + 60]
                lab = "act" if re.match(r"\)\[\*\(short \*\)\(param_1 \+ (100|0x64)\)\]", ctx) else "st"
                for i, e in enumerate(table_at[t][1]):
                    if e:
                        out.append(("tbl", f"{lab}{i}", e, f"entry {i} of table 0x{t:X}"))
        for m in fnref.finditer(text):
            t = by_name.get(m.group(1))
            if t is None or t == addr:
                continue
            if m.group(2):   # a call: helpers that only this function calls
                if callers.get(t) == {addr}:
                    out.append(("call", "sub", t, "helper called only by"))
                continue
            before = text[max(0, m.start() - 80):m.start()].split("\n")[-1].split(";")[-1]
            sm = store.search(before)
            cases = list(case_re.finditer(text, 0, m.start()))
            case = int(cases[-1].group(2), 0) if cases and cases[-1].group(2) else None
            if sm and sm.group(1) == "*param_1":
                lab, how = "n", "per-frame routine (object +0) set by"
            elif sm:
                off = int(sm.group(2) or sm.group(3), 0) * (1 if sm.group(2) else 4)
                lab, how = f"f{off:x}_", f"object +0x{off:X} routine set by"
            else:
                lab, how = "p", "code pointer used by"
            if case is not None:
                lab, how = f"case{case}" + ("" if lab == "n" else lab.rstrip("_")), how.replace(" by", f" in case {case} of")
            out.append(("ptr", lab, t, how))
        return out

    # slots of the TABLE_ROOTS tables that have no hand name are roots too (with a generated name)
    troots = {}
    for at, n, pat, cmt in TABLE_ROOTS:
        ws = [struct.unpack_from("<I", rom, at + 4 * i)[0] for i in range(n)]
        for i, w in enumerate(ws):
            if ws.count(w) > 1:   # a default routine in many slots: not this slot's own
                continue
            if w in funcs and w not in hand and w not in troots:
                troots[w] = (pat.format(i), cmt.format(i))
    rootname = dict(hand)
    rootname.update({a: n for a, (n, c) in troots.items()})
    # breadth first from every hand-named function; a routine reached from several roots at the same depth is shared
    parent = {}      # addr -> (parent addr, label, how, kind, root)
    also = collections.defaultdict(set)   # other roots reaching it at the same depth
    frontier = [(a, a) for a in sorted(rootname) if a in funcs]
    depth = {a: 0 for a, _ in frontier}
    while frontier:
        nxt = []
        for a, root in frontier:
            for kind, lab, t, how in edges(a):
                if t in rootname or t >= 0x200000 or t not in funcs:
                    continue
                if t in parent:
                    if parent[t][4] != root and depth[t] == depth[a] + 1:
                        also[t].add(root)
                    continue
                parent[t] = (a, lab, how, kind, root)
                depth[t] = depth[a] + 1
                nxt.append((t, root))
        frontier = nxt
    conflict = {t for t, r in also.items() if len(r) >= 3}   # generic helpers: leave unnamed
    for t in conflict:
        parent.pop(t, None)
    # names: parent name + label (+ a number when the parent hands out several with the same label)
    names = {a: (n, "") for a, n in hand.items()}
    names.update(troots)
    kids = collections.defaultdict(list)
    for t, (a, lab, how, kind, root) in parent.items():
        kids[(a, lab)].append(t)
    taken = set(rootname.values())
    for t in sorted(parent, key=lambda x: (depth[x], x)):
        a, lab, how, kind, root = parent[t]
        if a not in names:
            continue   # parent was shared
        same = sorted(kids[(a, lab)])
        base = names[a][0]
        if len(base) > 48:   # deep chains: restart from the root
            base = names[root][0] + "_d" + str(depth[t])
        n = f"{base}_{lab.rstrip('_')}" + (str(same.index(t) + 1) if len(same) > 1 and kind != "tbl" else "")
        while n in taken:
            n += "x"
        taken.add(n)
        pn = names[a][0]
        extra = "; also " + ", ".join(sorted(rootname[r] for r in also[t])) if also.get(t) else ""
        names[t] = (n, (f"{how} {pn}" if kind != "tbl" else f"{how}, used by {pn}") + extra)
    for a in hand:
        names.pop(a)
    if not dry:
        out = [BEGIN]
        for t in sorted(names):
            n, c = names[t]
            pc = pc_of.get(t, "-")
            if pc in used_pc:
                pc = "-"
            out.append(f"func\t{pc}\t{t:x}\t{n}\t{c}")
        out.append(END)
        open(NAMES, "w", encoding="utf-8", newline="\n").write("\n".join(lines + out) + "\n")
    total = sum(s for a, (n, s, t) in funcs.items() if a < 0x200000)
    nb = sum(funcs[a][1] for a in names)
    hb = sum(funcs[a][1] for a in hand if a in funcs)
    print(f"hand-named {len(hand)} ({hb * 100 / total:.1f}%), generated {len(names)} ({nb * 100 / total:.1f}%), "
          f"left shared (reached from several roots): {len(conflict)}")
    per_root = collections.Counter(rootname[parent[t][4]] for t in names if t in parent)
    print("largest families:", ", ".join(f"{r} {n}" for r, n in per_root.most_common(int(os.environ.get("FAMILIES", "15")))))


if __name__ == "__main__":
    main("--dry" in sys.argv)
