# Proofs worker 5 (2026-10-16, 9:14 AM - 11:35 AM)

Branch `claude/proofs-5`. The items proofs-4 did not reach, moved from the runtime into the port and captured against
MAME: results and command lines in [proof.md](../proof.md) ("Game over (proofs-5)", "Sky, palette, sprite banks, HUD
(proofs-5)"). All 0 differ except 5 sprite-bank captures that differ only where a vblank interrupt wrote.

- **Game over** (src/port/game_over.cpp; the runtime's mode5_game_over dispatches to it): mode 5's sub-modes
  `game_over_start` 0x97200, `game_over_scene` 0x97340, `game_over_map_start` 0x5CB50 (+ `map_scroll_set`),
  `game_over_map` 0x5CBF0, `game_over_text` 0x973A0, `game_over_wait` 0x97430; `player_st6_game_over` 0x5470,
  `player_game_over_wait` 0x5540, `game_over_camera` 0x97490, `game_over_walker_task` 0x5CCD0,
  `game_over_route_task` 0x5CF50. Lost MAME games with the new `run_capture.ps1 -Mortal` (lives not kept) and
  `-Shoot 0` (never fires: out in event 3) or the auto-aim (out in event 6: a longer route). 1,054 captures.
  Fixes while moving (disassembly): player_game_over_wait counts to exactly 0 and books the play time;
  game_over_wait clears the debug mode and books the game; the route task keeps its state in the task, without the
  runtime's safety stops; scene / map reset the matrix stack (coprocessor 0x08) first.
- **sky_task** 0x33F50 (src/port/sky.cpp; the runtime marks its two models as backgrounds): 100 captures.
- **Palette** (src/port/palette.cpp): `palette_bank2_upload` 0x1A300, `palette_restore_vblank` 0x1A710,
  `palette_anim_copy` 0x24E90, checked in palette RAM (new XOUT records): 34 captures. The runtime keeps its own
  renderer form of 0x24E90 (Runtime::palette_anim_copy, the polygon palette for the view; it hides the port's).
- **Sprite banks** (src/port/sprite_banks.cpp): 0x17A10 .. 0x17D50 (8 routines), character RAM checked: 158
  captures, 5 differ by a vblank interrupt inside the tile copy (bank records, slot map, palette shadow and character
  RAM match).
- **HUD state** (src/port/hud_state.cpp): `hud_popup_update`, `hud_player_notice_show`, `hud_player_start_draw`,
  `hud_lives_draw`, `hud_ammo_draw`: 453 captures (two players, lives lost).
- Tools: `run_capture.ps1 -Mortal`, `-Shoot <mode>`; mame_capture.lua writes the `-XRam` ranges at exit too ("XOUT"),
  hotd_proof compares them; hotd_proof's call_routine runs a ported target (one with a proof case) unless the capture
  recorded it as a boundary; `HOTD_PROOF_RANGE=1` prints each differing capture's byte ranges and fp.
- Headless checks after each batch (`--frames 8000 --autofire`; `--frames 20000 --bot 3 --bot-gameovers 1 / 2`, also
  `--bot-players 2`; `--render` on a game over): output identical to the proofs-4 build.
- **HUD (2), life bar, group starts** (hud_state.cpp, sprite_banks.cpp, game_over.cpp): `hud_start_prompt` 0x209A0
  (its prompt routines, table 0xAA450, are boundaries; the runtime draws them as free play's), `hud_sprites_load`,
  `hud_player_full_draw`, `hud_sprite107_blink / _draw`, the continue / game-over sprites 0x95010 .. 0x95110, the boss
  life bar 0x6BA90 / 0x6BAF0 / 0x6BC40 (the Chariot in the full-game recording, the blink-out too),
  `sprite_banks_init` 0x17930, the game-over groups' start entries 0x97740 / 0x5CCA0 / 0x5D4F0 / 0x5D4D0 / 0x5CE70.
- Left (UNVERIFIED): `continue_screen_draw` 0x26260 (no capture reached it), the debug-mode score lines (text_printf's
  format address), the sprite drawing routines themselves (0x17D80 .. 0x18110, tile / character RAM: the HUD's
  picture), tile_text_draw / the subtitles, the window masks.
