# Proofs worker 3 (overnight 2026-10-16)

Branch `claude/proofs-3`. The Chapter 1 routines proofs-2 left UNVERIFIED because no capture reached them, captured
in fresh auto-aim games (routes, two players) against MAME: results and command lines in [proof.md](../proof.md)
("Support routines (3)").

- Proven, 0 differ, nothing to fix: `enemy_act6_attack_researcher` / `enemy_act6_bite` / `enemy_act6_frame` (guard
  action 6, event 15: `-Route "1:3,1:9,1:11=0" -AimSkip "3e030,543b0,56360,56320"`), `hostage_shot_effect` (a hostage
  shot in the default auto-aim game and with `-AimPrefer <hostage states>`), `set_motion_at_frame` (one call per game,
  from scene_zombie_walk_frame), the two-player scene bodies `player_scene04_fall_backside_p2`,
  `player_scene06_reload_shoot_p2` / `_p2_p` (`-TwoPlayers -Route 1:5`).
- New capture option: `run_capture.ps1 -AimPrefer "<routines>"` (mame_dump.lua `HOTD_AIM_PREFER`): the auto-aim aims
  at objects running those routines first when one is on screen, else the nearest object (an aim-only list stalls the
  events: the zombies are never shot).
- Route note: in MAME `-Route 1:5` goes 12 -> 17 -> 13 (not 15: the runtime's default route differs); `1:5,1:12`
  did not run action 6 either. Event 15 with action 6 to its end: the 1:11=0 route with the guard skipped.
- Still UNVERIFIED (not reached): `set_motion2_blend` (only caller kind32_walk_turn), `object_draw_model` (draw mode
  2: the kind-32 held objects; not called in the full-game recording either: p3_full_draw), scenes 00 turn / hold, 05 p_p /
  p_p_p, 07 p1_p_p_p, `player_body_place / _camdr / _hold`, hostage_s01_run(_steps) (scenario 1 never freed).
