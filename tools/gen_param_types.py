#!/usr/bin/env python3
"""
gen_param_types.py - finds the arcade functions whose parameters are game objects (struct Obj, tools/ghidra/types.tsv)
and writes "param" lines for them (with the PC pair from private/decomp/match_functions.tsv) into a generated block of
tools/ghidra/types.tsv, for ApplyTypes.java.

A parameter counts as an object when the function reads or writes at least two of the object's characteristic fields
through it (state +0x5C, sub-state +0x60, action +0x64, type +0xCC, motion +0xD0, frame +0xF4, slots +0x2E6..+0x2EB,
record +0x368, flags +0x3C4.. ...), or one of them when the function is itself an object routine (stored into an
object's +0x00, created by task_create, or in the kind / prop tables). Offsets beyond the object (> 0x3C8) or a
name from a non-object family (text windows, camera jobs) rule it out. Hand lines outside the generated block win.

  py tools/gen_param_types.py          # rewrite the generated block, print a summary
  py tools/gen_param_types.py --dry
"""
import os, re, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
TYPES = os.path.join(HERE, "ghidra", "types.tsv")
BEGIN, END = "# BEGIN gen_param_types.py (generated: object parameters, see the tool)", "# END gen_param_types.py"
SIG = {0x5C, 0x60, 0x64, 0x66, 0xCC, 0xCE, 0xD0, 0xD2, 0xF4, 0xF6, 0xF8, 0xFE, 0x2E5, 0x2E6, 0x2E7, 0x2E8, 0x2EB,
       0x35C, 0x368, 0x37C, 0x3C4, 0x3C5, 0x3C6, 0x3C7, 0x58, 0x54, 0x90, 0x94}
NOT_OBJ = re.compile(r"^(window_|text_|test_|cam_job|camera_|eeprom_|bookkeeping|sprite_|tilemap_|palette_|geo_|"
                     r"mem_|task_group|coin_|credit_|gun_)")
# object families with their own struct (types.tsv "extends ... Obj"): function name -> struct for its object parameters
FAMILY = [(re.compile(r"^(enemy_|EN[A-Z]|enguard|kind00[03489]_|kind012_|kind096_|kind102_|Zombie_)"), "Enemy")]
ELEM = {"undefined4": 4, "int": 4, "uint": 4, "float": 4, "undefined2": 2, "short": 2, "ushort": 2, "undefined1": 1,
        "byte": 1, "char": 1, "undefined": 1, "code": 4}


def load_c(path):
    funcs, cur = {}, None
    for line in open(path, encoding="utf-8", errors="replace"):
        m = re.match(r"// ==== (\S+) @ ([0-9a-f]+) \((\d+) bytes\)", line)
        if m:
            cur = int(m.group(2), 16)
            funcs[cur] = [m.group(1), []]
        elif cur is not None:
            funcs[cur][1].append(line)
    return {a: (n, "".join(b)) for a, (n, b) in funcs.items()}


def param_types(text):
    """index -> (element size of a pointer parameter, 0 = int-like; variable name; already typed Obj *), from the C
    signature."""
    m = re.search(r"^\w[\w\s\*]*?\b\w+\(([^)]*)\)\s*$", text, re.M)
    out = {}
    if not m:
        return out
    for i, p in enumerate(m.group(1).split(",")):
        pm = re.match(r"\s*(\w+)\s*(\*?)\s*(\w+)\s*$", p)
        if pm and pm.group(1) != "void":
            elem = ELEM.get(pm.group(1), 4) if pm.group(2) else 0
            out[i + 1] = (elem, pm.group(3), pm.group(1) in ["Obj"] + [t for _, t in FAMILY] and pm.group(2) == "*")
    return out


def offsets(text, v, elem):
    """offsets used through the parameter variable v."""
    offs = set()
    for m in re.finditer(r"\(\(int\)%s \+ (0x[0-9a-f]+|\d+)\)" % v, text):
        offs.add(int(m.group(1), 0))
    if elem == 0:
        for m in re.finditer(r"\(%s \+ (0x[0-9a-f]+|\d+)\)" % v, text):
            offs.add(int(m.group(1), 0))
    else:
        for m in re.finditer(r"[^)]\(%s \+ (0x[0-9a-f]+|\d+)\)" % v, text):
            offs.add(int(m.group(1), 0) * elem)
        for m in re.finditer(r"%s\[(0x[0-9a-f]+|\d+)\]" % v, text):
            offs.add(int(m.group(1), 0) * elem)
        if re.search(r"\*%s\b" % v, text):
            offs.add(0)
    return offs


def obj_like(offs):
    """object fields among the offsets (characteristic fields and the work area 0x2E0..0x3C7), or None when an
    offset lies beyond an object."""
    if offs and max(offs) >= 0x3C8:
        return None
    return offs & SIG | {o for o in offs if 0x2E0 <= o < 0x3C8}


def main():
    dry = "--dry" in sys.argv
    funcs = load_c(os.path.join(ROOT, "private", "decomp", "arcade", "maincpu.c"))
    pc_of = {}
    for line in open(os.path.join(ROOT, "private", "decomp", "match_functions.tsv"), encoding="utf-8").read().splitlines()[1:]:
        f = line.split("\t")
        pc_of[int(f[1], 16)] = int(f[0], 16)
    # object routines: stored into +0 of an object, created as tasks, or in the kind / prop tables
    routines = set()
    names = {n: a for a, (n, _) in funcs.items()}
    for a, (n, text) in funcs.items():
        for m in re.finditer(r"\*\w+ = (\w+);", text):
            if m.group(1) in names:
                routines.add(names[m.group(1)])
        for m in re.finditer(r"task_create\((\w+),", text):
            if m.group(1) in names:
                routines.add(names[m.group(1)])
    rom = open(os.path.join(ROOT, "private", "arcade_exports", "regions", "maincpu.bin"), "rb").read()
    for base, n in ((0xAFDE0, 192), (0xAF960, 125)):
        for i in range(n):
            routines.add(struct.unpack_from("<I", rom, base + 4 * i)[0])
    lines = open(TYPES, encoding="utf-8").read().splitlines()
    if BEGIN in lines:
        i, j = lines.index(BEGIN), lines.index(END)
        lines = lines[:i] + lines[j + 1:]
    hand = set()
    for l in lines:
        c = l.split("\t")
        if c[0] == "param" and c[1] != "-":
            hand.add((int(c[1], 16), int(c[3])))
    sigs = {a: param_types(text) for a, (n, text) in funcs.items()}
    typed, usable = set(), {}
    for a, (n, text) in sorted(funcs.items()):
        if a >= 0xA6588 or NOT_OBJ.match(n):
            continue
        for k, (elem, v, is_obj) in sigs[a].items():
            if (a, k) in hand:
                continue
            if is_obj:                      # typed by an earlier run: keep it
                typed.add((a, k))
                continue
            o = obj_like(offsets(text, v, elem))
            if o is None:
                continue
            usable[(a, k)] = bool(o)
            if len(o) >= 2 or (o and k == 1 and a in routines):
                typed.add((a, k))
    # propagation through calls: g(x) with g's parameter an object makes the caller's parameter x one, and an object
    # parameter passed on makes the callee's parameter one, when that side uses at least one object field through it
    calls = []
    for a, (n, text) in funcs.items():
        var = {v: k for k, (elem, v, is_obj) in sigs[a].items()}
        for m in re.finditer(r"\b(\w+)\(([^;{}]*?)\);", text):
            if m.group(1) not in names:
                continue
            for i, x in enumerate(y.strip() for y in m.group(2).split(",")):
                pm = re.fullmatch(r"(?:\(\w+ ?\*?\))?(\w+)", x)
                if pm and pm.group(1) in var:
                    calls.append((a, var[pm.group(1)], names[m.group(1)], i + 1))
    typed |= hand
    added = True
    while added:
        added = False
        for a, k, g, i in calls:
            for src, dst in (((g, i), (a, k)), ((a, k), (g, i))):
                if src in typed and dst not in typed and usable.get(dst) and dst[0] < 0xA6588 \
                        and not NOT_OBJ.match(funcs[dst[0]][0]):
                    typed.add(dst)
                    added = True
    # player tasks (task size 0x24): the only field used is the player number +0x20 (as a char)
    ptask = set()
    for a, (n, text) in funcs.items():
        if a >= 0xA6588 or 1 not in sigs[a] or (a, 1) in hand or (a, 1) in typed:
            continue
        elem, v, is_obj = sigs[a][1]
        if "PlayerTask" in text.split("{")[0]:
            ptask.add(a)
            continue
        offs = offsets(text, v, elem)
        if offs and offs <= {0, 0x20} and 0x20 in offs and re.search(
                r"\*\(char \*\)\((?:\(int\))?%s \+ (?:0x20|8)\)" % v, text):
            ptask.add(a)
    out = []
    for a in sorted(ptask):
        pc = pc_of.get(a)
        out.append("	".join(["param", f"{a:x}", f"{pc:x}" if pc is not None else "-", "1", "PlayerTask*", "task"]))
    for a, k in sorted(typed - hand):
        pc = pc_of.get(a)
        st = next((t for rx, t in FAMILY if rx.match(funcs[a][0])), "Obj") if k == 1 else "Obj"
        out.append("\t".join(["param", f"{a:x}", f"{pc:x}" if pc is not None else "-", str(k), st + "*",
                              "obj" if k == 1 else "obj%d" % k]))
    nfunc = len({l.split("\t")[1] for l in out})
    print(f"object parameters: {len(out)} in {nfunc} functions ({sum(1 for l in out if not l.split(chr(9))[2] == '-')} with a PC pair)")
    if not dry:
        lines += [BEGIN] + out + [END]
        open(TYPES, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
