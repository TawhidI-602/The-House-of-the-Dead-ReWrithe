# Robust: crash / hang / soft-lock hunt with a human-like bot (overnight 2026-10-16)

Branch `claude/robust` (build folders `build-robust` = Release, `build-robust-a` = Release with asserts on, i.e. no
NDEBUG). Main merged in at 9a9fd99. Fixes are **UNVERIFIED (teaser)** (read from the typed decompilation).

## The bot and the watchdog

`hotd --frames N --bot <seed> [--bot-players 1|2] [--bot-gameovers N]` (src/game/bot.*, hooked into main.cpp's
headless loop). No `--lives-kept`: real hits, game overs, continues.

- Per seed and player a profile: skill (aim error sigma 4..30 arcade units), reaction 4..42 frames, shot gap
  5..25 frames, nearest-or-random target choice, wild shots, late / early reloads, lapses of 1..5 s (enemies hit
  it), hostages / researchers left alone except at a per-seed rate up to 10 % (a shot costs a life), continue
  chance 0 / 1 / random (pressed at a random moment of the ~15 s countdown, sometimes too late), start pressed in
  play now and then, shots in cut-scenes (skip requests).
- `--bot-players 2`: player 2 joins at a random frame, presses start in cut-scenes now and then, leaves (stops
  shooting, dies out, no continue) and rejoins later.
- A game over restarts the chapter as the window does, up to `--bot-gameovers` (2).
- Watchdog after each frame: soft-lock = nothing a player sees changes for 3600 frames (event script pc, event,
  game mode / sub-mode, camera position, player states / lives / scores / continue digit, enemies alive,
  letterbox); NaN in a world object's position or in a drawn matrix; task ring broken / not closing / > 2000 tasks;
  draw list > 20,000. Crash (SEH filter) and assert (SIGABRT) print `ROBUST: result=crash|assert` and exit 3 / 4.
- Last line: `ROBUST: result=clear|gameover|frames|problem ...` with counters and frame times.
- `tools\robust_seeds.ps1 -Seeds 1..128 [-Exe ...] [-Parallel 8]`: runs from a copy of the exe, 10 min wall
  timeout each; the seed picks models (arcade / PC), 1P / 2P (P2 joining, or both from the start), lives setting
  (9 / 0 / 3), teaser cuts on (4 in 5) or `--strict`. Writes results.tsv + one log a seed.

## Found and fixed

| # | symptom (seeds) | root cause | fix |
|---|---|---|---|
| 1 | soft-lock event 4 / 6 WAIT_ENALV with an idle player (b6, b43) | `projectile_hit_player` 0x6640 was a boundary stub: the Molbs and thrown weapons never hurt a player, so a player who stops shooting is never killed and the stop never ends | ported (src/game/enemy_support.cpp), 741e425 |
| 2 | soft-lock event 7 WAIT_ENALV (c31) | `researcher_reached_camera` 0x708E0 stub returned 0: the gift researcher (kind 64 states 7 / 14) ran at the camera for ever, enemy_left_route stayed 0 | ported with `researcher_rescue_bonus` 0x70960 (400 points), 5438b2e |
| 3 | NaN draw matrix, model 8 (b15, b42, b54, c44) | `blood_burst` 0x44EF0 reads its body part's camera-space place for 29 frames, also after the body freed its parts (faithful to the original) | `geo_draw_model` drops a model with a non-finite matrix (counted, `nan_draws=`); callers that tag `drawList.back()` check it was added, 5438b2e |

Bot fixes on the way: hostages avoided (else every seed died in event 0), an absent player 2 presses no start
(seed b50 had P2 continuing for ever at the Chariot without shooting: not a game bug).

## Results (final code, 256 seeds: d = 1..128 Release, e = 129..256 asserts on)

| config | runs | clear | stopped at 2nd game over | crash / assert / hang / soft-lock / NaN | game overs | continues + joins | frames |
|---|---|---|---|---|---|---|---|
| arcade 1P cuts | 51 | 3 | 48 | 0 | 97 | 144 | 653,551 |
| arcade 1P strict | 13 | 2 | 11 | 0 | 22 | 29 | 144,290 |
| arcade 2P (P2 joins) cuts | 34 | 27 | 7 | 0 | 17 | 291 | 656,856 |
| arcade 2P strict | 9 | 7 | 2 | 0 | 4 | 59 | 155,052 |
| arcade 2P from start cuts | 17 | 15 | 2 | 0 | 7 | 135 | 333,499 |
| arcade 2P from start strict | 4 | 4 | 0 | 0 | 2 | 25 | 76,139 |
| PC 1P cuts | 51 | 4 | 47 | 0 | 95 | 145 | 608,135 |
| PC 1P strict | 13 | 3 | 10 | 0 | 21 | 54 | 205,290 |
| PC 2P (P2 joins) cuts | 34 | 29 | 5 | 0 | 17 | 447 | 817,177 |
| PC 2P strict | 8 | 8 | 0 | 0 | 2 | 80 | 175,574 |
| PC 2P from start cuts | 18 | 15 | 3 | 0 | 9 | 131 | 364,445 |
| PC 2P from start strict | 4 | 3 | 1 | 0 | 2 | 14 | 66,209 |

All 256 exited 0; 0 bad memory accesses; max tasks in the ring 149, max shootable 29, max draw list 264.
Before the fixes (seeds 1..64, twice): 5 + 2 problems (above). 1P runs mostly end at the 2nd game over (bots
without continues); the bots played events 0-9, 11-13, 16-19 (not 10, 14, 15). 32 more seeds with `--route 1:0,1:3` / `--route 1:5` (300..331): 13 clear, 19 game over, 0 problems; still not 10 / 14 / 15 (the bots' shots re-pick the branch).
Headless the model source changes no logic (same results for the same seed / config).

## Frame cost (headless: the game logic, no drawing)

Quiet machine, 4 bot runs (7k-32k frames each, arcade / PC, 1P / 2P): mean 0.017 ms, p50 0.016 ms, **p99 (worst 1 %)
0.04 ms**, max 0.56 ms per game frame: ~0.3 % of a 16.7 ms frame. The multi-second "max" values in the parallel
batches' logs were the machine being busy (8 runs + a compile), not the game. Drawing (src/render) is not measured
here.

## Not covered / left

- The window loop itself (launcher, Esc, end screen, window resize) and rendering: headless only.
- `nan_draws` is 1..56 in ~10 % of runs: the blood bursts of freed parts now just are not drawn; a faithful fix
  would need what the arcade's TGP / geometrizer does with such data.
- Boundary stubs still met on the routes (see each log's "boundaries reached"): e.g. area_heading_pick,
  obj_velocity_clear, colour_blend32, frog_splash_spawn, shadow_update, sophie_skin_draw. None stalled a run.
- The bot never plays past the Chapter 1 clear (the teaser's end).

## robust-2 (branch `claude/robust-2`, build `build-robust2`, 2026-10-16)

### Rendering under the bot: `hotd --frames N --bot S --render`
New `--render` (with `--frames`): the headless run also draws every game frame (GameView + HUD) into an offscreen
framebuffer of a hidden window (no focus, nothing on screen), times it (synchronised by a 1-pixel read), counts
glDrawArrays (`Renderer::drawCalls`), checks glGetError, logs frames over 16.7 ms ("render: slow frame", split view /
HUD / GPU wait) and draw-call spikes (> 2x the median of the last 120 frames and +300), and ends with `RENDER: ...`
(mean / p50 / p99 / mean of the worst 1 % / max ms, calls, GL errors, spikes). `--shot-at F --shot-dir D` save PNGs
of that picture. `tools\robust_seeds.ps1 -Extra @("--render")` adds the RENDER line to results.tsv.

- 48 seeds (1..48: arcade / PC, 1P / 2P, lives 9 / 0 / 3, cuts / strict) + 12 route seeds (below), all rendered:
  **0 crashes, 0 GL errors, 0 draw-call spikes, 0 watchdog problems**, all exit 0. Draw calls 65-145 mean, max 254;
  draw list max 255.
- Draw time on a quiet-ish machine (RTX 5080, 992x768, serial): arcade 2P 20k frames mean 1.82 / p99 3.5 / worst 1 %
  8.1 / max 59 ms; arcade 1P mean ~1.6-1.9, p99 ~3.5; PC 2P 0.28 / 0.9 / 1.8 ms; PC 1P 0.43 / 0.96. The arcade shading
  costs ~6x the PC's, far inside 16.7 ms. The slow frames seen were other processes on the GPU / CPU.
- First frame with PC models: 70-990 ms in Hud::draw (PC sprites loaded on first use). GameView::init 3.3-4.3 s
  (arcade), 4-7 s (PC).
- Memory: a drawing process commits ~3.6 GB steady, ~5.9 GB peak at GameView::init (the arcade stage keeps ~1 GB of
  vertices on the CPU: 8631 models; headless ~80 MB). For the render worker (side notes robust-2-notes.md).
- The 6-way parallel batches had p99 20-30 ms / max 0.3-4.6 s: contention (6 x 3.6 GB + other workers' runs), not
  seen serially. The multi-second logic maxima at frame 63 in parallel batches (also in robust's logs): the first
  palette_grey_build frame, 0.1 ms alone, 5-17 ms with 8 parallel runs, seconds under memory pressure (first-touch
  paging): not a game bug.

### Events 10, 14, 15 (both sources): `--default-route` + `--route`
The bot's shots re-pick route_choice in the other events; `--default-route` holds those at 0 and `--route` forces the
branch. 24 seeds each, headless, arcade / PC by seed parity, 1P / 2P by seed:

| extra flags | events | clear | stopped at 2nd game over | problems |
|---|---|---|---|---|
| `--default-route --route 1:0,1:3` | 0 2 3 9 10 14 19 | 11 | 13 (in 10: 1, 14: 2, 19: 1) | 0 |
| `--default-route --route 1:3,1:9` | 0 1 3 9 11 15 18 19 | 12 | 12 (in 18: 1) | 0 |
| `--default-route --route 1:5` | 0 1 3 4 5 12 15 18 19 | 7 | 17 (in 15: 1, 18: 2, 19: 1) | 0 |

Plus 12 of those seeds rendered (4 per route, both sources, 4 in parallel): 12 / 12 clear, 0 GL errors,
0 spikes, draw calls max 235; mean 0.6-0.9 ms (PC) / 2.0-2.5 ms (arcade), p99 2-12 ms; max up to 5.5 s on two PC runs
= the first frame's PC sprite load under contention. No stall / crash / NaN in 10, 14, 15;
nothing to fix.

### The portable package (tools\make_portable_hotd.ps1 into a scratch folder)
10 files, 5.2 MB: hotd.exe, SDL3.dll, msvcp140 / _1 / _2, vcruntime140 / _1 (imports: those + the Windows UCRT
api-ms-win-crt-*, part of Windows 10/11): nothing missing. `--check-files` from the package: arcade 29/29, PC
156/156 + 12 music tracks, exit 0 (it prints 156 "Checking ..." progress lines).
Fixed:
- `hotd --frames ...` / `--no-launcher` from a package failed: "arcade regions missing in <package>/private/
  arcade_exports/regions" (a developer folder). Now, without the extracted regions, the ROMs the launcher remembers
  (launcher.ini) go through the files check (cached regions); with none: "no arcade ROMs chosen: start the launcher
  (Play.bat) once, or pass --arcade <ROM zip or folder>".
- A package made from a build folder without stb_vorbis (HOTD_STB_DIR / HOTD_FETCH_STB both off, as a fresh configure
  gives) plays no music and says nothing: make_portable_hotd.ps1 now warns.
Packaged `--frames 3000 --mute --autofire` (launcher.ini: the ROM zip, C:\Games\HOTD, PC models): exit 0, cues mixed,
music decoded; also with `--render --models pc`.

### Launcher settings without UI clicks
Headless runs read launcher.ini (LaunchConfig::load; models=1 there, so a bare `--frames` run uses PC models) and
`--models / --players / --difficulty / --lives / --blood / --strict` override the same fields (the launcher's Start
does the same cfg -> startGame). The report now prints `settings: difficulty D (dynamic level L) start lives S max
lives M blood B guards G pc models P`. Each non-default value:
- difficulty 0 / 1 / 3 / 4 -> dynamic level 1 / 4 / 11 / 15 (2 -> 7; a 2nd player +4). Enemies need more shots:
  autofire default route clears at 16,517 / 17,098 / 17,595 frames with 372 / 398 / 434 shots (0 / 2 / 4).
- lives 0 / 3 / 9 / 11 -> start-max 1-3 / 1-4 / 3-5 / 5-5 (the ROM table, as the launcher shows it).
- players 2 -> player 2 in play (state 5, 3 lives). models arcade / pc -> pcModels 0 / 1, the PC stage loads.
- blood 1-3 -> blood_color 1-3. Picture (--render --shot-at 830): arcade green turns the zombies' faces green but the
  bursts on bodies stay red and the blood pools go (props: types 27-29 only with red); PC non-red draws the install's
  TEXTURE.VMC (the user's: yellow), so GREEN shows yellow. Noted for the render worker.
- --strict -> guards 0.

### Left
- Memory footprint and the first-frame PC sprite hitch (render / hud areas).
- Arcade blood bursts in non-red colours; PC non-red colours need B_<colour>.VMC (PACK not cracked).
- The window loop itself (launcher clicks, Esc, resize) is still not driven.
