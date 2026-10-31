# Proofs worker 2 (overnight 2026-10-16)

Branch `claude/proofs-2`. The next most-called UNVERIFIED routines of the Chapter 1 play-through (ranked by a call
count of every runtime override in a headless `hotd --frames 20000 --autofire` run) moved from src/game into the port
as `Game<P>` members (`src/port/hit_effects.cpp`, `damage_types.cpp`, `body_hooks.cpp`, `scripted_extra.cpp`,
`enemy_helpers.cpp`, `combat_support.cpp`, `part_tree.cpp`, `player_start.cpp`, `prop_groups.cpp`; declarations
`src/port/decl/support_b.h`; proof cases `src/proof/cases/support_b_arcade.inc`, which the runtime's dispatch also
includes) and capture-proven against MAME: results and command lines in [proof.md](../proof.md) ("Support routines
(2)"). The runtime also runs the port's motion family now (L3, proven in l3_cr1) instead of its own copies in
src/game/creature.cpp.

- Headless checks after every batch: the free-route Chapter 1 run (16,711 frames) and routes 1:4, 1:0+1:3, 1:5, 1:3+1:9
  give the same frames, shots and boundary report as before the move.
- Fixed from the captures: velocity_for_arc_speed (the flight time from the floor distance to -> from, coprocessor 0x1F
  with y 0; the arc's vertical speed in the original's float order). From the disassembly while moving: enemy_dying's
  corpse frame zero-extended; kind102_enemy_walk_in's motion word not truncated; player_start_check / player_waiting_frame
  / player_st9_waiting as the original (player_start_pressed, the attract-demo / lesson starts, the prompt's player byte).
- Proven: about 140 routines in about 4,900 captures, 0 differ (the list and the command lines: proof.md). UNVERIFIED left
  (not reached): enemy_act6_frame / _bite (guard action 6; route 1:5 with the guards skipped by the auto-aim did not run
  it), hostage_shot_effect (its body proven as researcher_blood_effect), kind138_prop_group (ends in task_exit: no exit
  record), set_motion_at_frame, set_motion2_blend, object_draw_model, player scenes 00 turn / hold, 04 p2, 05 p_p /
  p_p_p, 06 p2 / p2_p, 07 p1_p_p_p, player_body_place / _camdr / _hold. `py tools/unverified_list.py`: 233 -> 134
  routines.
- Old captures re-checked with the moved routines: 0 differ as before; l3_cr1 / k64_full2 / kinds_a2 / l5_full1 keep
  their earlier differing captures (same captures as with the previous proofs build).
- Not moved: the drawing half of src/game/creature.cpp (object_draw, body_draw, part_draw, part_hit_mark: the port's
  versions add the shadow pass and fade the runtime does not draw; render's area), the tumbling boxes (tumble_bounce
  uses T1's stand-in for coprocessor 0x85, not bit-exact), players.cpp's PlStartInit / continue / join (rarely called;
  they need the play-time bookkeeping and HUD routines as boundaries).
- Other workers' files touched (minimal): src/game/runtime.h / runtime.cpp (moved routines' declarations and switch
  cases removed), t1_bounds.cpp (moved routines removed; logging wrappers kept for the blood marks, hit_screen_shake keeps
  the hud order fix), enemy_support.cpp (only player_hit's log wrapper left), players.cpp (player_start_check's log
  wrapper), creature.cpp (the motion family removed, body_hook_eye_beam / _target_beam calls).
- Capture notes: the Ghidra index size of a routine can end just before its last `ret` (Zombie_Dead_frame: pass
  `-Funcs 76e50:16c`); routines missing from the index (kind 96's frames 0x7EC30 / 0x7EC60) need explicit sizes too;
  object_draw's part hooks (9ce60, 37d80, 87550, 689e0, 68b50, 69540) still go into `-Boundary` by hand (the port's
  object_draw calls them through call_routine).
