# What is still not proven (worker "verify", 2026-10-20)

Every ported routine is proven by running it on recorded arcade state (MAME captures, `hotd_proof capture`, see
[../proof.md](../proof.md)). This page lists what is **not** proven yet, why, and what capture would prove it.
`py tools/unverified_list.py` lists the code's own `UNVERIFIED (teaser)` marks; this table is the same list, sorted
by what a player sees. "Result" = this round (worker "verify"; captures in wt-verify's buildf, the command lines in [../proof.md](../proof.md), the "verify" row). **No port difference was found: every routine captured matched the arcade** (the few differing captures are capture artifacts, explained there). After the round: 206 capture sets re-run on the merged build, 191 at 0 differ, the 15 others the known artifacts.

Update (worker "chstart", 2026-10-21): the chapter change (game_chapter_start / chapter_load) and the chapter's start
table moved from the runtime into the port and proven against fresh MAME captures (section "The chapter change").

## Chapter 2-4 enemies, bosses, props (src/port)

| Routine | Chapter | Why unproven | What would prove it | Result |
|---|---|---|---|---|
| `iron_chain_*` 0x48F20..0x49A70 (7 routines: the iron ball's chain) | 3 | marks never removed in decl/ch3_kinds.h | ch4p_full already has them | **proven**: ch4p_full re-run today, 0 differ (start 1, init 1, task 139, slack 1, simulate 130, fwd 59, back 124) |
| `iron_ball_drop` 0x48A50, `iron_ball_hitbox` 0x48C50 | 3 | ch3own_full differed: capture artifact (Ghidra size cut before the last `ret`, so the capture ran on into the next task) | the same routines with sizes `48a50:200`, `48c50:a0` | **proven**: vf_c3a 150 + 150, vf_c3b 150 + 150, 0 differ |
| `iron_ball_pos_attack` 0x48170 (the throw), `_pos_hit` 0x484D0, `_pos_6` 0x48920 | 3 | the recording's player always shot the zombies out of the wind-up | a fresh Chapter 3 game holding fire in event 0 | **proven** `_pos_attack` 150 / 150, `_pos_6` 150 / 150 (vf_c3a / vf_c3b), 0 differ; `_pos_hit` 0x484D0 still not reached |
| `kind188_spark` 0x4E30 / `_spark_anim` 0x4D90 (the route spot shot) | 3 | event 4 not on the recording's route; 0x4E30 also needs size `4e30:90` | Chapter 3 route 0 -> 2 -> 4, the auto-aim preferring 0x3E90 | **proven**: 0x4E30 21 + 21, 0x4D90 141 + 141, 0 differ |
| hostages s14 rescue 0x58410, s15 gift / wait / rescue 0x58650 / 0x586B0 / 0x58730 | 3 | the recording never rescued them (event 2 not on its route) | Chapter 3 route 0 -> 2, the auto-aim not shooting the hostages | **proven** s14 rescue 106 + 106, s15 gift 1, s15 wait 21 (vf_c3a: 1 capture of s14 wait differed only by the MAME script's route write, now held during captures); **s15 rescue 0x58730 still not reached** (the s15 hostage was never freed) |
| `prop121_kikai_item` 0x3FA80 | 3 | not reached; size `3fa80:60` | the same Chapter 3 run | **proven**: 2 + 2 captures, 0 differ |
| scene 15 / 16 player 2 steps 0x4DBB0 / 0x4DC00 / 0x4DC60 / 0x4DED0 / 0x4DF20 | 3 | one-player recording | a two-player Chapter 3 game with the cut-scenes played (`-TwoPlayers -NoSkip`) | **proven** (vf_c3b): 7 / 26 / 5 / 35 / 101 captures, 0 differ |
| scene 17 player 2 steps 0x4E0B0 / 0x4E100, the ret routines 0x4E0A0 / 0x4E150 / 0x4E270 | 4 | one-player recording | a two-player Chapter 4 game (`-TwoPlayers -NoSkip`) | **proven** 0x4E0B0 3, 0x4E100 8 (vf_c4c), 0 differ; the three ret routines were never entered |
| `kind191_route_by_rescues` 0x4D50 | 4 | not in the ch4p capture list | any Chapter 4 game | **proven** (vf_c4c: 1, 0 differ) |
| `magician_idle` 0x70AD0, `magician_fireball_hit` 0x73820 (a fireball shot down), the two-player paths | 4 | not reached by the one-player recording | a two-player Chapter 4 game; the auto-aim shooting fireballs | **done for `magician_fireball_hit` and the two-player paths** (arcfix, 2026-10-27: the whole-game differential's two-player Chapter 4 run, docs/proof.md, 0 differ; `magician_idle`, an empty routine: unreachable, arcfix2 2026-10-27, magician.md). Earlier: vf_c4d (`-Chapter 4 -TwoPlayers -AimPrefer 73010`, the Magician's lists, `-Every 20`) runs at about 1/30 speed (5+ hours to the fight) and was stopped; a one-player fresh Chapter 4 start crashes MAME ("I960: Unhandled 00") |
| `hermit_act24_guard_end` 0x86100 | 3 | no sequence of the recording uses action 24 | a Chapter 3 game that reaches it (none found) | not reached in vf_c3a / vf_c3b either |
| `chariot_act17_wait_combat` 0x99260 | 1 / 4 | the auto-aim never lost all its lives in the fight | a game that loses its last life at the Chariot (`-Mortal`) | not tried |
| `prop_door_fly` 0x3EB90 | 2 | **not reachable**: only flag 14 starts it, and no event script of any chapter sets flag 14 | none | - |

## The ending (moved into the port this round)

| Routine | Why unproven | What would prove it | Result |
|---|---|---|---|
| `game_ending_start` / `game_ending`, kinds 105 / 110 / 111 / 116 / 120 / 121 / 122 / 179, `sky_task_ending`, the bodies (kind 103 set-ups 0x58C40..0x59A80 and frames), `player_st7_idle` | written in the runtime (src/game), so no capture could check them | moved into src/port/ending.cpp; the full recording to its end and a two-player Chapter 4 game | **proven**: vf_end 669 captures (667 match; the 2 others: a vblank interrupt inside game_ending_start's tile copies, and kind 105's palette RAM not recorded, which vf_end2 recorded: 0 differ), vf_end2 58, vf_c4c 216 (two players: the bodies p1 b / p2 b, the results), 0 differ. Not reached: the bodies of player 2 alone (0x59660 / 0x596F0) |

## The chapter change (moved into the port: worker chstart, 2026-10-21)

| Routine | Why it was unproven | What proved it | Result |
|---|---|---|---|
| `game_chapter_start` 0x25D40, `chapter_load` 0x25DA0 | the runtime's own form (`Runtime::chapter_continue`): players kept in state 5 (the arcade: 5 -> 2, so their task is player_st2_rejoin: a full gun and 90 frames without hits), no sub-mode 1 frame, chapter_load's 2D part (chapter_screen_reset, the sprite banks, hud_sprites_load, hud_mode_init) not run there | moved into src/port/game_flow.cpp (the runtime keeps its own set-up around them: `Runtime::game_chapter_start` / `Runtime::chapter_load`); fresh MAME games captured at every chapter change | **proven**: cs_full (one player, Chapters 1 -> 2 -> 3 -> 4: 3 chapter changes + the new game's chapter_load), cs_2p / cs_2p2 (two players, 1 -> 2), cs_attract (5 chapter_loads of the attract loop, mode 3 and the title's): every capture matches except the bytes a vblank interrupt writes (chapter_load busy-waits for the vblank: `HOTD_PROOF_VBLANK=1`, docs/proof.md): 0 differ |
| the start table 0xAB210: `sky_task_start` 0x34130, `input_task_start` 0x5DA0, `InitSetPlayerTask` 0x5D30, `shake_task_start` 0x5DD0, `world_init` 0x4B650, `attack_slots_task_start` 0x31D40, `letterbox_task_start` 0x44AF0, `depth_lists_task_start` 0x53370, `demo_timer_start` 0x23CA0 | written in the runtime (`Runtime::chapter_task_start`) | moved into src/port/chapter_tasks.cpp (InitSetPlayerTask keeps a runtime form for a player started straight in play and 4P seats 3 / 4) | **proven**: cs_stattr (the attract loop: the title's, the demo's and the games' chapter loads, 66 captures), cs_st2p (a two-player game through the Chapter 1 -> 2 change: InitSetPlayerTask with both players in state 2, 26), cs_2p2 / cs_attract (8 + 47 more from the other task groups): 0 differ |

## The last stubs reached in play (worker stubs, 2026-10-21)

| Routine | Why it was unproven | What proved it | Result |
|---|---|---|---|
| `eyeball_pop_spawn` 0x43B90, `eyeball_pop` 0x43970 (task): a zombie's eye flies out of its head, bounces, leaves blood splats | a stub: no eyeball, and its two `rand` calls (spawn, each splat) were skipped | ported 1:1 from the disassembly (src/port/blow_off.cpp); a fresh MAME game | **proven**: cap_eye (`-Funcs 43b90,43970 -Seconds 900 -Max 80`) 2 spawns + 80 task frames, 0 differ. Not reached: the task's end (frame 240) and a bounce after the fifth (no splat) |
| `sophie_skin_draw` 0x21C30, `sophie_skin_build` 0x21920, `geo_polygon_ram_write` 0x19D0: Sophie's waist strip | a stub: her strip was never built or drawn | ported 1:1 (blow_off.cpp); the view draws the strip from the loader's template (`Runtime::sophie_skin_draw`, SkinDraw type 134); arcade models only (the PC draws none) | **proven**: cap_soph (`-Funcs 21c30 -Seconds 200 -Max 30 -Every 25 -NoSkip`) 30 captures, 0 differ (the geometrizer writes are hardware, not in the captures). Not reached: the second strip's shared corner (-1 in its map) |

## Chapter 1 leftovers (from proofs-2 .. proofs-11)

| Routine | Why unproven |
|---|---|
| camera routines `cctb33_mode5(_start/_frame/_impulse)`, `cam_job32_offset`, `camdr_mode2`, `game_cam_nop`, `unused_2d5d0 / _2d600`, `ChgCamMode_undefined`, `camera_chapter_reset`, `cam_next_job`, `game_cam_turn_to_point` | **not reachable**: no event script uses them (EVTALL.RBH, all 7 lists); some are debug / unused or have no callers |
| `player_scene05_knocked_flying_p_p(_p)`, `player_scene07_jump_down_a_p1_p_p_p` | the scenes end before these steps in every game tried (p11_s05 / s07) |
| `hostage_s03_thrown`, `hostage_shot_slide / _fall` | no throw in any capture; state 20 is set by no Chapter 1 scenario |
| `set_motion2_blend`, `object_draw_model`, `part_subtree_free`, `kind138_prop_group`, `heap_block_insert`, `hud_popup_clear` | not called in a 1100 s fresh game (p10_sweep) or the recordings; `heap_block_insert` only from unused routines |

## Runtime parts (src/game): not ported routines, the teaser's own forms

These are not single arcade routines run on arcade state, so a capture cannot check them as they are. To prove one it
has to be moved into src/port first (as the ending was).

| Part | Why unproven | What would prove it |
|---|---|---|
| task loop (`task_exit`, `task_unlink`, `task_group_run`) | C++ exceptions stand in for the arcade's longjmp | not capturable as one routine |
| `world_draw_task` 0x4B300, `letterbox_task` 0x44840, `body_part_draw_hook` 0x9CE60 | drawing: the runtime's renderer does the hardware part | port the RAM part, capture |
| game over screens (gameover.md) | the 3D parts are the port's and proven (proofs-5); colours / timing only checked by eye | MAME screenshots side by side |
| display, render, HUD, menus, PC Mode screens (display.md, render.md, hud.md, menuart.md, pcmode.md) | pictures, not logic: compared by eye with MAME / PC harness shots | screenshots |
| PC versions' own code for Chapters 2-4 (PC Mode, the PC port's forms) | Chapters 2-4 are ported in their arcade forms only (`--models pc` changes the pictures, not the logic); the PC harness has only Chapter 1 scripted games | PC harness games of Chapters 2-4 |
