# Chapter 4's own content and the ending (worker "ch2proof", 2026-10-19 evening)

Branch `claude/ch2proof`. All read from the arcade disassembly (the decompilation as a guide), UNVERIFIED (teaser)
unless "Proven on A" below.

## Chapter 4 (src/port/ch4_kinds.cpp, props_ch4.cpp, decl/ch4_kinds.h, decl/props_ch4.h, cases/ch4_kinds_arcade.inc)

| Ported | Routines |
|---|---|
| kind 191 | `kind191_route_by_rescues` 0x4D50 (route_choice = every hostage rescued) |
| kind 22 | `event_model_fade` 0x31390 (model 0x765, scale / fade from its record, until flag 213) |
| kind 154 | `kind154_prop_group` 0x3B020 + `prop090_drop` 0x3F600 (five falling pieces, table 0xAF8D0) |
| kind 25 | Curien's scene body: `kind25_kyurianb_curien` 0x87F10 and its nine states 0x87F80..0x881C0 (flags 128-131) |
| prop sub-types | 97-101 `prop_r4_mechanism` 0x3F460 / `_run` 0x3F480; 104 `prop104_kakaaa`; 108 `prop108_pushukeke`; 120 `prop120_kami_region` |
| player scenes | `player_scene17_dash` 0x4DF50 (+ 0x4E040 / 0x4E0B0 / 0x4E100, the ret routines 0x4E0A0 / 0x4E150 / 0x4E270), `player_body_at_scene_pos` 0x4E160 (+ 0x4E220) |

`hotd --chapter 4 --strict --autofire --lives-kept 1 --mute --frames 40000`, default route (events 0 1 2 4) and the
branch (`--route 4:1`: 0 1 3 4), both model sources: every event up to the Magician runs with no guard and nothing
unported but `boss_magician` 0x70AB0 (worker "hermit"); it stalls there at WAIT_ITEM 215 (the boss's flag).

### Proven on A (worker "ch4proof", ch4p_full, 2026-10-19)

The Magician's capture (magician.md: play_us_20261007_2238 from boot, `-Every 20 -Max 150`) also took these routines.
All of them came out 0 differ, so no fix was needed; their UNVERIFIED marks are replaced by "Proven on A (ch4p_full: n
captures, 0 differ)":

- `event_model_fade` 16, `player_scene17_dash` 1, 0x4E040 5, `player_body_at_scene_pos` 1.
- Kind 25: `kind25_kyurianb_curien` 1, `_wait_scene` 34, `_wait_point` 35, `_pointing` 5, `_wait_struck` 64,
  `_struck` 6, `_collapse_a` 3, `_collapse_b` 18, `_collapse_c` 2, `_wait_end` 18.
- Props: `prop_r4_mechanism` 1, `prop090_drop` 150, `prop104_kakaaa` 150, `prop108_pushukeke` 54,
  `prop120_kami_region` 1.
- Re-captured with sizes (ch4p_size: `3b020:140`, `3f480:180`, `4e220:50`, `-Every 3`): `kind154_prop_group` 1,
  `prop_r4_mechanism_run` 150, 0x4E220 134 captures, **0 differ**. The ch4p_full runs (1 / 1 / 0) do not count:
  the capture had no size for these routines ("0 ret").
- Not reached: 0x4E0B0 / 0x4E100 (player 2 of two), the ret routines 0x4E0A0 / 0x4E150 / 0x4E270. `kind191_route_by_rescues`
  0x4D50 was not in the list. All of these stay UNVERIFIED.

**The Chariot's DEVIATION** (`chariot_hit_reaction_end` 0x99210, below): capture ch4p_chariot (the same recording to
1300 s, every call: Chapter 1's fight and Chapter 4's rematch) has 28 captures, 0 differ. The ported routine goes on
with the drop when the Chariot is above the floor, which the original does not. 0 differ therefore means **the
recording never reaches the deviation's case**: no hand hit lands while the Chariot drops in. The routine is
otherwise proven as the original.

## The ending (src/game/ending.cpp, runtime)

After Chapter 4's clear `game_chapter_end_wait` goes on as the original: chapter + 1, sub-mode 7 =
`game_ending_start` 0x35160 (chapter slot 6, event 0 / 1 / 2 by who plays, task group 0xAC280; the world is
Chapter 4's: tables 0xC0BB0 / 0xC0BD0 slot 6), sub-mode 8 = `game_ending` 0x352A0 (the group; at the event list's
end rank_insert -> the name entry / ranking / end screen, `game_ending_rank`). Ported for it: `sky_task_ending`
0x34150, kinds 105 (caption), 110 (letterbox; the colour-curve fade-in not drawn), 111 (texture area), 116 (end
sprite), 120 / 122 (window open / close; 122 picks the branch: a top rank 1, a score ending in 0 2, else 0), 121
(sets its routine only), 179 (the results card), the characters' bodies (kind 103 set-ups 0x58C40 / 0x59090 / 0x59660 /
0x59A80 and their frames), `player_st7_idle` 0x55B0.

`hotd --chapter 4 --ending` (developer: Chapter 4's end count at its last frame) plays the ending to its end, 1 and 2
players, each branch (`--route 7:3=0` / `=1`, default 2), also with `--strict`: events 0 / 2 -> 3 -> 4 / 5 / 6, end
at frames 6728..7522, then the rank / end screen. `--render`: Rogan's closing scene, the staff roll over the
mansion scenes, the results card (GL errors 0).

## Compared with MAME (2026-10-19, after kind121)

`kind121_ending_scene_draw` 0x35410 ported (5f1d636). Side by side with fullgame_us snaps 724-786 (one every 2 s; ours
at the same time from the ending's start): Rogan's closing scene (camera, motions, captions), the staff roll (scenes,
credit sprites, the letterbox split, timing within a second), the final mansion shot and GAME OVER match by eye.

## The ending's look (worker "endlook", 2026-10-19)

- Closing room colours (160a097): texture_area_load's second argument is not a colour set but a cover sprite
  (0x34FF0: `sprite_place(0x1004000, .., b)` when b != -1); the colours come from the area alone (0xA9960 /
  0xA99A0 by area). `game_ending_start` 0x35160 loads no area, so the closing room (the Magician's room) keeps
  the one Chapter 4 left: event 4's script TEXCHGs area 12 (seen in a played `--chapter 4 --strict` run: areas
  4, 10, 9, 12, then the ending's own 4 9 8 7 6 5 3 1 2). Only the developer `--ending` start was wrong (it left
  Chapter 4's first area, 4: the sepia floor); it now loads area 12. Against fullgame_us 727 / 734: grey wall with
  the emblem, dark machinery, blue-grey floor match. Chapters 1-4 unchanged (no view / runtime change).
- PC staff roll (8ba75a8): THOTD.EXE's `game_ending_start` 0x473CD8 loads banks 0x5DC / 0x577 / 0x5EA (the
  arcade's 0x1D5 / 0x16F / 0x1E5; no bank for the arcade's 0x26E); its kind120 0x473E90 draws 0x5D466C[n] =
  0x5DC + n at the n-th opening (counter 0x5D4648): the PC's own 14 credit sheets (with the PC staff: lead
  programming, QA / PC engine). The arcade's n-th opening draws 0xAC2A0[record +0x70], records 0 1 2 7 8 3 4 5 6 9
  13 11 10 12; sprite_map_pc.inc maps the arcade's n-th sheet to the PC's n-th (kind122's close redraws 0x1E1 = the
  last, -> 0x5E9 director, as the PC's last). The digits 0x1E5 -> 0x5EA; 0x1E0 / 0x1E3 -> none. The caption
  sprites 0x16F.. -> 0x577.., END 0x1E4 / 0x270 -> 0x666 / 0x665 and 0x106 -> 0x50F were already the PC's (its
  kind105 0x4747EC / kind116 0x474894 load those). The results card (0x177 / 0x179.. / 0x1B5.. -> 0x57F.. / 0x5CA..)
  is the PC's art; the PC's own kind179 0x51ADDC only sets 0xD9F716 (not traced further).
- Shots (main checkout private\endlook\, not committed): `compare_fix.png` (MAME 727..784 | arcade | PC at the same
  times), `fix2_pc_roll.png` (all 14 PC sheets in order, then cleared), `areas.png` (areas 10 / 3 / 11 / 12 tried).
  Not compared with the real PC under the harness (reaching its ending needs a full play).
- Checks: `--input-test` 524 / 524; `--frames 40000` arcade and PC: chapter 1 ended at 16695; `--chapter 4 --ending`
  arcade and PC: the ending is over at 7522.

## Not done

- The palette curve fade-ins (0x18B50 / 0x189C0) and the window lines (hardware) are not drawn: no visible difference
  in the compared snaps.

## The ending's look, round 2 (worker "endlook")

- PC closing room hidden by a wall (26419ca): with the PC world file (pcAlt, chapter byte past 4) every WORLD6
  object was drawn at once (none is in the arcade's placement table, so the zone cull let them all through): the
  staff roll's scenes (0x907.., 0xCEB.., 0x1239.., ...) stood around the closing room, a brown wall in front of the
  camera. THOTD.EXE's world_init 0x491CD0 takes the placement table / zone lists 0x5E9658 / 0x5E963C by chapter byte
  (slot 6 = the same entries as the arcade's 0xC0BD0 / 0xC0BB0: 0x1116.., 0x1D1D / 0x1D1F, ...) and its world task
  draws only the draw zone's entries; the ending's zone 0 has none (the room is game-drawn: 0x1D1D / 0x1D1F, rebased
  as before). view.cpp now draws the alternate world file by the zone's table entries (`HOTD_PC_ALT_ALL=1`: old).
  Shots `private\endlook\wall_pc.png` (frames 360..6000: Rogan in the PC's closing room, then the staff roll). The
  attract / prologue (WORLD5) with `--title`: pixel-identical to before at 11 frames 400..5900.
- "Late timing" (END at MAME 784 where ours shows GAME OVER): not a timing gap. The snaps are 2 emulated seconds
  apart = ~114 game frames (57.13 game frames / s: sound_board.md), not 120. Counted so, MAME 784-785 END (~6860-
  7090), 786 GAME OVER, 787-788 results, 789 name entry; ours END from 6920 (kind105), GAME OVER by 7200, results
  7300: within a snap.

## Not done (round 2)

- The real PC's ending under the harness: not compared (needs a full play to reach).

- PC model source: the world is WORLD6 (fine), but the ending's 2D sprites (credit sheets 0x1D5.., the results) map to
  other PC pictures (blood / claw sprites over the staff roll); the PC's own ending pictures not traced.
## Gate (worker "ch4gate", 2026-10-19): playable flag ON

- Bot soft-lock (seed 3, 2P, both sources: event 0 WAIT_ENNUM for ever): Chapter 4 starts the Chariot's fight while it
  still drops in (action 2); a hit on its weapon hand in the air (reaction 19) ended the drop and the script walked it
  on 78 units above the floor, never reaching its step distance. The arcade code (0x982B0 / 0x99210) has no guard;
  chariot_hit_reaction_end now goes on with the drop (marked DEVIATION). MAME snaps 565-566: the arcade's Chariot
  lands out of view and fights on the floor. Headless --chapter 4 --strict --autofire (whose shots hit it in the
  air) now ends Chapter 4 at 37983 and the ending at 44867 (was 33258 / 40142); --route 4:1 43936 / 51458.
- Bot aim (src/game/bot.cpp isDecoy): the iron ball's hit-box trail / the ball, and the armoured Chariot's body
  (shots only spark) are left alone; one-player bots then get past event 0. The watchdog's soft-lock prints the
  shootables.
- 	ools\robust_seeds.ps1 -Extra --chapter,4,--bot-gameovers,40, seeds 1-32 (both sources, 1 / 2 players, some
  --strict): 0 crash / soft-lock / bad access / NaN; 7 clears (3, 10, 14 strict, 23, 28 one-player, 30, 31), each on
  through the ending to its end (ending events 0 / 1 / 2 by players, then 3, then 4 / 6). The bots' rescues never
  picked the branch; --strict --route 4:1 plays it to the end on both sources.
- --render (both sources): 0 GL errors. Chapter 3 -> 4 (now without --continue-chapters): 27279 / 59088, ending
  65972. Chapters 1 / 2 / 3 unchanged (16695 / 17270 / 27279); --input-test 524 / 524.

## The ending moved into the port and proven (worker "verify", 2026-10-20)

src/game/ending.cpp's routines are now `Game<P>` members (src/port/ending.cpp, decl/ending.h), read again against
the disassembly; the runtime keeps only sprite_init (its 2D layer) and its ranking flow (`game_ending_finish` ->
game_ending_rank). Dropped: the runtime's extra resets at the ending's start (flags 0x51E940.., the shot lists, the
chapter counters), which the arcade does not do there. Captures of the full recording to its end and of a fresh
two-player Chapter 4 game: everything **0 differ** but capture artifacts (proof.md, "verify" row). Not reached: the
bodies of player 2 alone (0x59660 / 0x596F0). Also proven: `kind191_route_by_rescues`, scene 17 player 2 (0x4E0B0 /
0x4E100). The ending still ends at 7522 (`--chapter 4 --ending`), 7366 with two players; Chapter 4's run 28083 / 34967.
