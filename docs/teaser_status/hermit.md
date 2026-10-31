# Worker "hermit": the Hermit (Chapter 3 boss) and Dr. Curien (2026-10-19)

Plan: [chapter2_3_plan.md](../chapter2_3_plan.md), worker "hermit". Branch `claude/ch3play`, build folder `build` in the
worktree (SDL3 from the main checkout's `build/_deps/sdl3-src` through `FETCHCONTENT_SOURCE_DIR_SDL3`, no download).
Arcade forms only (`src/port/inst/hermit.inc` is empty). Read from the arcade disassembly 0x85430..0x87F10
(`tools/i960dis.py`) with the typed decompilation as a guide; every coprocessor sequence is the disassembly's. The
decompilation was wrong in three places, fixed from the disassembly: `hermit_act21_lunge` (motion 48, the lunge distance
from the attack table +8, footstep frames from the step table), `hermit_spit_ball_draw` (translate + rotate before the
scale) and `hermit_spit_ball` (+0x33C / +0x340 / +0x344 are floats).

## Ported (src/port/boss_hermit.cpp, 37 routines; 36 proven on A, see "Proven on A")

| Group | Routines |
|---|---|
| Kind 40 | `boss_hermit` 0x85430, `hermit_entrance` 0x85550, `hermit_frame_outer` 0x85640, `hermit_frame` 0x85700 (the action script, as the Chariot's) |
| Actions | 1 `act1_pose`, 2-7 `act_walk`, 8-10 `act_step`, 11-15 `act_attack` (+ `hermit_attack_hit`), 16-19 `act_turn`, 20 `act20_spit`, 21 `act21_lunge`, 22-24 guard, 31 `act31_drop`, 32 `act32_hit_reaction` (+ `_pick`), 33 / 34 `act_wait_combat`; `hermit_action_done` |
| Hits / draw | `hermit_hits_and_draw` 0x86B20, `hermit_part_draw_hook` 0x87560 (mouth place, the face animation and hurt flash), `hermit_death_sink` 0x879E0 |
| Spit ball | `hermit_spit_ball` 0x86CB0, `_shot` 0x87060, `_draw` 0x870C0 |
| Tasks | `hermit_title_task` 0x87770 (camera path 92, sprites 0xDE / 0xE1), `hermit_life_bar_task` 0x878E0, `hermit_drop_dust_spawn` / `_dust`, `hermit_drop_sound_start` / `_sound`, `hermit_hit_sounds` |
| Kind 24 | `kind24_kyurian_curien` 0x87C50, `_wait`, `_rise`, `_float` (flags 128 / 129) |

Registered as the Hangedman: `decl/hermit.h`, `proof/cases/hermit_arcade.inc` (the runtime's dispatch), one line each in
game.h / game.cpp / proof/main.cpp. No new boundaries. Event flags the Hermit sets: 125 out of the hole, 127 fight
started; waits for 124 after the drop.

The face's hurt flash (role 27, odd frames while the block's +8 counts) is written to the geometrizer directly
(`hw_write32`: command 0x0B + the matrix, command 0x01 + the model's table words with attribute 0x802400). Since
2026-10-19 (second round) `Runtime::geo_write` turns such a write into a draw-list model (the ID looked up from the
model table 0x2E73530 by its words), so the face stays drawn; UNVERIFIED (teaser): the flash attribute itself is not
rendered, and hardware writes are not in the captures.

## Headless (2026-10-19)

- `hotd --chapter 3 --event 7 --frames 12000 --autofire --lives-kept 1 --mute --strict`, arcade and `--models pc`:
  **the Hermit is fought and killed, the chapter clears** (chapter clear 3875, end 3939, score 7140); only
  `part_hit_class17` (boundary) on the list. The fight: entrance and title 0-2628, wall phase (hit reactions push him
  down the wall, 160 -> 100 hit points), drop at 3208 (bounces, walks out, flag 124), floor phase from 3405, death sink
  ~3550 (kill score 1500).
- Without fire (`--event 7 --frames 15000 --strict`): no crash; he attacks (40 player hits), spits, the event waits on
  WAIT_ENALV as it should.
- Full Chapter 3 (`--chapter 3 --frames 90000 --autofire --lives-kept 1 --mute --default-route`, arcade and PC): events
  0 1 3 6 7, chapter clear 65905, end 66387, no crash; no Hermit routine on the unported list. **But the Hermit is not
  killed there**: leftovers of unported Chapter 3 objects (kind 153 Taran, a kind 32 zombie stuck in
  `iron_ball_follow`, props) keep `enemies_alive` up, so the guard passes event 7's WAIT_ENALV (0xFB034 / 0xFB080) while
  he is still on the wall, the camera moves on, and the autofire aims at the stuck kind 32 instead of him. This is the
  "ch3own" worker's scope (Chapter 3's own kinds); expect the fight to play through once those are ported.
- Chapter 1 unchanged: `--frames 40000 --autofire --mute --lives-kept 1`, arcade and PC: "chapter 1 ended" at 16695;
  `--input-test` 524 / 524.

## Proven on A (hermit_full, 2026-10-19)

`run_capture.ps1 -Root <main checkout> -Out <main>\private\proof\hermit_full -Name play_us_20261007_2238 -Seconds 1120
-Max 150 -Every 20 -TimeoutMinutes 120` with `-Funcs` / `-Ported` / `-Boundary` from `HOTD_ROOT=<main> py
tools/capture_lists.py --file src/port/boss_hermit.cpp --no-pc` (+ 0x87550 in -Boundary; the command is kept as
`command.ps1` in the folder), then `hotd_proof capture <folder> <main>\private\arcade_exports\regions`: **1,185 captures
(218,557 coprocessor calls, 23,536 boundary calls replayed), 0 differ** after two fixes:

- `hermit_act20_spit`: the random aim point is ((rand & 0xFF) - **127.5**) x 15.56 / 496 (0x42FF0000; it was 127).
- Harness: `point_clip_to_depth` 0x34920 (ported by ch2 since) was still a fixed boundary in `proof/bounds/kit_zombie.inc`;
  it now runs the port unless the capture recorded it as a boundary (as `spin_axis_from_velocity`).

Per routine: frame / hits_and_draw / part hook / life bar 150 each, spit ball 106 + draw 132 + shot 36, hit reaction 56,
death sink 35, lunge 34, spit 24, walk 22, entrance 18, drop 15, guard hold 12, Curien wait / rise / float 13 / 41 / 6,
the rest 1-6. Reached inside them (so proven with them): `hermit_drop_dust_spawn` (the landing in act31's captures).
Branch coverage (`branch_coverage.py`): every routine but act24 ran both ways on most branches; not reached: the
two-player paths of `hermit_attack_hit`, the sound tasks' last frames (only their first call sampled). The capture
data (4.6 GB) is kept compressed (LZX, 4.6:1), with `proof_result.txt`.

## Not done / next

- `hermit_act24_guard_end` (action 24): no sequence of the recording uses it.
- 2-player paths (`hermit_attack_hit`'s player pick, the spit table's 2-player half): a `-TwoPlayers` auto-aim game
  into Chapter 3 would reach them.
- Kind 25 (Curien, Chapter 4) not ported (Chapter 4 scope).
- (done, see Flash) The flash attribute 0x802400 in the renderer.

## Flash (worker flash, 2026-10-19)

What 0x802400 is: word 1 of the geometrizer's object command (0x01) is the texture header address (MAME
`geo_object_data`: point, header, model, count); bit 23 reads the rasteriser's header RAM (`model2_v.cpp`, written by
command 0x04). The boot (0xAE0..0xBD0, routine 0x21B0) fills eight blocks there with flat headers {0, n << 6, 0, n << 6}:
0x800800 / 0xC00 / 0x1000 / 0x1400 / 0x2000 / 0x2400 -> palette entries 0 / 1 / 2 / 3 / 6 / 7, 0x8F0400 -> 959,
0x8FEC00 -> 1017. Header 0x802400 = untextured, opaque, polygon palette entry 7 (0xFFFF white in every dump): the flashed
model is drawn flat white, each polygon lit by its own light parameter (MAME `draw_scanline_solid`), not a whitened
texture. Commits cbd6ec5 (renderer: DrawItem::flat, `Runtime::geo_draw_header` / `header_ram_flat`, geo_write for the
direct writes), 1ec1172 (the Magician's hook), c8b09f7 (the Magician's fades).
PC models (THOTD.EXE disassembled): the Hangedman's hook 0x51D84E draws nothing while +0x2FC is odd, the Hermit's face hook
0x4EF2B8 neither model on odd frames, the Magician's 0x51DF7E a white copy (PC models 0x1D3C..0x1D4A for 15 of his
parts, else model 0): the runtime does the same with the PC models.
Shots (off-screen `--render --shot-at`, main checkout's `private\flash\`; a shot shows the frame before's draw list):
`c3_arcade_sheet.png` / `hermit_face_compare.png` (`--chapter 3 --default-route --strict --autofire`, flash draws at
25201, 25307 ...: f25202 the face flat white-grey behind the shot's burst, f25203 normal), PC `c3_pc\`. MAME:
`mame_hermit_flash_0537_0523.png` (fullgame_us snap 0537: the face lit white). `--event 7` starts show a black 2D cover
over much of the arcade picture (also main's build 792bfff, not this change): use a full chapter run for shots.
