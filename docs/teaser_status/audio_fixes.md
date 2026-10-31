# Audio fixes (2026-10-19, worker claude/audiomute)

## Mute after dying and continuing (TODO "MONDAY, HIGH PRIORITY BUG")

The headless repro (`--lives-out 600 --start-at 0:900`) forced the death at frame 600, during Chapter 1's opening
cut-scene, before the stage music's cue (0xA01001 at 794). The continue at 900 then goes to player state 10
(`player_start_check` 0x6020: camera_status bit 0 is clear under a cut-scene camera, kind 1), and `player_paused` 0x5730
waits for that bit. Meanwhile the cut-scene's `evt_WAIT_FRM` 0x66D60 waits for a player in state 5 with lives
(`evt_run` 0x66AD0). Neither one moves, so the event script stops, no music cue comes and no enemies appear: the WAV is
silent. The four routines match the arcade decompilation 1:1. In the original nothing hurts a player during a
cut-scene, so this state cannot happen there. Audio::pause / musicHeld_ / boardHeld_ are not involved: the pause port is
the only caller and it never runs here.

Natural deaths and continues in play keep the music and effects: autofire with `--start-at` every 150 frames
(continues at 2000, 4250 and 8000, chapter 1 cleared at 16769), `--bot 3` (a continue at 5892), PC sound and the arcade
board.

Fix (test tool): `--lives-out F` now takes effect on the first frame from F that is in play (camera_status bit 0, a
player in state 5). The same repro now has the death at 812 and the continue at 900, and the sound goes on to the end.
RMS per second for seconds 15 to 39: minimum 0.025 / average 0.068 (PC sound) and 0.022 / 0.058 (arcade board), the
same with `--models arcade` and `--models pc`.

Still open: the user's own play-test case is not reproduced headless. If it comes back, ask for the chapter / place and
whether the game itself stopped (state 10 under a cut-scene) or only the sound did.

### The window-only paths (second pass, 2026-10-19)

I read every place a window run holds sound. None of them leaves a hold in place across a death, the continue screen
or the restart after a game over:

- **Focus-loss pause** (main.cpp `SDL_EVENT_WINDOW_FOCUS_LOST` -> `Pause::focus(false)`, game mode 4 only, so the
  continue countdown (sub-modes 3 / 4) and the name entry count too):
  - `Pause::enter` holds the sound with `Audio::pause(true)`.
  - Focus back keeps the pause on screen. `Hud::drawPause` is drawn whenever paused, in every state.
  - Continue (a click, Enter or Esc / P) calls `Pause::leave`, which releases it with `Audio::pause(false)`.
  - Exit calls `leaveGame`, which runs `Audio::silence`. That clears musicHeld_ and the voices and resets the board
    (boardHeld_ = false).
- **Menu music held while inactive** (1724761):
  - Only `Screen::Menu` (the PC front end) handles it. The game-over map, the continue screen and the name entry are
    `Screen::Game`, so they are not covered.
  - Leaving the menu while held releases it (`menuAudioHeld && screen != Menu` -> `pause(false)`).
- **boardHeld_**:
  - Only `Audio::pause` sets it.
  - `startBoard` and `resetBoard` (through `silence`) clear it.
- **musicHeld_**:
  - `pause(true)` sets it only while a track plays.
  - A new track (`playTrack`) clears it.
- **Restart after a game over** (`startGame`): it does not hold anything.

Simulated headless with the same `Pause` object (HOTD_PAUSE_AT=2000:120): autofire, the natural death at 1988, the
pause opened at 2000 in the continue countdown and closed 2 s later, then a continue at 2100. Second 34 (the pause) is
silent, and the sound goes on afterwards. RMS per second for seconds 35 to 45: 0.034 to 0.158 (PC sound), 0.059 to
0.167 (arcade board, `--models arcade` and `--models pc`). The window's own focus events are not injected: automated
runs ignore focus. Nothing to fix was found.

## PC pops / clicks and loudness (TODO "MONDAY FIRST ... the audio")

- Every voice starts with a 3 ms ramp, and a one-shot ramps down over its last 3 ms. A voice that is stopped (cue type
  2 / 5, a type-4 restart), restarted (the same sample within 1/15 s) or stolen keeps playing as a tail that ramps down
  over 3 ms (8 tail slots) instead of stopping on a sample away from the centre (128).
- PC master level `kPcLevel` 0.66 (effects and music, so the menus too).

Measured on Chapter 1 with autofire and `--lives-kept 1` for 6000 frames (`--audio-wav`):

| | before | after | arcade board |
|---|---|---|---|
| Effects only (`--no-music`): sample-to-sample steps > 0.2 | 130 | 7 | |
| Effects only: largest step | 1.10 | 0.25 | |
| Full mix: RMS | 0.129 | 0.085 | 0.085 |
| Full mix: samples above the limiter's 0.7 | 4770 | 45 | 63 |

Not done: no comparison with THOTD's own DirectSound mix yet (hotd_hook.c dump). The in-game pause still holds voices
at once, with no ramp, as the original does.

Checks: `--input-test` 524 checks, 0 failed. `--frames 40000 --autofire --mute --lives-kept 1` reaches "chapter 1 ended"
at 16695 with both `--models arcade` and `--models pc`.

## PC = THOTD (2026-10-19, third pass)

- **Unmapped arcade codes** (autofire, `--lives-kept 1`): Chapter 1 and Chapter 2 on both routes, `--chapter 3` and
  `--chapter 3 --event 7 --strict`.
  - The only unmapped code was 0xA00000, the arcade data's "no sound" entry, sent from a sound table at Chapter 3's
    start. It is now silent and not counted.
  - The unmapped set is empty on all six runs.
- **Our cues against THOTD's** (Chapter 1, aligned at the music cue):
  - THOTD side: PC harness, `run_pc.ps1 -Ticks 20000 -InfLives -InputScript scripts\game_aim.txt -CapFuncs 412000`,
    giving 2900 sound_cue calls (`tools/pc_cue_log.py` reads the capture).
  - THOTD sends two cues of its own that ours lacked:
    - cue 7 (every sample off) where the arcade sends 0xBE5000. 0x43E6E4 sends cues 2 + 7 in title_start,
      mode9_story_start, ranking_start and game_over_start; kind17_researcher_frame sends 7.
    - cue 1 (every sample off) before the clear music. kind176_chapter_clear sends cues 1 + 13.
    Both are added in `Audio::cue`.
  - Not exact: THOTD's own game_new and evt_NEXTEV send no cues, while ours maps the arcade's 0xA00002 / 0xBE5000
    there to cues 2 / 7. That happens at frame 0, at 608 (before the music) and at the game's end.
  - The music cues come in the same order on both sides: 9, then 2 + 12 (boss), then 2 + 13 (clear).
  - THOTD's `sound_cue` 0x412000 plays a type-3 track looping except cues 11 / 13, as ours does.
  - Patches 6 / 8 (pause / resume) were matched earlier (pause.md). Patch 9 (menu music = track 11) is in the
    rewrite.
  - Everything else differs only by route and aim:
    - THOTD's auto-aim fires about 5x more shots (cue 30: 1822 vs 348).
    - A few voices and props appear on one side only (PC: 51, 83, 88, 177; ours: 57, 89, 129, 133, 154, 159, 236).
- **Board queue:** `sound_cue` 0x154D0 now queues as the original and sends one command a frame to the board.
  `sound_send3` goes straight, where the original sends straight. MAME attract compare: 179 / 179 commands on the same
  frame. The PC sound still plays each cue at once, as THOTD's own `sound_cue` does.
