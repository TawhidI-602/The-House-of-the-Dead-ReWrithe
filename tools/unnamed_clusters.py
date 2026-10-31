#!/usr/bin/env python3
"""
unnamed_clusters.py - where is the arcade code that has no name yet? Groups the unnamed functions (neither hand nor
structural names in tools/ghidra/names.tsv) under their topmost unnamed referrer, so that naming one function (the
cluster's root) lets tools/name_routines.py name the rest structurally.

References come from the decompiled C (calls, code pointers, pointer tables cited by a function) plus "lda <address>"
operands and pointer words found in maincpu.bin (task routines handed to task_create, tables without a C label).

  py tools/unnamed_clusters.py [count]       roots by the bytes of unnamed code below them, with what refers to them
"""
import collections, os, re, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import name_routines as nr

ROOT = os.path.join(HERE, "..")


def main():
    # usage: unnamed_clusters.py [count] [--members <root hex>]  (members: list the cluster's functions)
    members = None
    if "--members" in sys.argv:
        i = sys.argv.index("--members")
        members = int(sys.argv[i + 1], 16)
        del sys.argv[i:i + 2]
    count = int(sys.argv[1]) if len(sys.argv) > 1 else 40
    funcs = nr.load_c(os.path.join(ROOT, "private", "decomp", "arcade", "maincpu.c"))
    funcs = {a: v for a, v in funcs.items() if a < 0x200000}
    rom = open(os.path.join(ROOT, "private", "arcade_exports", "regions", "maincpu.bin"), "rb").read()
    named = set()
    for l in open(nr.NAMES, encoding="utf-8"):
        c = l.rstrip("\n").split("\t")
        if c[0] == "func" and len(c) > 3 and c[2] != "-":
            named.add(int(c[2], 16))
    starts = sorted(funcs)
    import bisect

    def owner(addr):
        i = bisect.bisect_right(starts, addr) - 1
        if i >= 0 and addr < starts[i] + max(funcs[starts[i]][1], 4):
            return starts[i]
        return None

    by_name = {n: a for a, (n, s, t) in funcs.items()}
    refs = collections.defaultdict(set)          # target -> referrers (function addresses, or -data address)
    word = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\b")
    for a, (n, s, t) in funcs.items():
        for m in word.finditer(t):
            b = by_name.get(m.group(1))
            if b is not None and b != a:
                refs[b].add(a)
    fset = set(funcs)
    # pointer words anywhere in the ROM: inside code they are the displacement words of lda / callx
    for o in range(0, min(len(rom), 0x200000) - 3, 4):
        w = struct.unpack_from("<I", rom, o)[0]
        if w in fset:
            src = owner(o)
            if src is not None and src != w:
                refs[w].add(src)
            elif src is None:
                refs[w].add(-o)
    un = {a for a in funcs if a not in named}
    # parent among unnamed referrers: the unnamed functions referring to it
    up = {a: {r for r in refs.get(a, ()) if r in un and r != a} for a in un}
    roots = [a for a in un if not up[a]]
    # descendants reachable only through unnamed functions
    down = collections.defaultdict(set)
    for a in un:
        for r in up[a]:
            down[r].add(a)
    sizes = []
    for r in roots:
        seen, todo = {r}, [r]
        while todo:
            x = todo.pop()
            for y in down[x]:
                if y not in seen:
                    seen.add(y)
                    todo.append(y)
        sizes.append((sum(funcs[x][1] for x in seen), r, len(seen)))
        if r == members:
            for x in sorted(seen):
                print(f"{x:06x} {funcs[x][1]:5d}  <- {', '.join(funcs[y][0] if y > 0 else f'data 0x{-y:X}' for y in sorted(refs.get(x, ())))}")
            return
    # cycles without a root: count what is not covered
    covered = set()
    total = sum(funcs[a][1] for a in un)
    print(f"unnamed: {len(un)} functions, {total} bytes; cluster roots: {len(roots)}")
    for b, r, n in sorted(sizes, reverse=True)[:count]:
        who = []
        for x in sorted(refs.get(r, ())):
            who.append(f"data 0x{-x:X}" if x < 0 else funcs[x][0])
        print(f"{r:06x} {funcs[r][0]:<14} cluster {n:3d} funcs {b:6d} bytes  <- {', '.join(who[:4]) or '(nothing)'}")


if __name__ == "__main__":
    main()
