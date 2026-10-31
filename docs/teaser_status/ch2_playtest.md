# Chapter 2 play-test review (worker "ch2video", 2026-10-19)

Source: the user's recording "2026-10-19 17-46-06 HOTD ReWrithe Chapter 2 Test 1.mp4" (402 s, 1920x1080, widescreen,
arcade models, Sound arcade). Sampled at 1 fps, 4 fps around suspects; the Hangedman fight (4:38-6:12) was left to its
worker. References: our off-screen renders (`--chapter 2 --render --autofire --shot-at`, arcade and PC models: game
frame ~= (video seconds - 12.6) x 60 for the first minutes) and MAME's full-game snapshots
(private\mame_dumps\fullgame_us\snap\hotd 0174-0334 = Chapter 2, one every 2 s; its route skips the trapdoor / kitchen
/ dining hall, but shares the library / green room, the balcony hall, the bedrooms, the yellow corridor, the lab and
the moon scene).

Priority change from the coordinator (user): Chapters 2-4 playable first; only blocking / stalling / crash fixes now,
visual polish stays listed as open.

## Issues

| Video time | What | Likely cause | Status |
|---|---|---|---|
| 0:13-0:15 | Widescreen: during the chapter card ("The Second Chapter / Revenge") the window outside the 4:3 card showed the lit 3D world instead of black (Chapter 1's card has black sides only because its world is still black then) | the card is a 4:3 2D picture; nothing blacked the sides in 16:9 | **Fixed** (97a962f): `Hud::draw` clears outside the 4:3 screen while `evt_clrgfst_flags & 0x20` (kind177_chapter_card) is set. Checked 1920x1080, arcade + PC models (frame 60 black sides, frame 185 world full width) |
| 4:32-4:37 (and the title card / fight) | The Hangedman has no wings (MAME snaps 0279-0299: big green bat wings in the moon scene, title card and fight; ours, both model sources, frames 15000 / 15300: body only) | boss model / parts not drawn (boss_hangedman.cpp / hangedman_flyby.cpp) | **Open, boss worker** (reported to the coordinator) |
| 6:31-6:35 | The end-of-teaser text ("CHAPTER 2 CLEAR ... Chapter 3 is not finished yet") is drawn over the frozen name-entry picture (letters and clock visible behind the panel) | main.cpp's end screen draws the last 3D frame under its panel; same after Chapter 1 | Open, cosmetic (not Chapter 2 code) |
| 6:13-6:20 | Results screen in 16:9: the 1P ammo / torches sit at the window's left edge outside the 4:3 results picture | display2's edge-anchored HUD; MAME 0172 shows the HUD on the results screen too, so only the position is a widescreen choice | Open, cosmetic, by design? |
| 0:47-0:50, 1:05, 1:42-1:44, 2:00-2:01 | Very dark / near-black stretches (stair-hall alcove, kitchen door, dining-hall passage) | checked: our arcade render matches the video, and MAME's Chapter 2 snapshots are as dark in the shared rooms (also private\screenshots\compare\sheet_ch2.png); the PC models are much brighter | Not a bug (arcade lighting) |
| 2:02-2:17 | Green / blue patterned ceiling and walls in the library room (PC models: grey stone) | MAME 0206-0211 shows the same green / blue room | Not a bug |
| 2:47-3:06, 3:44-3:49 | Green wallpaper bedroom, blue-tile bedroom, yellow wallpaper corridor (PC: grey) | MAME 0220-0229, 0253-0255 identical | Not a bug |
| 0:22-0:24 | The shot barrel splits into halves that are black inside | MAME 0179 the same | Not a bug |
| 2:04 | A grey ring outline on the floor under a landing enemy | MAME 0226 the same (dust ring) | Not a bug |
| 0:20-0:21 | Camera passes through G's coat right after the door opens | the camera path; MAME's 2-s snaps cannot show it | Unverified, low |
| 1:52-1:57 | Zombie carrying a scientist overhead; shot, the scientist drops, stands, LIFE UP | looks like the arcade's (no MAME shot of this room) | Unverified, looks right |
| 3:11-3:17 | Balcony: scientist chased by a zombie, escapes right | MAME 0233-0235 the same scene | OK |
| whole video | Sound: game audio present from start to end (RMS -18..-27 dB per 10 s, no silent stretch) | the board-mute fix holds | OK |

No crash, soft-lock or stall was seen in the recording outside the boss fight.

## Headless checks (worktree build, RelWithDebInfo, after the fix)

- `--frames 40000 --autofire --mute --lives-kept 1`: chapter 1 ended 16695, chapter 2 ended 36962 (both model sources).
- `--chapter 2 --strict --autofire --lives-kept 1 --mute --frames 40000 --default-route`: chapter 2 ended 17162.
- `--chapter 2 --bot 11` (arcade) and `--bot 12 --bot-players 2 --models pc`: `result=gameover` after 2 game overs
  (normal for the bot), `bad_access=0`, no problem frame.
- `--input-test`: 524 checks, 0 failed.

## Open (for later polish)

Hangedman wings (boss worker); end-screen panel over the name entry; widescreen HUD position on the results screen;
the G / camera clip at 0:21 to check against a denser MAME capture.
