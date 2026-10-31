#!/usr/bin/env python3
"""
vmc_tool.py - export / import texture pages in The House of the Dead (PC, 1998) .VMC files.

Format (worked out from BIGGER.VMC on the user's own disc):
  "PIFF" <u32 size> "VVTP"            file header (little-endian sizes)
  then repeated chunks:
  "BODY" <u32 size>                    size = 64-byte page header + pixel data
    64-byte page header                kept byte-for-byte (page number, 256x256, 16bpp, masks...)
    width*height u16 pixels            bit 15 = opaque flag, bits 14-10 R, 9-5 G, 4-0 B
                                        0x0000 = transparent
Compressed blood-color files (B_RED.VMC etc.) use "PACK" chunks and are NOT supported yet.

Usage:
  python vmc_tool.py export BIGGER.VMC out_folder
  python vmc_tool.py import out_folder NEW_BIGGER.VMC

Edit the PNGs in out_folder with any editor. Keep them the same size.
Fully transparent pixels (alpha < 128) become transparent in-game; everything else is opaque.
Pages whose PNG you don't touch are copied back byte-for-byte from the original.

Requires: pip install pillow numpy
"""
import sys, os, json, struct, hashlib, base64
import numpy as np
from PIL import Image


def read_vmc(path):
    d = open(path, "rb").read()
    magic, total, kind = struct.unpack_from("<4sI4s", d, 0)
    if magic != b"PIFF" or kind != b"VVTP":
        raise SystemExit(f"{path}: not a VVTP PIFF file (got {magic!r}/{kind!r})")
    if total + 8 != len(d):
        print(f"warning: header size {total + 8} != file size {len(d)}")
    pages, p = [], 12
    while p < len(d):
        tag, size = struct.unpack_from("<4sI", d, p)
        if tag == b"PACK":
            raise SystemExit("This file uses compressed PACK chunks (blood-color variant). "
                             "Only uncompressed BODY files like BIGGER.VMC are supported for now.")
        if tag != b"BODY":
            raise SystemExit(f"unexpected chunk {tag!r} at offset {p:#x}")
        body = d[p + 8:p + 8 + size]
        hdr = body[:64]
        hdr_size = struct.unpack_from("<I", hdr, 0)[0]
        w, h = struct.unpack_from("<HH", hdr, 8)
        bpp = struct.unpack_from("<H", hdr, 14)[0]
        if hdr_size != 64 or bpp != 16 or size != 64 + w * h * 2:
            raise SystemExit(f"page at {p:#x}: unexpected header (hdr={hdr_size}, {w}x{h}, {bpp}bpp)")
        pages.append({"hdr": hdr, "w": w, "h": h, "raw": body[64:]})
        p += 8 + size
    return pages


def raw_to_rgba(raw, w, h):
    px = np.frombuffer(raw, dtype="<u2").reshape(h, w).astype(np.uint16)
    def expand(v):  # 5-bit -> 8-bit, exactly reversible with >> 3
        return ((v << 3) | (v >> 2)).astype(np.uint8)
    r = expand((px >> 10) & 31); g = expand((px >> 5) & 31); b = expand(px & 31)
    a = np.where(px & 0x8000, 255, 0).astype(np.uint8)
    odd = int(np.count_nonzero((px & 0x8000 == 0) & (px != 0)))
    return np.dstack([r, g, b, a]), odd


def rgba_to_raw(img):
    a = np.asarray(img.convert("RGBA")).astype(np.uint16)
    r, g, b, al = (a[..., i] for i in range(4))
    px = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3) | 0x8000
    px = np.where(al >= 128, px, 0).astype("<u2")
    return px.tobytes()


def sha1(path):
    return hashlib.sha1(open(path, "rb").read()).hexdigest()


def export(vmc, outdir):
    pages = read_vmc(vmc)
    os.makedirs(outdir, exist_ok=True)
    manifest = {"source": os.path.basename(vmc), "pages": []}
    odd_total = 0
    for i, pg in enumerate(pages):
        rgba, odd = raw_to_rgba(pg["raw"], pg["w"], pg["h"])
        odd_total += odd
        name = f"page_{i:03d}.png"
        fp = os.path.join(outdir, name)
        Image.fromarray(rgba, "RGBA").save(fp)
        manifest["pages"].append({
            "file": name,
            "header": base64.b64encode(pg["hdr"]).decode(),
            "png_sha1": sha1(fp),
        })
    # Original pixel data is stored alongside so untouched pages round-trip exactly.
    with open(os.path.join(outdir, "original_pixels.bin"), "wb") as f:
        for pg in pages:
            f.write(pg["raw"])
    json.dump(manifest, open(os.path.join(outdir, "manifest.json"), "w"), indent=1)
    print(f"exported {len(pages)} pages to {outdir}")
    if odd_total:
        print(f"note: {odd_total} pixels had the opaque flag clear but nonzero color; "
              "they are preserved only on pages you don't edit.")


def do_import(indir, out_vmc):
    manifest = json.load(open(os.path.join(indir, "manifest.json")))
    orig = open(os.path.join(indir, "original_pixels.bin"), "rb").read()
    chunks, off, changed = [], 0, 0
    for entry in manifest["pages"]:
        hdr = base64.b64decode(entry["header"])
        w, h = struct.unpack_from("<HH", hdr, 8)
        n = w * h * 2
        raw = orig[off:off + n]; off += n
        fp = os.path.join(indir, entry["file"])
        if sha1(fp) != entry["png_sha1"]:
            img = Image.open(fp)
            if img.size != (w, h):
                raise SystemExit(f"{entry['file']} is {img.size[0]}x{img.size[1]}, must stay {w}x{h}")
            raw = rgba_to_raw(img)
            changed += 1
        chunks.append(b"BODY" + struct.pack("<I", 64 + n) + hdr + raw)
    body = b"VVTP" + b"".join(chunks)
    with open(out_vmc, "wb") as f:
        f.write(b"PIFF" + struct.pack("<I", len(body)) + body)
    print(f"wrote {out_vmc} ({len(chunks)} pages, {changed} changed)")


if __name__ == "__main__":
    if len(sys.argv) != 4 or sys.argv[1] not in ("export", "import"):
        print(__doc__); sys.exit(1)
    (export if sys.argv[1] == "export" else do_import)(sys.argv[2], sys.argv[3])
