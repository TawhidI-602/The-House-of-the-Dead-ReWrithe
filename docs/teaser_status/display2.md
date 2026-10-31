# display2: widescreen HUD corners, blood splash colour, --route checks (worker "display2", 2026-10-16)

Branch `claude/display2`, build folder `build-display2`. Test runs with `HOTD_PREF_DIR=<scratch>`. Screenshots with the
headless `--render` path (offscreen framebuffer, no window shown), copies in private\screenshots\display2.

## 1. Widescreen 16:9: the per-player HUD at the window's edges

The 2D picture (arcade tile layer, PC sprites) was one picture of the 4:3 screen, centred. Now, in widescreen
(`aspect=0`), the per-player HUD moves to the window's left / right edges, keeping its distance from the edge of the
original's screen; everything else stays centred over the 4:3 part. Original 4:3: unchanged.

- Runtime::cellSide (4 layers x 64 x 64): written with every sprite placement (pc_cells_put, i.e. sprite_place /
  draw_anim / tinted / clipped): 1 = left, 2 = right when the sprite is one of the per-player HUD's (hud_state.cpp's
  routines: 1P / 2P frames 0xE / 0xF, bullets 0x10, torch base 0x14, torches 0xA8..0xC5, RELOAD 0x36 and the region's
  "shoot outside" hint (table 0xCA700), PRESS START 0x40, 0x107, CONTINUE? 0x113 / 0x114 and its digits (table
  0xCA720), GAME OVER 0xA5, HOLD YOUR FIRE 0x107) **and** lies wholly in one half of the 62-cell screen; else 0 (the
  hit marks 0x28..0x2D stay centred: splashes over the play area; changed in the second commit).
  Centred text (tile_text_draw) and text_layers_clear reset it.
- Arcade: Arcade2D::compose takes a side filter; the HUD composes the centre, left and right cells separately and
  draws the side passes moved by the picture's left edge (Hud::drawArcadePass, also the low-priority pass).
- PC: drawPc2D puts side cells into two more pictures, drawn moved the same way (the PC's own 640-pixel layout keeps
  its 20-pixel wider right margin).
- Checked (1280 x 720, `--bot 7 --bot-players 2`, frame 2600, both sources, both aspects): 1P torches / bullet at the
  bottom-left window corner, 2P's at the bottom-right, P2's RELOAD on the right; 4:3 as before. Continue (frame 260,
  `--lives-out 200`): the centre CONTINUE? 9 stays centred. UNVERIFIED (teaser): the score (debug mode 7 text only),
  per-side CONTINUE not looked at in a picture (P2's PRESS START BUTTON at the right edge and HOLD YOUR FIRE at the
  left one seen in single-player shots).

## 2. The screen blood splash and the blood colour

The red splash over the screen when a player is hit (frames ~1420-1460 of `--bot 7`) is the hit mark (slash / bite /
claw / stab, arcade sprites 0x28..0x2D on layer 2, PC 0x47E..0x483), not a 3D effect.

- **Arcade:** hud_player_notice_show 0x945D0 takes the sprite from the fixed table 0xCA5FC; the 2D bank's palette
  comes from the sprite sheet. Every reader of blood_color (0x51F9C0) in the decompilation: the EEPROM / test menu,
  vblank_words_upload 0x24D70 (polygon entries 1023 and 984 = [0xAB110 + 2 x blood], the 3D gore; 984 is then the
  palette animation's pulse 0x5206AE), the GGSET face lists, prop sub-types 27-29, the corpse models 0x5E4..0x5E9.
  No 2D routine: **the arcade draws the hit marks red with every blood colour.** No change.
- **PC:** hud_player_notice_show 0x50B444 picks from the fixed table 0x5F2DE8 too; its sprites come from TEXTURE.VMC
  (rebuilt from B_<colour>.VMC). Compared B_RED with B_GREEN / custom over every sprite page: only pages 219 / 220
  differ (the GAME OVER screen 0x4E3); the hit marks' pages 103-114 are the same red in every set. **The PC also keeps
  them red.**
- **Fixed:** the PC sprites were always BIGGER.VMC's (red) pages. Now PcSprites::load takes the blood setting and
  the blood texels (B_RED vs the colour's set; custom: B_RED's tinted, as the 3D pages) from the blood set
  (pc_loader.cpp pcBloodPages, Hud::setPcDir(dir, blood)): 1374 texels on pages 219 / 220. UNVERIFIED (teaser): the
  game over screen with non-red blood not looked at in a picture.
- So with the presets the splash staying red is the originals' behaviour. With the custom colour (ours, no original)
  it stays red as well; tinting it would go beyond both originals (not done).

## 3. `--route 1:3=0,1:4=2` hung: choice 2 is not valid at event 4

Chapter 1's branch table (route_branches 0xAAF80 -> 0xAABE0, 16 bytes an event: type, three targets, 0xFF = none):
event 4 is type 1 with targets 5 / 7 / none. route_choice 2 made NEXTEV 0x67A90 read the 0xFF target and jump to
event 255 (the game never got anywhere). No Chapter 1 event has a third target. `--route` is now checked against the
table when the game starts: an invalid choice stops with `--route 1:4=2: invalid choice; chapter 1 event 4 branches
by 0 (event 5), 1 (event 7)`; a choice at an event that does not branch by the route choice is reported and ignored.
Checked: the hang case exits 1 with the message; `1:3=0,1:4=1` runs 3000 frames.

Input test (`--input-test`): wide 494 / 494, 4:3 498 / 498 checks, 0 failed, arcade and PC models.
