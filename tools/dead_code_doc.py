#!/usr/bin/env python3
"""
dead_code_doc.py - sorts the arcade program's dead-code roots (`unused_<address>` in tools/ghidra/names.tsv) into
debug / cut feature / leftover / not code / fragment, writes docs/dead_code.md and puts the category and a short note
into each root's names.tsv comment. Evidence: the reading notes in NOTES below (from the C / disassembly, 2026-10-13);
roots without a note get a rule by size, region and what they call (marked "by rule", lower confidence).
Dead code is kept: never removed from names.tsv, Ghidra or the docs.

  py tools/dead_code_doc.py          (run tools/dead_code_sort.py's inputs first: the Ghidra export in private\\decomp)
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
NAMES = os.path.join(HERE, "ghidra", "names.tsv")
DOC = os.path.join(ROOT, "docs", "dead_code.md")

# address -> (category, note); read by hand
NOTES = {
    0x4ABD0: ("debug", "debug readout: window_printf of three 'CC%.3d' columns"),
    0x27780: ("debug", "an older gun-adjustment page ('DEFAULT PLAYER2 GUN ADJUSTMENT')"),
    0x14870: ("debug", "prints 'memory allocation error !!' on the text layer"),
    0x2F8A0: ("debug", "CPU fault report ('Constraint Faults')"),
    0x5C930: ("debug", "CPU fault report ('Constraint Faults')"),
    0x6D800: ("debug", "ROM check page ('IC113 / IC114 / IC115 CHECK')"),
    0x2EF10: ("debug", "test object (scale 1, yaw 0x4000); its routine prints its position and angles ('enZ', 'xa', 'ya', 'za')"),
    0x2DA20: ("debug", "test scene: Sam (creature type 11) at (0, 0, -103.2) facing away, motions 0x1DD then 0x1F1"),
    0x2E280: ("leftover", "spawns an enemy at the camera position (coprocessor transform), counts it in enemy_count / enemies_alive"),
    0x2DD20: ("leftover", "enemy set-up that counts the enemy (enemy_count / enemies_alive)"),
    0x6DAE0: ("leftover", "an older set-up of object kind 64 (body, creature type, enemy slot, kind64_frame)"),
    0x8A420: ("leftover", "an older set-up of object kind 65 (with enemy slot)"),
    0x8A490: ("leftover", "an older set-up of object kind 65 (without enemy slot)"),
    0x832D0: ("leftover", "an older bat set-up (bat_frame)"),
    0x77070: ("leftover", "enemy removal: frees its parts, attack slot and extras, uncounts it, ends the task"),
    0x76B30: ("leftover", "byte-identical copy of kind11_leap_move"),
    0x74774: ("fragment", "not a routine: the tail of a Magician fireball routine (uses registers it never sets)"),
    0x19C80: ("leftover", "screen effect: the whole palette to grey (palette_grey_average, colour RAM 0x1802000)"),
    0x18AD0: ("leftover", "screen-fade task launcher (fade level 0x517CA4 clamped to 0..0x20)"),
    0x195F0: ("leftover", "screen-fade task launcher (as 0x18AD0)"),
    0x19000: ("leftover", "screen-fade task launcher (as 0x18AD0)"),
    0x19C50: ("leftover", "screen-fade task launcher"),
    0x19FC0: ("leftover", "screen-fade task launcher"),
    0x19670: ("leftover", "screen-fade task launcher"),
    0x19980: ("leftover", "white fade task launcher (palette_fade_white)"),
    0x87250: ("open", "draws a model with the geometry engine; the string 'CONTINUE' is referenced"),
    0x30B50: ("leftover", "effect spawner: object at (x - 1.5, floor, z + 21), random angle, 180 frames"),
    0x30990: ("leftover", "effect spawner with a sound cue, on the floor"),
    0x9BF60: ("leftover", "score / rank sprite display (as kind 176's score draw)"),
    0x46D10: ("debug", "button-5 handler reading game_mode / gun_test_mode / scene_state"),
    0x98A0: ("debug", "sound command when the test menu opens"),
    0x46340: ("leftover", "an older floor / scene-quad probe (scene_quads counts, floor_y)"),
    0x27E20: ("leftover", "fills a table at 0x53E0E0 with crand() values"),
    0x250D0: ("leftover", "library: a rotation by an angle in degrees with double-precision sinr / cosr (the game uses the coprocessor)"),
    0x34AF0: ("leftover", "task launcher (task size 0x58)"),
    0x444F0: ("leftover", "task launcher (task size 0x60) with a short busy wait"),
    0x243C0: ("leftover", "queues a word pair for a vblank job (system API)"),
    0x4B750: ("debug", "reads the draw zones / placement tables (a zone display)"),
}

CATS = ["cut feature", "debug", "leftover", "fragment", "not code", "open"]


def rule(addr, size, callees, strings):
    if addr >= 0x100000:
        return "not code", "a 'function' Ghidra made in the data area (%d bytes)" % size
    if addr % 16:
        return "fragment", "address not 16-aligned: probably part of another routine (Ghidra split); by rule"
    if size <= 8:
        return "leftover", "empty stub (%d bytes: return only); by rule" % size
    if 0x13000 <= addr < 0x18800:
        return "leftover", "system library routine nobody calls%s; by rule" % ((" (" + callees + ")") if callees else "")
    if 0x49000 <= addr < 0x4C000:
        return "debug", "in the debug text / window block 0x49xxx-0x4Bxxx%s; by rule" % ((" (" + callees + ")") if callees else "")
    if callees:
        return "leftover", "calls %s; by rule" % callees
    return "leftover", "small helper (%d bytes), no calls; by rule" % size


def main():
    out = subprocess.run([sys.executable, os.path.join(HERE, "dead_code_sort.py")], capture_output=True, text=True,
                         check=True).stdout.splitlines()
    rows = []
    for line in out[1:]:
        c = line.split("\t")
        a, size = int(c[0], 16), int(c[1])
        if a in NOTES:
            cat, note = NOTES[a]
        else:
            cat, note = rule(a, size, c[5], c[7])
        rows.append((a, size, int(c[2]), cat, note))
    # names.tsv comments
    lines = open(NAMES, encoding="utf-8").read().split("\n")
    by = {a: (cat, note) for a, _, _, cat, note in rows}
    for i, line in enumerate(lines):
        c = line.split("\t")
        if len(c) >= 4 and c[0] == "func" and re.fullmatch(r"unused_[0-9a-f]+", c[3]):
            a = int(c[2], 16)
            if a in by:
                cat, note = by[a]
                while len(c) < 5:
                    c.append("")
                c[4] = "dead code (%s): %s" % (cat, note)
                lines[i] = "\t".join(c)
    open(NAMES, "w", encoding="utf-8", newline="\n").write("\n".join(lines))
    # the doc
    count = {k: sum(1 for r in rows if r[3] == k) for k in CATS}
    d = ["# Dead code in the arcade program", "",
         "Routines nothing refers to (no call, no code pointer, no table entry: `tools/unnamed_clusters.py`), named",
         "`unused_<address>` with their helpers `unused_<address>_sub / _p / _n`. They are **kept**: named, labelled and",
         "documented, never deleted from names.tsv, Ghidra or these docs; ported and proven only when needed (e.g. for",
         "the cut-content mod). Generated by `tools/dead_code_doc.py` (sorting rules and reading notes in it); the",
         "category and note are also the names.tsv comment of each root.", "",
         "Categories: **debug** (developer displays, test objects, fault and ROM-check pages), **cut feature** (part of a",
         "feature the final game lost), **leftover** (older versions of used routines, unused library / system API,",
         "empty stubs), **fragment** (not a routine start: a piece of another routine that Ghidra split off), **not",
         "code** (Ghidra 'functions' in the data area), **open** (not sorted yet). Notes marked \"by rule\" were sorted by",
         "size, address region and calls without reading the code.", "",
         "| Category | Roots |", "|---|---|"]
    d += ["| %s | %d |" % (k, count[k]) for k in CATS]
    d += ["", "## Cut content found in the dead code", "",
          "None of the dead routines refers to a cut creature (types 5 MrG, 10 syndy, 12 debu, 39 lubin, 52 disiprin,",
          "56 neozom, 66 baba, 75 hiru): those creatures have data in the ROM but no code at all. The cut features",
          "with code are listed on the [cut-content mod list](cut_content.md#cut-content-mod-list).", "",
          "## All roots", "", "| Address | Bytes | With helpers | Category | What it seems to do |", "|---|---|---|---|---|"]
    for a, size, cl, cat, note in sorted(rows, key=lambda r: (CATS.index(r[3]), r[0])):
        d.append("| 0x%X | %d | %d | %s | %s |" % (a, size, cl, cat, note.replace("|", "/")))
    open(DOC, "w", encoding="utf-8", newline="\n").write("\n".join(d) + "\n")
    print("dead_code.md: %d roots; %s" % (len(rows), ", ".join("%s %d" % (k, count[k]) for k in CATS)))


if __name__ == "__main__":
    main()
