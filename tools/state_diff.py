#!/usr/bin/env python3
"""
state_diff.py - read and compare per-frame state records (the differential checker, docs/decompilation.md
"Differential checker (plan)"). Records come from tools/mame_state_rec.lua (arcade, run_state_rec.ps1); the rewrite will
write the same format.

  py tools/state_diff.py info <state.rec>                 frames, game modes, task counts, rand_state calls
  py tools/state_diff.py show <state.rec> <frame> [--tasks]  the globals (and tasks) of one frame, by name
  py tools/state_diff.py diff <a.rec> <b.rec> [--max N] [--from F] [--ignore name,name]
                                                          first frames / fields that differ (frames matched by number;
                                                          tasks matched by run order: a different task list is reported
                                                          as such)
  py tools/state_diff.py rand <state.rec> [--from F --to T]  rand calls per frame (LCG steps between frames)

Field names: globals from the record's map (tools/state_map.tsv names, matched by address), task fields from the Obj
struct in tools/ghidra/types.tsv (arcade offsets). Floats are compared bit for bit.
"""
import os, re, struct, sys
from collections import Counter

HERE = os.path.dirname(os.path.abspath(__file__))
TYPES = os.path.join(HERE, "ghidra", "types.tsv")
MAP = os.path.join(HERE, "state_map.tsv")
BASE = {"char": 1, "byte": 1, "uchar": 1, "short": 2, "ushort": 2, "int": 4, "uint": 4, "float": 4, "code": 4}


class Frame:
    __slots__ = ("number", "counter", "mode", "rand", "globals", "tasks")


def read_rec(path):
    """yields (ranges, Frame) for each frame; ranges = [(address, size)]"""
    with open(path, "rb") as f:
        data = f.read()
    if data[:8] != b"HOTDREC1":
        raise SystemExit("%s: not a state record" % path)
    n = struct.unpack_from("<I", data, 8)[0]
    pos = 12
    ranges = []
    for _ in range(n):
        ranges.append(struct.unpack_from("<II", data, pos))
        pos += 8
    gsize = sum(s for _, s in ranges)
    frames = []
    while pos + 20 <= len(data):
        if data[pos:pos + 4] != b"FRAM":
            raise SystemExit("%s: bad frame at byte %d" % (path, pos))
        fr = Frame()
        fr.number, fr.counter, fr.mode, fr.rand = struct.unpack_from("<IIII", data, pos + 4)
        pos += 20
        fr.globals = data[pos:pos + gsize]
        pos += gsize
        if pos + 4 > len(data):
            break   # cut off (MAME stopped while writing)
        nt = struct.unpack_from("<I", data, pos)[0]
        pos += 4
        tasks = []
        for _ in range(nt):
            if pos + 8 > len(data):
                break
            a, s = struct.unpack_from("<II", data, pos)
            tasks.append((a, data[pos + 8:pos + 8 + s]))
            pos += 8 + s
        fr.tasks = tasks
        frames.append(fr)
    return ranges, frames


def load_map_names():
    names = {}
    if os.path.exists(MAP):
        for line in open(MAP, encoding="utf-8"):
            if line.startswith("#"):
                continue
            c = line.rstrip("\n").split("\t")
            names[int(c[1], 16)] = (c[0], c[5])
            if len(c) > 3 and c[3] != "-":
                names[int(c[3], 16)] = (c[0], c[5])   # PC records (tools/pcharness) use the PC addresses
    return names


def load_struct_fields(struct_name):
    """[(offset, size, name)] for a struct (arcade offsets), with extends resolved"""
    fields, base = {}, None
    sizes = {}
    lines = [l.rstrip("\n").split("\t") for l in open(TYPES, encoding="utf-8") if not l.startswith("#")]
    for c in lines:
        if c[0] == "struct":
            sizes[c[1]] = int(c[2], 16)
    def collect(name):
        out = {}
        for c in lines:
            if c[0] == "extends" and c[1] == name:
                out.update(collect(c[2]))
        for c in lines:
            if c[0] == "field" and c[1] == name and c[2] != "-":
                m = re.match(r"^(\w+)(\**)(?:\[(\d+)\])?$", c[4])
                if not m:
                    continue
                el = 4 if m.group(2) else BASE.get(m.group(1), sizes.get(m.group(1), 4))
                out[int(c[2], 16)] = (el, int(m.group(3) or 1), c[5], m.group(1) if not m.group(2) else "ptr")
        return out
    for off, (el, n, name, t) in collect(struct_name).items():
        fields[off] = (el, n, name, t)
    return fields


def field_name(fields, off):
    """name of the byte at offset off (e.g. pos[1], or +0x2f3 when not typed)"""
    best = None
    for o, (el, n, name, t) in fields.items():
        if o <= off < o + el * n:
            best = name + ("[%d]" % ((off - o) // el) if n > 1 else "")
    return best or "+0x%x" % off


def global_name(ranges, names, gpos):
    a = 0
    for addr, size in ranges:
        if gpos < a + size:
            nm = names.get(addr, ("g_%x" % addr, "-"))
            return "%s+0x%x (0x%x)" % (nm[0], gpos - a, addr + gpos - a), addr + gpos - a
        a += size
    return "?", 0


def value_at(blob, off, size=4):
    off &= ~3
    return blob[off:off + 4].hex() if off + 4 <= len(blob) else blob[off:].hex()


def cmd_info(path):
    ranges, frames = read_rec(path)
    if not frames:
        print("no frames"); return
    print("%s: %d frames (%d .. %d), %d globals ranges (%d bytes)" % (path, len(frames), frames[0].number,
          frames[-1].number, len(ranges), sum(s for _, s in ranges)))
    runs, cur = [], None
    for fr in frames:
        if cur is None or fr.mode != cur[0]:
            cur = [fr.mode, fr.number, fr.number]
            runs.append(cur)
        cur[2] = fr.number
    print("game_mode runs: " + ", ".join("mode %d frames %d-%d" % tuple(r) for r in runs))
    tc = [len(fr.tasks) for fr in frames]
    print("tasks per frame: min %d, max %d, mean %.1f" % (min(tc), max(tc), sum(tc) / len(tc)))
    seeds = Counter(fr.rand for fr in frames)
    print("rand_state values: %d distinct; frames where it is 0: %s" % (len(seeds), [fr.number for fr in frames if fr.rand == 0][:20]))


def lcg_steps(a, b, limit=5000):
    x = a
    for i in range(limit + 1):
        if x == b:
            return i
        x = (x * 0x41C64E6D + 0x3039) & 0xFFFFFFFF
    return None


def cmd_rand(path, lo, hi):
    ranges, frames = read_rec(path)
    prev = None
    line = []
    for fr in frames:
        if prev is not None and lo <= fr.number <= hi:
            n = lcg_steps(prev.rand, fr.rand)
            line.append("%d:%s" % (fr.number, "?" if n is None else n))
        prev = fr
    print(" ".join(line))


def cmd_show(path, number, with_tasks):
    ranges, frames = read_rec(path)
    names = load_map_names()
    fr = next((f for f in frames if f.number == number), None)
    if fr is None:
        raise SystemExit("frame %d not in the record" % number)
    print("frame %d: frame_counter %d, game_mode %d, rand_state %08x, %d tasks" % (fr.number, fr.counter, fr.mode, fr.rand, len(fr.tasks)))
    a = 0
    for addr, size in ranges:
        nm = names.get(addr, ("g_%x" % addr, "-"))
        print("  %-24s %06x %s" % (nm[0], addr, fr.globals[a:a + size].hex() if size <= 32 else fr.globals[a:a + 32].hex() + "..."))
        a += size
    if with_tasks:
        fields = load_struct_fields("Obj")
        for t, blob in fr.tasks:
            routine, = struct.unpack_from("<I", blob, 0)
            extra = ""
            if len(blob) >= 0xD4:
                pos = struct.unpack_from("<3f", blob, 0x20)
                st = struct.unpack_from("<hhh", blob, 0x5C)
                extra = " pos (%.2f %.2f %.2f) state %d/%d/%d type %d motion %d" % (pos + st + struct.unpack_from("<hxxh", blob, 0xCC))
            print("  task %06x size %x routine %06x%s" % (t, len(blob), routine, extra))


def cmd_diff(pa, pb, maxn, start, ignore):
    ra, fa = read_rec(pa)
    rb, fb = read_rec(pb)
    names = load_map_names()
    fields = load_struct_fields("Obj")
    if ra != rb:
        print("note: the two records use different maps; globals compared by address where both have them")
    byn = {f.number: f for f in fb}
    shown = 0
    compared = 0
    for x in fa:
        if x.number < start:
            continue
        y = byn.get(x.number)
        if y is None:
            continue
        compared += 1
        diffs = []
        for nm, va, vb in (("frame_counter", x.counter, y.counter), ("game_mode", x.mode, y.mode), ("rand_state", x.rand, y.rand)):
            if va != vb:
                diffs.append("%s: %x / %x" % (nm, va, vb))
        if ra == rb and x.globals != y.globals:
            seen = set()
            for i in range(len(x.globals)):
                if x.globals[i] != y.globals[i]:
                    name, addr = global_name(ra, names, i)
                    key = name.split("+")[0]
                    if key in ignore or (key, addr & ~3) in seen:
                        continue
                    seen.add((key, addr & ~3))
                    diffs.append("%s: %s / %s" % (name, value_at(x.globals, i), value_at(y.globals, i)))
        ta = [t for t, _ in x.tasks]
        tb = [t for t, _ in y.tasks]
        if ta != tb:
            diffs.append("task lists differ: %d / %d tasks; first difference at run position %s" % (
                len(ta), len(tb), next((i for i in range(min(len(ta), len(tb))) if ta[i] != tb[i]), min(len(ta), len(tb)))))
        for (t, a), (u, b) in zip(x.tasks, y.tasks):
            if t != u or a == b:
                continue
            for i in range(min(len(a), len(b))):
                if a[i] != b[i]:
                    nm = field_name(fields, i)
                    if nm.split("[")[0] in ignore:
                        continue
                    diffs.append("task %06x %s: %s / %s" % (t, nm, value_at(a, i), value_at(b, i)))
                    break
        if diffs:
            print("frame %d (mode %d):" % (x.number, x.mode))
            for d in diffs[:20]:
                print("  " + d)
            if len(diffs) > 20:
                print("  ... %d more" % (len(diffs) - 20))
            shown += 1
            if shown >= maxn:
                break
    print("%d frames compared, %s" % (compared, "%d with differences shown" % shown if shown else "all equal"))


def main():
    a = sys.argv[1:]
    def opt(name, default):
        if name in a:
            i = a.index(name)
            v = a[i + 1]
            del a[i:i + 2]
            return v
        return default
    if not a:
        print(__doc__); return
    if a[0] == "info":
        cmd_info(a[1])
    elif a[0] == "show":
        tasks = "--tasks" in a
        if tasks:
            a.remove("--tasks")
        cmd_show(a[1], int(a[2]), tasks)
    elif a[0] == "rand":
        lo, hi = int(opt("--from", "0")), int(opt("--to", "99999999"))
        cmd_rand(a[1], lo, hi)
    elif a[0] == "diff":
        maxn = int(opt("--max", "5"))
        start = int(opt("--from", "0"))
        ignore = set(opt("--ignore", "").split(",")) - {""}
        cmd_diff(a[1], a[2], maxn, start, ignore)
    else:
        print(__doc__)


if __name__ == "__main__":
    main()
