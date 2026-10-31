# Proofs worker 11 (weekend round 2, 2026-10-17 night)

Branch `claude/proofs-11`, build folder `build-proofs11` (as proofs-9 / -10). Results and command lines in
[proof.md](../proof.md) ("Opening scene bodies", "Hostage scenario 6"). UNVERIFIED 50 -> 43.

## New capture switches (tools/mame_dump.lua, run_capture.ps1)

- `-NoSkip` (HOTD_NOSKIP=1): the auto-aim keeps the trigger released while a cut-scene is skippable (evt_skip_on
  0x520944), so the scenes play to their end.
- `-HoldFire "c:e:seconds,..."` (HOTD_HOLDFIRE): no auto-aim fire for that long after the game enters the event.

## Proven

- p11_s00 (`-Shoot 0 -Seconds 110`; kept, compressed): scene 00 turn / turn_end / hold, 47 captures, 0 differ. Every
  earlier run had skipped the opening with the auto-aim's shots.
- p11_s06 (`-Route "1:3,1:9,1:11=0" -AimSkip "3e030,56f30"`, 900 s; kept, compressed): hostage scenario 6 and
  hostage_thank_leave_step, 22 captures, 0 differ. Found with the event data: Chapter 1's hostage records (kind 100)
  are ev0 actions 0 / 2 / 1, ev3 action 3, ev6 4, ev13 5 and 4, **ev15 6** (section 5), ev18 4.

## Not reached / not reachable (left UNVERIFIED)

- Camera routines: the event scripts (PC EVTALL.RBH, all 7 lists, evt_dis.py) use CCTB 16-21 / 33 / 64 only, CCTB 33
  with modes 4 / 6 / 7 only, CAMDR 3 only: `cctb33_mode5(_start/_frame/_impulse)`, `cam_job32_offset`, `camdr_mode2`
  are never started by the game's data; `game_cam_nop`, `unused_2d5d0 / _2d600`, `ChgCamMode_undefined` are debug /
  unused; `camera_chapter_reset` / `cam_next_job` have no direct callers (their bodies are inlined elsewhere).
- `player_scene07_jump_down_a_p1_p_p_p`: p11_s07 (`-Route "1:4,1:7" -NoSkip`) captured 4CDF0 5 times, never 4CE70
  (the scene ends before anim_time 96). `player_scene05_knocked_flying_p_p(_p)`: p11_s05 (`-Route 1:5 -NoSkip`) and
  p11_s05b (the same, `-TwoPlayers`, 420 s): scene 05 not reached past `_p`.
- `hostage_s03_thrown`: p11_s03 / p11_s03b (`-HoldFire "1:3:40"` / `"1:3:110"`): no throw.
- `chariot_act17_wait_combat`, `hud_popup_clear`: p11_c17 (`-Mortal -Continue 9`, 700 s): the auto-aim never lost all
  lives (Chapter 3 reached). hud_popup_clear needs a hit during a CCTB 33 mode 4-7 scene (the only popup source,
  player_hit_notice) within 60 frames of losing the last life.
- `hostage_shot_slide / _fall`: state 20, which no Chapter 1 scenario sets.

Deleted (exploratory, nothing captured or nothing new): p11_s03, p11_s03b, p11_s05, p11_s05b, p11_s07, p11_c17.
