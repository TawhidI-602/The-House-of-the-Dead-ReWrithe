#!/usr/bin/env python3
"""
arcade_props.py - objects the arcade draws that are neither placed (placement tables) nor creature parts: event objects such
as doors (0x73A `to0a` / 0x73B `todoor01a` at the mansion entrance), the clock, chandeliers (formats.md §27).

The game positions them from its event scripts (not emulated yet). This tool records from a replay where each one was drawn
and how: for every display list frame, the world transform of each such object (same camera recovery as creature_poses.py)
and the camera position. The viewer shows each object near the places the replay saw it, with the transform of the nearest
sighting (so a door opens as the camera passes it).

Usage:
  python arcade_props.py <regions folder> <dump folder> <texsets file> <out file>      (keep the output in private\\)
Output:
  prop <stage> <set index> <hex id> r00 r01 r02 r10 r11 r12 r20 r21 r22 tx ty tz     one transform of one object
  at x y z                                                                          camera positions that saw it
"""
import sys, os, glob, struct, collections
import numpy as np
import m2_level, m2_displaylist, m2_models, m2_anim
from creature_poses import frame_objects, TABLES, AREA_BYTE

STAGE_BYTE = 0x1F9D1
DEBRIS = ("nikubaan", "tamahi", "gee", "powa", "picha", "suiteki")


def names(md_bytes):
    """Object ID -> name: pointer table at main_data 0xEE4EB0 (CPU address 0x02xxxxxx of a "PN_..." string per ID, 0 = no
    name). The strings themselves are not one per ID (IDs without a name have none)."""
    out = []
    for i in range(7477):
        o = struct.unpack_from("<I", md_bytes, 0xEE4EB0 + 4 * i)[0] - 0x2000000
        ok = 0 <= o < len(md_bytes) and md_bytes[o:o + 3] == b"PN_"
        out.append(md_bytes[o + 3:md_bytes.index(b"\0", o)].decode("latin1") if ok else "")
    return out


def main(regions, dumps, texsets, out_path):
    mc, md, poly = m2_level.load_regions(regions)
    id_of = m2_displaylist.model_ids(md)
    pf = poly.view("<f4")
    nm = names(open(os.path.join(regions, "main_data.bin"), "rb").read())
    rom = m2_anim.Rom(regions)
    creature = set()
    for t in range(m2_anim.TYPES):
        s = rom.skeleton(t)
        stack = [s] if s else []
        while stack:
            n = stack.pop()
            creature.add(n["model"]); stack += n["children"]
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
    placed = {st: {pl["id"] for pl in m2_level.placements(mc, TABLES[st])} for st in range(4)}
    seen = collections.OrderedDict()   # (stage, set, id, rounded transform) -> [transform, camera positions]
    for f in sorted(glob.glob(os.path.join(dumps, "*_bufram.bin"))):
        tag = os.path.basename(f)[:-len("_bufram.bin")]
        wpath = os.path.join(dumps, tag + "_workram.bin")
        if not os.path.exists(wpath):
            continue
        w = open(wpath, "rb").read()
        stage, area = w[STAGE_BYTE], w[AREA_BYTE]
        if stage > 3:
            continue
        world_pos = {pl["id"]: np.array(pl["pos"]) for pl in m2_level.placements(mc, TABLES[stage])}
        objs, rc, eye = frame_objects(f, id_of, world_pos, size)
        if rc is None:
            continue
        set_index = sets.get(stage, []).index(area) if area in sets.get(stage, []) else 0
        for oid, r, t in objs:
            name = nm[oid] if oid < len(nm) else ""
            if (oid in placed[stage] or oid in creature or name.endswith("_DD") or name.startswith(DEBRIS)
                    or np.abs(r[1]).max() < 1e-3 or size(oid) < 2.0):
                continue
            key = (stage, set_index, oid, tuple(np.round(r.reshape(-1), 2)), tuple(np.round(t, 1)))
            if key not in seen:
                seen[key] = [r, t, []]
            cams = seen[key][2]
            if not cams or np.linalg.norm(np.array(cams[-1]) - eye) > 2.0:
                cams.append(tuple(eye))
        print(tag, stage + 1, len(seen), flush=True)
    with open(out_path, "w") as o:
        o.write(f"# arcade_props.py {dumps}\n")
        for (stage, set_index, oid, _, _), (r, t, cams) in seen.items():
            o.write(f"prop {stage} {set_index} {oid:x} " + " ".join(f"{x:.5f}" for x in r.reshape(-1)) + " "
                    + " ".join(f"{x:.3f}" for x in t) + "\n")
            for c in cams:
                o.write(f"at {c[0]:.2f} {c[1]:.2f} {c[2]:.2f}\n")
    print("wrote", out_path, len(seen), "transforms")


if __name__ == "__main__":
    if len(sys.argv) != 5:
        print(__doc__); sys.exit(1)
    main(*sys.argv[1:])
