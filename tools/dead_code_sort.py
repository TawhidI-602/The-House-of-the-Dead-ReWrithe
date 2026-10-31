#!/usr/bin/env python3
"""
dead_code_sort.py - the arcade program's dead code (functions nothing refers to: the `unused_<address>` roots of
tools/ghidra/names.tsv, found by tools/unnamed_clusters.py) with what each seems to be, as a first sort for
docs/dead_code.md: size, cluster (its _sub / _p helpers), strings, named callees and globals, byte-identical twins.

  py tools/dead_code_sort.py            table (tab-separated) of every root
Category hints (checked by hand afterwards): "leftover" = a stub (<= 8 bytes) or a byte-identical copy of a used
routine; "debug" = prints text / reads the test buttons / has debug strings; else "open".
"""
import collections, os, re, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
INDEX = os.path.join(ROOT, "private", "decomp", "arcade", "maincpu_index.tsv")
NAMES = os.path.join(HERE, "ghidra", "names.tsv")
ROM = os.path.join(ROOT, "private", "arcade_exports", "regions", "maincpu.bin")
C = os.path.join(ROOT, "private", "decomp", "arcade", "maincpu.c")

DEBUG_CALLEES = {"text_printf", "text_cursor", "debug_print", "print_hex"}


def main():
    names, comments = {}, {}
    for line in open(NAMES, encoding="utf-8"):
        c = line.rstrip("\n").split("\t")
        if len(c) >= 4 and c[0] in ("func", "data", "label") and c[2] not in ("-", ""):
            names[int(c[2], 16)] = c[3]
            comments[int(c[2], 16)] = c[4] if len(c) > 4 else ""
    idx = {}
    for line in open(INDEX, encoding="utf-8"):
        c = line.rstrip("\n").split("\t")
        if c[0] == "address" or not re.fullmatch(r"[0-9a-f]{8}", c[0]) or len(c) < 3 or not c[2].isdigit():
            continue
        a = int(c[0], 16)
        idx[a] = {"size": int(c[2]), "callers": [x for x in c[4].split(",") if x] if len(c) > 4 else [],
                  "callees": [int(x, 16) for x in c[5].split(",") if x] if len(c) > 5 else [],
                  "strings": c[6] if len(c) > 6 else ""}
    rom = open(ROM, "rb").read()
    # bytes of every function, for twins
    body = {a: rom[a:a + v["size"]] for a, v in idx.items() if a + v["size"] <= len(rom)}
    by_bytes = collections.defaultdict(list)
    for a, b in body.items():
        if len(b) >= 16:
            by_bytes[b].append(a)
    # globals per function from the C (named data only)
    gl = collections.defaultdict(set)
    cur = None
    data_names = {n for a, n in names.items()}
    for line in open(C, encoding="utf-8", errors="replace"):
        m = re.match(r"^// ==== (\w+) @ ([0-9a-f]+)", line)
        if m:
            cur = int(m.group(2), 16)
            continue
        if cur is not None:
            for w in re.findall(r"\b[a-z_][a-z0-9_]{3,}\b", line):
                if w in data_names and not w.startswith("unused_"):
                    gl[cur].add(w)
    roots = sorted(a for a, n in names.items() if re.fullmatch(r"unused_[0-9a-f]+", n))
    members = collections.defaultdict(list)
    for a, n in names.items():
        m = re.fullmatch(r"unused_([0-9a-f]+)_.*", n)
        if m:
            members[int(m.group(1), 16)].append(a)
    print("address\tsize\tcluster_bytes\thint\ttwin\tcallees\tglobals\tstrings")
    for a in roots:
        v = idx.get(a, {"size": 0, "callees": [], "strings": ""})
        cl = v["size"] + sum(idx.get(m, {"size": 0})["size"] for m in members[a])
        callees = sorted({names.get(x, "%x" % x) for x in v["callees"]})
        twins = [x for x in by_bytes.get(body.get(a, b""), []) if x != a]
        hint = "open"
        if v["size"] <= 8:
            hint = "leftover (stub)"
        elif twins:
            hint = "leftover (copy of %s)" % ",".join(names.get(t, "%x" % t) for t in twins)
        elif v["strings"] or any(x in DEBUG_CALLEES for x in callees):
            hint = "debug?"
        g = sorted(gl.get(a, set()) - set(callees))
        print("%x\t%d\t%d\t%s\t%s\t%s\t%s\t%s" % (a, v["size"], cl, hint, ",".join("%x" % t for t in twins),
                                                ",".join(callees)[:160], ",".join(g)[:120], v["strings"][:100]))


if __name__ == "__main__":
    main()
