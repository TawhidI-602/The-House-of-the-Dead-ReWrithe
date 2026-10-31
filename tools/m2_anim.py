#!/usr/bin/env python3
"""
m2_anim.py - arcade creature skeletons and motions from the program ROM and main_data (formats.md §23).

Tables in maincpu (program ROM, i960 address = file offset):
  0xC7240  root skeleton node per creature type (94 types; pointers into maincpu)
  0xC73C0  joint count per creature type (int32)
  0xC7540  motion -> first frame record (674 motions; 0x06xxxxxx = main_data 0x1000000 + offset, 0x00xxxxxx = maincpu)
  0xC7FD0  motion -> frame count (int32)
Skeleton node (maincpu): +0x00 model ID (0 = none), +0x04 offset x, y, z (float), +0x10 flag, +0x14 joint index,
  +0x18 child count, +0x1C child node pointers.
Frame record: 3 floats (root position x, y, z) then one int16 triple (a0, a1, a2) per joint (65536 = 360 degrees),
  padded to 4 bytes: size = (12 + 6 * joints + 3) & ~3. Frames of a motion are back to back.
Drawing (maincpu 0x37720 object, 0x36240 per part; the geometry coprocessor's matrix stack does the math):
  object: T(pos) * R(obj +0x34, +0x30, +0x2C) * scale(+0x94) * T(root) * R(joint 0)
  node:   parent * T(x, y, -z) * R(-a2, a1, a0) ; the node's model is drawn with that matrix, its children start from it
  R(g1, g2, g3) (coprocessor function 0x86) = Rz(-g1) * Ry(-g2) * Rx(-g3), so a joint's rotation is Rz(a2) Ry(-a1) Rx(-a0)
  (rotation matrices in the usual right-handed form; columns of the result are the axes' images).
The root translation in the game: (x - x0, y, -(z - z0)) with x0/z0 from frame 1 of the motion (moving motions are drawn in
place; the object position moves instead), or (x, y, -z) for some objects (object byte +0x3C6 bit 0).

Usage:
  python m2_anim.py <regions folder> types                     every creature type: joints, skeleton, model IDs
  python m2_anim.py <regions folder> motions                   every motion: frames, joints, address
  python m2_anim.py <regions folder> pose <type> <motion> <frame>   part matrices (frame 1-based)
  python m2_anim.py <regions folder> check <workram dump> <object address hex>   rebuild an object's parts from the ROM and
                                                               compare with the matrices in the dump
"""
import sys, os, struct
import numpy as np

SKEL, JOINTS, MOTION_PTR, MOTION_LEN = 0xC7240, 0xC73C0, 0xC7540, 0xC7FD0
TYPES, MOTIONS = 94, 674


def record_size(joints):
    return (12 + 6 * joints + 3) & ~3


class Rom:
    def __init__(self, regions):
        self.cpu = open(os.path.join(regions, "maincpu.bin"), "rb").read()
        self.data = open(os.path.join(regions, "main_data.bin"), "rb").read()

    def u32(self, a):
        return struct.unpack_from("<I", self.cpu, a)[0]

    def joints(self, t):
        return struct.unpack_from("<i", self.cpu, JOINTS + 4 * t)[0]

    def node(self, a):
        mdl, x, y, z, flag, idx, n = struct.unpack_from("<IfffIII", self.cpu, a)
        kids = [self.node(self.u32(a + 0x1C + 4 * i)) for i in range(n)]
        return {"addr": a, "model": mdl, "off": (x, y, z), "flag": flag, "joint": idx, "children": kids}

    def skeleton(self, t):
        a = self.u32(SKEL + 4 * t)
        return self.node(a) if a else None

    def motion(self, m):
        """(bytes, offset, frame count)"""
        p = self.u32(MOTION_PTR + 4 * m)
        n = struct.unpack_from("<i", self.cpu, MOTION_LEN + 4 * m)[0]
        if p >> 24 == 6:
            return self.data, p - 0x06000000 + 0x1000000, n
        if p >> 24 == 2:
            return self.data, p - 0x02000000, n
        return self.cpu, p, n

    def motion_joints(self, m):
        """Joint count from the size of the motion (every motion fits exactly one count), None for the last one."""
        if m + 1 >= MOTIONS:
            return None
        p, q = self.u32(MOTION_PTR + 4 * m), self.u32(MOTION_PTR + 4 * m + 4)
        n = struct.unpack_from("<i", self.cpu, MOTION_LEN + 4 * m)[0]
        if q >> 24 != p >> 24 or q <= p:
            return None
        fits = [j for j in range(1, 64) if (n * record_size(j) + 15) // 16 * 16 == q - p]
        return fits[0] if len(fits) == 1 else None

    def frame(self, m, f, joints):
        """Root position and [(a0, a1, a2)] in radians of frame f (1-based)."""
        buf, off, n = self.motion(m)
        o = off + (f - 1) * record_size(joints)
        root = struct.unpack_from("<3f", buf, o)
        a = struct.unpack_from(f"<{3 * joints}h", buf, o + 12)
        k = np.pi / 32768
        return root, [(a[3 * i] * k, a[3 * i + 1] * k, a[3 * i + 2] * k) for i in range(joints)]


def rx(t):
    c, s = np.cos(t), np.sin(t)
    return np.array([[1, 0, 0], [0, c, -s], [0, s, c]])


def ry(t):
    c, s = np.cos(t), np.sin(t)
    return np.array([[c, 0, s], [0, 1, 0], [-s, 0, c]])


def rz(t):
    c, s = np.cos(t), np.sin(t)
    return np.array([[c, -s, 0], [s, c, 0], [0, 0, 1]])


def joint_rotation(a):
    return rz(a[2]) @ ry(-a[1]) @ rx(-a[0])


def mat4(r=None, t=None):
    m = np.eye(4)
    if r is not None:
        m[:3, :3] = r
    if t is not None:
        m[:3, 3] = t
    return m


def pose(skel, angles, base):
    """[(node, entry matrix, model matrix)]: entry = parent's model matrix (what the game saves at part +0x7C), model =
    entry * T(x, y, -z) * R(joint). base = the matrix the root node's children start from."""
    out = []

    def walk(node, parent):
        x, y, z = node["off"]
        m = parent @ mat4(t=(x, y, -z)) @ mat4(joint_rotation(angles[node["joint"]]))
        out.append((node, parent, m))
        for c in node["children"]:
            walk(c, m)

    for c in skel["children"]:
        walk(c, base)
    return out


def root_base(root, angles, first_root=None):
    """Matrix the root's children start from, in object space (object transform not included)."""
    x, y, z = root
    if first_root is not None:
        x, z = x - first_root[0], z - first_root[2]
    return mat4(t=(x, y, -z)) @ mat4(joint_rotation(angles[0]))


def cmd_types(rom):
    for t in range(TYPES):
        s = rom.skeleton(t)
        if s is None:
            print(f"type {t}: none")
            continue
        ids, stack = [], [s]
        while stack:
            n = stack.pop(0)
            if n["model"]:
                ids.append(n["model"])
            stack = n["children"] + stack
        print(f"type {t}: {rom.joints(t)} joints, root {s['addr']:#x}, models " + " ".join(f"{i:x}" for i in ids))


def cmd_motions(rom):
    for m in range(MOTIONS):
        buf, off, n = rom.motion(m)
        where = "main_data" if buf is rom.data else "maincpu"
        print(f"motion {m}: {n} frames, {rom.motion_joints(m)} joints, {where} {off:#x}")


def cmd_pose(rom, t, m, f):
    j = rom.joints(t)
    root, ang = rom.frame(m, f, j)
    _, _, n = rom.motion(m)
    first, _ = rom.frame(m, 1, j)
    for node, _, mm in pose(rom.skeleton(t), ang, root_base(root, ang, first)):
        if node["model"]:
            print(f"{node['model']:x} joint {node['joint']} " + " ".join(f"{v:.5f}" for v in mm[:3, :].T.flatten()))


def cmd_check(rom, dump, obj):
    w = open(dump, "rb").read()
    base_addr = 0x500000
    u32 = lambda a: struct.unpack_from("<I", w, a - base_addr)[0]
    i16 = lambda a: struct.unpack_from("<h", w, a - base_addr)[0]
    f32s = lambda a, n: np.array(struct.unpack_from(f"<{n}f", w, a - base_addr))
    t = i16(obj + 0xCC)
    j = rom.joints(t)
    cur = u32(obj + 0xD8)
    m = i16(obj + 0xD0)
    buf, off, n = rom.motion(m)
    p0 = rom.u32(MOTION_PTR + 4 * m)
    f = (cur - p0) // record_size(j) + 1
    print(f"type {t} ({j} joints), motion {m} ({n} frames), frame {f} (object +0xF4 = {i16(obj + 0xF4)})")

    def saved(a):   # 12 floats: three column vectors, then the translation
        v = f32s(a, 12)
        return mat4(v[:9].reshape(3, 3).T, v[9:])

    # +0x1E4: the matrix after T(root), before the root joint's rotation
    root, ang = rom.frame(m, f, j)
    base = saved(obj + 0x1E4) @ mat4(joint_rotation(ang[0]))
    # runtime part tree (same shape as the ROM skeleton) for the saved entry matrices
    parts = []

    def walk(p):
        for i in range(u32(p + 0x34)):
            c = u32(u32(p + 0x24) + 4 * i)
            parts.append(c)
            walk(c)

    walk(u32(obj + 0xC8))
    worst_r = worst_t = 0.0
    for (node, entry, _), p in zip(pose(rom.skeleton(t), ang, base), parts):
        g = saved(p + 0x7C)
        dr, dt = np.abs(entry[:3, :3] - g[:3, :3]).max(), np.abs(entry[:3, 3] - g[:3, 3]).max()
        worst_r, worst_t = max(worst_r, dr), max(worst_t, dt)
        print(f"  joint {node['joint']:2d} model {node['model']:4x}: rotation error {dr:.5f}, translation error {dt:.4f}")
    print(f"worst: rotation {worst_r:.5f}, translation {worst_t:.4f}")


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    rom = Rom(sys.argv[1])
    cmd = sys.argv[2]
    if cmd == "types":
        cmd_types(rom)
    elif cmd == "motions":
        cmd_motions(rom)
    elif cmd == "pose":
        cmd_pose(rom, int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5]))
    elif cmd == "check":
        cmd_check(rom, sys.argv[3], int(sys.argv[4], 16))
    else:
        print(__doc__)
        sys.exit(1)


if __name__ == "__main__":
    main()
