#!/usr/bin/env python3
"""
m2_models.py - find and export Sega Model 2 models from a polygon ROM region (polygons.bin from m2_roms.py).

Model format (from MAME's model2_v.cpp, geo_parse_np_ns and model2_3d_process_polygon; little-endian 32-bit words):
  P0 (x, y, z float), P1 (x, y, z float)          the first edge
  then links of 10 words:
    attr                                           bits 0-1: 0 = end, 1 = quad, 2 = triangle (3 = quad too)
                                                   bits 8-9: which edge the next polygon starts from;
                                                             link type 0 = hidden polygon (never drawn, only moves the edge)
                                                   bits 10-11: z-sort mode, 12-16: signed texture header step, 17: double-sided,
                                                   18-22: texture parameter index, 27-31: LOD coefficient index
    normal (x, y, z float)
    Pa (x, y, z float), Pb (x, y, z float)         Pb only used by quads (always present)
  terminated by a single attr word with (attr & 3) == 0.
  Polygon n = P1(n-1), P0(n-1), Pa, [Pb]. Link types (attr bits 8-9) for the next edge:
    0, 2: (P0, P1) = (Pa, Pb)       1: P1 = Pa       3: P0 = Pb   (a triangle uses Pb = Pa)

Usage:
  python m2_models.py scan   <polygons.bin> <out.csv>            list every model found (word address, size, bounds)
  python m2_models.py obj    <polygons.bin> <out.obj> <addr>...   export models (word addresses, hex) to one .OBJ
  python m2_models.py range  <polygons.bin> <out.obj> <from> <to> export every model whose start is in [from, to)
Only write into private\\: the output is ROM data.
"""
import sys, struct, math, csv
import numpy as np

MAX_COORD = 1.0e5


def load(path):
    return np.fromfile(path, dtype="<u4")


def parse(words, floats, addr, max_links=20000, keep_hidden=False):
    """Parse one model at word address addr. Returns (polygons, end address) or None if it doesn't look like a model.
    polygons: list of (vertices [(x,y,z)...], normal (x,y,z), attr). Hidden (link type 0) polygons are left out
    unless keep_hidden is set."""
    n = len(words)
    if addr + 7 > n:
        return None
    p = floats[addr:addr + 6]
    if not np.all(np.isfinite(p)) or np.any(np.abs(p) > MAX_COORD):
        return None
    p0 = tuple(float(v) for v in p[0:3])
    p1 = tuple(float(v) for v in p[3:6])
    q = addr + 6
    polys = []
    for _ in range(max_links):
        if q >= n:
            return None
        attr = int(words[q])
        if attr & 3 == 0:   # end of model (the hardware ignores the other bits of this word)
            if q == addr + 6:
                return None
            return polys, q + 1
        if q + 10 > n:
            return None
        v = floats[q + 1:q + 10]
        if not np.all(np.isfinite(v)) or np.any(np.abs(v) > MAX_COORD):
            return None
        nrm = tuple(float(x) for x in v[0:3])
        length = math.sqrt(nrm[0] ** 2 + nrm[1] ** 2 + nrm[2] ** 2)
        if abs(length - 1.0) > 0.02 and length > 1e-3:   # unit normal, or zero (seen on some polygons)
            return None
        pa = tuple(float(x) for x in v[3:6])
        pb = tuple(float(x) for x in v[6:9])
        quad = attr & 1
        link = (attr >> 8) & 3
        if link != 0 or keep_hidden:   # link type 0 polygons are never drawn (MAME check_culling); they only move the edge
            polys.append(([p1, p0, pa, pb] if quad else [p1, p0, pa], nrm, attr))
        if not quad:
            pb = pa
        if link in (0, 2):
            p0, p1 = pa, pb
        elif link == 1:
            p1 = pa
        else:
            p0 = pb
        q += 10
    return None


def scan(words, start=0, stop=None):
    floats = words.view("<f4")
    found, a = [], start
    stop = len(words) if stop is None else stop
    while a < stop:
        r = parse(words, floats, a, keep_hidden=True)
        if r is None:
            a += 1
            continue
        polys, end = r
        shown = [p for p in polys if (p[2] >> 8) & 3]
        pts = np.array([v for poly in (shown or polys) for v in poly[0]])
        polys = shown
        found.append((a, end - a, len(polys), pts.min(axis=0), pts.max(axis=0)))
        a = end
    return found


def write_obj(path, models):
    """models: list of (name, polygons). Faces keep the hardware's winding; flip in Blender if needed."""
    with open(path, "w") as f:
        f.write("# Sega Model 2 models exported by m2_models.py\n")
        base = 1
        for name, polys in models:
            f.write(f"o {name}\n")
            for verts, _, _ in polys:
                for x, y, z in verts:
                    f.write(f"v {x:.6f} {y:.6f} {z:.6f}\n")
            for verts, _, _ in polys:
                f.write("f " + " ".join(str(base + i) for i in range(len(verts))) + "\n")
                base += len(verts)


if __name__ == "__main__":
    if len(sys.argv) < 4 or sys.argv[1] not in ("scan", "obj", "range"):
        print(__doc__); sys.exit(1)
    words = load(sys.argv[2])
    floats = words.view("<f4")
    if sys.argv[1] == "scan":
        found = scan(words)
        with open(sys.argv[3], "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["addr", "words", "polys", "minx", "miny", "minz", "maxx", "maxy", "maxz"])
            for a, size, npoly, lo, hi in found:
                w.writerow([f"{a:#x}", size, npoly] + [f"{x:.3f}" for x in lo] + [f"{x:.3f}" for x in hi])
        print(f"{len(found)} models, {sum(x[2] for x in found)} polygons -> {sys.argv[3]}")
    else:
        if sys.argv[1] == "obj":
            addrs = [int(x, 16) for x in sys.argv[4:]]
        else:
            lo, hi = int(sys.argv[4], 16), int(sys.argv[5], 16)
            addrs = [a for a, *_ in scan(words, lo, hi)]
        models = []
        for a in addrs:
            r = parse(words, floats, a)
            if r is None:
                raise SystemExit(f"no model at {a:#x}")
            models.append((f"m2_{a:07x}", r[0]))
        write_obj(sys.argv[3], models)
        print(f"wrote {len(models)} models to {sys.argv[3]}")
