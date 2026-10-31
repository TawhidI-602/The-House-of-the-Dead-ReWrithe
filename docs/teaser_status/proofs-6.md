# Proofs worker 6 (overnight 2026-10-16, last window)

Branch `claude/proofs-6`, build folder `build-proofs6`. The UNVERIFIED teaser routines in src/port (L3 player block,
L5 hostages / scene objects, L7 Chariot / Sophie) captured in fresh auto-aim Chapter 1 games against MAME: results and
command lines in [proof.md](../proof.md) ("Teaser leftovers (proofs-6)").

- Proven, 0 differ, nothing to fix: `player_hit_lose_life` 0x6520 (33 captures: a hostage shot by a player),
  `hostage_s03_dead` 0x5C460 (129), `sophie_wait_end` 0x963A0 (6), `chariot_attack_r` 0x98C80 (150),
  `chariot_act2_land` 0x98E70 (150), `chariot_act12_turn` 0x98DB0 (3 with `-Route 1:5`, 2 in a plain auto-aim game).
- `chariot_start_direct` 0x97C80: 1 capture, 0 differ (p6_cstart2, with the part draw hooks 0x9A040 / 0x9A280 as boundaries; without them the first capture differed only there).
- New `run_capture.ps1 -FiniteLives`: the life counter is not kept at 3 (continue / game-over paths).
- Still UNVERIFIED (not reached in four 1100 s games): `chariot_act17_wait_combat`, `sophie_lying_d`,
  `sophie_wait_frame37`, `hostage_s01_run(_steps)`, `hostage_s06_hanging / _wait / _thank`, `hostage_s03_thrown`,
  `hostage_thank_leave_step`, `hostage_shot_slide / _fall`, `kind178_scene_skip_frame`; the task_exit routines
  (kind162_end, kind164_door_burst, kind178_scene_skip_end, kind138_prop_group) cannot be captured.
- Not touched (proofs-5): game-over routines, sky_task, palette_anim_copy, sprites.cpp.
