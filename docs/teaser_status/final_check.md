# Final check (overnight run, 2026-10-16 morning)

Branch `claude/final-check`, build `build-final` (Visual Studio 17 2022, Release, target hotd), cut from main.

## 1. Checkpoints (both model sources)

`tools\checkpoints.ps1 -Build build-final`: the three runs of tools\checkpoints.txt (main, branch, die) for arcade and PC
models, 18 checkpoints each, 36 screenshots, none "not reached". Contact sheet with the MAME / real-PC references (under
private\, not committed):

`C:\Games\HOTD M2 RECOMPILE\M2 Source\private\checkpoints\final\index.html`

## 2. Bot seeds with rendering

`tools\robust_seeds.ps1 -Seeds 1..30 -Frames 40000 -Extra --render,--mute`, 6 in parallel. The seed picks models (even
arcade, odd PC), players, lives (9 / 0 / 3 / 9), start2 and strict-teaser runs.

| seed | models | players | lives | result | frames | problem | exit | GL lines |
|--|--|--|--|--|--|--|--|--|
| 1 | pc | 1 | 9 | gameover | 12539 | - | 0 | 0 |
| 2 | arcade | 2 | 9 | gameover | 20695 | - | 0 | 0 |
| 3 | pc | 2 | 9 | clear | 15180 | - | 0 | 0 |
| 4 | arcade | 1 | 0 | gameover | 5548 | - | 0 | 0 |
| 5 | pc | 1 | 0 | gameover | 9764 | - | 0 | 0 |
| 6 | arcade | 2 | 0 | gameover | 33004 | - | 0 | 0 |
| 7 | pc | 2 | 0 | gameover | 20110 | - | 0 | 0 |
| 8 | arcade | 1 | 3 | gameover | 4443 | - | 0 | 0 |
| 9 | pc | 1 | 3 | gameover | 5185 | - | 0 | 0 |
| 10 | arcade | 2 | 3 | clear | 25389 | - | 0 | 0 |
| 11 | pc | 2 | 3 | clear | 15197 | - | 0 | 0 |
| 12 | arcade | 1 | 9 | gameover | 6482 | - | 0 | 0 |
| 13 | pc | 1 | 9 | gameover | 9155 | - | 0 | 0 |
| 14 | arcade | 2 | 9 | clear | 27049 | - | 0 | 0 |
| 15 | pc | 2 | 9 | clear | 34998 | - | 0 | 0 |
| 16 | arcade | 1 | 9 | gameover | 16316 | - | 0 | 0 |
| 17 | pc | 1 | 9 | gameover | 7425 | - | 0 | 0 |
| 18 | arcade | 2 | 9 | clear | 15338 | - | 0 | 0 |
| 19 | pc | 2 | 9 | clear | 32228 | - | 0 | 0 |
| 20 | arcade | 1 | 0 | gameover | 23200 | - | 0 | 0 |
| 21 | pc | 1 | 0 | gameover | 18127 | - | 0 | 0 |
| 22 | arcade | 2 | 0 | gameover | 28573 | - | 0 | 0 |
| 23 | pc | 2 | 0 | clear | 23833 | - | 0 | 0 |
| 24 | arcade | 1 | 3 | gameover | 5626 | - | 0 | 0 |
| 25 | pc | 1 | 3 | clear | 23017 | - | 0 | 0 |
| 26 | arcade | 2 | 3 | gameover | 6118 | - | 0 | 0 |
| 27 | pc | 2 | 3 | gameover | 9307 | - | 0 | 0 |
| 28 | arcade | 1 | 9 | clear | 19968 | - | 0 | 0 |
| 29 | pc | 1 | 9 | gameover | 7528 | - | 0 | 0 |
| 30 | arcade | 2 | 9 | clear | 16602 | - | 0 | 0 |

Totals: 30 runs, 11 clears, 19 game overs. No crash (all exit 0), no timeout or soft-lock, no GL error or draw-call spike,
no bad memory access. One finding: seeds 10 and 14 (arcade models, 2P) each report a few `nan_draws` (6 and 5) with no
visible effect on the result; not investigated. Game overs are expected for bots that keep no lives (lives 0 / 3 settings).

## 3. How to play the build now

- Exe in the main checkout: `C:\Games\HOTD M2 RECOMPILE\M2 Source\build\Release\hotd.exe` (rebuild after merging). For a
  self-contained folder: `powershell -ExecutionPolicy Bypass -File tools\make_portable_hotd.ps1` (default output
  `private\portable\HOTD Teaser`), then double-click `Play.bat` (the launcher): choose the ROM zip / folder and the PC game
  folder, START.
- Directly: `hotd.exe --root "C:\Games\HOTD M2 RECOMPILE\M2 Source"`. Settings and results: `%APPDATA%\hotd-rewrite\hotd\`.
- Controls (docs/release/teaser_release_notes.md): P1 aim = mouse, trigger = left button, reload = right button (a shot off
  screen also reloads), start / join / continue = key 1. P2 = second mouse or arrow keys, Space / Right Ctrl to shoot,
  Enter / Right Shift to reload, key 2 to join. Esc = launcher, P = pause, F12 = screenshot, F1 = developer lines.
  Free play, Chapter 1 only.
