# Proofs worker 8 (weekend round, 2026-10-16 evening)

Branch `claude/proofs-8`, build folder `build-proofs8`. Polish round 2's two small items, then the UNVERIFIED routines
a capture can reach. Results and command lines in [proof.md](../proof.md) ("Blow-off, shadow, leftovers (proofs-8)").

## Polish round 2 items

- **part_blow_off / shadow_update stubs** (polish.md round 2, "Remaining"): ported from the disassembly into
  `src/port/blow_off.cpp`: `part_blow_off` 0x69550 (a zombie's head blown off: the part's model by hit type and step,
  the subtree detached, the head piece thrown back along the shooter's view), its falling piece (`severed_part_init`
  0x39830, the task `severed_part_fall` 0x69730 with the bounces, `severed_part_draw` 0x38B10, `part_node_draw_rec`
  0x38A50, `severed_part_free` 0x38FA0, `part_subtree_free` 0x390F0), the blood splat on each bounce
  (`floor_blood_splat` 0x43920, task `floor_blood_splat_task` 0x45230: model 0x2C7 for 120 frames), and
  `shadow_update` 0x6C9E0 (floor_probe under the object, then `shadow_projection` 0x6C490: the projection onto the
  probed floor's plane along the shadow light, stored matrix 3 + its copy 0x51FDF0). The runtime's
  `Runtime::shadow_update` keeps the view's shadow floor height (0x51FE18). Before: no head ever flew off (the head
  just changed model) and the kit zombies / Molbs drew their shadow with the last shadow_setup's plane.
  `eyeball_pop_spawn` 0x43B90 stays a stub (its task 0x43970 not ported).
- **HOLD YOUR FIRE! blink phase** (round 2 #8): `hud_sprite107_blink` (capture-proven) shows the caption while the
  frame counter 0x51F428 % 60 < 45; the counter runs from power-on. MAME fullgame_us snaps 0012 / 0015 / 0016 are
  dump lines 13 / 16 / 17 (snap 0000 = line 1): counters 1319 / 1664 / 1779 = off / on / on, exactly as the routine
  gives. Ours started a chapter with the counter at 0 (1015 frames behind the reference: "our frame f = its counter
  f + 1015"), so the phase was 55 frames off. Fix (src/game/chapter.cpp chapter_start, s1's file, 3 lines): a game
  started straight into a chapter (mode 4, counter still 0) starts the counter at 1015. Headless check: the counter
  is frame + 1015 from the first frame (HOTD_PALANIM_LOG).

## Tools

- `tools/mame_capture.lua`: a captured routine that calls task_exit itself (0x150D0 in -Boundary) now ends its
  capture there (a task_exit boundary record that changed nothing, the state at its entry as the exit record) instead
  of being dropped: the routines that end in task_exit can be proven (the port calls the task_exit stub and returns).
- `hotd_proof`: a case may return its value (`a.setResult(...)`), compared with the original's g0 at its return
  (`colour_blend32`).
- New proof cases: the camera routines reached only through the camera tables (0x2B180, 0x2BAA0, 0x2C060, 0x2CA30,
  0x2D6F0, 0x2D860, 0x2DA00, 0x6CB30, 0x6CC20, 0x6CC40), colour_blend32 0x23CC0, the blow-off / shadow routines.

## Captures

Three capture folders in private\proof (p8_blow, p8_sweep1, p8_sweep2p; about 7 GB written, compressed):

| capture | captures | differ | proven |
|---|---|---|---|
| p8_blow | 232 | 0 | part_blow_off, the falling piece (init, task, draw, node draw, free), the blood splat (spawn, task), shadow_update, shadow_projection |
| p8_sweep1 | 60 | 0 (+10 of 0x9C250: capture size) | hangedman_flyby_init / _start, kind162_end, kind164_door_burst, kind178_scene_skip_end, sophie_lying_d, sophie_wait_frame37, player_st2_rejoin, colour_blend32 |
| p8_sweep2p (2 players, route 1:4 / 1:7) | 65 | 1 (player_st3_join) | kind178_scene_skip_frame |

UNVERIFIED count 74 -> 65 (`py tools/unverified_list.py`; part_subtree_free newly marked, not reached).
Not done: player_st3_join's differing capture (the original makes a 0x7D call inside PlStartInit's row 3: not
found); the camera routines only reached through the camera tables (cases added, none called in these games);
eyeball_pop_spawn 0x43B90 (still a stub); severed_part_fall's / floor_blood_splat_task's task_exit end (p8_blow ran
before the task_exit capture change).
Other workers' files touched: src/game/chapter.cpp (s1: the frame counter seed, Runtime::shadow_update),
src/game/runtime.h (shadow_update override, setResult no-op), src/proof/main.cpp, tools/mame_capture.lua.
