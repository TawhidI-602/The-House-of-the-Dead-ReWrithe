#!/usr/bin/env python3
"""
arcade_states.py - which of several placements at the same position the game shows where (formats.md §27).

Some placement-table entries share one position: alternate states of one object (intact / damaged, open / closed door,
the mansion entrance 0x9E3 / 0x9E4 / 0x9E5); the game shows one of them at a time, switched by its scene events. Until
the event scripts are emulated, this tool records from a replay where each state was drawn: for every group of placements
at one position that the replay never (or once) draws together, the camera positions of the frames that drew each member.
The viewer then shows, per group, the member drawn nearest to the camera (members never drawn are hidden).

Usage:
  python arcade_states.py <regions folder> <dump folder> <out file>      (keep the output in private\\)
Output lines:
  group <chapter 0-3> <id> <id> ...          one group (hex IDs)
  seen <chapter> <hex id> x y z              a camera position where the replay drew that member
"""
import sys, os, struct, collections, glob
import numpy as np
import m2_displaylist
from mame_camera import camera

TABLES = (0xBEF00, 0xBFEA0, 0xC0640, 0xC0A90)
STAGE_BYTE = 0x1F9D1   # work RAM: 0-3 = Chapters 1-4


def groups(cpu):
    out = {}
    for ch, a in enumerate(TABLES):
        pos = collections.defaultdict(list)
        while True:
            r = struct.unpack_from("<IfffII", cpu, a)
            if r[0] == 0xFFFFFFFF:
                break
            key = (round(r[1], 3), round(r[2], 3), round(r[3], 3))
            if r[0] not in pos[key]:
                pos[key].append(r[0])
            a += 24
        out[ch] = [ids for ids in pos.values() if len(ids) > 1]
    return out


def main(regions, dumps, out_path):
    cpu = open(os.path.join(regions, "maincpu.bin"), "rb").read()
    md = np.fromfile(os.path.join(regions, "main_data.bin"), dtype="<u4")
    id_of = m2_displaylist.model_ids(md)
    gs = groups(cpu)
    seen = collections.defaultdict(list)       # (chapter, id) -> camera positions
    together = collections.Counter()
    for f in sorted(glob.glob(os.path.join(dumps, "*_bufram.bin"))):
        tag = os.path.basename(f)[:-len("_bufram.bin")]
        w = os.path.join(dumps, tag + "_workram.bin")
        if not os.path.exists(w):
            continue
        ch = open(w, "rb").read()[STAGE_BYTE]
        if ch > 3:
            continue
        drawn = {o for o, *_ in m2_displaylist.drawn(f, id_of)}
        cam = None
        for g in gs[ch]:
            s = [i for i in g if i in drawn]
            if len(s) > 1:
                together[(ch, tuple(g))] += 1
            for i in s:
                if cam is None:
                    cam = camera(dumps, tag)["pos"]
                seen[(ch, i)].append(cam)
    with open(out_path, "w") as o:
        o.write(f"# arcade_states.py {dumps}\n")
        for ch in range(4):
            for g in gs[ch]:
                if together[(ch, tuple(g))] > 1:
                    continue   # drawn together: not alternates
                o.write(f"group {ch} " + " ".join(f"{i:x}" for i in g) + "\n")
                for i in g:
                    for p in seen[(ch, i)]:
                        o.write(f"seen {ch} {i:x} {p[0]:.2f} {p[1]:.2f} {p[2]:.2f}\n")
    print("wrote", out_path)


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(__doc__)
        sys.exit(1)
    main(*sys.argv[1:])
