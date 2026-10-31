# zones (2026-10-17): "black / grey void through doorways" (user play-test video 4:53-4:59, 5:14, 5:29)

Result: **no zone bug found; nothing changed in the drawing.**

- world_draw_task (0x4B300, Ghidra maincpu.c) draws only `0x5430B0 + draw_zone * 0x32` (one list, 0xFF ends it):
  no neighbour zones. GameView::draw does the same (model lists, fades and turned placements included).
- Zone timeline (new `frame N: draw zone Z` line in headless runs) matches MAME fullgame_us: ours z34 -> z44 at
  10657 / 11115, MAME at t02540 / t02620 (458 vs 460 frames).
- Same moments, arcade source, default route: the researcher hall (ours f11200 vs MAME snap 0131), the monkey window
  room (t02760 snapshot), the red-carpet stair hall (t02860) match MAME in geometry and darkness.
- The user's route (`--route 1:4=1`: events 4 -> 7 -> 8 -> 19, zones 35 -> 36 -> 37 -> 44) reproduces the video frames
  1:1 (black area behind the researcher, dark doorways, the black doorway before the Sophie room). Drawing with zones
  off (`HOTD_NO_ZONES=1`) gives the same images there: nothing is culled, the areas are unlit / dark in the
  original's lights too. (No MAME capture of this route exists; MAME's run took events 6 / 19.)
- Real but test-only: `--event E` starts with draw zone 0 (world_init) until E's first BGTB (event 15: 1073 frames,
  event 5: 666, event 7: 303), so only unzoned pieces are drawn on the clear colour; with `--models arcade` an
  `--event` start drew black. The original's demo / chapter starts behave the same way (zone 0 until BGTB).
- Still open (render worker's area, not zones): PC world items get no zone light (`it.stage == stage_` only), so the
  PC source renders flat-lit in headless shots.

Test aids added (src/main.cpp, src/game/view.cpp): draw-zone log line; `HOTD_KEEP_LIVES=1` (player 1 never runs out:
full-route headless runs); `HOTD_NO_ZONES=1` (draw every placement, to check culling).

## Round 2 (coordinator): MAME on the user's route; PC light

- MAME run `run_mame_dump.ps1` with HOTD_ROUTE `1:0=1,1:2=0,1:3=0,1:4=1,1:7=0,1:8=0`, -InfiniteLives, -video none +
  Lua snapshots, dumps from 283 s (private\mame_dumps\zones_r4, compressed): events 7 -> 8, zones 27, 28, 35, 36, 37, 44,
  the same route as ours with `--route 1:4=1`. MAME z37 at 285.5 s, z44 at 289.0 s (3.5 s; ours 9459 -> 9659 = 200 frames).
- Researcher moment (video 4:53-4:59), ours vs MAME (zones_r4\compare_ours_top_mame_bottom.png): **MAME is black in the
  same places** (right of / behind the researcher, under the balcony, the doorways). The voids are the original's look.
- PC world without the zone light: faithful. THOTD.EXE draws textured polygons with diffuse 0xFFFFFF (d3d_tri_textured
  0x577870, alpha only from bit 0x80) and has no light setup; the renderer's PC path already follows it. Nothing to change
  from the PC files.
