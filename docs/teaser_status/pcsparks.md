# pcsparks: PC hit sparks, PC route-map walker (2026-10-16)

Branch `claude/pcsparks`, build folder `build-pcsparks`. Not merged. UNVERIFIED (teaser): judged by headless
`--render` shots (`--models pc --event 19 --autofire`, frames 1500-1534) against the real-PC harness captures
private\screenshots\pc_look\pcpolish\full t11205-t13956 (the Chariot fight, ~30 hits, never an orange ball).

1. **Hit sparks (pcpolish2 lead)**: the big square orange ball was not the flash 0x2D. `HOTD_LIST_DRAWN_AT` at the
   ball's frame shows no 0x2D, only the shot debris 0x660 (+ player) and the hit effect 0x219. New debugging switch
   `HOTD_PC_DUMP_MODEL=<hex id>` (src/assets/pc_loader.cpp) prints a PC model's polygons: 0x2D = 12 quads of the
   petal puff (page 589, 197..226 x 97..126, key 0xFC1F), 0x660 = one quad of the dithered orange ball (page 589,
   165..194 x 97..126). THOTD.EXE's shot_effect_debris (0x50BD70) differs from the arcade's (0x95630): it draws only
   once its timer is past 5, ends at 30, and shot_effect (0x50BF10) gives it 40 units a tick (0x5A7258) where the
   arcade's draws it from the first frame for 60 at 20. The first frames, right in front of the camera, were the
   ball. Now Runtime::geo_draw_model (PC models only) skips 0x660 / 0x661 from shot_effect_debris for n <= 5 and
   n > 30 and pushes the drawn place out along the ray from the eye by another 20 n (arcade logic unchanged; the
   camera's turn since the shot is left out). Result: white puffs with orange rims and streaks, as the PC.
   The flash's 0.08 scale (pcpolish) stays.
2. **Game-over route map walker (PC)**: the walker was already in the draw list in PC mode (game_over_walker_task,
   the port's, with the PC bodies 0x24.. / 0x2A1 ..), but the PC 2D layer (the map's opaque picture) was drawn over
   the 3D and hid it. Hud::drawUnder now draws the PC 2D layer under the 3D in mode 5 sub-modes 2 / 3 (and Hud::draw
   skips it that frame). The headless `--render` path now calls drawUnder as the window does (it never did: the
   arcade's low-priority cells were missing from headless shots too). Checked: frames 2640-2790 of
   `--models pc --lives-out 1500`: the figure walks up to FRONTYARD and falls on the trail, as the real PC's
   pcpolish2\gameover t03351 / t03400 / t03465. UNVERIFIED (teaser).

Other workers' files touched: src/game/runtime.cpp (geo_draw_model), src/main.cpp (headless drawUnder),
src/assets/pc_loader.cpp (debug dump), src/game/hud.* (drawUnder / draw).
