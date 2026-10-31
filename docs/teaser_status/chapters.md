# Worker "select": chapter select, chapter start / continue for Chapters 2-4 (2026-10-17)

Plan: [chapter2_3_plan.md](../chapter2_3_plan.md), worker 1 "select". Branch `claude/select`.

## What is there

- **One table:** `src/game/chapters.h` `kChapters[4]` (`playable`, a note). Chapters 1 and 2 on (Chapter 2 since
  2026-10-19, worker "ch2play", [ch2_kinds.md](ch2_kinds.md)); 3, 4 off. The release
  commit of a chapter flips its flag once its gate passes (`--strict` bot games, both sources, 1-2 players, clear).
- **Launcher:** a line "Chapter  < 1 >  2  3  4  (grey: not finished yet)": a playable chapter is picked by a click or
  Left / Right; the others are drawn grey (a second overlay over the same layout, `Launcher::greySpans_`) and do
  nothing. `launcher.ini` `chapter=1-4` (an unplayable one read from the file falls back to 1). Another chapter than 1
  starts straight away (no title / attract: the title's start button is Chapter 1's, as on the arcade).
- **Command line:** `--chapter N` (1-4): developer, any chapter; a warning on stderr when it is not playable.
  `--continue-chapters` (developer): a clear goes on into the next chapter even when it is not playable.
- **Chapter to chapter** (game_chapter_start 0x25D40 / chapter_load 0x25DA0: the port's since worker chstart,
  src/port/game_flow.cpp, capture-proven): at the end of the clear's count (`game_chapter_end_wait`) the next chapter
  is loaded when it is playable: chapter + 1 and sub-mode 1 (as the arcade, one frame without play), then
  game_chapter_start: players 1 / 2 in play go 5 -> 2 (their task is then player_st2_rejoin: PlStartInit's row 2, a
  full gun and 90 frames without hits; lives and score stay), chapter_load at the event NEXTEV left (the route's
  start event of the next chapter: heap, the 2D screen and sprite banks, character bodies, texture area, the task
  group 0xAB210, camera / shot lists / counters, the prompts) and sub-mode 2. The runtime's side
  (`Runtime::game_chapter_start` / `Runtime::chapter_load`): 4P seats 3 / 4 get a full gun, the 4P bodies, PC Mode's
  task counts, `chapterChanged` for the host. Else: as before (end screen "CHAPTER n CLEAR / Chapter n+1 is not
  finished yet", or the name entry).
- **Host:** `Runtime::chapterChanged` -> the run's chapter (`runChapter`: a game over restarts that chapter, not
  Chapter 1); the window re-inits the `GameView` whenever the game (mode 4) is in another chapter than the view
  (`viewChapter`); headless `--render` re-inits its view too (`gChapterChanged`).
- chapter_load for any chapter was already general (`Runtime::chapter_start`: texture area table 0xAB0B0, world_init's
  zone / placement tables by chapter, the event lists, the sky task); `GameView::init(chapter)` loads the stage's world,
  texture sets, colour rows and sky (arcade and PC). Not done (engine side, as for Chapter 1): music by chapter is the
  event scripts', sprite banks (0xAB240 by chapter: the HUD's 2D layer loads its own).

## Headless results (2026-10-17, `--autofire --lives-kept 1 --mute --default-route`, cut guards on)

| Run | Result |
|---|---|
| Chapter 1, arcade / PC | clear at frame 16695 (unchanged); input test 518 / 518 |
| `--chapter 2` arcade / PC (+ `--render`) | events 0 1 2 3 4 5 7 8 9 15, chapter end at **54752**; no crash, no bad access, GL errors 0 |
| `--chapter 3` arcade / PC (+ `--render`) | events 0 1 3 6 7, chapter end at **67349**; no crash, no bad access, GL errors 0 |
| Chapter 1 `--continue-chapters --render` | clear 16695 -> Chapter 2 starts (event 0), plays on (events 1, 2, ...) |
| `--chapter 2 --continue-chapters --render` (PC) | Chapter 2 end 54752 -> Chapter 3 starts, its world drawn (frame 57000), GL errors 0 |

Both chapters reach their end **only through the cut guards**: every WAIT_ENALV / WAIT_ENNUM / WAIT_ITEM that waits on an
unported object is passed after its time-out (about 1,300 frames each), so the runs are long (55-67 k frames) and the
bosses (Hangedman `hangedman_fight_init`, Hermit `boss_hermit`, Curien) are not fought: their item flags (Ch 2: 192-195,
199; Ch 3: 110, 125) are set by the guards. The worlds draw (screenshots at frames 600 / 1250, both sources: Chapter 2's
courtyard gate, Chapter 3's lab with Taran's window).

**The work queue** (every unported routine / stub / guard, calls per 1,000 frames, first frame):
[ch2_boundaries.md](ch2_boundaries.md) (67 entries: 22 unported routines, 8 stubs, 37 guards) and
[ch3_boundaries.md](ch3_boundaries.md) (73 entries: 19 unported, 9 stubs, 45 guards). Top of Chapter 2: kind 183
(the route sign), 133, 136, 168, prop067 / 064 / 014 / 078, `prop_flag_swap`, hostages s07-s10, kind 132, 075, 131,
`billboard_model`, the Hangedman. Top of Chapter 3: kind 153 Taran, prop121 / ch3 machine / 094 / 096, the iron ball
(`iron_ball_follow` + 4 stubs), `prop_flag_swap`, kind 186 / 189 / 188 / 106 / 150, hostages s12-s14, Curien, the Hermit.
`debug_halt` "enemy_guard halted" at the first hostage scenario of each chapter (s07, s12): the guard's action table
reaches an unported scenario (the runtime removes the task; no crash).

## Next (for the chapter workers / integ)

- A chapter's flag goes on in `kChapters` only after `--strict` clears (no guard passes) in bot games of both sources.
- `--strict` runs of Chapter 2 / 3 stall at the first guard today (expected).
- Done (worker chstart, 2026-10-21): the chapter change is the arcade's (players 5 -> 2, chapter_load in full), moved
  into the port and proven against MAME captures of chapter changes (docs/proof.md, the chstart row).
- UNVERIFIED (teaser): the view re-init per chapter (only checked headless).

## Update 2026-10-19 (worker "ch2play"): Chapter 2 playable

Chapter 2's gate passed: `--strict` reaches the chapter end (the Hangedman killed) on the default route and the three
branch routes, arcade and PC models, 1 and 2 players, no guard and no unported routine or stub (details and frames:
[ch2_kinds.md](ch2_kinds.md)). Its flag is on: the launcher offers it, and Chapter 1's clear (still frame 16695) goes
on into Chapter 2 (players kept), which clears too (end 36961 in the bot game). The table above (2026-10-17) is history.

## Check numbers after the chapter change port (worker chstart, 2026-10-21)

The arcade spends one frame in sub-mode 1 (game_chapter_start) between a chapter's end and the next one's play; the
runtime used to start the next chapter in the same frame. That frame shifts the autofire's 15-frame rhythm in every
later chapter, so the headless numbers of Chapter 2+ legitimately change (checked: with the chapter started in the
same frame again, every number comes back exactly, so the players' 5 -> 2, the 90 frames without hits and the
rest change nothing in these runs). Chapter 1 (16695, both model sources), Chapter 4 `--strict` (ending over at
34967), 4P (17785 / 39736 / 64434 / 92642, ending 100008), PC Mode (16860 / 37340), two players (17325 / 38614) and
Boss Mode (23462) are unchanged. Changed: the one-player whole game's Chapter 4 ends at 84870 (was 86358), the
ending is over at 92392 (was 93880); `--chapter 2`: Chapter 3 ends at 46149 (was 48024).
