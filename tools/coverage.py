#!/usr/bin/env python3
"""
coverage.py - which parts of each chapter a recorded run visited, from the arcade camera in MAME work RAM dumps.

For every dump: camera position (work RAM 0x51EA50, see mame_camera.py). Each placed model (placement tables, m2_level.py) gets a
bounding box in world space. A dump belongs to the chapter with the most boxes near the camera (chapters reuse coordinates, so
this is decided per dump and smoothed). A model counts as seen when, at some sample, part of its box was within SEE_DIST
units and inside a SEE_ANGLE cone around the camera direction. Never-seen models are areas of routes the run didn't take.

Usage:
  python coverage.py <regions folder> <dump folder> <out folder>
Writes coverage.csv (chapter, model ID, placement, visited, closest distance) and chapter<n>_map.png (top view, x right,
z up: grey/green = visited, red = never visited, yellow = camera path). Output belongs in private\\.
"""
import sys, os, csv, math
import numpy as np
import m2_level, m2_models
from mame_camera import camera

CHAPTERS = {1: 0xBEF00, 2: 0xBFEA0, 3: 0xC0640, 4: 0xC0A90}
SEE_DIST = 600.0     # units
SEE_ANGLE = 35.0     # degrees from the view direction (the screen is about +-30 degrees wide)


def boxes(mc, md, poly, table):
    out = []
    for k, pl in enumerate(m2_level.placements(mc, table)):
        rec, polys = m2_level.placed_model(md, poly, pl["id"], pl["pos"])
        if not polys:
            continue
        pts = np.array([v for vs, _, _ in polys for v in vs])
        out.append({"id": pl["id"], "k": k, "lo": pts.min(0), "hi": pts.max(0)})
    return out


def seen(p, f, b):
    """Was box b on screen from camera p looking along f? Within SEE_DIST and inside a cone of SEE_ANGLE degrees around the view
    direction (the closest point of the box is tested, so large boxes count as soon as part of them is in front)."""
    q = np.clip(p, b["lo"], b["hi"])          # closest point of the box
    v = q - p
    d = np.linalg.norm(v)
    if d < 1.0:
        return True
    return d <= SEE_DIST and float(v @ f) / d >= math.cos(math.radians(SEE_ANGLE))


def box_distance(p, lo, hi):
    d = np.maximum(np.maximum(lo - p, 0), p - hi)
    return float(np.sqrt((d ** 2).sum()))


def main(regions, dumps, out):
    os.makedirs(out, exist_ok=True)
    mc, md, poly = m2_level.load_regions(regions)
    chapters = {c: boxes(mc, md, poly, t) for c, t in CHAPTERS.items()}
    tags = [l.split()[0] for l in open(os.path.join(dumps, "index.txt"))]
    cams, fwds = [], []
    for t in tags:
        c = camera(dumps, t)
        p = np.array(c["pos"])
        if not np.all(np.isfinite(p)) or np.abs(p).max() > 1e5:
            cams.append((t, None, None)); fwds.append(None); continue
        # chapter vote: how many boxes the camera is near / inside
        votes = {ch: sum(box_distance(p, b["lo"], b["hi"]) < 1.0 for b in bs) for ch, bs in chapters.items()}
        best = max(votes, key=votes.get)
        cams.append((t, p, best if votes[best] > 0 else None))
        fwds.append(np.array(c["forward"]))
    # smooth: a chapter assignment must agree with its neighbours (majority of 5)
    raw = [c for _, _, c in cams]
    smooth = []
    for i in range(len(raw)):
        win = [x for x in raw[max(0, i - 2):i + 3] if x is not None]
        smooth.append(max(set(win), key=win.count) if win else None)
    rows = []
    for ch, bs in chapters.items():
        sel = [(p, f) for (t, p, _), f, c in zip(cams, fwds, smooth) if c == ch and p is not None]
        pts = [p for p, _ in sel]
        for b in bs:
            dmin = min((box_distance(p, b["lo"], b["hi"]) for p in pts), default=math.inf)
            b["visited"] = any(seen(p, f, b) for p, f in sel)
            rows.append([ch, f"{b['id']:#06x}", b["k"], b["visited"], f"{dmin:.0f}" if math.isfinite(dmin) else "",
                         " ".join(f"{x:.0f}" for x in b["lo"]), " ".join(f"{x:.0f}" for x in b["hi"])])
        draw_map(os.path.join(out, f"chapter{ch}_map.png"), ch, bs, pts)
        n_vis = sum(b["visited"] for b in bs)
        print(f"chapter {ch}: {len(pts)} camera samples, {n_vis}/{len(bs)} placed models visited; not visited: "
              + ", ".join(f"{b['id']:#x}" for b in bs if not b["visited"]))
    with open(os.path.join(out, "coverage.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["chapter", "model_id", "placement", "visited", "closest_camera_distance", "box_min_xyz", "box_max_xyz"])
        w.writerows(rows)
    with open(os.path.join(out, "camera_chapters.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["tag", "x", "y", "z", "chapter_raw", "chapter"])
        for (t, p, c), s in zip(cams, smooth):
            w.writerow([t] + ([f"{x:.1f}" for x in p] if p is not None else ["", "", ""]) + [c or "", s or ""])


def draw_map(path, ch, bs, pts, size=1400, margin=40):
    from PIL import Image, ImageDraw
    allp = np.array([b["lo"] for b in bs] + [b["hi"] for b in bs])
    lo, hi = allp.min(0), allp.max(0)
    scale = (size - 2 * margin) / max(hi[0] - lo[0], hi[2] - lo[2])
    def xy(x, z):
        return margin + (x - lo[0]) * scale, size - margin - (z - lo[2]) * scale
    img = Image.new("RGB", (size, size), (25, 25, 30))
    d = ImageDraw.Draw(img)
    for b in sorted(bs, key=lambda b: b["visited"]):
        x0, y0 = xy(b["lo"][0], b["hi"][2]); x1, y1 = xy(b["hi"][0], b["lo"][2])
        col = (80, 170, 90) if b["visited"] else (220, 60, 60)
        d.rectangle([x0, y0, x1, y1], outline=col, width=2)
        if not b["visited"]:
            d.text((x0 + 3, y0 + 3), f"{b['id']:#x}", fill=col)
    if pts:
        line = [xy(p[0], p[2]) for p in pts]
        d.line(line, fill=(240, 220, 60), width=2)
        d.ellipse([line[0][0] - 6, line[0][1] - 6, line[0][0] + 6, line[0][1] + 6], outline=(240, 220, 60), width=2)
    d.text((10, 10), f"Chapter {ch}: green = visited, red = never visited, yellow = camera path (circle = first sample); top view, x right, z up",
           fill=(255, 255, 255))
    img.save(path)


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(__doc__); sys.exit(1)
    main(*sys.argv[1:])
