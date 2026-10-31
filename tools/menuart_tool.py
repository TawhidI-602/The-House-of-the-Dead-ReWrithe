#!/usr/bin/env python3
"""
menuart_tool.py - inventory of SEGA's own 2D menu art and side-by-side images for the ported PC front end (docs/teaser_status/
menuart.md). Reads the player's own files only (the arcade ROM exports, the PC install folder or its .zip); writes PNGs
for review. Nothing it writes belongs in the repo: point the output at private\\menuart_scratch\\.

PC sprites: SPRITE\\SPR.RBH + the 256 x 256 pages of ART\\STAGE1\\CREATURE\\BIGGER.VMC (docs/formats.md §31), coloured
through one of the PC's TBL / PAL pairs (STAGE1 by default, as src/assets/sprites_pc.cpp; MENU / TITLE / HOTD / GREY /
BROWN on request; NONE = the direct 5-bit colours). Arcade sprites: tools/sprites_tool.py's decoder (table 0xA6590).

Usage:
  py tools/menuart_tool.py sheet-pc <out_prefix> [--pal STAGE1] [--per 120]   labelled contact sheets of every PC sprite
  py tools/menuart_tool.py sheet-arcade <out_prefix> [--per 120]             the same for the arcade's 0x276 sprites
  py tools/menuart_tool.py pick-pc <out.png> <ids...> [--pal X]                selected PC sprites, full size, labelled
  py tools/menuart_tool.py pick-arcade <out.png> <ids...>
  py tools/menuart_tool.py side <out.png> <ours.png> <real.png>                ours next to a PC harness capture
Options: --pc <PC folder or .zip> (default private\\portable\\HOTD ReWrithe\\data\\pc\\hotd pc.zip, else
private\\pcharness\\game), --regions <arcade regions folder> (sprites_tool's default).
Requires: pillow, numpy.
"""
import os, struct, sys, zipfile
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

args = sys.argv[1:]
def opt(name, default):
    if name in args:
        i = args.index(name); v = args[i + 1]; del args[i:i + 2]; return v
    return default

# the repo root holding private\ (--root; else the first parent of tools\ that has one: worktrees sit inside the repo)
ROOT = opt('--root', None)
if ROOT is None:
    ROOT = os.path.abspath(os.path.join(HERE, '..'))
    p = ROOT
    while not os.path.isdir(os.path.join(p, 'private')) and os.path.dirname(p) != p:
        p = os.path.dirname(p)
    if os.path.isdir(os.path.join(p, 'private')): ROOT = p

_pcdef = os.path.join(ROOT, 'private', 'portable', 'HOTD ReWrithe', 'data', 'pc', 'hotd pc.zip')
if not os.path.exists(_pcdef):
    _pcdef = os.path.join(ROOT, 'private', 'pcharness', 'game')
PC = opt('--pc', _pcdef)
PAL = opt('--pal', 'STAGE1').upper()
PER = int(opt('--per', '120'))
_regions = opt('--regions', os.path.join(ROOT, 'private', 'arcade_exports', 'regions'))
if _regions:
    sys.argv += ['--regions', _regions]
import sprites_tool as st   # noqa: E402  (reads --regions from sys.argv at import)
if _regions:
    st.REGIONS = _regions

# --- the PC files: a folder or a zip (names matched whatever their case, the install may sit one folder down) ---
_zip = None
def pc_read(rel):
    global _zip
    rel = rel.replace('\\', '/')
    if PC.lower().endswith('.zip'):
        if _zip is None:
            _zip = zipfile.ZipFile(PC)
        want = rel.lower()
        for n in _zip.namelist():
            nl = n.lower()
            if nl == want or nl.endswith('/' + want):
                return _zip.read(n)
        return None
    p = os.path.join(PC, *rel.split('/'))
    if not os.path.exists(p):
        # case-insensitive walk
        cur = PC
        for part in rel.split('/'):
            hit = [e for e in os.listdir(cur) if e.lower() == part.lower()] if os.path.isdir(cur) else []
            if not hit: return None
            cur = os.path.join(cur, hit[0])
        p = cur
    return open(p, 'rb').read()

class PcArt:
    def __init__(self, pal='STAGE1'):
        self.rbh = pc_read('SPRITE/SPR.RBH')
        n = struct.unpack_from('<I', self.rbh, 0x28)[0] // 4
        self.offs = struct.unpack_from('<%dI' % n, self.rbh, 0x28)
        self.vmc = pc_read('ART/STAGE1/CREATURE/BIGGER.VMC')
        self.pageoff = {}
        f, o = self.vmc, 12
        while o + 8 <= len(f):
            tag, size = f[o:o + 4], struct.unpack_from('<I', f, o + 4)[0]
            if tag == b'BODY':
                num, w, h = struct.unpack_from('<IHH', f, o + 12)
                if w == 256 and h == 256:
                    self.pageoff[num] = o + 8 + 64
            o += 8 + size
        self.lut = None
        if pal != 'NONE':
            tbl, pl = pc_read(pal + '.TBL'), pc_read(pal + '.PAL')
            if tbl and pl and len(tbl) == 65536 and len(pl) >= 1024:
                p = np.frombuffer(pl, np.uint8, 1024).reshape(256, 4)[:, :3]
                self.lut = p[np.frombuffer(tbl, np.uint8)]   # 65536 x 3
        self.cache = {}

    def count(self): return len(self.offs)

    def page(self, n):
        if n in self.cache: return self.cache[n]
        c = np.frombuffer(self.vmc, np.uint16, 256 * 256, self.pageoff[n]).reshape(256, 256)
        img = np.zeros((256, 256, 4), np.uint8)
        if self.lut is not None:
            img[..., :3] = self.lut[c]
        else:
            x5 = lambda v: ((v << 3) | (v >> 2)).astype(np.uint8)
            img[..., 0] = x5(c >> 10 & 31); img[..., 1] = x5(c >> 5 & 31); img[..., 2] = x5(c & 31)
        img[..., 3] = np.where(((c & 0x8000) != 0) & ((c & 0x7FFF) != 0x7C1F), 255, 0)
        self.cache[n] = img
        return img

    def info(self, i):
        o = 0x28 + self.offs[i]
        n = struct.unpack_from('<I', self.rbh, o)[0]
        hdr = struct.unpack_from('<6h', self.rbh, o + 4)
        pages = sorted({struct.unpack_from('<h', self.rbh, o + 16 + 16 * k + 12)[0] for k in range(min(n, 64))})
        return n, hdr, pages

    def sprite(self, i):
        o = 0x28 + self.offs[i]
        n = struct.unpack_from('<I', self.rbh, o)[0]
        hdr = struct.unpack_from('<6h', self.rbh, o + 4)
        W, H = max(hdr[4], 1), max(hdr[5], 1)
        if W > 2048 or H > 2048: return np.zeros((1, 1, 4), np.uint8)
        img = np.zeros((H, W, 4), np.uint8)
        for k in range(min(n, 64)):
            x0, y0, x1, y1, u, v, _, pg, fl = struct.unpack_from('<4hBBHhh', self.rbh, o + 16 + 16 * k)
            if pg not in self.pageoff or x1 <= x0 or y1 <= y0 or x0 < 0 or y0 < 0: continue
            src = self.page(pg)[v:v + y1 - y0, u:u + x1 - x0]
            dst = img[y0:y0 + src.shape[0], x0:x0 + src.shape[1]]
            src = src[:dst.shape[0], :dst.shape[1]]
            m = src[..., 3] > 0
            dst[m] = src[m]
        return img

def arcade_sprite(i):
    return st.arcade_sprite(i)

# --- contact sheets ---
def _font(sz):
    for f in ('consola.ttf', 'arial.ttf'):
        try: return ImageFont.truetype(f, sz)
        except Exception: pass
    return ImageFont.load_default()

def labelled_sheets(prefix, ids, get, cell=(168, 140), cols=8):
    fnt = _font(12)
    for s in range(0, len(ids), PER):
        chunk = ids[s:s + PER]
        rows = (len(chunk) + cols - 1) // cols
        can = Image.new('RGBA', (cols * cell[0], rows * cell[1]), (36, 36, 56, 255))
        d = ImageDraw.Draw(can)
        for k, i in enumerate(chunk):
            cx, cy = (k % cols) * cell[0], (k // cols) * cell[1]
            try: im = get(i)
            except Exception: im = np.zeros((1, 1, 4), np.uint8)
            h, w = im.shape[:2]
            pim = Image.fromarray(im)
            sc = min((cell[0] - 8) / w, (cell[1] - 22) / h, 1.0)
            if sc < 1: pim = pim.resize((max(1, int(w * sc)), max(1, int(h * sc))), Image.BILINEAR)
            d.rectangle([cx + 2, cy + 2, cx + cell[0] - 3, cy + cell[1] - 3], outline=(70, 70, 100, 255))
            can.alpha_composite(pim, (cx + 4, cy + 18))
            d.text((cx + 5, cy + 3), '0x%03X %dx%d' % (i, w, h), fill=(255, 230, 120, 255), font=fnt)
        out = '%s_%03X-%03X.png' % (prefix, chunk[0], chunk[-1])
        can.save(out); print(out)

def pick(out, ids, get):
    fnt = _font(14)
    ims = [(i, Image.fromarray(get(i))) for i in ids]
    CW = 1600; x = y = rowh = 0; pos = []
    for i, im in ims:
        w, h = im.size
        if x + w > CW and x > 0: x = 0; y += rowh + 26; rowh = 0
        pos.append((x, y)); x += w + 14; rowh = max(rowh, h)
    can = Image.new('RGBA', (CW, y + rowh + 26), (36, 36, 56, 255))
    d = ImageDraw.Draw(can)
    for (i, im), (px, py) in zip(ims, pos):
        can.alpha_composite(im, (px, py + 18))
        d.text((px, py + 2), '0x%03X' % i, fill=(255, 230, 120, 255), font=fnt)
    can.save(out); print(out)

def _ids(lst):
    out = []
    for a in lst:
        if '-' in a:
            lo, hi = [int(x, 16) for x in a.split('-')]; out += range(lo, hi + 1)
        else: out.append(int(a, 16))
    return out

if __name__ == '__main__':
    cmd = args[0] if args else ''
    if cmd == 'sheet-pc':
        pc = PcArt(PAL); labelled_sheets(args[1], list(range(1, pc.count())), pc.sprite)
    elif cmd == 'sheet-arcade':
        labelled_sheets(args[1], list(range(st.ARCADE_COUNT)), arcade_sprite)
    elif cmd == 'pick-pc':
        pc = PcArt(PAL); pick(args[1], _ids(args[2:]), pc.sprite)
    elif cmd == 'pick-arcade':
        pick(args[1], _ids(args[2:]), arcade_sprite)
    elif cmd == 'side':
        # side <out.png> <ours.png> <real.png>: both scaled to 1280 x 720, ours left, the real PC's right, labelled
        ims = [Image.open(p).convert('RGB').resize((1280, 720), Image.LANCZOS) for p in args[2:4]]
        can = Image.new('RGB', (2560 + 16, 720 + 34), (20, 20, 20))
        d = ImageDraw.Draw(can); f = _font(22)
        for k, (im, t) in enumerate(zip(ims, ('ReWrithe: ported front end (hotd --frontend-shot)',
                                              'THOTD.EXE under the PC harness (real)'))):
            can.paste(im, (k * (1280 + 16), 34)); d.text((k * (1280 + 16) + 8, 6), t, fill=(255, 230, 120), font=f)
        can.save(args[1]); print(args[1])
    else:
        print(__doc__)
