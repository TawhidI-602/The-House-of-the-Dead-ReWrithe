# Worker "hermit": the Magician (Chapter 4 boss) (2026-10-19)

Branch `claude/ch3play`. Kind 20 `boss_magician` 0x70AB0 and everything it reaches that was not ported: 48 routines
from `chapter_gap.py --chapter 4 --routines` plus two the index does not have (0x74150 the barrage shot's frame,
0x74C70 the barrage's homing helper). Read from the arcade disassembly 0x70AB0..0x75610 and the helpers' ranges
(`tools/i960dis.py`); the typed decompilation was only a guide (its coprocessor sequences drop the translate / rotate
steps of every effect draw, and several record fields are misnamed). Arcade forms only, all UNVERIFIED (teaser).

## Ported (src/port/boss_magician.cpp, decl/magician.h, cases/magician_arcade.inc)

| Group | Routines |
|---|---|
| Set-ups | `boss_magician` (table 0xC69B0 by +0x70), `magician_intro` (set-up 0: the scene with Dr. Curien, flags 112 / 208-215 / 224-226), `magician_fight_init` (set-up 1) |
| Fight | `magician_frame`, `_hit` (weak parts 4 / 6 / 11 / 13, sparks elsewhere), `_hit_spark`, `_hit_reaction`, `_vuln_clear`, `_hit_check_a/_b/_c` (the pattern table 0xC69C0's hit-point shares) |
| States | 0 `magician_idle`, 1 `_dying` (score 2500, flags 219-223, the flash, the palette fade out / in), 2 `_descend` (title sprites 0x1C9 / 0xE2, the boss life bar), 3 `_next_pattern`, 4 `_fireballs`, 5 `_attack` (swoops + the lunge), 6 `_barrage` |
| Projectiles | `magician_fireball` (+ `_aim`, `_hit`, `_spawn`, `_frame`), `magician_intro_object` (+ spawn), `fireball_trail` (+ copy), `magician_barrage_shot` + `_frame` (0x74150), `magician_barrage_spawn` + `_projectile` (0x74C70) |
| Effects / helpers | `magician_afterimage` (+ spawn), `magician_tracker` (+ spawn), `magician_part_effect` (+ task), `magician_death_flash` (+ spawn) |
| Shared | `palette_fade_in_task` / `_start`, `palette_fade_out_task` / `_start`, `palette_blend_white`, `any_player_in_play`, `body_find_part` (+ rec), `body_hit_part_find` (+ rec) |

Not ported (not reached by the Magician's code): `colour_row_queue` 0x1A930 (in the gap list through another path).
The brightness register 0x1800124 and the colour register 0x180248E are written with `hw_write32` (hardware, not
modelled by the runtime).

## Headless (2026-10-19)

- `hotd --chapter 4 --event 4 --frames 30000 --autofire --lives-kept 1 --mute --strict`, arcade and `--models pc`: the
  descent, the pattern steps 4 (fireballs) -> 5 (swoops) -> 4 -> 5 -> 6 (barrage) as his hit points fall (600 -> 0, 10
  a hit on the weak parts), the death (score 2500) and **"chapter 4 ended" at frame 23030**; no crash. Unported there
  (Chapter 4's own, worker ch2proof): kind 25 (Curien), props 0x3F460, `event_model_fade`, `player_body_at_scene_pos`.
- Full `--chapter 4 --frames 120000 --strict --default-route --autofire --lives-kept 1`: events 0 1 2 4, **chapter end
  30694**, no crash (the unported list: kinds 25 / 154 / 191, props 104 / 108 / 120 / 0x3F460, scene 17).
- Checks: input test 524 / 524; Chapter 1 end 16695 both sources; Chapter 2 `--strict --default-route` end 17270 (the
  same as main's own build at 591e5ee; the 17162 of the earlier notes is older); Chapter 3 `--strict --default-route`
  end 27279 both sources; Chapter 3 `--event 7 --strict` end 3939.

## Proven on A (worker "ch4proof", ch4p_full, 2026-10-19)

`run_capture.ps1 -Root <main checkout> -Out <main>\private\proof\ch4p_full -Name play_us_20261007_2238 -Seconds 1620
-Max 150 -Every 20` with the lists of `HOTD_ROOT=<main> py tools/capture_lists.py --file src/port/boss_magician.cpp
--funcs <Chapter 4's kinds / props, the iron chain> --no-pc` (kept as `command.ps1` in the folder; MAME needs ~2 h for
it, so give it -TimeoutMinutes 180), then `hotd_proof capture <folder> <main>\private\arcade_exports\regions`:
**2,671 captures (162,781 coprocessor calls, 21,657 boundary calls replayed); every Magician routine 0 differ** but
0x74150 / 0x74C70 (below). No fix was needed in the port.

Per routine (captures): `boss_magician` 1, `_intro` 144, `_fight_init` 1, `_frame` 150, `_hit` 150, `_hit_spark` 6,
`_hit_reaction` 35, `_vuln_clear` 1, `_hit_check_a/_b/_c` 2 / 1 / 1, `_dying` 106, `_descend` 23, `_next_pattern` 28,
`_fireballs` 36, `_attack` 60, `_barrage` 72, `_fireball` 97, `_fireball_aim` 1, `_fireball_spawn` 1, `_intro_object`
8 (+ spawn 1), `fireball_trail` 150 (+ copy 15), `_afterimage_spawn` 19, `_barrage_shot` 2, `_fireball_frame` 150,
`_barrage_spawn` 1, `_tracker_spawn` 1, `_part_effect_task` 72, `_part_effect` 4, `_death_flash` 9 (+ spawn 1),
`_afterimage` 150, `_tracker` 94; shared: `palette_fade_in_task` 19, `_start` 1, `palette_fade_out_task` 9, `_start` 1,
`palette_blend_white` 28, `any_player_in_play` 2, `body_find_part_rec` 74, `body_find_part` 11,
`body_hit_part_find_rec` 72, `body_hit_part_find` 9. Marked "proven (ch4p_full)" in decl/magician.h.

- 0x74150 `magician_barrage_shot_frame` (7 differ) and 0x74C70 `magician_barrage_projectile` (8 differ): capture
  artifacts. Neither routine is in the index, so the capture had no size for them (mame_capture.lua: "0 ret"): no
  exit was found at their `ret`, and each capture ran on into the following tasks until one of them called task_exit
  at the same frame depth (the "original" then called 0x2320 / 0x14120 of those tasks). The disassembly
  0x74150..0x749EC / 0x74C70..0x74EBC, read again against the port, matches it. Re-captured with sizes
  (`74150:8a0`, `74c70:250` in -Funcs and -Ported; `capture_lists.py --funcs "74150:8a0,74c70:250,..."` carries the
  sizes into both lists): **ch4p_size** (the same recording, `-Every 3 -Max 150`): `magician_barrage_shot_frame` 150,
  `magician_barrage_projectile` 150 captures, **0 differ** (the folder also re-took Chapter 4's 0x3B020 / 0x3F480 /
  0x4E220, ch4.md). Both are now marked proven.
- Not reached by the recording: `magician_idle` 0x70AD0 (state 0) and `magician_fireball_hit` 0x73820 (a fireball
  shot down); they stay UNVERIFIED.
  `magician_fireball_hit`: covered since (arcfix's two-player differential run, docs/proof.md). **`magician_idle` is
  unreachable** (arcfix2, 2026-10-27, from the disassembly): 0x70AD0 is referenced only by the state table 0xC6990
  (entry 0), read only by `magician_frame` (0x71050). The state +0x5C is stored by `magician_init` (0x70FF0: 2),
  `magician_hit` (0x7123C: 1), the three hit checks (0x7147C / 0x71510 / 0x71618: 3), `magician_descend` (0x71F0C: 3),
  `magician_fireballs` (0x725EC: 5) and `magician_next_pattern` (0x72184: the pattern table 0xC69C0's state at step
  +0x330). The table's states are 4, 5, 4, 5, 6, then -1 (the end: the step goes back, the state stays 3); the step
  starts at -1 (`_descend`) and only `_next_pattern` raises it before its read, so the word before the table (0xC69B8
  = 0, state 0) is never read. The Magician's block is never in state 0.
- Branch coverage (`branch_coverage.py`): most branches ran both ways. Never reached: 1 branch each in `_intro` and
  `_hit`; 3 in `_attack`; 4 each in `_dying`, `_fireballs` and `_intro_object`; 5 in `_fireball`; 7 in
  `_fireball_aim` (its two-player paths).

## Next

- (done 2026-10-19, worker flash) Rendering: the fade / flash attributes, the palette brightness register.

## Flash (worker flash, 2026-10-19)

- Part hook (1ec1172): type 76's case of 0x9CE60 (0x9E0F8..0x9E814) was not in the runtime: the vein cycles (classes 3 / 4
  / 11 / 12: blink_phase / 2 % 25 + 0x1C2A / 0x1C75 / 0x1C47 / 0x1C07), the flash (shot this frame, +0x2F4 or the part's
  +0xAC odd: header 0x802400, flat white; see hermit.md, Flash) and in chapter 4 with event flag 96 the vein parts with
  header 0x8F0400 (flat palette entry 959, palette_animation's pulsing 0x5206E0: the dark-red shapes on his legs in MAME).
  PC models: the PC's white copies 0x1D3C..0x1D4A (THOTD.EXE 0x51DF7E), the veins left out while flashing.
- Fades (c8b09f7): 0x1800124 is not a brightness register but palette RAM, 2D entry 0x92 (the low half of the word): the
  cut-scene bars' colour. palette_fade_out / _in blend the colour curves toward white (palette_blend_white) and write
  (31 - level) grey there; MAME fullgame_us t14340..t14460 (snaps 0716-0722): picture and bars to white, back to the blue
  sky, bars back to black, the 2D text not blended. Ours: `Runtime::paletteWhite` (the level while 0x51F704 is set) drawn
  as the existing white fade over the 3D, the bars in entry 0x92's colour (`backdrop555`). PC models: THOTD.EXE 0x4D1EE4
  state 0x11 -> 0x41B268, the screen to 2x over 1500 ms, kept until the ending's fade from white (pcFadeStart).
- Shots (main checkout `private\flash\`): `c4_arcade_sheet.png` / `c4_pc_sheet.png` (`--chapter 4 --event 4 --strict
  --autofire`): f12500 the vein glow, f19355 the flash (white), f19356 normal, f20400..21360 every 60 the death (to white,
  back to the sky; the bars grey -> white -> grey). MAME: `mame_dumps\fullgame_us\snap\hotd\0716..0722`. The arcade
  `--event 4` start keeps the black 2D cover over parts of the picture (as main's build; not this change).
- Checks (c8b09f7): input test 524 / 524; Chapter 1 ends 16695 (arcade and `--models pc`); Chapter 2 `--strict
  --default-route` 17270; Chapter 3 27279; Chapter 4 37983, the ending over at 44867.

## Next (after the proofs, worker ch4proof)

- A two-player capture (`-TwoPlayers`) for the two-player paths, and `magician_fireball_hit` (fireballs shot down).

## Slow-fire bots never killed him (worker "hangcmp", 2026-10-19)

`--chapter 4 --event 4 --strict --autofire --autofire-every 45`: 107 shots at him in 15,000 frames, his hit points
600 -> 570 (3 damaging hits), state 4 (fireballs) for ever (the event's WAIT_ENNUM); at every 15 frames 286 shots
took him to 110. The fight is not at fault: he takes damage only on parts 4 / 6 / 11 / 13 (magician_hit, proven), the
bots aimed at his centre (a spark), and a fixed rhythm against his animation found a weak part almost never. MAME's
recording (fullgame_us dumps, his object by routine 0x71010): descent at fc 76173, 600 -> 410 in ~600 frames, dead
at fc 81005 (~4,800 frames, 60 damaging hits; the replay keeps 6 rounds, so its shot count is not known).
Fix (src/game/bot.cpp `aimPoint`, used by `--bot` and `--autofire`): at the Magician the bots aim at a weak part's hit
sphere (part +0x5C, camera space, as the shot test reads it), another part each shot. Kill frame at `--event 4`:
every 15: 12212 (was 23030 per the notes above); 30: 18018; 45: 21332 (was never); 60: hit points 20 at 25,000.
`--bot` seeds 1-16, one player, `--chapter 4 --event 4 --bot-gameovers 40 --frames 60000`: kills 1 -> 6.
The checks' Chapter 4 numbers move with it: `--chapter 4 --strict --default-route --autofire --lives-kept 1` ends at
**28083 and the ending at 34967** (were 37983 / 44867; the same 6884-frame ending), `--route 4:1` 35153 / 42675 (were
43936 / 51458); Chapters 1-3 unchanged.
