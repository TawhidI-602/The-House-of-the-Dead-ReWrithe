# Proofs worker (overnight 2026-10-16)

Branch `claude/proofs`. The T1 boundaries the play-through calls most moved from `src/game/t1_bounds.cpp` (Runtime
overrides) into the port as `Game<P>` members (`src/port/attack_slots.cpp`, `enemy_scripted.cpp`, `floor_motion.cpp`,
`decl/support.h`; proof cases `src/proof/cases/support_arcade.inc`, which the runtime's dispatch also includes) and
capture-proven against MAME: results and command lines in [proof.md](../proof.md) ("Support routines").

- Proven (0 differ): attack_slots_update / _sort, attack_slot_take_waiting / _take_wide / _front_take /
  _nearest_object, enemy_attack_slot_take / _free, enemy_scripted_hit / _dead / _dead_fall, enemy_wait_camera_stop,
  Zombie_Dead, floor_probe, ground_height, jump_arc_set / _step / _step_b / _velocity / _velocity_b, obj_move_heading.
- Still UNVERIFIED: Zombie_Dead_frame past its first frame (not reached in the recordings or fresh games tried).
- Other workers' files touched (minimal): `src/game/runtime.h` (the moved routines' declarations removed),
  `src/game/runtime.cpp` (their six dispatch lines removed: support_arcade.inc dispatches them now),
  `src/game/enemy_support.cpp` (enemy_attack_slot_take / _free removed).
- Proof overrides (`src/proof/bounds/*.inc`) replay these routines only where an older capture recorded them as
  boundaries (`P::pc || recorded_next`); existing captures re-checked unchanged (guard_actions_full2, frog_full,
  k70_full4, cam_a1, dog_full2: 0 differ; kinds_a2 / k64_full2 differ as on main).
- Capture note: object_draw's part hooks (9ce60, 37d80, 87550, ...) must be in `-Boundary` by hand.
