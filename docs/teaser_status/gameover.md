# gameover (2026-10-16): mode 5's 3D parts

Branch claude/gameover. All UNVERIFIED (teaser); checked by the `die` checkpoints (`--bot 3 --bot-gameovers 1`, both
model sources) against the MAME life_scan dumps (snaps 0087-0094).

1. **Game-over scene** (sub-mode 1, src/game/flow.cpp): game_over_start 0x97200's heap reset, player_char_models_set,
   camera_pos / heading cleared, task group 0xCA790 (game_over_state0_camera 0x97740, input, InitSetPlayerTask -> state
   6 -> player_st6_game_over, shake) in 0x543640; game_over_scene 0x97340 runs it. game_over_camera 0x97490 from the
   disassembly (Ghidra lost the floats): eye (10, 5, 20), look-at (0, 14, 6) from the camera, the look-at height easing
   to 0 (x 0.9756 + 0.0244 a frame) once 0x520700 < 276. player_st6_game_over is now the full form (collapse motion
   table 0xA86E8, player_body_place, body routine player_body_camdr; in mode 5 the wait runs only the body routine).
   Shows: the fallen body's close-up on black (MAME snaps 0087-0089: the same framing). Ours at c16 (+60) is still
   mid-collapse where MAME's +44 snap already lies: snap timing or the motion's speed, not checked.
2. **Route map 3D** (sub-modes 2 / 3): game_over_map_start's heap reset and group 0xC5440 (game_over_camera_start
   0x5CCA0 + camera task 0x5D4F0, game_over_route_start 0x5D4D0 -> the existing route task, game_over_walkers_start
   0x5CE70) in 0x5433E0. game_over_walker_task 0x5CCD0 from the disassembly: the body at the scroll point (0x543400 /
   0x543402) through stored matrix 1 (copro 0x74), x by table 0xC5460, z 150, turned by 0x543404, collapse at
   0x543405. Shows (arcade models): the walker on the trail, fallen at the route's end (as MAME snap 0090).
3. Runtime::frame clears the draw list and reads the camera in mode 5 too; the view draws no world in mode 5 (only the
   game's draws) and nothing 3D from sub-mode 4 (GAME OVER).

Left: the walker's colour (darker / greenish than MAME's: light?), the scene's timing vs MAME frame by frame, the
GAME OVER group 0xCA7A8 (camera_globals_reset only), palette_fade_finish / sound_send3 0xBE5000 of game_over_start.
PC models: the map itself has no PC picture (polish 6), so only the walker shows on black.

## gameover-2 (2026-10-16, branch claude/gameover-2)

1. game_over_text 0x973A0: the task group 0xCA7A8 (start table: 0x97770 camera_globals_reset) in 0x543640.
2. game_over_start / game_over_text: palette_fade_finish 0x18680 when a fade runs (0x51F702; the runtime has no fade,
   so it only keeps the globals), sound_queue_reset, sound_send3 0xA00002 / 0xBE5000 (no PC sound), cue 0xA01003.
3. Fall timing: no difference. MAME's dumps carry the scene timer 0x520700: t00870 (snap 0086) = +44 (timer 316) shows
   the body mid-collapse, arm up; t00880 (snap 0087) = +102 lies down. Ours at +44 / +102 (`die`, arcade models) shows
   the same two poses. The sheet's mode start (midpoint between 1-second dumps) put c16 (+60) on snap 0087;
   tools\checkpoint_sheet.py now takes the game-over scene's start from the timer (c16 -> snap 0086).
   Seen: our background is a dark navy, MAME's pure black.
4. Walker light (not changed): game_over_map 0x5CBF0 / game_over_map_start / the walker task send no light (only
   geo_frame_begin, geo_view_setup 0x1E10: viewport / projection), so the geometrizer keeps the last light of the
   chapter, in MAME too. Ours (HOTD_LIST_DRAWN_AT on the `die` run): light (0.71, -0.71, 0) world, texture area 1 set
   0's diffuse / ambient (stale from the chapter: the bot dies in event 3, life_scan elsewhere), so the dark / greenish
   walker may be the death place's light, not a port error. Next: read the light record / texture area MAME holds at
   t00900 (no copro dump in life_scan) or compare a run that dies where life_scan does.
