# Proofs worker 10 (weekend round 2, 2026-10-16 night)

Branch `claude/proofs-10`, build folder `build-proofs10` (as proofs-9: `-DHOTD_FETCH_SDL3=ON
-DFETCHCONTENT_SOURCE_DIR_SDL3=<main checkout>/build/_deps/sdl3-src`). Results and command lines in
[proof.md](../proof.md) ("Continue (proofs-10)").

## Tools

- `run_capture.ps1 -Continue n` (mame_dump.lua `HOTD_CONTINUE`): player 1 continues up to n times (2 s after its
  state 0x51F284 turns 4: the game's coins, then 1P start). With `-TwoPlayers -Mortal -Shoot 0` both players run out
  of lives, player 1 continues and player 2 counts down on its own (player_continue_start).
- `capture_lists.py`: `HOTD_ROOT=<main checkout>` reads the indexes / ROM from the main checkout's private\ (a worktree
  has none; no junction).

## Batch 1: continue

p10_cont2 (compressed, ~150 MB on disk): player_st1_continue, player_continue_begin, player_continue_start,
continue_screen_draw: 10 captures, 0 differ. Fix: player_continue_frame calls `player_continue_nop` 0x94F20 first
(new empty virtual stub, a boundary in hotd_proof). UNVERIFIED 62 -> 58. hud_popup_clear not reached.

## Batch 2: one player's game over, sweep

- p10_gover (the same 2-player continue game; compressed): player_body_place, player_body_camdr, player_body_hold
  (player_st6_game_over: player 2's countdown runs out while player 1 plays on): 22 captures, 0 differ. UNVERIFIED
  58 -> 55.
- p10_sweep (a fresh auto-aim game, 1100 s, to Chapter 4 event 4; deleted): none of the 37 remaining UNVERIFIED
  arcade routines with a capture case was called once (the camera-table routines, camera_chapter_reset,
  kind138_prop_group, the hostage scenarios 1 / 6 and shot slide / fall, the scene bodies 00 / 05 / 07, the gift,
  set_motion2_blend, object_draw_model, chariot_act17, part_subtree_free).

## Batch 3: researcher gift

p10_gift (`-Route "1:4" -AimSkip 6dbd0`, 600 s; compressed): researcher_gift_spawn, researcher_reached_camera,
gift_effect: 21 captures, 0 differ after a harness fix (src/proof/bounds/researchers.inc: researcher_gift_spawn was
always a boundary). UNVERIFIED 55 -> 52.

## Batch 4: hostage scenario 1 freed

p10_s01 (a fresh auto-aim game, 300 s, `-AimSkip` the hostage routines; compressed): hostage_s01_run,
hostage_s01_run_steps: 20 captures, 0 differ. UNVERIFIED 52 -> 50.

## Not reached (no progress, left)

- Hostage scenario 6 (`hostage_s06_*`, `hostage_thank_leave_step`: state 9 = hanging) and `hostage_shot_slide /
  _fall` (state 20): p10_s06 (`-Route "1:3,1:9,1:11=0"`, 900 s: Chapter 1 went 11 -> 17 -> 13 -> 19, no event 15;
  deleted) and p10_demo (new `run_capture.ps1 -Attract`: no coin, the attract demos, which include Chapter 1 event
  15; 400 s, nothing called; kept compressed, ~120 MB, can be deleted).
- Everything p10_sweep listed (camera-table routines, kind138_prop_group, scene bodies 00 / 05 / 07, ...).
