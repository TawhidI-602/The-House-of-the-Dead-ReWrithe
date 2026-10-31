# Worker "hanged": the Hangedman's fight (Chapter 2 boss) (2026-10-17)

Plan: [chapter2_3_plan.md](../chapter2_3_plan.md), worker "hanged". Branch `claude/hanged`, build folder `build-hanged`.
Arcade (A) forms only (no Game<Pc> instantiation: `src/port/inst/hangedman_fight.inc`, `cases/hangedman_fight_pc.inc`
are empty). Read from the arcade disassembly (`tools/i960dis.py`), the typed decompilation as a guide (its coprocessor
sequences are garbled; every FIFO sequence was taken from the disassembly in its original order).

## Ported (src/port/boss_hangedman.cpp, 31 routines; 27 proven on A, see below)

| Group | Routines |
|---|---|
| Set-ups | `hangedman_fight_init` 0x4F960 (set-up 1, Chapter 2 event 15: hit points from the set-up block +0x35C, state 3, the captives), `hangedman_rematch_init` 0x527A0 (set-up 2, Chapter 4: state 14) |
| Frame | `hangedman_fight_frame` 0x4FAA0 (aggression, bob, hits, hit reaction, state table 0xC1150, velocity, draw, lists), `hangedman_hit` 0x4FC40, `hangedman_hit_reaction` 0x4FDF0, `hangedman_vuln_clear` 0x500E0 |
| States | 3 `hangedman_intro` 0x503E0 (voices, captives drop, title sprites 0xDD / 0xE0, camera hold, life bar task), 4 `hangedman_flap` 0x50700 (the bat swarm's waves), 5 `hangedman_rejoin`, 6 `hangedman_hover`, 7 `hangedman_claw_attack`, 8 `hangedman_return`, 9 `hangedman_flee`, 10 `hangedman_wait_reappear`, 11 `hangedman_dive_approach`, 12 `hangedman_dive_attack`, 13 `hangedman_death_fall`, 14 `hangedman_rematch_intro` |
| Helpers | `hangedman_hover_point` 0x52980, `hangedman_turn_toward` 0x52D20, `camera_set_direct` 0x301E0, `boss_aggression_init` / `_update` 0x305A0 / 0x305D0 (shared with the Magician) |
| Tasks | `boss_life_bar_task` 0x52C30 (also the rematch's), `hangedman_death_effect` 0x52CE0 + `hangedman_explosion` 0x52DA0, `hangedman_intro_effect_spawn` / `_effect` 0x31120 / 0x31160, `hangedman_captives` 0x703B0 + `hangedman_captive_frame` 0x70190 |
| Kind 19 | `billboard_model` 0x30690 (event 15's billboard, until flag 253) |

Registered as the Chariot: `src/port/decl/hangedman_fight.h`, `src/proof/cases/hangedman_fight_arcade.inc` (+ empty
`_pc.inc`), one line each in game.h / game.cpp / proof/main.cpp; no new boundaries (the HUD's sprite_place /
sprite_draw / life_bar_* are the Chariot's). `boss_hangedman` / `hangedman_flyby.cpp` untouched: set-up 1 / 2 reach
the new routines through `call_routine` (the runtime's dispatch).

Event flags (0x51E940 + n) the fight drives: 192 life bar up, 193 back after the swarm, 194 fled, 195 / 196 captives
drop / camera back, 197 swarm called off, 199 back in place, 202 captives fall; it waits for the event's 198, 200, 201.

## Headless (2026-10-17)

`hotd --chapter 2 --event 15 --frames 8000 --autofire --mute --lives-kept 1 --strict` (no guards), arcade and PC
models: **the fight runs to the kill and the chapter clears** (chapter clear 5745, chapter end 6227; only
`player_scene14_rattle_door` unported). The fight (the boss's +0x50 timer, about 2 per frame):

| t | state | what |
|---|---|---|
| 0-1416 | 3 intro | voices 31 / 32, captives fall (flags 202, 195, 196), title, life bar (192); 800 hit points |
| 1416-2557 | 4 flap | 30 swarm bats in waves (hangedman_bat_waves), the bot shoots him 800 -> 390; below share 6 the swarm is called off (197) |
| 2557 | 5 rejoin | flag 193, new enemy slot |
| 2559-5026 | 7 / 8 / 6 | claw attacks (patterns 0, 1, 2; the player is hit), return, hover; flinches on hits (MO 35) |
| 5026-6812 | 6 | below share 7 not hurt, waits for the camera frame |
| 6812-7990 | 9 flee | climbs to y 1000, off to (1300, 1000, -1500); flag 194 |
| 7990-8830 | 10 | hidden until the event's 201, glides back, 199, waits 198 / 200 |
| 8830-9913 | 11 / 12 | dive approach and dives (path 70), 150 -> 0 |
| 9913-10229 | 13 | death fall, kill score 1500, blast, blinks, removed; the event ends the chapter |

Chapter 1 unchanged: `--frames 20000 --autofire --mute --lives-kept 1 --default-route`, arcade and PC: clear 17104,
end 17586, output identical to the same build without the new cases; input test 518 / 518, 0 failed.

Full Chapter 2 (`--chapter 2 --default-route`, guards on): events 0 1 2 3 4 5 7 8 9 15, chapter end 53169 (was 54752);
the Hangedman and the billboard are off the unported list. But here the bot does not hurt him: arriving from event 9
(cut by guards), the camera's look-at differs from a start at event 15 (same eye, the boss behind the camera, view z
-95 against +104), so the autofire shoots him at z ~3 without hits; the guard then passes WAIT_ITEM 193 / 194 / 199.
That is the camera state left by the earlier (guarded) events, not the fight; with the flags 192-202 exempted from the
guard the chapter would stall at 194 in such runs, so the guard was left as it is.

## Proven on A (hanged_full, 2026-10-17)

`run_capture.ps1 -Root <main checkout> -Name play_us_20261007_2238 -Seconds 1600 -Max 150 -Every 4` with `-Funcs` /
`-Ported` / `-Boundary` from `capture_lists.py --funcs 4faa0,4fc40,4fdf0,50110,503e0,50700,50930,509f0,50c10,51020,
51290,515b0,51b40,51fa0,52c30,52da0,30690,70190,31160 --no-pc` (+ the draw hooks 9ce60,37d80,87550,689e0,68b50,69540),
then `hotd_proof capture <folder> <main>\private\arcade_exports\regions`: **2,343 captures (63,434 coprocessor calls,
12,011 boundary calls replayed), 0 differ**, first try. Per routine: fight_frame / hit / hit_reaction / death_fall /
flap / hover / wait_reappear / dive_attack / life bar / billboard / intro effect / captive frame 150 each, intro 140,
claw 138, return 123, flee 94, dive_approach 31, explosion 16, rejoin 1. Reached inside them (so proven with them):
hover_point, turn_toward, vuln_clear, camera_set_direct, boss_aggression_init / _update, intro_effect_spawn,
death_effect. Branch coverage (`branch_coverage.py`): most branches ran one or both ways; captive_frame only its first
frames (the 150-capture cap). The bulky capture data (8 GB) was deleted after the check (logs, coverage and the result
kept in private\proof\hanged_full, compressed); the command above re-creates it in about 45 minutes.

UNVERIFIED (teaser), not captured: `hangedman_fight_init` and `hangedman_captives` (run once per fight; not in -Funcs),
`hangedman_rematch_init` / `_intro` (Chapter 4). A fresh auto-aim game (`-Chapter 2`, no -Name) stalled in Chapter 2
event 0 for 1200 s (score 200, `ch 1 ev 0`): the harness's chapter start does not get past event 0's route object.

## Not done / next

- The bob: `hangedman_fight_frame` sinks 0.1 a frame while type 50 and +0x54 <= 0x1432; +0x54 stays 0 (nothing in
  the program writes it for him), so he sinks during the intro and the swarm (115 -> -45 in the bot game). As the
  original does (fight_frame 0 differ over 150 captures).
- The bot / proof harness: no boss-aware aim (the autofire aims at the nearest object, which works from event 15).
- Chapter 4's rematch (set-up 2, state 14) is ported but not run (Chapter 4 not playable).

## Proven (worker "ch2proof", 2026-10-19)

`hangedman_fight_init` (with `hangedman_captives` inside it), `hangedman_rematch_init` / `_intro` (Chapter 4 of the
full-game recording and of the auto-aim games): 0 differ, see [ch2proof.md](ch2proof.md). Nothing of the fight is UNVERIFIED now.

## User play-test 2026-10-19 (video "Chapter 2 Test 1", worker "ch2proof")

Compared with MAME's fullgame_us snaps 0276-0334 (event 15).

| Video | Seen | Cause | Fix |
|---|---|---|---|
| 4:32-5:58 (intro to death) | no wings anywhere (MAME: big green bat wings, flapping) | the part hook 0x9CE60's case for types 50 / 69 (0x9DACC) was not in the runtime's body_part_draw_hook: the wing model (+0x50 % 100) + 0x1401 drawn at joint 1 | ported (src/game/creature.cpp), commit 1ee9a37 |
| 4:45-5:20 | after the intro he hangs low over the roofs, small, off the moon; the bats keep attacking, two lives lost | the same hook writes the wing frame into +0x54, which hangedman_fight_frame's bob reads (sinks only while +0x54 <= 0x1432); with +0x54 = 0 he sank all the time (the "bob" note above) | same fix: he now hovers in front of the moon as in MAME |
| hits | no hit flash | the hook draws the parts with model record word 1 = 0x802400 while +0x2F4 is odd | flash (2026-10-19, cbd6ec5): 0x802400 is the header RAM's flat set of palette entry 7: drawn flat white, lit (see hermit.md, Flash); PC models: the part not drawn on those frames (THOTD.EXE 0x51D8D4). Shots `private\flash\hanged_sheet.png` (`--chapter 2 --event 15`, f2193 white limbs), the Chapter 4 rematch in `c3_arcade_sheet.png` |

Chapter 2 default route (`--strict`) now ends at 17270 (was 17162): the bob as the original's. Open: a fresh
side-by-side of the whole fight after the fix (camera swings to empty sky also happen in MAME's snaps 0306-0318);
the runtime's hook is not a capture-proven routine (src/game, not the port).

## Side-by-side after the wings fix (worker "hangcmp", 2026-10-19)

Ours: `--chapter 2 --event 15` and the full default route (`--strict` and guarded), arcade and PC models, `--render`
shots every 2 s plus dense runs (swarm, shot flash, death), `HOTD_TRACE_ENEMIES` traces of his object and the camera.
MAME: fullgame_us snaps 0276-0335, and a new replay of play_us_20261007_2238 with a work RAM dump + snap every 0.25 s
from 556 s to 672 s (private\hangcmp\mame_fight; his object read from 0x5433A0). Plus the user's video 4:30-6:02.

**Logic matches.** MAME's trace and ours line up state by state: intro 690 frames in both (fc 32020-32709 / ours
11385-12075), y 112-117 with the bob through intro and swarm, the claw / hover / return loop over the camera's roof
run (z -15 -> -885), flee (y to ~1750, z to ~-1950), wait, the camera to (1423, -105, -695), dive approach / dives,
death fall to the courtyard, 180-frame blink (drawn every other frame). The shot flash (orange ball, then the white
star), blood, HOLD YOUR FIRE! in the intro, the title card, the life bar (size / place / grey trail), the bats, the claw
scratch all as MAME's. Phase lengths differ only by how fast the player hurts him (MAME's player faster in the swarm,
slower in the claw phase).

| # | Difference | Cause | Status |
|---|---|---|---|
| 1 | With the guards on (the launcher's default, every user game) the camera drove off along the roof while he was still swarming at the moon, and later the event moved on to his wait / dive mid-claw-attack | the teaser guard set WAIT_ITEM flags still waiting after 20 s; 193 (swarm over) / 194 (fled) are his own flags (ported). MAME's claw phase alone is 1783 frames | **fixed** 5c61cda: flags 192-202 of Chapter 2 event 15 never forced. Guarded `--event 15 --autofire-every 30`: the camera now waits at the moon until state 7 (was: guard at frame 2332 while in state 4), halts ~40 frames on each claw hit as in MAME (fc 33645 / 34076 / 34278 / 34479). Guarded full Chapter 2: still ends 17270 |
| 2 | `--event N` starts (developer): the screen black but for the holes the title sprites cleared | chapter_screen_reset's black cover (layer 2) is lifted by event 0's script only | **fixed** 06339fd: lifted at the first 2D set-up of a game started past event 0 |
| 3 | The cut-scene after the fight (Curien, grey palette): MAME deep red-brown with black shadows, ours a washed-out mauve with lifted blacks (mame_fight snap 0442 vs ours f16560) | the grey / sepia palette (palette_grey_build) | **fixed** 18fe2eb (colors): the grey look rewrites the colour table per channel before the output curve; now wall 18,0,0 as MAME (polish.md, colors) |
| 4 | After the kill our body on the courtyard floor was not seen (MAME 0374-0380: wings by the crosshair) | his kill spot (z -502 here, -674 in MAME) puts him under the trees' dark canopy from this camera; he is drawn (every other frame, 22 models) | play-dependent, not a bug as far as checked (UNVERIFIED: occlusion not proven) |
| 5 | Single-wing frames in the swarm | the wing model's own flap frames (MAME 0287 shows the same) | none |

Not compared: two-player fights; Chapter 4's rematch (its WAIT_ITEM flags are not exempted from the guard, check
when Chapter 4 is playable).

## The teaser guard audit (worker "hangcmp", 2026-10-19)

The guards (`evt_watchdog`, on in every launcher game) forced a WAIT_ITEM flag after 20 s whatever ran, and passed
WAIT_ENNUM / WAIT_ENALV after 20 s only while unported routines ran. Now both act only when routines that are not
ported ran during the wait (a soft-lock net; with Chapters 1-4 and the ending ported it never acts). The 192-202
exemption above is folded into that rule. The other cut guard (`missing`: a creature without body_init /
creature_type_setup halts) is moot (both ported).

Guarded runs (no `--strict`), `--autofire --lives-kept 1 --mute`: Chapter 2 default + `2:0,2:5`, `2:0=0,2:1`,
`2:0=0,2:1=0,2:2`; Chapter 3 default + the five branch routes; Chapter 4 default + `4:1`; the ending (`--ending`).
Normal autofire: 0 guard actions before and after. Slow autofire (`--autofire-every 45`), before: 1 per run that
reaches Chapter 4: **WAIT_ITEM 193 at 0xFC1F4 (the Chapter 4 Hangedman rematch) forced at its frame 7853**, cutting the
rematch short (event 1 -> 2 at 11496 instead of 11672); after: 0, every chapter end frame the same otherwise. Stall
test: `HOTD_STALL_ROUTINE=4FAA0` (testing: that routine treated as unported) `--chapter 2 --event 15`: the guard sets
195 / 192 / 193 / 194 / 199 and passes the WAIT_ENALV / WAIT_ENNUM, the chapter ends (11567).
Seen on the way: with `--autofire-every 30` / `45` the bot never kills the Magician (Chapter 4 event 4, WAIT_ENNUM for
46,000+ frames, the player hit ~every 600 frames); no guard involved (he is ported), the bot's, not checked further.

## Motion check against MAME (worker "hangcheck", 2026-10-21)

User: "the Hangedman is a little clunky still, not sure he moves as in the arcade". Clips for judging:
`private\hangedman\index.html` (ours left, MAME right; 60 Hz and 144 Hz; intro, claw attacks, flee / return, last dive
and death; the wings before / after; Smooth Motion off / on).

**Logic: the same, frame by frame.** MAME replay of play_us_20261007_2238 with `tools/mame_hang_trace.lua` (one line
per game frame: his state, position, rotation words +0x2C / +0x30 / +0x34, +0x50, wing model +0x54, motion, motion
frame, flags, camera; a snapshot per frame), ours `--chapter 2 --event 15` with `HOTD_TRACE_HANG=5513CC`;
`tools/hang_trace_cmp.py`: from his first frame **716 frames identical in all 15 fields** (the whole 705-frame intro
and the swarm's start, camera included) until our bot's first hit (flags 0x103). After that the games differ only by
the player's shots; per state the pace is the same (mean / max movement a frame, MAME / ours): hover 1.89 / 1.90,
return 2.50 / 2.51, flee 8.45 / 8.44 (373 / 374 frames), wait 743 / 743 frames, dive approach 3.00 / 3.00. MAME's
own hitches: 46 lag frames (a vblank without a game frame) in the 5593-frame fight; ours has none.

**Drawn motion (Smooth Motion): one bug, fixed.** His wings (model 0x1401 + +0x50 % 100, drawn at joint 1) take a new
model ID every frame, so the render interpolation (rate.md) never matched them with the previous frame: they were
drawn at the newest frame while his body was drawn in between. `tools/hang_blend_check.py` on the in-between pictures
of the whole fight at 144 Hz (`HOTD_INTERP_CLIP` + `HOTD_TRACE_BLENDED`): wing-to-joint gap **mean 0.85, max 12.5
units, 68.5 % of the pictures** before; **0.000 in every picture** after (60 Hz too). Fix (src/game/view.cpp): models
left over after the (task, model, body, shadow) match pair up by (task, body, shadow) in order when the task left as
many over in both frames (the splashes' flip-book rule, generalised). Also checked, fine: his only "jumps" (drawn
unblended) are the logic's own: frame 3469 a 180-degree turn in one tick while out of sight (flee, +0x30 0x7F59 ->
0xFF5C) and 3912 his teleport back; the camera's 4 cuts in the intro are camera_set_direct's (MAME the same).
Matrices are blended, not angles, so there is no angle wrap-around problem.

Not a difference: the death (ours killed at the top of the dive, z -466, falls far off in the sky; MAME's player hit
him low near the camera, z -674, the camera looks down after him): the kill spot is the player's.

Tools: `HOTD_INTERP_CLIP=<out.mp4>,<from>,<to>,<hz>[,off]` (with `--render`: frames from..to as a <hz> display
shows them with Smooth Motion, to the local ffmpeg), `HOTD_TRACE_BLENDED=<file>[,task]`, `HOTD_TRACE_HANG=<object>`.

Seen in the clips, not his motion: while he flees (clip 03, ~3 s) MAME shows a dark building edge at the left of the
picture that ours does not draw, and ours shows a thin bright diagonal line near the moon (also in clip 04) that MAME
does not.
- The line: **fixed** (moonline). It is the top edge of the moon quad 0x14AC (moon22): with `HOTD_IDSHOT=1` (new: an
  item-index picture `ids_f<F>.png` beside each `--shot-at` picture) the line lies exactly on the border between the
  sky 0x14B9 and the moon item. The quad shows its texture once (texture points 0..size), but the bilinear filter's
  2x2 footprint at its edge wrapped into the texture's other side (the moon's middle) and drew one bright pixel row.
  Now polygons whose texture points all lie inside their texture (`kInsideRect`, arcade loader) keep the footprint
  inside it; repeated textures still wrap. Check: `--chapter 2 --event 15 --autofire --render --shot-at 2190` line
  gone; Chapters 1-4 frames 1500 / 3000 old vs new: only single polygon-edge pixels differ (PSNR 54-76 dB, one
  identical), no visible seams.
- The building edge: not a missing polygon. In the same clip frame MAME's moon sits ~110 px (of 640) further right
  than ours, and its building is shifted by the same amount: ours draws the building's edge just outside the
  picture's left side. The camera's turn differs there (after the first hits the two games' fights differ), the
  drawing does not. Not compared number for number (no MAME camera trace of that frame).

Checks (after `git merge main`): `--input-test` 1585 checks, 0 failed; Chapter 1 `--frames 40000` arcade and PC:
"chapter 1 ended" at 16695; `--chapter 4 --strict ... --default-route` arcade and PC: "the ending is over" at 34967.
