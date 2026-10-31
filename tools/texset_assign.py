#!/usr/bin/env python3
"""
texset_assign.py - which texture set (texture RAM contents) each placed arcade model of Chapters 1-4 is drawn with,
from a full-game MAME dump run (replay_hotd.ps1 with -TexRam; e.g. private\\mame_dumps\\fullgame_us).

The game reloads parts of texture RAM per area (docs/formats.md §13), so one dump per chapter leaves some models with the
wrong textures. For every dump: chapter = stage byte (work RAM 0x51F9D1, 0-3), camera from work RAM (mame_camera.py),
texture set = the contents of the texture RAM dump before it (only when the next texture RAM dump has the same contents, so
samples around a reload are skipped). A model gets the set that was loaded most often while it was drawn: with buffer RAM dumps
(HOTD_BUFRAM=1, replay_hotd.ps1 -BufRam) "drawn" = the game put an object command for it in its display lists (exact; see
m2_displaylist.py); without them, coverage.py's on-screen test (a rough guess). Models never seen get the chapter's most common set.
With buffer RAM dumps it also writes the Model 2 lighting per set: the light vector of the world objects (world space) and the
lighting parameter table.

Usage:
  python texset_assign.py <regions folder> <dump folder> <out file> [--pcmatch <PC install>]
      --pcmatch: per-model sets from the comparison with the PC models (texset_pcmatch.py) override the camera-based ones
Output (read by hotd_viewer / ArcadeSource; keep it in private\\, it names MAME dump files):
  sets <stage 0-3> <tex tag>/<color tag> ...   per texture set: the texture RAM dump and the dump its colors come from
  model <stage> <hex id> <set index>
  start <stage> <x> <y> <z> <yaw deg> <pitch deg>     first gameplay camera of the chapter
  zone <stage> <set index> <x> <y> <z>   a recorded camera position and the set loaded there (the viewer uses the nearest)
  rows <stage> <set index> <row>:<library entry> ...   odd color rows that scene events swap (most common state)
  sky <stage> <set index> <hex id> <dx> <dy> <dz>   sky model drawn around the camera (world offset) in that area
  light <stage> <set index> <x> <y> <z> <diffuse 0> <ambient 0> ... <diffuse 31> <ambient 31>
"""
import sys, os, math, hashlib, struct
import numpy as np
import m2_level, m2_displaylist, m2_models
from coverage import boxes, seen, box_distance, CHAPTERS
from mame_camera import camera

STAGE_BYTE = 0x1F9D1
ROW_LIBRARY, ROW_ENTRIES = 0xE211, 125   # color translation row library (192 bytes per row), formats.md §19
AREA_BYTE = 0x1F9B4    # current area block 1-12 (texture set), formats.md §17
NEAR = 250.0   # units
SKY_SHARE = 0.1   # share of an area's frames a camera-centered model must be in to count as its sky
WEIGHT_D = 50.0
VALID_FRACTION = 0.15   # a set counts as valid for a model if its in-view weight is at least this share of the best set's


def in_view(p, f, b, cos_max=math.cos(math.radians(30))):
    """Distance to the nearest point of the box (5 x 3 x 5 grid) that is within NEAR units and inside the view cone, or None.
    (coverage.seen tests only the box's closest point, which misses flat ground right under the camera.)"""
    g = np.stack(np.meshgrid(*[np.linspace(b["lo"][i], b["hi"][i], n) for i, n in enumerate((5, 3, 5))], indexing="ij"), -1).reshape(-1, 3)
    v = g - p
    d = np.linalg.norm(v, axis=1)
    ok = (d <= NEAR) & (d > 1e-3)
    ok[ok] = (v[ok] @ f) / d[ok] >= cos_max
    return float(d[ok].min()) if ok.any() else None


def rom_source(dumps, md8, tag):
    """'rom:<area block>' if the texture RAM dump equals the ROM rebuild (formats.md §17: RAM 1 starts with the area block;
    everything but the generated corner must match), else the dump tag itself."""
    t = open(os.path.join(dumps, f"{tag}_texram.bin"), "rb").read()
    for n in range(1, 13):
        if t[0x200000:0x200000 + 0xC0000] == md8[n * 0x100000:n * 0x100000 + 0xC0000] and            t[0x40000:0xC0000] == md8[0x40000:0xC0000] and t[0xC0000:0xE0000] == md8[n * 0x100000 + 0xC0000:n * 0x100000 + 0xE0000]:
            return f"rom:{n}"
    return tag


def main(regions, dumps, out):
    mc, md, poly = m2_level.load_regions(regions)
    md8 = md.tobytes()
    chapter_boxes = {c - 1: boxes(mc, md, poly, t) for c, t in CHAPTERS.items()}
    id_of = m2_displaylist.model_ids(md)
    lights = {c: {} for c in range(4)}      # signature -> [light vectors], params
    params_of = {}
    skies, n_buf, sizes = {}, {}, {}   # sky models per (stage, area, ID): camera offsets; buffer RAM samples per area

    def model_size(addr):
        if addr not in sizes:
            r = m2_models.parse(poly, poly.view("<f4"), addr)
            sizes[addr] = float(np.ptp(np.array([v for vs, _, _ in r[0] for v in vs]), axis=0).max()) if r and r[0] else 0.0
        return sizes[addr]
    row_counts = {}   # (stage, area, row) -> {library entry: samples}
    mc8 = mc.tobytes()
    row_index = {mc8[ROW_LIBRARY + 192 * e:ROW_LIBRARY + 192 * (e + 1)]: e for e in range(ROW_ENTRIES)}
    zones = []      # (stage, signature, camera position) of every usable sample
    set_tags = {}   # (stage, signature) -> gameplay tags with that set (colors are taken from the middle one)
    best_view = {}   # (stage, box index) -> (closest in-view distance, set)
    tags = [l.split()[0] for l in open(os.path.join(dumps, "index.txt"))]
    tex_tags = [t for t in tags if os.path.exists(os.path.join(dumps, f"{t}_texram.bin"))]
    sig = {t: hashlib.sha1(open(os.path.join(dumps, f"{t}_texram.bin"), "rb").read()[:0x300000]).hexdigest() for t in tex_tags}
    tex_times = [float(t[1:]) for t in tex_tags]

    def set_of(tag):
        """Current area block (work RAM 0x51F9B4, formats.md §17), which fixes texture RAM and the area palette."""
        a = open(os.path.join(dumps, f"{tag}_workram.bin"), "rb").read()[AREA_BYTE]
        return int(a) if 1 <= a <= 12 else None

    sets = {c: [] for c in range(4)}        # signature list per chapter, first-seen order
    first_tag = {}
    counts = {c: {} for c in range(4)}      # (box index, signature) -> samples
    usage = {c: {} for c in range(4)}
    starts = {}
    for tag in tags:
        stage = open(os.path.join(dumps, f"{tag}_workram.bin"), "rb").read()[STAGE_BYTE]
        if stage > 3:
            continue
        s = set_of(tag)
        if s is None:
            continue
        cam = camera(dumps, tag)
        p, f = np.array(cam["pos"]), np.array(cam["forward"])
        if not np.all(np.isfinite(p)) or np.abs(p).max() > 1e5 or np.abs(p).max() < 1e-3:   # 0,0,0 = no camera yet (boot)
            continue
        if not any(box_distance(p, b["lo"], b["hi"]) < 30.0 for b in chapter_boxes[stage]):
            continue   # not inside the chapter's geometry (boot, menus, cut-scene cameras outside the level)
        if s not in sets[stage]:
            sets[stage].append(s)
            first_tag[(stage, s)] = f"rom:{s}"
        usage[stage][s] = usage[stage].get(s, 0) + 1
        # which row-library entry each odd color translation row holds (formats.md §19); counted per area
        xl = np.fromfile(os.path.join(dumps, f"{tag}_colorxlat.bin"), dtype="<u2")
        if len(xl) >= 3 * 0x2000:
            xl = (xl & 0xFF).astype(np.uint8).reshape(3, 32, 256)[:, :, :64]
            for row in range(1, 28, 2):
                e = row_index.get(bytes(xl[:, row, :].T.reshape(-1)))
                if e is not None:
                    key = (stage, s, row)
                    row_counts.setdefault(key, {}).setdefault(e, 0)
                    row_counts[key][e] += 1
        zones.append((stage, s, p))
        set_tags.setdefault((stage, s), []).append(tag)
        if stage not in starts:
            yaw = math.degrees(math.atan2(f[0], f[2]))
            pitch = math.degrees(math.asin(max(-1.0, min(1.0, f[1]))))
            starts[stage] = (p, yaw, pitch, tag)
        bufram = os.path.join(dumps, f"{tag}_bufram.bin")
        if os.path.exists(bufram):
            # lighting from the display lists (world objects that are in them; most world models are not, see formats.md)
            world_ids = {b["id"] for b in chapter_boxes[stage]}
            n_buf[(stage, s)] = n_buf.get((stage, s), 0) + 1
            for o, off in m2_displaylist.sky_objects(bufram, id_of, model_size):
                skies.setdefault((stage, s, o), []).append(off)
            for o, lw, prm in m2_displaylist.drawn(bufram, id_of):
                if o in world_ids and lw is not None:
                    lights[stage].setdefault(s, []).append(lw)
                if prm and len(prm) >= 31:
                    params_of.setdefault((stage, s), prm)
        # texture set: a vote over the samples where the model is in view, weighted by closeness (the texture RAM always fits
        # what is on screen; closer = more of the screen). Neither "first time near" nor "closest" alone works everywhere.
        for k, b in enumerate(chapter_boxes[stage]):
            d = in_view(p, f, b)
            if d is not None:
                counts[stage][(k, s)] = counts[stage].get((k, s), 0) + 1.0 / (d + WEIGHT_D)
    with open(out, "w") as o:
        o.write(f"# texset_assign.py {dumps}\n")
        for c in range(4):
            if not sets[c]:
                continue
            # texture RAM from the set's first dump; palette and color tables from the middle of its gameplay span
            # (the first dumps can be in a fade or title card)
            o.write(f"sets {c} " + " ".join(first_tag[(c, s)] + "/" + set_tags[(c, s)][len(set_tags[(c, s)]) // 2]
                                            for s in sets[c]) + "\n")
            common = max(usage[c], key=usage[c].get)
            per_id = {}
            valid = {}
            for k, b in enumerate(chapter_boxes[c]):
                best = max(sets[c], key=lambda s: counts[c].get((k, s), 0))
                if counts[c].get((k, best), 0) == 0:
                    best = common
                per_id.setdefault(b["id"], best)
                # sets the model was clearly in view with (its textures are loaded there): the viewer follows the camera's
                # set only for these, so models far from their own area keep their own set
                top = counts[c].get((k, best), 0)
                ok = [sets[c].index(s) for s in sets[c] if top > 0 and counts[c].get((k, s), 0) >= VALID_FRACTION * top]
                valid.setdefault(b["id"], set()).update(ok or [sets[c].index(best)])
            for oid, s in sorted(per_id.items()):
                o.write(f"model {c} {oid:x} {sets[c].index(s)}\n")
                o.write(f"valid {c} {oid:x} " + " ".join(str(v) for v in sorted(valid[oid])) + "\n")
            for si, sg in enumerate(sets[c]):
                ls, prm = lights[c].get(sg), params_of.get((c, sg))
                if ls and prm:
                    lv = np.median(np.array(ls), axis=0)
                    lv /= np.linalg.norm(lv)
                    spread = max(float(np.linalg.norm(x - lv)) for x in ls)
                    o.write(f"light {c} {si} {lv[0]:.4f} {lv[1]:.4f} {lv[2]:.4f} "
                            + " ".join(f"{prm.get(i, (0, 255))[0]} {prm.get(i, (0, 255))[1]}" for i in range(32))
                            + f"   # {len(ls)} world objects, max deviation {spread:.3f}" + "\n")
            for st_, sg, zp in zones:
                if st_ == c:
                    o.write(f"zone {c} {sets[c].index(sg)} {zp[0]:.1f} {zp[1]:.1f} {zp[2]:.1f}" + "\n")
            # rows whose most common library entry differs from the area's own row list (scene events swap them)
            for sg in sets[c]:
                lst = struct.unpack_from("<I", mc8, 0xA99A0 + 4 * sg)[0]
                default = {1 + 2 * k: (struct.unpack_from("<I", mc8, lst + 4 * k)[0] - ROW_LIBRARY) // 192 for k in range(14)}
                swaps = []
                for row in range(1, 28, 2):
                    cnt = row_counts.get((c, sg, row))
                    if cnt:
                        e = max(cnt, key=cnt.get)
                        if e != default[row]:
                            swaps.append(f"{row}:{e}")
                if swaps:
                    o.write(f"rows {c} {sets[c].index(sg)} " + " ".join(swaps) + "\n")
            for (st_, sg, sky_id), offs in sorted(skies.items()):
                # a sky model counts if it is in at least SKY_SHARE of the area's frames
                placed = {b["id"] for b in chapter_boxes[c]}
                if st_ == c and sky_id not in placed and len(offs) >= n_buf.get((st_, sg), 1) * SKY_SHARE:
                    m = np.median(np.array(offs), axis=0)
                    o_line = f"sky {c} {sets[c].index(sg)} {sky_id:x} {m[0]:.2f} {m[1]:.2f} {m[2]:.2f}"
                    o.write(o_line + chr(10))
            if c in starts:
                p, yaw, pitch, tag = starts[c]
                o.write(f"start {c} {p[0]:.2f} {p[1]:.2f} {p[2]:.2f} {yaw:.2f} {pitch:.2f}   # {tag}\n")
            print(f"chapter {c + 1}: {len(sets[c])} texture sets ({', '.join(first_tag[(c, s)] for s in sets[c])}), "
                  f"usage {[usage[c][s] for s in sets[c]]}")
    print(f"wrote {out}")


def append_pcmatch(regions, pc_dir, out):
    """Run texset_pcmatch.py per chapter (same set order as the "sets" lines) and append its model/valid lines: they override the
    camera-based guesses (the PC comparison is objective; see texset_pcmatch.py)."""
    import subprocess, tempfile
    sets = {}
    for line in open(out):
        if line.startswith("sets "):
            f = line.split()
            sets[int(f[1])] = [x.split("/")[0][4:] for x in f[2:]]
    extra = []
    for stage, blocks in sorted(sets.items()):
        tmp = os.path.join(tempfile.gettempdir(), f"texset_pc_{stage}.txt")
        subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "texset_pcmatch.py"), regions,
                        pc_dir, str(stage + 1), *blocks, "--out", tmp], check=True, stdout=subprocess.DEVNULL)
        extra += open(tmp).read().splitlines()
    with open(out, "a") as o:
        o.write("# from texset_pcmatch.py (overrides the lines above)\n" + "\n".join(extra) + "\n")
    print(f"appended {len(extra)} PC-match lines")


if __name__ == "__main__":
    args = sys.argv[1:]
    pc_dir = None
    if "--pcmatch" in args:
        i = args.index("--pcmatch"); pc_dir = args[i + 1]; del args[i:i + 2]
    if len(args) != 3:
        print(__doc__); sys.exit(1)
    main(*args)
    if pc_dir:
        append_pcmatch(args[0], pc_dir, args[2])
