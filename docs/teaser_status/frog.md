# frog (weekend round, 2026-10-16): "the third frog zombie teleports to the mansion entrance"

## frog2 (2026-10-16 evening): **fixed** ("slides" after the bridge rescue)
- The route: rescue the event 3 researcher (`HOTD_AIM_SKIP=5AFE0`: only his attacker is shot) -> event 4 (set-ups 0 + 2).
- Cause: set-up 2's leap-in arc (kind69_frog_setup2_leap_in) ran **29** frames on A; the arcade passes **0x3C = 60**
  (maincpu.c, as the PC). Over 29 frames the arc from under the water (y -44.9) to the bank (y -24) never rises above
  the ground (its vertex lies at the end): the frog glided up out of the pond twice as fast as its leap motion. With 60
  it leaps to y -10 and drops onto the bank. The third frog is the one the player watches (the first two land while
  the camera still turns).
- Evidence: MAME, same route (rescue, frogs aim-skipped), work RAM every 0.1 s: the third frog (0x5F68FC) in state 8
  at motion frame 27 = (40.00, -10.03, -193.33), landing at frame 50; ours after the fix at frame 27 = (40.00, -10.04,
  -192.67), landing at ~50 (before: frame 18, never above y -24). Hops / leaps (states 4 / 5) matched MAME already.
- Debug recipe: `HOTD_AIM_SKIP=5AFE0,88280,89930 HOTD_TRACE_ENEMIES=1 HOTD_TRACE_EVERY=1 hotd --frames 5500 --autofire
  --lives-kept 1` (from event 0: event 4 at 4608, the third frog's leap-in at ~5275).

Status: **not reproduced headless; no fix committed.** Nothing found that writes a frog position in one step.

## What was run (build-frog, Game<A>, HOTD_TRACE_ENEMIES every frame, scan for > 3-8 units per frame)
- Routes: default (0, 1, 3, 9, 11, 17), `--route 1:3=0` (0, 1, 3, 4, 7, 8, 19), `--route 1:3=0,1:4=0,1:5=0`
  (... 4, 5, 6). Event 4 = set-ups 0 + 2, event 6 = set-up 4, event 9 = set-up 3.
- Frogs shot normally (autofire every 60) and frogs left alone (`HOTD_AIM_SKIP=88280,89930,89BC0,89FE0`, new), so
  they run whole leap / return / swim-out cycles; non-shootable phases followed with `HOTD_TRACE_OBJ=a,b,c` (now a list).
- `--players 2` in event 4; `--bot 1/2/3` (human-like shots) with `--route 1:3=0`.
- Result: every frog (and every other shootable object, dogs / agile creatures included) moves at most ~2 units a
  frame; leaps (state 5) go to 10 units in front of the camera and back over 35 frames; the event 6 / 9 swimmers
  leave at x > -125 / z > 250. Swimmer models (0x1DF..0x1E9) are drawn at sane camera-space positions.
- `--boundary-report` over event 4: no frog-related stub (shadow_update, sophie_skin_draw, obj_velocity_clear,
  point_clip_to_depth only).

## Leads not closed
- A set-up 3 swimmer (event 9) only leaves when x > -125 **and** the event is still 9; one still swimming when
  event 9 ends would swim along +x for ever (also in the original's logic; not seen headless: event 9 lasts long).
- `--route 1:3=0,1:4=2` (an out-of-range route choice) hangs the run (100 % CPU, no frames): T1's area.
- Needs from the user: the route / events (which pond), 1 or 2 players, and whether the frog jumps in one frame or
  flies (a 35-frame leap at the player from far away would look like a teleport).

## Tools added (src/main.cpp, debugging)
- `HOTD_AIM_SKIP=<routine hex,...>` (+ `HOTD_AIM_SKIP_FROM=<frame>`): the autofire leaves those objects alone.
- `HOTD_TRACE_OBJ` takes a comma-separated list.
