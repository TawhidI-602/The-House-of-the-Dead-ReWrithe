#!/usr/bin/env python3
r"""
add_to_steam.py - add "The House of the Dead: ReWrithe" to Steam as a non-Steam game (shortcuts.vdf entry + grid artwork).

Usage:
  py tools/add_to_steam.py [--exe <hotd.exe or HOTDReWritheSetup.bat>] [--pc <PC game folder>] [--steam <Steam folder>]
       [--userdata-id <id>] [--dry-run] [--test-vdf <copy of shortcuts.vdf>] [--grid-out <folder>] [--undo]
The artwork is composed from the player's own PC game files (sprites 0xAA title art, 0xD1 mansion backdrop, 0xD4 logo
with Rogan) and is
never shipped. Steam must be closed (it rewrites shortcuts.vdf on exit). The old file is backed up first as
shortcuts.vdf.hotd-backup-<timestamp>; --undo restores the newest backup.
For a portable package, pass --exe "<package>\bin\hotd.exe": the game finds the player's files in the package's data\
by itself, so Steam starts it directly (no HOTDReWritheSetup.bat; the overlay and the playtime work).
Requires: pillow, numpy.
"""
import sys, os, struct, zlib, time, glob, shutil, subprocess

NAME = 'The House of the Dead: ReWrithe'
OLD_NAMES = ('HOTD ReWrithe',)   # earlier names of the entry: updated in place, never duplicated
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..'))
argv = sys.argv[1:]
def opt(name, default=None):
    if name in argv:
        i = argv.index(name); v = argv[i + 1]; del argv[i:i + 2]; return v
    return default
def flag(name):
    if name in argv: argv.remove(name); return True
    return False
EXE = opt('--exe'); PC = opt('--pc'); STEAM = opt('--steam'); UID = opt('--userdata-id')
TESTVDF = opt('--test-vdf'); GRIDOUT = opt('--grid-out')
DRY = flag('--dry-run'); UNDO = flag('--undo')

# ---- binary KeyValues (0 map, 1 string, 2 int32, 8 end) ----
def read_map(b, p):
    d = {}
    while True:
        t = b[p]; p += 1
        if t == 8: return d, p
        e = b.index(0, p); k = b[p:e].decode('utf-8', 'replace'); p = e + 1
        if t == 0: d[k], p = read_map(b, p)
        elif t == 1:
            e = b.index(0, p); d[k] = b[p:e].decode('utf-8', 'replace'); p = e + 1
        elif t == 2: d[k] = struct.unpack_from('<i', b, p)[0]; p += 4
        else: raise ValueError('unsupported VDF type %d' % t)
def write_map(d):
    o = bytearray()
    for k, v in d.items():
        kb = k.encode() + b'\0'
        if isinstance(v, dict): o += b'\x00' + kb + write_map(v)
        elif isinstance(v, int): o += b'\x02' + kb + struct.pack('<i', v)
        else: o += b'\x01' + kb + str(v).encode('utf-8') + b'\0'
    return bytes(o) + b'\x08'
def load_vdf(path):
    if not os.path.exists(path): return {'shortcuts': {}}
    b = open(path, 'rb').read()
    if not b: return {'shortcuts': {}}
    if b[0] != 0: raise ValueError('bad shortcuts.vdf')
    e = b.index(0, 1)
    m, _ = read_map(b, e + 1)
    return {b[1:e].decode(): m}
def save_vdf(path, d):
    k = list(d)[0]
    open(path, 'wb').write(b'\x00' + k.encode() + b'\0' + write_map(d[k]) + b'\x08')

def signed(u): return u - (1 << 32) if u >= (1 << 31) else u

def make_entry(exe, startdir, icon, appid):
    return {'appid': appid, 'AppName': NAME, 'Exe': '"%s"' % exe, 'StartDir': '"%s"' % startdir, 'icon': icon,
            'ShortcutPath': '', 'LaunchOptions': '', 'IsHidden': 0, 'AllowDesktopConfig': 1, 'AllowOverlay': 1,
            'OpenVR': 0, 'Devkit': 0, 'DevkitGameID': '', 'DevkitOverrideAppID': 0, 'LastPlayTime': 0,
            'FlatpakAppID': '', 'tags': {}}

def find_steam():
    if STEAM: return STEAM
    try:
        import winreg
        k = winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'Software\Valve\Steam')
        return winreg.QueryValueEx(k, 'SteamPath')[0].replace('/', '\\')
    except Exception:
        return None

def steam_running():
    try:
        out = subprocess.run(['tasklist', '/FI', 'IMAGENAME eq steam.exe'], capture_output=True, text=True).stdout
        return 'steam.exe' in out.lower()
    except Exception:
        return False

# ---- art ----
def compose_art(pc):
    import numpy as np
    from PIL import Image
    saved = sys.argv
    sys.argv = ['sprites_tool.py', '--pc', pc]
    sys.path.insert(0, HERE)
    try:
        import sprites_tool as st
        title = st.pc_sprite(0xAA); back = st.pc_sprite(0xD3)
        # the hero: 0xD1, the mansion under its sky with no logo (Steam draws the logo over the hero; 0xD3 has the
        # logo baked into its top); the logo: 0xD4, THOTD's own logo with real transparency (no colour keying)
        try:
            hall = st.pc_sprite(0xD1); logo_spr = st.pc_sprite(0xD4)
        except Exception:
            hall = logo_spr = None
    except Exception as e:
        print('WARNING: PC sprites could not be loaded (%s); skipping artwork.' % e); return None
    finally:
        sys.argv = saved
    def img(a):
        im = Image.fromarray(a, 'RGBA'); bg = Image.new('RGBA', im.size, (0, 0, 0, 255)); bg.alpha_composite(im)
        return bg.convert('RGB')
    T, B = img(title), img(back)
    def cover(im, w, h):
        s = max(w / im.width, h / im.height)
        r = im.resize((max(w, round(im.width * s)), max(h, round(im.height * s))), Image.LANCZOS)
        x = (r.width - w) // 2; y = (r.height - h) // 2; return r.crop((x, y, x + w, y + h))
    def fit(im, w, h):
        s = min(w / im.width, h / im.height)
        r = im.resize((round(im.width * s), round(im.height * s)), Image.LANCZOS)
        c = Image.new('RGB', (w, h), (8, 8, 10)); c.paste(r, ((w - r.width) // 2, (h - r.height) // 2)); return c
    from PIL import ImageFilter
    if logo_spr is not None:
        # logo: all of 0xD4, the lettering with Rogan standing on the right (user: keep him), cropped to its pixels
        # with a small margin
        a = logo_spr.copy()
        # specks: connected pieces (8-neighbour) under 150 pixels are dropped (stray pixels of the sprite's colour
        # key); every letter, the crow and Rogan are pieces of thousands
        op = a[..., 3] > 0
        seen = np.zeros(op.shape, bool)
        hgt, wid = op.shape
        for y0, x0 in zip(*np.nonzero(op)):
            if seen[y0, x0]: continue
            seen[y0, x0] = True; stack = [(y0, x0)]; piece = []
            while stack:
                y, x = stack.pop(); piece.append((y, x))
                for ny in (y - 1, y, y + 1):
                    for nx in (x - 1, x, x + 1):
                        if 0 <= ny < hgt and 0 <= nx < wid and op[ny, nx] and not seen[ny, nx]:
                            seen[ny, nx] = True; stack.append((ny, nx))
            if len(piece) < 150:
                for y, x in piece: a[y, x, 3] = 0
        ys, xs = np.nonzero(a[..., 3])
        logo = Image.fromarray(a, 'RGBA').crop((max(0, xs.min() - 4), max(0, ys.min() - 4), xs.max() + 5, ys.max() + 5))
    else:
        # fallback: the red lettering (plus its outline) in the upper half of the 640x480 title art, rest transparent
        sy = int(T.height * 0.44)
        ar = np.asarray(T.crop((0, 0, T.width, sy))).astype(int)
        red = ((ar[..., 0] > ar[..., 1] + 60) & (ar[..., 0] > ar[..., 2] + 60)).astype(np.uint8) * 255
        m = Image.fromarray(red).filter(ImageFilter.MaxFilter(9))
        ys, xs = np.nonzero(np.asarray(m))
        box = (xs.min(), ys.min(), xs.max() + 1, ys.max() + 1) if len(xs) else (0, 0, T.width, sy)
        la = np.dstack([np.asarray(T.crop((0, 0, T.width, sy))), np.asarray(m)])
        logo = Image.fromarray(la.astype(np.uint8), 'RGBA').crop(box)
    H = img(hall) if hall is not None else B
    # cover: blurred dark backdrop, title art fitted to the width
    bgd = cover(T, 600, 900).filter(ImageFilter.GaussianBlur(14)).point(lambda v: v * 0.45)
    r = T.resize((600, round(T.height * 600 / T.width)), Image.LANCZOS); bgd.paste(r, (0, (900 - r.height) // 2))
    return {'p.png': bgd, '.png': fit(T, 920, 430), '_hero.png': cover(H, 1920, 620),
            '_logo.png': logo, '_icon.png': fit(T, 256, 256)}

def main():
    steam = find_steam()
    targets = []   # (vdf path, grid dir)
    if TESTVDF:
        targets.append((TESTVDF, GRIDOUT or os.path.join(os.path.dirname(TESTVDF), 'grid')))
    else:
        if not steam or not os.path.isdir(steam): sys.exit('Steam folder not found; use --steam.')
        ud = os.path.join(steam, 'userdata')
        ids = [UID] if UID else [d for d in os.listdir(ud) if d.isdigit() and d != '0']
        for i in ids:
            cfg = os.path.join(ud, i, 'config')
            if os.path.isdir(cfg):
                targets.append((os.path.join(cfg, 'shortcuts.vdf'), GRIDOUT or os.path.join(cfg, 'grid')))
        if not targets: sys.exit('No userdata\\<id>\\config folder found.')
    if not TESTVDF and not DRY and steam_running():
        sys.exit('Steam is running. Close Steam completely (tray icon -> Exit) and run this again.')
    if UNDO:
        for vdf, _ in targets:
            bk = sorted(glob.glob(vdf + '.hotd-backup-*'))
            if not bk: print('No backup for', vdf); continue
            print(('Would restore ' if DRY else 'Restoring ') + bk[-1] + ' -> ' + vdf)
            if not DRY: shutil.copyfile(bk[-1], vdf)
        return
    exe = os.path.abspath(EXE or os.path.join(ROOT, 'build', 'Release', 'hotd.exe'))
    if not os.path.exists(exe): print('WARNING: %s does not exist (entry is still written).' % exe)
    startdir = os.path.dirname(exe)
    quoted = '"%s"' % exe
    crc = zlib.crc32((quoted + NAME).encode()) | 0x80000000
    appid = signed(crc)
    pre = str(crc)
    arts = compose_art(PC or os.path.join(ROOT, 'private', 'pcharness', 'game'))
    for vdf, grid in targets:
        d = load_vdf(vdf); sc = d['shortcuts']
        icon = os.path.join(grid, pre + '_icon.png') if arts else ''
        key = next((k for k, v in sc.items() if v.get('AppName') == NAME), None)
        if key is None:   # an entry under an earlier name: update it in place (its old grid art is removed below)
            key = next((k for k, v in sc.items() if v.get('AppName') in OLD_NAMES), None)
        old = sc[key].get('AppName') if key is not None else None
        act = 'update entry %s' % key if key is not None else 'add new entry'
        if key is None:
            key = str(max([int(k) for k in sc if k.isdigit()] + [-1]) + 1)
        print('%s%s in %s\n  Exe=%s\n  StartDir="%s"\n  appid=%d (grid prefix %s)' %
              ('Would ' if DRY else '', act, vdf, quoted, startdir, appid, pre))
        if arts:
            for suf, im in arts.items():
                print('  %s%s (%dx%d)' % ('would write ' if DRY else 'writing ', os.path.join(grid, pre + suf), im.width, im.height))
        if DRY: continue
        if os.path.exists(vdf):
            bk = '%s.hotd-backup-%s' % (vdf, time.strftime('%Y%m%d-%H%M%S'))
            shutil.copyfile(vdf, bk); print('  backup:', bk)
        sc[key] = make_entry(exe, startdir, icon, appid)
        os.makedirs(os.path.dirname(os.path.abspath(vdf)), exist_ok=True)
        save_vdf(vdf, d)
        if arts:
            os.makedirs(grid, exist_ok=True)
            for suf, im in arts.items(): im.save(os.path.join(grid, pre + suf))
        if old and old != NAME:   # renamed: the old name's grid art (its app id came from the old name) goes
            oldpre = str(zlib.crc32((quoted + old).encode()) | 0x80000000)
            for suf in ('p.png', '.png', '_hero.png', '_logo.png', '_icon.png'):
                f = os.path.join(grid, oldpre + suf)
                if os.path.exists(f): os.remove(f); print('  removed old art', f)
    print('(dry run: nothing written)' if DRY else 'Done.')

if __name__ == '__main__':
    main()
