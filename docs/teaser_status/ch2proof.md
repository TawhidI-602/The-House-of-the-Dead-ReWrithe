# Worker "ch2proof" (2026-10-19): Chapter 2's new routines against MAME

Branch `claude/ch2proof`, build folder `build` (worktree `wt-ch2proof`). Goal: prove the Chapter 2 routines the
workers "ch2", "ch2play" and "hanged" left `UNVERIFIED (teaser)` ([ch2_kinds.md](ch2_kinds.md), [hanged.md](hanged.md)),
fix any difference, and run bot games of Chapter 2.

## Captures (A, MAME 0.289)

All with `tools\run_capture.ps1 -Root <main checkout>`, `-Ported` = `capture_lists.py --funcs <the 84 routines> --no-pc`'s
list, `-Boundary` = its list + the draw hooks `37d80,87550,689e0,68b50,69540` (as proof.md's support rows), then
`hotd_proof capture <folder> <main>\private\arcade_exports\regions`. Folders `private\proof\ch2p_*` (logs, proof.txt,
coverage kept and compressed; the bulky capture.bin / copro.txt deleted after the check: the command lines re-create
them).

| Capture | Command (besides the lists) | Captures | Result |
|---|---|---|---|
| ch2p_full | `-Name play_us_20261007_2238 -Seconds 1620 -Max 40 -Every 4` (the full-game recording: default route, Chapter 4's rematch) | 962 (98,751 coprocessor calls, 12,978 boundary calls replayed) | 9 differ: all capture artifacts (below) |
| ch2p_r10 | fresh auto-aim game, `-Route "2:0,2:5" -Seconds 1300 -Max 40 -Every 4` (events 10 12 13 6) | 1,437 | 38 differ: artifacts |
| ch2p_r11 | the same, `-Route "2:0=0,2:1"` (events 11 12 13) | 1,338 | 31 differ: artifacts |
| ch2p_r14 | the same, `-Route "2:0=0,2:1=0,2:2" -Every 2`, 17 routines (event 14, scenes 10-14) | 130 | 0 differ |
| ch2p_fix | `-Name play_us_20261007_2238 -Seconds 1000 -Every 2 -Funcs "96ec0:1f0,41ff0:148,3eb90,4d710"` | 80 | 0 differ |
| ch2p_door | the same, `-Funcs 96ec0:1f0 -Skip 60 -Every 5 -Max 60` (the door's state 1) | 60 | 0 differ |

The artifacts: `kind168_double_door` 0x96EC0 and `prop_hit_drop_piece` 0x41FF0 have their `ret` outside Ghidra's
index size (0x96EC0: a Ghidra split at 0x96FDC, ret at 0x970AC; 0x41FF0: ret at 0x42134, capture_lists.py kept 0x140),
so mame_capture.lua never closed those captures at their return and recorded later calls into them. With the sizes
given (`96ec0:1f0`, `41ff0:148`) they pass (ch2p_fix, ch2p_r14, ch2p_door). The runs of the other routines are
unaffected (one capture open at a time: the open artifact only blocked later captures for a while).

Proof harness changes (src/proof): the former boundaries ported by "ch2" / "ch2play" (`enemy_fall_death_check`,
`prop_hit_drop`, `prop_lid_fly`, `prop_bookshelf_collapse` / `_remains`, `obj_velocity_clear`, `area_heading_pick`,
`frog_splash_spawn`, `point_clip_to_depth`) were plain boundary overrides, so their port never ran in a capture: now
the `recorded_next` form (older captures that recorded them as boundaries still replay them); cases added for the six
that had none.

## Result per routine (0 differ unless said; captures per run)

No port difference found: no routine changed.

| Routine | Captures |
|---|---|
| kind183_shootable_sign 0x3A40 | full 40, r10 40, r11 40 |
| kind168_double_door 0x96EC0 (+ tail 0x96FDC) | fix 40, r14 40, door 60 (states 0 / 1), door2 60 (states 1-3) |
| kind075_sound_repeat 0x92BE0 | 16 x 3 |
| kind131_prop_group 0x3AAA0, kind133_prop_grid 0x39BC0, kind136_prop_group 0x39F60 | 3, 3, 2 (one-shot spawners) |
| kind132_ebitb_vomit 0x5FDA0 / _frame 0x5FF60 / _dead 0x60330 / drips_draw 0x603E0 | 3 / 97 / 109 / 73 |
| enemy_fall_death_check 0x93340 / _land 0x934B0 | 120 / 92 |
| kind187_fridge_door 0x970B0 | r10 40, r11 40 |
| kind184_lift 0x3B90 / _frame 0x3BE0 | r10 1 / 40 |
| kind185_elevator 0x3CA0 / _frame 0x3CF0 | r11 1 / 40 |
| kind151_prop_group 0x3AEF0, kind152_prop_group 0x3A500 | 2, 2 |
| kind150_ebitan_slide 0x61240 / _frame 0x613D0 / _dying 0x61C10 | r10 1 / 40 / 40 |
| obj_velocity_clear 0x2E870, area_heading_pick 0x2E550, frog_splash_spawn 0x308B0, frog_splash 0x30820, point_clip_to_depth 0x34920 | 10, 8, 3, 83, 14 |
| prop015_fuuu 0x3C6B0, prop064_hit_drop 0x3E5F0, prop067_r2_lid 0x3EF60, prop_flag_swap 0x3BFA0 | 120 each |
| prop_glass_shatter 0x40160, prop_anim_once 0x42CF0, prop023_kokonoha_leaves 0x3C770, prop014_jinmengyo_fish 0x3C4A0 | 4, 106, 80, 120 |
| prop078_r2_door 0x3E9B0 | 120 (the event-1 branch not reached: see prop_door_fly) |
| prop084_niku 0x3ED70, prop083_kyochuu 0x3EE00 / _fall 0x3EEE0 | 80, 17 / 20 |
| prop077_r2_wall 0x3E880 / _wall_pieces 0x3AD60 / _wall_piece 0x3E8F0, prop080_statue_piece 0x3EC90, prop085_fruit_scheduled 0x3BC40 | 80, 2, 80, 80, 80 |
| prop008_bookshelf_topple 0x3E800 | 120 |
| prop_hit_drop 0x42140 / _piece 0x41FF0, prop_lid_fly 0x422B0, prop_fall_spin 0x42EC0 | 13 / fix 40 + r14 40, 2, 80 |
| prop_bookshelf_collapse 0x40BB0 / prop_book_fly 0x40980, prop_bookshelf_remains 0x40F50 / prop_blink_remains 0x42E70 | 7 / 80, 3 / 42 |
| hostage s07 0x56FE0 / 0x57010; s08 0x57090 / 0x57110 / 0x571F0 | 2 / 40; 4 / 58 / 23 |
| hostage s09 0x57510 / 0x57540 / 0x575B0; s10 0x576B0 / 0x57710 / 0x57780 | 3 / 49 / 40; 3 / 113 / 40 |
| hostage s11 0x57930 / 0x579A0 / 0x57A20 | 2 / 14 / 40 |
| player scenes 09 0x4D0B0 / _p1 0x4D1A0 / _p1_p 0x4D240 / _p1_p_p 0x4D280 / _p1_p_p_p 0x4D2C0 | 3 / 120 / 27 / 24 / 24 |
| player scenes 10 0x4D2F0, 11 0x4D450, 12 0x4D5C0, 13 0x4D710, 14 0x4D870 / _p 0x4D9B0 / _p_p 0x4DA20 | 3, 1, 3, 1 (r14), 4 / 84 / 109 |
| hangedman_fight_init 0x4F960 (+ hangedman_captives 0x703B0, run inside it) | 3 |
| hangedman_rematch_init 0x527A0 / hangedman_rematch_intro 0x528D0 (Chapter 4) | 3 / 120 |

## Still open (left UNVERIFIED)

- `prop_door_fly` 0x3EB90: started only by prop078_r2_door in event 1 when flag 14 (0x51E94E) is set; no event script
  of any chapter sets flag 14 (EVTALL.RBH, evt_dis.py), and the sub-type 78 doors stand in event 9. Not reachable.
- (`kind168_double_door`: proven. ch2p_door2 = `-Skip 30 -Every 100000 -Always 51e960:ff0000`: 60 captures from the
  open flag on, states 1 -> 2 -> 3, 0 differ; only its last frame's task_exit call itself was not captured.)

## Bot games (Chapter 2)

`tools\robust_seeds.ps1 -Exe build\Release\hotd.exe -Seeds 1..32 -Parallel 8 -Frames 80000 -Extra "--chapter","2"`
(seeds pick arcade / PC models, 1 / 2 players, lives 9 / 0 / 3, `--strict` 1 in 5).

- Before: 16 seeds, 15 soft-locks, all in mode 5 sub-mode 3 (the game over's route map) with reads outside memory at
  0x2BA86598. Cause: `game_over_route_task` 0x5CF50 walks scene_history from Chapter 1; a game started at Chapter 2
  has Chapter 1's row empty (0xFF), so the route record is 0xC1810 - 0xC0 (garbage: the map never ends). Not a port
  error (the arcade cannot start past Chapter 1). Fix (src/game/chapter.cpp `chapter_start`): the earlier chapters'
  rows get their default routes (Chapter 1: 0 1 3 4 5 6 19, 2: 0 1 2 3 4 5 7 8 9 15, 3: 0 1 3 6 7), as if played.
- After: 32 seeds, **0 crash / assert / soft-lock / NaN / bad access**; 8 clears (1P and 2P, both sources, one under
  `--strict`), 24 stopped at the second game over (the bot's limit).

## Checks

`--input-test` 524 / 524; `--frames 40000 --autofire --mute --lives-kept 1` arcade and PC: "chapter 1 ended" at 16695
(Chapter 2 then ends at 36962); `--chapter 2 --strict --autofire --lives-kept 1 --mute --frames 40000
--default-route`: chapter 2 ended at 17162 (unchanged: no port change).
