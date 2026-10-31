# pcpolish4: the user's PC-model play-test findings (2026-10-19, branch claude/pcfix)

Build folder `build` in the worktree. Every run with `HOTD_PREF_DIR=<scratch>`, headless `--render` shots. Shots in
`private\screenshots\pcpolish4\` (not committed). Stopped early (priority change: Chapters 2-4 first).

## 1. Attract demo: G over plain black (FIXED)

User shots hotd_20261018_0210*_f1865..f2012 (the prologue, mode 9, chapter byte 5). Reproduced:
`--models pc --title --render --shot-at 1865/1946/2012`.

Cause: the visible room there is not a world placement but room_n1a 0xCF3 drawn by the game itself
(kind114_model_timed, arcade 0x16210, which sets the room's matrix: the model lands at (245, 58, 363) + its local
coordinates). The PC's WORLDn.RBH objects are stored placed already (world coordinates: 0xCF3 is 101..343 / 0..72 /
333..442), and THOTD.EXE's kind114 (0x440E6C) draws it with the camera matrix only. We applied the game's matrix to the
placed PC model, so the room sat a second offset away (x 346..588, z 696..805) and the camera saw nothing (debugging:
the PC zone items 0xCF5 / 0xCF7 were drawn, but lie behind the camera, which looks along -x).

Fix (view.cpp addModel): a game-drawn model that is a PC world-file object is placed relative to its arcade placement
(W x P^-1); logged once per ID ("PC world object ... drawn by the game"). In a Chapter 1 autofire run no other world
object is drawn by the game, so nothing else moves.
Also (THOTD.EXE pc_chapter_files_load 0x418AA0, bytes read): chapter 0-3 -> WORLD1-4, **4 / 5 (how-to-play, prologue)
-> WORLD5.RBH, 6 (ending) -> WORLD6.RBH**, palette stage%d.pal for 1..4 else STAGE1. PcSource::loadStage takes 4-6 and
GameView::pcAltFor draws that world file (and takes the game-drawn world objects from it) while the chapter byte is
4-6 (`HOTD_PC_NO_ALT_WORLD=1` switches it off). WORLD5 has 37 objects incl. 0x1D1D / 0x1D1F (not in the arcade table:
always drawn; far away at z 1800+, not seen in the shots).

Shots: `attract\pc_before_f*.png`, `pc_after_f*.png`, `arcade_f*.png`; `attract\sheet_pc_vs_arcade.png` (the whole
attract loop, frames 400..5900). After: the room with the pillar as in the arcade (brighter: the PC is unlit).
Not changed, seen: f800 the arcade shows the white fade where the PC picture does not; f4500 (mode 3) the PC floor
darker (same items drawn).

## 2. Chapter 1: floating skull, door "phasing through" (OPEN)

Shots (autofire, every 400 frames PC vs arcade, and PC every 100 in the mansion 6000-12700): `ch1\sheet_*`, `ch1\pc_*`,
`ch1\door_11900-12200_pc_vs_arcade.png`. Found so far:
- **Door at f12000** (on the way to the Chariot room): the PC shows door_n1a 0x163 (game-drawn, at 292, 1, 405) bright
  and whole; the arcade draws the same door but inside room5a_CT07 0xD1F, the model-list entry of world placement
  0xCE8 room5a_CT00a (list 0xBE440). The PC files have **no** 0xCE8 polygons (WORLD1's header points at an empty group
  list) and no 0xD1F (nor the clock 0xD12 tokei..): our PC view draws nothing for that placement, so the door stands
  in front of / through the walls that 0xD1F would give. Lead: what THOTD.EXE draws for 0xCE8's list (its world task
  with an empty table entry draws nothing; then the PC itself may show the door the same way: check the real PC).
- **Floating skull**: not found in the shots yet. Checked: no PC world object is drawn by the game in Chapter 1
  (so not the 0xCF3 kind of offset); every PC world object is in the arcade's placement table (zone culling applies);
  model bounds PC vs arcade (HOTD_CHECK_PC_IDS) for every ID drawn in 5500..12700: only animation-frame differences
  (<= 12 units). Leads: a skull prop (kind 128 sub-type 16, prop_skull_burst pieces 0x2CA / 0x2CB) or the kind 147
  table group (chairs, table, plates, a skull) with a support model missing in the PC files (gaps logged: 0xD03
  sekizob_a and pieces, 0x770.. tuubo, 0xBB5.. kashi, 0xB6C.. churun, 0xCE5 r1mado_02a) - ask the user for an F12 shot.
- Real PC capture for comparison: `pc_ch1\pc_t*.png` (run_pc_shots.ps1 -Script game_aim.txt -Ticks 15000 -Every 100,
  153 captures; not yet compared).

## 3. Ranking / name entry (FIXED, branch claude/pcrank)

Shots in `private\pcrank\` (not committed): `before_attract_pc|arcade` (f5700 / f5900), `before_ne_pc|arcade`
(name entry f7600-8200), after: `s4_attract_pc` (ranking), `after_ne_pc` (name entry, keys HOT<D -> "HOD").
Name entry reached with `--chapter 4 --ending --name-entry --ranking-file <table of ten 0-point entries> [--name-keys]`
(rank_insert at 7522, sub-mode 10 from 7523).

What the real PC does: THOTD.EXE keeps the arcade's ranking (0x44D288..) and name entry (0x4B3E0C..) code with its
own sprite numbers, but never runs them (the attract is gone; mode 4 sub-modes 9 / 10 are empty): its live ranking
is the front end's screen_high_scores 0x423038 on the desk picture 0xD2. The dead code is the reference for the
sprites; the PC files lack the ranking room 0x123B and every name-entry model (0xAC8-0xAF4). A harness capture of
the front-end Ranking screen was tried (menu script, cursor polls): it landed on Options / PC Mode, not repeated.

Causes and fixes:
- **Letters** (TXG for TAG): the thumbnail map took the PC 'X' 0x5BC for the ranking font's 'A' 0x22C; the small font
  0x1E6..0x22A (1ST / 2ND / 3RD, nTH on the name entry) matched wrong glyphs. From the PC's tables (font 0x44B070,
  name entry's 0x5EB + rank / 0x60F / 0x603, ordinals 0x5EDD60 = arcade 0xC5530 + 0x405): 0x1E6..0x22A -> +0x405,
  0x22D..0x230 -> 0x5C4..0x5C7, 0x22C -> 0x5A5 (the PC table's 0x5C3 is a tiny boxed 'A'). sprites_tool.py OVERRIDES,
  map regenerated (only those entries changed).
- **Rank cards** missing: 0x231 + card is drawn at y + 0xAA518[card] (down to y - 25), a tall sprite with the card in
  its lowest rows, bottom row first. The PC cell layer kept a sprite only at its top-left cell: above the screen it
  was dropped, and the next card's clear rect took the rest. pc_cells_put: the cards sit at their first picture row
  (clear only from there); any sprite with its top above the screen sits at row 0 with its upper rows clipped. The PC
  card art is at the arcade's size (224 x 232): Hud::drawPc2D clips such pictures in 8-pixel cells.
- **Cut sky** behind the rows: no PN_room2_01a in the PC files. With the PC models the ranking now stands on the desk
  picture 0xD2 (the PC's own high-score screen's, = arcade 0x11F), put in the PC 2D layer 2 under the table
  (ranking_header_task); the next mode's 0xA1 cover clears it.
- **Name entry** showed the backdrop and title only (the 3D letters, timer, underscores have no PC polygons):
  name_entry_model_draw records each piece's projected place (Runtime::pcNameModels) and the HUD draws PC sprites there:
  carved font 0x5A5.. (and . - ? !), menu arrows 0xD8 / 0xD9 (yellow when highlighted), END in carved letters, timer
  in the ranking digits 0x59B.., the underscores as bars, the highlight as a frame. UNVERIFIED (teaser): no real-PC
  picture of this screen exists.

Fixed (smallfix, c540781): the arcade name entry showed the ending's Chapter 4 hall behind the letters (`before_ne_arcade`): its placement table is still the ending's, which GameView took for another stage's world (view.cpp extraWorld_); now only the texture set is taken, no world. Not changed:
the "1P" mark (0x125 -> PC 0x5C8) sits half off the left edge as the arcade's cell 0.

## Checks (after the fix)
`--input-test` 524 checks, 0 failed; `--frames 40000 --autofire --mute --lives-kept 1` (arcade and `--models pc`):
"chapter 1 ended" at 16695; `--chapter 2 --strict --autofire --lives-kept 1 --mute --frames 40000 --default-route`:
chapter 2 ended at 17162.
