#!/usr/bin/env python3
"""
m2_textures.py - Model 2C texture sheets and per-polygon texture data for House of the Dead (see docs/formats.md §13).

Texture sheets: main_data blocks 0-12 (1 MB each, CPU 0x02000000 + n * 0x100000; the program lists them at maincpu 0xA99E0).
  The game copies a block into texture RAM (two RAMs = two sheets). A block is 1024 x 2048 texels in RAM layout, 4 bits per texel,
  one 16-bit word = 2 x 2 texels (bits 12-15: x even/y even, 8-11: x odd/y even, 4-7: x even/y odd, 0-3: x odd/y odd).
  The rasterizer sees a 2048 x 1024 sheet: x < 1024 = RAM rows 0-1023, x >= 1024 = RAM rows 1024-2047.
Per polygon (in the order of the model's links, hidden ones included):
  texture point list (texture ROM, u16 units, from the model table): v, u per vertex, 13.3 fixed point (texels)
  texture header (4 x u16, texture ROM): h0 bits 0-2 width (32 << n), 3-5 height, 6/7 wrap x/y, 8/9 mirror x/y, 13 translucent?,
  14 textured, 15 checker; h1 bits 0-7 luma table base; h2 bits 0-5 x / 32, 6-10 y / 32, 12 sheet select, 13-15 microtexture;
  h3 bits 6-15 color (palette) index. After each polygon the header address moves by the signed attr bits 12-16, times 4.
Colors (MAME draw_scanline_tex), using tables dumped from MAME (tools\\mame_dump.lua):
  luma = lumaRAM[h1 * 128 + (shade * 16) / 2] (0-63; lighting not applied: full brightness),
  base = paletteRAM[0x1000 + color index] (5-5-5, red in the low bits),
  channel = colorxlat[channel][base_channel * 256 + luma] & 0xFF (red, green, blue tables 0x2000 words apart).
Without color tables the output is greyscale (shade * 17).
Translucent textures (h0 bit 13): shade 15 is transparent. Mirroring (h0 bits 8/9) is baked into the image (texture + flipped copy).
Checker (h0 bit 15, every other pixel) is not applied.

Usage:
  python m2_textures.py sheet <regions folder> <block> <out.png>
  python m2_textures.py level <regions folder> <placement table hex> <sheet0> <sheet1> <out folder> [<dump prefix>]
      <sheet0>/<sheet1>: a main_data block number, or "<..._texram.bin>:0" / ":1" for a MAME texture RAM dump (exact: the game
      assembles each sheet from parts of block 0 and the stage block, plus generated parts; see docs/formats.md §13).
      writes level.obj + level.mtl + tex\\*.png (one image per texture/color combination).
      <dump prefix> = path + tag of a MAME dump, e.g. ...\\ch1_run1\\t00440 (uses _palette.bin, _colorxlat.bin, _luma.bin).
Write output only into private\\.
"""
import sys, os
import numpy as np
import m2_level, m2_models


def load_colors(prefix):
    pal = np.fromfile(prefix + "_palette.bin", dtype="<u2")
    xlat = np.fromfile(prefix + "_colorxlat.bin", dtype="<u2")
    luma = np.fromfile(prefix + "_luma.bin", dtype=np.uint8)[0::2]
    return pal, xlat, luma


def shade_colors(colors, h1, h3):
    """RGB for the 16 shades of a texture with luma table h1 & 0xFF and color index h3 >> 6 (array 16 x 3, uint8)."""
    pal, xlat, luma = colors
    base = int(pal[0x1000 + ((h3 >> 6) & 0x3FF)]) & 0x7FFF
    lum = np.minimum(luma[(h1 & 0xFF) * 128 + (np.arange(16) * 16 >> 1)], 63).astype(np.int32)
    out = np.zeros((16, 3), np.uint8)
    for c in range(3):
        out[:, c] = xlat[c * 0x2000 + ((base >> (5 * c)) & 0x1F) * 256 + lum] & 0xFF
    return out

SHEET_W, SHEET_H = 2048, 1024


def sheet(main_data_u16, block):
    w = main_data_u16[block * 0x80000:(block + 1) * 0x80000].reshape(1024, 512).astype(np.int32)
    ram = np.zeros((2048, 1024), np.uint8)
    ram[0::2, 0::2] = (w >> 12) & 15
    ram[0::2, 1::2] = (w >> 8) & 15
    ram[1::2, 0::2] = (w >> 4) & 15
    ram[1::2, 1::2] = w & 15
    return np.concatenate([ram[:1024], ram[1024:]], axis=1)


def sheet_from_source(main_data_u16, source):
    """source = block number (int or digit string), or '<texram dump>:<0|1>' for a texture RAM dump from mame_dump.lua
    (RAM 0 = bytes 0-0xFFFFF, RAM 1 = bytes 0x200000-0x2FFFFF; same layout as a block)."""
    if isinstance(source, int) or str(source).isdigit():
        return sheet(main_data_u16, int(source))
    path, ram = str(source).rsplit(":", 1)
    words = np.fromfile(path, dtype="<u2")[int(ram) * 0x100000:int(ram) * 0x100000 + 0x80000]
    return sheet(words, 0)


def textured_polys(poly, tex, rec):
    """Visible polygons of a model with their texture data: (verts, [(u, v) texels], header words, attr)."""
    polys, _ = m2_models.parse(poly, poly.view("<f4"), rec["addr"], keep_hidden=True)
    out, tp, th = [], rec["tpa"], rec["tha"]
    untextured = rec["tpa"] == rec["tha"]
    for verts, normal, attr in polys:
        n = len(verts)
        uv = tex[tp:tp + 2 * n].astype(np.int32) if not untextured else np.zeros(2 * n, np.int32)
        hdr = [int(x) for x in tex[th:th + 4]]
        if (attr >> 8) & 3:   # visible
            out.append((verts, [(uv[2 * i + 1] / 8.0, uv[2 * i] / 8.0) for i in range(n)], hdr, attr))
        if not untextured:
            tp += 2 * n
        step = (attr >> 12) & 0x1F
        th += (step - 32 if step & 0x10 else step) * 4
    return out


def write_level(regions, table, block0, block1, out, dump_prefix=None):
    os.makedirs(out, exist_ok=True)
    mc, md, poly = m2_level.load_regions(regions)
    tex = np.fromfile(os.path.join(regions, "textures.bin"), dtype="<u2")
    md16 = md.view("<u2")
    from PIL import Image
    sheets = [sheet_from_source(md16, b) for b in (block0, block1)]
    for i in range(2):
        Image.fromarray(sheets[i] * 17).save(os.path.join(out, f"sheet{i}.png"))
    os.makedirs(os.path.join(out, "tex"), exist_ok=True)
    colors = load_colors(dump_prefix) if dump_prefix else None
    materials = {}   # texture + color + mode -> (name, image width, image height); one image each so wrapping UVs repeat

    def material(h):
        s, x, y = (h[2] >> 12) & 1, 32 * (h[2] & 0x3F), 32 * ((h[2] >> 6) & 0x1F)
        w, hh = 32 << (h[0] & 7), 32 << ((h[0] >> 3) & 7)
        mirror_x, mirror_y, translucent = (h[0] >> 8) & 1, (h[0] >> 9) & 1, (h[0] >> 13) & 1
        key = (s, x, y, w, hh, mirror_x, mirror_y, translucent) + ((h[1] & 0xFF, (h[3] >> 6) & 0x3FF) if colors else ())
        if key not in materials:
            name = "t" + "_".join(str(k) for k in key)
            shade = sheets[s][y:y + hh, x:x + w] if x + w <= SHEET_W and y + hh <= SHEET_H else np.zeros((hh, w), np.uint8)
            if mirror_x:
                shade = np.concatenate([shade, shade[:, ::-1]], axis=1)
            if mirror_y:
                shade = np.concatenate([shade, shade[::-1, :]], axis=0)
            rgb = shade_colors(colors, h[1], h[3])[shade] if colors else np.repeat((shade * 17)[..., None], 3, axis=2)
            alpha = np.where(shade == 15, 0, 255).astype(np.uint8) if translucent else np.full(shade.shape, 255, np.uint8)
            Image.fromarray(np.dstack([rgb, alpha]), "RGBA").save(os.path.join(out, "tex", name + ".png"))
            materials[key] = (name, shade.shape[1], shade.shape[0])
        return materials[key]

    vn = vt = 0
    with open(os.path.join(out, "level.obj"), "w") as f:
        f.write("mtllib level.mtl\n")
        for k, pl in enumerate(m2_level.placements(mc, table)):
            rec = m2_level.model_record(md, pl["id"])
            if rec["addr"] is None:
                continue
            f.write(f"o id_{pl['id']:04x}_{k}\n")
            off = np.array(pl["pos"])
            for verts, uvs, h, attr in textured_polys(poly, tex, rec):
                textured = (h[0] >> 14) & 1 and rec["tpa"] != rec["tha"]
                for v in verts:
                    x, y, z = np.array(v) + off
                    f.write(f"v {x:.4f} {y:.4f} {z:.4f}\n")
                if textured:
                    name, w, hh = material(h)
                    for u, v in uvs:
                        f.write(f"vt {u / w:.6f} {1 - v / hh:.6f}\n")
                    f.write(f"usemtl {name}\n")
                    f.write("f " + " ".join(f"{vn + i + 1}/{vt + i + 1}" for i in range(len(verts))) + "\n")
                    vt += len(verts)
                else:
                    f.write("usemtl untextured\n")
                    f.write("f " + " ".join(str(vn + i + 1) for i in range(len(verts))) + "\n")
                vn += len(verts)
    with open(os.path.join(out, "level.mtl"), "w") as f:
        for name, _, _ in materials.values():
            f.write(f"newmtl {name}\nKd 1 1 1\nmap_Kd tex/{name}.png\nmap_d tex/{name}.png\n\n")
        f.write("newmtl untextured\nKd 0.5 0.5 0.5\n")
    print(f"wrote {out}\\level.obj ({vn} vertices, {len(materials)} textures)")


if __name__ == "__main__":
    if len(sys.argv) < 2 or sys.argv[1] not in ("sheet", "level"):
        print(__doc__); sys.exit(1)
    if sys.argv[1] == "sheet":
        from PIL import Image
        md16 = np.fromfile(os.path.join(sys.argv[2], "main_data.bin"), dtype="<u2")
        Image.fromarray(sheet(md16, int(sys.argv[3])) * 17).save(sys.argv[4])
    else:
        write_level(sys.argv[2], int(sys.argv[3], 16), sys.argv[4], sys.argv[5], sys.argv[6],
                    sys.argv[7] if len(sys.argv) > 7 else None)
