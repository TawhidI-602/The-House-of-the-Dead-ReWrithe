# The Remake menu's blood splatter

The blood around the Remake-style menu is not a picture: the game draws it itself when the menu first opens, from a
handful of rules and seeded random numbers. This page explains how, step by step. The code is
`Frontend::rSplatter` in `src/game/frontend_remake.cpp` (with `tint` / `bloodColour` above it and the three tiles in
`drawRPanel`).

The layout follows the HOTD Remake's menu (heavy at the top corners, thinner down the sides, runs from the top,
almost nothing in the middle). We do not copy its artwork; every shape here is generated.

## 1. Random numbers that never change

Everything comes from one hash function:

```cpp
float rnd(int x, int y, int s)   // 0..1, always the same for the same (x, y, s)
```

`rnd(blob, 7, 741)` means "random number 7 of blob number `blob`, in the family 741". Because it is a hash and not a
running generator, the splatter is identical every time and on every PC, and changing one part does not reshuffle
the rest.

Two smoother helpers are built on it:

- **value noise** `vnoise(x, y, s)`: random numbers on a grid of whole numbers, blended smoothly in between. Looks
  like soft blurry blotches.
- **fbm** ("fractal" noise) `fbm(x, y, s)`: three value noises added, each twice as fine and half as strong. Gives
  blotches with smaller blotches on their edges, which is what natural stains look like.

```
vnoise:  ░░▒▒▓▓▒▒░░          fbm:  ░▒░▒▓▒▓▓▒░▒░
         (smooth hills)              (hills with bumps on them)
```

## 2. A thickness field, shaded once

The splatter is not painted shape by shape. Every shape writes into four per-pixel buffers, and one final pass turns
them into colour:

| Buffer | Meaning | Combined by |
|---|---|---|
| `cov` | how much of the pixel is covered (0..1, soft on the edge = smooth edges) | the largest |
| `hgt` | how thick the blood is there (0 thin film .. 1 thick pool) | the largest |
| `gls` | how glossy (wet highlight allowed) | the largest |
| `ton` | the shape's own shade (dark red .. near black) | taken from the thickest shape |

Each shape is described by a **signed distance** `f`: how far inside its edge a point is (in menu units, 480 to
the screen's height). The cover is `clamp(f * S + 0.5)` where `S` is pixels per unit, so the edge fades over exactly
one pixel at any resolution: crisp, never jagged.

Thickness inside a shape is a "dome": 0 at the edge, rising to 1 a little way in, then flat:

```cpp
auto dome = [](float f, float r) { const float q = clamp01(f / r); return 1 - (1 - q) * (1 - q); };
```

```
thickness   ___________
           /           \        small r: a flat pool with a steep rim
__________/             \_____  (that is where the light and the wet look come from)
```

## 3. The shapes

### Stains (the smeared corners)

For every pixel: how close it is to the top-left corner (an ellipse 26 % of the width wide, half the height tall), to
the top-right one, and to the left / right edge. The closest one is the "base". Then noise is added and the result
cut off with a short ramp:

```cpp
m = base + (fbm(x * 0.025, y * 0.025) - 0.5) * 0.9 + (vnoise(x * 0.16, y * 0.16) - 0.5) * 0.22;
cover = clamp01((m - 0.26) / 0.14) * 0.78;
```

The big fbm makes the stain's ragged, cloudy outline; the fine noise mottles it. The stains are fairly thick (dark)
and never glossy: dried smears.

### Blobs (the impacts)

15 blobs placed by hand as fractions of the width / height (so they frame the panel on every screen shape), each
with a size, a direction its spray flies, a spread and a number of runs:

```cpp
{W * 0.035f, H * 0.06f, 17, 0.75f, 0.7f, 2},   // x, y, radius, spray angle (radians), spread, runs
```

A blob's ragged edge: its radius changes with the angle around the centre, read from fbm along a circle:

```cpp
const float a = std::atan2(uy - cy, ux - cx);
const float rr = r * (0.62f + 0.62f * fbm(cos(a) * 1.4f + 7, sin(a) * 1.4f + seed, 721));
f = rr - distance;
```

```
        .-~~-.           noise around the angle pushes the edge in and out
     .-'      '~.
    (    blob     )~.    plus 3-7 "lobes" (small round discs on the rim)
     '-.,      .-'       for the bulges a real splash has
        '~~--'
```

Big blobs (radius 5+) are glossy, small ones barely.

### Spray (droplets thrown from a blob)

About `3 x radius + 6` droplets per blob, mostly along the blob's direction (plus or minus its spread). The further a
drop flew, the smaller it is and the more it is stretched:

```cpp
d  = r * (1.05 + q^1.6 * 4.5);            // distance (q random 0..1)
dr = (0.35 + rnd^2 * r * 0.26) * (1.15 - q * 0.6);   // size
el = 1.0 + q * 1.7;                         // stretch
```

Each drop is a teardrop pointing away from the blob: thin tail at the back, fat head at the front (the way a flying
drop lands):

```
  blob  ●   ·  ∙ ━●   ━━●        further = longer tail, smaller head
```

### Fine spatter

520 tiny drops not tied to a blob: 60 % from the two top corners, 30 % thrown in from the sides, 10 % sparse from the
bottom corners. Distance from the source is random but squeezed towards it (`rnd^1.7`), so they are dense at the
corners and thin out towards the middle. Same teardrop shape, stretched away from the source.

### Runs (drips)

From some blobs (1-3 each) and 6 from the top edge. A run is a wobbly vertical line:

- its centre wanders sideways with value noise (a few units) and leans a little;
- its width goes from `w` at the top to a third of it at the bottom (`w * (1 - 0.68 * t^0.7)`);
- it ends in a **bead**: a small round disc a bit wider than the tail, glossy - the drop that collects at the end of
  a real drip.

```
   ███
    ██
    █         thinning
     █
     ●        the bead
```

## 4. The shading (one pass)

For each covered pixel:

1. **Colour by thickness**: thin = lighter red `0xA0180E`, mid `0x5A0604`, thick = dark red `0x2E0302`, or with the
   shape's tone nearly black `0x140101`. Thin edges are lighter, cores are dark, like real blood.
2. **Opacity by thickness**: `cover * (0.42 + 0.55 * thickness)`. Thin film is see-through, pools are solid.
3. **Wet highlight** (only where glossy): the slope of the thickness gives a surface direction (a "normal"); light from
   the upper left, a sharp highlight (`pow(..., 48)`) where the surface faces it. Because blobs are flat with a steep
   rim, the highlight is a thin bright crescent on their upper-left rims and on the beads.
4. **Blood Color**: the colour goes through `tint()` (below).

## 5. Placement: framing the panel

Everything is placed in fractions of the screen: the corners and the outer ~12 % on each side get almost all of it,
the middle stays clear for the panel and the logo (which are drawn over it anyway). On a 4:3 or 3:2 screen the same
fractions put it closer to the panel; on 21:9 further out.

## 6. The vignette

Separate from the blood: the background (`rBackground`) darkens towards the corners (a cached per-width table,
`1 - 0.6 * ((distance - 0.35) / 0.85)^1.4`). The splatter sits on top of it.

## 7. Blood Color

`bloodColour()` picks the menu's blood colour from the Blood Color setting: red, green, blue, purple, yellow (PC), or
the custom colour / a texture pack's `[recolour] blood` (brightest channel scaled to red's level). `tint(c)` then
splits any of the menu's reds into "a white part" (the smaller of green and blue) and "an amount of red", and lays
that amount on the blood colour instead. So the dark cores, light edges and highlights keep their shading, only the
hue changes. Red is left exactly as it was.

It applies to the splatter, the panel's red bottom, the arcade smear under it, the claw marks, the handprint switches
and the focus bars. Changing the setting redraws the cached pieces at once (the tiles are keyed by the colour).

## 8. Your own pattern (the seed)

Every player gets their own splatter. The pattern comes from one number, the **seed** (`splatSeed`, saved in
launcher.ini as `menublood=`). The first start picks a random one. Clicking the backdrop (anywhere outside the panel
and the buttons, strips, tabs and rows), pressing a pad's right stick (R3) or the **B** key makes a new one: the old
pattern fades into the new one in 0.35 s with a wet hit sound (the PC's cue 32, the arcade's blood burst).

What the seed changes, so every pattern still frames the panel:

- the noise families (`rnd` / `fbm` / `vnoise` third argument) are shifted by `seed * 131`: different stain edges,
  spray, fine drops and runs;
- each of the 15 blobs moves a little around its place (up to 2.5 % of the width, 3.5 % of the height), grows or
  shrinks by up to 22 %, turns its spray by up to 0.35 radians and gets one run more or fewer.

The hand-placed rules (the corner stains, the blobs' table, the fine drops' regions) stay. Seed 0 is the original
pattern (`HOTD_FE_SPLAT=0` for the PNG tool).

## 9. Rendering: on the GPU, like a modern menu

The menu is not one picture any more. It is a few layers the graphics card puts together every frame
(`Hud::drawRemake`):

```
 backdrop      the live attract's texture (liveattract: drawn by the GPU, nothing uploaded) or the hall picture;
               a small shader makes it grey and dark, tints it (settings pages) and adds the vignette, per window pixel
 splatter      a texture drawn once per pattern / colour / size, on a worker thread (Frontend::splatterLayer, 50-90 ms)
 widgets       panel, logo, paper strips, tabs, rows, prompts without the focus (a texture, redrawn on a new page)
 focus parts   the strip / row under the focus or hand and the help line, each a small texture of its own
 hand          the bony hand's sprite
```

- The widget and splatter layers are drawn at **min(window height, 1440) lines** (2 to 3 pixels a menu unit, in
  quarters): 1080 lines on a 1080p screen (the text at the screen's own resolution), 1440 on the Surface's 1824. The
  GPU scales them with linear filtering and mipmaps (premultiplied alpha, so no dark fringes). The splatter is soft,
  so its bilinear scaling cannot be seen.
- A frame with nothing new costs almost no CPU: no pixel loops, no upload. The live attract's picture never leaves the GPU. A focus
  change draws only the parts under the focus.
- Measured in the game (hidden window, 120 frames, `HOTD_MENU_PERF=1`; `HOTD_MENU_CPU=1` for the old way):

| | Old (CPU composing) | New (GPU layers) |
|---|---|---|
| 1920 x 1080, settings page: CPU a frame | 5.19 ms (most 12.2) | 0.02 ms (most 0.13) |
| 1920 x 1080: CPU + GPU (glFinish) | 5.66 ms | 0.20 ms |
| 2736 x 1824, settings page: CPU a frame | 5.55 ms (most 14.3) | 0.02 ms (most 0.11) |
| 2736 x 1824: CPU + GPU | 5.87 ms | 0.23 ms |

  A focus change: 2-10 ms once (its parts drawn at the higher resolution). The shaders are a few lines each (one
  texture read, no loops): fine on Intel UHD.
- **Blood Color in the backdrop**: the settings pages' tinted backdrop follows the colour (`backdropTint`): red is
  exactly the old tint (red x 1.5 + 14, green and blue x 0.55); another colour keeps the same dark grey part and lays
  the red's excess on that colour. The main menu's backdrop gets the same tint at 30 % (a faint cast; red: a faint red).
  The current tab's label and the other red letters follow the colour too (`paperInk`: on the cream paper a light
  colour such as yellow is darkened to stay readable).
- **Idle**: the Remake main menu left alone for 5 s fades away in 0.6 s (panel, logo, prompts, splatter, hand) and the
  backdrop shows lighter with a gentler vignette; any input brings the menu back at once and does nothing else.
- The PNG tool (`--frontend-shot`) and the Classic style still compose on the CPU (the same layers, the same maths).

## Tunable constants

| Where | Constant | What it changes |
|---|---|---|
| stains | `0.26f`, `0.24f` (ellipse widths), `0.5f`, `0.46f` (heights) | how far the corner stains reach |
| stains | `0.045f` | how wide the side bands are |
| stains | `* 0.9f` on the fbm | how ragged the stains are |
| stains | `(m - 0.26f) / 0.14f` | where the stain stops and how sharply |
| blobs | the `blobs[]` table | where the impacts are, their size, spray direction, number of runs |
| blobs | `0.62f + 0.62f * fbm` | how irregular a blob's edge is |
| blobs | `dome(f, r * 0.28f)` | flat pool (small) vs rounded drop (large) |
| spray | `r * 3.0f + 6` | drops per blob |
| spray | `1.0f + q * 1.7f` | how stretched far drops are |
| spatter | `520`, the 0.31 / 0.6 / 0.76 / 0.9 splits | how many fine drops and where they come from |
| runs | `6` top runs, `30 + ... * 130` | how many and how long |
| runs | `0.68f` | how much a run thins out |
| shading | `0xA0180E`, `0x5A0604`, `0x2E0302`, `0x140101` | the thin / mid / thick / darkest colours |
| shading | `0.42f + 0.55f * ...` | how see-through the thin parts are |
| shading | `48.0f`, `0.7f` | the highlight's sharpness and strength |

## Experimenting

Draw the menu to a PNG without opening the game (needs the PC files; `HOTD_PREF_DIR` keeps your own settings safe):

```
set HOTD_PREF_DIR=%TEMP%\hotd-test
set HOTD_FE_STYLE=remake
set HOTD_FE_BLOOD=1            (optional: 0 red .. 4 yellow, or a colour like 3060FF)
hotd.exe --pc C:\Games\HOTD --frontend-shot splat.png --size 1920x1080 --frontend-screen 1
```

`--frontend-screen 10` draws a settings page (Video); `HOTD_FE_BENCH=300` also prints the frame times (the old
CPU way and the GPU layers' CPU part) and a new splatter's time; `HOTD_FE_SPLAT=<n>` picks the pattern. Change a
constant, rebuild, shoot again.
