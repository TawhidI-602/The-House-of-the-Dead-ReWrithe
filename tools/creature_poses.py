#!/usr/bin/env python3
"""
creature_poses.py - posed creatures (every part's world-space transform) from replay frames, for the viewer.

The arcade draws each creature as separate part models (object IDs shared with the PC's CREAT1/CREAT2.RBH, same local
coordinates), each with its own matrix in the display list (m2_displaylist.py). For a frame (buffer RAM dump), the camera is
recovered from a placed world model in the same list (world models have no rotation: matrix rotation = camera rotation,
eye = placement - R^T * translation); every other object's world transform is then R^T * M (rotation) and R^T * t + eye.
Skipped: placed world models, objects at the camera (HUD: translation under 5 units) and sky models (under 300 units, over
1000 units big).
Shadows: the arcade draws a low-polygon copy of many parts squashed onto the floor (transform with a zero vertical row);
those get the suffix "-shadow" on their category. The PC files have no shadow models.
Category per object: in a boss frame, IDs never seen in the zombie frames are "bosses"; tiny parts (under 1.5 units) or IDs
drawn 6+ times in one frame are "effects"; the rest "zombies".

Usage:
  python creature_poses.py <regions folder> <dump folder> <texsets file> <out file>
Frames are listed in FRAMES below (from the full-game replay). Output lines (keep in private\\):
  pose <stage> <set index> <zombies|bosses|effects>[-shadow] <hex id> r00 r01 r02 r10 r11 r12 r20 r21 r22 tx ty tz <creature>
      <creature>: frame * 100 + creature number in the frame (-1 for effects); shadows belong to the creature before them
  view <stage> <tag> x y z yaw pitch        the frame's camera (degrees)
"""
import sys, os, math, collections
import numpy as np
import m2_level, m2_displaylist, m2_models

TABLES = {0: 0xBEF00, 1: 0xBFEA0, 2: 0xC0640, 3: 0xC0A90}
# (stage, tag, boss frame?)
FRAMES = [(0, "t01060", False), (0, "t03200", True), (1, "t04400", False), (1, "t06000", True),
          (2, "t09400", False), (2, "t10300", True), (3, "t11400", False), (3, "t11900", True),   # t11900: Chapter 4 Hangedman rematch (area 10)
          (3, "t13540", True)]   # the Magician, final boss: last (game order; N steps through them in this order)
AREA_BYTE = 0x1F9B4
GROUP_DISTANCE, GROUP_DISTANCE_BOSS = 25.0, 90.0   # units from the creature's centre before a new creature starts


def frame_objects(path, id_of, world_pos, size):
    """[(id, R world 3x3, t world)] for the first display list of the dump that contains a placed world model."""
    w = np.fromfile(path, dtype="<u4")
    f = w.view("<f4")
    for b in range(0, 0x8000, m2_displaylist.BLOCK):
        ww, ff = w[b:b + m2_displaylist.BLOCK], f[b:b + m2_displaylist.BLOCK]
        objs, matrix, i = [], None, 0
        while i < len(ww) - 13:
            op = int(ww[i])
            if op == m2_displaylist.END:
                break   # the rest of the block is older data (MAME fills empty buffer RAM with END words too)
            if op == m2_displaylist.MATRIX:
                matrix = ff[i + 1:i + 13].astype(np.float64); i += 13; continue
            if op == m2_displaylist.OBJ:
                oid = id_of.get((int(ww[i + 1]), int(ww[i + 2]), int(ww[i + 3])))
                if oid is not None and matrix is not None:
                    r = np.array([[matrix[0], matrix[3], matrix[6]], [matrix[1], matrix[4], matrix[7]], [matrix[2], matrix[5], matrix[8]]])
                    objs.append((oid, r, matrix[9:12].copy()))
                i += 5; continue
            i += 1
        cam = next(((r, world_pos[o] - r.T @ t) for o, r, t in objs if o in world_pos), None)
        if cam is None:
            continue
        rc, eye = cam
        out = []
        for oid, r, t in objs:
            if oid in world_pos or np.linalg.norm(t) < 5.0 or (np.linalg.norm(t) < 300.0 and size(oid) > 1000.0):
                continue   # world, HUD, sky (the sky is drawn around the camera at (0, -9, 0))
            out.append((oid, rc.T @ r, rc.T @ t + eye))
        return out, rc, eye
    return [], None, None


def main(regions, dumps, texsets, out_path):
    mc, md, poly = m2_level.load_regions(regions)
    id_of = m2_displaylist.model_ids(md)
    pf = poly.view("<f4")
    sizes = {}

    def size(oid):
        if oid not in sizes:
            rec = m2_level.model_record(md, oid)
            r = m2_models.parse(poly, pf, rec["addr"]) if rec["addr"] is not None else None
            sizes[oid] = float(np.ptp(np.array([v for vs, _, _ in r[0] for v in vs]), axis=0).max()) if r and r[0] else 0.0
        return sizes[oid]

    sets = {}
    for line in open(texsets):
        if line.startswith("sets "):
            f = line.split()
            sets[int(f[1])] = [int(x.split("/")[0][4:]) for x in f[2:]]
    frames = []
    for stage, tag, boss in FRAMES:
        world_pos = {pl["id"]: np.array(pl["pos"]) for pl in m2_level.placements(mc, TABLES[stage])}
        objs, rc, eye = frame_objects(os.path.join(dumps, f"{tag}_bufram.bin"), id_of, world_pos, size)
        area = open(os.path.join(dumps, f"{tag}_workram.bin"), "rb").read()[AREA_BYTE]
        frames.append((stage, tag, boss, objs, rc, eye, area))
        print(f"{tag}: stage {stage + 1}, area {area}, {len(objs)} posed parts")
    zombie_ids = {o for st, tag, boss, objs, *_ in frames if not boss for o, _, _ in objs}
    with open(out_path, "w") as out:
        out.write(f"# creature_poses.py {dumps}\n")
        for frame_no, (stage, tag, boss, objs, rc, eye, area) in enumerate(frames):
            if rc is None:
                continue
            fwd = rc[2]
            out.write(f"view {stage} {tag} {eye[0]:.2f} {eye[1]:.2f} {eye[2]:.2f} {math.degrees(math.atan2(fwd[0], fwd[2])):.2f} "
                      f"{math.degrees(math.asin(max(-1.0, min(1.0, fwd[1])))):.2f}\n")
            set_index = sets.get(stage, []).index(area) if area in sets.get(stage, []) else 0
            counts = collections.Counter(o for o, _, _ in objs)
            # creatures: the parts of one creature come one after another in the display list; a new creature starts where
            # the next (non-shadow, non-effect) part is far from the current creature's parts
            group, centre, n_in, next_group = -1, None, 0, 0
            link = GROUP_DISTANCE_BOSS if boss else GROUP_DISTANCE
            for oid, r, t in objs:
                cat = "bosses" if boss and oid not in zombie_ids else ("effects" if size(oid) < 1.5 or counts[oid] >= 6 else "zombies")
                shadow = np.abs(r[1]).max() < 1e-3
                if shadow:
                    cat += "-shadow"   # projected flat onto the floor (vertical row zero): the arcade's shadow models
                if not shadow and not cat.startswith("effects"):
                    if centre is None or np.linalg.norm(t - centre) > link:
                        group, centre, n_in, next_group = next_group, t.copy(), 0, next_group + 1
                    n_in += 1
                    centre = centre + (t - centre) / n_in
                g = frame_no * 100 + group if group >= 0 and not cat.startswith("effects") else -1
                out.write(f"pose {stage} {set_index} {cat} {oid:x} " + " ".join(f"{x:.5f}" for x in r.reshape(-1))
                          + " " + " ".join(f"{x:.3f}" for x in t) + f" {g}\n")
    print("wrote", out_path)


if __name__ == "__main__":
    if len(sys.argv) != 5:
        print(__doc__); sys.exit(1)
    main(*sys.argv[1:])
