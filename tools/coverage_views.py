#!/usr/bin/env python3
"""
coverage_views.py - one view per group of never-visited models (from coverage.py), for recognizing missed branch routes.

For each group: the camera sample of the run that came closest (same chapter), looking at the group's center, slightly from above.
Writes views_<name>.json (for blender_views.py) and prints which dump tag each view should take its texture RAM/colors from.

Usage:
  python coverage_views.py <coverage folder> <dump folder> <out folder> <name>=<chapter>:<id>,<id>,... [...]
"""
import sys, os, csv, json
import numpy as np


def main(cov, dumps, out, groups):
    os.makedirs(out, exist_ok=True)
    rows = list(csv.DictReader(open(os.path.join(cov, "coverage.csv"))))
    cams = [r for r in csv.DictReader(open(os.path.join(cov, "camera_chapters.csv"))) if r["x"]]
    snaps = sorted(os.listdir(os.path.join(dumps, "snap", "hotd")))
    index = [l.split()[0] for l in open(os.path.join(dumps, "index.txt"))]
    texram_tags = sorted(f[:6] for f in os.listdir(dumps) if f.endswith("_texram.bin"))
    plan = []
    for g in groups:
        name, spec = g.split("=")
        ch, ids = spec.split(":")
        ids = {int(i, 16) for i in ids.split(",")}
        boxes = [r for r in rows if r["chapter"] == ch and int(r["model_id"], 16) in ids]
        lo = np.min([[float(x) for x in r["box_min_xyz"].split()] for r in boxes], axis=0)
        hi = np.max([[float(x) for x in r["box_max_xyz"].split()] for r in boxes], axis=0)
        center = (lo + hi) / 2
        chcams = [c for c in cams if c["chapter"] == ch]
        dist = [np.linalg.norm(np.array([float(c["x"]), float(c["y"]), float(c["z"])]) - center) for c in chcams]
        near = chcams[int(np.argmin(dist))]
        pos = np.array([float(near["x"]), float(near["y"]), float(near["z"])])
        pos[1] += 60.0                                   # a bit higher (+y = up) to see over low walls
        fwd = center - pos
        fwd /= np.linalg.norm(fwd)
        view = {"name": name, "pos": pos.tolist(), "forward": fwd.tolist(), "down_or_up": [0.0, 1.0, 0.0],
                "right": [0, 0, 0], "screenshot": os.path.join(dumps, "snap", "hotd", snaps[index.index(near["tag"])])}
        json.dump([view], open(os.path.join(out, f"views_{name}.json"), "w"), indent=1)
        tex = max((t for t in texram_tags if t <= near["tag"]), default=texram_tags[0])
        plan.append((name, ch, near["tag"], tex, float(np.min(dist))))
        print(f"{name}: chapter {ch}, nearest sample {near['tag']} ({np.min(dist):.0f} units away), texture RAM from {tex}")
    with open(os.path.join(out, "plan.csv"), "w", newline="") as f:
        csv.writer(f).writerows([["name", "chapter", "sample", "texram", "distance"]] + plan)


if __name__ == "__main__":
    if len(sys.argv) < 5:
        print(__doc__); sys.exit(1)
    main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4:])
