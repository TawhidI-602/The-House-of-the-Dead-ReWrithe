#!/usr/bin/env python3
"""
m2_level.py - build an arcade level from the arcade's own data: placement tables (maincpu) + model table (main_data) + models (polygons).

Found 2026-10-07 (see docs/formats.md §12):
  Model table   main_data 0xE73530 (CPU 0x02E73530), 16 bytes per object ID, in the order of the geometry engine's
                "object data" command: u32 texture point address, u32 texture header address,
                u32 0x800000 | polygon ROM word address, u32 link count
  Placement     maincpu, 24 bytes per record, list ended by 0xFFFFFFFF:
                  u32 object ID, f32 x, y, z, u32 flag (0xFF, or 0xNNNN0001), u32 pointer (0 or a maincpu offset)
                  world position = model vertex + (x, y, z); object IDs are the same as in the PC version's RBH files.

Usage:
  python m2_level.py tables  <regions folder>                       list the placement tables found in maincpu
  python m2_level.py obj     <regions folder> <table offset> <out.obj>   export one placed level (offset in hex)
  python m2_level.py compare <regions folder> <table offset> <WORLDn.RBH> <out.csv>
                                                                     per object ID: arcade model + placement vs the PC object
Write output only into private\\.
"""
import sys, os, csv, struct
import numpy as np
import m2_models

MODEL_TABLE = 0xE73530
TABLE_LOCATIONS = (0xBEF00, 0xBFEA0, 0xC0640, 0xC0A90)   # found by scanning maincpu; see "tables"


def load_regions(folder):
    mc = np.fromfile(os.path.join(folder, "maincpu.bin"), dtype="<u4")
    md = np.fromfile(os.path.join(folder, "main_data.bin"), dtype="<u4")
    poly = m2_models.load(os.path.join(folder, "polygons.bin"))
    return mc, md, poly


def model_record(md, oid):
    tpa, tha, obj, count = (int(x) for x in md[(MODEL_TABLE >> 2) + 4 * oid:(MODEL_TABLE >> 2) + 4 * oid + 4])
    return {"addr": obj & 0x7FFFFF if obj & 0x800000 else None, "raw": obj, "count": count, "tpa": tpa, "tha": tha}


def placements(mc, offset):
    out, i = [], offset >> 2
    while int(mc[i]) != 0xFFFFFFFF:
        oid = int(mc[i])
        x, y, z = struct.unpack("<3f", mc[i + 1:i + 4].tobytes())
        out.append({"id": oid, "pos": (x, y, z), "flag": int(mc[i + 4]), "ptr": int(mc[i + 5])})
        i += 6
    return out


def placed_model(md, poly, oid, pos):
    rec = model_record(md, oid)
    if rec["addr"] is None:
        return rec, None
    r = m2_models.parse(poly, poly.view("<f4"), rec["addr"])
    if r is None:
        return rec, None
    off = np.array(pos)
    return rec, [([tuple(np.array(v) + off) for v in vs], n, at) for vs, n, at in r[0]]


def main():
    cmd, folder = sys.argv[1], sys.argv[2]
    mc, md, poly = load_regions(folder)
    if cmd == "tables":
        for t in TABLE_LOCATIONS:
            p = placements(mc, t)
            ids = [r["id"] for r in p]
            print(f"maincpu {t:#07x}: {len(p)} placements, IDs {min(ids):#x}-{max(ids):#x}")
        return
    table = int(sys.argv[3], 16)
    if cmd == "obj":
        models = []
        for k, pl in enumerate(placements(mc, table)):
            rec, polys = placed_model(md, poly, pl["id"], pl["pos"])
            if polys:
                models.append((f"id_{pl['id']:04x}_{k}", polys))
        m2_models.write_obj(sys.argv[4], models)
        print(f"wrote {len(models)} placed models to {sys.argv[4]}")
    elif cmd == "compare":
        import pc_rbh
        verts, objects = pc_rbh.read_world(sys.argv[4])
        from compare_world import triangles, surface_distance
        verts = np.array(verts)
        pc = {o["id"]: o for o in objects if o["polys"]}

        def score(pc_id, polys):
            o = pc[pc_id]
            p = verts[sorted({i for q in o["polys"] for i in q[0]})]
            a = np.array([v for vs, _, _ in polys for v in vs])
            d_pa = surface_distance(p, triangles([vs for vs, _, _ in polys]))                    # PC vertices -> arcade surface
            d_ap = surface_distance(a, triangles([[verts[i] for i in q[0]] for q in o["polys"]]))  # arcade vertices -> PC surface
            return np.median(d_pa), np.percentile(d_pa, 90), np.median(d_ap), np.percentile(d_ap, 90)

        # 1) same ID; 2) PC objects left over are tried against placements that found no same-ID partner
        pls = placements(mc, table)
        built = [placed_model(md, poly, pl["id"], pl["pos"]) for pl in pls]
        result = {}
        for k, (pl, (rec, polys)) in enumerate(zip(pls, built)):
            if pl["id"] in pc and polys:
                s = score(pl["id"], polys)
                if s[0] <= 1.0:
                    result[k] = (pl["id"], s, "same ID")
        used_pc = {v[0] for v in result.values()}
        for pc_id in sorted(set(pc) - used_pc):
            best = None
            for k, (pl, (rec, polys)) in enumerate(zip(pls, built)):
                if k in result or not polys:
                    continue
                s = score(pc_id, polys)
                if best is None or s[0] < best[1][0]:
                    best = (k, s)
            if best and best[1][0] <= 1.0:
                result[best[0]] = (pc_id, best[1], "different ID (other instance)")
                used_pc.add(pc_id)

        rows = []
        for k, (pl, (rec, polys)) in enumerate(zip(pls, built)):
            row = [f"{pl['id']:#06x}", f"{rec['raw']:#x}", rec["count"]] + [f"{v:.3f}" for v in pl["pos"]] + [f"{pl['flag']:#x}", f"{pl['ptr']:#x}"]
            if k in result:
                pc_id, s, how = result[k]
                rows.append(row + [f"{pc_id:#06x}"] + [f"{x:.3f}" for x in s] + [how])
            elif polys and pl["id"] in pc:
                s = score(pl["id"], polys)
                rows.append(row + [f"{pl['id']:#06x}"] + [f"{x:.3f}" for x in s] + ["same ID, but the shapes differ (median > 1)"])
            else:
                note = "no arcade model" if not polys else ("PC object has no polygons" if pl["id"] in {o["id"] for o in objects} else "no matching PC object")
                rows.append(row + [""] * 5 + [note])
        for pc_id in sorted(set(pc) - used_pc):
            rows.append([""] * 8 + [f"{pc_id:#06x}"] + [""] * 4 + ["PC object without an arcade match"])
        with open(sys.argv[5], "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["arcade_id", "model_raw", "links", "x", "y", "z", "flag", "ptr", "pc_id",
                        "pc_to_arcade_median", "pc_to_arcade_p90", "arcade_to_pc_median", "arcade_to_pc_p90", "note"])
            w.writerows(rows)
        print(f"{len(pls)} placements, {len(pc)} PC objects with polygons: {len(used_pc)} matched "
              f"({sum(1 for v in result.values() if v[2] != 'same ID')} through another ID), "
              f"{len(set(pc) - used_pc)} PC objects unmatched, {len(pls) - len(result)} placements without a PC partner")


if __name__ == "__main__":
    if len(sys.argv) < 3 or sys.argv[1] not in ("tables", "obj", "compare"):
        print(__doc__); sys.exit(1)
    main()
