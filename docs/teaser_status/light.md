# light (2026-10-16, branch `claude/light`, build folder `build-light`)

Last window of the overnight run. UNVERIFIED (teaser) unless said otherwise.

(Commits went 3, 2, 1: the long Chapter 1 runs for 1 ran in the background meanwhile.)

1. **Results researchers (0x65E): no difference left (no change).** kind176_chapter_clear 0x9B880 / _results /
   _backdrop send no light (only palette_restore_vblank when the grey look is on); 0x65E takes the frame's last light
   record. Ours (main run, HOTD_LIST_DRAWN_AT=16737 = clear + 120): world light (0,-1,0) (world_draw_task's record),
   texture area 4 set 2, parameters 11 x 808 (diffuse 128 / ambient 192) and 2 x 111 (112 / 192), coat colour palette
   entry 1003 = (28,30,31). MAME fullgame_us t03440 (snap 0172): entry 1003 the same, the colour translation RAM equal
   row for row to the set's dump (fullgame_bufram t03220), unchanged from the Chariot fight through t03460; light
   records 0x520A40 (-0.31,-0.85,-0.42) / world 0x5430C0 (0,-0.96,0.27) (with ambient 192 any of these gives luma
   192..255). Pixels, window's lower half (low-saturation pixels, ours box-scaled to 496 x 384): MAME p10 / p50 / p90
   (121,140,149) / (150,172,184) / (196,225,240), ours (118,136,144) / (151,172,184) / (196,226,240). Side by side the
   two look alike (ours smoother: bilinear vs the snap's point look). The "brighter / whiter" note in sparks.md
   predates the palette animation / colour fixes merged since, or came from the sheet's scaling; not reproduced.
2. **Route-map walker light: the same as MAME (no change).** Correction to gameover.md item 4: the light the walker
   gets is not the chapter's stale one: game_over_camera_task 0x5D4F0 = camera_matrices_update every frame, which
   rebuilds camera_light_dirs 0x520A40 and sends dirs[0] (0x8000A0); body_draw's parts use those records. MAME
   life_scan t00910-t00940 (sub-mode 3): dirs (0.707,-0.707,0) / (0,1,0) / (0,1,0) / (0,0.707,0.707); ours (`die` run,
   HOTD_LIST_DRAWN_AT=2996): every walker part light (0.71,-0.71,0), diffuse / ambient the ROM table 0x7A0 (as the
   arcade). What is kept from the chapter is the colour state: MAME's colour translation and luma tables are byte-equal
   from t00700 (play) through t00960 (GAME OVER), only the palette RAM changes (palette animation, map sprites); ours
   keeps texture area 1 (the bot dies in event 3, life_scan in event 0), so the remaining tint difference is the place
   of death. Side by side (snap 0092 vs c17, zoomed): coat dark teal, grey trousers, brown arms in both.
3. **PC snow hue: fixed (the stage palette).** THOTD.EXE's stage start (0x418B07, string "stage%d.pal", stage clamped
   to 1..4) hands STAGEn.PAL to the render driver's palette call (vtable + 0x34, 0x419032): the textures go through
   STAGEn.TBL (65536 bytes: palette index by 16-bit texel, the opaque bit included; index 0 = every transparent texel
   and nothing else) into the palette's 256 colours. The mountain band (0x608, page 589) is the same in BIGGER.VMC /
   TEXTURE.VMC (only key texels differ); its snow texel (29,25,25) maps to index 241 = (220,209,189). PcSource::loadStage
   now converts the pages that way (pc_loader.cpp readVmcPages; `HOTD_PC_NO_PAL=1` = the old direct 5-bit colours).
   Frame 1060 `--models pc`, snow band p75 / p90: before (222,198,198) / (239,206,206), now (220,209,189) / (220,209,189);
   the real PC capture fw2 t764 (214,203,187) / (220,209,189). The clouds turn greenish-cream as in the capture.
   No fog: the only D3D render states THOTD sets (0x583820) are z, blend 5/6, TEXTUREMAPBLEND modulate, colour key,
   Gouraud, no cull, dither, no specular, filters. Not done: title.pal / menu.pal for the other screens.

## light-2 (coordinator's checks)

- **All real-PC frames, with / without the palette:** checkpoints event 0 at tick + 296 for the 18 fw2 captures (and
  the two older real_pc_t454 / t614): mean absolute difference over a 32 x 24 box-scaled grid, palette vs
  `HOTD_PC_NO_PAL=1`: 50.1 vs 51.4 overall, the palette closer in all 20 frames (aligned frames t661-t952: 34.8 / 26.8 /
  39.1 / 35.5 / 45.1 / 46.4 / 21.4 vs 35.4 / 28.1 / 39.2 / 36.3 / 45.9 / 47.2 / 21.6). The early ticks are not the same
  moment (+296 holds from about t614; the real t301 still shows the title card), so their absolute values mean little.
- **Stage switch:** pc_chapter_files_load 0x418AA0 (chapter 0-3 -> n = chapter + 1) loads WORLDn.RBH and stagen.pal
  together, once per chapter, not per event; PcSource::loadStage(chapter) does the same (STAGE1-4 all differ).
- **Brown (the PC's grey look): done.** The palette loader 0x566B30 reads <name>.pal (entry 0 forced black) and
  <name>.tbl (else builds a nearest-colour table); 0x419028 switches to brown.pal while 0xDA0CBA is set and back to the
  stage palette when it clears. PcSource also converts the pages through BROWN.TBL / BROWN.PAL
  (TextureSet::layersBrown); the renderer draws those while RenderOptions::grey (Runtime::greyPalette) instead of the
  shader's sepia mapping, for PC sets. Frames 200 / 230: the bright pixels' r : g : b 1 : 0.92 : 0.66, the real PC's
  brown frame (fw2 t404) 1 : 0.92 : 0.67; the real one is darker (it is fading in there; not compared further).
  UNVERIFIED (teaser): that 0xDA0CBA follows the same moments as the arcade's palette_grey_on.
