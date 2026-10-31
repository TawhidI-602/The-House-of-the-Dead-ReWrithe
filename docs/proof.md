# Proof: the port against recorded runs

The rewrite is proven system by system, not described: each ported routine (one per original function, same names)
is run on recorded state and its results compared with what the original did, word for word.

- **A** (arcade): records from MAME 0.289 (`tools\run_state_rec.ps1`: work RAM per frame; `tools\run_copro_probe.ps1`:
  coprocessor calls). Every record has a `recording.txt` header (MAME version, command line, script hashes).
- **PA / PM** (PC Arcade Mode / PC Mode): records from the PC harness ([pc-harness.md](pc-harness.md)), same
  HOTDREC1 format.

The checker is `hotd_proof` (`src/proof/main.cpp`, CMake target `hotd_proof`, no SDL). It links `hotd_logic`, the
library of ported game code (`src/arcade/`, later `src/game/`). Exit code 0 = no differences.

| System | Command | Records | Result |
|---|---|---|---|
| Coprocessor (A only; the PC has none) | `hotd_proof tgp <copro.txt> [copro_data.bin]` | from-boot `-Funcs all` traces | 3.3 M calls, 0 differ (2026-10-13) |
| Shooting: shot test (A): `shot_resolve` and everything below it (`shot_test_object / _part / _body / _pairs / _list_a / _list_b`, `ray_sign_test`, `shot_targets_sort`, `shot_hit_nearest`) | `hotd_proof capture <folder>` | `run_capture.ps1 -Name rectest_us -Funcs 454e0 -If 51f288:2,51f354:2` (60 shots), the same with `-MinCalls 8 -Boundary 30ef0` (40 shots that hit scene quads) | 100 captures, 1154 coprocessor calls, 21 k changed bytes: 0 differ (2026-10-13). Not reached yet: the hit flash of a scene quad with a code other than 0 / 4 |
| Shooting: gun input and ray (A): `player_shoot`, `shot_ray_setup` | `hotd_proof capture <folder>` | `run_capture.ps1 -Name rectest_us -Funcs 5a80 -If 51f5ec:2 -Boundary 95270,154d0` | 80 captures (about 33 shots, 529 coprocessor calls, 46 shot-effect / sound calls replayed): 0 differ (2026-10-13). Not reached yet: the recorded-input sources (attract demo, mode 8) and gun_test_mode |
| Damage (A): `enemy_part_hits`, `part_hit` (recursive), `rand`, `add_score`, `difficulty_get` | `hotd_proof capture <folder>` | `run_capture.ps1 -Name rectest_us -Funcs 684b0 -IfArg 58:6 -Boundary <sound, effects, blow-off, bleeding, the 12 damage callbacks>` | 80 captures (rectest_us) + 250 captures over 300 s of the full-game recording `play_us_20261007_2238` (`-Ported 684b0,68010`): 319 boundary calls replayed (blood bursts, bleeding and bleed start, the eye pop, a blow-off, 3 of the 12 damage callbacks, body_free_cut_parts): 0 differ (2026-10-13) |

Method per system (frame N -> N+1): load the recorded state at frame N into the C++ structures, run the ported
routine(s) for one frame, compare the state they own with frame N+1. Values outside the ported system that the
routines read (e.g. random numbers, other tasks' fields) come from the record, so each system can be proven alone.
| Enemies: the standard enemy's frame (A): `enemy_main`, `enemy_footsteps`, `enemy_hit_reaction`, `enemy_check_death`, `enemy_voice`, `enemy_ai`, `enemy_ai_motion` (+ the damage routines) | `hotd_proof capture <folder>` | `run_capture.ps1 -Name rectest_us -Funcs 77410:3e0 -IfArg 58:6 -Ported <all of them> -Boundary <drawing, motions, slots, effects, sound, player_hit ...>` | 100 captures, 26,885 coprocessor calls, 474 boundary calls replayed (a player hit, deaths, hit motions): 0 differ (2026-10-13) |
| Enemies: guard set-up and actions (A): `enemy_guard`, `enemy_frame_begin / _end`, actions 2, 3, 5, 12 (`enemy_act*`), `ENGuardOpeningDummy`, `opening_dummy_walk`, `ENGuardZombieRunAfterResearcher`, `enguard_scripted_frame`, `ENGuardZombieBiteAgent` | `hotd_proof capture <folder>` | `run_capture.ps1 -Name play_us_20261007_2238 -Seconds 300 -Funcs <them>` | about 270 captures, 34 k coprocessor calls; whole-game run: actions 7, 14, 15, 17 / 18 (120 captures, 34.5 k calls), elevator_ride and the chainsaw's frame (57 captures, 7.2 k calls); chapter-3 recording `play_us_ch3_20261008_0008` (actions 14, 17: 60 captures, 17 k calls); `walk_to_attack_position` (0x1CA50, 30 captures over the first 300 s, boundaries angle_turn_toward / move_along_heading): 0 differ (2026-10-13 / 14). `enemy_frame_end_ch3_ev0` (chapter-3 recording `play_us_ch3_20261007_2335`, 30 captures, 8.4 k calls). Written, not reached by any recording (full game, the three chapter-3 recordings): actions 1, 6, 13, 16, 20, enemy_act7_frame_b, `enemy_act12_frame`, `enemy_act16_frame(_b)`; to reach them: a play that lets the researcher-chasing zombies and the chapter-3 event enemies run their whole script (not shooting them early), and the chainsaw zombie's second attack pattern. `walk_to_attack_position`: 29 of its 30 captures had two enemies counted (the aim beside the camera), 1 had one; the no-enemy path (direction 0) did not run |

## Routine captures (A)

The frame-level plan above turned out to be the wrong cut for the arcade: routines run inside tasks in the middle of a
frame and use the coprocessor, whose state is not in RAM. So the arcade side is proven **per routine call**:

- `tools\run_capture.ps1` replays a recorded `.inp` session in MAME with the **debugger enabled but headless**
  (`-debug -debugger none`; `tools/mame_capture.lua`): breakpoints at the routine's entry and at each of its `ret`
  instructions (found in the ROM from the routine's size in the Ghidra index). At entry: the global registers and all
  of work RAM (0x500000-0x5FFFFF); at the `ret` with the same frame pointer: the same again. The run also logs every
  coprocessor call from boot (`tools/mame_copro_probe.lua`, `-Funcs all`); each capture stores how many calls had
  started at entry and at exit. The debugger does not change the run: the trace is byte-identical to one made without
  it (157,620,052 bytes both times).
- `-IfArg off:mask` captures only calls whose first argument points at a word with one of these bits (e.g. a
  creature hit this frame: obj +0x58 & 6).
- `-If addr:mask,...` captures only calls where one of these words has a bit set at entry (e.g. a player fired);
  `-MinCalls n` keeps only calls that made at least n coprocessor calls (to reach rare paths such as scene-quad hits).
- **Boundaries** (`-Boundary addr,...`): routines not ported yet that a captured routine calls are recorded as their
  effect (the RAM bytes they changed, and their coprocessor calls) and replayed when the port calls its stub, in
  order (a boundary called inside another one is part of the outer one's effect). The proof fails if the port calls
  a boundary the original did not call at that point, or misses one.
- **Older captures after a port** (A and PC): a routine ported after a capture recorded it as a boundary is replayed
  from that capture's record, not run: its override in `src/proof/bounds/*.inc` checks `recorded_next(F::x)` (is the
  capture's next recorded boundary call this routine?) and calls `boundary` if so, the ported C++ otherwise. Fresh
  captures that list it in `-Funcs` / `-Ported` never record it as a boundary, so the port runs and is checked there.
  Such a routine has to be a `virtual` member declared without a body (the proof overrides it); routines with no PC
  form yet use `if (P::pc || recorded_next(F::x))`. Set up so far for the creature core / player_hit (L3), the six
  dog helpers and kind 70's set-ups / think routines (k32_full4, dog_skip, dog_r9b, k70_full1-4 / r3 / r6 / skip,
  k32dog_pa, k32b_pm, k70_pa / pm: 0 differ again, 2026-10-15).
- `hotd_proof capture <folder>` brings a reference `Tgp` along the trace to each capture's entry count, loads the
  entry RAM, runs the ported routine (`hotd::arcade::Arcade`, `src/arcade/`), checks every coprocessor call it makes
  (function, inputs, outputs) against the trace in order and the call count at exit, then compares all of work RAM
  with the exit image (except the routine's stack, 32 KB from its frame pointer: the i960 stack grows upward).
  A deliberate one-bit bug (dropping the 0x100 hit bit in `shot_hit_nearest`) is caught by 2 of the 60 shots.

The port works on the original layouts: `tools/gen_cpp_types.py` generates `src/arcade/types.h` (every struct of
types.tsv at its arcade offsets, pointers as 32-bit arcade addresses) and `src/arcade/globals.h` (every named data
address of names.tsv); `src/arcade/mem.h` maps the arcade address space (work RAM, program ROM, main_data).

### Branch coverage

A matching capture proves the paths that ran, not the routine's every branch. `run_capture.ps1` therefore also writes
`coverage.txt`: for every conditional branch (`b<cc>`, `cmp*b*`, `bbs` / `bbc`) of every `-Ported` routine, whether
its taken edge and its fall-through edge ran while a capture was open. `mame_capture.lua` puts a breakpoint on each
such branch (enabled only while a capture is open), single-steps one instruction when it is hit and reads the new pc;
once both edges have run, the breakpoint is removed, so the cost stays small. `py tools/branch_coverage.py [--edges]
<folders>` merges several runs and lists per routine the branches that ran both ways, one way, or not at all.
**Proven means the paths that ran**: an edge that never ran is listed as unproven next to the proof. Not covered by
this: jump tables (`bx` through a table: their cases show up as the branches inside each case). Proofs made before
2026-10-14 (second session) have no coverage file; it comes with their next capture run.

Build: `cmake --build build --config Release --target hotd_proof`.

## PA / PM routine captures

The same per-routine method on the PC port, inside the PC harness ([pc-harness.md](pc-harness.md); the port's
layout question is settled in [port.md](port.md): one template routine, instantiated for each version's layout).

- **Capture** (`tools/pcharness/hotd_hook.c`, `run_pc.ps1 -CapFuncs <PC addresses> -CapBoundary <...> [-CapIfArg
  off:mask] [-CapIf addr:mask] [-CapMax n] [-CapFront 2|5]`): an `int3` at each captured and each boundary routine's
  entry; a vectored exception handler records the state, puts the original byte back for one single step and re-arms
  it. The return address on the stack is swapped for a stub (an `int3` of the hook's), so the return is caught
  without knowing the routine's `ret`s. Memory = the EXE's writable image (DGROUP, .bss), the heap blocks the harness
  hands out (fixed addresses), and a window of the stack; written as byte runs against a shadow image (4 KB pages),
  with the committed ranges ("MAPS") so that all-zero pages are mapped too. Per capture: the entry state and
  registers (Watcom register convention: arguments in eax, edx, ebx, ecx, then the stack), the x87 state, the
  routine's own changes before each boundary call, each boundary call's arguments, result and effect, and the rest
  at the return. A routine that never returns (the task system unwinds it) is recorded as dropped. **The captures do
  not disturb the game:** the state record of a capture run equals a plain run's over all 3200 ticks.
- **Check:** `hotd_proof pccapture <folder> [THOTD.EXE]` loads the EXE's code section (it holds read-only tables,
  e.g. the part roles), keeps the same shadow image along the file, runs `Game<Pc>`'s routine on each entry image,
  replays the boundaries in order (their arguments are compared too: registers and stack words) and compares every
  mapped byte with the original's result (except the stack below the return's esp). PA and PM are told apart by the
  capture's `frontend_state` (2 = Arcade Mode, 5 = PC Mode); the same C++ routine runs both, with the PC code's own
  test.
- **Branch coverage** as on the arcade side: `-CapPorted` (default `-CapFuncs`) lists the ported routines;
  `tools/pc_branches.py` (capstone) finds their conditional jumps, the hook breaks on each while a capture is open and
  single-steps it; `coverage.txt` in the same format, read by `tools/branch_coverage.py`.
- Differences from the arcade side: the PC layout (objects 8 bytes larger from +0x10, packed part roles, other
  addresses), x87 at 24-bit precision (float32 per operation: C++ float maths in the PC's operation order), the PC's
  own tables (square root, sin / cos), no coprocessor.

| System | Records | Result |
|---|---|---|
| Damage (PA / PM): `enemy_part_hits` 0x4C4168, `part_hit` 0x4C3D58 (+ rand, add_score, difficulty_get) | scripted games `scripts/game2.txt` (PA) and `game_pm.txt` (PM): part hits (`-CapIfArg 3c:6`) and hit creatures (`-CapIfArg 60:6`) | 17 + 17 part_hit, 401 + 560 enemy_part_hits captures, about 1,000 boundary calls with their arguments: 0 differ (2026-10-14). Coverage: part_hit 16 of 42 branches both ways, 9 never reached (metal sparks / class 17 effects, an empty replacement model, eye blow-offs); enemy_part_hits 3 of 17 (the scripted games give few kills: the kill credit, PC's last-killer rule, eye scores did not run) |
| Shooting (PA / PM): `shot_resolve` 0x489C84 and its tests (`shot_test_object / _part`, `ray_sign_test` with the PC's table square root and PM hit size, `shot_hit_nearest`, `shot_targets_sort`), `shot_ray_setup` 0x414810 (PC-only helpers `pc_vec_angles`, `pc_atan2`, `pc_matrix_push / _transform`, the sin / cos table), `player_shoot` 0x4365C4 (its PC form: mouse input routine, PC demo stream, PM reload timer by timeGetTime) | the same scripted games; shot_resolve with `-CapIf d9f060:2,d9f12c:2` (a player fired) | 159 shot tests, 159 rays, 3,000 player_shoot calls (with about 3,200 boundary calls and their arguments): 0 differ (2026-10-14). Coverage: object-only hits and scene items not exercised (all depth-list objects had part trees), the attract-demo input, player 2, the PM reload (timer) path did not run |
| Enemies (PA / PM): the standard enemy's frame: `enemy_main` 0x4D9A90 with `enemy_footsteps`, `enemy_hit_reaction`, `enemy_check_death`, `enemy_voice`, `enemy_ai` 0x4DA030 (PC turn: `pc_matrix_identity / _rotate_y / _transform2`, `pc_vec_angles2`, `pc_vec_cos`, `pc_angle_turn`), `enemy_ai_motion`, the leave routines `enemy_leave` / `enemy_leave_after_hit`, and the damage routines | the same scripted games: every enemy frame (600) and frames of hit enemies (`-CapIfArg 60:6`, 340 / 400) | 1,939 captures, about 9,200 boundary calls with arguments: 0 differ (2026-10-14). Coverage: 58 of 268 branches both ways, 123 never reached (the scripted games have few kills, no headless zombies, no attacks on player 2) |
| Enemies (PA / PM): guard set-up and actions: `enemy_guard` 0x4A5180 (with `pc_guard_link_prey`), `enemy_frame_begin / _end`, actions 3 / 5, `ENGuardOpeningDummy`, `opening_dummy_walk`, `ENGuardZombieRunAfterResearcher`, `enguard_scripted_frame`, `ENGuardZombieBiteAgent` (`pc_bite_frame_lookup`, the effect through the PC matrices), `walk_to_attack_position` 0x447450 (float32 slot offset, table length) | the same scripted games (chapter 1's opening) | 693 captures, about 3,000 boundary calls: 0 differ (2026-10-14). The other guard actions (1, 2, 6, 7, 12-20, elevator_ride, the act7 / act12 / act16 frames) are not reached by the scripted PC games (chapter 1 opening only): they need PC plays of chapters 2-3 |
| Enemy kinds (A / PA / PM), first batch (src/port/enemy_kinds.cpp, drafted with tools/ghidra_to_cpp.py): A: `kind000_enemy_spawn` 0x77110, kind 2 (`kind2_zombie_kick` / `_wait` / `_fight`), kind 10 (`kind10_zombie_scene` / `_wait`); PA / PM: `kind000_enemy_spawn` 0x4D974C, kind 2's spawn and wait | A: the full-game recording (`private\proof\kinds_a`); PC: the auto-aim games | A 121 captures, PA 49, PM 49: 0 differ (2026-10-14); then A kind 3 (33), kind 5's fight (30), kind 8 (34), kind 9's spawn (7): 0 differ. then kinds 5 and 11 whole (93, chapter 3). Written, not reached by any recording: kinds 1, 4, 6, 12; kind 7 not written (A re-run `kinds_a2`; on the PC they are not in chapter 1). PC differences: sound numbers; kind 10's camera-frame test compares the float's bits as an integer; the PC reads kind 0's sub-state-8 motion from its table; an option (`pc_no_extra_model`) hides kind 5's plain model |
| Enemy kinds (PA / PM), long games: kinds 0, 2, 3, 4, 5, 6 (spawn + frame), 8, 9, 10, 12 (PC addresses in names.tsv) | `run_pc.ps1 -Ticks 60000 -InfLives -InputScript scripts\game_aim.txt` / `game_pm_aim.txt` (auto-aim, lives kept) | 459 + 454 captures, about 2,600 boundary calls: 0 differ (2026-10-14) |
| The researchers (kind 64, src/port/enemy_researchers.cpp; A and PC): spawner, frame, the nine set-ups and 21 states | A: full-game recording (k64_full / k64_full2) + auto-aim games with `-AimSkip 6dbd0` (researchers not aimed at) on `-Route "1:3"` (event 9), `"1:5=0"` (6), `"1:4"` (7), `"1:3,1:9=0"` (14), kind 65 captured with them; PA / PM: `game_aim.txt` / `game_pm_aim.txt` with `-AimSkip 4ccf20` | A: 334 + 1,935 + 1,569 + 2,473 + 1,469 + 1,070 captures, 0 differ apart from 5 captures recorded with Ghidra's short routine sizes (missing boundary records; re-captured with `addr:size`); PA: 1,787, PM: 1,813, 0 differ. Coverage: set-ups 0-8 and their states on A, all but set-ups 2 / 5 / 7 / 8 on PA / PM (default route) |
| The bats (kinds 66-68, src/port/enemy_bats.cpp; A and PC): spawners, bat_frame / bat_swarm_frame, states 3-6, 9, 10, set-ups 0 / 5 | A: full-game recording (bats_full); PA: auto-aim game (bats_pa) | A: 6,015 captures, PA: 9,392, 0 differ. Not reached: states 1 / 2 (hanging, take-off), set-ups 1 / 4 |
| Kind 32, the zombies with a kit (src/port/enemy_kit_zombie.cpp, enemy_kit_zombie_iron.inc, enemy_kit_held.inc; A and PC): spawner, the axe thrower, the drum carrier, the iron-ball swinger and its ball task, kind32_hits / part hits, the axe release, the held objects (thrown axe, carried drums / barrel: launch, flight, shot check, knocked away, break-up, draw) | A: full-game recording (k32_full4); PM: auto-aim game (k32b_pm); PA: auto-aim game (k32dog_pa) | A k32_full4: 6,885 captures, 0 differ; PM k32b_pm: 9,711, 0 differ; PA k32dog_pa: 9,760, 0 differ; not reached: kind32_part_hits, iron_ball_hit_wait_end, the throw at the camera, drop1 |
| Kind 97, the Kenfis dogs (src/port/enemy_dogs.cpp; A and PC): spawner, set-ups 0-4 and their frames, pack list and motion helpers, dog_frame, dust, hits, the bite, leap, waiting and circling states, and the helpers dog_knockback, dog_st2_move (two players), dog_angle_to_target, dog_st3_steer, dog_set3_fall (coprocessor 0x2D), dog_set4_path | A: full-game recording (dog_full2), fresh auto-aim games (dog_aim; dog_skip / k97_skip with `-AimSkip 7ea80` so the dogs live and bite; k97_2p the same with `-TwoPlayers`); routes: dog_r9b `-Route "1:3"`, dog_r14b `"1:3,1:9=0"`, k97_r15 `"1:3,1:9,1:11=0"` (the wall breaks: event 17), **k97_r15b `-Route "1:3,1:9,1:11=0" -AimSkip 3e030`** (the wall's pieces not shot: kind145_route_hold would keep route_choice 1, so event 11 -> 15, set-up 3); PA / PM: see the PC rows (kind 97) | A (2026-10-15 evening, all helpers ported): k97_skip 4,932 (dog_angle_to_target 400, dog_st3_steer 400 direct, plus inside dog_st3_circle 400), k97_2p 5,332 (dog_st2_move 400), k97_r15 601 (dog_set4_frame 323 with dog_set4_path), k97_r15b 752 (dog_set3_frame 98 with dog_set3_fall: 0x2D 10 / 10; dog_frame 71, 4 of them with the knock-back running: a shot dog survived in event 15): all 0 differ. Earlier: dog_full2 601, dog_aim 243, dog_skip 4,132, dog_r9b 601, dog_r14b 397, 0 differ. **Kind 97 complete on A** |
| Kind 97, the Kenfis dogs (PA / PM): everything of the A row, the six helpers included (PC copies of the matrix helpers: suffix 11 / 12) | `game_aim.txt` / `game_pm_aim.txt` with `-InfLives`: `-AimSkip 4df7bc` (dogs live: k97skip_pa / _pm), the same with `-TwoPlayers` and `game_aim_2p.txt` / `game_pm_aim_2p.txt` (k97_2p_pa / _pm), `-Route "1:3,1:9,1:11=0" -AimSkip 47f288` (event 15: k97r15b_pa / _pm); earlier k32dog_pa, dogskip_pm, dogr9_pa / _pm, dogr14_pa | PA: k97skip_pa 30,132 (dog_angle_to_target 3,000, dog_st3_steer 3,000), k97_2p_pa 33,132 (dog_st2_move 3,000), k97r15b_pa 709 (dog_set3_frame 98 with dog_set3_fall, dog_set4_frame 323 with dog_set4_path; 4 dog_frame captures with the knock-back); PM: k97skip_pm 30,132 (3,000 each), k97_2p_pm 33,132 (dog_st2_move 3,000), k97r15b_pm 713 (98 / 323; knock-back in 5 dog_frame captures), k97aim_pm 244: all 0 differ |
| Kind 65 on A, more (with kind 64's runs) | the same auto-aim route games | scenes 0 / 2 / 3 / 4, the hit check, damage and death: 216 + 724 + 196 frame captures, 0 differ (scene 1 is in molb_full3) |
| The frog (kind 69, src/port/enemy_frog.cpp; A and PC): frame, states 1 / 2 / 4 / 5, set-ups 0 / 2 / 3 (4: its swimmers run set-up 3) | A: full-game recording (frog_full) + auto-aim games with `-Route "1:3"` (event 9) / `"1:5=0"` (event 6); PA / PM: auto-aim games (`game_aim.txt` / `game_pm_aim.txt`, frog_pa, frog_pa_r9, frog_pm) | A: 124 + 61 + 154 captures, 0 differ; PA: 213 (full game) + 162 (`-Route "1:3"`), PM: 254 (default route, passes event 6), all 0 differ. State 3 (stale stack word) and set-up 1 (in no event script) unreached |
| Kind 70, the agile creatures (src/port/enemy_agile.cpp; A and PC written): spawn, set-ups 0-12, frame, hits, both hit reactions, death, states 0-29 with the five think routines, Drake's knife (throw, spawn, flying task), Moody's throw and projectile task, the target helpers | A: full-game recording (k70_full1-4), fresh auto-aim games with `-AimSkip 8c090` (k70_skip; k70_r3 / k70_r6 with `-Route "1:3"` / `"1:5=0"`) | A: 973 + 2,822 + 6,334 + 1,317 + 5,878 + 1,247 + 1,547 = 20,118 captures, 0 differ; kind 70 branches 197 of 561 both ways, 151 never. Not reached: states 11 / 17, set-ups 1 / 7 / 12. PA / PM not run yet (docs/kind70_status.md) |
| The event script interpreter (src/port/event.cpp; A, PC written): `evt_chapter_start`, `evt_task`, `evt_run` and every handler of the opcode table (ET1P / ET2P, ENTB / ENTB2 / ENTB3 / ENTBE with ENTB's camera-relative placement, SCNINIT, NEXTEV with route_branches, CCTB, CAMDR / CAMDR2, the WAIT_* opcodes ...) | A: full-game recording: `evt_run` every frame from boot (evt_run_ch1, 45,000 frames: attract, Chapters 1-2, the start of 3) and each handler on its own over the whole game (evt_ops_full); auto-aim games with `-Route "1:0"`, `"1:3"`, `"1:4"`, `"1:5"`, `"1:4,1:7"`, `"1:3,1:9=0"`, `"1:5,1:12=0"` (handlers except the waits): every Chapter 1 event 0-19 | A: 45,000 + 5,796 + 1,903 + 2,005 + 1,845 + 2,050 + 2,033 + 2,040 + 2,092 = 64,864 captures, 0 differ; branches 40 of 81 both ways, 12 never (SKPETO / SKPETU / EV3JP1 are in no recorded script, CAMDR2 with a player out, mode 8). Boundaries: camera drive modes, task_create, sound, colour rows, texture areas, shadow / light reset. PA / PM not run yet |
| The camera (src/port/camera.cpp; A): `camera_task` (per frame: the main job by camera_stopped, cam_next_job's loop, slot jobs, `camera_matrices_update`), `camera_task_start`, every CCTB job (16-21, 33, 64 jump / defer / play), CAMDR 1 / 3 and CCTB 33 modes 4 / 6 / 7 with their per-frame routines (`camera_frame_mode4/6/7`, `camera_mode_run`: camdr_mode1_frame, camdr_mode3_view_pos, cctb33_mode4 / 6 / 7 start and walk), the game camera at the stops (`game_cam_stop_update` and its sub-states), `camera_path_eval`, `camera_look_at`, `matrix_identity_set` | full-game recording, Chapter 1 from the game start into Chapter 2: `-Funcs 2b100,6cab0,2b3a0,2b040 -If 51f494:4` (game modes) sampled with the new `-Every` / `-Skip` (cam_dbg: every 2nd frame of the first 60 s; cam_a1: every 5th frame; cam_a2: every odd frame) | 28,931 captures, 1.8 M coprocessor calls, 0 differ (2026-10-15). Two harness fixes on the way: coprocessor words the model cannot know (MA0: 0x0F of a zero vector, camera record 2 copying an all-zero record 0) are taken from the trace; a data word shaped like a command word (0x42008484) that the probe logged as a call of its own is joined back. Not reached (Chapter 1 does not use them; marked UNVERIFIED in decl/camera.h): cam_next_job (no callers), game_cam_turn_to_point (sub-state 7), CCTB 32, CCTB 33 mode 5, CAMDR 2, the undefined-mode / GameCam30 nop error stops. PC forms not written yet |
| Support routines (src/port/attack_slots.cpp, enemy_scripted.cpp, floor_motion.cpp, decl/support.h; A; first written as runtime overrides in src/game/t1_bounds.cpp, moved into the port 2026-10-16): `attack_slots_update`, `attack_slots_sort`, `attack_slot_take_waiting / _take_wide / _front_take / _nearest_object`, `enemy_attack_slot_take / _free`, `enemy_scripted_hit`, `enemy_scripted_dead`, `enemy_scripted_dead_fall`, `enemy_wait_camera_stop`, `Zombie_Dead`, `floor_probe`, `ground_height`, `jump_arc_set / _step / _step_b / _velocity / _velocity_b`, `obj_move_heading` | `hotd_proof capture <folder>` | `run_capture.ps1 -Name play_us_20261007_2238` (and `play_us_ch3_20261008_0008` for the slots), `-Ported` = capture_lists.py's list, boundaries = its list + the draw hooks `9ce60,37d80,87550,689e0,68b50,69540` (object_draw's part hooks: capture_lists.py does not find them): t1p_slots_upd (`-If 53e230:ffffffff -Every 5`), t1p_slots_upd2 / t1p_slots_ch3 (`-If 53e230:fffffffe`: 2+ users), t1p_scripted2, t1p_shit (`-Funcs 535f0 -If 51fdb0:ff`: hits), t1p_sdead (`-Every 5 -Seconds 1530`), t1p_wcs (`-Funcs 77350 -If 51f628:2`), t1p_take, t1p_floor, t1p_floor2 (`-Every 25 -Seconds 1530`), t1p_zdead | slots update / sort 480 + slot takes / frees 274 + nearest 4, scripted hit 82 (22 hits, 8 deaths), dead 260, dead_fall 155, wait_camera_stop 140, Zombie_Dead 1, floors / arcs / heading 1010: **0 differ** (2026-10-16). Fixed from the captures and the disassembly: attack_slots_sort writes its scratch array 0x53E240 (two radix passes) and has no 16-entry cap; attack_slots_update keeps the original's frame (position indices not range checked) and no >= 16 guard; attack_slot_nearest_object's coprocessor 0x1F input is (holder x, 0, z, object x, 0, z); enemy_scripted_hit's hit motion word is not truncated to 16 bits. Not reached: `Zombie_Dead_frame` past its first frame (UNVERIFIED; one Zombie_Dead in the whole recording, none in fresh auto-aim chapter 1 / 2 games), more than one floor quad in floor_probe / ground_height, jump_arc_velocity's end, attack_slot_front_take's two-player paths, enemy_attack_slot_take's attack_side / two-player paths. PC forms not written (vtable only, ProofPc replays them) |
| Support routines (2) (src/port/hit_effects.cpp, damage_types.cpp, body_hooks.cpp, scripted_extra.cpp, enemy_helpers.cpp, combat_support.cpp, part_tree.cpp, player_start.cpp, prop_groups.cpp, decl/support_b.h; A; first written in the runtime (src/game/t1_bounds.cpp, enemy_support.cpp, players.cpp, runtime.cpp), moved into the port 2026-10-16, proofs-2): the hit effects (`blood_drip_at / blood_drip`, `blood_burst_spawn / blood_burst`, `part_bleed_spawn / _start / part_bleed`, `hit_effect_at / hit_splash`, `floor_splat_spawn(_b) / floor_splat / floor_dust`, `wall_dust_spawn / wall_dust`, `hop_dust_spawn / hop_dust`, `prop_spurt_spawn / prop_spurt`, `part_hit_spark / part_spark_anim`, `shot_flash_spawn / shot_flash`, `shot_effect / shot_effect_flash / shot_effect_debris`, `hit_spark`, `muzzle_flash_spawn / muzzle_flash`), the 11 `part_damage_type*` routines, `body_hook_eye_beam / _target_beam`, `player_hit_notice`, `hit_screen_shake`, `kind102_enemy_wait_camera / _walk_in`, `kind101_corpse`, `turn_motion_set`, `voice_message`, the enemy helpers (`depth_list_add`, `attacker_list_add`, `enemy_target_distance`, `difficulty_level`, `angle_to_camera`, `enemy_dying`, `enemy_corpse(_sink)`, `part_tree_free_children`, `body_free_parts`, `enemy_free_extras`, `angle_turn_toward`, `walk_to`, `enemy_scripted_to_ai`, `turn_toward_camera`, `move_along_heading`), kind 96's frames (enemy_walkers.cpp, now run by the runtime), combat support (`enemy_slot_take`, `attack_queue_role`, `attack_front_busy`, `kill_score_award / _shooter`, `enemy_kill_score`, `researcher_scream_penalty`, `researcher_hit_by_player`, `researcher_blood_effect`, `molb_floor_stick / _above / _bounce`, `velocity_for_arc(_speed)`, `room_bounds_bounce`, `camera_side_set`, `player_in_play`, `combat_active`, `pick_target_player(_any / _rand)`, `spline_segment_init / _eval / _advance`, `is_demo`), the part tree (`part_tree_detach_severed(_rec)`, `body_free_cut_parts`, `body_role_model_set(_rec)`, `body_node_model(_rec)`, `body_part_set_model(_rec)`, `axe_hand_model_hide`), the start check (`player_can_start`, `player_start_pressed`, `credit_lockout_reset`, `player_start_check`, `player_st9_waiting`, `player_waiting(_frame)`), `prop068_furukagi`, `Zombie_Dead_frame`, the players' cut-scene bodies (player_body.cpp: scenes 00 / 02 / 03-08 but the parts marked), `sophie_skin_template_copy`, `turn_motion_set_speed` | `hotd_proof capture <folder>` | `run_capture.ps1`, `-Ported` / boundaries = capture_lists.py's lists (+ the draw hooks by hand): `-Name play_us_20261007_2238 -Seconds 1530`: p2_fx_spawn (`-Every 3`), p2_fx_tasks (`-Every 41`), p2_dmg (`-Every 13`), p2_hooks (`-Every 97 -Always 51f338:1,51f404:1`), p2_notice (`-Funcs 6290,6350 -If 51f338:1,51f404:1`), p2_sx, p2_ehelp (`-Every 23`), p2_k96 (`-Funcs 7ec30:30,7ec60:180`: not in the index), p2_combat (`-Every 29`), p2_ptree (`-Every 17`), p2_pstart (`-Every 97`), p2_pbody, p2_motion, p2_muzzle; fresh auto-aim games: p2_zdead_r39b (`-Route 1:3,1:9 -Funcs 78fc0:18,76e50:16c`: the index size stops before the last `ret`), p2_route03 / p2_route5 / p2_route47 (`-Route 1:0,1:3` / `1:5` / `1:4,1:7`: kinds 101 / 102, the part tree, the branch scenes 03-07), p2_route4 (`-Route 1:4`: prop 068), p2_pstart2p (`-TwoPlayers`: player 2's start) | 226 + 309 effect captures, 28 part damage, 177 + 60 hooks / notice / shake, 52 + 81 Zombie_Dead(_frame) / hit_spark / voice / turn, 436 + 99 enemy helpers / kind 96, 386 combat, 303 part tree, 202 + 407 start check, 311 + 502 + 550 + 592 bodies / routes, 60 prop 068, 46 muzzle flash: **0 differ** (2026-10-16). Fixed from the captures and the disassembly: velocity_for_arc_speed's flight time is the floor distance (coprocessor 0x1F (to x, 0, z, from x, 0, z)) and the arc's vertical speed is ((dy + dy) + (g n) n) / (n + n) in that order (T1 had a 3D distance and another float order); enemy_dying's frame count index and +0xF8 are zero-extended; kind102_enemy_walk_in's motion word is not truncated to 16 bits. Old captures re-checked: guard_actions_full2, frog_full, k70_full4, cam_a1, dog_full2, enemy_part_hits_full, enemy_main_hit, k32_full4, bats_full, t1p_*, k97_r15: 0 differ; l3_cr1 / k64_full2 / kinds_a2 / l5_full1: the same differing captures as before. Not reached (UNVERIFIED): `enemy_act6_frame / _bite` (route 1:5 with the guards skipped by the auto-aim did not run action 6), `hostage_shot_effect` (no hostage shot; its body is researcher_blood_effect's), `kind138_prop_group` (ends in task_exit: no exit record), `set_motion_at_frame`, `set_motion2_blend`, `object_draw_model`, scenes 00 turn / hold, 04 p2, 05 p_p / p_p_p, 06 p2 / p2_p, 07 p1_p_p_p, `player_body_place / _camdr / _hold`. PC forms not written (vtable only, ProofPc replays them) |
| Support routines (3) (proofs-3: routines of earlier rows no capture had reached; A): `enemy_act6_attack_researcher`, `enemy_act6_bite`, `enemy_act6_frame` (guard action 6, event 15), `hostage_shot_effect` (a hostage shot by a player), `set_motion_at_frame`, the players' cut-scene bodies `player_scene04_fall_backside_p2`, `player_scene06_reload_shoot_p2`, `player_scene06_reload_shoot_p2_p` | `hotd_proof capture <folder>` | `run_capture.ps1`, `-Ported` / boundaries = capture_lists.py's lists + the draw hooks `9ce60,37d80,87550,689e0,68b50,69540`, fresh auto-aim games: p3_act6_r15 (`-Route "1:3,1:9,1:11=0" -AimSkip "3e030,543b0,56360,56320" -Seconds 900`: event 11's wall pieces and the researcher-attacking guard not shot, so event 15's guard runs action 6 to its end), p3_default (`-Seconds 900`: the auto-aim shot a hostage; set_motion_at_frame from scene_zombie_walk_frame), p3_act6_r5 (`-Route 1:5 -AimSkip 543b0,56360,56320`: events 12 -> 17, no action 6), p3_scn47_2p / p3_scn5_2p (`-TwoPlayers -Route "1:4,1:7"` / `"1:5"`); new `-AimPrefer "<routines>"` (mame_dump.lua HOTD_AIM_PREFER: aim at those first when on screen, else the nearest) | p3_act6_r15: 109 captures (attack_researcher 7, bite 50, act6_frame 1, hostage_shot_effect 50, set_motion_at_frame 1; 18,145 coprocessor calls, 2,196 boundary calls replayed); p3_default / p3_act6_r5: 51 each (hostage_shot_effect 50, set_motion_at_frame 1); p3_scn47_2p 51 (scene 06 p2 50), p3_scn5_2p 151 (scene 04 p2 50, 06 p2 50, 06 p2_p 50): **0 differ** (2026-10-16), nothing to fix. Not reached (UNVERIFIED): `set_motion2_blend` (only caller kind32_walk_turn: no kind-32 walk turn in these runs), `object_draw_model` (draw mode 2: only enemy_kit_held.inc sets it), scenes 00 turn / hold, 05 p_p / p_p_p, 07 p1_p_p_p, `player_body_place / _camdr / _hold`; `set_motion_at_frame` ran once per game (scene_zombie_walk_frame; hostage_s01_run_steps never: scenario 1 not freed). p3_hshot2 (`-AimPrefer <the hostage states> -Seconds 600`): hostage_shot 50, hostage_shot_effect 50, 0 differ. p3_full_draw (`-Name play_us_20261007_2238 -Seconds 1530`, the unreached routines above as funcs): none of them called |
| Palette (proofs-4; A): `palette_animation` 0x25310 (every frame, geo_frame_begin; the title screen's branch calls palette_animation_step 0x24020, a boundary), `palette_grey_build` 0x1A400 (the sepia look: kind 172's chapter opening, kind 182), `vblank_callback_add` 0x2780; moved from src/game/palette_anim.cpp / palette.cpp into `src/port/palette.cpp` | `hotd_proof capture <folder>` | `run_capture.ps1`, fresh auto-aim games: p4_palanim (`-Funcs 25310 -Boundary 24020 -Seconds 240 -Max 120 -Every 7`: from boot, title screen + Chapter 1 start), p4_palanim_c1 (`-Seconds 420 -Max 200 -Skip 2500 -Every 41`), p4_palanim_c3 (the same with `-Chapter 3`), p4_grey2 (`-Funcs 1a400,2780 -XRam 1802000:800 -Seconds 300`). New `-XRam "addr:len,..."` (mame_capture.lua HOTD_CAP_XRAM): memory outside work RAM recorded at each capture's entry ("XRAM" records, mapped by hotd_proof): palette_grey_build reads palette bank 2 0x1802000 | palette_animation 520 captures (35 title-screen calls of 0x24020 replayed), palette_grey_build 2, vblank_callback_add 8: **0 differ** (2026-10-16), nothing to fix (the first p4_grey run without -XRam differed only in the bank-2 copies 0x51CCB0 / 0x51D4B0: the palette RAM was not in the capture). Headless `hotd --frames 6000 --autofire`: the same palette log (HOTD_PALANIM_LOG) and output as main's build. Not done: the vblank copy 0x24E90 (writes the polygon palette, not work RAM) |
| Player support / shadow (proofs-4; A): `projectile_hit_player` 0x6640 (the Molbs' and thrown weapons' hits; src/port/player_support.cpp), `shadow_light_reset` 0x6C2A0, `shadow_setup` 0x6C760 (src/port/shadow.cpp); moved from src/game/enemy_support.cpp / chapter.cpp (the runtime keeps the projectile log line and shadowFloor_) | `hotd_proof capture <folder>` | `run_capture.ps1`, fresh auto-aim games, no boundaries (capture_lists.py: none): p4_hit / p4_hit_2p (`-Funcs 6640,708e0,70960 -Seconds 900 -Max 80`, the second `-TwoPlayers`), p4_shadow (`-Funcs 6c2a0,6c760 -Seconds 600 -Max 100 -Every 37`) | projectile_hit_player 33 (8 one player, 25 two players), shadow_light_reset 1, shadow_setup 100 (400 coprocessor calls): **0 differ** (2026-10-16), nothing to fix. `researcher_rescue_bonus` 0x70960 (player_support.cpp; 0 calls in p4_hit / p4_hit_2p): p4_res_full (`-Name play_us_20261007_2238 -Funcs 708e0,70960 -Seconds 1530`) 2 captures, 0 differ. Not reached (UNVERIFIED): `researcher_reached_camera` 0x708E0 (not called in the recording either) |
| Game over (proofs-5; A): mode 5's sub-modes `game_over_start` 0x97200, `game_over_scene` 0x97340, `game_over_map_start` 0x5CB50 (+ `map_scroll_set` 0x5CF30), `game_over_map` 0x5CBF0, `game_over_text` 0x973A0, `game_over_wait` 0x97430; the players' `player_st6_game_over` 0x5470 / `player_game_over_wait` 0x5540; the scene's camera `game_over_camera` 0x97490; the route map's `game_over_walker_task` 0x5CCD0 and `game_over_route_task` 0x5CF50; moved from src/game/flow.cpp into `src/port/game_over.cpp` (the runtime's mode5_game_over dispatches to them) | `hotd_proof capture <folder>` | `run_capture.ps1` with the new `-Mortal` (lives not kept: a lost game) and `-Shoot 0` (the player never fires: game over in Chapter 1's event 3 at about 150 s); `-Ported` / boundaries = capture_lists.py's: p5_go1 (`-Funcs 5470,5540,97490,5ccd0,5cf50 -Max 80 -Every 4 -Seconds 400 -Mortal -Shoot 0`), p5_go_2p (the same with `-TwoPlayers -Every 5`), p5_go_route2 (`-Funcs "5cf50:578,5470,5540" -Max 400 -Seconds 1100 -Mortal`: the auto-aim plays on and dies in event 6, a longer route; the size takes in game_over_timer_tick 0x5D4A8, which the route task jumps to with `b` once the route has ended), p5_mode5 (`-Funcs 97200,97340,5cb50,5cbf0,973a0,97430 -Max 60 -Every 5 -Seconds 400 -Mortal -Shoot 0`) | p5_go1 177 (5470 1, 5540 68, 97490 68, 5CCD0 37, 5CF50 3), p5_go_2p 178 (5CCD0 60 with two walkers), p5_go_route2 567 (5CF50 296, 5540 270), p5_mode5 132 (97340 55, 5CBF0 38, 97430 36, the others 1): **0 differ** (2026-10-16). Fixed from the disassembly while moving: player_game_over_wait counts to exactly 0 and books the play time (bookkeeping_play_end), game_over_wait clears the debug mode 0x51F42C and books the game, the route task keeps its state in the task (+0x50 .. +0x68) without the runtime's safety stops, game_over_scene / game_over_map reset the matrix stack (coprocessor 0x08: 0x880080 = 0x808) first. Harness: hotd_proof's call_routine now runs a ported target that has a proof case unless the capture recorded it as a boundary (player_camdr_run -> player_body_camdr 0x62C0). Not captured: game_over_wait's last frame (next_mode; sampled every 5th) |
| Sky, palette, sprite banks, HUD (proofs-5; A): `sky_task` 0x33F50 (src/port/sky.cpp); `palette_bank2_upload` 0x1A300, `palette_restore_vblank` 0x1A710, `palette_anim_copy` 0x24E90 (src/port/palette.cpp; they write palette RAM 0x1802000); the sprite banks `sprite_slots_mark` 0x17A10, `sprite_bank_size` 0x17AA0, `sprite_slots_free` 0x17AF0, `sprite_bank_alloc` 0x17B20, `sprite_bank_free` 0x17BD0, `sprite_bank_palette_load` 0x17C10, `sprite_bank_tiles_load` 0x17CB0, `sprite_bank_load` 0x17D50 (src/port/sprite_banks.cpp); the per-player HUD state `hud_popup_update` 0x94490, `hud_player_notice_show` 0x945D0, `hud_player_start_draw` 0x94670, `hud_lives_draw` 0x94740, `hud_ammo_draw` 0x949F0 (src/port/hud_state.cpp; the cells: sprite_draw_anim / sprite_draw, boundaries) | `hotd_proof capture <folder>` | p5_sky1 (`-Funcs 33f50 -Boundary 14120 -Max 100 -Every 37 -Seconds 360`), p5_pal (`-Funcs 1a300,1a710,24e90 -XRam 1802000:800 -Max 30 -Seconds 400`), p5_banks (`-Funcs 17a10,17aa0,17af0,17b20,17bd0,17c10,17cb0,17d50 -Boundary 1a980 -XRam 1080000:80000 -Max 20 -Seconds 400 -Mortal -Shoot 0`), p5_hud2p (`-Funcs 94740,949f0,94670,94490,945d0 -Boundary 154d0,17660,17e40,180b0,209a0,a4ec0 -Max 150 -Every 7 -Seconds 700 -Mortal -TwoPlayers`). New: mame_capture.lua writes the `-XRam` ranges at exit too ("XOUT" records) and hotd_proof compares them, so routines that write palette / character RAM are checked there (p5_pal: the bank-2 copies change about 1,200 bytes of palette RAM each) | sky_task 100 (51 with the sky drawn) + 100 in Chapter 3 (p5_sky3, the same with `-Chapter 3`), palette 34 (1A300 2, 1A710 2, 24E90 30), HUD 453 (94490 150, 94740 150, 949F0 150, 94670 2, 945D0 1): **0 differ**. Sprite banks: 158 captures, 153 0 differ; 5 (17D50 3, 17CB0 2: the long tile copies) differ only in what a vblank interrupt inside the capture wrote (`HOTD_PROOF_RANGE=1`: the interrupt's stack 0x510430..0x510530, the vblank list 0x511FE0, the palette queue 0x517CA0 / 0x51DCB0.., the scroll shadows 0x51E8D0, coin_update's counter 0x51F560, 0x51F790, 0x520660..); the bank records, the slot map, the palette shadow and character RAM match. Not reached: hud_lives_draw's debug-mode score print (text_printf's format address unknown: the port passes 0) |
| HUD (2), life bar, game-over group starts (proofs-5; A): `hud_start_prompt` 0x209A0 (the prompt routines of table 0xAA450 are boundaries), `hud_sprites_load` 0x263D0, `hud_player_full_draw` 0x94CC0, `hud_sprite107_blink` 0x94F40 / `_draw` 0x94FD0, the continue / game-over sprites `hud_continue_sprite` 0x95010, `hud_continue_digit` 0x95040, `hud_continue_label` 0x95080, `hud_game_over_sprite` 0x950E0, `hud_game_over_sprite_final` 0x95110, the boss life bar `life_bar_init` 0x6BA90, `life_bar_draw` 0x6BAF0, `life_bar_update` 0x6BC40 (src/port/hud_state.cpp); `sprite_banks_init` 0x17930 with `palette_text_white` 0x1B270 (sprite_banks.cpp); the game-over task groups' start entries `game_over_state0_camera` 0x97740, `game_over_camera_start` 0x5CCA0, `game_over_camera_task` 0x5D4F0, `game_over_route_start` 0x5D4D0, `game_over_walkers_start` 0x5CE70 (game_over.cpp) | `hotd_proof capture <folder>` | p5_prompt2 (`-Funcs 209a0 -Boundary 20c50,20cb0,20d80 -Every 100000 -Always 51e8a0:ffffffff,51e8a4:ffffffff,51e8a8:ffffffff,51e8ac:ffffffff -Max 100 -Seconds 400 -Mortal -Shoot 0`: the calls with a prompt on or going off), p5_cont / p5_cont2p (`-Funcs 95010,95040,95080,950e0,95110,26260[,209a0] -Boundary 17d80,17e40,180b0,1a980[,20c50,20cb0,20d80] -Mortal -Shoot 0`, then `-Mortal -TwoPlayers` with the auto-aim: player 2 is out while player 1 plays on), p5_hud3 (`-Funcs 263d0,94cc0,94f40,94fd0` with the bank routines ported, `-Every 3 -Seconds 600`), p5_lifebar / p5_lifebar_init / p5_lifebar_end (`-Name play_us_20261007_2238 -Seconds 1530`: the Chariot's bar; `-Funcs 6ba90,6baf0,6bc40 -Every 3 -Max 150`, `-Funcs 6ba90` with the bank routines in `-Ported`, `-Funcs 6bc40,6baf0 -Every 100000 -Always 543530:ff00` for the blink-out), p5_bankinit (`-Funcs 17930,1b270 -XRam 1080000:80000 -Seconds 60`: boot), p5_starts / p5_starts_2p (`-Funcs 97740,5cca0,5d4f0,5d4d0,5ce70 -Every 5 -Seconds 400 -Mortal -Shoot 0`, the second `-TwoPlayers`: two walkers) | prompt 100 (+ 100 + 40 with no prompt on), continue / game-over sprites 4 + 47, HUD 72 (263D0 1, 94CC0 6, 94F40 60, 94FD0 5), life bar 302 + 2 + 160 (the blink-out and the bank freed), sprite_banks_init 1, group starts 42 + 42: **0 differ** (2026-10-16). The first p5_lifebar run's 2 life_bar_init captures differed: sprite_bank_load was not in its `-Ported` list, so its palette_queue_add was not recorded as a boundary (re-captured: p5_lifebar_init). Not reached (UNVERIFIED): `continue_screen_draw` 0x26260; the debug-mode (7) score lines of hud_lives_draw / hud_player_full_draw (text_printf's format address not known: the port passes 0) |
| Teaser leftovers (proofs-6; A): `player_hit_lose_life` 0x6520 (a hostage shot by a player: hostage_shot_penalty), `hostage_s03_dead` 0x5C460, `sophie_wait_end` 0x963A0, the Chariot's `chariot_attack_r` 0x98C80 (action 18), `chariot_act2_land` 0x98E70 (2), `chariot_act12_turn` 0x98DB0 (12) | `hotd_proof capture <folder>` | `run_capture.ps1`, `-Funcs` / `-Ported` / boundaries = capture_lists.py's lists for the 20 UNVERIFIED L3 / L5 / L7 routines + the draw hooks `9ce60,37d80,87550,689e0,68b50,69540`, fresh auto-aim games, `-Seconds 1100`: p6_def (`-AimPrefer <the hostage states>`, as p3_hshot2), p6_r5 (the same with `-Route 1:5`), p6_2p (`-TwoPlayers -Route "1:4,1:7"`, hostages preferred), p6_chlives (`-AimSkip 97d20,993a0,97a90 -FiniteLives`: new switch, the life counter not kept at 3; the lives ran out in Chapter 1, so no Chariot) | player_hit_lose_life 2 + 10 + 11 + 10, hostage_s03_dead 44 + 43 + 42, sophie_wait_end 2 + 2 + 2 (a fresh game plays Sophie's scene to its end; the recording skipped it), chariot_attack_r 50 x 3, chariot_act2_land 50 x 3, chariot_act12_turn 3 (p6_r5) + 2 (p6_cstart: a plain auto-aim game, `-Funcs 97c80,99260,98db0 -Seconds 420`, boundaries + 0x9A040 / 0x9A280): **0 differ** (2026-10-16), nothing to fix (14,348 coprocessor calls, 1,878 boundary calls replayed in p6_def). `chariot_start_direct` 0x97C80 (1 per game) differed only because 0x9A040 / 0x9A280 (the Chariot's part draw hooks, boundaries in the L7 runs) were not in `-Boundary`: p6_cstart2 (`-Funcs 97c80,99260 -AimPrefer <the hostage states> -Seconds 1100`, boundaries + 0x9A040 / 0x9A280) 1 capture (321 coprocessor calls, 36 boundary calls), **0 differ**. Not reached in any of the four (UNVERIFIED): `chariot_act17_wait_combat` (combat inactive during a walk), `sophie_lying_d` / `sophie_wait_frame37`, hostage scenarios 1 / 6 (`hostage_s01_run(_steps)`, `hostage_s06_*`), `hostage_s03_thrown`, `hostage_thank_leave_step`, `hostage_shot_slide / _fall`, `kind178_scene_skip_frame` |
| Runtime core (proofs-7; A): moved from src/game into the port, proof cases `src/proof/cases/tasks_arcade.inc`. Heap and task creation (src/port/tasks.cpp): `heap_reset` 0x14640 (+ `skin_buffers_alloc` 0x35090), `mem_alloc` 0x14710, `mem_free` 0x149B0, `heap_block_free` 0x14A50, `task_group_create` 0x14C10, `task_create` 0x14EA0, `task_create_child` 0x14FA0, `task_clear_work` 0x15080. Mode 4 (game_flow.cpp): `mode4_game` 0x25C20, `game_play` 0x25F40, `game_frame` 0x262A0, `game_chapter_end` 0x25FF0, `game_chapter_end_wait` 0x26020, `game_continue_begin` 0x26080, `game_continue_countdown` 0x26100, `set_mode` 0xDE70, `player_st4_out_of_lives` 0x5260, `player_continue_frame` 0x5300. Chapter resets / tasks (chapter_tasks.cpp): `shot_lists_clear` 0x453C0, `chapter_reset_53e1b0` 0x32290, `chapter_counters_reset` 0x67480, `camera_reset` 0x6CA60, `attack_slots_task` 0x31C00, `depth_lists_task` 0x53480. Kind 176 (chapter_clear.cpp): 0x9B690, 0x9B880, 0x9B9F0, 0x9BC00, 0x9BDC0. support_c.cpp: `task_create_child_alloc` 0x15010, `spin_axis_from_velocity` 0x33EC0, kind 182 0x9B600 / 0x9B620 / 0x9B660, `object_draw_body` 0x37D80, `body_part_draw_mat` 0x37B80, `tumble_corners` 0x6A970, `tumble_update` 0x6ADD0 (+ `tumble_start` 0x6AF70 inside it), `body_drips_draw` 0x60430. player_join.cpp: `PlStartInit` 0x5870, `player_camdr_mode1` 0x61C0, `player_camdr_mode3` 0x4B7D0. The runtime now draws through the port's object_draw / body_draw / part_draw (l3_cr1) | `hotd_proof capture <folder>` | fresh auto-aim games, `-Ported` / `-Boundary` = capture_lists.py's (a worktree: its tools with `run_capture.ps1 -Root <main checkout>`): p7_tasks (`-Funcs 14640,35090,14710,14810,149b0,14a50,14c10,14ea0,14fa0,15080 -Boundary 17660,a4ec0,14d40 -Seconds 240 -Max 30`), p7_chtasks (`-Funcs 453c0,32290,67480,31c00,53480,6ca60 -Seconds 200 -Max 30 -Every 7`), p7_depth (`-Funcs 53480 -IfNe 520934:520938,52093c:520940`: new switch, the calls where a list changed), p7_flow_fin (`-Funcs 25c20,25f40,262a0,25ff0,26020,26080,26100,5230 -FiniteLives -Seconds 700 -Max 40 -Every 25`), p7_mode4 (`-Funcs 25c20` with the table's 0x25C40 / 0x25D40 / 0x35160 in `-Boundary`, `-Seconds 75`), p7_cont2p (`-Funcs 5260,52c0,5300,26080,26100,5230 -Mortal -TwoPlayers -Every 20`), p7_clear (`-Funcs 9b690,9b880,9b9f0,9bc00,9bdc0,25ff0,26020 -Seconds 1150 -Every 12`), p7_small (`-Funcs 15010,33ec0,9b600,9b620,9b660,67f60 -Seconds 1100 -Every 3`), p7_join (`-Funcs 4f80,4fe0,4ff0,5870,94430 -TwoPlayers -Mortal`, PlStartInit's row routines as boundaries), p7_setmode (`-Funcs 61c0,61f0,4b7d0,4b7f0,de70 -Mortal -Shoot 0`), p7_draw1 (`-Funcs 37d80,37b80,6a970,6add0,6af70,60430 -Boundary 1a80,2320,14120,6b380 -Seconds 1100 -Every 7`) | p7_tasks 164, p7_chtasks 64, p7_depth 10, p7_flow_fin 130 (1 differ: mode4_game's game_new frame, 0x25C40 not a boundary: rerun as p7_mode4, 4), p7_cont2p 82, p7_clear 46, p7_small 67, p7_join 13, p7_setmode 12, p7_draw1 123: **0 differ** (2026-10-16). Fixed on the way: game_submode / 0x51F4A8 written as 32 bits; the play-time counters counted in the continue sub-modes; game_frame's coprocessor focal length (0x7D) never sent; set_mode cleared only the mode / sub-mode; kind 176's grey flag cleared with a 32-bit write (now palette_restore_vblank); the runtime's body_draw left out the fade and object_draw_model the translation / rotation / shadow pass (the port's now run); player_st1_continue: the popup's sprite and the play-time books. Not reached (UNVERIFIED): `heap_block_insert` 0x14810 (unused callers), `player_continue_begin` 0x5230 / `player_continue_start` 0x52C0 (a coin in the countdown), `player_st1_continue` / `_st2_rejoin` / `_st3_join`, `hud_popup_clear`, `player_camdr_mode4` 0x61F0 / `player_camdr2_mode4` 0x4B7F0, `object_draw_model`. The task loop (0x14CE0 / 0x14D40 / 0x150D0 / 0x15110: longjmp) stays the runtime's |
| Blow-off, shadow, leftovers (proofs-8; A): new `src/port/blow_off.cpp`: `part_blow_off` 0x69550 (was a stub), `severed_part_init` 0x39830, `severed_part_fall` 0x69730 (task), `severed_part_draw` 0x38B10, `part_node_draw_rec` 0x38A50, `severed_part_free` 0x38FA0, `floor_blood_splat` 0x43920, `floor_blood_splat_task` 0x45230, `shadow_update` 0x6C9E0 (was a stub), `shadow_projection` 0x6C490; earlier UNVERIFIED: `hangedman_flyby_init` 0x4EEB0 / `_start` 0x4F080, `kind162_end` 0x53360, `kind164_door_burst` 0x96660, `kind178_scene_skip_frame` 0x9C250 / `_end` 0x9C2C0, `sophie_lying_d` 0x962E0, `sophie_wait_frame37` 0x96350, `player_st2_rejoin` 0x4FE0, `colour_blend32` 0x23CC0 | `hotd_proof capture <folder> <regions>` | `run_capture.ps1 -Root <main checkout>`, `-Ported` / `-Boundary` = capture_lists.py's + the draw hooks `9ce60,37d80,87550,689e0,68b50,69540`, fresh auto-aim games: p8_blow (`-Funcs 69550,39830,69730,38b10,38a50,38fa0,390f0,43920,45230,6c9e0,6c490 -Seconds 900 -Max 30`), p8_sweep1 (the 54 UNVERIFIED routines with a case as `-Funcs`, `-Seconds 1100 -Max 10`), p8_sweep2p (the same with `9c250:68`, `-TwoPlayers -Route "1:4,1:7"`). New: mame_capture.lua keeps a capture whose routine calls task_exit itself (task_exit in -Boundary: its record + the state at its entry as the exit); hotd_proof compares a case's returned value (`a.setResult`) with the original's g0 | p8_blow 232 (part_blow_off 11, the piece's task / draw 30 each, severed_part_free 11, splat 30 + 30, shadow_update / shadow_projection 30 each), p8_sweep1 60, p8_sweep2p 65: **0 differ** (2026-10-16) but player_st3_join (1 capture: a 0x7D call in the original inside PlStartInit's row 3, not found; stays UNVERIFIED). Not reached: `part_subtree_free` 0x390F0 (only head subtrees blown off), the camera routines reached through the tables only (cases added: 0x2B180, 0x2BAA0, 0x2C060, 0x2CA30, 0x2D6F0, 0x2D860, 0x2DA00, 0x6CB30, 0x6CC20, 0x6CC40), camera_chapter_reset, the hostage scenarios 1 / 6, the continue-with-a-coin paths, the player scene bodies of earlier rows, chariot_act17_wait_combat, researcher_gift_spawn / _reached_camera, set_motion2_blend, object_draw_model, kind138_prop_group, continue_screen_draw. kind178_scene_skip_frame needs its size given (`9c250:68`: the index stops before its last ret) |
| The last stubs (stubs; A): `eyeball_pop_spawn` 0x43B90, `eyeball_pop` 0x43970 (task), `sophie_skin_draw` 0x21C30, `sophie_skin_build` 0x21920, `geo_polygon_ram_write` 0x19D0 (src/port/blow_off.cpp) | `hotd_proof capture <folder> <regions>` | `run_capture.ps1 -Root <main checkout>`, `-Ported` = p8_blow's + these, `-Boundary 14120,14200,150d0,17660,a4ec0,9ce60,37d80,87550,689e0,68b50,69540`, fresh auto-aim games (captures in wt-stubs build, not kept): cap_eye (`-Funcs 43b90,43970 -Seconds 900 -Max 80`), cap_soph (`-Funcs 21c30 -Seconds 200 -Max 30 -Every 25 -NoSkip`) | cap_eye 82 (spawn 2, task 80; 652 coprocessor calls, geo_draw_model replayed 80 times), cap_soph 30 (390 coprocessor calls): **0 differ** (2026-10-21). Not reached: eyeball_pop's task_exit end, a bounce after the fifth; the second strip's shared corner |
| Player join (proofs-9; A): `player_st3_join` 0x4FF0 (+ its tail `player_body_reinit` 0x5008, src/port/player_join.cpp) | `hotd_proof capture <folder> <regions>` | `run_capture.ps1 -Root <main checkout> -Funcs 4ff0:b4 -TwoPlayers -Route "1:4,1:7" -Seconds 1100 -Max 10`, `-Ported` = capture_lists.py's with `4ff0:b4`, `-Boundary` = capture_lists.py's + the draw hooks: p9_join2 | 1 capture (7 coprocessor calls, play_time_start / 0x17E40 replayed): **0 differ** (2026-10-16). proofs-8's differing capture was the capture's size, not the port: the index gives 0x4FF0 12 bytes (Ghidra split it at 0x5008), so the capture ran on past the routine's ret (at 0x50A0, in 0x5008's range) into the next game frame (game_frame's 0x7D focal call). The size has to be given (`4ff0:b4`) in -Funcs and in -Ported (else play_time_start's call site in 0x5008's range is no boundary). Not reached: the debug-mode body rebuild (0x51F42C != 0). Also `player_camdr_mode4` 0x61F0 and `player_camdr2_mode4` 0x4B7F0 (PlStartInit row 3's set-ups; not captured inside 4FF0's capture, nested): p9_camdr (`-Funcs 61f0,4b7f0`, the same game), 1 capture each, **0 differ** |
| Continue (proofs-10; A): `player_st1_continue` 0x4F80, `player_continue_begin` 0x5230, `player_continue_start` 0x52C0 (+ `player_continue_frame` 0x5300 inside its task), `continue_screen_draw` 0x26260 | `hotd_proof capture <folder>` | `run_capture.ps1 -Root <main checkout> -Funcs 4f80,5230,52c0,26260,94430 -TwoPlayers -Mortal -Shoot 0 -Continue 2 -Seconds 420 -Max 20` (new `-Continue n`: mame_dump.lua HOTD_CONTINUE, player 1 inserts the coins and presses start 2 s after its state turns 4, up to n times), `-Ported` = capture_lists.py (`HOTD_ROOT=<main checkout>` in a worktree) with `4ff0:b4`, boundaries = its list + the prompt table 0xAA450 `20c50,20cb0,20d80` + `94f20` | p10_cont2: 10 captures, **0 differ** after one fix: `player_continue_frame` now calls `player_continue_nop` 0x94F20 (ret only, a new virtual stub) first, as the original does (p10_cont: 2 differing 52C0 captures, the call missing; deleted). Not reached: `hud_popup_clear` 0x94430 (no popup up at a continue) |
| Game over of one player (proofs-10; A): `player_body_place` 0x4B980, `player_body_camdr` 0x62C0, `player_body_hold` 0x6330 (player_st6_game_over 0x5470: the countdown of player 2 runs out while player 1 plays on) | `hotd_proof capture <folder>` | as the continue row: `-Funcs 4b980,62c0:70,6330:20 -TwoPlayers -Mortal -Shoot 0 -Continue 2 -Seconds 420 -Max 10`, boundaries = capture_lists.py + the draw hooks `9ce60,37d80,87550,689e0,68b50,69540` + `20c50,20cb0,20d80,94f20` | p10_gover: 22 captures (3,841 coprocessor calls, 407 boundary calls replayed), **0 differ**, nothing to fix |
| Researcher gift (proofs-10; A): `researcher_gift_spawn` 0x707E0, `researcher_reached_camera` 0x708E0, `gift_effect` 0x1DE90 (kind 64 state 14, Chapter 1 event 7) | `hotd_proof capture <folder>` | `run_capture.ps1 -Root <main checkout> -Funcs 707e0,708e0,1de90 -Route "1:4" -AimSkip 6dbd0 -Seconds 600 -Max 10`, boundaries = capture_lists.py + the draw hooks | p10_gift: 21 captures, **0 differ** after one harness fix: hotd_proof replayed researcher_gift_spawn as a boundary always (now the port body unless the record has the call, as researcher_reached_camera) |
| Hostage scenario 1 freed (proofs-10; A): `hostage_s01_run` 0x566E0, `hostage_s01_run_steps` 0x5A4B0 | `hotd_proof capture <folder>` | `run_capture.ps1 -Root <main checkout> -Funcs 566e0,5a4b0,5c770,5c7c0,5c870 -AimSkip "56630,56660,566e0,56540,565b0" -Seconds 300 -Max 10` (the auto-aim leaves the hostages alone and shoots the captor), boundaries = capture_lists.py + the draw hooks | p10_s01: 20 captures (3,410 coprocessor calls, 400 boundary calls replayed), **0 differ**, nothing to fix. Not reached: `hostage_thank_leave_step`, `hostage_shot_slide / _fall` |
| Opening scene bodies (proofs-11; A): `player_scene00_turn` 0x4BD00, `player_scene00_turn_end` 0x4BD70, `player_scene00_hold` 0x4BDB0 (the opening run's last steps: the turn at camera frame 450; not in the index: sizes given) | `hotd_proof capture <folder>` | `run_capture.ps1 -Root <main checkout> -Funcs 4bd00:70,4bd70:40,4bdb0:30 -Shoot 0 -Seconds 110 -Max 20` (the auto-aim's shots skipped the opening in every earlier run; with `-Shoot 0` it plays to its end), `-Ported` = capture_lists.py, boundaries = the draw hooks | p11_s00: 47 captures (4BD00 20, 4BD70 20, 4BDB0 7), **0 differ**, nothing to fix |
| Hostage scenario 6 (proofs-11; A): `hostage_s06_hanging` 0x56EF0, `hostage_s06_wait` 0x56F30, `hostage_s06_thank` 0x56FC0, `hostage_thank_leave_step` 0x5C770 (the hanging researcher, Chapter 1 event 15's fifth section: record 38260, type 23 action 6) | `hotd_proof capture <folder>` | `run_capture.ps1 -Root <main checkout> -Funcs 56ef0,56f30,56fc0,5c770 -Route "1:3,1:9,1:11=0" -AimSkip "3e030,56f30" -Seconds 900 -Max 10` (event 11's wall pieces and the researcher not shot: event 11 -> 15), boundaries = capture_lists.py + the draw hooks | p11_s06: 22 captures (6,567 coprocessor calls, 841 boundary calls replayed), **0 differ**, nothing to fix. Not reached: `hostage_s03_thrown` (p11_s03 / _s03b: `-HoldFire "1:3:40"` / `"1:3:110"`, no throw), `hostage_shot_slide / _fall` (state 20: no Chapter 1 scenario sets it) |
| Chapter 2 (ch2proof; A): its kinds 183, 168, 75, 131-133, 136, 150-152, 184, 185, 187 (ch2_kinds.cpp), the prop sub-types (props_ch2.cpp), hostage scenarios 7-11 (hostages_ch2.cpp), player scenes 09-14, the bookshelf collapse / remains, frog splash, `area_heading_pick`, `obj_velocity_clear`, `point_clip_to_depth`, `enemy_fall_death_check / _land`, `hangedman_fight_init` (+ `hangedman_captives`), `hangedman_rematch_init / _intro` | `hotd_proof capture <folder> <regions>` | `run_capture.ps1 -Root <main checkout>`, lists = capture_lists.py `--funcs` (84 routines) + the draw hooks: ch2p_full (`-Name play_us_20261007_2238 -Seconds 1620 -Max 40 -Every 4`), ch2p_r10 / _r11 / _r14 (auto-aim, `-Route "2:0,2:5"` / `"2:0=0,2:1"` / `"2:0=0,2:1=0,2:2"`), ch2p_fix / ch2p_door (`-Funcs 96ec0:1f0,41ff0:148`: their ret lies past Ghidra's size) | about 4,000 captures, **0 differ** but capture artifacts of 0x96EC0 / 0x41FF0 without the sizes; nothing to fix ([ch2proof.md](teaser_status/ch2proof.md)). Not reached: `prop_door_fly` (flag 14: no event sets it) |
| Chapter 4 (ch4proof; A): the Magician (boss_magician.cpp, 47 of 49 routines + the shared palette fades / body-part searches), Chapter 4's kinds 22 / 25 / 154, its prop sub-types (props_ch4.cpp), player scenes 17, the iron chain (ch3_kinds.cpp), `chariot_hit_reaction_end`; coprocessor 0x52 / 0x84 | `hotd_proof capture`, `hotd_proof tgp <copro.txt>` | ch4p_full (`-Name play_us_20261007_2238 -Seconds 1620 -Max 150 -Every 20`, capture_lists.py `--file src/port/boss_magician.cpp --funcs ...`; ~2 h in MAME), ch4p_size (`-Funcs 74150:8a0,74c70:250,3b020:140,3f480:180,4e220:50`: not in the index or no `ret` in its size), ch4p_chariot (`-Funcs 99210 -Seconds 1300`) | 2,671 + 585 + 28 captures, **0 differ** but the size-less routines' artifacts in ch4p_full; nothing to fix ([magician.md](teaser_status/magician.md), [ch4.md](teaser_status/ch4.md)). Not reached: `magician_idle`, `magician_fireball_hit`, player 2's scene-17 steps |
| The ending and the last Chapter 2-4 leftovers (verify; A; buildf of worktree wt-verify, not kept in private): the ending moved from src/game into `src/port/ending.cpp` (`game_ending_start` / `game_ending`, `sky_task_ending(_start)`, kinds 105 / 110 / 111 / 116 / 120-122 / 179, the bodies 0x58C40..0x59B10, `player_st7_idle`, `score_rank_get`); Chapter 3's iron ball (`_pos_attack`, `_pos_6`, `_drop`, `_hitbox`), `kind188_spark(_anim)`, `prop121_kikai_item`, hostages s14 rescue / s15 gift / wait, scenes 15 / 16 player 2; Chapter 4's `kind191_route_by_rescues`, scene 17 player 2 | vf_end: `-Name play_us_20261007_2238 -Seconds 1900 -Every 30 -Max 200` (the ending's lists, sizes `353e0:30,35410:b00,35f30:c0,360d0:f0`); vf_end2: the same with `-XRam 1802000:800 -Max 20` (no game_ending in -Funcs, so the tasks it runs are captured); vf_c3a: fresh `-Chapter 3 -Route "3:0,3:2,3:4" -AimSkip <s14 / s15 states> -AimPrefer 3e90 -HoldFire "3:0:25" -Every 3 -Max 150` (sizes `48a50:200,48c50:a0,4e30:90,58410:238,4df20:30,4ded0:50,3fa80:60,49050:90`); vf_c3b: `-Chapter 3 -TwoPlayers -NoSkip`; vf_c4c: `-Chapter 4 -TwoPlayers -NoSkip -Seconds 1300 -Every 10 -Max 100` (to the two-player ending) | vf_end 669 captures: 667 match, 2 capture artifacts (a vblank interrupt in game_ending_start's sprite tile copies; kind 105's palette_grey_build reads palette RAM, not recorded there: vf_end2 with it: match); vf_end2 58: 48 match, 10 differ only in palette RAM written by boundaries (not replayed into -XRam); vf_c3a 1,545: 1 differ (the MAME script's -Route write landed inside a hostage_s14_wait capture: mame_dump.lua now holds it during captures); vf_c3b 1,660, vf_c4c 216: **0 differ**. Not reached: `iron_ball_pos_hit` 0x484D0, s15 rescue 0x58730, `hermit_act24_guard_end`, the bodies of player 2 alone, the scene-17 ret routines. A fresh one-player Chapter 4 game crashes MAME ("I960: Unhandled 00", deterministic at the same point; two-player starts work) |
| The chapter change and the chapter's start table (chstart; A; captures in build\cap of worktree wt-chstart, not kept in private): `game_chapter_start` 0x25D40, `chapter_load` 0x25DA0 (src/port/game_flow.cpp; the runtime's set-up around them: `Runtime::game_chapter_start` / `Runtime::chapter_load`), the start table 0xAB210's `sky_task_start` 0x34130, `input_task_start` 0x5DA0, `InitSetPlayerTask` 0x5D30, `shake_task_start` 0x5DD0, `world_init` 0x4B650, `attack_slots_task_start` 0x31D40, `letterbox_task_start` 0x44AF0, `depth_lists_task_start` 0x53370, `demo_timer_start` 0x23CA0 (src/port/chapter_tasks.cpp); new boundaries `chapter_screen_reset` 0x17280, `hud_mode_init` 0x205A0 | `HOTD_PROOF_VBLANK=1 hotd_proof capture <folder>` | `run_capture.ps1 -Root <main checkout>`, `-Ported` = capture_lists.py's, `-Boundary "14d40,17280,17660,17e40,1a980,205a0,34ff0,a4ec0"`: cs_full (`-Funcs "25d40,25da0" -Seconds 1150 -Max 50`: a fresh one-player game to Chapter 4), cs_2p (the same, `-TwoPlayers -Seconds 420`), cs_2p2 / cs_attract (`-Funcs` + the nine start routines, `-TwoPlayers -Seconds 420` / `-Attract -Seconds 300`), cs_st2p / cs_stattr (only the nine start routines, so they are captured inside chapter_load too: `-TwoPlayers -Seconds 420` / `-Attract -Seconds 200`) | cs_full 4 (game_chapter_start 3: Chapters 1 -> 2, 2 -> 3, 3 -> 4; the new game's chapter_load 1), cs_2p 2, cs_2p2 10, cs_attract 52 (chapter_load 5: the attract demo's and the title's), cs_st2p 26, cs_stattr 66 (every start routine, inside chapter_load too; demo_timer_start in the attract demo, InitSetPlayerTask at the chapter change with both players in state 2): **0 differ** with `HOTD_PROOF_VBLANK=1`. Without it 13 captures differ, every byte in what a vblank interrupt writes: chapter_load busy-waits for the vblank (0x51F706, a palette upload pending), so the interrupt always runs inside its capture (its frame 0x510430.., the once-list 0x511F00.., the palette queue 0x51DCB0.., coin_update's prompts 0x51E820.., counters 0x51F560 / 0x51F79A, vblank_words_upload's colours 0x520660..; one input_task_start capture was hit by one too). The new hotd_proof switch `HOTD_PROOF_VBLANK=1` leaves those bytes out of the comparison (default off: the other sets' counts stay); `HOTD_PROOF_BYTES=1` prints every differing byte (entry / port / original) |

PC Mode on the arcade logic (docs/pcmode.md; pcmode5, 2026-10-20): `hotd_proof pmmaths [THOTD.EXE]` checks the
runtime's copies of PC data against the EXE itself: the character table (Game<A>::pm_table vs 0x5CE348: 7 of 7) and
the table square root (Game<A>::pm_fast_sqrt, its table rebuilt from a formula, vs pc_fast_sqrt with the EXE's table
0x5CCEFC): every one of the 2^32 float inputs, **0 differ** (5 s). pcmode6 (2026-10-20) added the PC's turn and shot
maths (port/pm_maths.h): the rebuilt sin / cos table (8192 floats vs 0x5697B0), arctangent table (1025 ints vs
0xC653B8) and identity matrix (0x5CE4F4): 0 differ; then a differential run of pm::enemy_turn / pm::shot_ray /
pm::ray_sign against Game<Pc>'s enemy_turn_pc / shot_ray_setup / ray_sign_test on a PC memory image made from the EXE
(2,000,000 turns, 300,000 rays, 1,200,000 ray tests) and pm::copro_matrix's layout against the coprocessor model
(1000 cases): **0 differ** (10 s in all). Not a capture proof: the PM runtime has not been compared with a PC trace
(the PM capture files of 2026-10-14 are gone; new ones need a PC harness run).

pcmode7 (2026-10-20) added **THOTD.EXE's own code** to the check: `src/proof/x86emu.h` is a small x86 + x87
interpreter (the integer instructions of Watcom code, the x87 with its precision control: at the PC's 24 bits every
result is rounded once to a float, sums with an integer operand exactly as the x87's extended register would; rounding
control for fist / fistp and frndint, so `__CHP` chops). `src/proof/pmmaths_exe.inc` runs the EXE's routines on the
same PC memory image (Watcom register convention: eax, edx, ebx, then the stack) with random inputs and compares their
results with port/pm_maths.h and the runtime's PM forms. Nothing is captured and THOTD.EXE never runs. 102 routines,
2,065,600 cases, **0 differ**:
- every copy of the matrix stack helpers: rotate y (11 copies + the camera file's), rotate x (7 + camera), rotate z /
  rotate_b (9 + camera; rotate_b is a z turn), translate (10 + camera), transform (12), identity (6), the products
  pc_matrix_mul8 / mul_stored / mul_stored10;
- pc_vec_angles (10 copies), pc_atan2 and the x16 copy, the table square roots (11 copies), the bats file's sin / cos,
  both floor distances, pc_vec_cos, pc_angle_turn (found: the angle + step is the exact sum rounded once, fild + fadd,
  which the float port got wrong for angles past 2^24; fixed in Game<Pc> too);
- camera_matrices_update 0x45F140 run up to its shadow_setup call on random camera records: stored matrix 1 (camera),
  0 (inverse, also left as the current matrix), 5 / 6 (record 0's inverse / view) = pm::camera_matrix /
  camera_inverse;
- on a PC object: move_along_heading, angle_to_camera, walk_to, turn_toward_camera, attacker_list_add's key,
  enemy_target_distance (side from the current matrix), velocity_for_arc / _speed, attack_slot_nearest_object; and two
  where the arcade's form is already the PC's: angle_turn_toward, camera_side_set.
The interpreter stops on any opcode it does not know (never guesses) and the run fails if a routine touches an
unmapped address.

pmboss (2026-10-21) added the bosses and the rest of pcmode7's list: **171 routines, 3,426,100 cases, 0 differ** (35 s
in all):
- each boss file's own helpers block (the Chariot's 0x5134EC, the Hangedman's 0x497070, the Magician's 0x4CF570, the
  Hermit's 0x4EB224, kind 17's 0x50C230; every file has the whole block at the same offsets): rotate y / z / x,
  translate, transform, identity, pc_vec_angles, the table square root, mul8, mul_stored;
- the view test (pm::view_test: pc_view_test 0x4A2808, pc_view_test2 0x4E3124 and the five boss files' copies at
  +0x16B8), with the projection constants 640 / -620 and the viewport -320, -240, 320, 240 that the PC harness
  captures read (k97aim_pm, frog_pm, molb_pm: 0xD53058 / 0xD5305C and *0xD5A5C0 + 0x20..);
- the matrix angles (pm::matrix_angles8: pc_matrix_angles8 0x4E2E84 and the five copies at +0x1418);
- the C library rand 0x578A6B (pm::crand; the thread-data getter stubbed), 100,000 calls from 50 seeds;
- tumble_corners 0x4C5924 (the 8 box corners);
- boss routines with the PC's own rules, restated from the PC code: the Chariot's act 14 (0x516780) and act 12
  (0x5167CC), its hit reaction (0x5169CC: x back to -40, Chapter 1 z to 80, Chapter 4 z to -100), the Hangedman's
  hover point (0x49CAD0, camera_path_eval stubbed: scale 2, odd patterns, +30 to x).
pmboss round 2 (2026-10-27): two whole Magician / Hermit routines with the game's rand 0x471828 and task_create
0x43E400 stubbed: the Magician's fireball aim 0x4D3DEC (every kind / count / 1-2 players: the PC's +-124 / +-82 /
+-164 / 32 offsets, x 0.0020161290, stored 1) and the Hermit's spit 0x4EE4C0 (step 2 up to hermit_hits_and_draw:
the aim x 0.0020161290 through stored 6, the unit vector by the table square root, speed 1.0, scale x 0.3), each
20,000 cases, 0 differ (a changed constant in the restated form is caught: 571 / 20,000 and 20,000 / 20,000 differ).
Now **173 routines, 3,466,100 cases, 0 differ**.
Also new in `hotd_proof pmmaths`: PC Mode's PC products on the arcade's own body matrices (pm::from_copro_words, then
mul_stored with stored matrix 1, or stored 1 times a node with mul) against the coprocessor's 0x77 / 0x09: 4000
cases, largest relative difference 0. The interpreter learned the segment-override prefixes (cs: jump tables) and
`mov al, [moffs]`.

## PM-runtime vs PC-trace differential run (pccap, 2026-10-27)

The first version of the whole-game check (docs/pcmode.md row 6 / pcmode7's "not done"): the PC harness records a
game (state every tick, the gun records, `-Map tools\pcharness\pccap_map.tsv`), `tools/pc_diff.py` turns the record
into per-frame state and player 1's gun, our runtime replays that gun (`HOTD_GUN_REPLAY`) and writes the same state
(`HOTD_STATE_TRACE`), and `pc_diff.py diff` reports the first frame each field differs. How to run it:
docs/pc-harness.md "Differential run". Results (PC copy settings: difficulty 0, 5 lives, max 5: ours `--difficulty 0
--lives 11`):

| Run | Same as the PC | First divergence |
|---|---|---|
| PC Mode, Chapter 1 (`pccap_pm_ch1b`, 17,289 frames, auto-aim) | **since pmbody: all 17,289 frames (below)**; pccap's first version: frames 0-424: mode, chapter, event, rand state, floor_y, enemies alive / count, both players, every shootable object's state, hit points and position (within 0.2; within 0.01 to frame 256); the intro skip (the car), the first zombie's walk, the first hit's damage | frame 425: the first zombie's second hit lands on another body part on the PC (role 9: 35 + 15 = 50 damage) than ours (role 5 again: 45); positions differed by 0.01 there (the parts' camera-space positions come from the arcade's body transform in PM). From then on the play differs (rand at 485) |
| Boss Mode, All Bosses (`pccap_boss_all`) | the Chariot fight: rand, objects, hit points, clock start (670) / stop (2074) / win (2596) frames; the Hangedman's entrance (3139) and clock start (3318) | frame 1703: one Chariot hit one shot later (positions 0.1 apart: the Chariot's motion maths is still the arcade's); the Hangedman's position 0.14 apart at his entrance; the fight's play differs from frame 4281 (a bat hits ours) |
| Boss Mode, Hangedman / Hermit / Magician alone | entrance 542 / 159 / 1648 and clock start 721 / 338 / 1827, as the PC; the Hermit's clock stop frame (2006) too | the fights' play (positions 0.1-0.3) |

Found and fixed on the way: Boss Mode ran the arcade's maths and rules (the PC's run the PC's in every mode: bats
without bob, the PC's camera matrix and shot ray, table square roots, kill credit, shots through scenery): Boss Mode
now takes the `pc_path()` branches; THOTD.EXE has no grandfather clock (kind 137 is `jmp task_exit`), whose two
random numbers had shifted the Chariot fight's; the PC's game_new tick is left out of the frame count.
## Whole-game differential (A vs the runtime, every frame; worker ch4proof, 2026-10-27)

The routine captures above prove each ported routine on its own. The whole-game differential proves the **game as the
runtime plays it** (`hotd.exe`: the port plus the runtime's task loop, stand-ins, draw hooks and vblank work): it
replays a MAME run frame by frame and compares the state at the end of every frame.

- **Arcade side:** `tools\run_diff_rec.ps1 -Out <folder> -Name play_us_20261007_2238 -Root <main checkout> -Chapter n
  [-Seconds s]` (headless, `-str`, MAME's Lua only: tools/mame_diff_rec.lua + mame_dump.lua's session). From the 30th
  game frame of chapter n it writes `snapshot.bin` (work RAM), `state.rec` (HOTDREC1: tools/state_map.tsv's globals
  and every task run, per frame), `full_<F>.bin` (work RAM every 600 frames) and `inputs.bin`: per frame the work RAM
  bytes that changed between the end of the last frame and frame_tick's frame_counter increment (the vblank
  interrupt: guns, flash cycle, counters; wait_vblank's switches; the session's lives / ammo writes), plus the writes
  that came **in the middle** of the frame's logic: an interrupt (i960 process controls bit 13) or a session write,
  each with the number of tasks run before it. Chapter 4 + ending: about 25 min of MAME.
- **Runtime side:** `HOTD_PREF_DIR=<scratch> hotd --root <main checkout> --mute --diff-replay <folder>` starts from
  the snapshot (no chapter start, guards off), applies each frame's writes at the same points (the mid-frame ones after
  the task they interrupted), runs the arcade's `player_input_update` (0x5DF0) in the input task instead of the mouse
  stand-in, and writes `ours.rec` + `ours_full_<F>.bin`.
- **Compare:** `py tools/state_diff.py diff <folder>\state.rec <folder>\ours.rec` (first frames / fields that differ),
  `py tools/ram_diff.py <folder>\full_<F>.bin <folder>\ours_full_<F>.bin --ignore 500000-501fff,510400-5104ff,513000-517fff`
  (the whole work RAM; the ignored ranges are i960 stack frames).

Results (play_us_20261007_2238, the user's recorded game with infinite lives; frames are MAME frame numbers):

| Run | Frames compared | Differ | What is left |
|---|---|---|---|
| Chapter 4 + the ending + the name entry (63351..91804: Chapter 4 to 83134 with the Magician's fight 72176..83132, the ending 83135..91804) | 28,454 | **0** | at 91805 the arcade leaves the game for the ranking screen (mode 7); the runtime's own front end takes over there (not compared) |
| Chapter 1 (1069..40093; arcfix re-recording 1068..41244) | 40,176 | **0** (was 4) | the ammo artefact gone: mame_diff_rec.lua now holds a session write asked for in the middle of a task (the refill) to the next new task, the point where the replay applies it |
| Chapter 2 (19994..37797; arcfix 19993..38943) | 18,950 | **0** (was 2) | the same |
| Chapter 3 (38355..63667; arcfix 38354..64818) | 26,464 | **0** (was 2) | fixed: depth_list_add (frames 54784 / 54818) |
| Two players, Chapter 4 + the Magician + the ending (arcfix `p2`: a fresh game, both players, Lua inputs only; 2323..28852) | 26,530 | **0** | at 28853 the arcade leaves for the ranking (mode 7; the runtime's front end, not compared) |

Whole work RAM at every 600th frame (Chapter 4 run): equal but the i960 stacks, the geometrizer's polygon-RAM address
0x51E8CC and the skin vertex buffers (heap block 0x543B1C): drawing only; in the name entry also the 2D palette shadows.

Divergences found and fixed (first differing frame before the fix):
- `blood_burst` (Chapter 4 frame 71502, one task fewer): the nan worker's `blood_burst_release` ended a burst whose
  part was freed; the original lets it run on, reading the freed block (geo_draw_model's guard skips its NaN draws):
  removed. Chapter 4 then equal to 83135.
- The ending's colour-curve fade-in (83136, task 0x189C0 missing): `palette_curve_fade_in_start` 0x18B50 and its task
  ported (src/port/ending.cpp).
- The name entry's flying letter (91177): depth +0x40 step 0x387BA882 = 6e-5 (was 1.5e-5).
- The coprocessor's MA0 after functions 0x73 - 0x77 (Chapter 1 frame 1222: camera slot 2's yaw of a zero vector;
  Chapter 3 frame 44233: the Taran's matrix yaw): read from the TGP program (0x4B1 / 0x4BA: the slot word; 0x6B8 /
  0x6C0: word 9 of the multiply's left matrix); `hotd_proof tgp` on the from-boot traces still 0 differ.
- The Chapter 3 elevator (47046): stops below 0xC3A9199A = -338.2 (the port compared with -338.4).
- body_part_draw_hook (Chapter 1 frame 4726): type 55 (the Hangedman's fly-by) sets its owner's +0x54 (the bob
  direction) and draws its wing model; the other types' animated models (48, 59, 60, 64, 70, 78, 84, 88, 92, 93).
- Chapter 1's cut-scene props (8015, a task missing): the magazine dropped at a reload (0x9CCD0, frame 2 of motions
  0xC2 / 0xC3) and the cat pieces (0x43330 / 0x44D80, motions 0x1CE / 0x1CF / 0x1D2) ported; the gun's place now kept
  in the part's +0x5C and turned on z too, as the original.
- The runtime's frame and vblank work: frame_tick's counter 0x51F794 (never counted) and frame_counter in the game
  over; the vblank count 0x512064 no longer reset; color_row_set / texture_area_load write their colour rows into the
  shadow tables 0x512D20.. (palette_grey_build averages them) and the area's texture source / palette list;
  palette_queue_add / colour_row_queue fill their queues and palette_flush's work RAM part empties them.

Divergences found and fixed by worker arcfix (2026-10-27):
- `depth_list_add` 0x45480 (Chapter 3 frames 54784 / 54818, an entry MAME had beyond the count): the original leaves
  out only z < 0 (`cmpr`, `bl`), so a NaN z joins the list with key 0. A thrown axe's first frame (`flying_axe_spin`)
  still has its task block's old word 0xFFFFFF82 (a NaN) in +0x40; the port's `!(0 <= z)` left it out.
- The ammo refill (Chapter 1 / 2): a test-harness artefact, now gone at the source (mame_diff_rec.lua, above).

The two-player run (`run_diff_rec.ps1 -Out <dir> -Root <main> -StartChapter 4 -Chapter 4 -TwoPlayers -AimSkip 73010
-Seconds 520`: a fresh game, coins / both starts / the auto-aim from MAME's Lua, the auto-aim never shoots the
fireballs, both players' lives kept; 331 s of MAME) plays Chapter 4 with both players to the ranking: the
two-player paths (`magician_fireball_aim`'s two-player targets, `players_in_game` 2 everywhere) and
`magician_fireball_hit` (7 calls in the replay, both players in play) are now covered, 0 differ. `magician_idle`
(state 0, an empty routine) was still not reached. Not covered: the PC model source and PC Mode (the same arcade
logic; PC Mode has `hotd_proof pmmaths`).

Whole work RAM, arcfix runs (every 600th frame, the same ignore list): Chapter 2 equal; Chapters 3 / 2P equal but
0x51E8CC and the skin buffers; Chapter 1 also 0x51CCB1..0x51D6xx (2D colour data MAME writes in the chapter's first
600 frames, the runtime leaves 0: drawing only, not found yet).

**Whole work RAM, worker arcfix2 (2026-10-27).** `ram_diff.py` over every `full_<F>.bin` of all four recordings
(Chapter 1 66 dumps, 2: 31, 3: 44, 2P: 44; ignored: the stacks and the skin block 0x543B1C..):
- **0x51CCB0..0x51DCAF is not 2D data:** it is `palette_grey_build`'s save of palette RAM bank 2 (the **polygon**
  palette, 1024 words) and its grey copy (decompilation.md "Palette RAM bank 2"). The runtime had no palette RAM, so
  both copies stayed 0. Writer found with a MAME write tap on 0x1802000..0x18027FF from power-on into Chapter 1
  (a variant of tools/mame_boot_writers.lua): palette_init 0x16AB0, texture_palette_upload_vblank 0x1F70 +
  vblank_words_upload 0x24D70, palette_anim_copy 0x24E90, palette_bank2_upload 0x1A300 / palette_restore_vblank
  0x1A710 and the Magician's 0x180248E. **Ported** (src/game/palette.cpp; the runtime keeps the bank as a 2 KB page).
  **Drawing:** the arcade's sepia scenes draw the polygons with the greyed bank; our shader used each polygon's own
  colour. Fixed (renderer.cpp, the vertex shader greys entries 0-7 / 24-1023, the animated ones stay in colour):
  Chapter 1's opening (frame 230) against MAME's sepia shot `private\colors\mame_grey1.png`: the scientist's coat
  120,76,52 = MAME 120,76,52 (was 120,107,84). The 2D layer / HUD / text are not touched by the grey look (their
  palette is banks 0 / 1).
- **The harness:** `mame_diff_rec.lua` now also writes `palette2.bin` (bank 2 at the snapshot; `--diff-replay` loads
  it, older recordings rebuild it from the work RAM) and taps, as mid-frame interrupt writes, the whole palette /
  colour-row queues 0x51DCB0..0x51E18F, the prompt records 0x51E820.. and 0x520660..0x5206AF (the probe's 16-byte
  granules missed a long queue's emptying). `--diff-replay` runs the runtime's 2D set-up without changing the
  snapshot's work RAM (it had queued the banks' palettes again) and does the palette RAM part of a once-list routine
  the arcade's interrupt ran in the middle of chapter_load (the new area's palette).
- **Fixed, game state:** the ending's `sprite_init` (runtime) also ran hud_mode_init's reset of coin_update's
  counter 0x51F560 (2P run, MAME frame 21488: the counter 0x4B02 -> 0); the original's sprite_init 0x170C0 does not.
- Re-recorded in the worktree (build\diff\ch1f / ch2p / ch3p, the same .inp): per-frame state **0 differ**
  (Chapter 1 40,176, Chapter 2 18,950, Chapter 3 26,464 frames; the old recordings and 2P too).
- **Left, all drawing or harness:** 0x51E8CC (the polygon-RAM address); the skin buffers; 0x51E190..0x51E52D /
  0x51E7D0..0x51E800 (`uv_scroll_model`'s water buffer, Chapter 1 from the canal: the runtime draws the water
  itself); 0x51DD58..0x51DDDC (Chapter 1 from 20268, Chapters 2 / 3 / 2P at the last dumps: stale words past the
  palette queue's 0 end, never read: MAME's interrupt emptied the queue between two sprite_bank_load calls of one
  task, the replay can only apply it after the task); 0x51E868 / 0x51E888 (Chapters 2 / 3 at the chapter change: a
  per-player coin prompt record's blink state, written by the game loop's coin_update; the runtime models only the
  free-play prompts).
- `magician_idle`: **unreachable** (teaser_status/magician.md: 0x70AD0 is only in the state table, and no store
  of the state +0x5C can make it 0).

**Whole work RAM, worker arcpal (2026-10-27).** Two of the leftovers above were game RAM after all, now kept:
- **0x51E868 / 0x51E888** are not blink flags: they are +8 of the shared prompt / credit records 0x51E860 / 0x51E880,
  the credits credit_prompt_draw 0x20010 / credit_count_draw 0x1FC00 last showed (+10 the coin fraction). hud_mode_init
  zeroes them at every mode change; on the next frame coin_update sees credits 1 (free play) != 0, copies them and
  restarts its counter 0x51F560. The runtime only ran its prompts outside the game, so after the chapter change they
  stayed 0. `Runtime::coin_credits_seen` (src/game/attract.cpp) now does that check every frame in every mode
  (sprites_frame, after the counter); what free-play players see is unchanged (the arcade's extra prompt draw on that
  frame is not made).
- **0x51E190..0x51E52D / 0x51E7D0..0x51E803**: `uv_scroll_model`'s buffer, ported (decompilation.md "World drawing":
  the copro 0x7B visibility test decides which placements write it).
- Results with the private\diff_recordings recordings (copied into the worktree; the replay writes there), every
  600th-frame dump: **0x51E868 / 0x51E888 and the water buffer equal everywhere** (were: Chapter 1 from 20268, 35
  dumps; Chapter 1 7668 / 8268..; Chapters 2 / 3 at the chapter change). Per-frame state: Chapter 1 40,176, Chapter 2
  18,950, Chapter 3 26,464 frames **0 differ**; 2P 26,530 frames, the one difference is frame 28853 where the arcade
  leaves for the ranking (the same in the old ours.rec). Still left: 0x51E8CC, the skin buffers, the stale queue words
  0x51DCC0..0x51DDDC, and in Chapters 2 / 3's chapter-change dumps 0x51D340..0x51D3E7 / 0x51DB40..0x51DBE7 (these old
  recordings have no palette2.bin, so the bank is rebuilt without an earlier area's longer list tail; arcfix2's
  re-recordings were equal).
- **Blood entry 1023** (drawing; decompilation.md "Palette RAM bank 2"): MAME runs with each blood colour (headless,
  `-str`, Lua inputs; mame_dump.lua's auto-aim session, a snapshot every 0.1 s from 41 to 80 s): the pixels that
  differ between setting 0 and 1 / 2 / 3, brightest tenth: red 207,1,1 -> green 1,145,33 / blue 73,1,207 / purple
  172,1,127. Ours (`--render`, Chapter 1 frames 1000-4000 every 10): red 194,15,12 -> 10,137,29 / 79,24,197 /
  168,20,123 (the same colours; these pixels also hold GGSET's recoloured faces). Custom 00FF00: red 215,0,0 -> 0,215,0
  (only entry 1023 changes). Palette RAM in MAME: 0x801F / 0xB340 / 0xFC12 / 0xE01C, and 0xA94A in the sepia opening.

### pmbody (2026-10-27): the whole Chapter 1 capture

After the PC's body transform and the camera / motion causes it uncovered (docs/pcmode.md "The PC's body transform"),
the same run (`--difficulty 0 --lives 11`, now compared to 9 digits: `pc_diff.py trace ... --digits 9`,
`HOTD_STATE_DIGITS=9`, `--tol 1e-10`) is **identical to the PC over all 17,289 frames** (rand, event, chapter,
floor_y, enemies, every shootable object's state / hit points / position). Frames reached, cause by cause (first difference of any field): 424 (body transform) -> 424 with positions exact to
263 (camera angles) and to 424 (camera paths: the PC's CAMALL.RBH keys) -> 622 (look-at) -> 1990 (turn-back angles,
view position) -> 3349 (a blend from the motion table) -> 3770 (floor_probe) -> rand / events / enemies exact to the
end, positions to 9445 (Murrer floor modes) -> 12191 (barrel); then to 9 digits: 3309 (hostage hands) -> 3764
(spline) -> 4155 (heading moves) -> 5013 (jump arcs) -> 8211 (room ride) -> 11532 (hand anchors) -> the end (the
Chariot's hooks). Left, trace-only: player 1's lives (the capture's `-InfLives`
keeps them at 3) and a bullet-impact mark's stale "shootable" bit (4970).
Boss Mode captures after it, with the bosses' PC forms in Boss Mode too (`--boss-mode <b>`, 9 digits): Chariot and
Hangedman alone identical over their whole captures; Hermit positions 1.5e-5 from 534 (no rand / hit difference);
Magician no rand difference (shootable-effect lists from 1984); All Bosses rand at 12481 (was 4288).
`hotd_proof pmmaths` now also runs the helper blocks of the body file (0x474CE8), the part hook (0x51AF1C), the camera
(0x45D218), the barrel / rain file (0x42F520), camdr (0x4C87C0), hostages (0x4A5AB8), spline (0x465D04) and moves
(0x4C1B60): each block's transform, vec_angles, translate, turns, table square root, identity, mul, mul_stored,
vec_cos (+0x914), invert (+0xE80), matrix angles (+0x1250 / +0x1418) and view test, plus the camera heading 0x4C9D28.
New routines written instruction by instruction from the x87 code (`invert`, `matrix_angles_yp`, `path_curve`,
`camera_heading`, the jump arcs). 321 routines, 0 differ.

### pmrest2 (2026-10-27): every PC capture identical

With the replay's lives as the harness's (`HOTD_LIVES_MIN=3`; the bosses' aggression level reads them), the PC's
camera look mode 2, its eye / target beams, the PC's missing kinds (131 / 146 / 147 / 152 / 175: `jmp task_exit`)
and kind 134's gravity, and pc_diff.py leaving out the heap's leftover words in effect tasks (docs/pcmode.md
"pmrest2"), **all six PC captures are identical to the PC over every frame** (9 digits, `--tol 1e-10 --inf-lives`):

| Capture | Frames | Before (pmbody) | Now |
|---|---|---|---|
| PC Mode Chapter 1 (`pccap_pm_ch1b`) | 17,289 | identical but lives / a stale mark (4970), masked list differences | identical |
| Boss Mode Chariot / Hangedman | 2,660 / 3,664 | identical | identical |
| Boss Mode Hermit | 2,446 | positions 1.5e-5 apart from 534 | identical |
| Boss Mode Magician | 9,162 | shootable lists from 1984, positions from 3626 | identical |
| Boss Mode All Bosses | 18,726 | rand from 12481 | identical |

`hotd_proof pmmaths` adds the camera's look mode 2 (0x45FF68 run from 0x46009B to 0x4600EB) and the whole eye beam
0x51E258 / target beam 0x51E4C8 (the part matrix with a body's scale, the camera, the point, the look angles before:
the angles, the eased look angles and the turned matrix), 20,000 cases each; draw type 1's part routine 0x477EC4 (its
home place, the model draw a `ret`); the iron ball file's helper block 0x48A630: 340 routines, 0 differ.

PC Mode Chapters 2-4 (a new whole-game capture, `pccap_pm_all`; docs/pcmode.md "PC Mode Chapters 2-4"): Chapter 2 now
the same as the PC to its end (to 1e-3; 9 digits but a single 1-ulp frame of a kind 70 at 20478); Chapter 3 first
differs at 33633 (the iron ball's hit boxes), rand from 35463; Chapter 4 not compared.

**pmch34** (2026-10-27): the capture re-recorded; this time the PC played the whole game (76,522 frames, the
ending and the ranking). With the fixes in docs/pcmode.md "PC Mode Chapters 3-4" the replay is the same as the PC
(rand, events, enemies, players, every object to 1e-3) through all four chapters, every chapter ending on the PC's
frame (15180 / 32699 / 50862 / 69380), and into the ending to 76019 (`pc_diff.py diff --tol 1e-3 --inf-lives
--no-ball-boxes`). Left: the dropped iron ball's hit boxes (33633: the PC reads its dead owner's freed and reused
heap block), kind 70's fall target 1e-3 off (from 20486; 9 digits: 20478), the ending's last scene 222 frames
longer. `hotd_proof pmmaths` adds kind 162's helper block 0x49D0A0 (the door's move): 355 routines, 0 differ.

## Re-checking every capture set (proofcheck, 2026-10-20)

All 200 capture sets under `private\` (`proof\*`, `captures\*`: `hotd_proof capture <dir> private\arcade_exports\regions`;
`pcharness\runs\*`: `hotd_proof pccapture <dir>`) were re-run on main after the 4-player merge: 180 at 0 differ, 20
differing. A hotd_proof built at cf0ad57 (before the night of 2026-10-19 / 20) gives the same 17 arcade results (the
Chapter 3 / 4 sets have nothing ported there), so none of that night's merges caused them. Causes and state:

- **Fixed in the port** (a real difference): l6_p128a (`prop_gold_frog_leap` 0x3DBE0 blinks after frame 0x27, the port
  had 8, so it skipped object_draw on even frames 10-38; 6 captures, since L6 wrote it on 2026-10-15): 0 differ.
- **Fixed in the proof cases** (a routine ported after the capture recorded it as a boundary: now `virtual` with a
  `recorded_next` override that replays the record): guard_actions (`walk_to_attack_position` 0x1CA50, ported
  2026-10-14), p7_cont2p (`player_continue_nop` 0x94F20, made a boundary by proofs-10 on 2026-10-16), l5_r5 /
  l5_full1 (`splash_9f1_spawn` 0x44B10, ported by L6), the PC runs all_pm / all_pa_r9 / all_pm_r9
  (`kind32_act4_iron_ball`, `kind32_act4_recover`, `kind32_part_hits`, `kind32_axe_release`, ported 2026-10-14 / 15
  after those runs): all 0 differ.
- **Stale captures, superseded**: kinds_a2 / kinds_ch3 (32 each: kind 5's hidden-model draw `0x93720` / `0x937A0`
  calls geo_draw_model 0x14120, which was not in their `-Boundary` list, so the original's model draw ran inside
  the capture with its coprocessor calls). Re-captured as **kinds_ch3_k5** (`-Name play_us_ch3_20261008_0008
  -Funcs 93720,937a0 -Seconds 400 -Max 30`, kinds_ch3's lists + `14120`): 32 captures, **0 differ**.
- **Known capture artifacts, documented where the capture was made** (the port is not shown wrong; nothing to fix):
  ch3own_full 24 (`iron_ball_drop` / `iron_ball_hitbox` captures run past the routine's end: ch3_kinds.md),
  ch4p_full 15 (size-less routines 0x74150 / 0x74C70: Chapter 4 row), k64_full2 1 (0x6DBD0 #820: the last
  short-size capture, researchers row), l3_cr1 1 (set_motion #2: an interrupt inside the capture, l3.md), p7_flow_fin
  1 (0x25C40 not a boundary, rerun as p7_mode4), p8_sweep1 10 (kind178_scene_skip_frame needs `9c250:68`, proven
  in p8_sweep2p), p8_sweep2p 1 (player_st3_join needs `4ff0:b4`, proven in p9_join2), p5_banks 5 (a vblank
  interrupt inside the long tile copies), p5_lifebar 2 (sprite_bank_load not in its `-Ported`, re-captured as
  p5_lifebar_init), route_ch1 1 (`kind11_ebita_leap_frame` #17: the MAME script's ammo refill 0x51F2C1 landed
  inside the capture; mame_dump.lua holds its writes during captures since then).

So every capture set is at 0 differ or listed here. To re-check: run the commands above over every folder with a
`capture.bin`; the sets listed as artifacts keep their counts.

## Capture tools

- `py tools/capture_lists.py --file src/port/<kind>.cpp [--file ...] [--funcs addr,...]` prints the argument lists
  of a capture, A (`-Funcs` / `-Ported` / `-Boundary`) and PC (`-CapFuncs` / `-CapBoundary`), from game.h, the
  indexes and names.tsv: `-Ported` = every `Game<P>` member with an arcade address that is not a stub (a stub: a `virtual` member
  with a body; a `virtual` one only declared is ported, on the PC unless its proof override replays it with `P::pc ||`) (plus the routines a
  C++ dispatcher handles in a `case F::x:`); boundaries = the unported callees (index, and `call`s past Ghidra's
  short size) of the funcs and of the ported routines they reach, the stubs their C++ calls, and, where the C++ calls
  through an address (`call_routine`, `part_damage_callback`), the `F::` routines and `G::` routine tables it names.
  Arcade sizes by the next-index-start rule, cut at the last `ret` (jump tables after a `bx` allowed; padding followed
  by code or data ends the routine). On the PC, ported PC-only helpers and routines that are stubs on the PC (crand ->
  pc_crand) are never boundaries; callees with no stub at all (C library helpers such as pc_float_chop, a file's own
  matrix copy) are left out with a warning. `--compare-a / --compare-pc <ps1>` diff with a hand-built command line.
  Checked against the 2026-10-15 lists (dog_full2, k32_full4, k32dog_pa): the same funcs and sizes (enemy_voice
  0x2C0 by its jump table); the hand lists had extra boundaries of other kinds (harmless) and the unreferenced
  thrown_tumble_old cluster; the tool adds the dogs' PC matrix stubs and the dynamic targets (enemy_dying,
  Zombie_Dead, hit_spark, enemy_scripted_dead).
- Sizes in `-Funcs` / `-Ported` (`addr:size`): the capture finds a routine's `ret`s inside its Ghidra index size, which
  can stop just before the last one (Zombie_Dead_frame 0x76E50: `76e50:16c`, else only the calls that end on the early
  `ret` are captured); routines missing from the index (kind 96's frames 0x7EC30 / 0x7EC60) need a size as well. A
  routine that always ends in task_exit (kind138_prop_group) has no exit record and cannot be captured.
- `run_capture.ps1 ... -Skip n -Every n -Always "addr:mask,..."` samples the calls that pass `-If` / `-IfArg`: the first n of each routine are skipped, then every n-th is captured, and between them those where one of the words & mask != 0 (a routine that runs every frame over a whole chapter: about 2 MB per capture). `capture_lists.py` reads the decl/ files game.h includes (the per-kind sections).
- `py tools/dump_kind.py <arcade start> <end> [--pc <start> <end>] [--name n]`: compact disassembly into
  `private/dumps/<n>_a.txt` / `_pc.txt` (PC: each index function cut at its index size; notes for overruns, jump
  tables before the next function, and bytes the index size leaves out).
- `tools\pcharness\run_queue.ps1 <queue file>`: runs `run_pc.ps1` argument lines one after another (waits while
  `-MaxGames`, default 2, THOTD run: other sessions' games run side by side with it; `-TimeoutSeconds` on every run), then `hotd_proof pccapture` on each, one summary
  line per run in `private\pcharness\queue_log.txt`.
- **Two-player games** (code that runs only with two players, e.g. the dogs' `dog_st2_move`): `run_capture.ps1
  -TwoPlayers` (A: twice the coins, 2P start 0.5 s after 1P start, player 2's lives kept with the infinite lives);
  PC: `run_pc.ps1 -TwoPlayers -InfLives` with `scripts\game_aim_2p.txt` / `game_pm_aim_2p.txt` (space joins player 2,
  tested on PA: players_in_game 1 -> 2). Player 2 never shoots; player 1's auto-aim plays.
- **Lost games** (proofs-5): `run_capture.ps1 -Mortal` keeps no lives (the game over runs), `-Shoot <mode>` overrides
  HOTD_SHOOT (`0`: the player never fires and is out in Chapter 1's event 3, about 150 s; without it the auto-aim
  plays on and dies later). `-XRam` ranges are recorded at exit too ("XOUT") and compared by hotd_proof.
  `HOTD_PROOF_RANGE=1 hotd_proof capture ...` prints each differing capture's byte ranges (16-byte granules) and fp:
  a vblank interrupt inside a long routine shows as writes to 0x510430.., 0x511FE0, 0x517CA0, 0x51F560 ...

### hangfix (2026-10-29): the Hangedman's phase change on new damage timings

User: the Hangedman "takes a while to realise he is in the 2nd phase". Four new arcade runs (fresh games, MAME's Lua
inputs only, `tools\run_diff_rec.ps1 -Root <main checkout> -Chapter 2`; out of git in the worktree's build\diff):
fast1p (`HOTD_AIM_PREFER=4faa0`: the auto-aim shoots him first; from Chapter 1, `-Seconds 760`), slow1p
(`HOTD_FIRE_DUTY=2:15:1:3`: new mame_dump.lua switch, in Chapter 2 event 15 the gun fires 1 s out of every 4;
`-Seconds 840`), def2p (`-StartChapter 2 -TwoPlayers -Seconds 480`), slow2p (the same with `HOTD_FIRE_DUTY=2:15:1:2`
and the preference, `-Seconds 560`). (A one-player `-StartChapter 2` game stalls in the stage select: frame 2218, the
auto-aim never calibrates; the one-player runs start in Chapter 1.) Replayed with `hotd --diff-replay`:

| Run | Frames compared | Differ | Hit points < 160 (not hurt) | Flies off (state 9) | Dies |
|---|---|---|---|---|---|
| fast1p (17469..43535) | 26,065 | **0** | 32906 | 33526 | 34750 |
| slow1p (17469..48137) | 30,667 | **0** | 33998 | 34418 | 36524 |
| def2p (2323..27441) | 25,117 | **0** | 17999 | 18585 | 19829 |
| slow2p (2323..32043) | 29,719 | **0** | 18736 | 19276 | 20963 |

Verdict: the port matches the arcade; the delay is the arcade's. `hangedman_hover` (0x509F0, state 6): below the
block's share 7 in event 15 he is "not hurt" at once but waits for `camera_frame >= block +0x20` before state 9, 420 -
620 frames in these runs (`py tools/hang_phase_trace.py <folder>/state.rec`: the task with routine
0x4FAA0: state +0x5C, pattern +0x320, hit points +0x70, flags +0x324 bit 3).

Gallery boss numbers from the same kind of data: the boss life bar's maximum 0x543536 in the MAME work RAM dumps
(private\diff_recordings full_*.bin): Chariot 480 (Chapter 1 and 4), Hangedman 800 (Chapter 2) / 560 (Chapter 4),
Hermit 160, Magician 600; the types as the arcade's title sprites print them (0xDF / 0xE0 / 0xE1 / 0xE2: 27, 041,
6803, 0).
