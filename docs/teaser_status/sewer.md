# sewer: the attract's sewer zombie and the how-to-play torches (2026-10-17, branch claude/sewer)

Build folder `build-sewer`. MAME: private\captures\attract2_flow (snaps every 0.5 s from 28 s: snap N = 28 + N/2 s)
and private\captures\sewer_torch (tools\mame_flow_trace.lua, snaps 60-65 s, every 1 s).

## 1. The sewer zombie (scene 6): not a bug, drawn as in MAME

Frame alignment: MAME's scene 6 starts at frame 1675 (FDC10) and the spawn op FDCA0 runs at +8; ours (arcade
models, `--title`) at 1501 and +5 (MAME's 3 lag frames). MAME snap 6 (31.0 s = frame 1783) therefore equals our
frame 1606-1607: side by side the same picture (the brown mummy's leg and arm at the right edge, the green zombie under
the arch, the lamp). The mummy (both copies of record 0xFE370, at -74 / 148, drawn twice like the ROM's list) stays in
the draw list (HOTD_LIST_DRAWN_AT 1600 / 1606) and slides out of the right edge at 1609 (4:3) / 1613 (16:9) as the
camera passes it (camera z 128 -> 143 between 1600 and 1610). PC models: the same (PC run has no title, so -361
frames). The user's video (0:39.2) shows it too; the earlier "not drawn" came from comparing frames a few apart.

## 2. The how-to-play torches' lower half (fixed)

mode8_lives_task (0x26FB0) draws, after the flames, the torch base 0x14 clipped to its lower row (sprite_draw_clipped,
x from 0xAB2E4 + 4i, rows 0x2C only: the lamps); ours skipped it ("the flicker clip is not drawn"). Also the flames'
phases come from the table 0xAB2F0 (0, 5, 3, 0, ...), not i * 3. Read from the disassembly (tools\i960dis.py). Arcade
and PC sprites both show the lamps now; arcade torches match MAME's snap at 62 s cell for cell (flame phases too).
