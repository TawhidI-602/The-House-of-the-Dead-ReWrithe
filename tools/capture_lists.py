#!/usr/bin/env python3
"""capture_lists.py - the argument lists of a proof capture (docs/proof.md), built from the port and the indexes.

Usage:
  py tools/capture_lists.py --funcs 7c9a0,7ea80,...          arcade routines to capture
  py tools/capture_lists.py --file src/port/enemy_dogs.cpp   every arcade routine of that file's section of game.h
                            [--file ...] [--funcs ...] [--no-pc] [--no-a]
                            [--compare-a <ps1>] [--compare-pc <ps1>]   diff against a hand-built command line

Prints, ready to paste:
  A:  -Funcs (sizes by the next-index-start rule) -Ported (every Game<P> member with an arcade address but stubs,
      plus the funcs) -Boundary (callees of the funcs and of the ported routines they reach that are not ported, plus
      every virtual stub's address the C++ of those routines calls on the arcade side).
  PC: -CapFuncs (the funcs' PC pairs, tools/ghidra/names.tsv) -CapBoundary (the same rule on private/decomp/pc's index
      and the C++'s PC side).
Warnings (to stderr): funcs without a PC pair, ported PC-only helpers among the callees (never boundaries), callees
of the original that the C++ neither ports nor calls as a stub, sizes kept at Ghidra's because the extra range is not
code.

Sizes: Ghidra's index size is often short (jump tables, split routines). The size used is next index start - address
when that exceeds Ghidra's size by more than 12, cut after the last `ret` in the range (plus padding words up to the
next start); if the extra range does not disassemble as code (tools/i960dis.py), Ghidra's size stays.
"""
import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import i960dis  # noqa: E402
PRIV = os.environ.get("HOTD_ROOT") or ROOT   # a worktree: HOTD_ROOT = the main checkout (its private\)

GAME_H = os.path.join(ROOT, "src", "port", "game.h")
PORT_DIR = os.path.join(ROOT, "src", "port")
A_INDEX = os.path.join(PRIV, "private", "decomp", "arcade", "maincpu_index.tsv")
PC_INDEX = os.path.join(PRIV, "private", "decomp", "pc", "thotd_index.tsv")
NAMES = os.path.join(ROOT, "tools", "ghidra", "names.tsv")
MAINCPU = os.path.join(PRIV, "private", "arcade_exports", "regions", "maincpu.bin")
THOTD = os.path.join(PRIV, "private", "pcharness", "game", "THOTD.EXE")


def warn(msg):
    print("warning: " + msg, file=sys.stderr)


# ---------------------------------------------------------------- game.h
class Member:
    def __init__(self, name, stub_a, stub_pc, a, pc, pc_only, section):
        self.name, self.stub_a, self.stub_pc, self.a, self.pc, self.pc_only, self.section = \
            name, stub_a, stub_pc, a, pc, pc_only, section

    def stub(self, ver):
        return self.stub_a if ver == "a" else self.stub_pc


def pc_replayed():
    """Ported routines the PC proof still replays (no PC form yet): `if (P::pc || recorded_next(F::x))` in
    src/proof/bounds/*.inc."""
    names = set()
    d = os.path.join(ROOT, "src", "proof", "bounds")
    for f in os.listdir(d):
        if f.endswith(".inc"):
            text = open(os.path.join(d, f), encoding="utf-8").read()
            names |= set(re.findall(r"P::pc \|\| recorded_next\(F::(\w+)\)", text))
    return names


def game_h_lines():
    """game.h's lines with the per-system declaration files (#include "port/decl/<k>.h") read in place."""
    for line in open(GAME_H, encoding="utf-8"):
        m = re.match(r'\s*#include "(port/decl/\w+\.h)"', line)
        if m:
            yield "    // ---\n"   # a file's members belong to the sections it names itself
            yield from open(os.path.join(os.path.dirname(GAME_H), "..", m.group(1)), encoding="utf-8")
        else:
            yield line


def parse_game_h():
    """Members of Game<P> with their arcade / PC addresses from the `// 0xADDR` / `// PC 0xADDR` comments."""
    members = []
    section = ""
    in_class = False
    replayed = pc_replayed()
    for line in game_h_lines():
        if line.startswith("template <class P> class Game"):
            in_class = True
            continue
        if not in_class:
            continue
        if line.startswith("};"):
            break
        m = re.match(r"\s*// --- .*\((\S+?)\) ---", line)
        if m:
            section = m.group(1)
            continue
        if re.match(r"\s*// ---", line):
            section = ""
            continue
        code, _, comment = line.partition("//")
        m = re.search(r"(\w+)\s*\(", code)
        if not m or code.strip().startswith(("template", "struct", "explicit", "~")):
            continue
        name = m.group(1)
        # a stub (boundary) is a virtual member with a body; a virtual one only declared is a ported routine kept
        # virtual (the runtime's overrides, the proof's replay of older captures): ported on A, and on the PC unless
        # the PC proof still replays it
        virtual = bool(re.match(r"\s*virtual\b", code))
        stub_a = virtual and "{" in code
        stub_pc = stub_a or (virtual and name in replayed)
        a = pc = None
        ma = re.match(r"\s*0x([0-9A-Fa-f]+)(?![0-9A-Fa-f'])", comment)
        if ma:
            a = int(ma.group(1), 16)
        mp = re.search(r"\bPC 0x([0-9A-Fa-f]+)", comment)
        if mp:
            pc = int(mp.group(1), 16)
        members.append(Member(name, stub_a, stub_pc, a, pc, "PC only" in comment, section))
    return members


# ---------------------------------------------------------------- C++ call graph
def strip_comments(s):
    s = re.sub(r"/\*.*?\*/", " ", s, flags=re.S)
    s = re.sub(r"//[^\n]*", "", s)
    return re.sub(r'"(\\.|[^"\\])*"', '""', s)


def match_close(s, i, op, cl):
    """i at `op`; returns the index after the matching `cl`."""
    d = 0
    while i < len(s):
        if s[i] == op:
            d += 1
        elif s[i] == cl:
            d -= 1
            if d == 0:
                return i + 1
        i += 1
    return len(s)


def statement_end(s, i):
    """the end of the statement or block starting at i (after whitespace)."""
    while i < len(s) and s[i].isspace():
        i += 1
    if i < len(s) and s[i] == "{":
        return i, match_close(s, i, "{", "}")
    d = 0
    j = i
    while j < len(s):
        c = s[j]
        if c in "({":
            d += 1
        elif c in ")}":
            d -= 1
        elif c == ";" and d == 0:
            return i, j + 1
        j += 1
    return i, len(s)


def version_ranges(body):
    """(start, end, versions) of the `if constexpr (P::pc)` / `(!P::pc)` branches and their else."""
    out = []
    for m in re.finditer(r"\bif\s+constexpr\s*\(", body):
        p0 = m.end() - 1
        p1 = match_close(body, p0, "(", ")")
        cond = re.sub(r"\s", "", body[p0 + 1:p1 - 1])
        if cond == "P::pc":
            yes, no = {"pc"}, {"a"}
        elif cond == "!P::pc":
            yes, no = {"a"}, {"pc"}
        else:
            continue
        s0, s1 = statement_end(body, p1)
        out.append((s0, s1, yes))
        m2 = re.match(r"\s*else\b", body[s1:])
        if m2:
            e0, e1 = statement_end(body, s1 + m2.end())
            out.append((e0, e1, no))
    return out


DYNAMIC = ("call_routine", "call_routine_f", "part_damage_callback")   # stubs that call through an address


def cpp_calls(member_names):
    """name -> {"a" / "pc": {"calls": callee names, "F": F:: names, "G": G:: names,
    "cases": F:: names of `case` labels, "dyn": calls a DYNAMIC stub}}
    over every Game<P>:: definition in src/port."""
    calls = {}
    for fn in sorted(os.listdir(PORT_DIR)):
        if not fn.endswith((".cpp", ".inc", ".h")):
            continue
        s = strip_comments(open(os.path.join(PORT_DIR, fn), encoding="utf-8").read())
        for m in re.finditer(r"Game<P>::(\w+)\s*\(", s):
            name = m.group(1)
            p1 = match_close(s, m.end() - 1, "(", ")")
            k = p1
            while k < len(s) and s[k] not in "{;":
                k += 1
            if k >= len(s) or s[k] != "{":
                continue
            body = s[k:match_close(s, k, "{", "}")]
            ranges = version_ranges(body)
            d = calls.setdefault(name, {v: {"calls": set(), "F": set(), "G": set(), "cases": set(), "dyn": False}
                                     for v in ("a", "pc")})

            def vers_at(pos):
                vs = {"a", "pc"}
                for r0, r1, v in ranges:
                    if r0 <= pos < r1:
                        vs = vs & v
                return vs

            for c in re.finditer(r"(?<![\w.>:])(\w+)\s*\(", body):
                callee = c.group(1)
                if callee not in member_names or callee == name:
                    continue
                for v in vers_at(c.start()):
                    d[v]["calls"].add(callee)
                    if callee in DYNAMIC:
                        d[v]["dyn"] = True
            for c in re.finditer(r"\b([FG])::(\w+)", body):
                for v in vers_at(c.start()):
                    d[v][c.group(1)].add(c.group(2))
            for c in re.finditer(r"\bcase\s+F::(\w+)\s*:", body):   # a dispatcher's case: handled in the C++
                for v in vers_at(c.start()):
                    d[v]["cases"].add(c.group(1))
    return calls


def read_globals():
    """{"a" / "pc": {"G": {name: addr}, "F": {name: addr}}} from src/port/globals.h."""
    out = {"a": {"G": {}, "F": {}}, "pc": {"G": {}, "F": {}}}
    ns = st = None
    for line in open(os.path.join(PORT_DIR, "globals.h"), encoding="utf-8"):
        m = re.match(r"namespace (a|pc) \{", line)
        if m:
            ns = m.group(1)
            continue
        m = re.match(r"struct ([FG]) \{", line)
        if m:
            st = m.group(1)
            continue
        if line.startswith("};"):
            st = None
            continue
        m = re.search(r"constexpr uint32_t (\w+) = 0x([0-9A-Fa-f]+);", line)
        if m and ns and st:
            out[ns][st][m.group(1)] = int(m.group(2), 16)
    return out


# ---------------------------------------------------------------- indexes
def read_index(path):
    """addr -> (name, size, callees)"""
    idx = {}
    for line in open(path, encoding="utf-8", errors="replace"):
        f = line.rstrip("\n").split("\t")
        if len(f) < 6 or f[0] == "address":
            continue
        try:
            a = int(f[0], 16)
            size = int(f[2])
        except ValueError:
            continue
        callees = set()
        for c in f[5].split(","):
            c = c.strip()
            if c and not c.startswith("EXTERNAL"):
                try:
                    callees.add(int(c, 16))
                except ValueError:
                    pass
        idx[a] = (f[1], size, callees)
    return idx


def read_names():
    """(arcade -> pc, pc -> name, arcade -> name) for funcs."""
    a2pc, pcname, aname = {}, {}, {}
    for line in open(NAMES, encoding="utf-8"):
        if line.startswith("#"):
            continue
        f = line.rstrip("\n").split("\t")
        if len(f) < 4 or f[0] != "func":
            continue
        pc = int(f[1], 16) if f[1] not in ("-", "") else None
        a = int(f[2], 16) if f[2] not in ("-", "") else None
        if a is not None:
            aname[a] = f[3]
            if pc is not None:
                a2pc.setdefault(a, pc)
        if pc is not None:
            pcname.setdefault(pc, f[3])
    return a2pc, pcname, aname


# ---------------------------------------------------------------- arcade sizes
class Sizer:
    def __init__(self, idx):
        self.idx = idx
        self.starts = sorted(idx)
        self.code = open(MAINCPU, "rb").read() if os.path.exists(MAINCPU) else None
        self.notes = {}

    def next_start(self, a):
        import bisect
        k = bisect.bisect_right(self.starts, a)
        return self.starts[k] if k < len(self.starts) else None

    def size(self, a):
        """(size, extended?) by the next-index-start rule."""
        g = self.idx[a][1] if a in self.idx else None
        n = self.next_start(a)
        if g is None or n is None or n - a <= g + 12 or self.code is None:
            return g, False
        def word(x):
            return int.from_bytes(self.code[x:x + 4], "little")

        has_bx = any(t.startswith("bx ") for _, _, t in i960dis.dis(self.code, 0, a, a + g))
        ins = list(i960dis.dis(self.code, 0, a + g, n))
        end = None
        after_ret = word(a + g - 4) == 0x0A000000
        for k, (x, _, t) in enumerate(ins):
            w = word(x)
            if t == "ret":
                end, after_ret = x + 4, True
                continue
            if w == 0 and after_ret:   # padding after a ret: up to the next start, or an unindexed routine follows
                rest = [y for y, _, _ in ins[k:] if word(y) != 0]
                if rest:
                    self.notes[a] = (f"ends at the ret before the padding: {rest[0]:#x} after it is an unindexed "
                                     f"routine or data (kept {g:#x})")
                else:
                    end = n
                break
            if t.startswith("bx "):
                has_bx = True
            if t.startswith(".word") or t.startswith("mem0x") or "?mode" in t:
                if not (has_bx and a <= w < n):   # a jump table's entry points back into the routine
                    self.notes[a] = f"extra range from {a + g:#x} is not code (at {x:#x}): kept {g:#x}"
                    return g, False
            after_ret = False
        if end is None or end - a <= g:
            if end is None and a not in self.notes:
                self.notes[a] = f"{n - a - g:#x} bytes after Ghidra's end hold no ret: kept {g:#x}"
            return g, False
        return end - a, True

    def calls_in(self, a, size):
        """call / callx targets in the routine (the index misses those past Ghidra's short size)."""
        out = set()
        if self.code is None or size is None:
            return out
        for _, _, t in i960dis.dis(self.code, 0, a, a + size):
            m = re.match(r"(call|callx) (0x[0-9a-f]+)$", t)
            if m:
                out.add(int(m.group(2), 16))
        return out


def fmt(a):
    return f"{a:x}"


def parse_list(s):
    out = []
    for x in s.split(","):
        x = x.strip()
        if x:
            p = x.split(":")
            out.append((int(p[0], 16), int(p[1], 16) if len(p) > 1 else None))
    return out


def ps1_args(path):
    s = open(path, encoding="utf-8").read()
    return {m.group(1): m.group(2) for m in re.finditer(r"-(\w+)\s+\"([^\"]*)\"", s)}


def compare(label, mine, theirs, names, why=None):
    """mine / theirs: {addr: size or None}"""
    only_m = sorted(set(mine) - set(theirs))
    only_t = sorted(set(theirs) - set(mine))
    sizes = sorted(a for a in set(mine) & set(theirs) if theirs[a] is not None and mine[a] != theirs[a])
    print(f"## {label}: {len(mine)} here, {len(theirs)} in the file; {len(set(mine) & set(theirs))} in both")
    for a in only_m:
        print(f"   + {a:x} {names.get(a, '?')}" + (f"  ({why[a]})" if why and a in why else ""))
    for a in only_t:
        print(f"   - {a:x} {names.get(a, '?')}")
    for a in sizes:
        print(f"   size {a:x} {names.get(a, '?')}: here {mine[a] if mine[a] is None else hex(mine[a])}, file {theirs[a]:#x}")


class Words:
    """reads 32-bit words of the arcade program ROM or of the PC EXE (for tables of routine addresses)."""
    def __init__(self, ver):
        self.code = self.d = None
        if ver == "a" and os.path.exists(MAINCPU):
            self.code = open(MAINCPU, "rb").read()
        elif ver == "pc" and os.path.exists(THOTD):
            import x86dis
            self.d, self.secs = x86dis.load(THOTD)
            self.read = x86dis.read

    def word(self, a):
        if self.code is not None:
            return int.from_bytes(self.code[a:a + 4], "little") if a + 4 <= len(self.code) else None
        if self.d is not None:
            b = self.read(self.d, self.secs, a, 4)
            return int.from_bytes(b, "little") if len(b) == 4 else None
        return None

    def routine_table(self, a, starts, limit=64):
        """the routine addresses of a table at a (stops at the first word that is neither 0 nor a routine start)."""
        out, zeros = set(), 0
        for k in range(limit):
            w = self.word(a + 4 * k)
            if w is None:
                break
            if w == 0:
                zeros += 1
                if zeros > 4:
                    break
                continue
            if w not in starts:
                break
            zeros = 0
            out.add(w)
        return out if len(out) >= 2 else set()


def analyse(ver, funcs, members, byname, graph, glob, idx, addr_of, ported, extra_callees):
    """The routines reached from funcs (C++ calls and index callees, through ported routines) and the boundaries.
    Returns (reached addrs, boundaries, {addr: callees}, unported callees without a stub, targets called through an
    address)."""
    reached_names = set()
    reached = set(funcs)
    stubs = set()
    callees_all = {}
    starts = set(idx)
    dyn_any = False
    frefs, grefs = set(), set()
    names_of = {}
    for m in members:
        if not m.stub(ver) and addr_of(m) is not None:
            names_of.setdefault(addr_of(m), []).append(m.name)
    todo_n = [n for a in funcs for n in names_of.get(a, [])]
    todo_a = list(funcs)
    while todo_n or todo_a:
        while todo_n:
            n = todo_n.pop()
            if n in reached_names:
                continue
            reached_names.add(n)
            g = graph.get(n, {}).get(ver)
            if not g:
                continue
            dyn_any |= g["dyn"]
            frefs |= g["F"]
            grefs |= g["G"]
            for c in g["calls"]:
                cm = byname[c]
                if cm.stub(ver):
                    stubs.add(c)
                elif c not in reached_names:
                    todo_n.append(c)
                    a = addr_of(cm)
                    if a is not None and a not in reached:
                        reached.add(a)
                        todo_a.append(a)
        while todo_a:
            a = todo_a.pop()
            cs = set(idx[a][2]) if a in idx else set()
            cs |= extra_callees(a)
            callees_all[a] = cs
            for n in names_of.get(a, []):
                if n not in reached_names:
                    todo_n.append(n)
            for c in cs:
                if c in ported and c not in reached:
                    reached.add(c)
                    todo_a.append(c)
    unported = set()
    for cs in callees_all.values():
        unported |= {c for c in cs if c not in ported}
    stub_addrs = {addr_of(byname[s]) for s in stubs if addr_of(byname[s]) is not None}
    all_stub_addrs = {addr_of(m) for m in members if m.stub(ver) and addr_of(m) is not None}
    dyn = set()
    if dyn_any:   # call_routine & co.: the F:: routines and the G:: routine tables these routines name
        words = Words(ver)
        dyn |= {glob[ver]["F"][f] for f in frefs if f in glob[ver]["F"]}
        for gname in grefs:
            if gname in glob[ver]["G"]:
                dyn |= words.routine_table(glob[ver]["G"][gname], starts)
        dyn = {a for a in dyn if a not in ported}
    # an unported callee without a stub anywhere in game.h is left out: the C++ does its work inline (a C library
    # helper such as pc_float_chop, a file's own copy of a matrix routine); listed, it would be a boundary the port
    # never calls
    no_stub = sorted(unported - all_stub_addrs - dyn)
    why = {}
    for a in unported & all_stub_addrs:
        why[a] = "callee"
    for a in stub_addrs:
        why.setdefault(a, "stub")
    for a in dyn:
        why.setdefault(a, "address")
    return reached, set(why), callees_all, no_stub, dyn, why


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--funcs", default="", help="arcade addresses, comma-separated (addr[:size])")
    ap.add_argument("--file", action="append", default=[], help="a src/port file: its game.h section's routines")
    ap.add_argument("--no-a", action="store_true")
    ap.add_argument("--no-pc", action="store_true")
    ap.add_argument("--compare-a", help="a run_capture.ps1 command line to diff with")
    ap.add_argument("--compare-pc", help="a run_pc.ps1 command line to diff with")
    args = ap.parse_args()

    members = parse_game_h()
    byname = {}
    for m in members:
        byname.setdefault(m.name, m)
    a2pc, pcname, aname = read_names()
    aidx = read_index(A_INDEX)
    pidx = read_index(PC_INDEX)
    glob = read_globals()
    sizer = Sizer(aidx)
    graph = cpp_calls(set(byname))

    def a_of(m):
        return m.a

    def pc_of(m):
        return m.pc if m.pc is not None else (a2pc.get(m.a) if m.a is not None else None)

    funcs = []   # (arcade addr, explicit size)
    for a, sz in parse_list(args.funcs):
        funcs.append((a, sz))
    for f in args.file:
        base = os.path.basename(f)
        found = [m for m in members if m.section == base and not m.stub_a and m.a is not None]
        if not found:
            warn(f"{f}: no section '({base})' with arcade addresses in game.h")
        funcs += [(m.a, None) for m in found]
    seen = set()
    funcs = [(a, s) for a, s in funcs if not (a in seen or seen.add(a))]
    if not funcs:
        ap.error("no routines: give --funcs or --file")
    fa = [a for a, _ in funcs]

    ported_a = {m.a for m in members if not m.stub_a and m.a is not None}
    stub_pc = {pc_of(m) for m in members if m.stub_pc and pc_of(m) is not None}
    # a routine the arcade port implements but the PC calls through a stub (crand -> pc_crand) is a PC boundary
    ported_pc = {pc_of(m) for m in members if not m.stub_pc and pc_of(m) is not None} - stub_pc
    # routines a C++ dispatcher handles in a `case F::name:` (e.g. an empty one): ported, never boundaries
    for ver, ported in (("a", ported_a), ("pc", ported_pc)):
        for g in graph.values():
            ported |= {glob[ver]["F"][f] for f in g[ver]["cases"] if f in glob[ver]["F"]}
    pc_only_helpers = {pc_of(m): m.name for m in members if not m.stub_pc and m.a is None and pc_of(m) is not None}
    for a in fa:
        if a not in ported_a:
            warn(f"func {a:x} {aname.get(a, '?')} is not a non-virtual Game<P> member with that address in game.h")

    # ---------------- arcade
    if not args.no_a:
        sizes = {a: sizer.size(a) for a in sorted(ported_a | set(fa))}
        for a, s in funcs:
            if s is not None:
                sizes[a] = (s, True)
        reached, bnd, callees_all, no_stub, dyn, why = analyse(
            "a", fa, members, byname, graph, glob, aidx, a_of, ported_a,
            lambda a: sizer.calls_in(a, sizes.get(a, (None,))[0]))
        for a in no_stub:
            callers = sorted(x for x, cs in callees_all.items() if a in cs)
            warn(f"A: {a:x} {aname.get(a, aidx.get(a, ('?',))[0])} is called by {','.join(fmt(x) for x in callers)} "
                 f"but has no stub in game.h: left out (inlined in the C++; if the port lacks it, add a stub)")
        for a, note in sorted(sizer.notes.items()):
            if a in reached:
                warn(f"A size {a:x} {aname.get(a, '?')}: {note}")

        def item(a):
            sz, ext = sizes.get(a, (None, False))
            return f"{a:x}:{sz:x}" if ext and sz else f"{a:x}"

        allp = sorted(ported_a | set(fa))
        print(f"# A: {len(fa)} funcs, {len(allp)} ported, {len(bnd)} boundaries ({len(dyn)} of them called through "
              f"an address: tables / F:: routines), {len(reached)} ported routines reached")
        print(f'-Funcs "{",".join(item(a) for a in fa)}" -Ported "{",".join(item(a) for a in allp)}" '
              f'-Boundary "{",".join(fmt(a) for a in sorted(bnd))}"')
        if args.compare_a:
            t = ps1_args(args.compare_a)
            names = dict(aname)
            names.update({a: v[0] for a, v in aidx.items() if a not in names})
            compare("A -Funcs", {a: sizes[a][0] if sizes[a][1] else None for a in fa},
                    dict(parse_list(t.get("Funcs", ""))), names)
            compare("A -Ported", {a: sizes[a][0] if sizes[a][1] else None for a in allp},
                    dict(parse_list(t.get("Ported", ""))), names)
            compare("A -Boundary", {a: None for a in bnd}, {a: None for a, _ in parse_list(t.get("Boundary", ""))},
                    names, why)

    # ---------------- PC
    if not args.no_pc:
        pfuncs = []
        for a in fa:
            m = next((m for m in members if not m.stub_a and m.a == a), None)
            pc = pc_of(m) if m else a2pc.get(a)
            if pc is None:
                warn(f"PC: func {a:x} {aname.get(a, '?')} has no PC pair (names.tsv): not captured on the PC")
            elif pc in stub_pc:
                warn(f"PC: func {a:x} {aname.get(a, '?')}: its PC routine {pc:x} is a stub on the PC: not captured")
            elif pc not in pfuncs:
                pfuncs.append(pc)
        reached, bnd, callees_all, no_stub, dyn, why = analyse(
            "pc", pfuncs, members, byname, graph, glob, pidx, pc_of, ported_pc, lambda a: set())
        helpers = sorted({c for cs in callees_all.values() for c in cs if c in pc_only_helpers})
        for a in helpers:
            warn(f"PC: {a:x} {pc_only_helpers[a]} is a ported PC-only helper called here: NOT a boundary (left out)")
        for a in no_stub:
            callers = sorted(x for x, cs in callees_all.items() if a in cs)
            warn(f"PC: {a:x} {pcname.get(a, pidx.get(a, ('?',))[0])} is called by "
                 f"{','.join(fmt(x) for x in callers)} but has no stub in game.h (inlined, a C library helper, or "
                 f"the port lacks it: then add a stub): left out")
        print(f"# PC: {len(pfuncs)} funcs, {len(bnd)} boundaries ({len(dyn)} of them called through an address), "
              f"{len(reached)} ported routines reached")
        print(f'-CapFuncs "{",".join(fmt(a) for a in pfuncs)}" -CapBoundary "{",".join(fmt(a) for a in sorted(bnd))}"')
        if args.compare_pc:
            t = ps1_args(args.compare_pc)
            names = dict(pcname)
            names.update({a: v[0] for a, v in pidx.items() if a not in names})
            compare("PC -CapFuncs", {a: None for a in pfuncs},
                    {a: None for a, _ in parse_list(t.get("CapFuncs", ""))}, names)
            compare("PC -CapBoundary", {a: None for a in bnd},
                    {a: None for a, _ in parse_list(t.get("CapBoundary", ""))}, names, why)


if __name__ == "__main__":
    main()
