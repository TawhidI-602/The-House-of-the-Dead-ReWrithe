#!/usr/bin/env python3
"""
texset_seen.py - add to a texture set file (texset_assign.py output) every set the replay drew each model in.

texset_assign.py and texset_pcmatch.py sometimes leave out a set a model is really drawn with (e.g. 0x1AAF, the Chapter 4
Hangedman rematch room, was only "valid" in set 3 although the game draws it while area 10 = set 1 is loaded; 0x923, the
Chapter 1 courtyard fence). The game always draws with the texture RAM of the loaded area, so a model drawn while area A
is loaded is valid in A's set. This tool scans every dump of a replay (stage byte, area byte, display list) and appends
"valid" lines (the viewer adds the sets of later lines to earlier ones).

Usage:
  python texset_seen.py <regions folder> <dump folder> <texsets file>      (appends to the texsets file, in private\\)
"""
import sys, os, glob, collections
import numpy as np
import m2_displaylist

STAGE_BYTE, AREA_BYTE = 0x1F9D1, 0x1F9B4


def main(regions, dumps, texsets):
    md = np.fromfile(os.path.join(regions, "main_data.bin"), dtype="<u4")
    id_of = m2_displaylist.model_ids(md)
    sets = {}
    for line in open(texsets):
        if line.startswith("sets "):
            f = line.split()
            sets[int(f[1])] = [int(x.split("/")[0][4:]) if x.startswith("rom:") else None for x in f[2:]]
    seen = collections.defaultdict(set)
    for f in sorted(glob.glob(os.path.join(dumps, "*_bufram.bin"))):
        w = os.path.join(dumps, os.path.basename(f)[:-len("_bufram.bin")] + "_workram.bin")
        if not os.path.exists(w):
            continue
        wr = open(w, "rb").read()
        stage, area = wr[STAGE_BYTE], wr[AREA_BYTE]
        if stage not in sets or area not in sets[stage]:
            continue
        s = sets[stage].index(area)
        for oid, *_ in m2_displaylist.drawn(f, id_of):
            seen[(stage, oid)].add(s)
    with open(texsets, "a") as o:
        o.write("# from texset_seen.py (sets the replay drew each model in; adds to the lines above)\n")
        for (stage, oid), ss in sorted(seen.items()):
            o.write(f"valid {stage} {oid:x} " + " ".join(str(x) for x in sorted(ss)) + "\n")
    print("appended", len(seen), "valid lines to", texsets)


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(__doc__); sys.exit(1)
    main(*sys.argv[1:])
