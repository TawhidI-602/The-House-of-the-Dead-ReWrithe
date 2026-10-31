#!/usr/bin/env python3
"""
compare_world.py - match the objects of a PC world file to arcade models and export both to .OBJ in PC coordinates.

For every PC object it looks for arcade models with nearly the same bounding-box size, aligns each candidate by
translation only (bounding-box centers, then refined on nearest vertices), and keeps the best one. Translation only is
deliberate: if the versions differed by rotation, scale or axis direction, the matches would fail, so a good match
also confirms that the axes and scale are the same.

Usage:
  python compare_world.py <WORLDn.RBH> <polygons.bin> <models.csv> <out_folder>
Writes into out_folder (keep it inside private\\):
  pc_world.obj         the PC world, as stored
  arcade_matched.obj   each matched arcade model, moved by its offset into PC coordinates
  matches.csv          PC id, arcade address, offset (arcade -> PC = subtract), median and 90th percentile distance
"""
import sys, os, csv
import numpy as np
import pc_rbh, m2_models

SIZE_TOLERANCE = 0.05   # of the largest dimension, plus 1 unit
GOOD_MEDIAN = 2.0       # units; a match is "good" when half the PC vertices are this close to the arcade model


def nearest(p, a):
    """Distance and index of the nearest point of a for each point of p (brute force, chunked)."""
    dist, idx = np.empty(len(p)), np.empty(len(p), int)
    for s in range(0, len(p), 256):
        d2 = ((p[s:s + 256, None, :] - a[None, :, :]) ** 2).sum(-1)
        idx[s:s + 256] = d2.argmin(1)
        dist[s:s + 256] = np.sqrt(d2[np.arange(len(d2)), idx[s:s + 256]])
    return dist, idx


def triangles(polys):
    """Polygon list (vertex lists of 3 or 4) -> array of triangles, shape (n, 3, 3)."""
    tris = []
    for vs in polys:
        v = np.asarray(vs, dtype=np.float64)
        tris.append(v[[0, 1, 2]])
        if len(v) == 4:
            tris.append(v[[0, 2, 3]])
    return np.array(tris).reshape(-1, 3, 3)


def surface_distance(p, tris, chunk=128):
    """Distance from each point of p to the nearest triangle (closest point on triangle, Ericson's method), vectorised."""
    out = np.empty(len(p))
    a, b, c = tris[:, 0], tris[:, 1], tris[:, 2]
    ab, ac = b - a, c - a
    for s in range(0, len(p), chunk):
        q = p[s:s + chunk, None, :]
        ap = q - a
        d1, d2 = (ab * ap).sum(-1), (ac * ap).sum(-1)
        bp = q - b
        d3, d4 = (ab * bp).sum(-1), (ac * bp).sum(-1)
        cp = q - c
        d5, d6 = (ab * cp).sum(-1), (ac * cp).sum(-1)
        va, vb, vc = d3 * d6 - d5 * d4, d5 * d2 - d1 * d6, d1 * d4 - d3 * d2
        denom = va + vb + vc
        denom = np.where(np.abs(denom) < 1e-12, 1e-12, denom)
        v, w = vb / denom, vc / denom
        closest = a + ab * v[..., None] + ac * w[..., None]            # inside the face
        # edges and corners (applied in reverse priority so the most specific wins)
        def edge(m, start, d, t):
            return np.where(m[..., None], start + d * t[..., None], closest)
        t_bc = np.clip((d4 - d3) / np.where((d4 - d3) + (d5 - d6) == 0, 1e-12, (d4 - d3) + (d5 - d6)), 0, 1)
        closest = edge((va <= 0) & (d4 - d3 >= 0) & (d5 - d6 >= 0), b, c - b, t_bc)
        t_ac = np.clip(d2 / np.where(d2 - d6 == 0, 1e-12, d2 - d6), 0, 1)
        closest = edge((vb <= 0) & (d2 >= 0) & (d6 <= 0), a, ac, t_ac)
        t_ab = np.clip(d1 / np.where(d1 - d3 == 0, 1e-12, d1 - d3), 0, 1)
        closest = edge((vc <= 0) & (d1 >= 0) & (d3 <= 0), a, ab, t_ab)
        closest = np.where(((d6 >= 0) & (d5 <= d6))[..., None], c, closest)
        closest = np.where(((d3 >= 0) & (d4 <= d3))[..., None], b, closest)
        closest = np.where(((d1 <= 0) & (d2 <= 0))[..., None], a, closest)
        out[s:s + chunk] = np.sqrt(((q - closest) ** 2).sum(-1)).min(1)
    return out


def align(p, a, start):
    """Refine the translation that maps p onto a. Returns (offset, distances)."""
    off = start.copy()
    for _ in range(8):
        dist, idx = nearest(p + off, a)
        close = dist < max(2.0, np.percentile(dist, 50))
        if close.sum() < 3:
            break
        step = np.median(a[idx[close]] - (p[close] + off), axis=0)
        off += step
        if np.abs(step).max() < 1e-3:
            break
    return off, nearest(p + off, a)[0]


def main(world, polygons, models_csv, out):
    os.makedirs(out, exist_ok=True)
    verts, objects = pc_rbh.read_world(world)
    verts = np.array(verts)
    rows = list(csv.DictReader(open(models_csv)))
    ext = np.array([[float(r[k]) for k in ("minx", "miny", "minz", "maxx", "maxy", "maxz")] for r in rows])
    asize, acenter = ext[:, 3:] - ext[:, :3], (ext[:, 3:] + ext[:, :3]) / 2
    words = m2_models.load(polygons)
    floats = words.view("<f4")

    matches, placed = [], []
    for o in objects:
        if not o["polys"]:
            continue
        p = verts[sorted({i for poly in o["polys"] for i in poly[0]})]
        lo, hi = p.min(0), p.max(0)
        size = hi - lo
        cands = np.nonzero(np.all(np.abs(asize - size) <= SIZE_TOLERANCE * size.max() + 1, axis=1))[0]
        best = None
        for i in cands:
            polys = m2_models.parse(words, floats, int(rows[i]["addr"], 16))[0]
            a = np.array([v for poly in polys for v in poly[0]])
            off, dist = align(p, a, acenter[i] - (lo + hi) / 2)
            if best is None or np.median(dist) < best[2]:
                best = (rows[i]["addr"], off, np.median(dist), np.percentile(dist, 90), polys)
        if best is None:
            matches.append([f"{o['id']:#06x}", "", "", "", "", "", "", len(o["polys"]), ""])
            continue
        addr, off, med, p90, polys = best
        matches.append([f"{o['id']:#06x}", addr] + [f"{x:.3f}" for x in off] + [f"{med:.3f}", f"{p90:.3f}", len(o["polys"]), len(polys)])
        if med <= GOOD_MEDIAN:
            placed.append((f"m2_{int(addr, 16):07x}_for_pc_{o['id']:04x}",
                           [([tuple(np.array(v) - off) for v in vs], n, at) for vs, n, at in polys]))

    with open(os.path.join(out, "matches.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["pc_id", "arcade_addr", "dx", "dy", "dz", "median", "p90", "pc_polys", "arcade_polys"])
        w.writerows(matches)
    m2_models.write_obj(os.path.join(out, "arcade_matched.obj"), placed)
    with open(os.path.join(out, "pc_world.obj"), "w") as f:
        f.write(f"# {os.path.basename(world)} exported by compare_world.py\n")
        for x, y, z in verts:
            f.write(f"v {x:.6f} {y:.6f} {z:.6f}\n")
        for o in objects:
            f.write(f"o pc_{o['id']:04x}\n")
            for idx, _, _ in o["polys"]:
                f.write("f " + " ".join(str(i + 1) for i in idx) + "\n")
    good = sum(1 for m in matches if m[5] and float(m[5]) <= GOOD_MEDIAN)
    print(f"{len(matches)} PC objects, {sum(1 for m in matches if m[1])} with a size match, {good} good (median <= {GOOD_MEDIAN})")


if __name__ == "__main__":
    if len(sys.argv) != 5:
        print(__doc__); sys.exit(1)
    main(*sys.argv[1:])
