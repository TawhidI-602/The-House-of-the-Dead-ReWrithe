# Halloween release: Chapters 2 and 3, the gap analysis and the plan (written 2026-10-17)

Goal (user, 2026-10-17): **Chapters 1 and 2 definitely, Chapter 3 if possible, Chapter 4 after Halloween (~2026-10-28)**,
plus a **chapter select in the launcher** (a chapter is selectable once it plays through; the others are greyed out,
never broken). Work starts Monday 2026-10-19, 4 PM, with a new weekly budget: parallel workers per chapter.
The summary and the dates are in [teaser_plan.md](teaser_plan.md) ("Halloween release: Chapters 1-3"); this file is the
analysis behind them.

How this was measured (the method of teaser_plan.md, refined; reproducible with `py tools/chapter_gap.py --root
"<main checkout>" --chapter N [--events] [--routines]`):

- The chapter's events: `tools/evt_dis.py` on EVTALL.RBH (lists 1 and 2 = Chapters 2 and 3), every spawn record
  (ENTB / ENTBE / ENTB3 and the ET1P* / ET2P* variants, which are the same lists with a player-count filter: kind at +0,
  creature type = byte +0x24, action / hostage scenario = byte +0x25; ENTB2 = scene objects), each kind mapped through
  the arcade kind table (0xAFDE0) to its routine. Chapter 1's table from teaser_plan.md reproduces exactly.
- **What is ported** is read from `src/port/decl/*.h` (a routine with its arcade address and no inline stub body); a
  `virtual ... { }` stub is a boundary and counts as missing. Today: **1,130 routines, 307 KB** of the 551 KB program
  (teaser_plan.md, 10-15: 93 KB), 50 UNVERIFIED routines left (`py tools/unverified_list.py`).
- **Missing sizes** are arcade i960 bytes from `private/decomp/arcade/maincpu_index.tsv`: the call graph from each kind's
  root, **now including function pointers** (immediates that are routine addresses, and tables of routine pointers such
  as the Hangedman's set-up / state tables, the hostage scenario table 0xC1580, the prop sub-type table 0xAF960), which
  the 10-15 walk missed (+20-40 % hidden). Routines that Chapter 1's ordinary kinds reach (drawing hooks, text
  printing, effects: 16 KB that Chapter 1 already runs without, mostly the runtime's own) are not counted. Index sizes
  are sometimes a few bytes short; real sizes are still perhaps 10-20 % larger (computed branches the scan cannot see).
- The first walk's baseline was wrong for kind 18 (Chapter 1's fly-by pulls in the whole fight through the set-up
  table); the numbers below use the "ordinary kinds only" baseline and count the bosses and hostages on their own.

## Headline

| | Chapter 2 | Chapter 3 (alone) | Chapter 3 on top of 2 |
|---|---|---|---|
| Boss | Hangedman: **14.4 KB**, 30 routines | Hermit: **8.9 KB**, 33 routines (+ Curien 0.7 KB) | same |
| Enemy / scene kinds | 9.5 KB (16 kinds) | 8.5 KB (8 kinds) | 5.4 KB (kind 150 and kind 3 come from Ch 2) |
| Hostage scenarios | 3.1 KB (s7-s11) | 3.4 KB (s12-s15) | 3.4 KB |
| Prop sub-types (kind 128) | 3.4 KB (9 sub-types) | 1.9 KB (6) | 1.3 KB |
| **Missing total** | **30.5 KB** (about 23 KB on the default route) | **23.3 KB** (about 21 KB default) | **19.7 KB** |

With the +10-20 % for computed branches: Chapter 2 about 34-37 KB, Chapter 3 about 22-28 KB more. For scale: the
Chapter 1 teaser plan counted 110-130 KB and the overnight run (10-15 .. 10-17) delivered it; Chapters 2 + 3 are about
half of that, and nearly all of the new logic is the two bosses and a long tail of small props / doors / hostages.
**No new file format, no new engine system** is needed (the rest is listed under "Engine and runtime").

## Chapter 2: events, routes and what they spawn

Branch table (arcade 0xAAD20): 0 -> 1 / 10, 1 -> 2 / 11, 2 -> 3 / 14, 3 -> 4, 4 -> 5, 5 -> 7 / 6, 6 / 7 -> 8, 8 -> 9,
9 -> 15 (the Hangedman), 10 / 11 -> 12, 12 -> 13, 13 / 14 -> 4 (a loop back into the main line), 15 -> chapter clear.
`route_choice` 0 everywhere gives the **default route 0, 1, 2, 3, 4, 5, 7, 8, 9, 15**; 6, 10-14 are branches (events 12 and
10-11 are the rescue / route-object side line). `*` = something missing (see the tables below); `**` = the boss.

| Event | Kinds spawned (scene objects after "scene") |
|---|---|
| 0 (route) | 0, 32, 128 (sub-type 67\*), 162; scene 168\*, 177, 178 |
| 1 (route) | 70, 96, 128, 136\*, 162, 183\* |
| 2 (route) | 0, 21, 68, 70, 98, 99, 100 (s7\*), 128, 134, 135, 162, 169 |
| 3 (route) | 0, 21, 32, 66, 70, 96, 128, 134, 162, 169 |
| 4 (route) | 0, 10\*, 21, 33, 36, 70, 100 (s8\*), 128 (14\*, 64\*), 133\*, 162, 169 |
| 5 (route) | 0, 70, 96, 98, 100 (s9\*), 128 (30\*, 31\*), 162, 183\*; scene 75\* |
| 6 | 0, 70, 128 (30\*, 31\*), 162; scene 184\* |
| 7 (route) | 0, 32, 162; scene 161, 178 |
| 8 (route) | 0, 128, 132\*, 162 |
| 9 (route) | 0, 3\*, 32, 64, 70, 98, 100 (s10\*), 128 (78\*), 131\* |
| 10 | 0, 6, 12, 32, 66, 70, 150\*, 160, 162; scene 178 |
| 11 | 0, 3\*, 66, 96, 162; scene 185\* |
| 12 | 0, 4, 32, 70, 96, 98, 99, 100 (s3, s11\*), 128 (77\*, 83\*, 84\*), 132\*, 152\*, 162; scene 187\* |
| 13 | 0, 32, 66, 70, 96, 128, 135, 151\*, 162 |
| 14 | 128; scene 178 |
| 15 (route) | **18\*\*** (the Hangedman, set-up 1, 800 hit points), **19\*** (billboard); scene 67, 161, 176, 178, 182 |

Event opcodes Chapter 2 uses (36): ATCMD BGTB CAMDR CAMDR2 CCTB CHGSM CLTB ENTB ENTB2 ENTB3 ENTBE ET1P3 ET1PE ET2P ET2P3
ET2PE FKRTB GGCOL GGSET ITEMON KGTB NEXTEV RNWCJ SCNINIT SKIP SKYKD SKYTB SNDON TEXCHG WAIT_CAMFRM WAIT_CAMJOB
WAIT_ENALV WAIT_ENNUM WAIT_FRM WAIT_ITEM WLTB. **All are ported** (the interpreter has every opcode; proven on every frame of
Chapters 1-2 of the full-game recording, teaser_plan.md). Camera paths up to 91 (CAMALL, generic).
Event 15 is driven by flags the boss must raise: it waits on WAIT_ITEM 192-195 and 199 and sets ITEMON 198, 200, 201, 252, 253 (waits 254), so
a fight that never sets them stalls the chapter (the teaser's event guard would only pass the wait: not acceptable here).

New creature types over Chapter 1 (kind, type / action): kind 0: 32, 33, 45, 51, 58; kind 32 actions 1-4; kind 96: 43, 44;
kind 98: 16 / action 2, 30 / 7; kind 99: type 28 / action 13; kind 150 types 4 (0-2). All are data in the creature tables
(`creature_type_setup`, motion lists); kind 70's set-ups are all ported (docs/kind70_status.md).

### Chapter 2: kinds missing (arcade address, size)

| Kind | Routine | Missing | Notes |
|---|---|---|---|
| **18** Hangedman | `boss_hangedman` 0x4EE70, set-up table 0xC1190: 0 fly-by (ported), **1 `hangedman_fight_init` 0x4F960**, 2 rematch 0x527A0 (Chapter 4) | **14,420 B, 30 routines** | see "Bosses" |
| 19 billboard | `billboard_model` 0x30690 | 388 B | event 15's ENTB record |
| 150 | `kind150_ebitan_slide` 0x61240 (+ frame 2,048 B, dying 436 B) | 2,872 B | events 10 and 13 (branches); also Chapter 3 event 3 |
| 136 | `kind136_prop_group` 0x39F60 + `prop023_kokonoha_leaves` 0x3C770 (988 B) | 1,384 B | event 1 (route) |
| 152 | `kind152_prop_group` 0x3A500 + `prop085_fruit_scheduled` 0x3BC40 (852 B) | 1,056 B | event 12 |
| 133 | `kind133_prop_grid` 0x39BC0 + `prop015_fuuu` 0x3C6B0 | 616 B | event 4 (route) |
| 151 | `kind151_prop_group` 0x3AEF0 + `prop080_statue_piece` 0x3EC90 | 504 B | event 13 |
| 168 | `kind168_double_door` 0x96EC0 + tail 0x96FDC | 496 B | event 0 scene object |
| 132 | `kind132_ebitb_vomit` 0x5FDA0 + frame | 476 B | events 8 (route), 12 |
| 183 | `kind183_shootable_sign` 0x3A40 | 324 B | events 1, 5 (route): **a route object** (0x3A40 family) |
| 187 | `kind187_fridge_door` 0x970B0 | 304 B | event 12 |
| 184 / 185 | `kind184_lift` 0x3B90 + frame, `kind185_elevator` 0x3CA0 + frame | 260 / 208 B | events 6, 11 (branches) |
| 131 | `kind131_prop_group` 0x3AAA0 | 164 B | event 9 (route) |
| 10 | `kind10_remove` 0x76FC0 (kind 10 itself is ported) | 172 B | event 4 (route) |
| 3 | `enemy_fall_death_land` 0x934B0 | 228 B | events 9, 11 (Chapter 3 too) |
| 75 | `kind075_sound_repeat` 0x92BE0 | 92 B | event 5 scene object |
| Kind 128 sub-types | 14 fish 0x3C4A0 (528 B), 30 / 31 `prop_flag_swap` 0x3BFA0 (588 B incl. glass shatter / anim_once), 64 `prop064_hit_drop` (64 B), 67 `prop067_r2_lid` (368 B), 77 `prop077_r2_wall` (684 B), 78 `prop078_r2_door` (728 B), 83 `prop083_kyochuu` (340 B), 84 `prop084_niku` (144 B) | 3,444 B | sub-type table 0xAF960; all other Chapter 2 sub-types are `prop_default` (ported) |
| Hostages (kind 100) | scenarios 7-11: `hostage_s07_crawl` 0x56FE0 (160 B), `s08_curled` (1,096 B), `s09_flee_thank` (396 B), `s10_walk_off` (620 B), `s11_reluctant_gift` (840 B); scenario 3 is ported | 3,112 B | scenario = record byte +0x25: event 2 s7, 4 s8, 5 s9, 9 s10, 12 s3 / s11 |

Everything else Chapter 2 spawns is ported: kinds 0, 4, 6, 12 (spawn family), 21, 32, 33, 36, 64, 66, 67 / 68 (bats: 67 is the
Hangedman's swarm), 70, 96, 98, 99, 128, 134, 135, 160-162, 169, 176-178, 182 (all A-proven except where proof.md says
"not reached").

## Chapter 3: events, routes and what they spawn

Branch table (0xAAE20): 0 -> 1 / 2, 1 -> 3 / 4, 2 -> 3 / 4, 3 -> 6 / 5, 4 -> 6 / 5, 5 / 6 -> 7 (the Hermit), 7 -> clear.
Default route **0, 1, 3, 6, 7** (five events); 2, 4, 5 are the other branch. Chapter 3 is short: 8 events.

| Event | Kinds spawned |
|---|---|
| 0 (route) | 0, 5, 9, 32, 70, 96, 99, 100 (s12\*), 106\*, 128 (87\*, 94\*, 95\*, 121\*), 153\*, 162; scene 161, 177, 189\* |
| 1 (route) | 0, 7\*, 8, 32, 96, 99, 100 (s13\*, s14\*), 128 (96\*), 153\*, 162; scene 161, 186\* |
| 2 | 0, 8, 11, 32, 70, 98, 99, 100 (s14\*, s15\*), 128, 153\*, 160, 162; scene 161, 186\* |
| 3 (route) | 0, 32, 128 (95\*), 150\*, 162; scene 161 |
| 4 | 0, 32, 70, 128 (86\*), 162; scene 161, 186\*, 188\* |
| 5 | 0, 8, 32, 162; scene 186\* |
| 6 (route) | 0, 3\*, 8, 32, 70, 96, 162; scene 186\* |
| 7 (route) | **40\*\*** (the Hermit, record type 105); scene 24\* (Curien), 78, 161, 176, 178, 182 |

Opcodes: Chapter 1's set plus **PYPMD** (ported, `evt_PYPMD` 0x66FE0; Chapter 3 uses it) and ET1PE (generic). New
creature types over Chapter 1 (kind 0): 53, 61-63, 65, 72, 73, 83, 84, 86, 89; kind 99 types 53 / 62 / 63 / 73 with actions 9,
14, 15, 17, 18 (all guard actions 1-20 are in `enemy_guard`); kind 32 actions 5, 6; kinds 5, 7, 8, 9 are the spawn variants
(ported; 7 needs `enemy_fall_death_land`). The "Taran" (kind 153, types 48 / 90) is Chapter 3's new enemy.

| Kind | Routine | Missing | Notes |
|---|---|---|---|
| **40** Hermit | `boss_hermit` 0x85430 | **8,900 B, 33 routines** | see "Bosses" |
| 24 Curien | `kind24_kyurian_curien` 0x87C50 (+ wait, rise, float) | 652 B | event 7 scene object after the Hermit (Chapter 4's kind 25 is the same family, 708 B) |
| 153 Taran | `kind153_taran` 0x638D0, `_frame` 0x63AC0 (2,312 B), reach check, fall off | 3,712 B | events 0-2; 11 spawn records (types 48 / 90, actions 0-10) |
| 150 | `kind150_ebitan_slide` (shared with Chapter 2) | 2,872 B | event 3 |
| 106 padlock | `kind106_padlock` 0x5C0C0 + lock spark | 604 B | event 0 |
| 188 route spot | `kind188_route_spot` 0x3E90 | 496 B | event 4: a route object |
| 186 moving object | `kind186_moving_object` 0x3D80 + frame | 264 B | events 1, 2, 4, 5, 6: likely the route objects (0x3A40 family) |
| 189 locked door | `kind189_locked_door` 0x4240 | 276 B | event 0 |
| 7 / 3 | `enemy_fall_death_land` | 228 B | events 1, 6 |
| Kind 128 sub-types | 86 `prop_flag_swap` (588 B, shared with Chapter 2), 87 / 95 `prop_ch3_machine` 0x3F0D0 (708 B), 94 `prop094_r3_window` (80 B), 96 `prop096_koba` (112 B), 121 `prop121_kikai_item` (412 B) | 1,900 B | |
| Hostages | scenarios 12-15: `s12_cell` (596 B), `s13_platform_gift` (1,020 B), `s14_message_route` (776 B), `s15_gift` (992 B) | 3,384 B | event 0 s12, 1 s13 / s14, 2 s14 / s15 |

## Bosses

Both bosses go through the **action-script engine** that the Chariot uses (ported: `boss_chariot.cpp`, 14.8 KB, and the
armour / part damage; docs/decompilation.md "Bosses"), so the sizes below are the boss-specific parts only.

**Hangedman** (kind 18, Chapter 2 event 15, PC `boss_hangedman` 0x498F84): `boss_hangedman` (ported) dispatches through
0xC1190 by the record's set-up (+0x64): 0 = the fly-by (ported, Chapter 1 event 3 / Chapter 2), **1 = `hangedman_fight_init`
0x4F960** (state 3, plus `hangedman_captives`: a woman and a researcher hanging from him), 2 = `hangedman_rematch_init` 0x527A0
(Chapter 4). 15 states (table 0xC1150, state +0x5C). Routines (missing): `hangedman_fight_frame` 0x4FAA0 (416 B),
`hangedman_hit` / `_hit_reaction` / `_vuln_clear` (432 / 704 / 40), `_death_fall` 0x50110 (716), `_intro` 0x503E0 (772),
`_flap` 0x50700 (512), `_hover` 0x509F0 (544), `_rejoin` (180), `_claw_attack` 0x50C10 (1,032), `_return` (620), `_flee`
(780), `_wait_reappear` 0x515B0 (1,392), `_dive_approach` 0x51B40 (1,108), **`_dive_attack` 0x51FA0 (2,040)**,
`_rematch_init` / `_rematch_intro`, `_hover_point` (644), `_turn_toward`, `_explosion`, `_death_effect`, `_captive_frame`
0x70190 (532), `_captives` (224), `_intro_effect(_spawn)` 0x31120 / 0x31160, `boss_aggression_init / _update` 0x305A0 /
0x305D0 (the level goes up every 1,200 frames, shared with the Magician), `boss_life_bar_task` 0x52C30, `camera_set_direct`
0x301E0. The bat swarm (kind 67 / bats, "hangedman_bat_waves") is ported. Its motions are `MO_b2_*`. Needs, outside the logic: a 2D life bar and an intro card (`sprite_place` is a
boundary stub today; the Chariot's life bar / title card are the template, `src/port/boss_chariot.cpp`).
**The same 14.4 KB give Chapter 4 its rematch** (set-up 2), so it is not wasted.

**Hermit** (kind 40, Chapter 3 event 7, `boss_hermit` 0x85430): same engine, "differs only in the action tables"
(decompilation.md). Missing: `boss_hermit` (284), `hermit_entrance` (236, out of the hole), `hermit_frame_outer` / `_frame`
0x85700 (192 / 840), `hermit_act1_pose`, `_wait_combat`, `_turn`, `_step`, `_walk` (172 / 160 / 260 / 224 / 388), guard
acts 22-24, **`hermit_act20_spit` (740) with `hermit_spit_ball` (880) + `_shot` / `_draw`**, `act21_lunge`, `act_attack`
(400), **`act31_drop` (668)**, `act32_hit_reaction` + pick (276 / 108), `hermit_hits_and_draw` (392), `hermit_part_draw_hook`
(528), `hermit_title_task` (360: the title card), `hermit_life_bar_task` (184), drop dust / sounds / death sink. The event
waits on item 125 and on enemy count; the Curien scene (kind 24) closes the chapter.

Weak-spot aiming: both bosses need a boss-aware auto-aim in the proof harness (`-AimPrefer` exists) and in the bot
(src/game/bot.cpp has no boss handling).

## Needed / have / proven / missing (systems)

| Needed | Have | Proven | Missing |
|---|---|---|---|
| Event interpreter, all 60+ opcodes, branch tables, route choice | yes (`port/event.cpp`) | A: every frame of Chapters 1-2 of the full-game recording; Chapter 3 events only through the three Chapter 3 recordings of other systems | Chapter 3's events were not replayed against MAME (a run_capture `-Chapter 3` game) |
| Camera (jobs 16-21, 32, 33, 64, CAMDR 1-3, paths) | yes (`port/camera.cpp`) | A, Chapter 1 | paths of Chapters 2 / 3 are data (CAMALL), same code; `camera_set_direct` 0x301E0 for the Hangedman |
| Creature core, enemy frame, guard actions 1-20, kinds 0-12 (spawn family), 32, 64-70, 96-99, kind 70 all set-ups | yes | A / PA / PM (proof.md) | the Chapter 2 / 3 types and actions above are data on proven code; kinds 3 / 7 need `enemy_fall_death_land`, kind 10 `kind10_remove` |
| Hostages | scenarios 0-6 | A (s6 and a few states unverified) | **s7-s15: 6.5 KB** |
| Props, doors, scene objects | Chapter 1's | A, Chapter 1 | the kinds and sub-types in the tables: **~8 KB** (Ch 2) / **~4.5 KB** (Ch 3) |
| Bosses | Chariot, Hangedman fly-by | Chariot A | Hangedman fight 14.4 KB, Hermit 8.9 KB + Curien |
| World, textures, sky, zones | `hotd_viewer` shows all four chapters; `GameView::init(chapter)` / `ArcadeSource::loadStage(chapter)`; texture areas from the ROM (table 0xAB0B0 by chapter), zone lists (0xC0BB0 / 0xC0BD0 / 0xC0BF0 by chapter) | viewer vs MAME screenshots, all four chapters (formats.md) | in-game check of the chapters' worlds, skies, lights, palette animation, colour rows (the Chapter 2 pool was yellow once, formats.md) |
| PC models (PC world + models) | `pc_loader` loads WORLDn.RBH per chapter | Chapter 1 | the PC side of the new kinds' models (bosses: check `CREAT*.RBH`); the design falls back to the arcade model for a category the PC lacks |
| Music / sound | cue table, OGG tracks | Chapter 1 | cues 10 (Chapter 2) and 16 (Chapter 3) are in the table; the Hangedman / Hermit voices go through the same `sound_cue` |
| Chapter start, select, flow | `Runtime::chapter_start(chapter, event, players, mode)` takes any chapter; the title / attract / prologue are Chapter 1 | Chapter 1 | below |

## What changes in the runtime (the chapter-start path)

`Runtime::chapter_start` (src/game/chapter.cpp) already sets `chapter`, resets the game state and loads the texture area
from table 0xAB0B0 by chapter. What is Chapter-1 only today:

1. **The start.** `src/main.cpp:914` calls `chapter_start(<chapter in RAM = 0 after boot>, event, players)`; there is no
   `--chapter`. `attract.cpp:1128` (the title's start button) calls `chapter_start(0, 0, players)`. The launcher
   (`LaunchConfig`, launcher.cpp, "Teaser limits: Chapter 1 only" text at main.cpp:1174) has no chapter. Needed: `--chapter N`
   (1-4) and `LaunchConfig::chapter`, passed to `chapter_start`, to `GameView::init(chapter)` (it already reads RAM
   `chapter`, so setting it first is enough) and to the title (start button -> the chosen chapter, or no title when a
   chapter is chosen).
2. **The end.** `Runtime::game_chapter_end_wait` (clear.cpp:52) turns the original's "load chapter + 1" into `chapterDone`
   (the host shows "CHAPTER n CLEAR / Thank you for playing the teaser", main.cpp:1808, hud.cpp:617; the name entry only
   at the end of the game). For the release: after Chapter n clears, **continue into n + 1 if it is playable** (the
   original keeps lives, score and the players' state: `chapter_start` resets them, so a `keepPlayers` variant is
   needed), else the end screen / name entry; the end-of-game wording changes from "the teaser".
3. **Per-run state.** `GameView` is built once per run (`view->init`, main.cpp:1241 and the restart path ~983): a chapter
   change inside a run re-initialises the view (stage, texture sets, sky) as the attract's `prefetch` does; a game-over
   restarts the chapter that was running (it already does, through the RAM chapter).
4. **Test aids that assume Chapter 1:** `tools/checkpoints.txt` (event names of Chapter 1), `--default-route` (works for
   any chapter: route_choice 0), `--route C:E[=V]` (validated against the branch table of any chapter), `--event E` (starts
   with draw zone 0 until the event's first BGTB), the T1 branch scripts (`tools/t1_branches.ps1`), `tools/robust_seeds.ps1`
   (needs a `--chapter`), the MAME side (`run_capture.ps1 -Chapter 2|3` exists: `HOTD_START_STAGE`).
5. **Teaser cuts.** `cuts.guards` (an event wait that only an unported object can satisfy is passed, logged): useful
   during development for Chapters 2 / 3 (they run, unported kinds are logged no-ops), but a release chapter is only
   marked playable when it runs with `--strict` (no guard passes) in the bot games.
6. **Chapter select + gating.** See below.

### Chapter select in the launcher

One extra line "Chapter: 1 2 3 4" (click or Left / Right); the shape is the existing launcher's (value lines, `Hot`
regions, launcher.ini key `chapter=`). State per chapter lives in one table `kChapters[4]` (new `src/game/chapters.h`:
`playable`, a short note): **a chapter is enabled only after its gate passes** (a tool `tools/chapter_gate.ps1`: N bot games
of that chapter, both model sources, 1 and 2 players, `--strict`, no problem / stall, clear reached; the release commit
sets the flag), the others are drawn grey with "after Halloween" / "not finished" and cannot be selected: never broken.
Chapter 4 is grey. Start with the chosen chapter = a new game there (lives from the setting, score 0, as the arcade's
chapter_load does for a new game). Open question for the user: "selectable once it plays through" is read here as *the
build's* readiness, not the player's progress; a player-progress unlock ("cleared" marks in launcher.ini, later chapters
locked until cleared) is a small extra on the same table and can be switched on if wanted.

## Suggested split into parallel workers (file ownership)

Rules as in the overnight rules (own worktree + branch + `build-<name>`; nothing from `private\`; commit per batch; UNVERIFIED
marks `// UNVERIFIED (teaser)`; status file `docs/teaser_status/<name>.md`; the coordinator merges). Shared append-only
points: the include lists in `src/port/game.h` (decl headers), `src/port/game.cpp` (inst), `src/proof/main.cpp` (bounds,
cases) and `tools/ghidra/names.tsv`; one line each, rebase before commit, one worker runs the names.tsv -> generated
headers chain at a time. New kinds follow docs/port.md "Adding a kind" (a `decl/<x>.h`, `inst/<x>.inc`, `src/port/<x>.cpp`,
`proof/cases/<x>_arcade.inc`, `proof/bounds/<x>.inc`; `CMake` globs the decl / cases files).

| Worker | Task | Owns (new files unless noted) | Size |
|---|---|---|---|
| **select** (first, Mon evening) | `--chapter N`, `LaunchConfig::chapter`, the launcher line + greyed entries, `kChapters[]`, `keepPlayers` chapter-to-chapter flow, end screen wording, view re-init per chapter, `docs/teaser_status/chapters.md`; then: headless Chapter 2 / 3 runs with the cut guards to list every stall and unported call (a boundary report per chapter: the others' work queue) | `src/main.cpp`, `src/game/launcher.*`, `host.*`, `chapter.cpp`, `clear.cpp`, `flow.cpp`, `boot.*`, new `src/game/chapters.h`, the title's start in `attract.cpp` (one line) | small logic, mostly plumbing |
| **hanged** (Chapter 2 boss) | set-up 1 + all states, captives, intro, aggression, life bar, kind 19, the event-15 items 192-199 / 252-254; set-up 2 comes cheap (Chapter 4) | `src/port/boss_hangedman.cpp`, `decl/hangedman_fight.h` + inst / cases / bounds (`hangedman_flyby.cpp`, `decl/hangedman.h` stay untouched) | 14.4 KB (30 routines), the biggest single item |
| **ch2** (Chapter 2's own) | kinds 150, 131-133, 136, 151, 152, 168, 183-185, 187, 75; kind 3 / 7 `enemy_fall_death_land`, kind 10 `kind10_remove`; the 9 prop sub-types; hostage scenarios 7-11; route-object kinds (183 first: the route choice) | `src/port/ch2_kinds.cpp`, `props_ch2.cpp`, `hostages_ch2.cpp` + their decl / inst / cases; lands kind 150 and `prop_flag_swap` first (Chapter 3 needs them) | 16 KB direct (kinds 9.5 + props 3.4 + hostages 3.1) |
| **hermit** (Chapter 3 boss) | the Hermit with its title card, life bar, spit ball; Curien (kind 24; kind 25 for Chapter 4) | `src/port/boss_hermit.cpp`, `decl/hermit.h` + inst / cases / bounds; reads (does not edit) `boss_chariot.cpp` for the shared engine | 9.5 KB |
| **ch3** (Chapter 3's own; starts when ch2 has landed 150 or in parallel without it) | kind 153 Taran, 106, 186, 188, 189; prop sub-types 87 / 94-96 / 121; hostage scenarios 12-15; Chapter 3 route objects | `src/port/ch3_kinds.cpp`, `props_ch3.cpp`, `hostages_ch3.cpp` + their decl / inst / cases | ~10 KB |
| **integ** (from Wed 10-21, when select + the first kinds exist) | bot play-throughs per chapter (seeds, both model sources, 1-2 players), the gate tool, per-chapter checkpoints (`tools/checkpoints.txt` runs + MAME references via `run_capture -Chapter`), route enumeration, boundary / unverified reports, crash fixes (small edits in `src/port`, noted in its status file) | `tools/*`, `docs/teaser_status/ch2.md` / `ch3.md`, `src/game/bot.*` (boss weak-spot aim), `src/game/t1_bounds.cpp` | no new routines |
| **look** (optional, parallel from Mon; uses `hotd_viewer` until `--chapter` lands) | Chapter 2 / 3 worlds in game: sky, zones, lights, palette animation, colour rows, PC world; boss models on both sources | `src/render/**`, `src/game/view.*`, `src/assets/**` (not `chapter.cpp`) | verification and fixes |

If the budget cannot carry seven workers, drop in this order: look (the viewer already shows all stages), ch3 (move it
after hermit in the same session), integ (the select worker does the headless runs). Chapter 2 is the priority line:
**select, hanged, ch2 first**; hermit and ch3 follow, because Chapter 3 is "if possible".

### Proofs

Unverified is allowed (as for Chapter 1); proofs follow where cheap. Material: the full-game MAME recording
`play_us_20261007_2238` passes both bosses (Hangedman, Hermit) and all hostages of the player's route; the Chapter 3
recordings `play_us_ch3_*`; fresh auto-aim games for any chapter: `run_capture.ps1 -Chapter 2|3 -Route "2:5=1,..." -AimPrefer
<routines> -AimSkip <routines>` (`HOTD_START_STAGE`). The usual rule: routines not reached by a capture are marked UNVERIFIED
and listed by `tools/unverified_list.py`; the release notes count them.

## Timeline (from Mon 2026-10-19, 4 PM, to the release ~10-28)

- **Mon 10-19 evening:** select starts (`--chapter 2` headless runs by ~10 PM); hanged, ch2, hermit start reading and
  porting; look starts. **Gate 1 (Tue night):** `hotd --chapter 2 --frames N --default-route` and `--chapter 3` run through
  every default-route event with logged no-ops, and the boundary reports exist.
- **Tue-Thu 10-20..22:** logic rounds; ch2 lands kinds on the default route first (183, 136, 133, 132, 131, 168, hostages 7-10,
  props 14 / 30 / 31 / 64 / 67 / 78), hanged lands the fight; integ starts bot games of Chapter 2.
- **Fri 10-23:** **Gate 2:** Chapter 2 plays start to finish headless (Hangedman killable, `--strict`). If not, Chapter 2 is
  the only thing everyone works on over the weekend; Chapter 3 gets only what is left.
- **Sat-Sun 10-24..25:** Chapter 3 integration (Hermit, Taran, route), bot games and checkpoints of Chapters 2 and 3, the
  branch routes of Chapter 2 (10-14), gate tool runs. **Gate 3 (Sun night):** Chapter 3 playable or greyed out.
- **Mon-Tue 10-26..27:** release pass (docs/release: notes, README "Chapters 1-3", launcher texts, package, clean-profile
  test), buffer. Branch events (Chapter 2 events 6, 10-14; Chapter 3 events 2, 4, 5) are stretch: a branch event with a
  missing kind runs through the cut guard only in development builds; for the release the route objects are checked so
  that the player cannot enter a broken branch, or the missing kind is finished.

## Risks (honest)

- **Bosses unproven and unaimable by the bot.** Both fights are ~23 KB of new, uncaptured code driven by item flags the
  events wait on; a wrong flag stalls the chapter. Mitigation: record-and-compare against the full-game recording (it plays
  both fights), the event-15 / event-7 item trace, boss-aware aim in the bot.
- **Route objects.** Branches are chosen by shootable objects (the 0x3A40 family: kinds 183, 186, 188, 189 are the
  candidates); a non-default branch can reach kinds not yet ported. Default-route games are the gate; branches follow.
- **Hidden size.** The pointer scan covers tables of routine pointers, not computed jumps; +10-20 % on the numbers above.
- **World / look of Chapters 2 and 3 in the game** has not been run through the runtime at all (the viewer matches MAME,
  but zone lights, palette animation, the colour-row swaps of Chapter 2's pool and Chapter 3's special cases at 0x51E9A1..
  were only checked in the viewer, formats.md §28).
- **Shared files.** Seven workers append to four shared include lists and names.tsv; keep commits small, rebase often.
- **Budget.** The weekly budget decides; the cut line is Chapter 3 (greyed out), then Chapter 2's branch events, never
  a half-working chapter in the selectable list.
