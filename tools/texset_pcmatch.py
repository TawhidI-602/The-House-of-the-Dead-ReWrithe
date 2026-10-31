#!/usr/bin/env python3
"""
texset_pcmatch.py - which arcade texture set (area block, formats.md §17) each placed model of a chapter is meant to be drawn with,
decided by comparing with the PC version of the same model. No MAME data needed: ROM + PC install only.

For every textured arcade polygon, a few surface points are sampled. At each point: the arcade brightness for each candidate area
block (texel shade -> luma table, times the brightness of the polygon's base color from the ROM palette records), and the PC
brightness at the nearest point of the PC model with the same object ID (its VMC texture at the interpolated UV). The set whose
arcade brightness correlates best with the PC brightness wins. Wrong sets hold unrelated textures (correlation near 0).

tools/texset_assign.py --pcmatch runs this for Chapters 1-4 and appends the result to its output (later lines override).

Usage:
  python texset_pcmatch.py <regions folder> <PC install> <chapter 1-4> <area block> [<area block> ...] [--out <file>]
Prints per model: correlation per set and the winner. With --out, writes "model/valid" lines like texset_assign.py
(stage index = chapter - 1) to <file>; keep it in private\\.
"""
import sys, os, struct
import numpy as np
import m2_level, m2_textures, pc_rbh

TABLES = {1: 0xBEF00, 2: 0xBFEA0, 3: 0xC0640, 4: 0xC0A90}
ALIAS = {0x1D4B: 0xA27}
LUMA = 0xE9AAC0
MIN_SCORE, MARGIN, CONFIDENT = 0.10, 0.05, 0.10
# only results with a winner of at least this correlation are written (0x111C, the Chapter 1 arena, wins block 3 with +0.17 but
# is really drawn with block 4: its PC model covers other rooms too)
CONFIDENT_SCORE = 0.18
SAMPLES = np.array([[1 / 3, 1 / 3, 1 / 3], [0.6, 0.2, 0.2], [0.2, 0.6, 0.2], [0.2, 0.2, 0.6], [0.45, 0.45, 0.1],
                    [0.1, 0.45, 0.45], [0.45, 0.1, 0.45]])


def texture_ram(md8, mc8, n):
    """formats.md §17 rebuild; returns the two sheets (2048 x 1024 shades)."""
    ram = bytearray(0x400000)
    ram[0:0x100000] = md8[0:0x100000]
    ram[0xC0000:0x100000] = md8[n * 0x100000 + 0xC0000:n * 0x100000 + 0x100000]
    ram[0x200000:0x300000] = md8[n * 0x100000:(n + 1) * 0x100000]
    ram[0x2C0000:0x2E0000] = md8[0xC0000:0xE0000]
    for k in range(8):
        src = struct.unpack_from("<I", mc8, 0xC1610 + ((n - 1) * 8 + k) * 4)[0] - 0x02000000
        for r in range(64):
            d = ((k // 2) * 64 + r) * 0x400 + (k % 2) * 0x80
            ram[d:d + 0x80] = md8[src + r * 0x80:src + r * 0x80 + 0x80]
    w = np.frombuffer(bytes(ram), dtype="<u2")
    return [m2_textures.sheet(w[s * 0x100000:], 0) for s in (0, 1)]


def palette(mc8, n):
    pal = np.zeros(1024, np.int32)
    for rec, at in ((0, 0), (n, 500)):
        p = struct.unpack_from("<I", mc8, 0xA9960 + 4 * rec)[0]
        cnt = struct.unpack_from("<H", mc8, p)[0]
        pal[at:at + cnt] = np.frombuffer(mc8[p + 2:p + 2 + 2 * cnt], dtype="<u2")[:1024 - at]
    return pal


def read_vmc_pages(path, pages):
    d = open(path, "rb").read()
    out, p = {}, 12
    while p < len(d):
        tag, size = struct.unpack_from("<4sI", d, p)
        if tag == b"BODY":
            num = struct.unpack_from("<I", d, p + 12)[0]
            if num in pages:
                px = np.frombuffer(d, "<u2", 256 * 256, p + 8 + 64).reshape(256, 256).astype(np.int32)
                lum = ((px >> 10) & 31) * 0.3 + ((px >> 5) & 31) * 0.59 + (px & 31) * 0.11
                opaque = ((px & 0x8000) != 0) & ((px & 0x7FFF) != 0x7C1F)
                out[num] = np.where(opaque, lum, np.nan)
        p += 8 + size
    return out


def nearest_uv(pc_tris, pc_uvs, pts):
    """For each point: UV (and page) at the nearest point of the nearest PC triangle (nearest by centroid, then projected)."""
    cen = pc_tris.mean(axis=1)
    res = np.full((len(pts), 2), np.nan)
    which = np.zeros(len(pts), np.int64)
    for a in range(0, len(pts), 512):
        d = ((pts[a:a + 512, None, :] - cen[None]) ** 2).sum(-1)
        which[a:a + 512] = d.argmin(1)
    t = pc_tris[which]
    v0, v1, v2 = t[:, 0], t[:, 1], t[:, 2]
    e1, e2, q = v1 - v0, v2 - v0, pts - v0
    d00, d01, d11 = (e1 * e1).sum(1), (e1 * e2).sum(1), (e2 * e2).sum(1)
    d20, d21 = (q * e1).sum(1), (q * e2).sum(1)
    den = d00 * d11 - d01 * d01
    den[den == 0] = 1
    b1 = np.clip((d11 * d20 - d01 * d21) / den, 0, 1)
    b2 = np.clip((d00 * d21 - d01 * d20) / den, 0, 1 - b1)
    b0 = 1 - b1 - b2
    uv = pc_uvs[which]
    res = uv[:, 0] * b0[:, None] + uv[:, 1] * b1[:, None] + uv[:, 2] * b2[:, None]
    dist = np.linalg.norm(v0 * b0[:, None] + v1 * b1[:, None] + v2 * b2[:, None] - pts, axis=1)
    return res, which, dist


def main():
    args = sys.argv[1:]
    out = None
    if "--out" in args:
        i = args.index("--out"); out = args[i + 1]; del args[i:i + 2]
    regions, pcdir, chapter, blocks = args[0], args[1], int(args[2]), [int(x) for x in args[3:]]
    mc, md, poly = m2_level.load_regions(regions)
    mc8, md8 = mc.tobytes(), md.tobytes()
    tex = np.fromfile(os.path.join(regions, "textures.bin"), dtype="<u2")
    luma = np.frombuffer(md8[LUMA:LUMA + 0x4000], np.uint8)
    sheets = {n: texture_ram(md8, mc8, n) for n in blocks}
    pals = {n: palette(mc8, n) for n in blocks}

    verts, objects = pc_rbh.read_world(os.path.join(pcdir, f"ART/STAGE{chapter}/WORLD/WORLD{chapter}.RBH"))
    verts = np.array(verts)
    pages = {p[1] for o in objects for p in o["polys"] if p[2]}
    vmc = read_vmc_pages(os.path.join(pcdir, "ART/STAGE1/CREATURE/BIGGER.VMC"), pages)
    pc = {}
    for o in objects:
        tris, uvs, pg = [], [], []
        for idx, page, uv in o["polys"]:
            if not uv:
                continue
            uvp = np.array(uv).reshape(-1, 2)
            for tri in ((0, 1, 2), (0, 2, 3)) if len(idx) == 4 else ((0, 1, 2),):
                tris.append(verts[[idx[k] for k in tri]]); uvs.append(uvp[list(tri)]); pg.append(page)
        if tris:
            cid = ALIAS.get(o["id"], o["id"])
            e = pc.setdefault(cid, [[], [], []])
            e[0] += tris; e[1] += uvs; e[2] += pg

    lines = []
    for pl in m2_level.placements(mc, TABLES[chapter]):
        oid = pl["id"]
        rec = m2_level.model_record(md, oid)
        if rec["addr"] is None or rec["tpa"] == rec["tha"] or oid not in pc:
            continue
        pts, puv, hdrs = [], [], []
        for vs, uvs, h, attr in m2_textures.textured_polys(poly, tex, rec):
            if not (h[0] >> 14) & 1:
                continue
            V = np.array(vs[:3]) + np.array(pl["pos"]); U = np.array(uvs[:3])
            tris = [(V, U)] if len(vs) == 3 else [(V, U), (np.array([vs[0], vs[2], vs[3]]) + np.array(pl["pos"]),
                                                            np.array([uvs[0], uvs[2], uvs[3]]))]
            for TV, TU in tris:
                pts.append(SAMPLES @ TV); puv.append(SAMPLES @ TU); hdrs += [h] * len(SAMPLES)
        if not pts:
            continue
        pts = np.concatenate(pts); puv = np.concatenate(puv)
        T = pc[oid]
        uv, which, dist = nearest_uv(np.array(T[0]), np.array(T[1]), pts)
        pcl = np.full(len(pts), np.nan)
        for i in range(len(pts)):
            page = T[2][which[i]]
            if page in vmc and dist[i] < 2.0:
                pcl[i] = vmc[page][int(uv[i, 1]) % 256, int(uv[i, 0]) % 256]
        ok = ~np.isnan(pcl)
        if ok.sum() < 20:
            continue
        scores = {}
        for n in blocks:
            al = np.zeros(len(pts))
            for i, h in enumerate(hdrs):
                s = (h[2] >> 12) & 1
                x0, y0 = 32 * (h[2] & 0x3F), 32 * ((h[2] >> 6) & 0x1F)
                w, hh = 32 << (h[0] & 7), 32 << ((h[0] >> 3) & 7)
                u, v = int(np.floor(puv[i, 0])), int(np.floor(puv[i, 1]))
                if (h[0] >> 8) & 1:
                    u %= 2 * w; u = u if u < w else 2 * w - 1 - u
                if (h[0] >> 9) & 1:
                    v %= 2 * hh; v = v if v < hh else 2 * hh - 1 - v
                sx, sy = x0 + u % w, y0 + v % hh
                shade = int(sheets[n][s][sy % 1024, sx % 2048])
                base = int(pals[n][(h[3] >> 6) & 0x3FF])
                bright = (base & 31) * 0.3 + ((base >> 5) & 31) * 0.59 + ((base >> 10) & 31) * 0.11
                al[i] = luma[(h[1] & 0xFF) * 128 + shade * 8] * bright
            a, b = al[ok], pcl[ok]
            scores[n] = float(np.corrcoef(a, b)[0, 1]) if a.std() > 0 and b.std() > 0 else 0.0
        best = max(scores, key=scores.get)
        # no clear winner: every set stays valid (the viewer then follows the camera's area)
        valid = blocks if scores[best] < MIN_SCORE else [n for n in blocks if scores[n] >= scores[best] - MARGIN]
        print(f"{oid:#06x} samples {int(ok.sum()):5}  " + "  ".join(f"blk{n}:{scores[n]:+.2f}" for n in blocks) + f"  -> {best}")
        second = max([scores[n] for n in blocks if n != best], default=-1.0)
        if scores[best] >= CONFIDENT_SCORE and scores[best] - second >= CONFIDENT:
            lines.append((oid, blocks.index(best), [blocks.index(n) for n in valid]))   # only confident results are written
    if out:
        with open(out, "w") as f:
            for oid, b, valid in lines:
                f.write(f"model {chapter - 1} {oid:x} {b}\nvalid {chapter - 1} {oid:x} {' '.join(map(str, valid))}\n")
        print("wrote", out)


if __name__ == "__main__":
    if len(sys.argv) < 5:
        print(__doc__); sys.exit(1)
    main()
