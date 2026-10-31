# pcpolish2: `hotd --models pc` against the real PC game, continued (2026-10-16 evening)

Branch `claude/pcpolish2`, build folder `build-pcpolish2`. Continues pcpolish.md's "Remaining, ranked".

## Captures (private\screenshots\pc_look\pcpolish2, LZX-compressed)
- `gameover`: real PC, `run_pc_shots.ps1 -Script menu_noaim.txt -Mortal -Every 50` (new `-Mortal`: no -InfLives),
  2560 x 1440 whole frame (the 4:3 game is the middle 1920 x 1440). The player dies in event 1 at about tick 2150:
  CONTINUE 9..0 (t02158-t03014), the fall (t03064-t03303), the route map (t03351-t03465), GAME OVER (t03512-).
  The game then stops ticking (the harness hook waits at front 0 / screen 8): stopped by hand.
- `ours_go`, `ours_end`, `ours_hit3`, `ours_yellow`: ours (`--lives-out 1500`, `--event 19 --autofire`, `--blood 4`).

## Fixed (one commit each)
1. **Yellow blood** (`--blood 4`, launcher "YELLOW"): the PC's B_YELLO.VMC (the fifth colour of THOTD.CFG; the
   harness's install). Custom moves to 5; a launcher.ini without `bloodv=2` reads its old 4 as custom. The arcade
   has no yellow (its table 0xAB110 has four colours, the setting is masked & 3): arcade models draw red. With PC
   models the logic gets "not red" (green: the 0x5E7 / 0x5E9 / 0x5E5 choices and the props' +1 as the PC's byte 4;
   GGSET recolours only the arcade palette).
2. **Game-over route map (PC)**: was black with the trail. The map sprites 0x243.. map to the PC's own set + 0x3F7
   (0x63A..: the PC's route task 0x4B16D4 loads banks 0x63A / 0x63E; its route records 0x5EA064 and reveal records
   0x5E9FE0 equal the arcade's 0xC1810 / 0xC1790 with ids + 0x3F7). The PC 2D layer now keeps the whole 64-row
   tilemap, follows the scroll registers (0x100A000 / 0x100A008 per layer, as Arcade2D::compose) and draws a clipped
   draw's part only (Runtime::pcClip). Checked against t03400: the same map, labels and place; the red trail
   from Start to GATE. Not there: the walker (3D, not ported: polish.md).
3. **Continue screen (PC)**: no PRESS START BUTTON (0x40) in a game (the PC's hud_start_prompt is empty) and the PC's
   "Continues n" (hud_continues_text 0x41B304: fonts 0x1000000 / 0x2000000 = tables 0x5CF4A4 / 0x5CF2A4, centred on
   0x270, y 0x1C2 / 0x1BE). Checked against t02264: the same. The count: 9 less the continues taken (the PC counts its
   option + 1 down; the rewrite plays in free play) UNVERIFIED past the first continue.
- Same as the real PC (checked, no change): CONTINUE? and its digit (size, place), the in-play HUD (bullets, 1P,
  torches, cells), the fall, GAME OVER plate; the Chariot's unarmoured look (pink) and the red claw mark.

## Not fixed (leads)
- **Hit sparks**: drawn at a Chariot hit (HOTD_LIST_DRAWN_AT): shot flash 0x2D (+frame), 0x263, 0x218, debris
  0x660. Our 0x2D's first frames are a big square orange-red ball; the PC capture t12050 shows white puffs with orange
  rims and streaks (our later flash frames and sparks look like them). Not the 0x80 alpha bit (an additive test left
  the ball unchanged). The PC's shot_effect_flash 0x50BE08 draws the same model + frame. Next: the harness's per-routine
  capture of geo_draw_model at one hit (-CapFuncs) to see which IDs / frames the PC draws, or whether its texture's
  outer texels are colour-keyed.
- **Chariot end**: the bar looks empty from ~frame 2300 of `--event 19 --autofire` but the hit points stay at 4 (of
  440) for ~1400 frames: the autofire bot rarely lands the last hit (the PC's auto-aim does at once: death ~50 ticks
  after the empty bar). Not a logic difference seen (chariot_unarmoured_hits kills at <= 0). The life bar then ends
  with the death as on the PC.
- PC harness: after GAME OVER the hooked game stops advancing ticks (pc.log `poll ... front 0 screen 8`).
