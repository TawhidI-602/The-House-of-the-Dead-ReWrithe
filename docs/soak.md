# Soak / crash hunt (worker "soak", 2026-10-27)

Goal: the game playable start to finish. A matrix of headless, muted runs across the modes and options, each with a
wall-clock timeout, every log scanned for crashes, hangs, GL errors, the watchdog, memory faults, NaN draws and the run's
expected end line.

## How to run

```
powershell -ExecutionPolicy Bypass -File tools\soak.ps1 -Root "<main checkout>" [-Set quick|full] [-Jobs 6] [-Only <regex>]
```

- `-Root`: the folder with `private\` (game data). `-PcDir`: the PC game's folder for the menu shots (only read,
  default `C:\Games\HOTD`). `-Exe` (default `build\Release\hotd.exe`), `-Out` (default `build\soak`), `-Timeout` (s, 900).
- Every run gets its own `HOTD_PREF_DIR` (`<Out>\pref\<run>`); a run past its timeout is killed by its own PID.
- Output: `<Out>\summary.md` (one row per run), `<Out>\<run>.log` (the command line + the whole log), the menu
  pictures in `<Out>\shots`. Exit code = the number of failed runs.
- A run fails on: a non-zero exit (a crash shows as `exit 0xC0000005` etc.), the timeout (`HANG`), `GL error` /
  `gl_errors>0`, `watchdog:` / `ROBUST: result=problem|restart-failed`, `access outside the mapped memory`,
  `nan_draws>0`, an assert / abort, a render set-up failure, or its expected line missing.
- Quick set: about 200 runs, 3 min with 6 at once on this PC. Full set: 340 runs, 5 min.

## What the matrix covers

| Group | Runs |
|---|---|
| Arcade, every chapter | Chapters 1-3 (on into the next) and Chapter 4 `--strict --default-route` to "the ending is over"; arcade and PC models |
| Drawing (`--render`, offscreen GL) | arcade / PC models x Draw Depth sort / buffer x Aspect 16:9 / 16:10 / 4:3; Ragdoll mod on; languages en / ko / ar; every chapter both models (Chapter Select); 4P with PC models; Boss Mode; the continue / GAME OVER screens (1P, 4P, both models, Boss Mode); bots with game overs |
| Players | 2P (join at 600), 2P with a CPU teammate, 3P + a late join, 4P, 4P all-CPU teammates (easy / normal / hard), 4P per chapter |
| Difficulty | 0 and 4 |
| Continue paths | `--lives-out` (CONTINUE countdown, GAME OVER, the restart) 1P / 4P / PC Mode; `--bot` seeds (human-like play, continues, joins / leaves, game overs restart the chapter, the watchdog) 1P / 2P / 4P / PC Mode / Chapters 2-4 |
| PC Mode | every character 0-6 (P1), 2P, 4P, every chapter (2P), Chapter 4 to the ending, lives out |
| Boss Mode | each boss, All Bosses, All Bosses 4P |
| Sonic Mode | frenzy / overkill / nightmare; the whole game (full set) |
| Whole game (full set) | arcade `--strict`, 4P, PC Mode, Sonic, PC models drawn: Chapter 1 to the ending in one run |
| Menus (`--frontend-shot`) | Classic and Remake styles x en / ko / ar x the screens (quick: 20, full: 0-36, 119, 121) |
| Window flow | `--input-test` (the menus, a game, the pause's Exit, the next game; now also: no Boss Mode / PC Mode left over) |
| Name entry after the ending (misc17) | Chapter 4 to the ending, `--name-entry --ranking-low --name-keys ...`: arcade and PC models (1P, "ABC"), 2P, 4P (both rounds: players 1 / 2, then 3 / 4), END at once (an empty name) |


Not covered by the headless matrix: online play (hotd_nettest, tools\netplay*_test.ps1). The name entry was
window-only: a headless game's score never makes the ROM's default top 10, so the arcade went straight to the
ranking; `--ranking-low` (testing) starts the table at 0 points. One key every 8 frames; END by one player ends the
entry for both (the arcade's one done flag), so in the 2P / 4P runs the second name stays blank.

## Results (2026-10-27)

| Run | Runs | Failed |
|---|---|---|
| quick set (first pass) | 77 game runs | 0 |
| menus + bots + continue screens | 135 | 0 |
| full set | 340 | 0 |
| full set again after merging main (bossbug, depthcmp) | 340 | 0 |
| input test after the merges (with the 2 new soak checks) | 1996 checks | 0 |
| Chapter 1 arcade / `--models pc` | | chapter 1 ended at 16695 / 16695 |
| Chapter 4 `--strict` arcade / PC models | | the ending is over at 34967 / 34967 |
| 4P Chapter 1 | | chapter 1 ended at 22201 |

No crash, hang, GL error, watchdog stop, memory fault or NaN draw in any run. Whole game in one run: arcade ending at
frame 101216, 4P 125169, PC Mode (`--pc-char 3,4`) 92282, Sonic frenzy 101216 (same logic frames).

### Regression sweep (worker "soak2", 2026-10-27, main 9f6a09d1 after ~25 merges)

| Run | Result |
|---|---|
| full set (incl. name entry 1P arcade / PC, 2P, 4P, empty; menus 0-36, 119, 121 x classic / remake x en / ko / ar) | 345 runs, 0 failed |
| menus + language renders again after the fix below | 237 runs, 0 failed |
| `netplay_menu_test.ps1 -Mode all` | 5 / 5 PASS |
| `netplay_test.ps1` 2P arcade / 2P PC Mode / 2P All Bosses | result=ok, hashes equal (8543b41a4be0f433 / cbc6db51f02aadaf / 27a2d2e5d2559f67) |
| `netplay4_test.ps1` 4 PCs / 4 PCs All Bosses | SYNC PASS (604ed7f9d8b2db42 / 7f12017958ed4be3) |
| `netplay_join_test.ps1` rejoin / rejoin All Bosses / PC Mode mid-game join | result=ok both sides (back in at 3487 / 3041; joined at 3919) |
| `hotd_nettest` | all passed |
| `--mods-test` | 8 checks, 0 failed |
| `--lang-check` en / ko / ja / ar / he | 5 / 3 / 5 / 5 / 5 too wide (the known PC Mode scores sample names only), 0 cut |
| `hotd_proof pmmaths` | 340 routines, 6,607,100 cases, 0 differ |
| input test | 2071 checks, 0 failed |
| Chapter 1 arcade / PC models; 4P | chapter 1 ended at 16350 / 16350; 22283 |
| `--chapter 2` run; Chapter 4 `--strict` both models | Chapter 2 16550, Chapter 3 48114; the ending is over at 34967 / 34967 |

Whole game in one run now: arcade / Sonic 100017, 4P 118017, PC Mode 89859, PC models drawn 100017.

After merging main again (pmch34: PC Mode Chapters 3-4 on the PC's rules): full set 345 runs, 0 failed (PC Mode,
Boss Mode, name entry all ok; whole PC Mode game 87666); standard checks the same as above; input test 2071 / 0;
`hotd_proof pmmaths` 355 routines, 0 differ; `hotd_nettest` passed; online 2P arcade 8543b41a4be0f433, 2P PC Mode
59e5a33d79de96b5 (new: pmch34 changed PC Mode), 4 PCs All Bosses SYNC PASS 7f12017958ed4be3, rejoin ok;
`--lang-check ar` 0 cut.

## Bugs found

| Bug | Where | State |
|---|---|---|
| Boss Mode left with the pause's Exit stayed on: the next Arcade / Chapter Select / PC Mode game started Boss Mode again | main.cpp `backToMenu` (bossModeOpt never cleared on that path) | **fixed** (found by both soak and bossbug the same day; the merged fix is bossbug's, with its own input-test game) |
| After a PC Mode game the menu's attract ran with PC Mode's rules and an Online Co-op game (Game Speed Auto) ticked at PC Mode's 60 Hz instead of the arcade's 57.52 | main.cpp (gPcMode only set by the modes that start from the menu) | **fixed**: cleared with the menu as well; input test checks it |
| `--bot-skill` without `--bot` in a window run crashed (a null bot: the loop was outside the `if`) | main.cpp (developer option) | **fixed** |
| Menus in ar: several strings still English ("Players", "Player 2: Human", "Start", "Accept"); ko name entry in English | lang files (samples only, as known) | noted, not a crash |
| Frame-time spikes up to ~190 ms in `--render` runs | only with 6 runs at once (GPU shared); a single run: max 5.5 ms | not a bug |
| soak2: a menu value cut with "..." lost its translation (Arabic Co-op page, classic: "Host / Join..." in English) | frontend.cpp / frontend_remake.cpp: cut in English before the lookup | **fixed**: translated first, then cut; `--lang-check` counts such a cut (ar: 14 before, 0 after) |
| soak2: classic menu in ar / ko: rows touch or overlap a little on Co-op and Firing Range | the language font is taller than the PC font's row | noted, not changed |

Harness notes: `--frontend-screen` 5, 6, 23 draw the main menu (not separate screens there).
