#!/usr/bin/env python3
"""
m2_displaylist.py - read the geometry DSP display lists from MAME buffer RAM dumps (mame_dump.lua HOTD_BUFRAM=1).

Buffer RAM (CPU 0x00900000, 512 KB) holds the command lists the i960 builds for the geometry DSP, one per 64 KB block.
Commands (MAME model2_v.cpp geo_process_command, opcode bits 23-27; the game writes these exact opcode words):
  0x00800101 object data:     texture point address, texture header address, 0x800000 | model address, link count
  0x05800B0B matrix write:    12 floats (MAME transform_point: x' = m0 x + m3 y + m6 z + m9, ...; world/model -> camera)
  0x05000A0A light source:    3 floats, camera space (normals are transformed by the matrix, the light is not)
  0x07800F0F end of list (also MAME's fill value for empty buffer RAM; anything after it in the block is older data)
  0x03000606 texture params:  start index * 4, count, then count x (param, f32 distance coefficient);
                              param bits 0-7 diffuse, 8-15 ambient, 16-23 specular scale, 24-31 specular control
Polygon luma (MAME geo_parse_*): clamp(|n . light| * diffuse + ambient, 0, 255), 0 for |n . light| if light and viewer are on
different sides; the parameter index is polygon attr bits 18-22.

Usage (as a module): drawn(bufram path, id_of) -> list of (object ID, light in world space or None, params table or None)
  python m2_displaylist.py <regions folder> <..._bufram.bin>     print the objects drawn and their lights
"""
import sys, os
import numpy as np

OBJ, MATRIX, LIGHT, PARAMS, END = 0x00800101, 0x05800B0B, 0x05000A0A, 0x03000606, 0x07800F0F
BLOCK = 0x4000   # words per display list


def model_ids(md, last_id=0x1E00, table=0xE73530):
    """(texture point, texture header, model word) -> object ID, from the model table in main_data (u32 array)."""
    t = md[table // 4:table // 4 + 4 * last_id].reshape(-1, 4)
    return {(int(a), int(b), int(c)): i for i, (a, b, c, _) in enumerate(t) if c & 0x800000}


def parse_block(w, f, id_of):
    out, matrix, light, params = [], None, None, None
    i, n = 0, len(w)
    while i < n:
        op = int(w[i])
        if op == END:
            break   # the rest of the block is older data
        if op == MATRIX and i + 12 < n:
            matrix = f[i + 1:i + 13].astype(np.float64)
            i += 13
        elif op == LIGHT and i + 3 < n:
            light = f[i + 1:i + 4].astype(np.float64)
            i += 4
        elif op == PARAMS and i + 2 < n and int(w[i + 2]) <= 32:
            start, count = int(w[i + 1]) >> 2, int(w[i + 2])
            vals = [int(x) for x in w[i + 3:i + 3 + 2 * count:2]]
            params = {(start + k) & 31: (v & 0xFF, (v >> 8) & 0xFF) for k, v in enumerate(vals)}
            i += 3 + 2 * count
        elif op == OBJ and i + 4 < n:
            key = (int(w[i + 1]), int(w[i + 2]), int(w[i + 3]))
            oid = id_of.get(key)
            if oid is not None:
                lw = None
                if matrix is not None and light is not None:
                    r = np.array([[matrix[0], matrix[3], matrix[6]], [matrix[1], matrix[4], matrix[7]], [matrix[2], matrix[5], matrix[8]]])
                    lw = r.T @ light   # camera -> world (rotation part)
                    nrm = np.linalg.norm(lw)
                    lw = lw / nrm if nrm > 1e-6 else None
                out.append((oid, lw, params))
            i += 5
        else:
            i += 1
    return out


def sky_objects(path, id_of, model_size, max_offset=300.0, min_size=1000.0):
    """Objects drawn around the camera (matrix translation under max_offset) that are bigger than min_size: sky domes and
    backdrops. Returns [(object ID, offset from the camera in world space)]."""
    w = np.fromfile(path, dtype="<u4")
    f = w.view("<f4")
    out = []
    for b in range(0, len(w), BLOCK):
        ww, ff, matrix, i = w[b:b + BLOCK], f[b:b + BLOCK], None, 0
        while i < len(ww) - 13:
            op = int(ww[i])
            if op == END:
                break
            if op == MATRIX:
                matrix = ff[i + 1:i + 13].astype(np.float64); i += 13; continue
            if op == OBJ:
                oid = id_of.get((int(ww[i + 1]), int(ww[i + 2]), int(ww[i + 3])))
                if oid is not None and matrix is not None and np.linalg.norm(matrix[9:12]) < max_offset                         and model_size(int(ww[i + 3]) & 0x7FFFFF) > min_size:
                    r = np.array([[matrix[0], matrix[3], matrix[6]], [matrix[1], matrix[4], matrix[7]], [matrix[2], matrix[5], matrix[8]]])
                    out.append((oid, r.T @ matrix[9:12]))
                i += 5; continue
            i += 1
    return out


def drawn(path, id_of):
    w = np.fromfile(path, dtype="<u4")
    f = w.view("<f4")
    out = []
    for b in range(0, len(w), BLOCK):
        out += parse_block(w[b:b + BLOCK], f[b:b + BLOCK], id_of)
    return out


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(__doc__); sys.exit(1)
    md = np.fromfile(os.path.join(sys.argv[1], "main_data.bin"), dtype="<u4")
    res = drawn(sys.argv[2], model_ids(md))
    ids = sorted({o for o, _, _ in res})
    print(f"{len(res)} object commands, {len(ids)} distinct IDs: " + " ".join(f"{i:#x}" for i in ids))
    for oid, lw, p in res[:40]:
        print(f"  {oid:#06x} light {np.round(lw, 3) if lw is not None else None}")
    if res and res[0][2]:
        print("params:", res[0][2])
