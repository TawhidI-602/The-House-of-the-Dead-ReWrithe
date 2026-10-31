# pause: the PC port's in-game pause, ported (2026-10-17, branch claude/pause)

Build folder `build-pause`. ROADMAP phase 7 ("The PC port's pause feature"). Everything here is **UNVERIFIED
(teaser)**: ported from the decompilation (private\decomp\pc\thotd.c) and tools/x86dis.py; the picture was checked by
eye in one of our screenshots, not yet against a PC harness capture.

## What THOTD.EXE does

State: `0x5CD358` paused, `0x5CE844` the selection (0 Continue, 1 Exit), `0x5CE848` "the play-time clock ran".
One C++ routine per original function in `src/game/pause.cpp` (`game::Pause`) and `Hud::drawPause`:

| THOTD.EXE | ours | what it does |
|---|---|---|
| 0x41ADF0 | `Pause::frame` | every main-loop frame. Not paused: in play only (game mode `0xDA0CDC` = 4, `shot_enable` set, the screen fade at rest: 0x41B204), an **Escape (DIK 0x01) or P (DIK 0x19)** edge pauses (no pause in cut-scenes, menus, game over, name entry). Paused: mouse y motion past +-15 picks Exit / Continue; Down / keypad 2 and Up / keypad 8 move the selection (wrapping); Enter / keypad Enter / Space / a mouse button activate it: Continue = cue 0x10C and the pause ends, Exit = 0x41AD34; Escape / P again end the pause (no cue). A selection change plays cue 0x10B. (Light-gun buttons via gamebios are left out: patch 3 turns the gun off.) |
| 0x41ACA8 | `Pause::enter` | selection 0, paused; the play-time clock stops (0x412B14, boss modes' timer: not ported); the CD pause 0x565BE0; message 0x418 to the sound window: every playing DirectSound buffer `Stop()`s (keeps its position). |
| 0x41ACF0 | `Pause::leave` | not paused, selection 0; the clock goes on (0x412B00); the CD resume 0x565B90; message 0x419: those buffers `Play()` again, from where they stopped. |
| 0x41AD34 | `Pause::exit` | Exit: frontend_state 7 (the menus), the CD stops (0x565D20), 0x419 then 0x407 (every effect off), clock cleared. Ours: the host leaves the game (`Audio::silence`) for the main menu (no launcher: the game quits). |
| 0x4185BC msg 0xF | `Pause::focus` | the window procedure 0x5660C0 on WM_ACTIVATEAPP -> 0x5633D0 posts message 0xF: **inactive -> the pause** (Continue selected), **active again -> the pause ends** (whatever opened it). Ours: SDL focus lost / gained, in every window run except automated ones (input test, --shot-at, --checkpoints, --auto-start). |
| 0x41AB7C | `Hud::drawPause` | three string-table labels centred on the 640 x 480 screen: 0x5F "PAUSED" at y 200 (font 0x2000000: sprites table 0x5CF2A4), 0x5E "Continue" at y 0xF8 and 0x24 "Exit" at y 0x110 (font 0x1000000: table 0x5CF4A4; the selected one 0x1000000 + 0x40000: table 0x5CF6A4, the yellow set = +10). Width 0x427F64 (glyph width + 1, space 8), drawn by 0x41AA54. |
| 0x418CC8 | `Hud::draw` with `pauseSelection >= 0` | while paused no `pc_sprites_render`, no subtitle (0x41B114), no sights: only the frozen 3D picture and the pause menu. No game frame runs (`frame_tick` only when not paused). |

Without the PC sprites the menu is drawn as text (overlay) with the same three lines.

## The user's patches (C:\Games\HOTD\PATCHES.md) and our behaviour

- **Patch 6 (before-pausefix -> corrected 6)**: the CD pause 0x565BE0 paused (and saved a resume command) whenever a
  track had been set (`cur_track != -1`), even after it had ended; now it asks 0x565E10 ("status cdaudio mode" = playing). Pausing with no music (after the
  stage-clear jingle) saves and resumes nothing (the original's bogus resume started the credits on Alt+Tab).
  Ours: `Audio::pause` holds the music only if a track is in the mixer (`musicHeld_ = hold && music_`).
- **Patch 8 (before-resumefix)**: the CD resume replayed the saved `play from <position> to ...` command, which near a
  track's end could start the next track; now it replays the current track via 0x565D60. With ogg-winmm a resumed song
  always started from the top. Ours: nothing is cued again on resume; the track and every held effect go on from the
  sample where they stopped (the original CD behaviour, without the next-track bug).
- Sounds started while paused (the menu's cues 0x10B / 0x10C) play at once (DirectSound: new buffers play; ours:
  new voices are not held).

## Audio continuity check (headless, --audio-wav)

Developer hook `HOTD_PAUSE_AT=F:N`: at the first game frame from F where the pause can open (Escape edge through
`Pause::frame`), N frames paused (their 1/60 s of the held mix is still written), then P closes it. Window runs: the same
variable opens the pause, takes the F12 screenshot after N loops and closes it.

Run: `--frames 3600 --autofire --lives-kept 1 --audio-wav` with and without `HOTD_PAUSE_AT=3000:120` (the pause
opened at frame 3493, the first in play from 3000; Track03 playing, RMS 2183 in the frame before), compared with a
small script (int16 stereo, 735 frames per game frame):

- before the pause: identical to the run without a pause (2 x 3493 x 735 values);
- during the 120 paused frames: all zero (nothing new was cued);
- after the pause: **bit-identical** to the unpaused run from the pause frame on (157,290 values, to the run's end),
  i.e. the music and the effects continue exactly where they stopped; nothing restarts.

## Checks

- Input test (`--input-test`, HOTD_PREF_DIR = scratch): **524 checks, 0 failed**. Changed steps: 145 P + a click
  in one frame = the pause, no shot; 146 Down = Exit selected (no player input); 147 P = running, the click does not
  fire; 170 Escape = the pause (Continue), 171 Down = Exit, 172 mouse 101's click on Exit = the main menu. The first
  game is still in its opening scene there (shot_enable 0, no pause on the PC), so the test holds shot_enable at 1
  for those steps.
- Headless `--frames 20000 --autofire --mute --lives-kept 1`: arcade and `--models pc` both "chapter 1 ended" at 16695.
- Screenshot (window, `HOTD_PAUSE_AT=3400:20`, PC models): PAUSED white, Continue yellow, Exit white, centred over
  the frozen picture, no HUD / sights (private\screenshots\hotd_20261017_035238_f3493.png, not committed).

## Controls now

Esc / P in play: the pause (was: Esc = leave the game, P = the developer pause). Exit in the pause menu = the old Esc.
N (one frame while paused) only with `--dev-lines`.

## Left

- A PC harness capture of the real pause screen (placement / colours compared by eye only with our own picture).
- The menus (front end) do not hold the audio on focus loss (the PC's message 0xF pauses the CD there too).
- The play-time clock (0x412B14 / 0x412B00) is not ported (the boss modes are not in the teaser).
