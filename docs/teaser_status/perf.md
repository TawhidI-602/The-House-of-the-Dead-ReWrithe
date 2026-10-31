# perf (branch `claude/perf`, build `build-perf`, 2026-10-16)

Measured with `hotd --frames 1500 --bot 1 --render --mute --models arcade|pc` (RTX 5080, 992x768). `--render` now prints
the process memory after the view's set-up (`render: view ready in N ms (ws_mb=.. ws_peak_mb=.. commit_mb=..
commit_peak_mb=..)`) and a last line `MEMORY: ...` (working set / committed, current and peak, MB).

## 1. Memory

| source | before: working set (peak) | before: committed (peak) | after: working set (peak) | after: committed (peak) |
|---|---|---|---|---|
| arcade | 2529 (4482) MB | 3647 (5932) MB | 591 (748) MB | 726 (879) MB |
| PC | 2747 (4482) MB | 3931 (5932) MB | 450 (560) MB | 555 (642) MB |

`GameView::init` 4.3 s -> 0.54 s (arcade), 4.0 s -> 0.39 s (PC). Pictures at frames 830 / 1400: arcade identical pixel
for pixel to the robust-2 build; PC differs by at most 8 levels on a few % of pixels (fix-world-2's later main changes).

What it was:
- The arcade loader built every model once per texture set (Chapter 1: one copy per TEXCHG area), the copies differing
  only in the base colour from that set's polygon palette. Now one mesh (`Mesh::textureSet` / `variant` -1) and the
  renderer looks the base colour up in the drawn set's palette (`TextureSet::basePalette`, a 1024 x 1 R16UI texture per
  set, `uSetPal`; `Vertex::color` keeps the preferred set's for the debugging lists).
- `Renderer::upload` copied every vertex into one more CPU array before `glBufferData`: now `glBufferSubData` per mesh.
- After the upload `GameView` frees the CPU vertices (`Mesh::releaseVertices`, `vertexCount()` / `triangleCount()` still
  answer) and the texture pages; kept: the generated skins (rebuilt every frame) and everything with
  `HOTD_LIST_DRAWN_AT` (its lists read the vertices).
- Model source PC: the arcade stage is still loaded (placements, draw zones, names) but no longer layered or uploaded
  (it is never drawn there); it is drawn only if the PC stage fails to load.
Not done: loading only the IDs Chapter 1 uses (no longer needed for memory; all 1..0x1FFF are still built, ~0.1 s).

## 2. The first PC frame
`Hud::setPcDir` loads the PC sprites (SPR.RBH + BIGGER.VMC pages) at once (was: on the first frame drawn with PC
models). First frame before: 73-92 ms here (70-990 ms in robust-2's runs, cold cache / contention); after: PC run max
2.6 ms over 1500 frames (frame 14), mean 0.31 ms; the load (~35 ms warm) is now part of the set-up.

## 3. Start -> the first game frame
The launcher's Start (`enterGame`) now logs `start: N ms (regions, sound, game, view and HUD)`; headless runs print
`start: regions .. sound .. game ..` and `view: arcade stage loaded in .. ms, the rest ..` (`--arcade <zip>` takes the
launcher's files-check path). Warm cache, 3 runs each, `--arcade private\hotd.zip --frames 3 --render`:

| source | before | after |
|---|---|---|
| arcade | regions ~1 + sound ~40 + game ~25 + view 3.7-4.3 s = ~4 s | 1 + 35-45 + 24-32 + view 370-387 ms = ~0.45 s |
| PC | same + view 3.6-4.0 s + first frame 70-990 ms (HUD sprites) | 1 + 35-42 + 23-24 + view 376-387 ms (sprites included) = ~0.45 s |

The cuts are item 1's (the per-set mesh copies were most of the arcade stage's build time and of the upload; model
source PC no longer layers / uploads the arcade stage) and item 2's. What is left of the view's ~0.38 s: the arcade
stage ~90 ms (all model IDs 1..0x1FFF), coplanar layers + upload ~140-220 ms, the rest (shaders, animations) ~70 ms.
The files check on Start reads its cache (1 ms); the first check of a new ROM zip is not counted here.

## 4. Arcade blood colour (non-red bursts)
The bursts, drips and gore pieces (akabushu 0x7-0x23 from blood_burst, 0x46B-0x4A3 from blood_drip, nikubaan ...)
take their colour from polygon palette entry **1023**, not from the GGSET colour rows (rows 2-5, which recolour the
faces). maincpu 0x24E5C stores `[0xAB110 + 2 x blood_color]` there (formats.md §24: 0x801F red, 0xB340 green, 0xFC12
blue, 0xE01C purple); the loader had the effect record's 0x8018 for every setting. `GameView::draw` (arcade models)
now sends entry 1023 by the blood colour through the renderer's palette override (`setPaletteAnimation`, only when it
changes; `HOTD_NO_BLOOD_ENTRY` = the old look). Checked: all 810 fullgame_us MAME dumps (red) have 0x801F there, so
red bursts are now a little brighter (r 31 instead of 24), as in MAME. Pictures at frame 250 (bot seed 1, burst on the
first zombie): red / green / blue / purple bursts match the faces. UNVERIFIED (teaser): no non-red MAME capture.
Not changed: entry 984 (animated by 0x25310 in the original, the game does not run the palette animation yet; the
viewer does); blood pools (props types 27-29) stay red-only as in the original; PC models: fix-world-2's B_<colour>.VMC.

## Check after merging main
20,000-frame rendered bot runs (seed 7, 2 players, blue blood), both sources in parallel: both clear Chapter 1 at
16,308 frames, 0 GL errors, 0 spikes, 0 bad accesses. Arcade: draw mean 0.74 / p99 1.6 / max 11.8 ms (frame 0),
working set 589 MB (peak 747), committed 734 MB (peak 880). PC: 0.28 / 0.65 / 3.0 ms, 449 MB (567), 550 MB (648).

## Edits in other workers' files (kept small)
- src/main.cpp (t1): `memoryReport()` (the `render: view ready` memory and the `MEMORY:` line) and the `start:` timings.
- src/game/hud.* (hud): `setPcDir` loads the PC sprites at once (`loadPcSprites`).
- src/render/renderer.*, src/game/view.* (render / polish): the per-set palette texture, the upload without a copy, the
  CPU release after the upload, model source PC not uploading the arcade stage, palette entry 1023 by the blood colour.
- src/assets/arcade_loader.cpp (s1's ArcadeSource): one mesh per model, `TextureSet::basePalette`.

## Left
- The palette animation (maincpu 0x25310, ArcadePaletteFx, verified in the viewer) is not run in the game: entry 984
  and the pulsing lights keep their loaded colours.
- Loading only Chapter 1's model IDs (all 1..0x1FFF are still built; ~90 ms, no longer a memory problem).
- A cold-cache start (first run after boot) and the first files check of a new ROM zip were not measured.
