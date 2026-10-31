# pcsweep: magenta / missing models and textures with PC models (branch claude/pcsweep, build-pcsweep)

## LIFE UP! (0x3BB / 0x3BC) magenta: the real PC draws it so (no change)

- The PC model (CREAT2.RBH) is one textured quad, group head 0xA3040001 (kind 0x2004, double-sided, alpha 0xFF), page
  589 of BIGGER.VMC: 0x3BB uv (161..254, 229..250), 0x3BC (161..254, 205..226). Page 589 holds MISS SHOT / LIFE -1 /
  LIFE UP! twice each: a blue set and a pink-magenta set on a magenta (0x7C1F) key. 0x3BB = the pink one.
- THOTD.EXE draws it plainly: hostage_gift_fly 0x4483DC / gift_effect (the 0x3BB copy before it) -> geo_draw_model,
  no tint; pc_mesh_polygons_draw 0x578BB0 -> the textured-quad driver entry (0x577600 form): diffuse 0xFFFFFFFF,
  texture = the record's page. Through STAGE1.TBL / PAL the letters' texels go to palette 193 (218,0,255) and
  235 / 250 (lavender highlights): what ReWrithe draws.
- **Proof, real PC** (harness run, game_aim.txt, 4400 ticks; kept in private\screenshots\pc_look\lifeup, compressed):
  pc_t03727 / pc_t03757 = LIFE UP! over the bridge researcher in the same magenta with lavender highlights;
  pc_t01300 = MISS SHOT (magenta) / LIFE -1 (orange) the same. ReWrithe bot 1 frame 3760 and bot 5 frame 12640
  (MISS SHOT / LIFE -1) match. The arcade's red / blue is the arcade's look; PC mode keeps the PC's.

## Sweep (PC models, whole chapter)

- New: pc_loader logs `PC texture gap: page N is not in <file> (drawn magenta)` for a page a PC model uses that the
  VMC lacks (the only path that draws the magenta fill).
- Runs (offscreen `--render`, a shot every 40 frames, 14000 frames each, `--models pc`): `--bot 1`, `--bot 2 --route
  1:0,1:3`, `--bot 5 --bot-players 2`, `--autofire --default-route`; every shot scanned for magenta pixels
  (R, B >= 190, G <= 70). Results: no texture gap; magenta only on the PC's own LIFE UP! / MISS SHOT models (real-PC
  look above). The `PC model gap` lines are the known ones (render.md "PC gaps": shadows `_DD`, item_kira, tokei,
  kyochuu, kashi / churun, banhi, mizuzuzu, namkou, ball_*, tuubo break frames ...; not in CREAT1 / CREAT2 / WORLD1,
  checked by ID: the PC draws nothing there either).
- Static check of all three PC files (a script over every mesh): no untextured polygon indexes past its mesh's colour
  table; group kinds only 0 / 2 / 0x2001 / 0x2004 (all handled); no unknown kind.
- Rest of the chapter: `--bot 1 --event 13` to the clear (events 13, 19, boss; 8032 frames): no texture gap, magenta
  only on MISS SHOT. Every `PC model gap` ID of all five runs checked against the three PC files' object tables:
  absent, or present with an empty polygon list (0x365 / 0x366 / 0x461 / 0x462 / 0x790 / 0x791 / 0x7B1 / 0x7B2 in
  CREAT1, 0xCE8 / 0xCFD in WORLD1); none is a loader miss.

## Left
- Nothing magenta / missing to fix in Chapter 1 with PC models. Not compared: the LIFE UP! item's on-screen size
  against the real PC (different moments in the two captures).
