# Worker "ch3own": Chapter 3's own kinds, prop sub-types, hostage scenarios and player scenes (2026-10-19)

Plan: [chapter2_3_plan.md](../chapter2_3_plan.md), row "ch3". Branch `claude/ch3own`. Files (new): `src/port/ch3_kinds.cpp`,
`props_ch3.cpp`, `hostages_ch3.cpp`, `decl/ch3_kinds.h`, `decl/props_ch3.h`, `decl/hostages_ch3.h`, `inst/ch3_kinds.inc`,
`src/proof/cases/ch3_kinds_arcade.inc`, `props_ch3_arcade.inc`, `hostages_ch3_arcade.inc`; one registry line each in
`game.h`, `game.cpp`, `proof/main.cpp`. Everything reaches the port through the runtime's dispatch (the cases files):
kinds by their routine, prop sub-types through 0xAF960, hostage scenarios through `hostage_call`'s default, the player's
cut-scene actions through CCTB 18. All arcade forms, **UNVERIFIED (teaser)**, written from the disassembly
(`tools/i960dis.py`; Ghidra's C garbles the coprocessor FIFO code and missed arguments, e.g. `iron_ball_pos_swing`'s
motion, `body_node_pos_at_frame`'s node offsets, kind 106's / 186's draw matrices).

## Ported

| Batch | Routines |
|---|---|
| 1 | the iron ball of kind 32 action 4 (Chapter 3 event 0): `iron_ball_follow` 0x47C70, `_pos_idle`, `_pos_swing` (motion arg), `_pos_attack` (the throw: clip at depth 10, `projectile_hit_player`), `_pos_hit`, `_pos_pull` (+ `_pull_sub`), `_pos_6`, `_pos_7`, `_drop`, `_hitbox` (trail hit boxes), `_draw`, `iron_ball_hand_pos` / `_node_pos` / `body_node_pos_at_frame` 0x49BB0, `iron_ball_in_reach` (= 1); kind 106 padlock + lock spark; prop sub-types 87 / 88 / 93 / 95 (`prop_ch3_machine`), 94, 96, 121 (+ run); hostage s12 (cell) |
| 2 | kind 153 Taran: set-up, `kind153_taran_frame` (with its Ghidra split `kind153_taran_reach_check` inline), `_fall_off`; kind 189 locked door + `kind189_lock_task` (the locks pick the route) |
| 3 | hostages s13 (platform gift), s14 (voice message, sets the route), s15 (gift); kind 186 moving object (+ frame), kind 188 route spot (+ `kind188_spark`, `_spark_anim`) |
| 4 | player cut-scene actions 15 (`player_scene15_jump_down_kb` + 5 steps) and 16 (`player_scene16_run_jump` + 5 steps, two of them unnamed: 0x4DED0 / 0x4DF20 named `_p2` / `_p2_p` here) |
| 5 | former boundaries first reached in Chapter 3: `part_hit_class17` 0x43860 + `part_class17_anim` 0x450B0 (core_bounds.h stub removed), `room_ceiling_bounce` 0x2FD30, `impact_effect_spawn` 0x30C90 + `impact_effect` (agile.h stubs removed); still virtual, PC forms vtable only |

Kind 150 (event 3) was already ported by worker "ch2"; the Hermit / Curien are worker "hermit"'s (merged from main).

## Results

`hotd --chapter 3 --strict --autofire --lives-kept 1 --mute --frames 90000`, both model sources identical:

| Route | Events | Chapter end |
|---|---|---|
| `--default-route` | 0 1 3 6 7 | 27279 (Hermit killed, no guard) |
| `--route 3:0=1,3:2=0` | 0 2 3 5 7 | 22269 |
| `--route 3:0=1,3:2=1,3:4=0` | 0 2 4 6 7 | 21564 |
| `--route 3:0=1,3:2=1,3:4=1` | 0 2 4 5 7 | 20949 |
| `--route 3:0=0,3:1=1,3:4=0` | 0 1 4 6 7 | 22419 |
| `--route 3:0=0,3:1=0,3:3=1` | 0 1 3 5 7 | 26664 |

Before this worker `--strict` stalled at event 0 (0xF4094 WAIT_ENALV, frame 874: the iron-ball zombies waiting for
their ball). Boundary report (default route, both sources): only the mouse stand-in, `--lives-kept`, and
`iron_chain_start` (below). Two players (`--players 2`): ends too. Chapter 1 (16695, then Chapter 2 36962) and
Chapter 2 alone (17162) unchanged on both sources; `--input-test` 524 / 524.

Note: porting `room_ceiling_bounce` moved the default route's end from 24489 to 27279 (kind 70 creatures now bounce
off the ceiling plane and keep attacking: more lives lost under `--lives-kept`); this is the routine as the disassembly
has it, not checked against MAME.

## Batch 6 and the gate (2026-10-19, second round)

- **The chain** ported: `iron_chain_start`, `_init` (coprocessor 0x52 modelled in `port/tgp.cpp`: yaw / pitch /
  length, from the TGP program at 0x151), `_task`, `_slack`, `_simulate` (0x84 modelled: the program at 0x60B, length then
  0x83's angles), `_constrain_fwd` / `_back`. The ball no longer writes through a null chain pointer. Seen in a
  `--render --shot-at` picture behind the event 0 window. **Proven on A (worker "ch4proof", 2026-10-19)**: capture
  ch4p_full (see magician.md; play_us_20261007_2238 from boot): `iron_chain_start` 1, `_init` 1, `_task` 139,
  `_slack` 1, `_simulate` 130, `_constrain_fwd` 59, `_constrain_back` 124 captures, 0 differ (UNVERIFIED marks
  removed). The two coprocessor models: `hotd_proof tgp ch4p_full\copro.txt` (the full from-boot trace, 55.3 M
  calls) has **0x52 8 / 8 and 0x84 22,465 / 22,465 equal** but one: trace line 7,178,019 is a 0x84 header logged
  with no input or output words ("42008484 in  out"), and the next few lines' outputs are out of step by a word.
  That is a fault in the log, not the model, so 0x52 / 0x84 are no longer UNVERIFIED in port/tgp.cpp. The same
  trace also has 0x85 x 190 not modelled, and 190 differ each in 0x20 / 0x71 (not the chain's; open).
- names.tsv: `player_scene16_run_jump_p2` 0x4DED0 / `_p2_p` 0x4DF20 (PC pairs not identified); globals.h regenerated
  (it also picked up names other workers had added to names.tsv). ch3_boundaries.md: the new reports (2 entries: the
  mouse stand-in, `--lives-kept`).
- **Fix (runtime, `src/game/chapter.cpp`)**: `game_over_route_task` 0x5CF50 reads scene_history[0] (Chapter 1's first
  event) for its route in every chapter; a game started straight into Chapter 2 / 3 had 0xFF there and read a table
  outside the ROM (bot game overs: bad_access 1800, then a soft-lock). `chapter_start` now sets that byte to 0 for a
  chapter > 0 (what the arcade always has, as it always plays Chapter 1 first).

Gate (`tools
obust_seeds.ps1 -Extra --chapter,3,...`, 32 seeds, both model sources, 1 / 2 players, some `--strict`):
**no crash, assert, hang, soft-lock, bad access or NaN draw**. Clears: 5 (seeds 10, 22, 30 arcade 2P; 19 (strict), 31
PC 2P). **No one-player bot cleared**: every one-player bot (even with 100+ continues, `--bot-gameovers 40`) stays in
event 0: the two iron-ball zombies behind the window swing their balls, and the swinging ball leaves a trail of shootable
hit boxes (`iron_ball_hitbox`, ~18 alive, 24 frames each, shot = spark only) that soak up the bots' shots, and the thrown
ball hits a player who has not shot the zombie in its wind-up. This is the routines as the disassembly has them; whether
the arcade's zombies are as hard to hit there needs a MAME look. `--render` (default route, both sources): GL errors 0.
Chapter 1 -> 2 -> 3 with `--continue-chapters`: 16695 / 36962 / 58374, both sources. **Chapter 3's playable flag is NOT
flipped** (the one-player bots never clear).

## Left

- The one-player bot / event 0 question above (MAME comparison of the iron-ball zombies), then the playable flag.
- Proofs (captures) of everything above; PC forms.

## Proofs (worker "hermit", 2026-10-19, in progress)

- Harness: `part_hit_class17`, `room_ceiling_bounce`, `impact_effect_spawn`, `iron_ball_in_reach`, `iron_ball_hand_pos`
  (ported by ch3own) were still fixed boundaries in `src/proof/bounds/{core,agile,kit_zombie}.inc`: they now run the port
  unless the capture recorded the call (as `spin_axis_from_velocity`); proof cases added for them and for
  `iron_ball_pos_idle` / `body_node_pos_at_frame` (`cases/ch3_kinds_arcade.inc`). hermit_full re-checked: 0 differ.
- Capture `ch3own_full` (`run_capture.ps1 -Root <main> -Name play_us_20261007_2238 -Seconds 1110 -Max 60 -Every 10`,
  the 62 routines of ch3_kinds.h / props_ch3.h / hostages_ch3.h before the chain batch, from `capture_lists.py --funcs`,
  kind 153's sizes given by hand `638d0:1e8,63ac0:b88` (the frame runs through its Ghidra split 0x64224 to the ret at
  0x64644)), `hotd_proof capture`: **1,321 captures (61,791 coprocessor calls, 10,580 boundary calls replayed): 1,297
  match**; the 45 routines captured with 0 differ lost their UNVERIFIED mark (decl comments + the .cpp notes).
  Differ: `iron_ball_drop` (17 / 17) and `iron_ball_hitbox` (7 / 60): in each the original's record goes on into the
  next task (task_current 0x5122A8 differs, then a geo_draw_model the port does not make), i.e. the captures run past
  the routine's own end (task_exit / the ball's last frame); not a port difference shown yet, both stay UNVERIFIED.
  Not reached (stay UNVERIFIED): 0x48920, 0x48170, 0x484D0, 0x4D90, 0x4E30, 0x3FA80, the scene-15 / 16 steps
  0x4DBB0 / 0x4DC00 / 0x4DC60 / 0x4DED0 / 0x4DF20, hostage s14 / s15 rescue and s15 gift / wait (0x58410, 0x58650,
  0x586B0, 0x58730); the chain routines (0x48F20 .. 0x49A70) were not in this capture.
- **Event 0 in MAME** (per-frame state of the recording, `run_state_rec.ps1 -Name play_us_20261007_2238 -From 36500`,
  private\proof\ch3_ev0_state2): Chapter 3 starts at tick 38325; the two iron-ball zombies are up at ~38950 (alive 2,
  18 ball hit boxes, 36 once the balls drop); the player fires 18 shots in 275 ticks (one every ~11 ticks), the first
  zombie dies at 39127 (after ~15 shots; it had started its throw (0x477D0) at ~39060 and was shot out of it), the
  second at 39240; **0 player hits** there (2 in the rest of event 0, which ends at 45048). So the recording's player
  gets through by shooting the zombies' bodies fast enough to break the wind-up; it does not show the one-player bot's
  problem: the bot (every 15 ticks, nearest shootable) spends its shots on the ball hit boxes.

## Proofs (worker "verify", 2026-10-20)

Fresh MAME games (docs/proof.md, the "verify" row; [unverified.md](unverified.md)): `iron_ball_pos_attack` (the throw),
`_pos_6`, `iron_ball_drop` / `_hitbox` (with the right sizes: the ch3own_full differences were the capture running on
past the routine), `kind188_spark(_anim)`, `prop121_kikai_item`, hostages s14 rescue / s15 gift / wait, the scenes
15 / 16 player 2 steps: **0 differ**; the iron chain's marks (proven in ch4p_full) removed. No port fix needed.
Still not reached: `iron_ball_pos_hit`, s15 rescue, `hermit_act24_guard_end`.
