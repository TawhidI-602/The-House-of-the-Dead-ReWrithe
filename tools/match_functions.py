#!/usr/bin/env python3
"""
match_functions.py - pairs PC (THOTD.EXE) functions with their arcade (i960) originals.

The PC port is a direct port of the arcade program: matching functions call their helpers in the same order and touch
the same globals in the same order. Starting from seed pairs (the event-script opcode tables, and any pair given on the
command line), each matched pair's call sequences are aligned and the callees paired, until nothing new is found.
Globals are paired the same way (votes over all matched pairs).

  py tools/match_functions.py [pc:arcade ...]      extra seed pairs, hex
Input:  private/decomp/pc/thotd.c, private/decomp/arcade/maincpu.c (tools/ghidra/run_*.ps1), THOTD.EXE.orig, maincpu.bin
Output: private/decomp/match_functions.tsv (pc, arcade, sizes, how), private/decomp/match_globals.tsv (pc, arcade, votes)
"""
import os, re, struct, sys
from collections import Counter, defaultdict
from difflib import SequenceMatcher

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..", "private", "decomp")
PC_EXE = r"C:\Games\HOTD\THOTD.EXE.orig"
MAINCPU = os.path.join(HERE, "..", "private", "arcade_exports", "regions", "maincpu.bin")

# PC-only helpers that have no arcade counterpart (debug print, nop): dropped from PC call sequences before aligning
PC_ONLY = {0x4182E4, 0x471658,
           0x491FB4,   # tlPOLY_PlotMesh (PC renderer)
           0x4A50EC,   # PC error-message wrapper (arcade prints the text itself)
           0x4488E4}   # pc_continues_init (the PC's game_ending_start calls it where the arcade uploads a palette)
ARC_ONLY = {0x347A0}

# seed tables: (PC table VA, arcade table address, entries)
TABLES = [(0x5EEAC0, 0xC6510, 0x60),    # event script opcode handlers (formats.md §29, decompilation.md)
          (0x5D3888, 0xAB484, 33),      # camera drive modes (CAMDR)
          (0x5D8118, 0xAFDE0, 192),     # object kinds (task routine per enemy record kind / effect; 128-154 and 160-191 too)
          (0x5D19A0, 0xA9600, 10),      # game modes (per-frame routine, mode = 0x51F494 / high half of 0xDA0CD8)
          (0x5D8418, 0xB00E0, 80),      # camera job types (CCTB sub-operation: 16-21, 32, 33 camera mode, 64 path segment)
          (0x5D363C, 0xAB1E0, 9),       # mode 4 sub-modes 0-8 (9 / 10 are empty on the PC)
          (0x5D3668, 0xAB210, 11),      # chapter task list (task_group_create in chapter_load)
          (0x5D464C, 0xAC280, 7)]       # ending task list (game_ending_start)

# arcade operator test menu (the PC has none): its routines and the code only they reach are never paired.
# Roots: the TEST mode tick, menu open / select, the test players, the gun test and adjustment screens, and the
# 12 menu items (table 0xA8C50, decompilation.md); closure in test_menu_code().
TEST_ROOTS = {0x84C0, 0x8640, 0x86A0, 0x5800, 0x5850, 0xC6D0, 0xB5F0, 0xBFE0}
TEST_TABLE = (0xA8C50, 12)

# pairs confirmed by reading both (decompilation.md)
SEEDS = [(0x436FE4, 0x6410),             # player takes a hit
         (0x50B5B0, 0x94740),            # HUD: lives / torches (+ SCORE with title code 7)
         (0x4434B0, 0x17E40),            # draw 2D sprite at a text cell (animated)
         (0x443810, 0x180B0),            # draw 2D sprite at a text cell
         (0x43BA54, 0xDC20),             # apply game settings
         (0x4A4FC0, 0x55F40), (0x4A34B4, 0x55CF0), (0x4A4418, 0x543B0), (0x4A4790, 0x54720),   # enemy_guard actions 2, 3, 6, 7
         (0x4365C4, 0x5A80), (0x489C84, 0x454E0), (0x414810, 0x32CF0), (0x50BF10, 0x95270),    # player_shoot, shot_resolve, shot_ray_setup, shot_effect
         (0x4C0DB0, 0x66AD0), (0x412000, 0x154D0), (0x43E400, 0x14EA0), (0x46A91C, 0x39180),    # evt_run, sound_cue, task_create, set_motion
         (0x4C3D58, 0x68010)]                                                                  # part_hit

CALL = re.compile(r"\b([A-Za-z_]\w*)\s*\(")
WORD = re.compile(r"\b([A-Za-z_]\w*)\b")
FUNRX = re.compile(r"FUN_([0-9a-f]{8})")
DATRX = re.compile(r"_?DAT_([0-9a-f]{8})")
REFRX = re.compile(r"(?<![\w&])([A-Za-z_]\w*)\b(?!\s*\()")   # function names used as values (code pointers)
PTRRX = re.compile(r"&PTR_\w*?_([0-9a-f]{8})")
CONST = re.compile(r"(?<![\w.])(0x[0-9a-fA-F]+|\d+)(?![\w.])")


def load_names(col):
    """names from tools/ghidra/names.tsv (applied to the exports): name -> address, functions and data, one version"""
    funcs, data = {}, {}
    for line in open(os.path.join(HERE, "ghidra", "names.tsv"), encoding="utf-8"):
        c = line.rstrip("\n").split("\t")
        if line.startswith("#") or len(c) < 4 or c[col] == "-":
            continue
        (funcs if c[0] == "func" else data)[c[3]] = int(c[col], 16)
    return funcs, data


def parse(path, col):
    fnames, dnames = load_names(col)
    # the export's own headers name every function as it was exported (names.tsv may have renamed some since)
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = re.match(r"// ==== (\S+) @ ([0-9a-f]+) \(", line)
            if m:
                fnames.setdefault(m.group(1), int(m.group(2), 16))

    def fun(tok):
        m = FUNRX.fullmatch(tok)
        return int(m.group(1), 16) if m else fnames.get(tok)

    def dat(tok):
        m = DATRX.fullmatch(tok)
        return int(m.group(1), 16) if m else dnames.get(tok)

    funcs, cur = {}, None
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = re.match(r"// ==== (\S+) @ ([0-9a-f]+) \((\d+) bytes\)", line)
            if m:
                cur = int(m.group(2), 16)
                funcs[cur] = {"size": int(m.group(3)), "calls": [], "globs": [], "consts": set(), "tables": [], "refs": []}
                continue
            if cur is None or line.startswith(("void ", "undefined", "int ", "uint ")) and "(" in line and line.rstrip().endswith(")"):
                continue   # skip the signature line (it names the function itself)
            line = re.sub(r"/\*.*?\*/", " ", line)   # comments from names.tsv (they hold names and numbers)
            if line.lstrip().startswith("/*"):
                continue
            funcs[cur]["calls"] += [a for a in map(fun, CALL.findall(line)) if a is not None]
            funcs[cur]["globs"] += [a for a in map(dat, WORD.findall(line)) if a is not None]
            funcs[cur]["tables"] += [int(x, 16) for x in PTRRX.findall(line)]
            funcs[cur]["refs"] += [a for a in map(fun, REFRX.findall(line)) if a is not None]
            for c in CONST.findall(line):
                v = int(c, 16) if c.lower().startswith("0x") else int(c)
                if 9 <= v < 0x100000 and not 0x400000 <= v:   # skip tiny numbers and addresses
                    funcs[cur]["consts"].add(v)
    return funcs


def pc_word(d, va):
    w = struct.unpack_from("<I", d, va - 0x5A0000 + 0x187400)[0]
    # table entries that point at a "jmp rel32" thunk (the export skips thunks): use the jump target
    if 0x410000 <= w < 0x597000 and d[w - 0x410000 + 0x400] == 0xE9:
        w = (w + 5 + struct.unpack_from("<i", d, w - 0x410000 + 0x401)[0]) & 0xFFFFFFFF
    return w


def hand_pairs():
    """func lines of names.tsv outside the generated name_routines.py block that give both addresses: pairs read
    by hand (or checked when named), used as seeds so the weaker global / constant votes cannot move them"""
    out, gen = [], False
    for line in open(os.path.join(HERE, "ghidra", "names.tsv"), encoding="utf-8"):
        if line.startswith("# BEGIN name_routines.py"):
            gen = True
        elif line.startswith("# END name_routines.py"):
            gen = False
        c = line.rstrip("\n").split("\t")
        if not gen and len(c) >= 4 and c[0] == "func" and "-" not in (c[1], c[2]):
            out.append((int(c[1], 16), int(c[2], 16)))
    return out


def test_menu_code(arc, rom):
    """arcade functions of the operator test menu: the roots, then every function whose callers / referrers
    (calls, code pointers, entries of the tables they index) are all test-menu code"""
    def table_entries(ta):
        out = []
        for i in range(64):
            try:
                w = struct.unpack_from("<I", rom, ta + 4 * i)[0]
            except struct.error:
                break
            if w == 0:
                continue
            if w not in arc:
                break
            out.append(w)
        return out
    users = defaultdict(set)
    for f, v in arc.items():
        for c in v["calls"] + v["refs"] + [w for t in v["tables"] for w in table_entries(t)]:
            if c != f:
                users[c].add(f)
    test = set(TEST_ROOTS) | set(table_entries(TEST_TABLE[0])[:TEST_TABLE[1]])
    grown = True
    while grown:
        grown = False
        for f in arc:
            if f not in test and users[f] and users[f] <= test:
                test.add(f)
                grown = True
    return test


def main():
    pc = parse(os.path.join(ROOT, "pc", "thotd.c"), 1)
    arc = parse(os.path.join(ROOT, "arcade", "maincpu.c"), 2)
    exe = open(PC_EXE, "rb").read()
    rom = open(MAINCPU, "rb").read()
    test = test_menu_code(arc, rom)
    ARC_ONLY.update(test)
    print(f"arcade test-menu functions never paired: {len(test)}")
    match, back, how = {}, {}, {}

    def add(p, a, why):
        if p in PC_ONLY or a in ARC_ONLY or p not in pc or a not in arc:
            return False
        if p in match or a in back:
            return False
        match[p], back[a], how[p] = a, p, why
        return True

    for pt, at, n in TABLES:
        words = [pc_word(exe, pt + 4 * i) for i in range(n)]
        for i in range(n):
            if words.count(words[i]) > 1:   # one PC routine in several slots (a stub for kinds the PC dropped)
                continue
            add(words[i], struct.unpack_from("<I", rom, at + 4 * i)[0], "table")
    seeded = sum(add(p, a, "names.tsv") for p, a in hand_pairs())
    print(f"pairs from names.tsv (hand block): {seeded}")
    # other function-pointer tables: runs of (function start | 0) words, at least 4 long with 3 functions, paired when
    # the PC run and an arcade run have the same length and the same zero pattern (and no conflicting known pair)
    def runs(words, base, isfun):
        out, i = [], 0
        while i < len(words):
            j = i
            while j < len(words) and (words[j] == 0 or isfun(words[j])):
                j += 1
            k = j
            while k > i and words[k - 1] == 0:
                k -= 1
            while i < k and words[i] == 0:
                i += 1
            if k - i >= 4 and sum(1 for w in words[i:k] if w) >= 3:
                out.append((base + 4 * i, words[i:k]))
            i = j + 1
        return out
    pcw = struct.unpack_from("<%dI" % (0x6CE400 // 4), exe, 0x187400)
    pruns = runs(pcw, 0x5A0000, lambda w: w in pc)
    arw = struct.unpack_from("<%dI" % (len(rom) // 4), rom, 0)
    aruns = runs(arw, 0, lambda w: w in arc)
    bykey = defaultdict(list)
    for at, ws in aruns:
        bykey[tuple(w == 0 for w in ws)].append((at, ws))
    tabpairs = 0
    for pt, ps in pruns:
        cands = bykey.get(tuple(w == 0 for w in ps), [])
        good = [(at, ws) for at, ws in cands
                if all(match.get(p, a) == a and back.get(a, p) == p for p, a in zip(ps, ws) if p and a)]
        if len(good) == 1:
            at, ws = good[0]
            for p, a in zip(ps, ws):
                if p and a and add(p, a, "table %x/%x" % (pt, at)):
                    tabpairs += 1
    print(f"pointer tables: PC {len(pruns)}, arcade {len(aruns)}; pairs from tables {tabpairs}")
    for p, a in SEEDS + [tuple(int(x, 16) for x in s.split(":")) for s in sys.argv[1:]]:
        add(p, a, "seed")

    gvotes = defaultdict(Counter)
    done_globs = set()

    def propagate(todo):
        while todo:
            p, a = todo.pop()
            pcalls = [c for c in pc[p]["calls"] if c not in PC_ONLY and c != p]
            acalls = [c for c in arc[a]["calls"] if c not in ARC_ONLY and c != a]
            if pcalls and acalls:
                if len(pcalls) == len(acalls):
                    pairs = list(zip(pcalls, acalls))
                else:
                    sm = SequenceMatcher(None, [match.get(c, -1 - i) for i, c in enumerate(pcalls)], acalls, autojunk=False)
                    pairs = []
                    for tag, i1, i2, j1, j2 in sm.get_opcodes():
                        if tag in ("equal", "replace") and i2 - i1 == j2 - j1:
                            pairs += list(zip(pcalls[i1:i2], acalls[j1:j2]))
                for cp, ca in pairs:
                    if add(cp, ca, "calls of %x" % p):
                        todo.append((cp, ca))
            # code pointers stored by the pair (e.g. an enemy's next state routine): aligned like calls
            pr = [c for c in pc[p]["refs"] if c not in PC_ONLY and c != p]
            ar = [c for c in arc[a]["refs"] if c not in ARC_ONLY and c != a]
            if pr and len(pr) == len(ar):
                for cp, ca in zip(pr, ar):
                    if add(cp, ca, "refs of %x" % p):
                        todo.append((cp, ca))
            # function tables the pair indexes (dispatchers): pair entry by entry while both sides are functions or 0
            pt, at = pc[p]["tables"], arc[a]["tables"]
            if pt and len(pt) == len(at):
                for tp, ta in zip(pt, at):
                    for i in range(256):
                        try:
                            wp, wa = pc_word(exe, tp + 4 * i), struct.unpack_from("<I", rom, ta + 4 * i)[0]
                        except struct.error:
                            break
                        if wp == 0 and wa == 0:
                            continue
                        if wp not in pc or wa not in arc:
                            break
                        if add(wp, wa, "table %x/%x" % (tp, ta)):
                            todo.append((wp, wa))

    def vote(p, a):
        if p in done_globs:
            return
        done_globs.add(p)
        pg, ag = pc[p]["globs"], arc[a]["globs"]
        if not pg or not ag:
            return
        if len(pg) == len(ag):
            pairs = zip(pg, ag)
        else:   # align on distinct-global order
            pu = list(dict.fromkeys(pg)); au = list(dict.fromkeys(ag))
            pairs = zip(pu, au) if len(pu) == len(au) else []
        for gp, ga in pairs:
            gvotes[gp][ga] += 1

    # globals referenced per function (sets) and the reverse index for the arcade side
    pset = {f: set(v["globs"]) for f, v in pc.items()}
    aset = {f: set(v["globs"]) for f, v in arc.items()}
    users = defaultdict(set)
    for f, gs in aset.items():
        for g in gs:
            users[g].add(f)

    propagate(list(match.items()))
    for rnd in range(30):
        for p, a in list(match.items()):
            vote(p, a)
        gmap = {gp: c.most_common(1)[0][0] for gp, c in gvotes.items()
                if c.most_common(1)[0][1] >= 2 or len(c) == 1}
        mapped_arc = set(gmap.values())
        cand = {}
        for p in pc:
            if p in match or p in PC_ONLY:
                continue
            S = {gmap[g] for g in pset[p] if g in gmap}
            if len(S) < 2:
                continue
            scores = Counter()
            for g in S:
                for a in users[g]:
                    if a not in back:
                        scores[a] += 1
            best = None
            for a, n in scores.most_common(3):
                A = aset[a] & mapped_arc
                j = n / len(S | A)
                r = pc[p]["size"] / max(arc[a]["size"], 1)
                if j >= 0.5 and 0.25 <= r <= 4 and (best is None or j > best[1]):
                    best = (a, j)
            if best:
                cand[p] = best
        # mutual best only
        bestfor = {}
        for p, (a, j) in cand.items():
            if a not in bestfor or j > bestfor[a][1]:
                bestfor[a] = (p, j)
        new = [(p, a) for a, (p, j) in bestfor.items() if cand[p][0] == a and add(p, a, "globals %.2f" % j)]
        propagate(list(new))
        # shared constants (arcade object offsets may be 8 less than the PC's): at least 4 distinct, mutual best
        if not new:
            apool = [a for a in arc if a not in back and len(arc[a]["consts"]) >= 4]
            aexp = {a: arc[a]["consts"] | {c + 8 for c in arc[a]["consts"] if 0x50 <= c < 0x400} for a in apool}
            ccand = {}
            for p in pc:
                pcs = pc[p]["consts"]
                if p in match or len(pcs) < 4:
                    continue
                best = None
                for a in apool:
                    r = pc[p]["size"] / max(arc[a]["size"], 1)
                    if not 0.33 <= r <= 3:
                        continue
                    n = len(pcs & aexp[a])
                    j = n / max(len(pcs), len(arc[a]["consts"]))
                    if n >= 4 and j >= 0.6 and (best is None or j > best[1]):
                        best = (a, j)
                if best:
                    ccand[p] = best
            bestfor = {}
            for p, (a, j) in ccand.items():
                if a not in bestfor or j > bestfor[a][1]:
                    bestfor[a] = (p, j)
            new = [(p, a) for a, (p, j) in bestfor.items() if ccand[p][0] == a and add(p, a, "consts %.2f" % j)]
            propagate(list(new))
            print(f"round {rnd}: +{len(new)} by constants")
        print(f"round {rnd}: +{len(new)} by globals, total {len(match)}")
        if not new:
            break

    with open(os.path.join(ROOT, "match_functions.tsv"), "w", encoding="utf-8") as f:
        f.write("pc\tarcade\tpc_size\tarcade_size\thow\n")
        for p in sorted(match):
            a = match[p]
            f.write(f"{p:08x}\t{a:08x}\t{pc[p]['size']}\t{arc[a]['size']}\t{how[p]}\n")
    with open(os.path.join(ROOT, "match_globals.tsv"), "w", encoding="utf-8") as f:
        f.write("pc\tarcade\tvotes\tother\n")
        for gp in sorted(gvotes):
            (ga, n), *rest = gvotes[gp].most_common()
            f.write(f"{gp:08x}\t{ga:08x}\t{n}\t{' '.join('%x:%d' % r for r in rest[:3])}\n")
    print(f"functions matched: {len(match)} (PC {len(pc)}, arcade {len(arc)}); globals paired: {len(gvotes)}")


if __name__ == "__main__":
    main()
