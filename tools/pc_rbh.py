#!/usr/bin/env python3
"""
pc_rbh.py - read The House of the Dead (PC, 1998) .RBH world files (ART\\STAGEn\\WORLD\\WORLDn.RBH) and export them to .OBJ.

Container (little-endian), same PIFF style as .VMC:
  "PIFF" <u32 size> "RBHF"
  "RBHH" <u32 0x24>  u32 0, u32 size of BODY 0, u32 2, u32 0, u32 size of BODY 1, 4 x u32 0
  "BODY" (0)  object table + object headers + polygon groups
  "BODY" (1)  vertex positions (float x, y, z)
  "BODY" (2)  empty in WORLD1
  "RELC" x 3  relocation lists: u32 0, u32 target body, then u32 offsets (in BODY 0) of pointer fields into that body

BODY 0 starts with the object table, 12 bytes per entry: u16 id, u16 0xFFFF, u32 header offset, u32 0.
Entries with offset 0 (and a nonzero last field) aren't objects and are skipped.
Object header (offsets from the header start; meanings marked ? are guesses):
  +0x00 u32 offset of the object's polygon groups (in BODY 0)    +0x04 u32 vertex offset in BODY 1 (bytes)
  +0x08 u32 own offset    +0x10 u32 vertex count    +0x14 u32 polygon count
  +0x18 f32 bounding radius?    +0x20 3 x f32 center?    (header is 0x2C bytes; the next object's groups usually follow it)
  Polygon groups follow one another and end with a head of 0xFFFF0000.
Polygon group: u32 head, u32 sizes, then records.
  head: bits 0-15 record count, bits 16-22 kind, bit 23 unknown flag, bits 24-31 flags (seen 0x80-0x83, 0xA0-0xA3, 0xC0, 0xE0)
  sizes: bits 16-31 = length of the records in 16-bit words (next group = group + 8 + 2 * that)
  kind 0, untextured triangle: u16 i0, i1, i2, u16 ?                                         (8 bytes)
  kind 1, untextured quad:     u16 i0, i1, i2, i3, u16 ?                                     (10 bytes)
  kind 2, textured triangle:   u16 i0, i1, i2, u16 texture, u16 0, u16 0xFFFF, 3 x (f32 u, v) (36 bytes)
  kind 4, textured quad:       u16 i0, i1, i2, i3, u16 texture, u16 0, 4 x (f32 u, v)         (44 bytes)
Vertex indices are relative to the object's first vertex. UVs are in texels (0-256).

Usage:
  python pc_rbh.py info <WORLDn.RBH>
  python pc_rbh.py obj  <WORLDn.RBH> <out.obj>
Write output only into private\\ (it is game data).
"""
import sys, struct


def chunks(d):
    magic, size, kind = struct.unpack_from("<4sI4s", d, 0)
    if magic != b"PIFF" or kind != b"RBHF":
        raise SystemExit(f"not an RBHF PIFF file ({magic!r}/{kind!r})")
    out, p = [], 12
    while p < len(d):
        tag, n = struct.unpack_from("<4sI", d, p)
        out.append((tag, p + 8, n))
        p += 8 + n
    return out


def read_world(path):
    d = open(path, "rb").read()
    bodies = [(o, n) for tag, o, n in chunks(d) if tag == b"BODY"]
    b0, (b1, b1n) = bodies[0][0], bodies[1]
    verts = [struct.unpack_from("<3f", d, b1 + i) for i in range(0, b1n, 12)]
    objects, q = [], b0
    while struct.unpack_from("<H", d, q + 2)[0] == 0xFFFF:
        oid, _, off, extra = struct.unpack_from("<HHII", d, q)
        q += 12
        if off == 0:   # e.g. id 0xCFD in WORLD1: offset 0, last field 0xFFFF0CED; not an object (meaning unknown)
            continue
        h = b0 + off
        groups, voff, _, _, nv, npoly = struct.unpack_from("<6I", d, h)
        radius = struct.unpack_from("<f", d, h + 0x18)[0]
        center = struct.unpack_from("<3f", d, h + 0x20)
        base = voff // 12
        polys, g = [], b0 + groups
        while True:
            head, sizes = struct.unpack_from("<II", d, g)
            if head == 0xFFFF0000:   # end of the object's groups
                break
            count, kind = head & 0xFFFF, (head >> 16) & 0x7F
            r = g + 8
            for _ in range(count):
                if kind == 0:
                    idx = struct.unpack_from("<3H", d, r); tex = struct.unpack_from("<H", d, r + 6)[0]; uv = (); r += 8
                elif kind == 1:
                    idx = struct.unpack_from("<4H", d, r); tex = struct.unpack_from("<H", d, r + 8)[0]; uv = (); r += 10
                elif kind == 2:
                    idx = struct.unpack_from("<3H", d, r); tex = struct.unpack_from("<H", d, r + 6)[0]
                    uv = struct.unpack_from("<6f", d, r + 12); r += 36
                elif kind == 4:
                    idx = struct.unpack_from("<4H", d, r); tex = struct.unpack_from("<H", d, r + 8)[0]
                    uv = struct.unpack_from("<8f", d, r + 12); r += 44
                else:
                    raise SystemExit(f"object {oid:#x}: unknown polygon group {head:#010x} at {g - b0:#x}")
                polys.append(([base + i for i in idx], tex, uv))
            g += 8 + 2 * (sizes >> 16)
        if len(polys) != npoly:
            raise SystemExit(f"object {oid:#x}: {len(polys)} polygons, header says {npoly}")
        objects.append({"id": oid, "offset": off, "first_vertex": base, "vertex_count": nv,
                        "radius": radius, "center": center, "polys": polys})
    return verts, objects


if __name__ == "__main__":
    if len(sys.argv) < 3 or sys.argv[1] not in ("info", "obj"):
        print(__doc__); sys.exit(1)
    verts, objects = read_world(sys.argv[2])
    if sys.argv[1] == "info":
        print(f"{len(verts)} vertices, {len(objects)} objects, {sum(len(o['polys']) for o in objects)} polygons")
        for o in objects:
            c = ", ".join(f"{x:.1f}" for x in o["center"])
            print(f"  id {o['id']:#06x}  verts {o['first_vertex']:5}+{o['vertex_count']:<4} polys {len(o['polys']):4}  r {o['radius']:7.1f}  center ({c})")
    else:
        with open(sys.argv[3], "w") as f:
            f.write(f"# {sys.argv[2]} exported by pc_rbh.py\n")
            for x, y, z in verts:
                f.write(f"v {x:.6f} {y:.6f} {z:.6f}\n")
            for o in objects:
                f.write(f"o pc_{o['id']:04x}\n")
                for idx, _, _ in o["polys"]:
                    f.write("f " + " ".join(str(i + 1) for i in idx) + "\n")
        print(f"wrote {sys.argv[3]}")
