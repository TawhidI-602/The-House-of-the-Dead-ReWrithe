#!/usr/bin/env python3
"""
mame_camera.py - read the arcade camera from MAME work RAM dumps (tools\\mame_dump.lua) and write views.json for blender_views.py.

Work RAM 0x51EA50 (dump offset 0x1EA50) holds the view matrix in the geometry engine's layout (MAME transform_point:
x' = m0 x + m3 y + m6 z + m9, y' = m1 x + m4 y + m7 z + m10, z' = m2 x + m5 y + m8 z + m11), world -> camera; camera +z = forward.
The camera position is the point that maps to (0, 0, 0). Found 2026-10-07 in Chapter 1 dumps; check it for other stages.

Usage:
  python mame_camera.py <dump folder> <out views.json> <tag> [<tag> ...]     (tags from index.txt, e.g. t00480)
"""
import sys, os, json
import numpy as np

VIEW_MATRIX = 0x1EA50


def camera(dump_folder, tag):
    m = np.fromfile(os.path.join(dump_folder, f"{tag}_workram.bin"), dtype="<f4")[VIEW_MATRIX // 4:VIEW_MATRIX // 4 + 12].astype(np.float64)
    rot = np.array([[m[0], m[3], m[6]], [m[1], m[4], m[7]], [m[2], m[5], m[8]]])   # rows: camera x, y, z axes in world
    t = m[9:12]
    pos = -rot.T @ t
    return {"pos": pos.tolist(), "right": rot[0].tolist(), "down_or_up": rot[1].tolist(), "forward": rot[2].tolist()}


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print(__doc__); sys.exit(1)
    folder, out, tags = sys.argv[1], sys.argv[2], sys.argv[3:]
    snaps = sorted(os.listdir(os.path.join(folder, "snap", "hotd")))
    index = [l.split()[0] for l in open(os.path.join(folder, "index.txt"))]
    views = []
    for tag in tags:
        v = camera(folder, tag)
        v["name"] = tag
        v["screenshot"] = os.path.join(folder, "snap", "hotd", snaps[index.index(tag)])
        views.append(v)
        print(tag, "pos", np.round(v["pos"], 1), "forward", np.round(v["forward"], 3))
    json.dump(views, open(out, "w"), indent=1)
