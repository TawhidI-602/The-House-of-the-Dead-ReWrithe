#!/usr/bin/env python3
"""
pc_diff.py - the PC Mode differential run (pccap, docs/pc-harness.md "Differential run"): the PC harness's trace
(tools/pcharness/run_pc.ps1 -Map tools/pcharness/pccap_map.tsv: state.rec with the gun records) against our runtime
(HOTD_STATE_TRACE, fed with the PC's own gun through HOTD_GUN_REPLAY), frame by frame from the first game-mode-4 frame
(the PC's: the tick after its first mode-4 tick, which only runs game_new; checked: floor_y's first change, the boss
entrance and the Chariot's first frame land on the same frame then).

  py tools/pc_diff.py trace <state.rec> <out.txt>     the PC record in the runtime's HOTD_STATE_TRACE format
  py tools/pc_diff.py gun <state.rec> <out.txt>       player 1's shots / reloads for HOTD_GUN_REPLAY
  py tools/pc_diff.py diff <pc.txt> <ours.txt> [--to F] [--tol 0.05] [--max 20] [--offset N] [--from F] [--inf-lives]
                                                      [--no-ball-boxes]
                                                      the first frames where they differ, by field
Fields: mode, chapter, event, rand state, floor_y, enemies alive / count, players 1 / 2 (state, lives, score, rounds),
the Boss Mode clock (Boss Mode records), the shootable objects (state, hit points, position; matched as a sorted
list, positions within --tol). PC object layout = the arcade's + 8 (flags +0x60, state +0x64, hit points +0x78,
position +0x28); the Player records are the same on both.
"""
import struct, sys

PC = dict(game_mode=0xDA0CDC, chapter=0xDA0CF4, event=0xDA0CF3, rand=0xDA10F0, floor_y=0xDA0B48, alive=0xDA0CD0,
          count=0xDA0CCE, players=0xD9F05C, gun=0xD9F5E0, boss_mode=0xC723B8, clock0=0x5CD37C, shown=0x5CD380,
          clock_ms=0xC723D0, mouse_buttons=0x12FDB80)


def frames(path):
    """yields (tick, mode, getter(addr, size) -> bytes or None, tasks [(addr, bytes)])"""
    f = open(path, "rb")
    h = f.read(12)
    if h[:8] != b"HOTDREC1":
        raise SystemExit("%s: not a state record" % path)
    n = struct.unpack_from("<I", h, 8)[0]
    rng = [struct.unpack("<II", f.read(8)) for _ in range(n)]
    offs, o = [], 0
    for a, s in rng:
        offs.append((a, s, o))
        o += s
    gsize = o
    while True:
        hd = f.read(20)
        if len(hd) < 20:
            return
        num, cnt, mode, rnd = struct.unpack_from("<IIII", hd, 4)
        g = f.read(gsize)
        b = f.read(4)
        if len(b) < 4:
            return
        tasks = []
        for _ in range(struct.unpack("<I", b)[0]):
            ts = f.read(8)
            if len(ts) < 8:
                return   # cut off (the game was stopped while writing)
            a, s = struct.unpack("<II", ts)
            tasks.append((a, f.read(s)))

        def get(addr, size, g=g):
            for a, s, oo in offs:
                if a <= addr and addr + size <= a + s:
                    return g[oo + addr - a: oo + addr - a + size]
            return None
        yield num, mode, rnd, get, tasks


def i32(b, o=0): return struct.unpack_from("<i", b, o)[0]
def f32(b, o=0): return struct.unpack_from("<f", b, o)[0]


def game_frames(rec):
    """yields (game frame, tick, mode, rand, get, tasks) from the first game-mode-4 tick on, without the PC's game_new
    ticks (game mode 4 with player 1 in state 0: the tick a game starts in, which ours runs inside its first frame;
    also each next boss's game in All Bosses), so the numbers are our runtime's (HOTD_STATE_TRACE)"""
    gf, started = -1, False
    for num, mode, rnd, get, tasks in frames(rec):
        mode &= 0xFFFF
        if not started and mode != 4:
            continue
        started = True
        if mode == 4 and i32(get(PC["players"], 4)) == 0:
            continue
        gf += 1
        yield gf, num, mode, rnd, get, tasks


def pc_trace(rec, out, digits=2):
    first = None
    with open(out, "w") as o:
        for gf, num, mode, rnd, get, tasks in game_frames(rec):
            first = num if first is None else first
            line = "F %d %d %d %d %08X %.3f %d %d" % (
                gf, mode, struct.unpack("<b", get(PC["chapter"], 1))[0], struct.unpack("<b", get(PC["event"], 1))[0],
                rnd, f32(get(PC["floor_y"], 4)), struct.unpack("<h", get(PC["alive"], 2))[0],
                struct.unpack("<h", get(PC["count"], 2))[0])
            for p in range(2):
                pl = get(PC["players"] + 0xCC * p, 0x40)
                line += " | %d %d %d %d %X" % (i32(pl, 0), i32(pl, 0xC), i32(pl, 8), pl[0x3D], struct.unpack_from("<I", pl, 4)[0])
            bm = get(PC["boss_mode"], 4)
            if bm is not None:
                line += " | B %d %d %d %d" % (i32(bm) if i32(bm) else 0, i32(get(PC["clock0"], 4)) != 0,
                                             i32(get(PC["shown"], 4)), i32(get(PC["clock_ms"], 4)))
            else:
                line += " | B 0 0 0 0"
            # a task can be in a tick's record twice (seen: a fireball trail copy in the tick it was made, pmrest2):
            # once per task, its last record
            last = {}
            for a, body in tasks:
                last[a] = body
            for a, body in last.items():
                if len(body) >= 0x80 and body[0x60] & 1:
                    fmt = " | O %%X %%d %%d %%.%df %%.%df %%.%df" % (digits, digits, digits)
                    line += fmt % (struct.unpack_from("<I", body, 0)[0], struct.unpack_from("<h", body, 0x64)[0],
                                                           struct.unpack_from("<h", body, 0x78)[0], f32(body, 0x28), f32(body, 0x2C), f32(body, 0x30))
            o.write(line + "\n")
    print("%s: game frame 0 = PC tick %s" % (out, first))


def pc_gun(rec, out):
    first, prev_rounds, n = None, None, [0, 0, 0]
    prev_btn = 0
    with open(out, "w") as o:
        for gf, num, mode, rnd, get, tasks in game_frames(rec):
            first = num if first is None else first
            pl = get(PC["players"], 0x40)
            gun = get(PC["gun"], 8)
            if gun is None:
                raise SystemExit("no gun records in %s (record with -Map tools/pcharness/pccap_map.tsv)" % rec)
            x, y = struct.unpack_from("<hh", gun, 4)
            rounds = pl[0x3D]
            btn = get(PC["mouse_buttons"], 8)   # (records made since pccap's second map: the mouse buttons)
            pulled = False
            if btn is not None:
                cur = i32(btn, 4)
                pulled = (cur & 1) != 0 and (prev_btn & 1) == 0
                prev_btn = cur
            # a shot: one round less (Player +4 bit 1, "fired this frame", is cleared again within the tick); a pull
            # that fired nothing (cut-scene skips, shots off): the button's press edge
            if prev_rounds is not None and rounds < prev_rounds:
                o.write("%d %d %d 1\n" % (gf, x, y)); n[0] += 1
            elif prev_rounds is not None and rounds > prev_rounds:   # refilled: the reload pull
                o.write("%d %d %d 2\n" % (gf, x, y)); n[1] += 1
            elif pulled:
                o.write("%d %d %d 1\n" % (gf, x, y)); n[2] += 1
            prev_rounds = rounds
    print("%s: %d shots, %d reloads, %d other pulls (game frame 0 = PC tick %s)" % (out, n[0], n[1], n[2], first))


# runtime tasks whose +0x58 word has bit 0 set without being objects: shot_effect_debris 0x95630 / _flash 0x95140 (own work
# area there), a task being unlinked 0x15110; effect tasks that never set, read or list +0x58 (task_create does not
# clear it, so it is the heap's last word there: the PC's heap differs): impact_effect 0x30C00 (the Magician's hit
# sparks), prop_bullet_impact_mark 0x42F20, prop_collapse_anim 0x3FF70, severed_part_fall 0x69730, hermit_title_task 0x87770, part_class17_anim 0x450B0, part_spark_anim 0x44FC0, prop_anim_once 0x42CF0, hit_spark 0x83030 (pmrest2); the blood marks hostage_shot_effect 0x1DCD0 / researcher_blood_effect 0x1DB30 (0x58-byte tasks: +0x58 is past their block), kind7_falling_piece 0x942D0 / PC 0x00506674, chariot_spark_anim 0x9AE20, chariot_hit_glow 0x9AB90; the player tasks player_in_game_frame 0x50B0 / player_idle 0x55D0 (the PC's players are no tasks; pmch34)
IGNORE = {0x95630, 0x95140, 0x15110, 0x43E628, 0x30C00, 0x42F20, 0x3FF70, 0x69730, 0x87770, 0x450B0, 0x44FC0, 0x42CF0, 0x83030, 0x1DCD0, 0x1DB30, 0x942D0, 0x00506674, 0x9AE20, 0x9AB90, 0x50B0, 0x55D0}   # (PC 0x43E628: its task being unlinked)
# objects whose state word (+0x60) is never written (the heap's last word, as above): compared as state 0 (and hit
# points 0). The kit
# zombies' held / thrown objects: ours held_object_frame 0x81030 / held_object_flight 0x82790, the PC's 0x4E59C4 /
# 0x4E6EAC; a lid's copy falling (its state and hit points too): ours prop_fall_spin 0x42EC0, the PC's 0x4835D8; the iron ball's hit boxes 0x48C50 / 0x48E150 and its follow task 0x47C70 / 0x48D18C (pmrest2) and drop 0x48A50 / 0x48DFA0, chariot_hitbox_task 0x993A0 / 0x516DB8 (pmch34)
STATELESS = {0x81030, 0x4E59C4, 0x82790, 0x4E6EAC, 0x42EC0, 0x4835D8, 0x48C50, 0x48E150, 0x47C70, 0x48D18C, 0x48A50, 0x48DFA0, 0x993A0, 0x516DB8}


# --no-ball-boxes: leave out the iron ball's hit boxes (0x48C50 / PC 0x48E150): the PC's dropped ball makes them
# through its dead owner's freed and reused block (the PC's heap, docs/pcmode.md pmch34), which ours does not copy
DROP = set()


def parse(line):
    parts = line.rstrip("\n").split(" | ")
    h = parts[0].split()
    r = dict(frame=int(h[1]), mode=int(h[2]), chapter=int(h[3]), event=int(h[4]), rand=h[5], floor_y=float(h[6]),
             alive=int(h[7]), count=int(h[8]))
    r["p1"] = parts[1].split()[:4]
    r["p2"] = parts[2].split()[:4]
    r["boss"] = parts[3].split()[1:]
    objs = []
    for p in parts[4:]:
        w = p.split()
        x, y, z = float(w[4]), float(w[5]), float(w[6])
        if int(w[1], 16) in IGNORE or int(w[1], 16) in DROP:
            continue
        if int(w[1], 16) in STATELESS:
            w[2] = w[3] = "0"
        if (x == 0.0 and y == 0.0 and z == 0.0) or not 0 <= int(w[2]) <= 255:
            continue   # task group heads, effects and the like (their +0x58 word is not an object's flags)
        objs.append((int(w[2]), int(w[3]), x, y, z))
    r["objs"] = sorted(objs, key=lambda t: (t[2], t[4], t[3]))
    return r


def diff(pcf, ourf, to=None, tol=0.05, mx=20, offset=0, frm=0, inf_lives=False):
    A = {}
    for line in open(pcf):
        r = parse(line); A[r["frame"]] = r
    B = {}
    for line in open(ourf):   # --offset N: our frame f is the PC's f + N (Boss Mode: the PC's walk-up ticks)
        r = parse(line); r["frame"] += offset; B[r["frame"]] = r
    last = min(max(A), max(B))
    if to is not None:
        last = min(last, to)
    first = {}
    shown = 0
    for fr in range(max(0, offset, frm), last + 1):
        a, b = A.get(fr), B.get(fr)
        if a is None or b is None:
            continue
        diffs = []
        for k in ("mode", "chapter", "event", "rand", "floor_y", "alive", "count", "p1", "p2"):
            x, y = a[k], b[k]
            if inf_lives and k in ("p1", "p2"):   # the capture's -InfLives keeps the PC's lives: not compared
                x, y = x[:1] + x[2:], y[:1] + y[2:]
            if x != y:
                diffs.append((k, a[k], b[k]))
        if a["boss"][0] != "0" or b["boss"][0] != "0":
            if a["boss"][1:3] != b["boss"][1:3]:
                diffs.append(("boss clock on / shown", a["boss"][1:3], b["boss"][1:3]))
        oa, ob = a["objs"], b["objs"]
        if fr - 1 in A and A[fr - 1]["chapter"] != a["chapter"]:
            oa = ob = []   # the PC's record of a chapter change tick is taken after its heap reset (no tasks)
        if len(oa) != len(ob):
            diffs.append(("objects", len(oa), len(ob)))
        else:
            for x, y in zip(oa, ob):
                if x[:2] != y[:2] or any(abs(p - q) > tol for p, q in zip(x[2:], y[2:])):
                    diffs.append(("object", x, y))
                    break
        for d in diffs:
            if d[0] not in first:
                first[d[0]] = (fr, d[1], d[2])
        if diffs and shown < mx:
            shown += 1
            print("frame %d: %s" % (fr, "; ".join("%s PC %s ours %s" % d for d in diffs)))
    print("compared game frames 0..%d" % last)
    if not first:
        print("no difference")
    for k, (fr, x, y) in sorted(first.items(), key=lambda kv: kv[1][0]):
        print("first %-22s frame %6d: PC %s ours %s" % (k, fr, x, y))


if __name__ == "__main__":
    a = sys.argv[1:]
    if len(a) >= 3 and a[0] == "trace":   # [--digits N]: positions with N decimals (ours: HOTD_STATE_DIGITS=N)
        pc_trace(a[1], a[2], int(a[4]) if len(a) >= 5 and a[3] == "--digits" else 2)
    elif len(a) >= 3 and a[0] == "gun":
        pc_gun(a[1], a[2])
    elif len(a) >= 3 and a[0] == "diff":
        kw = dict(to=None, tol=0.05, mx=20, offset=0, frm=0)
        i = 3
        while i < len(a):
            if a[i] == "--to": kw["to"] = int(a[i + 1]); i += 2
            elif a[i] == "--tol": kw["tol"] = float(a[i + 1]); i += 2
            elif a[i] == "--max": kw["mx"] = int(a[i + 1]); i += 2
            elif a[i] == "--offset": kw["offset"] = int(a[i + 1]); i += 2
            elif a[i] == "--from": kw["frm"] = int(a[i + 1]); i += 2
            elif a[i] == "--inf-lives": kw["inf_lives"] = True; i += 1
            elif a[i] == "--no-ball-boxes": DROP.update({0x48C50, 0x48E150}); i += 1
            else: raise SystemExit("unknown option " + a[i])
        diff(a[1], a[2], **kw)
    else:
        print(__doc__)
