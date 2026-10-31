"""Arcade sound codes -> PC cue numbers (docs/runtime.md "Sound").

The arcade's sound_cue (0x154D0) takes a sound-board command (0xA0xxxx); the PC's (0x412000) an index into its cue
table (THOTD.EXE 0x5CBDA8). The main source (overnight audio, 2026-10-16): the arcade's list of every sound code
(program ROM 0xA8CDC, the sound test's order) is the PC table's order, list entry k = PC cue k + 6; all 77 routine
pairs agree with it. For codes outside the list: the routine pairs of private/decomp/match_functions.tsv: the sound_cue
constants of both versions' decompiled C, in order, paired where both routines pass the same number of constants.

Then the event scripts: the SNDON (opcode 90) operands of the arcade program ROM and of the PC's EVTALL.RBH, aligned
(Needleman-Wunsch; a pair already known must agree, music 0xA010nn is cue 8 + nn) for the codes only the scripts use.

  py tools/cue_pairs.py <THOTD folder> [out.tsv]      writes arcade_code <tab> pc_cue <tab> source (default: stdout); a .inc out = C++ rows (src/game/cue_map.inc)
"""
import collections, re, struct, sys, os

HERE = os.path.dirname(os.path.abspath(__file__))
PRIVATE = os.environ.get("HOTD_PRIVATE", os.path.join(HERE, "..", "private"))   # a worktree: the main checkout's
PRIV = os.path.join(PRIVATE, "decomp")
CUE = re.compile(r"\bsound_cue\((0x[0-9a-fA-F]+|\d+)\)")
HEAD = re.compile(r"^[A-Za-z_][\w \*]*?\b(\w+)\([^;]*\)\s*$", re.M)


def bodies(path, addr_of):
    text = open(path, encoding="utf-8", errors="replace").read()
    out = {}
    heads = list(HEAD.finditer(text))
    for i, m in enumerate(heads):
        end = heads[i + 1].start() if i + 1 < len(heads) else len(text)
        a = addr_of(m.group(1))
        if a is not None:
            out[a] = [int(c, 0) for c in CUE.findall(text, m.end(), end)]
    return out


def index(path):
    names = {}
    for line in open(path, encoding="utf-8"):
        f = line.rstrip("\n").split("\t")
        if len(f) > 1 and re.fullmatch(r"[0-9a-fA-F]+", f[0]):
            names[f[1]] = int(f[0], 16)
    return names


def sndon(words, ok):
    return [words[i + 1] for i in range(len(words) - 1) if words[i] == 90 and ok(words[i + 1])]


def script_pairs(known):
    rom = open(os.path.join(PRIVATE, "arcade_exports", "regions", "maincpu.bin"), "rb").read()
    evt = open(os.path.join(sys.argv[1], "EVT", "ALLSTAGE", "EVTALL.RBH"), "rb").read()[0x28:]
    a = sndon(struct.unpack("<%dI" % (len(rom) // 4), rom[:len(rom) // 4 * 4]), lambda v: v >> 16 == 0xA0)
    p = sndon(struct.unpack("<%dI" % (len(evt) // 4), evt[:len(evt) // 4 * 4]), lambda v: v < 269)

    def want(c):
        return known.get(c, 8 + (c & 0xFF) if c >> 8 == 0xA010 else None)

    def score(c, q):
        w = want(c)
        return 1 if w is None else (3 if w == q else -6)
    n, m = len(a), len(p)
    S = [[0.0] * (m + 1) for _ in range(n + 1)]
    for i in range(1, n + 1):
        S[i][0] = -i
    for j in range(1, m + 1):
        S[0][j] = -j
    for i in range(1, n + 1):
        for j in range(1, m + 1):
            S[i][j] = max(S[i - 1][j - 1] + score(a[i - 1], p[j - 1]), S[i - 1][j] - 1, S[i][j - 1] - 1)
    i, j, out = n, m, []
    while i and j:
        if S[i][j] == S[i - 1][j - 1] + score(a[i - 1], p[j - 1]):
            if want(a[i - 1]) is None or want(a[i - 1]) == p[j - 1]:
                out.append((a[i - 1], p[j - 1]))
            i, j = i - 1, j - 1
        elif S[i][j] == S[i - 1][j] - 1:
            i -= 1
        else:
            j -= 1
    return out


def sound_list():
    rom = open(os.path.join(PRIVATE, "arcade_exports", "regions", "maincpu.bin"), "rb").read()
    w = struct.unpack("<%dI" % (len(rom) // 4), rom[:len(rom) // 4 * 4])
    i = next(i for i in range(len(w) - 3) if w[i:i + 3] == (0xA00002, 0xA01000, 0xA01001)) - 1
    j = i + 1
    while w[j] >> 16 == 0xA0:
        j += 1
    return list(w[i:j])


def main():
    ai = index(os.path.join(PRIV, "arcade", "maincpu_index.tsv"))
    pi = index(os.path.join(PRIV, "pc", "thotd_index.tsv"))
    ab = bodies(os.path.join(PRIV, "arcade", "maincpu.c"), ai.get)
    pb = bodies(os.path.join(PRIV, "pc", "thotd.c"), pi.get)
    votes = collections.defaultdict(collections.Counter)
    for line in open(os.path.join(PRIV, "match_functions.tsv"), encoding="utf-8"):
        f = line.split("\t")
        if f[0] == "pc":
            continue
        p, a = int(f[0], 16), int(f[1], 16)
        ca, cp = ab.get(a, []), pb.get(p, [])
        if ca and len(ca) == len(cp):
            for x, y in zip(ca, cp):
                votes[x][y] += 1
    code = {c: v.most_common(1)[0][0] for c, v in votes.items()}
    # the arcade's list of every sound code (program ROM 0xA8CDC, 261 words: 0, 0xA00002, the music 0xA01000..,
    # the effects 0xA011xx.., the voices 0xA014xx ..0xA01475) is the PC cue table's order: list entry k = PC cue k + 6
    # (PC cues 0-7: none and the commands); every routine pair above agrees with it
    known, how = {}, {}
    for k, c in enumerate(sound_list()):
        if k >= 2:
            known[c], how[c] = k + 6, "list"
    for c, q in code.items():
        if c in known and known[c] != q:
            sys.stderr.write("routine pair 0x%06X -> %d disagrees with the list (%d)\n" % (c, q, known[c]))
        elif c not in known and c >> 16 == 0xA0:
            known[c], how[c] = q, "code"
    # the PC table keeps the arcade's order: a script pair is taken only between its trusted neighbours
    cand = collections.defaultdict(collections.Counter)
    for code, cue in script_pairs(known):
        if code not in known:
            cand[code][cue] += 1
    keys = sorted(known)
    for code in sorted(cand):
        lo = max([k for k in keys if k < code], default=None)
        hi = min([k for k in keys if k > code], default=None)
        for cue, n in cand[code].most_common():
            if (lo is None or known[lo] < cue) and (hi is None or cue < known[hi]):
                known[code] = cue
                how[code] = "script"
                break
    out = open(sys.argv[2], "w") if len(sys.argv) > 2 else sys.stdout
    if out.name.endswith(".inc"):     # src/game/cue_map.inc
        out.write("// generated by tools/cue_pairs.py: {arcade sound code, PC cue}; list = the arcade sound list (ROM 0xA8CDC), code = routine pairs, script = SNDON alignment\n")
        for code in sorted(known):
            out.write("{0x%06X, %d},   // %s\n" % (code, known[code], how[code]))
        return
    out.write("# arcade sound code -> PC cue (tools/cue_pairs.py): code from routine pairs, script from SNDON alignment\n")
    for code in sorted(known):
        out.write("%06X\t%d\t%s\n" % (code, known[code], how[code]))


main()
