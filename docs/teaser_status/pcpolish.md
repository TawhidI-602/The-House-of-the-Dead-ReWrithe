# pcpolish: `hotd --models pc` against the real PC game, Chapter 1 (2026-10-16)

Branch `claude/pcpolish`, build folder `build-pcpolish`. Not merged. Stopped at the coordinator's usage limit (69 %);
the game-over and HUD comparison is not done yet (see "Next").

## Method
- Real PC: one harness run (`run_pc_shots.ps1 -Script game_aim.txt -Ticks 18000 -Every 50`, InfLives, window at
  -20000), 359 captures in `private\screenshots\pc_look\pcpolish\full` (compressed). The PC played the default route:
  event starts (tick) 0: 211, 1: 1484, 3: 1942, 4: 3811, 5: 5306, 6: 6487, 19: 10551, Chapter 2 event 0: 15097
  (read from state.rec, PC event byte 0xDA0CF3).
- Ours: `--models pc --autofire --lives-kept 1 --default-route --render`, a shot every 50 frames
  (`private\checkpoints\pcpolish_base\every50`) plus the 18 checkpoints (`pcpolish_base\pc`). Event starts (frame)
  0, 1833, 2546, 4587, 6258, 7774, 12291, clear 16616.
- Matching: each capture against our shots within +-700 frames of the same event offset, mean absolute difference on a
  48 x 32 grid (scratch scripts, not committed); good matches (mad < 6) at the opening, the courtyard, events 4 / 5 / 6,
  the Chariot intro and the results.
- **Capture framing:** the old captures (this run, fw2, lifeup) are the top-left 2048 x 1152 of a 2560 x 1440 client
  (DPI virtualization): game pixels x 0..576, y 0..384 of 640 x 480, i.e. no HUD. Fixed in run_pc_shots.ps1
  (SetProcessDPIAware: 2560 x 1440, whole frame); not used for a full run yet.

## Fixed (one commit each)
1. **2D colours (results screen, all PC sprites):** the sprites went straight from 5-bit colour; the PC puts them
   through the stage palette like the textures (STAGE1.TBL / PAL). Results plate p50 (147,155,180) real, (146,153,180)
   now, teal before. `sprites_pc.cpp`, `HOTD_PC_NO_PAL=1` = old.
2. **Alpha-0x7F polygons drawn opaque:** the Chariot room's ceiling lights (0x111D, 34 quads, head bit 0x80) were
   (123,123,127) vs the real (242,242,244) over the black void. The PC code sets diffuse alpha 0x7F with blending on,
   but the real output is opaque. `pc_loader.cpp`, `HOTD_PC_HALF_ALPHA=1` = old.
3. **No rain / drip splashes:** THOTD.EXE's splash_9f1_spawn 0x486FB0 is `ret 0xC`; ours drew splash crowns
   (0x9F1-0xA05) on the ground. `view.cpp` skips them in PC mode.
4. **Shot flash scale 0.08 (PC 0x50BE08; arcade 0.1)** for 0x2D-0x36, and the hostage blood mark 0x3F5 / 0x3F6 at
   camera depth 1.1 (PC 0x447F10 / 0x4480A0; arcade 1.0). `view.cpp`.

## Same as the real PC (checked)
Sky, mountains, courtyard, mansion, corridors, event 5 / 6 walls, Chariot room walls and Sophie: colours and lighting
within a few levels (mad 1.4-5 on matched frames); point sampling; subtitles; LIFE UP / MISS SHOT colours; Chariot
life bar colours.

## Remaining, ranked
| # | where | difference | lead |
|---|---|---|---|
| 1 | game over, CONTINUE, HUD (bullets, 1P, lives) | not compared: no full-frame PC capture yet | run_pc_shots now captures the whole frame; a menu_noaim.txt run dies into CONTINUE / GAME OVER |
| 2 | hits (all events) | spark look differs: PC white puffs with orange rims + long thin streaks, a blue crescent; ours a round orange-red ball with petals at the same kind of moment (c09 / t06802, c11 / t12050). Not the same frame of the animation (autoaim fires several shots); flash / blood-mark models are the PC's own | trace the IDs drawn on one hit in the harness (HOTD_PC capture of geo_draw_model) vs `HOTD_LIST_DRAWN_AT` |
| 3 | shot debris 0x660 | PC: 30 frames, 40 units a frame, drawn from frame 6 (0x50BD70 / 0x50BF10); ours (arcade logic) 60 frames, 20 a frame, from frame 0: debris visible right in front of the camera | logic is the arcade's (polish2 area); a PC-mode view filter by depth < 240 would approximate it |
| 4 | blood colour in comparisons | harness game = yellow (THOTD.CFG), ours red by default; `--blood` has 0-3 only, yellow (4) not selectable from the command line | add `--blood 4` (B_YELLO.VMC) for comparisons |
| 5 | Chariot end | PC shows the life bar (empty) at t13300 where ours (f15450) shows none; Chariot death timing differs (mad 50-110 for ticks 13250-13560) | hud / boss timing, not PC look |
| 6 | opening | PC plays the "1998.12.18" prologue and fades in; ours starts on the card (polish.md 10) | known |
| 7 | other 0x80 users (sparks 0x39-0x4A, effects 0x6-0x23, world 0x928 / 0xCF5) | now opaque like the ceiling: UNVERIFIED individually | harness frames of a burning / spark close-up |

Gaps logged (PC draws nothing either): as render.md "PC gaps" and pcsweep.md; no new gap found. No arcade fallback was
added anywhere.

## Round 3 (pcpolish3, 2026-10-17; branch `claude/pcpolish3`, build folder `build-pcpolish3`)

Method: real PC `run_pc_shots.ps1 -Script game_aim.txt -Ticks 16700 -Every 50` (whole 2560 x 1440 frames, the 4:3 game
in the middle; `private\screenshots\pc_look\pcpolish3\full`; PC event starts as pcpolish's, the run is deterministic)
against `hotd --models pc --size 1280x720 --aspect wide / 4:3` checkpoints (tools\checkpoints.txt's main run,
`private\checkpoints\pcpolish3_base`), dense opening / Chariot intro shots (`pcpolish3_seq`, `--blood 4`), each
checkpoint matched to the nearest-looking PC frame within +-900 ticks (64 x 48 thumbnails). The unskipped opening:
pcpolish2's `gameover` run (menu_noaim). Same as the PC (checked): the Chariot intro cut-scene frame for frame (room,
Sophie, the black floor gap, the weak-point card), courtyard, mansion front, event 5 / 6 rooms, hit sparks (the petal
puffs), the results plate, 16:9 shows more at the sides only.

### Fixed (one commit each)
1. **Captures were white** (harness): `Process.MainWindowHandle` was the game's own window, not dgVoodoo's presenting
   one; run_pc_shots now captures every visible window of the process and keeps the one with varied pixels.
2. **PC screen fade** (chapter start, kind 104): THOTD.EXE's main view context (0xD5A5C0) has a brightness (+0x64:
   0x10000 normal, 0 black, 0x20000 white), a target (+0x68) and milliseconds left (+0x5C), stepped every drawn frame
   in 0x57CCE0 (a straight line) and applied as a colour scale. evt_chapter_start (0x4C0CF0) calls 0x41B2A8: from black
   over 1000 ms (0x41B248, the ending: from white over 3000); kind 104 (0x448674) calls 0x41B228: from white over 1000.
   The captures: the chapter card and opening rise from black in a straight line (max level 13 / 25 / 37 / 42 / 64 ..
   112 at t253 .. t703: the harness clock gives the renderer a millisecond a tick, so 1000 ticks there). Ours drew the
   card at full brightness. Now Runtime::PcFade + Hud::pcFadeDraw (PC models only; the arcade's kind 104 white
   fade is not drawn with PC models).
3. **2D in the grey look**: the PC's grey look (0x419028: brown.pal while 0xDA0CBA) goes to its one palette, so the
   sprites turn brown with the 3D (capture t14350, Sophie's death: HOLD YOUR FIRE! brown). PcSprites keeps the pages
   through BROWN.TBL / PAL too; Hud::drawPc2D uses them while Runtime::greyPalette.on.

### Remaining, ranked
| # | where | difference | lead |
|---|---|---|---|
| 1 | game start | the PC plays the "1998.12.18" prologue (white fade, kind 104) before the card; ours starts on the card | polish.md 10 (flow, not look) |
| 2 | other PC screen fades | ported: 0x41B2A8 / 0x41B248 (evt_chapter_start 0x4C0D2B / 0x4C0D32; game_chapter_end_wait 0x4587ED / 0x4587F5 goes through chapter_start) and 0x41B228 (kind 104). Left: 0x41B268 (to white over 1500 ms) from the Magician's death 0x4D2420 (Chapter 4, not in the teaser). 0x41B288 / 0x41B2C8 / 0x41B2E4 have no caller in THOTD.EXE (E8 / E9 / absolute scan): unused | - |
| 3 | blood bursts | ours draw big red clouds over a killed zombie (c23 / c25 / c28); the matched PC frames are other moments (yellow blood) | a PC capture at the same kill (CapFuncs on the blood spawn) |
| 4 | hit sparks at the sight | the PC often shows a dense star of overlapping puffs (its auto-aim fires every few ticks); ours sparser | compare at equal fire rates before calling it a difference |
| 5 | Chariot end | life bar / death timing (pcpolish #5) | logic, not look |
| 6 | the sight in the grey look | the PC sight goes through the palette too (brown); ours keeps its colours (built once) | Hud::buildSight per palette |
| 7 | results "1P" | the PC shows the results without "1P" in most frames (blink?); ours shows it at clear + 120 | hud blink phase on the results screen |
