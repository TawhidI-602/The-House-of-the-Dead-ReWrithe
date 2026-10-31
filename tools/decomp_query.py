#!/usr/bin/env python3
"""
decomp_query.py - search the Ghidra exports in private/decomp (made by tools/ghidra/run_pc.ps1 / run_arcade.ps1).

  py tools/decomp_query.py pc|arcade show <address hex> [...]        print the decompiled function(s) containing it
  py tools/decomp_query.py pc|arcade grep <regex> [--count]           functions whose C matches (with match count)
  py tools/decomp_query.py pc|arcade callers <address hex>            calling functions (from the index)
  py tools/decomp_query.py pc|arcade callees <address hex>
  py tools/decomp_query.py pc|arcade strings <regex>                  functions referencing matching strings
  py tools/decomp_query.py pc|arcade tree <address hex> [depth]       call tree below a function (sizes, strings)
"""
import os, re, sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "private", "decomp")
FILES = {"pc": ("pc", "thotd"), "arcade": ("arcade", "maincpu")}


def load(which):
    d, base = FILES[which]
    path = os.path.join(ROOT, d, base)
    funcs = []   # (addr, name, size, text)
    cur = None
    with open(path + ".c", encoding="utf-8", errors="replace") as f:
        for line in f:
            m = re.match(r"// ==== (\S+) @ ([0-9a-f]+) \((\d+) bytes\)", line)
            if m:
                cur = [int(m.group(2), 16), m.group(1), int(m.group(3)), []]
                funcs.append(cur)
            elif cur is not None:
                cur[3].append(line)
    index = {}
    with open(path + "_index.tsv", encoding="utf-8", errors="replace") as f:
        next(f)
        for line in f:
            r = line.rstrip("\n").split("\t")
            if len(r) >= 6 and re.fullmatch(r"[0-9a-f]{8}", r[0]):
                index[int(r[0], 16)] = r
    return funcs, index


def containing(funcs, a):
    best = None
    for f in funcs:
        if f[0] <= a < f[0] + max(f[2], 1) or f[0] == a:
            best = f
    if best is None:   # body may be non-contiguous: nearest start below
        below = [f for f in funcs if f[0] <= a]
        best = max(below, key=lambda f: f[0]) if below else None
    return best


def addrs(field):
    return [int(x, 16) for x in field.split(",") if x and not x.startswith("EXTERNAL")]


def main():
    if len(sys.argv) < 4:
        print(__doc__)
        return 1
    which, cmd, arg = sys.argv[1], sys.argv[2], sys.argv[3]
    funcs, index = load(which)
    byaddr = {f[0]: f for f in funcs}
    if cmd == "show":
        for a in sys.argv[3:]:
            f = containing(funcs, int(a, 16))
            print(f"// {f[1]} @ {f[0]:#x} ({f[2]} bytes)")
            print("".join(f[3]).rstrip())
    elif cmd == "grep":
        rx = re.compile(arg)
        for f in funcs:
            n = sum(len(rx.findall(l)) for l in f[3])
            if n:
                print(f"{f[0]:08x} {f[2]:6d} {n:4d} {f[1]}")
    elif cmd in ("callers", "callees"):
        r = index.get(int(arg, 16))
        for a in addrs(r[4] if cmd == "callers" else r[5]) if r else []:
            ir = index.get(a)
            print(f"{a:08x} {ir[2] if ir else '?':>6} {ir[1] if ir else ''} {ir[6][:100] if ir and len(ir) > 6 else ''}")
    elif cmd == "strings":
        rx = re.compile(arg, re.I)
        for a, r in sorted(index.items()):
            if len(r) > 6 and rx.search(r[6]):
                print(f"{a:08x} {r[2]:>6} {r[1]}  {r[6][:150]}")
    elif cmd == "tree":
        depth = int(sys.argv[4]) if len(sys.argv) > 4 else 2
        seen = set()

        def walk(a, d):
            r = index.get(a)
            print("  " * d + f"{a:08x} {r[2] if r else '?'} {r[6][:80] if r and len(r) > 6 else ''}")
            if a in seen or d >= depth or not r:
                return
            seen.add(a)
            for c in addrs(r[5]):
                walk(c, d + 1)
        walk(int(arg, 16), 0)
    return 0


if __name__ == "__main__":
    sys.exit(main())
