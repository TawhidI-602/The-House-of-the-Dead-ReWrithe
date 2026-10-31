# Input: the real mouse / keyboard path, checked end to end (worker "input", 2026-10-16)

Branch `claude/input`, build folder `build-input`. Nobody had played with real input yet, and nobody had tried two
physical mice. This branch adds a test mode that drives the normal window loop with injected SDL events, fixes the
bugs it found (and one found by reading SDL's source), and lists what only a person can still check.

## The test mode: `hotd --input-test <results.txt>`

`--input-test` opens the normal window **hidden** (no focus taken, no sound: `--mute`, swap interval 0), goes through
the launcher (`--auto-start`), and runs exactly one game frame per loop. Each loop it pushes events with
`SDL_PushEvent` into the real event queue (mouse motion / buttons with distinct mouse IDs, key down / up / repeat,
window resizes with `SDL_SetWindowSize`). After the frame it checks what reached the game: `Runtime::gun` (arcade
aim, on screen, trigger), the player record (`aim`, `on_screen`, `rounds`), and new counts in the input task
(`Runtime::shotsIn / reloadsIn / startsIn / inputFrames`, src/game/stand_ins.cpp). Results go to the file, one
`ok` / `FAIL` line per check, last line `N checks, M failed`.

Run (about 10 s): `build-input\Release\hotd.exe --root "<repo>" --input-test results.txt`.

Display worker (2026-10-16): run it with `HOTD_PREF_DIR=<scratch folder>` (the Start click saves launcher.ini) and
with `--aspect wide|4:3` / `--models arcade|pc`; frames 72-79 added (off the game area in every case: window edges,
outside the window, the 4:3 bars, P2's keyboard), 417 checks; see docs/teaser_status/display.md section 4.

What it checks (408 checks, all pass now; most are the per-frame "input task ran once" / "desktop cursor" checks):

| Area | Checks |
|---|---|
| Mapping | the desktop cursor at 1280x720, 992x768, 496x384, 600x900, 1600x900: two aim points each give the expected arcade point (496 x 384 screen, centre 0, y up), the player's `aim` and `on_screen`; the HUD crosshair pixel (window pixels) is where the game aims |
| Resize | P2's gun keeps its point of the game screen across each resize |
| Trigger | press + release in one frame = exactly one shot; held 5 frames = one shot; two clicks in one frame = one shot (the arcade reads the trigger once a frame); clicks on two frames in a row = two shots; the input task runs once per frame |
| Reload | right button; trigger at the window's corner (off the screen); trigger at x 300 (off the 496-wide screen, 16:9 window); x 247 still shoots; 6 rounds after the reload |
| Start | middle button = that mouse's player; keys 1 / 2 |
| P2 keyboard | Space / Right Ctrl = trigger, Enter / Right Shift = reload, a key repeat does nothing; Right arrow held 30 frames moves P2 lw/90 pixels a frame, P1 does not move; Alt+Enter is not a reload, F11 does nothing to the guns |
| Pause | clicks while paused do not fire, no game frame runs, nothing fires after P again |
| Two mice | (relative mode, IDs 101 / 202, SDL's one cursor in `motion.x/y` emulated as SDL reports it): each gun moves only by its own mouse, also when both move in one frame; each mouse's left / right / middle go to its player; the desktop cursor's ID-0 motion and clicks are ignored; a mouse pushed out of the window aims off the screen (reload), also at the top of a 496x384 window |
| Esc | in the game: back to the launcher, relative mode off; Start again: a clean second game (desktop cursor mode, P2 in the middle); forced chapter clear: Esc on the end screen goes back to the launcher |

Baseline: the same test against the unchanged input code failed 19 of 255 checks (commit 65df6ea's message), and
the end-screen Esc failed (bce5b06).

## Bugs found and fixed (one commit each)

1. **Two mice never worked on Windows** (9d93c33). Found in SDL 3.2's source (src/video/windows/SDL_windowsevents.c):
   outside relative mouse mode every mouse event comes from WM_MOUSEMOVE / WM_*BUTTON* with `SDL_GLOBAL_MOUSE_ID`
   (0); per-device IDs come only from raw input, which SDL enables only in relative mode. So a second mouse moved
   P1's cursor and P2 never got one. Now: with 2 or more mice (`SDL_GetMice`) and launcher "Player 2 input: a
   second mouse", the game runs in relative mode (`SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE` = the Windows pointer
   speed); each gun moves by its own mouse's motion, clamped to 5 % past the window so that pushing the mouse out of
   the window aims off the screen; ID-0 events (the desktop cursor's warps) are ignored. Leaving the game turns it
   off (the launcher gets its cursor back). In the old code P1 followed `motion.x/y`, which in relative mode is SDL's
   one cursor moved by both mice (the test's frame-152 failure).
2. **P2 keyboard aim clamped a mouse gun into the window every frame** (873d728): moved only while an arrow is held;
   the held arrows come from the key events (cleared on focus loss), so injected keys work too.
3. **A gun at the window's edge was on the screen** (f4bd146): `|y| <= 192` let the top / bottom edge (exactly
   +-192) shoot instead of reload; a 496x384 window had no off-screen spot at all. Now strictly inside.
4. **A click while paused fired when the game went on** (3110574).
5. **A resize / maximize / fullscreen moved the guns' aim** (16a56f0): guns are kept in window coordinates; they
   are now rescaled to keep their point of the game screen (P1's desktop cursor: until its next motion).
6. **No fullscreen at all** (a0d6c26): F11 / Alt+Enter toggle it (Alt+Enter is no longer P2's reload). Release notes
   updated (fullscreen keys; the captured mouse with two mice).
7. **Esc on the end screen quit the program** (53e4615): it goes back to the launcher now (without the launcher it
   still quits).

Also: the mice SDL sees are logged at the game's start (80e71c9). **This PC lists two: the mouse and the keyboard
(ROYUAN Gamakay TK75: its mouse interface).** So counting listed devices (fix 1 as first committed) would capture
a single player's mouse.

8. **Relative mode only once a second mouse is really used** (coordinator's request). The game starts with the
   desktop cursor; while a game runs with P2 input "a second mouse" it registers Windows raw mouse input on its own
   window (legacy mouse messages stay on) and watches WM_INPUT through `SDL_SetWindowsMessageHook`. A device counts
   only after it sends motion or a button (not the wheel). When a second device has done so, the game switches to
   relative mode: P1 = the device used first, P2 = the second. The second mouse's first event before the switch
   still goes to the desktop cursor. SDL's relative mode then uses its own raw input, and leaving the game removes
   the registration. Test: one mouse used plus a listed keyboard-mouse that sends nothing keeps the desktop cursor
   (frames 10-148). A second mouse used switches to relative mode with P1 = 101 and P2 = 202 (frame 149). The
   registration runs in the test too, without errors, but the hidden window gets no raw input, so the test feeds the
   device list directly. 408 checks, 0 failed.

## Not checked (needs a person)

- Two physical mice: that Windows' raw input really gives two IDs, each crosshair follows its own mouse, the speed
  feels like the desktop cursor (relative mode with the system scale), and P2 joins with its middle button.
- That WM_INPUT really reaches the hook while playing (one mouse: no switch; a second mouse moved: the switch), then
  alt-tab out and back and Esc to the launcher giving the cursor back.
- Fullscreen for real (F11 / Alt+Enter): the hidden test window only records it as pending, so the test covers the
  rescale by resizes, not a real mode switch; also high-DPI scaling (window pixels != window size: the test
  machine's hidden window had the same).
- The physical keyboard path (SDL_GetKeyboardState was replaced by the key events; real key presses use the same
  events, but no real key was pressed).
- Touch / pen (absolute positions, P1) untested.

Gamepads, motion aiming, the touchpad and light guns (worker "pads", 2026-10-19): docs/teaser_status/pads.md; the input test
has their cases at frames 331-600 (867 checks).

menufix (2026-10-21): the input test starts from the default settings (launcher.ini's arcade= / pc= folders kept,
command-line options such as `--models` / `--aspect` still apply) and never writes launcher.ini, so a settings folder
used by other runs (models=1, menustyle=1) no longer fails the Remake walk's Right / Left on Models, and one run no
longer leaves the Remake style saved for the next.
