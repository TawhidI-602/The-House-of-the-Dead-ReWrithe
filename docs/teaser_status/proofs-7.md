# Proofs worker 7 (weekend round, 2026-10-16)

Branch `claude/proofs-7`, build folder `build-proofs7`. The runtime routines a normal play-through runs every frame
or every game (the heap and the task creation, mode 4's game flow, the chapter's resets and tasks, the continue and
join states, the chapter clear, the draws) moved from src/game into the port and captured against MAME: results and
command lines in [proof.md](../proof.md) ("Runtime core (proofs-7)").

- New port files: `src/port/tasks.cpp` (heap, task creation), `game_flow.cpp` (mode4_game, game_play, game_frame,
  the continue sub-modes and states, the chapter end), `chapter_tasks.cpp` (shot_lists_clear, chapter_reset_53e1b0,
  chapter_counters_reset, attack_slots_task, depth_lists_task), `chapter_clear.cpp` (kind 176), `support_c.cpp`
  (task_create_child_alloc, spin_axis_from_velocity, kind 182, draw mode 1, tumble, body drips), `player_join.cpp`
  (PlStartInit, the start states, hud_popup_clear, the camera-drive set-ups). Proof cases: `src/proof/cases/tasks_arcade.inc`.
- The runtime draws through the port's object_draw / body_draw / part_draw (proven in l3_cr1); its view keeps the
  skins (skin_draw override) and the shadow pass (0x77 with matrix 3 inside object_draw / body_draw). Headless draw
  lists compared with the new `HOTD_DRAW_HASH=1|2` (a hash per frame; 2 leaves the fade out): identical but the fade.
- Fixes (differences from the original found on the way): game_submode / 0x51F4A8 written as 32 bits (the
  neighbouring half-words cleared); the play-time counters counted in the continue sub-modes too; fading bodies drawn
  opaque (body_draw's draw_fade_begin missing); the coprocessor's focal length (0x7D) never sent by game_frame (the
  sky's first 62 frames); kind 176 cleared the 16-bit grey flag with a 32-bit write instead of palette_restore_vblank;
  object_draw_model drew the model without its translation / rotation (and no shadow pass); player_st1_continue left
  the popup sprite and the play-time books out; player_st3_join's debug-mode body re-init.
- Tools: `run_capture.ps1 -IfNe "a:b,..."` (capture only when two words differ: depth_lists_task's list pointers) and
  `-Root <checkout>` (a worktree runs its own tools on the main checkout's private\).
- Captures (11 folders in private\proof\p7_*, all compressed: about 14 GB written, 4.1 GB on disk): 0 differ.
  UNVERIFIED count 114 -> 74 (`py tools/unverified_list.py`).
- Left (UNVERIFIED): the task loop (longjmp), letterbox_task (its bar sprites would double the HUD's), world_draw_task
  (the engine draws the world), body_part_draw_hook 0x9CE60 (6 KB, the runtime's is partial), player_paused,
  apply_settings / chapter_start (the launcher's settings, s1's), tumble_bounce (coprocessor 0x85 not modelled), the
  continue-with-a-coin and join paths (no capture game reaches them), object_draw_model (not reached).
- Merged into main's title / title-2 (branch claude/merge7): Runtime::game_mode_frame runs title-2's sub-modes 9 / 10
  (the name entry) itself and every other sub-mode through the port's mode4_game; Runtime::game_chapter_end_wait keeps
  title-2's end (game_ending_rank with the title or --name-entry, else chapterDone) before the port's routine.
  Checked: arcade / PC headless clears, --input-test 417 checks 0 failed, --title attract -> Chapter 1 -> clear ->
  name entry "TAW" -> ranking -> attract, game over -> ranking, p7 captures 0 differ (p7_flow_fin's known 1),
  checkpoints c01 / c03 / c06 / c08 identical to main's.
