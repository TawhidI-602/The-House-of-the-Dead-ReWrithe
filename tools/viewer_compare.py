#!/usr/bin/env python3
"""
viewer_compare.py - the viewer's arcade (and PC) render next to MAME's screenshot, from the arcade camera of the same moment.

Usage:
  python viewer_compare.py <dump folder> <out folder> <chapter 1-4> <tag> [<tag> ...]
Runs build\\Release\\hotd_viewer.exe in batch mode (hidden window) with the camera read from each tag's work RAM dump
(mame_camera.py; roll is ignored), and writes <out>\\<tag>_compare.png: MAME | viewer arcade | viewer PC.
Output belongs in private\\ (MAME screenshots).
"""
import sys, os, math, subprocess
import numpy as np
from PIL import Image
from mame_camera import camera

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main(dumps, out, chapter, tags):
    os.makedirs(out, exist_ok=True)
    snaps = sorted(os.listdir(os.path.join(dumps, "snap", "hotd")))
    index = [l.split()[0] for l in open(os.path.join(dumps, "index.txt"))]
    lines = []
    for tag in tags:
        c = camera(dumps, tag)
        p, f = c["pos"], c["forward"]
        yaw = math.degrees(math.atan2(f[0], f[2]))
        pitch = math.degrees(math.asin(max(-1.0, min(1.0, f[1]))))
        for src in ("arcade", "pc"):
            lines.append(f"{tag}_{src} {chapter} {src} {p[0]:.3f} {p[1]:.3f} {p[2]:.3f} {yaw:.3f} {pitch:.3f}")
    shots = os.path.join(out, "shots.txt")
    open(shots, "w").write("\n".join(lines) + "\n")
    exe = os.path.join(ROOT, "build", "Release", "hotd_viewer.exe")
    subprocess.run([exe, "--shots", shots, "--out", out, "--size", "496x384", "--dumps", dumps], check=True, timeout=300)
    for tag in tags:
        mame = Image.open(os.path.join(dumps, "snap", "hotd", snaps[index.index(tag)])).convert("RGB").resize((496, 384))
        row = Image.new("RGB", (496 * 3, 384))
        row.paste(mame, (0, 0))
        row.paste(Image.open(os.path.join(out, f"{tag}_arcade.png")), (496, 0))
        row.paste(Image.open(os.path.join(out, f"{tag}_pc.png")), (992, 0))
        row.save(os.path.join(out, f"{tag}_compare.png"))
        print("wrote", os.path.join(out, f"{tag}_compare.png"))


if __name__ == "__main__":
    if len(sys.argv) < 5:
        print(__doc__); sys.exit(1)
    main(sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4:])
