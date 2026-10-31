# Halloween release: Chapters 1-3 (user, 2026-10-17)

**New goal for ~2026-10-28:** Chapters 1 and 2 definitely, Chapter 3 if possible, Chapter 4 after Halloween, and a
**chapter select in the launcher** (a chapter is selectable once it plays through; unfinished ones are greyed out,
never broken). Chapter 1 is done (below: the teaser plan, written 2026-10-15, kept as history). The gap analysis, sizes
and the worker split are in [chapter2_3_plan.md](chapter2_3_plan.md); in short:

- **Missing** (arcade code, `py tools/chapter_gap.py`): Chapter 2 **30.5 KB** (Hangedman 14.4 KB + 16 kinds 9.5 KB +
  hostages s7-11 3.1 KB + 9 prop sub-types 3.4 KB), Chapter 3 **23.3 KB** (Hermit 8.9 KB + Curien 0.7 KB + 8 kinds 8.5 KB +
  hostages s12-15 3.4 KB + 6 prop sub-types 1.9 KB), 19.7 KB of it on top of Chapter 2. Of 551 KB, 307 KB is ported.
  The interpreter, the camera and every event opcode already run Chapters 2 and 3; no new format or engine system.
- **Plan** (from Monday 10-19, 4 PM; new weekly budget): parallel workers per chapter: *select* (chapter start / select /
  flow, first), *hanged* (Chapter 2 boss), *ch2* (Chapter 2's kinds, props, scene objects, hostages, world), *hermit*
  (Chapter 3 boss), *ch3* (Chapter 3's own), *integ* (bot games, gate tool, checkpoints), *look* (worlds in game).
- **Gates:** Tue 10-20 night: `--chapter 2` / `3` headless runs through the default routes (stall list); Fri 10-23: Chapter 2
  plays start to finish `--strict`; Sun 10-25 night: Chapter 3 playable or greyed out; Mon-Tue 10-26/27: release pass.
  The cut line is Chapter 3 (greyed), then the branch events; never a half-working chapter in the select.

---

# Halloween teaser: what is left, and the plan (written 2026-10-15)

Target (ROADMAP "Milestone: Halloween teaser"): **Chapter 1 start to finish, playable on Windows, arcade or PC
models, local co-op, around 2026-10-28.** Unproven routines are allowed but marked unverified.

How this was measured: Chapter 1's 20 event scripts disassembled (`tools/evt_dis.py`, list 0), every spawn record
(ENTB / ENTB3 / ENTBE: 40-byte records, kind at +0, type = byte +0x24; ENTB2: short scene-object entries) mapped
through the arcade kind table (0xAFDE0) to its routine; then, per system, the arcade call graph
(`private/decomp/arcade/maincpu_index.tsv`) walked from its roots, stopping at routines already ported in
`src/port/game.h`. Sizes are arcade i960 bytes. **They count direct calls only:** routines reached only through
tables (state routines set as object +0, scenario tables) are partly missed, so real sizes are about 20-40 % larger.
For scale: everything ported so far is **93 KB** of arcade code (322 routines, 2026-10-13..15); the whole program
is 551 KB.

## Chapter 1: events, routes and what they spawn

Branch table (arcade 0xAABE0): event 0 -> 1 / 2, 3 -> 4 / 9, 4 -> 5 / 7, 5 -> 6 / 12, 7 -> 8 / 16, 9 -> 10 / 11,
11 / 12 -> 15 / 17, then 6, 8, 13, 14, 18 -> 19 (the Chariot); 10 -> 14, 15 / 16 -> 18, 17 -> 13. Event 8 is empty
(shared code). `route_choice` 0 everywhere gives the **default route 0, 1, 3, 4, 5, 6, 19** (the full-game
recording's route); the other events are reached by shooting route objects or by rescues.

| Event | Kinds spawned (object kind numbers; scene objects from ENTB2 in brackets) |
|---|---|
| 0 | 99, 100, 128, 181 [161, 172, 173, 177, 178] |
| 1 | 0, 96, 99 [173] |
| 2 | 102 [173] |
| 3 | 0, 11, 18, 96, 97, 98, 100, 128, 160 [17, 161, 173, 178] |
| 4 | 0, 32, 34, 70, 96, 128 [69, 161, 163, 164, 173, 178, 180] |
| 5 | 0, 2, 21, 64, 70, 134, 162, 169, 175 [164] |
| 6 | 0, 21, 32, 33, 35, 38, 64, 65, 70, 76, 77, 96, 98, 100, 128, 135, 137, 139, 140, 147, 162, 169, 175 [69, 161] |
| 7 | 0, 1, 32, 34, 64, 65, 70, 128, 162 [164, 166] |
| 9 | 0, 11, 32, 64, 65, 66, 96, 97, 101, 128, 144, 160 [67, 69, 161, 173] |
| 10 | 0, 66, 143, 148, 149, 162 |
| 11 | 0, 11, 32, 64, 97, 128, 141, 142, 145, 160, 162 [178] |
| 12 | 0, 162 [178] |
| 13 | 0, 1, 4, 21, 32, 38, 66, 70, 98, 100, 128, 137, 138, 139, 146, 162, 169 [167] |
| 14 | 0, 64, 65, 70, 76, 77, 128, 135, 140, 162 [161] |
| 15 | 0, 70, 97, 99, 100, 162 |
| 16 | 0, 32, 70, 162 [178] |
| 17 | 0, 1, 70, 128, 162 [166] |
| 18 | 0, 21, 32, 38, 70, 98, 100, 128, 137, 139, 162, 169, 175 |
| 19 | 0, 37 (the Chariot, 480 hit points), 128, 137, 162 [16 Sophie, 78, 161, 170, 176, 178] |

Event opcodes Chapter 1 uses (38): ATCMD BGTB BSFT CAMDR CAMDR2 CCTB CHGSM CLTB ENTB ENTB2 ENTB3 ENTBE ET1P2 ET1P3
ET2P ET2P3 ET2PE FKRTB GGCOL GGSET ITEMON KGTB NEXTEV RNWCJ SCNINIT SHDWLT SKIP SKYKD SKYTB SNDON TEXCHG
WAIT_CAMFRM WAIT_CAMJOB WAIT_ENALV WAIT_ENNUM WAIT_FRM WAIT_ITEM WLTB. Camera paths 0-31 (CCTB 64).

## Needed / have / proven / missing

"Missing" = arcade code not ported yet, from the call-graph walk (direct calls; see above). A = arcade,
PA / PM = PC Arcade Mode / PC Mode.

### Game logic (src/port, `Game<P>`)

| Needed | Have | Proven | Missing |
|---|---|---|---|
| Coprocessor model (A maths) | yes (`port/tgp.*`) | A, 3.3 M calls | - |
| Shooting: gun -> ray -> nearest hit | yes | A, PA, PM | gun hardware read / calibration (replaced by our input) |
| Damage, part hits, kill / part scores | yes | A, PA, PM | effect spawners (blood, sparks, blow-off) are boundaries, in "helpers" below |
| Standard enemy frame (`enemy_main`, `enemy_ai`, hit reaction, death) | yes | A, PA, PM | - |
| Kind 99 `enemy_guard` + actions (events 0, 1, 15) | yes | A, PA, PM (actions 1, 6, 13, 16, 20 written, not reached) | - |
| Kinds 0, 1, 2, 4, 11 (spawn family) | yes | A, PA, PM (1, 4 written, not reached on A) | - |
| Kinds 32 (kit zombies), 33-35 (Murrer), 64 (researchers), 65, 66 / 67 (bats), 69 (frog), 97 (dogs) | yes | A, PA, PM (gaps listed in proof.md) | dog helpers (boundaries, in "helpers") |
| Kind 96 `kind096_zombie_walker` (events 1, 3, 4, 6, 9: **the first zombies of the game**) | no | - | 0.2 KB (+ creature core) |
| Kind 98 `enemy_guard_b` (events 3, 6, 13, 18) | no | - | 0.04 KB (calls into enemy_guard) |
| Kinds 101, 102 (branch events 2, 9 only) | no | - | 0.7 KB |
| **Creature core**: `body_init`, `set_motion` family (blend, second motion, at-frame), `part_init`, `creature_type_setup`, part alloc / free | boundaries | - | **~3.7 KB**; every spawn needs it |
| Part matrices (`body_draw` 0x37720: poses the parts; the shot test reads these matrices) | boundary | - | 1.1 KB (+ 0x36240); its geometry output becomes our draw list |
| Enemy helpers now boundaries (attack slots, splines, jump arcs, floor / ground probes, turn and walk helpers, dog helpers, effect spawners, `player_hit`, projectile hits) | boundaries | - | ~17 KB (of the 26 KB boundary list; ~5 KB more is drawing, done by the engine instead) |
| **Event interpreter** (`evt_run`, the 38 opcodes, NEXTEV + branch tables, SCNINIT, ENTB*) | yes (`port/event.cpp`, L1, 2026-10-15; A and PC forms) | A: every frame of Chapters 1-2 + all 20 Chapter 1 events by route games, 0 differ (proof.md) | - (camera drive modes are L2's) |
| **Camera** (camera task, CCTB job ring, jobs 16-21 / 32 / 33 / 64, CAMDR modes, path evaluation, camera matrices) | engine plays CAMALL paths approximately | no | **7.8 KB** |
| Game flow: `frame_tick`, `game_frame` (task groups, `shot_resolve`, attack slots), mode 4 game, mode 5 game over / continue | no | - | ~3 KB needed of 8.7 KB reachable (the rest is boot, text printf, sound board init: the engine replaces them) |
| Task system (`task_create`, `_child`, `task_exit`, `task_group_run` with its setjmp-like unwind point 0x23B00) | boundaries | - | ~1 KB; in the runtime (see risks: tasks can **yield**) |
| Player, gun input, lives, HUD logic (`player_input_update`, `gun_input_update`, `player_hit`, `player_start_check`, `player_life_add`, `hud_lives_draw`) | `player_shoot` only | `player_shoot` A, PA, PM | 3.6 KB |
| Hostages, kind 100 (events 0, 3, 6, 13, 15, 18; 26 scenarios) + voice lines (kind 161) | no | - | 2.5 KB direct + scenario states (~4 KB real) |
| **Kind 70** (Saruzou monkeys; the same engine runs Drake / Moody / kageta in later chapters) (events 4-7, 13-18) | no | - | **30.9 KB** for all types; Chapter 1 needs the monkeys: est. 18-22 KB (set-ups to be read from the ENTBE records) |
| **Chariot**, kind 37 (event 19) + life bar + title card | no | - | **14.8 KB** |
| Hangedman fly-by (kind 18 set-up 0, event 3) | no | - | ~1-2 KB (`hangedman_flyby_*`) |
| Props and scene kinds: 128 `prop_object` + tumbling pieces, 21, 38 barrel, 76 / 77, 134-149, 160 splash, 162 door, 169, 175 rat, 181 car | no | - | 12.4 KB (default route: 128, 134, 135, 137, 139, 140, 147, 21, 38, 76, 77, 160, 162, 169, 175, 181 ≈ 9 KB) |
| Scene kinds from ENTB2: Sophie 16, 17, 78, 161, 163-167 doors, 170 glass, 172 opening title, 173, 176 chapter clear, 177 chapter card, 178 skip, 180 | no | - | ~2.6 KB direct (+ Sophie's scene chain) |
| **Total missing logic** | | | **~94 KB direct + ~17 KB helpers ≈ 110-130 KB real** (default route only: ~95-110 KB) |

### Engine (src/assets, src/render, src/game, src/main.cpp)

| Needed | Have | Missing |
|---|---|---|
| Window, OpenGL renderer, Model 2 shading, PC / arcade world, sky, draw zones, texture areas, palette animation, per-category model switching | yes, in `hotd_engine` (used by `hotd_viewer`) | reuse in the game executable |
| Creature meshes, skeletons, 674 motions | yes (`arcade_anim`, PC CREAT*.RBH) | draw from the **port's object state** (below) |
| Game executable | `hotd` = `src/main.cpp`, an empty SDL window | **the runtime host**: map the original address space from the player's files, run `Game<P>` 60 times a second, implement the boundaries (tasks, drawing, sound, input) |
| Drawing the game's objects | viewer poses creatures itself | a draw list filled from the port: `geo_draw_model` / `geo_matrix_position` (the one funnel every model goes through) with the current coprocessor matrix, or the PC's `pc_matrix_position_draw`; creatures, props, effects and Sophie then all appear without per-kind code |
| Camera | viewer's free camera / approximate ride | read the ported camera block (work RAM 0x51EA50..) |
| Input, 2 players | mouse look only | per-player crosshair from mouse 1, second mouse (SDL3 mouse IDs), or keyboard; trigger, off-screen reload, start / join |
| Audio | none | SDL3 audio + mixer; `sound_cue` through the PC cue table (THOTD.EXE 0x5CBDA8, 12 bytes per cue) to `SOUND\GAMESNDS.RBH` / `O_*.RBH` (**format not decoded**) and OGG music (`Music\TrackNN.ogg`; needs a decoder, e.g. stb_vorbis) |
| HUD / 2D | debug-font overlay | crosshairs, bullets, torches (lives), score, CONTINUE / PRESS START; sprites from the PC's `SPRITE\SPR.RBH` (**not decoded**) or the arcade's 2D layer; fallback: a plain HUD of our own, labelled placeholder |
| Front end | none | a minimal launcher (data paths, model source, P1 / P2 input) that starts straight in Chapter 1; no attract / title for the teaser |
| Packaging | `tools/make_portable.ps1` (viewer) | the same for `hotd`; README for a public repo (no game data) |
| Official-files check (user, 2026-10-15) | hashes in formats.md (PC files) | the launcher checks that the player's files are the official releases, by **content only**: names of folders and archives are free (the player picks any folder, e.g. "hotd pc dump"; any zip name). Arcade: match every ROM by CRC32 / SHA-1 against MAME's `hotd` Revision A set, whatever the zip or file is called (zip or loose files). PC: the files the rewrite reads, found by their path inside the chosen folder (case-insensitive), checked by hash; a file not found there (renamed or moved) is looked for by size + hash among the folder's files, once, and the result remembered; a patched install passes as long as those files are unchanged. A mismatch gives a clear message (which file, which release expected), never a silent failure. Part of R4 |

## Decision: which logic runs the teaser

**Decided by the user (2026-10-15): arcade logic (A); several sessions in parallel are fine as long as the weekly
usage stays under 85 % (18 % at the time of the decision).** The options as they were put:

`Game<P>` is one template for both versions, but the runtime host maps one address space and needs that version's
data: **A** needs the arcade program ROM (the player's dump), **PC** needs THOTD.EXE's data sections and the PC
heap layout.

- **Recommended: A (arcade logic) for the teaser.** Every routine is proven on A first, the coprocessor model is
  exact, MAME captures run several at a time (the PC harness runs one game at a time), and the event scripts and
  tables come straight from the ROM. Models stay switchable (PC or arcade), so "arcade or PC models" still holds.
  Cost: the teaser needs the arcade dump; **PC-only owners cannot play it** (the ROADMAP's "PC only must work
  fully" is then met after the teaser).
- Alternative: PC logic. Works for PC-only owners, but needs every new routine checked on the PC as well (serial
  harness), the PC address space set up (EXE image, fixed heap blocks), and the PC-only helpers that are still stubs.
  Roughly +3-4 sessions.

## Critical path

1. **Runtime host** (address space, task system, frame loop, draw list from the port) - nothing is playable
   without it.
2. **Event interpreter + camera** ported - they drive everything along the rail.
3. **Creature core** (body_init / set_motion / part matrices) - every spawned creature, and the shot test, needs it.
4. **Kind 70** (largest kind on the default route: events 4, 5, 6) and the **Chariot** (the chapter's end).
5. Input for two players, player hits / lives / continue, HUD, sound.

Everything else (hostages, props, doors, scene objects, Hangedman fly-by, kinds 96 / 98) is many small pieces that
parallel sessions can take in any order once (1)-(3) exist.

## Plan in sessions (12 sessions + buffer, 2-3 at a time)

Rules for parallel sessions:
- **One writer per file.** Logic sessions each write a new `src/port/<system>.cpp`; the runtime session owns
  `src/main.cpp`, `src/game/`, `src/render/`, `src/assets/` and `CMakeLists.txt`. `src/port/game.h` and
  `tools/ghidra/names.tsv` are shared: append-only edits, small commits, rebase before committing; one session
  runs the names.tsv -> generated-headers chain at a time.
- **Own build folder per session** (`build-<name>`), never another session's.
- MAME captures can run side by side; the PC harness one at a time (not needed for the teaser if A is chosen).
- **Unverified marking:** every routine written without a capture proof gets `// UNVERIFIED (teaser)` on its line in
  `game.h`; a small script lists them (count shown by the launcher and in the release notes). Proofs follow after the
  teaser, as the proof harness already allows.

| Round (dates) | Session A: runtime / engine | Session B: logic | Session C: logic |
|---|---|---|---|
| 1 (10-15 .. 10-16) | **R1 runtime skeleton** (first task below) | **L1 event interpreter** | **L2 camera** |
| 2 (10-17 .. 10-18) | **R2 the ride**: chapter start through `evt_chapter_start`, ported interpreter + camera driving the renderer; 2-player input into the gun-input layer; shots | **L3 creature core + part matrices**, kinds 96 / 98 / 160, `player_hit` and the player block (lives, invulnerability) | **L4 kind 70, part 1**: spawn, frame, hits, death, monkey think, the set-ups of events 4-6 |
| 3 (10-19 .. 10-21) | **R3 audio + HUD**: SDL3 mixer, PC sound RBH decode, OGG music, cue table; HUD (PC sprites or placeholder), crosshairs; game over / continue (mode 5) | **L5 hostages + scene objects**: kind 100 scenarios on the default route, 161 voice lines, doors 162-167, title / chapter cards 172 / 177 / 178, 173, 180, Hangedman fly-by | **L6 kind 70, part 2 + props**: remaining kind-70 states of Chapter 1, 128 + tumbling pieces, 21, 38, 76 / 77, 134-147, 169, 175, 181 |
| 4 (10-22 .. 10-24) | **R4 game flow and polish**: chapter clear (176), P2 join mid-game, settings (difficulty, lives, blood colour), model-source switch, launcher with the official-files check, portable package | **L7 the Chariot**: kind 37 actions, armour, part damage, death, life bar, title card; Sophie's event-19 scene (16, 78, 170) | **T1 integration**: full default-route play-throughs, both model sources, 2 players; crash / stall fixes; unverified list; captures for the cheapest unproven routines |
| Buffer (10-25 .. 10-27) | fixes from the user's play-test | **stretch:** branch events 2, 7-18 (kinds 1, 4, 101, 102, 66 / 67, 141-149, 145 ...) | release notes, README, public-repo check (nothing from `private/`) |

Round 1's three sessions are independent: R1 runs on proven code only (snapshot start, below), L1 and L2 are new files
proven by their own captures. From round 2 on, the runtime session integrates whatever the logic sessions have
committed.

**Cuts, in order, if behind:** (1) default route only, with a teaser option that keeps `route_choice` at 0 and turns
unported kinds into a logged no-op task instead of a crash; (2) skip proofs for props and scene objects; (3) kind 70
monkeys only; (4) placeholder HUD; (5) PC sound files only, no arcade sound board; (6) no title / attract (already
planned).

## Risks (honest)

- **Size.** ~110-130 KB of logic is left against 93 KB ported in the last three days (proofs included). Drafting
  without proofs (`tools/ghidra_to_cpp.py`) is faster, but 12 sessions only fit if three run in parallel most of the
  time, and **parallel sessions burn the weekly usage limit three times as fast** (stop at 85 %). If the week's
  budget runs out, the date slips, not the scope rules above.
- **Kind 70** is 31 KB for all its types; the Chapter 1 share is an estimate until the ENTBE records' set-ups are
  read. It is on the default route three times (events 4, 5, 6), so it cannot be cut.
- **Task system.** `task_group_run` unwinds through a setjmp-like point that returns "exited" **or "yielded"**: if
  some routines yield in the middle (a coroutine wait), the C++ needs fibers or a rewrite of those routines into
  state steps. To check first thing in R1.
- **Unknown formats:** PC sound RBH and `SPR.RBH` are not decoded (each probably a session's quarter, possibly
  more). The placeholder HUD and a silent build are the fallbacks.
- **Hidden sizes:** the call-graph walk misses table-reached routines (scenario tables, state tables), +20-40 %.
- **Shared files** (`game.h`, `names.tsv`) between 3 writers: merge conflicts, regenerated headers.
- **Unverified code in a public teaser:** a crash or stall (a WAIT_ENALV waiting on an enemy that never dies) ends a
  play-through; T1 and the cuts above are the defence.

## First three tasks (round 1, can start at once, in parallel)

1. **R1 runtime skeleton** (owner of `src/main.cpp`, `src/game/runtime.*`, CMake target `hotd`): map the arcade
   address space from `private\hotd.zip` through the existing arcade loader (program ROM at 0, main_data at
   0x2000000, work RAM 0x500000); `class Runtime : Game<A>` with real `task_create` / `task_create_child` /
   `task_exit` (C++ exception back to the task loop), `task_group_run`, `call_routine` through the per-kind
   registries, `copro` on a live `Tgp`; first check whether tasks yield (risk above). Start from a **work-RAM
   snapshot** of the full-game replay inside Chapter 1 event 1 (`private\mame_dumps\fullgame_us` or a fresh dump)
   instead of booting, run `game_frame`'s task groups every 1/60 s, draw the world at the snapshot's camera and the
   creatures through a `geo_draw_model` draw list (or, as a first step, from object type / motion / frame with
   `arcade_anim`), and shoot with the mouse through the proven `player_shoot`. **Done when** the event-1 zombies
   walk, attack and die to mouse shots in the window.
2. **L1 event interpreter** (`src/port/event.cpp`): `evt_task`, `evt_run`, `evt_chapter_start`, the 38 Chapter 1
   opcodes (the CCTB ring hand-off to the camera stays a boundary until L2 lands), NEXTEV with the branch tables,
   SCNINIT, ENTB / ENTB2 / ENTB3 / ENTBE / ET*; arcade GGCOL / GGSET / TEXCHG as engine hooks. Prove with
   `run_capture.ps1` on `play_us_20261007_2238` (`evt_run` per frame) and a `-Route` game for the branches.
   **Done when** captures over the whole of Chapter 1 show 0 differences.
3. **L2 camera** (`src/port/camera.cpp`): `camera_task_start`, `cam_next_job`, CCTB jobs 16-21 / 32 / 33 / 64,
   CAMDR modes 0-3 and the per-frame modes 4 / 6 / 7, `camera_path_eval`, `camera_look_at`,
   `game_cam_turn_to_point`, `game_cam_stop_update`, `camera_matrices_update`. Prove on the same recording (camera
   task per frame, coprocessor calls included). **Done when** Chapter 1's camera captures show 0 differences.

Decided (user, 2026-10-15): A logic; parallel sessions allowed, stopping new work before the weekly usage passes
85 % (check with get_usage before starting each round; 18 % at the decision).
