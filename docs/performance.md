# Performance (the perf pass of 2026-10-21)

Goal: load fast and run smoothly on slow machines (Surface Pro 7, Steam Deck, Intel graphics) with the game logic and the
pictures unchanged. Earlier work (memory, the first PC frame): `docs/teaser_status/perf.md`.

## How it is measured

- `hotd --frames N --autofire --mute --lives-kept 1 --render [--models pc]` (headless, hidden window, offscreen
  picture). The end prints `RENDER:` (frame time with the GPU wait, draw calls), `MEMORY:` and now `PERF:` lines:
  per frame the logic (`Runtime::frame`), the sound (`Audio::renderFrame`; with `--audio-wav` the mix and the arcade
  sound board), `under` (the arcade 2D drawn under the 3D), `view` (the 3D), `hud`, `gpu_wait` and `gl_state_calls`
  (uniforms / texture and vertex-array binds a frame, `Renderer::stateCalls`), and `PERF: chapter N load`.
  `HOTD_PERF=1` prints the `PERF:` line without `--render`.
- `view: set-up .. ms (sources, animations, skins), arcade stage .., the rest .. (shaders, PC stage, layers, upload;
  the GL part here ..)`: where a view's load goes.
- Start-up: `HOTD_STARTUP_TIME=1 hotd --root .. [--auto-start]` (hidden window): `startup: the launcher's / the menu's /
  the game's first frame at N ms` since the process started, then it ends. `HOTD_MENU_WAIT=N`: --auto-start's
  Arcade after N ms in the menu (as a player who looks at the menu first). The window's `start: N ms (regions, sound,
  game, view and HUD)` line is the Start -> game set-up.
- Pictures: `--shot-at` frames of the before / after builds compared byte for byte (Chapter 1 + the start of
  Chapter 2: frames 300, 1500, 4000, 9000, 15000, 16800, 20000; Chapter 4 + the ending: 500 .. 33000, including the
  staff roll's other worlds; the attract with the prologue: 1000 .. 8900): **all identical, both model sources**.
  Sound: a 6000-frame `--audio-wav` identical byte for byte.
- RTX 5080 / 16 threads, 992 x 768. Other workers were using the machine at times; the numbers below are from quiet
  stretches (the A/B runs under load are not used).

## Results

| What | Before | After |
|---|---|---|
| Arcade start from the menu (Start -> game ready, window) | 1540-1780 ms | 310 ms; **~90 ms** after a second on the menu |
| Start-up: the launcher's first frame | 455-485 ms | 290-310 ms |
| Start-up: the menu's first frame (launcher.ini: Start by itself) | 780-820 ms | 690-720 ms |
| Chapter view load (`render: view ready`, arcade / PC) | 1460 / 770 ms | 320 / 330 ms |
| Chapter change, time in that frame (arcade / PC) | 1350-1560 / 600-690 ms | **25-40 / 24-25 ms** (prepared during the clear) |
| Ending's staff roll, 3 world changes (arcade / PC) | 1260 ms each / 50-60 ms | **26-29 ms** / no slow frame |
| Prologue (attract) first frame (arcade) | 105 ms | no slow frame |
| CPU a frame: 2D under + 3D + HUD, arcade (Chapter 1, 1500 frames) | 0.78 + 1.41 = 2.2 ms | 0.14 + 0.11 + 0.26 = 0.5 ms |
| CPU a frame, PC models | 0.13 + 0.17 = 0.30 ms | 0.09 + 0.14 = 0.23 ms |
| GL state calls a frame (Chapter 1 start, arcade / PC) | 2680 / 1810 | 470 / 290 |
| GL state calls a frame (whole Chapter 1 run, arcade / PC) | 1580 / 1200 | 320 / 230 |
| Draw calls a frame (Chapter 1, arcade / PC) | 62 / 47 | 62 / 47 (unchanged) |
| Rendered frame incl. GPU wait, Chapter 1, arcade mean / p99 | 2.26 / 3.67 ms | 0.81 / 1.36 ms |
| Worst frame, Chapter 4 + ending (arcade / PC) | 1263 / 61 ms | 29 / 59 ms |
| Sound board, per game frame (audio thread) | 0.65 ms | 0.61 ms (68000: 1220 -> 920 cycles a sample) |
| Memory peak, Chapter 1 -> 3 rendered run (arcade, commit) | 1528 MB | 1210 MB |
| Game logic a frame | 0.03 ms | 0.03 ms (not touched) |

## What changed

1. **Decal layers** (`markCoplanarLayers`, 1.2 s of every arcade chapter load): `getenv` was called per triangle;
   now read once, the models run on up to 8 threads (`markCoplanarLayersAll`), the plane buckets are a hash map.
   1178 -> 70 ms. Same marks (34720 triangles in Chapter 1).
2. **Load ahead** (`GameView::prepare` / `finish`, main.cpp `ViewPrefetch`): a view's load is split into the part
   without GL (files, models, layers; any thread) and the GL part (shaders, upload, ~40 ms). Chapter 1's view is
   prepared while the menu shows (Arcade then starts at once); the next chapter's during the chapter clear; the
   attract's / ending's other worlds (`prefetchExtra`) fully on a worker with their own sources, the staff roll
   loading Chapters 3, 2, 1 one ahead. A different chapter or setting loads as before. Background loads use half the
   threads (the game keeps its share).
3. **Arcade 2D layer** (`Arcade2D::compose`): composed up to seven times a frame (the low pass twice, the widescreen
   sides three times each), per pixel; now once per pass and side, the cells looked up once a line, straight into the
   picture.
4. **Renderer**: uniforms / texture binds sent only when they change, each item's stage / ranges looked up once
   (were once per pass), the PC pages' filter set once.
5. **Start-up**: the PC sprite pages (BIGGER.VMC) read once per run and converted on several threads (the menu loads
   them twice, the HUD once more: `Frontend::load` 220 -> 130 ms); the gamepad subsystem (~150 ms of device
   enumeration) starts after the first frame.
6. **Sound board**: the 68000 reads its program ROM / RAM directly, the MIDI queue is checked without a lock when
   empty. The SCSP (MAME's code, ~2600 cycles a sample, 75 % of the board) is unchanged; the board runs on the audio
   thread, not in the frame.
7. CPU copies of a stage freed on a worker after the upload (freeing ~200 MB took ~15 ms of a frame); the extra
   worlds' copies are freed too; the headless chapter change releases the old view's GL objects (it leaked ~250 MB a
   chapter in `--render` runs).

## Low-end GPUs

Draw calls stay ~50-60 a frame (one per mesh range; batching would change the draw order and so the pictures). The
state calls between them are down ~5x, which matters most on Intel / AMD integrated drivers. The CRT filter is off by
default; the arcade's per-pixel colour pipeline is the shader's main cost and is what makes the arcade look; it is not
changed. Frame time with the GPU wait on this machine: 0.8 ms (arcade) / 0.4 ms (PC).

## Left

- PC models, Chapter 4 -> ending: the PC's WORLD5 / WORLD6 files load on first use (`pcAltFor`, 31 + 44 ms frames).
- The extra worlds' upload (~25 ms, arcade textures) still happens in the frame they appear.
- The SCSP's own cost (MAME code, not ours to change).
- Start-up before the launcher: SDL video (45 ms), the window and GL context (150 ms), the display mode query (100 ms):
  driver time.
- Memory: the menu's and the HUD's PC sprite pages are separate copies (~180 MB each with the brown pages).

## Menus (menuperf, 2026-10-26)

User: "The Remake menu's Jukebox page lags even on my RTX 5080 PC; the whole Remake menu is laggy on my Surface Pro 7
(Intel graphics). Optimise for low-end devices."

### How it is measured

`HOTD_MENU_PERF_TOUR=N hotd --root .. --menu-style remake --mute --size WxH [--live-bg auto|on|off]` (hidden window):
waits for the live attract, then opens every Remake page in turn for N frames (Main menu, More Modes, Extras, Options'
Game / Video / Audio / Controls / Gameplay / Mods / System tabs, a dropdown, Chapter Select, Boss Mode, PC Mode, High
Scores, the Jukebox playing a song, the Gallery, its Cut-scenes page), pressing Down six times then Up six times (the
focus moving; on the Jukebox: the chosen row), and prints per page: the CPU a frame (the whole menu frame: input, the
page, the live attract, the draw calls; mean and most), the live attract's share, the GPU a frame (GL timer queries,
read a few frames later: no stall), the page's first frame ("open"), the uploads, the time between frames.
`HOTD_GPU_CLASS=weak` makes this PC count as integrated graphics (Live Background's Auto / 30-a-second paths).
RTX 5080 + a 16-thread CPU; the "before" build is the tour's own commit (nothing else changed).

### What was slow

- **The Jukebox** rebuilt its whole page whenever its time text or progress changed (each second, each 0.4 % of the
  song) and on every Up / Down: the whole screen's widgets drawn again on the CPU (20-65 ms) and uploaded (8-22 MB).
- **Every focus move** drew the lit widget and the help line again on the CPU: 10-35 ms a move (the paper strip's
  noise ~12 ms, the text's outline search ~1-5 ms a text).
- **Page opens** 50-350 ms: the paper strips, torn panels and texts of the whole page, on one thread.
- **The frame pace**: a 16 ms sleep after each frame (16 ms + the frame's own time: 45-58 fps, uneven; with vsync a
  missed refresh now and then).
- **On integrated graphics**: the live attract (a full 3D frame at the window's size, 60 a second) is most of the
  menu's GPU time; at 2736 x 1824 (the Surface's screen) about 3-6 times the rest of the menu.

### What changed (the pictures are the same: `--frontend-shot` of 10 pages at 1920 x 1080 and 2736 x 1824 compared
byte for byte before / after, except the Video tab's new row)

1. **Frame pacing** (main.cpp `menuPace`): the launcher, the menus and the end screen run on a fixed beat at the
   display's refresh (the game's limiter; vsync on: the swap waits, the limiter as the fallback); never unlimited.
2. **Focus parts kept** (`partCache_`): a lit widget's / the help line's part is drawn once per page and copied when the
   focus comes back.
3. **Live widgets** (`Widget::live`): the Jukebox's time, bar and list rows are each their own small part, updated in
   place (`jukeboxInPlace`) when only the time, the progress or the chosen row (on the same list page) changed; the
   page itself is no longer built again.
4. **Faster drawing, same pixels**: rText's outline uses the disc's offsets once and skips areas without letters;
   rPaper skips the edge noise where it cannot matter and works out the per-column / per-row noise once; tiles are
   copied where blending would give the same bytes; the paper strips, torn panels and the texts' outline / shadow /
   composite run on a small pool of 3 helper threads (`RowPool`).
5. **Live Background** (Options > Video): Auto / On / Off; Auto is off on integrated graphics and when the menu runs
   slow; on weak graphics (or a GPU over 6 ms a menu frame) the attract is drawn 30 times a second and at most 1080
   lines high (docs/menu_remake.md section 3).
6. **GPU timing without stalls**: the menu's GPU time comes from timer queries read back later (no glFinish /
   glReadPixels outside screenshots and the developer perf switches; the live attract's still is read once per model
   source / blood colour, as before).

### Results (ms; mean (most) over the page's frames; "open" = the page's first frame)

1920 x 1080, live attract on:

| Page | CPU before | CPU after | Open before | Open after |
|---|---|---|---|---|
| Main menu | 1.12 (18.8) | 0.33 (5.3) | 24 | 15 |
| Options: Game | 0.72 (10.5) | 0.32 (4.8) | 198 | 53 |
| Options: Video | 1.02 (11.1) | 0.47 (5.4) | 59 | 21 |
| Options: dropdown | 0.86 (33.2) | 0.57 (18.9) | 36 | 17 |
| Chapter Select | 0.85 (25.2) | 0.33 (14.1) | 178 | 52 |
| Boss Mode | 0.51 (26.3) | 0.33 (14.4) | 182 | 55 |
| PC Mode | 0.43 (12.2) | 0.41 (20.4: the attract loading scenery) | 96 | 59 |
| High Scores | 0.17 (0.5) | 0.27 (0.6) | 149 | 44 |
| **Jukebox, playing** | **3.85 (48.6)**, 781 KB uploaded a frame | **0.73 (22.2)**, 663 KB | 274 | 96 |

2736 x 1824 (the Surface's screen), live attract on:

| Page | CPU before | CPU after | GPU (5080) after | Open before | Open after |
|---|---|---|---|---|---|
| Main menu | 1.56 (27.5) | 0.42 (7.6) | 0.24 | 52 | 18 |
| Options: Game | 0.92 (13.6) | 0.38 (6.3) | 0.30 | 226 | 60 |
| Options: Video (scrolls) | 1.56 (61.5) | 0.83 (26.8) | 0.28 | 71 | 24 |
| Options: dropdown | 1.31 (63.4) | 0.53 (28.8) | 0.30 | 48 | 21 |
| Chapter Select | 0.78 (31.5) | 0.37 (17.2) | 0.39 | 237 | 64 |
| Boss Mode | 0.57 (32.1) | 0.36 (16.7) | 0.31 | 219 | 63 |
| High Scores | 0.11 (0.3) | 0.27 (0.6) | 0.30 | 212 | 61 |
| **Jukebox, playing** | **5.22 (65.6)** | **0.87 (29.2)** | 0.47 | 349 | 113 |

Every page's mean CPU is now under 1 ms (target 2 ms). The worst frames left are actions that build a new page (a
dropdown opening, a settings list scrolling, the Jukebox's list page turning): 17-29 ms on this CPU.

Frame pace: before 16.4-21.5 ms between frames (mean; 16 ms sleep + the frame, uneven), now the display's beat (the hidden
test window: the 66 fps fallback limiter, 15.2 ms).

**Integrated graphics** (`HOTD_GPU_CLASS=weak`, 2736 x 1824; the GPU column measured on the 5080): the menu's GPU time
a frame with the live attract on at 60 a second 0.24-0.47 ms; On on weak graphics (30 a second, 1080 lines) 0.11-0.27
ms; Auto there = off (the still) 0.07-0.20 ms. An Intel UHD 620 has roughly 1/30-1/40 of the 5080's memory bandwidth
and much less fill rate (the menu's 2D layers are bound by those): **estimated** 10-19 ms a frame with the live
attract at full rate (not 60 fps), 4-11 ms with On (30 a second, 1080 lines), about 3 ms with Auto / Off: steady 60
fps. Not measured on a real UHD 620 / the Surface: please check (docs/testing_checklist.md section 1a).

### optui (2026-10-26): scrolling and dropdowns

`HOTD_FE_STYLE=remake HOTD_FE_BENCH_ACT=30 HOTD_FE_DD=4 hotd --frontend-shot x.png --frontend-screen 3 --size WxH`
times the Video page's actions (a frame each, the GPU layers' CPU part, as the menu): the rows scrolled by the wheel
and a dropdown opened / closed. Three changes:

1. **rText's pictures kept** (`textCache_`): a text starts on a whole pixel; its finished picture (shadow, outline,
   letters) is kept by text / size / colour / look and copied wherever the same text goes again (its wear pattern now
   counts from the text's start, so it is the same picture anywhere; it looks the same).
2. **Partial static layer** (`staticWs_`): a settings page built again redraws only the part where widgets changed
   (the widgets there, clipped to it); another page / size / colour: all of it.
3. **The lit row's part kept by its look** on a settings page (was: by the widget's address, new at every build).

| | 1920 x 1080 before | after | 2736 x 1824 (2160 x 1440 page) before | after |
|---|---|---|---|---|
| Rows scrolled (a frame) | 14.2 ms | 4.5 ms | 18.7 ms | 4.7 ms |
| Dropdown open / close | 11.4 ms | 3.9 ms | 16.2 ms | 4.8 ms |

("before": this branch with the crosshair preview, before the three changes.)

### Left

- (done, leftovers: the Jukebox's list page turning redraws and uploads only the page number's part: 7.6 -> 3.2 ms at
  2736 x 1824; the GPU's widget texture now takes any redrawn part alone, TexSubImage2D.)
- The Gallery's first open (~190 ms: its models load) is unchanged.
- The Classic style still composes on the CPU each frame (its 640 x 480 page; not part of this pass).

## Low-end pass (misc18, 2026-10-27)

User: "the Remake menu is laggy on my Surface Pro 7 (Intel Iris Plus); the game should run smoothly on weak hardware."

### How it was measured

The menu tour (above) now also opens the **Firing Range** and the **Language** page (the Mods page is Options' Mods
tab) and has a **fill** column: the screens' worth of pixels the menu's layers shade a frame (an integrated GPU's cost
is mostly fill and memory traffic). Every run on **2 CPU cores** (the process's affinity set to cores 0-1, nothing
changed in Windows), Live Background Auto, a settings folder whose Jukebox lengths and background stills were already
made (so no measuring / capture workers ran during the tour). `HOTD_GPU_CLASS=weak` = the Surface's path. This PC
has no integrated GPU and the GPU's clock cannot be lowered without system settings, so GPU times are the RTX 5080's
(an Iris Plus is roughly 20-40x slower: the fill numbers are the guide). Gameplay: `--render --size WxH --chapter N
--frames 6000` on 2 cores.

New testing switches: `HOTD_MENU_MASK_CHECK=1` (with the tour: each page's 60th frame drawn again with the whole
layers and the two pictures compared byte for byte; `HOTD_MENU_MASK_PNG=<dir>` keeps them when they differ),
`HOTD_MENU_GPU_LOG=1` (GPU frames over 3 ms with the slide / live state), `HOTD_MENU_GPU_ADD=ms` (the Auto tier
sees a slower GPU).

### What was slow

1. **Menu Frame Rate Auto = 30 fps on integrated graphics** (optui): on the Surface the whole menu ran at 30
   pictures a second (the hand, the focus, scrolling): this alone is the "laggy" feel. Its GPU time at 2736 x 1824
   with Auto's still background is ~0.1 ms on the 5080, an estimated 2-5 ms on an Iris Plus: 60 fps fits.
2. **Fill**: every frame the backdrop, the splatter (two during its cross-fade) and the widgets' layer were each drawn
   over the whole picture (3-4 screens of pixels at 2736 x 1824 = 15-20 million blended pixels a frame), though most
   of the splatter's and much of the widgets' layer is empty.
3. CPU per frame is small (0.1-0.9 ms a menu frame on 2 cores, gameplay 2D + 3D + HUD ~0.6 ms); the CPU costs left are
   page opens (50-240 ms at 2736 x 1824 on 2 cores) and the first-run capture of the background stills (Live
   Background off / Auto on weak graphics: the attract runs unseen once, ~30-60 s).

### What changed (the pictures are the same: the mask check, 20 pages at 1280 x 720 and at 2736 x 1824: **0 bytes
differ** on every page)

1. **Auto tier** (main.cpp `menuFpsAuto`, `menuTierCap`): Auto starts at 120 (a graphics card) or **60** (integrated;
   was 30) and steps down by what this PC measures, not by its name: the menu loop's GPU time (timer queries, read
   later, no stall) and its late frames over 180-frame windows (the slides' unseen capture left out): at 120, over
   6 ms or a third of the frames late -> 60; at 60, over 12 ms or a third late -> 30. Only down, for this run; the log
   says `menu frame rate: Auto: the menu's GPU .. ms a frame, .. of 180 frames late: N fps`. A set rate (30 / 60 / 120)
   is never changed. Test: `HOTD_MENU_GPU_ADD=14` (weak, 2736 x 1824): stepped to 30 after the first window.
2. **Tiles** (hud.cpp `maskUpdate` / `maskRects`): the widgets' and splatter's layers keep which 32 x 32 tiles have a
   pixel (updated with the uploaded part only); each layer's quad is drawn scissored to those tiles and their
   neighbours (the filter's reach), the scissors cut along the same tile edges (no pixel blended twice, no gap). The
   same quad, so the same bytes.

### Results (2 cores; ms a frame, mean (most); fill = screens of pixels shaded a frame)

2736 x 1824, integrated-graphics path (`HOTD_GPU_CLASS=weak`, Live Background Auto = the stills):

| Page | Frame gap before | after | Fill before | after | CPU before | after |
|---|---|---|---|---|---|---|
| Main menu | 30.9 (30 fps) | **15.4 (60 fps)** | 3.0 | 2.1 | 0.65 | 0.82 |
| Options: Game | 30.9 | 15.4 | 3.0 | 2.9 | 0.40 | 0.45 |
| Options: Video | 30.9 | 15.4 | 3.0 | 2.4 | 0.49 | 0.50 |
| Options: Mods | 30.9 | 15.4 | 3.0 | 2.7 | 0.27 | 0.38 |
| Chapter Select | 30.9 | 15.6 | 3.0 | 2.4 | 0.30 | 0.36 |
| Jukebox (playing) | 30.9 | 15.4 | 3.0 | 2.6 | 0.42 | 0.50 |
| Firing Range | 30.9 | 15.4 | 3.0 | 2.8 | 0.44 | 0.51 |
| Language | 30.9 | 15.4 | 3.0 | 2.3 | 0.32 | 0.37 |
| Gallery viewer | 30.9 | 15.4 | 0.3 | 0.25 | 0.04 | 0.07 |

("before" fill: backdrop + splatter + widgets, each the whole picture, 3.0; the counter is new, so it is worked out,
not measured. The hidden test window's fallback limiter gives 15.4 ms at 60.) GPU on the 5080: 0.05-0.18 ms a frame
before and after (too fast to show the fill). Page opens unchanged: Options 143 ms, Chapter Select 150, Jukebox 240,
Gallery 210 (at 1280 x 720: 80 / 80 / 130 / 200).

1280 x 720, a graphics card (live attract on): CPU 0.2-1.0 ms, GPU 0.07-0.6 ms, fill 2.1-2.8 (was 3.0), unchanged
otherwise.

Gameplay (arcade models, 2 cores, unchanged by this pass; for the record): CPU a frame 2D under 0.23 / 0.14 ms, 3D
0.09-0.17 / 0.06-0.11 ms, HUD 0.34-0.38 / 0.25-0.28 ms (1280 x 720 / 2736 x 1824); GPU wait 0.6-1.4 / 0.4-0.5 ms on
the 5080 (Chapters 1-4, 6000 frames each).

**Estimate for the Surface Pro 7** (not measured on it): the menu at 60 fps with ~1.5-4 ms of GPU a frame and under
2 ms of CPU; Auto drops to 30 only if the measured GPU time passes 12 ms.

### Left

- (done by earlyz, below: Arcade Sort no longer writes `gl_FragDepth`, the same pictures.)
- Page opens (50-240 ms on 2 cores at the Surface's size): a whole page drawn on the CPU and uploaded.
- The first-run capture of the background stills (Live Background off / Auto on weak graphics) runs the attract
  unseen at up to 1080 lines every frame for ~30-60 s: drawing only the frames a still is taken from would cut it,
  but the view's per-frame state makes those stills not certainly the same.
- The Firing Range with the live attract: one 50 ms frame when it opens (the attract's world loading meanwhile).

## Early depth test for Arcade Sort (earlyz, 2026-10-27)

Draw Depth "Arcade Sort" (the default) gave every arcade polygon its sort key through `gl_FragDepth`, written by
every pixel: that turns the GPU's early depth test off, so hidden pixels were shaded too.

### What changed (render/renderer.cpp, the vertex shader and Renderer::draw)

- The key now comes from the vertex depth: all three corners get the same clip z = c x w (c from the 16-bit key
  only), and the arcade draws use the depth range 0 .. 2^-8, the key at (key + 0.25) / 2^24. So close to 0 the float
  error of z x w / w, of clipping and of the interpolation is under 0.02 of a depth step, and every pixel of a
  polygon stores the same 24-bit value: ties stay exact (the reason depthcmp moved to `gl_FragDepth`). Depth clamp
  and the near plane behave as before (the same clip z as the old path).
- The item's place (7 bits) no longer fits beside the key, so the depth-writing passes draw the arcade items
  last-first instead (items past 127 first, in list order, as they shared place 127): on a key tie the earlier
  model, now drawn later, wins by LEQUAL, inside one model the later polygon, as before. PC / pack models keep
  their places and order (their own ties: a first try that reversed them too changed the PC door leaves' seam).
- `HOTD_SORT_FRAGDEPTH=1` keeps the old path (also `HOTD_SORT_EXACT`). New measuring: `--render` prints
  `PERF: gpu_3d` (a timer query around the 3D) and, with `HOTD_FS_COUNT=1`, `fs_screens_3d` (the 3D's fragment
  shader runs a frame, in screens of pixels: ARB_pipeline_statistics_query).

### Same pictures

`--render --size 1280x720`, every 30th frame, old path against new, pixels compared: **0 pixels differ in 6,924
frames**: arcade models Chapter 1 (566 frames), 2 (666), 3 (666), Chapter 4 `--strict --default-route` with the
ending (1165), the title / attract (`--title`, 399); the same five runs with `--models pc` (3,462 frames). The 21
depthcmp places lie in these runs (their exact frames were not written down).

### GPU time (RTX 5080; 6000 frames a chapter, gpu_3d mean ms, old -> new)

| | Ch 1 | Ch 2 | Ch 3 | Ch 4 |
|---|---|---|---|---|
| 1280 x 720 | 0.084 -> 0.080 | 0.071 -> 0.066 | 0.070 -> 0.065 | 0.067 -> 0.065 |
| 2736 x 1824 | 0.377 -> 0.355 | 0.321 -> 0.296 | 0.301 -> 0.280 | 0.306 -> 0.295 |

Fragment shader runs (3000 frames, screens a frame, mean / p99, the same at both sizes): Chapter 1 2.74 / 5.3 ->
2.60 / 4.2, Chapter 2 2.68 / 6.5 -> 2.48 / 5.0, Chapter 3 2.27 / 4.3 -> 2.13 / 3.5, Chapter 4 2.43 / 3.3 ->
2.37 / 3.2. So 3-8 % less GPU time on average, up to ~25 % less shading in the heaviest frames: the early test
only skips what something already drawn hides, and the game's list is not drawn near to far.

### Left

- ~~A depth-only pre-pass~~: done (prepass, below).

## Depth-only pre-pass (prepass, 2026-10-27)

### What changed (render/renderer.cpp Renderer::draw, the fragment shader's prepassDiscards)

- The opaque pass is drawn twice: first depth only (colour writes off, `uPrepass`: the shader runs only its discards
  (checker, culling, see-through texels, the PC's far distance, pack pictures' alpha) and stops), the same items,
  order, uniforms and depth ranges; then the colours with depth test EQUAL and no depth writes. The depth buffer ends
  as before; only the fragments at the final depth are shaded and the last drawn of them wins, as with LEQUAL (alpha
  is 1 in this pass, so the later one covers). The sky pass, decals and blended passes are unchanged.
- Both Draw Depth modes. Not with `HOTD_SORT_FRAGDEPTH` (no early test there) or the debug views (ids, tie).
- **Auto by the graphics** (main.cpp `depthPrepassAuto`, the Live Background's `weakGpu` names): on with integrated /
  software graphics, off on a graphics card (on the RTX 5080 it costs a little in Chapters 1 and 4 with Arcade Sort,
  below). `HOTD_PREPASS=0 / 1` and `HOTD_GPU_CLASS=weak|strong` decide instead; the log says "3D depth pre-pass: on/off".

### Same pictures

`--render --size 1280x720`, every 30th frame, `HOTD_PREPASS=0` against `=1`: **0 pixels differ in 6,924 frames with
Arcade Sort and 0 in 6,924 with `--draw-depth buffer`**: Chapters 1-3, Chapter 4 `--strict --default-route` with the
ending, the attract `--title`; arcade models and `--models pc` (the earlyz scan).

### GPU (RTX 5080; 6000 frames a chapter, gpu_3d mean ms and fs_screens_3d mean, off -> on)

| Arcade Sort | Ch 1 | Ch 2 | Ch 3 | Ch 4 |
|---|---|---|---|---|
| 1280 x 720 ms | 0.082 -> 0.086 | 0.068 -> 0.065 | 0.067 -> 0.063 | 0.067 -> 0.072 |
| 2736 x 1824 ms | 0.359 -> 0.361 | 0.299 -> 0.274 | 0.285 -> 0.242 | 0.297 -> 0.309 |
| fragment shader runs (screens) | 2.60 -> 3.41 | 2.13 -> 3.17 | 2.23 -> 3.26 | 2.07 -> 3.37 |
| of which the opaque colours (on) | 0.81 | 1.02 | 1.02 | 1.29 |

| Depth Buffer | Ch 1 | Ch 2 | Ch 3 | Ch 4 |
|---|---|---|---|---|
| 1280 x 720 ms | 0.082 -> 0.082 | 0.066 -> 0.065 | 0.066 -> 0.060 | 0.067 -> 0.066 |
| 2736 x 1824 ms | 0.344 -> 0.341 | 0.281 -> 0.265 | 0.266 -> 0.235 | 0.285 -> 0.275 |
| fragment shader runs (screens) | 2.54 -> 3.25 | 2.00 -> 3.00 | 2.09 -> 3.11 | 2.00 -> 3.01 |

The runs count both passes: the depth-only pass runs the cheap shader about as often as the old opaque pass (~2.1-2.6
screens), and the full shader now runs ~0.8-1.3 screens for the opaque colours (Arcade Sort, measured by skipping that
step; key ties are shaded once per tied polygon) instead of ~2. On the RTX 5080 the full shader is cheap, so the extra
vertex work and draw calls about cancel it: -4 to -15 % in Chapters 2-3, +1 to +7 % in Chapters 1 and 4 with Arcade
Sort. On integrated graphics (fill rate the limit) the saved shading should count far more: not measured here (no
integrated GPU on this PC); check on the Surface Pro 7 (docs/testing_checklist.md).

## More PCs: Mesa, GL debug output, Safe Mode, a 2-core run (compat, 2026-10-29)

Only an RTX 5080 here; a friend's Radeon 610M showed flicker (fixed: invariant `gl_Position`) and a crash on a
resolution change (fixed: NvOptimus / PowerXpress exports). To catch more driver differences without other GPUs:

- **Mesa llvmpipe** (mesa-dist-win 26.2.4, `opengl32.dll` + `libgallium_wgl.dll` next to a copy of hotd.exe,
  `GALLIUM_DRIVER=llvmpipe`; also with `MESA_GL_VERSION_OVERRIDE=3.3 MESA_GLSL_VERSION_OVERRIDE=330`: a plain 3.3
  driver). 38 frames at 960 x 540 (Chapters 1-4, the Chapter 4 ending, the attract; arcade and PC models; pre-pass
  off and on) against the RTX 5080: at most 106 pixels per frame differ by more than 32 levels (single texel-edge /
  polygon-edge pixels: rounding, nothing missing or wrong), 3,519 pixels in all > 8 levels. Pre-pass on = off on both
  drivers (0 pixels). Every shader compiles on Mesa with no message. The menu tour (every Remake page, live attract
  on and off) and Chapters 1-4 runs with `HOTD_GL_DEBUG=1`: 0 errors, 0 warnings after the fixes.
- **`HOTD_GL_DEBUG=1`**: a debug context; KHR_debug messages and the shader logs into hotd.log (the first 3 of each
  kind, then counted; a summary at the end). Found and fixed:
  - NVIDIA, every frame: texture units 5-7 (the texture-pack samplers) empty without a pack (undefined for integer
    samplers in 3.3): complete 1x1 spare textures there now (`Renderer::makeSpareTextures`; units 1, 2, 4 too when a
    set has none).
  - NVIDIA, 137 times in the menu tour: the Remake backdrop's texture drawn with no picture (the dark fill while the
    live attract loads): a 1x1 black picture from the start.
  - Mesa, 64,000 times in the menu tour: the generated skins rewritten each frame into a `GL_STATIC_DRAW` buffer:
    the stage buffers are `GL_DYNAMIC_DRAW` (NVIDIA still keeps them in video memory; Chapter 1 1280 x 720 frame time
    0.74-0.81 ms before and after, the same pictures).
  - The backdrop's dithering used a `sin()` hash of the pixel position, imprecise on some GPUs (patterns): an integer
    hash.
  - Left (notes, not problems): NVIDIA's "pixel transfer synchronized" (screenshots' glReadPixels) and "vertex shader
    recompiled based on GL state" (once per program).
- **`--gl-check`**: no game data needed: compiles the start-up shaders, draws the CRT filter, fails on a GL error or a
  debug problem. In CI on Mesa (.github/workflows/release.yml).
- **Pre-pass off by default** on every GPU (`HOTD_PREPASS=1` forces it on).
- **Safe Mode** (`--safe-mode`, `Play (Safe Mode).bat`): a 1280 x 720 window, Live Background Off, menus 60 fps,
  vsync, no CRT filter, no texture packs.
- **Fallbacks**: a CRT-filter framebuffer the driver refuses turns the filter off (logged). The log names the driver
  (`graphics: OpenGL ...`) and warns below 3.3.

### Low-end run: 2 CPU cores (start /affinity 3), 1280 x 720, Live Background Off (RTX 5080)

No texture budget exists (the stages hold ~720 MB of process memory at most).

| Run | Frame mean | p99 | Worst 1 % | Result |
|---|---|---|---|---|
| Chapter 1, arcade models (`--render`, 16,400 frames) | 1.37 ms | 2.97 ms | 4.71 ms | chapter 1 ended |
| Chapter 1, PC models | 0.84 ms | 2.64 ms | 3.07 ms | chapter 1 ended |
| Chapter 4 strict + the ending | 1.15 ms | 5.36 ms | 10.0 ms | the ending is over |
| Menu tour, Live Background Off (21 pages) | slowest page 5.9 ms CPU (main menu: its first frames) | | | 65.8 pictures / s |
| Menu tour, Safe Mode | slowest page 1.4 ms CPU | | | 65.4 pictures / s |

Game logic stays at ~0.04 ms a frame; the 2D layers and the GPU wait are most of the frame. Two cores are plenty.
On Mesa llvmpipe (all CPU, 3-4 threads) the game is playable but slow: 10-35 ms a frame at 960 x 540, the menu
35-43 ms a frame at 1280 x 720 (a software driver is the worst case, not a target).
## DirectX through ANGLE (angle, 2026-10-29)

Video > Renderer: DirectX (ANGLE) for graphics without OpenGL 3.3 (docs/renderer_backends.md). Its costs, RTX 5080:

- **Shaders:** ANGLE compiles HLSL through d3dcompiler: 1.3-1.9 s per view (OpenGL ~0.1 s). Kept by ANGLE's blob
  cache in memory and in `angle_shaders.bin`: the next view and the next start 13 ms.
- **Frames** (`--render`, Chapter 1, arcade models, 992 x 768, 16,100 frames, two runs each): OpenGL 1.09-1.12 ms
  mean (p99 3.2-3.4), DirectX (ANGLE) 2.59-2.61 ms mean (p99 4.4-4.8): ANGLE's translation adds ~1.5 ms a frame of
  CPU here, far inside a 16.7 ms frame; old CPUs will feel it more.
- No GPU timer queries on ES: the menu tour's GPU column and Menu Frame Rate Auto's GPU time read 0 there (Auto then
  decides by the late frames alone).
