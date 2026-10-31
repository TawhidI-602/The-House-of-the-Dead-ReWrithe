#!/usr/bin/env python3
r"""
chapter_gap.py - what a chapter still needs (docs/chapter2_3_plan.md; the method of docs/teaser_plan.md, refined).

For one chapter of the arcade game: its events (EVTALL.RBH list 0-3), every record the event scripts spawn (ENTB / ENTBE /
ENTB3 / ET* lists: kind at +0, creature type = byte +0x24, action / scenario = byte +0x25; ENTB2: scene objects), each kind
mapped through the object-kind table (arcade 0xAFDE0) to its routine, then the arcade call graph
(private/decomp/arcade/maincpu_index.tsv: direct calls, plus function-pointer immediates and tables of function pointers
found in the routine bodies) walked from the root. "Ported" = a routine declared with its address in src/port/decl/*.h
without an inline stub body; a stub (`virtual ... { }`) is a boundary, counted as not ported. Routines reachable from the
ordinary kinds of Chapter 1 ("common": drawing hooks, text, effects ... the Chapter 1 game already runs without them)
are left out of the "missing" sums. Sizes are arcade i960 bytes from the index (often a little short).

    py tools/chapter_gap.py --root "C:\Games\HOTD M2 RECOMPILE\M2 Source" --chapter 2 [--events] [--routines]

--root  the main checkout (it holds private\); the src/port headers are read from the checkout this script is in.
--chapter 1-4 (default 2); --events: the per-event table; --routines: every missing routine; --boss <kind>: one kind's list.
"""
import argparse
import collections
import glob
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import evt_dis as E  # noqa: E402

CODE_END = 0xA6588
EXCL = {18, 37, 100, 40, 24, 20}            # bosses / hostages / Curien: counted on their own
SPAWN_OPS = ("ENTB", "ENTBE", "ENTB3", "ET1PE", "ET1P", "ET1P3", "ET2PE", "ET2P", "ET2P3")
SCENE_OPS = ("ENTB2", "ET1P2", "ET2P2")
STUB = re.compile(r"^\s*virtual\s+[\w:<>\s\*&]+?\s+\w+\([^)]*\)\s*\{.*\}")


class Arcade:
    def __init__(self, root):
        pr = os.path.join(root, "private")
        self.rom = open(os.path.join(pr, "arcade_exports", "regions", "maincpu.bin"), "rb").read()
        self.idx = {}
        for ln in open(os.path.join(pr, "decomp", "arcade", "maincpu_index.tsv"), encoding="utf-8").read().splitlines()[1:]:
            f = ln.split("\t")
            if len(f) < 3 or len(f[0]) != 8:
                continue
            try:
                a = int(f[0], 16)
            except ValueError:
                continue
            callees = [int(x, 16) for x in f[5].split(",") if x.strip()] if len(f) > 5 and f[5] else []
            self.idx[a] = dict(name=f[1], size=int(f[2]), callees=callees)
        self.fn = set(a for a in self.idx if a < CODE_END)
        self.ported, self.stubs = set(), set()
        for fn in glob.glob(os.path.join(HERE, "..", "src", "port", "decl", "*.h")):
            for line in open(fn, encoding="utf-8"):
                if "(" not in line or "//" not in line:
                    continue
                code, com = line.split("//", 1)
                m = re.findall(r"0x([0-9A-Fa-f]{3,6})\b", com)
                if not m or re.match(r"\s*PC\b", com.strip()):
                    continue
                (self.stubs if STUB.match(code) else self.ported).add(int(m[0], 16))
        self.stubs = set(a for a in self.stubs if a in self.idx)
        self.kinds = {k: self.u32(0xAFDE0 + 4 * k) for k in range(192)}
        self.cache = {}

    def u32(self, o):
        return struct.unpack_from("<I", self.rom, o)[0]

    def refs(self, a):
        if a not in self.cache:
            s = set(self.idx[a]["callees"]) if a in self.idx else set()
            if a in self.idx:
                for o in range(a, min(a + self.idx[a]["size"], len(self.rom) - 4), 4):
                    v = self.u32(o)
                    if v in self.fn and v != a and v >= 0x10000 and v % 16 == 0:
                        s.add(v)
                    elif CODE_END <= v < 0x200000 and v % 4 == 0 and v + 8 < len(self.rom):
                        run, p = [], v
                        while p + 4 <= len(self.rom) and self.u32(p) in self.fn and self.u32(p) % 16 == 0:
                            run.append(self.u32(p)); p += 4
                        if len(run) >= 2:
                            s.update(run)
            self.cache[a] = set(x for x in s if x in self.idx)
        return self.cache[a]

    def reach(self, root):
        seen, st = set(), [root]
        while st:
            a = st.pop()
            if a in seen or a not in self.idx:
                continue
            seen.add(a)
            st.extend(self.refs(a))
        return seen

    def size(self, s):
        return sum(self.idx[a]["size"] for a in s)


def records(evt, exe, lst):
    """per event: ({kind: set((type, action))}, {scene kind}, [opcodes])"""
    E.load(evt, exe)
    out = []
    evs = E.events(lst)
    for ei, st in enumerate(evs):
        end = evs[ei + 1] if ei + 1 < len(evs) else st + 4000
        lines = []
        E.dis(st, end, lines.append)
        kinds, scene, ops = collections.defaultdict(set), set(), collections.Counter()
        for ln in lines:
            if ":" not in ln or "[" not in ln:
                continue
            body = ln.split(":", 1)[1].strip()
            name = body.split()[0]
            ops[name] += 1
            if name in SPAWN_OPS or name in SCENE_OPS:
                for a in eval(body[body.index("["):]):
                    o = 0x28 + a
                    k = struct.unpack_from("<I", E.d, o)[0]
                    if name in SCENE_OPS:
                        scene.add(k)
                    else:
                        w = struct.unpack_from("<I", E.d, o + 36)[0]
                        kinds[k].add((w & 0xFF, (w >> 8) & 0xFF))
        out.append((kinds, scene, ops))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True)
    ap.add_argument("--chapter", type=int, default=2)
    ap.add_argument("--pc", default="")
    ap.add_argument("--events", action="store_true")
    ap.add_argument("--routines", action="store_true")
    ap.add_argument("--boss", type=int, default=-1)
    a = ap.parse_args()
    pc = a.pc or os.path.join(a.root, "private", "pcharness", "game")
    evt, exe = os.path.join(pc, "EVT", "ALLSTAGE", "EVTALL.RBH"), os.path.join(pc, "THOTD.EXE")
    A = Arcade(a.root)
    ch1 = records(evt, exe, 0)
    chn = records(evt, exe, a.chapter - 1)
    common = set()
    for kinds, scene, _ in ch1:
        for k in set(kinds) | scene:
            if k not in EXCL and A.kinds[k] in A.idx:
                common |= A.reach(A.kinds[k])
    ok = lambda s: s - A.ported - A.stubs - common  # noqa: E731
    print("ported: %d routines, %d bytes; common (Chapter 1's ordinary kinds) reach %d routines, %d bytes, of which "
          "not ported %d bytes" % (len(A.ported & set(A.idx)), A.size(A.ported & set(A.idx)), len(common), A.size(common),
                                    A.size(common - A.ported - A.stubs)))
    used = collections.defaultdict(set)
    for ei, (kinds, scene, ops) in enumerate(chn):
        for k in kinds:
            used[k] |= kinds[k]
        for k in scene:
            used.setdefault(k, set())
    sc = [A.u32(0xC1580 + 4 * i) for i in range(26)]
    tot = set()
    print("\nChapter %d: %d events, %d kinds / scene objects" % (a.chapter, len(chn), len(used)))
    for k in sorted(used):
        r = A.kinds[k]
        if r not in A.idx:
            print("  kind %3d  (no routine)" % k); continue
        m = ok(A.reach(r))
        if k not in EXCL:
            tot |= m
        print("  kind %3d %06x %-34s root %-8s missing %5d B (%d routines)%s" % (
            k, r, A.idx[r]["name"], "ported" if r in A.ported else "NOT", A.size(m), len(m), "  [counted separately]" if k in EXCL else ""))
        if a.routines or k == a.boss:
            for x in sorted(m, key=lambda x: -A.idx[x]["size"]):
                print("        %06x %-40s %d" % (x, A.idx[x]["name"], A.idx[x]["size"]))
    print("distinct missing over the chapter's ordinary kinds: %d B" % A.size(tot))
    scen = set(ac for ks in (e[0] for e in chn) for k, v in ks.items() if k == 100 for _, ac in v)
    s_tot = set()
    for s in sorted(scen):
        m = ok(A.reach(sc[s]))
        s_tot |= m
        print("  hostage scenario %2d %06x %-26s missing %4d B" % (s, sc[s], A.idx[sc[s]]["name"], A.size(m)))
    print("hostage scenarios total: %d B" % A.size(s_tot))
    props = set(t for ks in (e[0] for e in chn) for k, v in ks.items() if k == 128 for t, _ in v)
    p_tot = set()
    for t in sorted(props):
        r = A.u32(0xAF960 + 4 * t)
        m = ok(A.reach(r)) if r in A.idx else set()
        if m:
            print("  prop sub-type %3d %06x %-26s missing %4d B" % (t, r, A.idx[r]["name"], A.size(m)))
        p_tot |= m
    print("prop sub-types total: %d B" % A.size(p_tot))
    if 18 in used:
        t = [A.u32(0xC1190 + 4 * i) for i in range(3)]
        m = set()
        for x in t:
            m |= ok(A.reach(x))
        print("Hangedman (kind 18 set-ups 0xC1190): %d routines %d B" % (len(m), A.size(m)))
    for k, nm in ((40, "Hermit"), (24, "Curien (kind 24)"), (37, "Chariot")):
        if k in used:
            m = ok(A.reach(A.kinds[k]))
            print("%s: %d routines %d B" % (nm, len(m), A.size(m)))
    if a.events:
        print("\nevent | kinds (* = something missing)")
        for ei, (kinds, scene, ops) in enumerate(chn):
            cells = []
            for k in sorted(kinds):
                bad = A.kinds[k] in A.idx and ok(A.reach(A.kinds[k]))
                cells.append("%d%s" % (k, "*" if bad else ""))
            print("%5d | %s | scene %s" % (ei, ", ".join(cells), sorted(scene)))


if __name__ == "__main__":
    main()
