# rate: the game speed, the frame rate and render interpolation (2026-10-19/20, branch claude/rate)

## The real rate

- **Arcade: 57.5241 Hz.** The game logic runs once per vblank IRQ (MAME `model2.cpp` `screen_vblank` -> IRQ line 0).
  The screen is `set_raw(32 MHz / 2, 656, .., 424, ..)`: 16,000,000 / (656 x 424) = 57.52416 Hz, one tick every
  17,384,000 ns exactly.
- The 57.13 game frames per second MAME shows (sound_board.md) are this rate minus lag frames: frames where the main
  loop did not finish before the next vblank (MAME's i960 timing; the loads, e.g. the prologue's world load,
  attract2.md). Our port never lags (loads are done ahead or on a worker), so its rate is the vblank rate.
- **PC (THOTD.EXE): 60 Hz.** `pc_main_loop` (0x419168): `ticks = (timeGetTime() - start) * 60 / 1000`, runs
  `frame_tick` until caught up (reset after 500 ms or 30 ticks behind), drawing decoupled. Same arcade logic, only the
  rate differs, so there is one setting: **Game speed = Arcade (default) / 60 Hz (PC)**, with either model source.

## Settings

| Setting | launcher.ini | command line | values |
|---|---|---|---|
| Game Speed | `gamespeed=` 0 / 1 | `--game-speed arcade\|60` | Arcade 57.52 Hz (default), 60 Hz (PC) |
| Frame Rate | `framerate=` | `--frame-rate` | Display Refresh (default), 30, 60, 75, 90, 100, 120, 144, 165, 180, 200, 240, 280, 300, 360, 480, 500, 540, Unlimited (no cap, vsync off) |
| Vsync | `vsync=` 0 / 1 | `--vsync on\|off` | On (default) |
| Smooth Motion | `smooth=` 0 / 1 | `--smooth on\|off` | On (default) |

All four are rows of More Options and of the text launcher. Online: the host's game speed goes to the client in the
settings packet (kProtocol 6: Sonic Mode then gameSpeed after the routes); both PCs tick alike.

## How

- **Tick:** the window loop's fixed time step advances `next` by num / den ns (17384000 / 1 or 1e9 / 60), through
  `gSonic.advanceNs` (carries the remainder; Sonic Mode shortens play ticks). Headless / proof / input-test / --frames
  runs are frame-based: unchanged (the input test keeps its 60 loops a second).
- **Presentation:** a cap of N fps = the limiter at N (vsync as set); Display Refresh = vsync paces it (the limiter
  at refresh x 1.1 only as a fallback; with vsync off the limiter at the refresh); Unlimited = neither. Smooth Motion
  off: a picture only when a new game frame exists (a 60 Hz display repeats one now and then); otherwise the loop
  waits for the next tick.
- **Render interpolation (view side only):** the frame loop copies the drawn state (camera, focal, draw list, skins:
  `game::FrameSnapshot`) before each tick; `GameView::draw(rt, w, h, prev, alpha)` draws the 3D picture at
  alpha = (now - the newest tick's due time) / its period between the two. Camera: position lerp, orientation slerp,
  focal lerp; a camera cut (> 40 units or > 30 degrees in a tick) draws the newest frame whole. Models: matched by
  `DrawModel::task` (task_current at the draw, set by the runtime, never read by the logic) + model + body + shadow +
  order, or for a task drawing one model whose ID changes (flip-books) by the task alone; world matrices blended
  element by element with the axes rescaled (parts joined at a point stay joined); unmatched / jumped (> 40 units or
  > 60 degrees) items as the newest frame has them. Skins: their anchors the same way. HUD, 2D layer, sights: not
  blended (the sights are the live input). Screenshots / checkpoints draw the newest frame exactly.
- **Audio:** the device pulls the mixer by its own clock: the music and the arcade board (68000 + SCSP run per output
  sample, `SoundBoard::render`; MIDI bytes one per 14 samples) play at their true speed whatever the tick rate. Only the
  cue times follow the game. The `--audio-wav` file renders one game frame per tick: 44100 / 57.524 = 766.6 samples.

## Tests (2026-10-20)

- `--frames 40000 --autofire --mute --lives-kept 1`, arcade and `--models pc`: "chapter 1 ended" at 16695 (unchanged).
- `--input-test`: 866 checks, 0 failed.
- hotd_nettest: all passed; tools\netplay_test.ps1 (host `--game-speed 60`, 6000 frames): equal hashes, the client
  reports "game speed 60 Hz".
- Hidden window, `--auto-start --autofire --mute --stop-at 1200` (~20 s): arcade 57.53 ticks/s, 60 Hz 60.01 ticks/s
  (smooth on / off alike); smooth off renders exactly one picture per tick; `--frame-rate 240 --vsync off` 217
  renders/s, unlimited 302 renders/s (5.2 pictures per tick).
- `--interp-dump <dir> <F>` saved pictures of frames 900-902 (motion: 116 of 117 items blended) and 333-335 (the cut at
  334: the newest frame drawn whole, no pop) to private\rate\.
- Attract WAV (`--title --sound arcade --frames 6000 --audio-wav`): the demo's music cue 0xA01001 at frame 4485 = 77.95 s
  after mode 9 (was 74.7 s at 60 Hz; MAME 78.5 s). The remaining 0.55 s (~32 frames) are MAME's lag frames.
