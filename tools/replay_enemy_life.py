"""replay_enemy_life.py - usage: py tools/replay_enemy_life.py private/mame_dumps/<run>
Scan work RAM dumps for enemy objects (tasks whose routine is enemy_main 0x77410) and print +0x70 / +0x72 over
time, to see whether +0x70 is a life that goes down from the spawn record's +0x22 value."""
import glob, os, struct, sys, collections

d = sys.argv[1]
ROUT = {0x77410: "enemy_main", 0x778c0: "check_death", 0x76b80: "dying"}
track = collections.defaultdict(list)
for f in sorted(glob.glob(os.path.join(d, "t*_workram.bin"))):
    t = os.path.basename(f)[:6]
    b = open(f, "rb").read()
    for o in range(0, len(b) - 0x3C8, 4):
        if struct.unpack_from("<I", b, o + 0xC)[0] != 0x80018691:
            continue
        r = struct.unpack_from("<I", b, o)[0]
        if r not in (0x77410, 0x76b80):
            continue
        life, lmax = struct.unpack_from("<hh", b, o + 0x70)
        typ = struct.unpack_from("<h", b, o + 0xCC)[0]
        serial = struct.unpack_from("<I", b, o + 8)[0]
        track[serial].append((t, ROUT.get(r, hex(r)), typ, life, lmax))
for s, rows in sorted(track.items()):
    if len(rows) > 1 or rows[0][3] != rows[0][4]:
        print(s, " ".join(f"{t}:{r[:5]}:{ty}:{l}/{m}" for t, r, ty, l, m in rows))
print(len(track), "enemies seen")
