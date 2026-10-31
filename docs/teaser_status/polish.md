# polish: Chapter 1 checkpoints vs MAME / the real PC (overnight 2026-10-16)

Branch `claude/polish`, build folder `build-polish`. Not merged.

## Checkpoints (tools\checkpoints.txt, 18 moments, three runs)

`.\tools\checkpoints.ps1 -Build build-polish [-Runs main,branch,die] [-Sources arcade,pc]` -> `private\checkpoints\<stamp>\index.html`
(our arcade / PC shot, frame and event; the MAME snap of the same place; the real-PC harness capture where one exists).

- **main** (`--autofire --lives-kept 1 --default-route`: events 0 1 3 4 5 6 19, the route of the MAME fullgame_us
  replay): c01 title card, c02 opening (scientist), c03 the bite cut-scene, c04 event 1, c05 event 3 (courtyard gate),
  c06 / c07 event 4 (shoot three / the stairs), c08 event 5, c09 event 6, c10 Chariot intro (Sophie), c11 Chariot fight
  (life bar), c12 results.
- **branch** (`--default-route --route 1:0,1:3`: 0 2 3 9 10 14 19): c13 event 2, c14 event 9 (MAME: inflives1).
- **die** (`--bot 3`, no lives kept; game over at frame ~2800 in the window): c15 CONTINUE, c16 the game over scene,
  c17 the route map, c18 GAME OVER (MAME: life_scan, Japanese region).
- New: `hotd --checkpoints` lines `<name> mode <M> <S> <F>`; `--bot <seed>` in the window; checkpoint_sheet.py finds
  MAME references by the frame counter 0x51F428 in four dump sets (17 of 18 found; event 2 has none).
- Runs: `private\checkpoints\polish_run1` (before the fixes), `polish_run2` (after).

## Differences, ranked by how much a player notices them

| # | where | difference (ours vs MAME / real PC) | status |
|---|---|---|---|
| 1 | all play, all cut-scenes, both | Cut-scene bars inverted: black bars ~60 lines high at the top and bottom during ALL play (the HUD sat on black, the floor / ceiling cut off), none in the cut-scenes (MAME: 40-line bars with HOLD YOUR FIRE! / PRESS START in them). letterbox_task's scene_state was read the wrong way round; the bar panel's row height was 11 px, not 8 | **fixed** (polish 2) |
| 2 | every frame, both | The developer lines (P1 lives / frame / event / HUD note) over the top-left of the screen | **fixed** (polish 3): off at the start, F1 / `--dev-lines` |
| 3 | c16 game over scene, both | MAME: a 3D close-up of the fallen player on black for ~4 s (mode 5 sub-mode 1); ours keeps the frozen play view | open: the game-over scene's task / camera not ported (not cheap) |
| 4 | c12 results, both | MAME: the rescued researchers stand as 3D models inside the metal frame (RESCUED x 3, snaps 0171-0173); ours (RESCUED x 0 on this route) shows the low-priority room picture 0xA6 there; the PC frame has a researchers photo baked in | **fixed** (sparks 2): model 0x65E (a fixed group of nine, not per rescue) drawn by kind176; lit brighter than MAME |
| 5 | c17 route map, both | No walkers (the players' 3D figures walking the trail); MAME shows the walker at the route's end. PC: the map sprites 0x243 / 0x244 map to wrong PC pictures (the title card's scratches, "CONTINUE?" left over), no map | walkers open (fix-world-2: walker / map camera tasks not ported). PC: **fixed** (polish 6): 0x243 / 0x244 now draw nothing (SPR.RBH has no map picture) |
| 6 | c01 title card, PC | "HOLD YOUR FIRE!" drawn over the white chapter card (the PC 2D ignored the window masks; the arcade hides layer 0 under the card's window layer) | **fixed** (polish 4) |
| 7 | hits, both | MAME shows a large white / orange star hit spark at almost every hit (snaps 0029, 0056, 0058, 0079, 0155); ours rarely shows one (c09 has a fireball) | **fixed** (sparks 3): the star is shot_effect_flash at depth 1, cut by the near plane 1.0; now no near clip (depth clamp), as the Model 2 |
| 8 | c03 (+765), arcade | "HOLD YOUR FIRE!" missing in the bite cut-scene (MAME snap 0016 has it in the bottom bar; ours shows it at +300); after polish 2 the rest of the frame matches 0016 | open, minor: probably the caption's blink / timing |
| 9 | c10 / c11 Chariot, PC | Sophie's room floor near-white (point-sampled BIGGER.VMC texels) | known (fix-world 3): the real PC looks the same per render's harness run |
| 10 | opening, PC | The real PC plays a "1998.12.18" prologue before the chapter card (harness ticks 50-200) and fades the card in; ours starts on the card | open, low: PC-only intro |
| 11 | c11 Chariot room, arcade | The walls look darker than MAME's in the shots | not a bug (fix-world-2 1: pixel values equal) |
| 12 | MAME crosshair | MAME's snaps show MAME's own light-gun crosshair; ours none in autofire runs | not a game difference |

After the fixes (`polish_run2`): c02 / c03 / c08 / c13 / c14 now frame like MAME (bars in the cut-scenes; floor,
ceiling and HUD over the 3D in play). PC route map: black with the red trail (no PC map picture).

Not differences: c01 card = MAME life_scan's card (the US replay skipped it); c02 opening / c06 / c07 stairs scene, c15
CONTINUE? 9..0, c18 GAME OVER match; the scene timing differs because the replays were played by a person.

## Other workers' files touched (minimal)
src/game/clear.cpp (letterbox_task's letterbox values), src/game/hud.cpp (bar height, PC window masks), src/main.cpp
(checkpoint modes, bot in the window, `--dev-lines`), src/assets/sprite_map_pc.inc + tools/sprites_tool.py (0x243 / 0x244).

## Round 2 (polish2, 2026-10-16 afternoon; branch `claude/polish2`, build folder `build-polish2`)

Arcade models, `tools\checkpoints.txt` (now 30 moments: c19-c30 added for each event's middle, the opening's
courtyard, the zombie close-up, the Chariot's later phases), runs `private\checkpoints\polish2_base` (main before),
`polish2_run2`, `polish2_run3` (after). Our event 0 frame f = MAME fullgame_us frame counter f + 1015 (frame 304 matches
snap 0012 best); after the opening the MAME player's pace differs, so later references are the same place, not the
same frame. MAME snaps are 2 s apart: motion (8-frame sequences) was only checked on our side (title card, zombie
hits); dense MAME captures for motion are still to do.

| # | where | difference (ours vs MAME) | status |
|---|---|---|---|
| 1 | opening courtyard (event 0 ~1600, MAME 0023 / 0024) | Researchers' / zombies' feet sunk into the ground (user report). Not a height bug: MAME's floor height is -25 too (work RAM 0x51EA40); the researchers walk through hei01_03a's kerb, whose top is 2 units over the floor. The Model 2 sorts polygons (the ground first); our depth buffer hid the feet | **fixed** (0747d76): the game's models take their lowest 3 units' depth from up to 14 units nearer along the view ray. UNVERIFIED |
| 2 | every zombie close-up (event 1 +248, MAME 0028) | "Kawaii" huge pupils (user report): the zombie's white eyeball decal (zonbichana_kao 0x781, a translucent quad textured 448,992) lies up to 0.014 behind the face's black socket and was hidden but for a crescent. Not b830dd3 (flatten on / off identical). markCoplanarLayers missed it: its normal buckets (x 16) put eye and face two buckets apart | **fixed** (4597c2b): x 8 buckets + neighbours, decal layers 0.05 model units nearer along the view ray. PC models: eyes in the texture, unchanged |
| 3 | Chariot room (event 19, MAME 0147-0170) | Floor a magnified blur (window-glow texture) instead of ornate tiles; walls near black instead of orange wallpaper. Cause: the **Model 2 microtexture** (texture header 0 bit 12; MAME model2_3d_render / fetch_bilinear_texel level -1) not drawn | **fixed** (1a1f470): detail texture from the other sheet (header 2 bits 13-15), coordinates x 2^(2^minlod), blended up to ~50 % nearer than level 0 |
| 4 | Chariot room (MAME 0147 / 0155) | No shadows under Sophie and the Chariot: the room's floor is at 1.05, the shadows at the floor height 1.00 | **fixed** (f56c418): floor decals 0.3-unit dip pull |
| 5 | title card (all chapters, ~3 s) | A seam across the middle on every other frame (the film flicker's halves showed different pictures): the card writes the upper half after a raster wait, which the Model 2 shows from the next frame | **fixed** (fdefd37): the upper half's cells reach tile RAM at the next frame. Left: frame 2 shows the grey 0xA2 layer in the upper-left before the first upper write (1 frame) |
| 6 | c16 game over scene | 3D close-up of the fallen player not ported (polish round 1 #3) | open |
| 7 | c17 route map | walkers not ported (round 1 #5) | open |
| 8 | bite cut-scene / opening | HOLD YOUR FIRE! blink phase differs (ours on where MAME off at 0012, off at 0015 / 0016) | **fixed** (proofs-8): the blink runs on the frame counter from power-on; a direct chapter start now starts it at the MAME reference's 1015 (proofs-8.md) |
| 9 | every shot | MAME's white screen flashes (snaps 0032, 0102, 0117, 0122, 0156) are the light-gun shot flash; ours off by design (`--shot-flash`) | not a bug |
| 10 | c22 courtyard pond | pale flat pond: MAME 0051 the same | not a bug |
| 11 | all arcade scenes (user rule) | Arcade mode keeps the arcade's own colours and lighting: checked per fix against MAME snaps (0012 / 0023 / 0028 / 0147 / 0155: colour rows, luma, light parameters unchanged; microtexture blends shades before the luma / colour tables like MAME). Standing check for every later fix | checked (this round) |

Remaining to check: dense MAME captures (tools\replay_hotd.ps1 -Every 0.0174 -DumpFrom/-Seconds over ~8 frames) for
zombie hit / death / door motion; eyeball_pop_spawn is still a stub (part_blow_off ported and proven in proofs-8);
shadow_update ported (proofs-8); sophie_skin_draw stub.

Debug switch added: `HOTD_DUMP_WORLD=<file>` (with HOTD_LIST_DRAWN_AT): every drawn triangle in world space and the
camera position, for ray casts (what covers what).
Other workers' files touched: src/render/renderer.* and src/game/view.cpp (render's), src/assets/arcade_loader.cpp
(microtexture bits), src/game/sprites.cpp + runtime.h (hud's: the card latch), src/assets/assets.h (DrawItem).

## Round 3 (polish3, 2026-10-17; branch `claude/polish3`, build folder `build-polish3`)

Inputs: the full checkpoint set on main c55b722 (`private\checkpoints\polish3_base`, both sources, 30 moments, 29 MAME
references; after: `polish3_run3`), the user's play-test recording (2026-10-16
23-37-39.mp4, arcade models; frames read only, scratch copies) and a new depth-tie sweep: `hotd --render --shot-at F
...` with `HOTD_ZFIGHT=1` draws each shot twice more with every triangle's depth nudged 2e-6 (clip z / w) to a
pseudo-random side, then the other, marks the pixels whose winner changes (an area, not a crossing line;
zfight_f<F>.png) and lists the item pairs. `HOTD_COPLANAR_KEY=<hex>` lists one model's near-coplanar overlapping
triangle pairs with their plane offsets and layers.

| # | where | difference | status |
|---|---|---|---|
| 1 | event 19 end, arcade (user play-test 6:22) | Lying Sophie's green iris quads poke out of her face. The floor pull (feet4: a lying body, uFloorPull 3, up to 14 units nearer) and the decal layer's 0.05 pull were two projections; the layer's replaced the floor's, so the eye texture (layer 1) fell 14 units behind its iris quads (layer 0, flattenUnderTranslucent) | **fixed** (77e6528): one pull = floor + layer along the view ray. Frames 4060-4156 of `--event 19 --autofire`: irises inside the eyes |
| 2 | everywhere, arcade (user: "textures still flicker") | Depth ties inside models: the mansion facade's window panes (niwa01_03b / 03c, events 0-4), room_2b / room_2ari (event 5), room_n3a (event 6), room_5b (event 19 corridor), _dam chest layers. markCoplanarLayers bucketed planes by round(d x 4) (a 0.005 tilt 10 units from the origin moves d by 0.76) and tested overlap by centroids only | **fixed** (dd10105): normal buckets only, seven sample points a side, up to 0.1 in front. Default route every 30 frames (566 shots) + branch route (300): before 23 of 113 sampled frames tied (up to 344 px), now 5 frames with 1-5 px. Checkpoints 0-0.4 % changed |
| 3 | video one-frame blips (not bugs) | corpses blinking every other frame for 40 frames (enemy_corpse 0x76CB0, the original), rain, blood bits, the courtyard's moving fog bands, outlines in motion | not a bug |
| 4 | c15 CONTINUE | the checkpoint at +120 showed play: the die bot continues 20-1100 frames in | **fixed** (42fd483): shot at +15; polish3_run3 shows CONTINUE? 9 / LIFE -1 in both sources (the PC's MISS SHOT is magenta on the real PC too, pcsweep.md) |
| 5 | room edges / open doors, both (c09's left edge; lying Sophie's room's left half black at 4:3) | black / grey void where the next zone is not drawn | another worker (zones) |
| 6 | event 5 doors, PC | the two door halves 0x741 / 0x745 overlap at the seam: depth ties (~300 px, frames 7020-7080) | open (the PC's own z-buffer may do the same; not compared) |
| 7 | PC creature parts (0x596, 0x446, 0x7a2, 0x7b3, 0x444), the Chariot's blood spray 0x14 | small depth ties inside PC part models (1-200 px a frame); 0x14 is an animated spray | open, minor |
| 8 | hits, PC | spark look: PC white puffs with orange rims vs our orange ball (pcpolish2 lead) | open |
| 9 | opening, PC | the "1998.12.18" prologue + card fade-in (prologue.md: arcade attract mode 9) | open, large |
| 10 | c12 results | RESCUED x n from the first results frame; MAME 0171 shows the group first, the text from 0172 | open, minor (timing) |
| 11 | hits, arcade | checked frame by frame (event 1, frames 2129-2144): white-centred star, orange rim, streaks, as MAME 0029 / 0155 | not a difference |
| 12 | Sophie's shadow, arcade (c10, video) | hard black shadow polygons around her: MAME 0147 the same | not a difference |

Still open from rounds 1-2: dense MAME captures for motion; eyeball_pop_spawn / sophie_skin_draw stubs; PC title.pal /
menu.pal. Other workers' files touched: src/render/renderer.* (render's: pulls, layers, debug ties), src/main.cpp
(the HOTD_ZFIGHT shot pass).

## Dark doorways indoors (user play-test 2026-10-17): matches the original

MAME on the same route (route 1:0=1, 1:2=0, 1:3=0, 1:4=1, 1:7=0, 1:8=0) shows the same black areas through the doorways
as ours (the "Thank you! Take this." room; private\mame_dumps\zones_r4\compare_ours_top_mame_bottom.png). The original
world task 0x4B300 draws only the current zone (docs/teaser_status/zones.md). Not a bug.

## colors (2026-10-19): the grey / sepia cut-scenes on the arcade models

Branch `claude/colors`. Shots (never committed): main checkout `private\colors\` (`before\`, `after\` per scene;
`sheet_before\pairs_N.png` = the allch_20261019 contact sheet as ours | MAME pairs; `mame_grey1.png`,
`mame_grey_end.png` = the MAME sepia references).

**Cause.** palette_grey_build (0x1A400) does not recolour the picture: it rewrites the colour (gamma) table. For every
entry (base value 0-31, luma 0-63) of every channel c, `sepia[(curve_r + curve_g + curve_b)(entry) / 3 / 4][c]` (ROM
ramp 0xAA050) goes into the queue 0x517CB0, which the vblank uploads to the hardware colour table 0x1810000
(color_table_upload_all 0x1B0C0 / _part 0x1B1C0, a 0x80-byte row per base value); MAME's output curve
(`max((v - 64) * 255 / 191, 0)`, model2_v.cpp) still follows. So each channel keeps its own base colour (a red coat
stays a red-brown, a black shadow stays black: the ramp's dark end ~57,33,49 is under the curve's 64). Our shader
instead took the final, already-curved colour, averaged it into one grey level and looked that up in the ramp after
the output curve: everything a single mauve hue, blacks lifted to 57,33,49.

**Fix** (18fe2eb, src/render/renderer.cpp): with the grey look on, the arcade path builds each channel from the three
curves at that channel's own base value and luma, looks it up in the ramp, then applies the output curve. The 2D layer
is unaffected (its palette bank 2 is greyed separately, as before). The PC models keep their own look: THOTD.EXE draws
the grey scenes through BROWN.PAL (TextureSet::layersBrown), untouched.

| scene (frames, `--default-route --autofire --lives-kept 1`) | before | after vs MAME |
|---|---|---|
| Chapter 1 opening, kind 172 (182-243) | mauve, lifted blacks | MAME fullgame_us 0011: deep red-brown house, cream sky: matches |
| Chapter 1 end, Sophie (15911-16213) | mauve | red-brown, black shadows (no MAME snap inside the grey frames) |
| Chapter 2 end, Curien (16485-16787) | mauve, wall 59,34,49 | hangcmp mame_fight 0442: wall 18,0,0 = ours 18,0,0; window 160,129,92 vs 168,131,93 |
| Ending, END card (`--chapter 4`, 44194-44627) | mauve | fullgame_us 0784 / 0785 (Sophie's ending): deep red-brown with black, the same look |

Chapter 3 has no grey scene. Sweep of the rest of the contact sheet (Chapters 1-4, ending, ours | MAME): no other
lifted-black / washed-out difference; the remaining differences are timing / camera (different moment, different
event) not colour. The colour-table white fades (palette_blend_white: kind 104's title fade, the Magician's death)
are drawn as a white overlay; with the output curve affine above 64 that is the same result as blending the table
(255 - (255 - c) t through the curve = the curved colour blended to white by t), so no change there.

Checks: input test, Chapter 1 16695 (arcade and `--models pc`), `--chapter 2/3/4 --strict --autofire --lives-kept 1
--mute --default-route` 17270 / 27279 / 37983, ending 44867.
