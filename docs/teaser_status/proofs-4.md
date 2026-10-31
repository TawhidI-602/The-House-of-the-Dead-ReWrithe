# Proofs worker 4 (overnight 2026-10-16)

Branch `claude/proofs-4`. Started at 76 % of the 5-hour window (stop at 86 %), so only the cheapest items.

- Moved into the port (`src/port/palette.cpp`, declarations `src/port/decl/support_b.h`, proof cases
  `support_b_arcade.inc`) and capture-proven, 0 differ: `palette_animation` 0x25310 (520 captures, title screen,
  Chapter 1, Chapter 3), `palette_grey_build` 0x1A400 (2), `vblank_callback_add` 0x2780 (8). Results and command lines:
  [proof.md](../proof.md) ("Palette (proofs-4)"). The runtime calls the port's (Runtime::palette_grey_build adds only
  grey_palette_sync for the renderer). Headless 6000-frame run: identical to main's build.
- Also moved and proven, 0 differ: `projectile_hit_player` 0x6640 (33 captures, one and two players; src/port/player_support.cpp), `shadow_light_reset` 0x6C2A0 (1) / `shadow_setup` 0x6C760 (100; src/port/shadow.cpp). `researcher_rescue_bonus` 0x70960 (2, the full-game recording). Moved but not reached (UNVERIFIED): `researcher_reached_camera` 0x708E0 (the gift scene's researcher states 7 / 14: no capture run reached them).
  Headless 17,000-frame autofire run: identical output to main's build.
- New capture option `run_capture.ps1 -XRam "addr:len,..."`: memory outside work RAM at each capture's entry (palette
  bank 2 for palette_grey_build).
- Not reached (usage): palette_anim_copy 0x24E90, palette_bank2_upload / palette_restore_vblank, sky_task, the sprites.cpp game logic, the
  game-over routines (flow.cpp).
