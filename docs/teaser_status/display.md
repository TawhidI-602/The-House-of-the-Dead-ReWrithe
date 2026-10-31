# Display: frame timing, crosshairs, aspect, off-screen reload, blood colour (worker "display", 2026-10-16)

Branch `claude/display`, build folder `build-display`. Test runs use `HOTD_PREF_DIR=<scratch folder>` (new: the
settings / cache folder instead of SDL's pref path, so a test never touches the player's launcher.ini).

## 1. Fixed 60 Hz game logic

The window loop already ran ticks from a deadline (`next`), but rendering was capped by the same 1/60 s limiter and
there was no way to test other render rates. Now (src/main.cpp):

- the game logic is a fixed time step with an accumulator: `next` advances 1/60 s per tick; a rendered frame runs
  0, 1 or (catching up) up to 4 ticks; a stall longer than that drops the time instead of speeding up later; a tick
  due within a quarter frame runs now (no 0 / 2 tick beats at 60 fps), the long-run rate stays exactly 60;
- rendering: vsync (swap interval 1) plus the frame limiter (60 fps) as the fallback where vsync is off / refused
  or the display runs at 144 / 240 Hz;
- `--render-fps N` (debug): the render limit (0 = unlimited, vsync off); the rates are logged every 5 s, and every
  window run prints `timing: <ticks> game ticks, <renders> renders in <s> s: <ticks/s>, <renders/s>` at the end;
- `--stop-at F`: a window run ends after game frame F (prints the usual report).

Test (`--auto-start --mute --bot 7 --stop-at 900`, arcade models, hidden window):

| render limit | ticks | renders | ticks / s | renders / s | report |
|---|---|---|---|---|---|
| 60 (default) | 900 | 898 | 60.07 | 59.9 | reference |
| 240 | 900 | 3498 | 60.12 | 233.7 | identical |
| 0 (unlimited) | 900 | 8245 | 60.13 | 550.9 | identical |

(ticks / s slightly above 60: the first tick runs at the first frame.) UNVERIFIED on a real 144 / 240 Hz monitor.

## 2. Crosshairs: the originals' gun marks

The arcade draws no crosshair in play (a light gun); its own "gun mark" is the test menu's GUN SETTING -> mark check
(test_gun_mark_check 0xB0C0: `geo_draw_model` of table 0xA94D0 = models **0x660 (P1) / 0x661 (P2)**). The PC has the
same two models (its shot effect uses 0x660 + player too) in its own files. They are flat glowing marks, 5.3 units
wide, P1 red / orange, P2 blue (their own textures: the colour per player as each original).

- The view draws them (GameView::overlay, camera-space DrawModels after the game's) at the gun's point of the arcade
  screen, depth 0.5 (inside the near plane 1: depth clamp keeps them in front of everything), about 24 lines of the
  384-line screen wide (the test screen's scale is not decoded: UNVERIFIED size).
- Launcher "Crosshair: auto (follows the models) / arcade / PC" (launcher.ini `crosshair=0/1/2`, `--crosshair`).
  auto = the model source's marks. The other source's models are not loaded (the no-mixing rule), so a forced choice
  that differs from the model source keeps the old drawn crosshair (red / blue "+").
- Checked: screenshots at frame 1500, arcade and PC models, 2 players: P2's blue mark at the centre (P1's at the
  hidden window's 0,0 corner). Not checked: how the PC draws them in play (pc harness not used).

## 3. Aspect: widescreen 16:9 (default) / original 4:3

The view already fit the window's height (GameView::toArcade / fromArcade: the arcade's focal length 496 on its 384
lines), so a wide window is **hor+**: the same vertical view, a wider horizontal one, the whole window the game area.
Launcher "Aspect" (launcher.ini `aspect=0/1`, `--aspect wide|4:3`):

- widescreen 16:9 (default, the user's decision): as before;
- original 4:3: the arcade's 496 x 384 screen pillarboxed: black bars over the sides after the 3D (before the HUD).
  The guns' on-screen test is already `|x| < 248` of the arcade screen, so a gun in a bar is off the screen (reload).

The HUD's 2D layers (arcade tile layer, PC sprites) are one picture of the original screen, centred over the 4:3 part
in both modes (anchoring the corners to the window's edges would split centred text: not done). Screenshots at 1280 x
720, frames 1500 / 2400 (arcade models): the 4:3 picture is the wide one's centre; no object missing at the wide edges
in these two frames. Not checked: the game logic's own off-screen tests (draw zones, the original's 4:3 culls) over a
whole run; `HOTD_NO_CULL` shots differ from the culled ones (back faces too, not looked into). UNVERIFIED.

## 4. Off-screen reload in every case

The gun's on-screen test (src/main.cpp, into Runtime::gun) is now the game area's: widescreen = the whole window
(`|x| <` the window's half width in arcade units, strictly: the left / right edge is off), 4:3 = the 496 x 384
screen (the bars are off); `|y| < 192` (top / bottom edge off) and outside the window off in both. Before, the wide
parts of a widescreen window counted as off the screen although the game is drawn there. Both players (mice, P2's
keyboard), both model sources.

`--input-test` frames 72-79 (1600 x 900 window): P1 trigger at the window's left / right edge, left / right of the
window, at x 300 (wide: a shot, 4:3: reload); P2 keyboard at the edge, at x 300, in the left bar's middle. Frame 95
(x 300) and the 600 x 900 portrait check (x -200 lies outside that window: off) follow the aspect. The two-mice part
already pushes each mouse out of the window (159, 163). Results (`--aspect wide|4:3`, `--models arcade|pc`):
**417 checks, 0 failed** in all four runs.

## 5. Custom blood colour (EXPERIMENTAL)

Launcher "Blood colour: red / green / blue / purple / CUSTOM (experimental)" + "custom colour #RRGGBB" (click: presets
white (the default) / yellow / orange / cyan / pink / near-black; launcher.ini `blood=4`, `bloodrgb=RRGGBB`;
`--blood 4 --blood-rgb RRGGBB`). The game logic runs with red (blood_color 0: the GGSET face rows stay red).

- Arcade: polygon palette entry 1023 (the bursts / drips / gore) = the colour as RGB555 (R low, as 0x801F).
- PC: B_RED.VMC's pages, the texels that differ from B_GREEN.VMC's (the blood) tinted: the red set's brightness
  times the colour (312,822 texels on the Chapter 1 pages); the grey (brown.pal) pages untouched.
- Checked: bot runs at frames 1950-2100, red vs custom green: the frames differ; the PC log reports the tint.
  Not looked at closely. The screen splash at frame 1500 stays red in both sources with **every** setting (also green
  0-3): another texture / palette, not found (left).
- Release notes: the display settings (widescreen is the default, decided; custom blood labelled experimental).
