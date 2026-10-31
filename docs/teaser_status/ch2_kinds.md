# Worker "ch2": Chapter 2's own kinds, prop sub-types and hostage scenarios (2026-10-17)

Plan: [chapter2_3_plan.md](../chapter2_3_plan.md), worker "ch2". Branch `claude/ch2`. Files (new): `src/port/ch2_kinds.cpp`,
`props_ch2.cpp`, `hostages_ch2.cpp`, `decl/ch2_kinds.h`, `decl/props_ch2.h`, `decl/hostages_ch2.h`,
`src/proof/cases/ch2_kinds_arcade.inc`, `props_ch2_arcade.inc`, `hostages_ch2_arcade.inc`; one registry line each in
`game.h`, `game.cpp`, `proof/main.cpp`. Hostage scenarios reach the port through `hostage_call`'s default (the
runtime's dispatch), prop sub-types through the table 0xAF960 (`prop_object`): no Chapter 1 file changed.
All routines arcade forms only, `UNVERIFIED (teaser)` (read from the decompilation, the disassembly where Ghidra's
coprocessor code was garbled).

## Measure

`hotd --chapter 2 --frames 90000 --autofire --mute --lives-kept 1 --default-route --models arcade --boundary-report`
(cut guards on: "guards" = teaser guards passed) and the same with `--strict` (where it stalls). Chapter 1:
`--frames 20000` both model sources, logs compared line by line with the build without this worker's cases (identical:
clear at 17104), `--input-test` 518 / 518.

| Batch | Ported | Guards | Report entries | Chapter end | `--strict` stalls at |
|---|---|---|---|---|---|
| before | - | 37 | 67 | 54752 | 0xED018 WAIT_ENALV (frame 3065) |
| 1 | kinds 183, 168, 75, 131, 133; props 15, 64, 67; hostage s07 | 28 | 51 | 45149 | 0xEE324 WAIT_ENALV (frame 5250) |
| 2 | kind 136 (+ prop023 leaves); `prop_flag_swap` (+ glass shatter, anim once); hostages s08, s09, s10 | 5 | 23 | 19991 | 0xF3994 WAIT_ITEM 195 (frame 11280: the Hangedman) |
| 3 | kind 132 (the vomiting Ebitan); props 14 (fish), 78 / 79 (door, `prop_door_fly`); `enemy_fall_death_check` / `_land` (kind 3) | 5 | 19 | 20172 | same (the boss) |
| 4 | branch kinds 187, 184, 185, 151 (+ prop080), 152 (+ prop085); props 77 (wall + pieces), 83 (+ fall), 84; hostage s11 | 5 | 19 | 20172 | same (the boss) |
| 5 | kind 150 (the sliding Ebitan, also Chapter 3); `prop008_bookshelf_topple`; `prop_hit_drop` (+ piece), `prop_lid_fly` (+ `prop_fall_spin`) | 5 | 17 | 20172 | same (the boss) |

After batch 5 every Chapter 2 route (default and the three branch routes below) reaches the Hangedman with no guard;
the unported entries left in the reports are the boss (`hangedman_fight_init`, `billboard_model`: worker "hanged")
and the player's scripted scene motions (`player_scene09..14_*`, not this worker's list). Chapter 3 default route
still runs (no crash; its own scenarios s12-s14 are the ch3 worker's).

Branch routes (`--route`, no `--default-route`; autofire breaks event 0's lid, which picks event 10 by itself):

| Route (`--route`) | Events | Guards before batch 4 | After batch 4 | Unported left (not the boss) |
|---|---|---|---|---|
| `2:0,2:5` | 0 10 12 13 4 5 6 8 9 15 | 44 | 5 | kind 150 (event 10), `prop008_bookshelf_topple` (both ported in batch 5) |
| `2:0=0,2:1` | 0 1 11 12 13 4 5 7 8 9 15 | - | 5 | `prop008_bookshelf_topple` (batch 5) |
| `2:0=0,2:1=0,2:2` | 0 1 2 14 4 5 6 8 9 15 | - | 5 | `prop008_bookshelf_topple` (batch 5) |

After batch 2 the default route (events 0 1 2 3 4 5 7 8 9 15) plays without any guard up to the boss: the 5 guards
left are event 15's WAIT_ITEM 192-195 / 199 (worker "hanged"). Two hostages rescued in the bot run.

Shared-file touches: `src/port/decl/props.h` lost the `prop_hit_drop` / `prop_lid_fly` stubs (same treatment);
`src/port/decl/core_bounds.h` lost the `enemy_fall_death_check` stub (ported in ch2_kinds.cpp,
still `virtual` so the proof's boundary override in `bounds/core.inc` stays; `inst/ch2_kinds.inc` instantiates its
empty PC form for the vtable, as support_b does). Chapter 1's kinds call it as well; the Chapter 1 bot logs stay
identical.

Latest report: [ch2_kinds_boundaries.md](ch2_kinds_boundaries.md).

## Worker "ch2play" (2026-10-19): the rest of Chapter 2, the playable flag

Branch `claude/ch2play`. Measured first (`--chapter 2 --frames 90000 --autofire --lives-kept 1 --mute --default-route
--boundary-report`, both model sources): with the earlier workers' batches the default route already reached the
chapter end under `--strict` (the Hangedman camera arrival of [hanged.md](hanged.md) came from the guarded earlier
events; with no guard left he is in view, shot and killed). Left: player cut-scenes 09-14 (unported) and six stubs.

| Batch | Ported (all UNVERIFIED (teaser), read from the disassembly) |
|---|---|
| 1 | player cut-scenes 09-14 (`player_body.cpp`): `player_scene09_stance_shoot` (+ 4 steps), `player_scene10_jump_down_b`, `player_scene11_jump_down_c` (common part `player_scene_jump_down`), `player_scene12_reload_kick`, `player_scene13_knocked_flying_b`, `player_scene14_rattle_door` (+ 2 steps) |
| 2 | former stubs: `prop_bookshelf_collapse` 0x40BB0 + its task `prop_book_fly` 0x40980, `prop_bookshelf_remains` 0x40F50 + `prop_blink_remains` 0x42E70 (props_ch2.cpp); `frog_splash_spawn` 0x308B0 + task `frog_splash` 0x30820, `area_heading_pick` 0x2E550, `obj_velocity_clear` 0x2E870, `point_clip_to_depth` 0x34920 (ch2_kinds.cpp; still virtual for the proof's boundary overrides, PC forms vtable only) |

Note `prop_bookshelf_collapse`: each piece's push uses the heading computed at the end of the piece before it (the
first piece 0), as the original's register reuse does. These routines are shared with Chapter 1 (bookshelves, molb,
frogs, the kit zombie); Chapter 1's default bot run does not reach them (its clear frame is unchanged).

Results (`--strict`, `--frames 40000 --autofire --lives-kept 1 --mute`), arcade and PC models identical:

| Route | Events | 1 player: clear / end | 2 players: end | Report |
|---|---|---|---|---|
| `--default-route` | 0 1 2 3 4 5 7 8 9 15 | 16680 / 17162 | 17995 | only the mouse stand-in, `--lives-kept` |
| `2:0,2:5` | 0 10 12 13 4 5 6 8 9 15 | 20040 / 20522 | 21340 | same |
| `2:0=0,2:1` | 0 1 11 12 13 4 5 7 8 9 15 | 20415 / 20897 | 20968 | same |
| `2:0=0,2:1=0,2:2` | 0 1 2 14 4 5 6 8 9 15 | 17970 / 18452 | 18978 | same |

`--render` (default route, both sources): GL errors 0. Screenshots of event 15: the intro faces him, the fight's life
bar falls, the death blast. No guard, no unported routine, no stub left in any Chapter 2 run: **Chapter 2's playable
flag is on** (`src/game/chapters.h`). Chapter 1: clear 16695 (both sources), then Chapter 2 starts (players kept) and
clears too (end 36961, no guard); `--input-test` 524 / 524.

Left: proofs (captures) of everything marked UNVERIFIED (the ch2 / hanged / ch2play routines); the mouse stand-in for
`player_input_update` is the rewrite's own input, not a gap.

## Proven (worker "ch2proof", 2026-10-19)

Every routine of this file's lists is now proven on A (0 differ) except `prop_door_fly` (not reachable): see
[ch2proof.md](ch2proof.md).
