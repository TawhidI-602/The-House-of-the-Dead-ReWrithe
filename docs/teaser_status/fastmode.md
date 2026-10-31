# Sonic Mode (experimental fast mode)

The user's idea (ROADMAP "Future: experimental Fast mode", named **Sonic Mode** by the user): the camera moves faster,
the bosses are faster (especially the Magician), and normal enemies attack a little faster. Cut-scenes and dialogue stay at
normal speed. It is opt-in, and the arcade game is unchanged when it is off.

## Phase 1 (done): presets on the tick schedule

| Preset | Play | Boss fight |
|---|---|---|
| Off (default) | x1 | x1 |
| Frenzy (was Hasty) | x1.2 | x1.2 |
| Frenzy | x1.5 | x1.5 |
| Nightmare | x1.5 | x1.75 |

Where to set it: launcher.ini `sonicmode=off|hasty|frenzy|nightmare`, the More Options row "Sonic Mode", the text
launcher's row, or `--sonic-mode <preset>` (alias `--speed`).

**How it works** (`src/game/sonic.h`): the logic itself is not changed. Every tick is the arcade's tick, so a headless
run with Sonic Mode on plays exactly the same frames as with it off (Chapter 1 still ends at 16695). What changes is how
long a tick lasts on screen. In play, a tick costs 1/factor of a display frame, counted in exact 1/42-frame units (x1 42,
x1.2 35, x1.5 28, x1.75 24). The window loop advances its `next` deadline by that amount (`SonicClock::advanceNs`,
which carries the remainder, so it stays exact over time). The cost depends only on the game state before the tick,
never on the wall clock. Both PCs in an online game therefore pace the same ticks the same way.

**Phase detection** (`sonicPhase`, the game's own flags). It is play only when all of these hold:
- game_mode 4 and game_submode 2 (game_play). This excludes the continue countdown (3/4), the chapter end (5/6), name
  entry (9/10), the ending (mode 4 sub-mode 7, mode 9), game over (mode 5), the attract demo (mode 3) and the title.
- scene_state 2 with no letterbox bars (`Runtime::letterbox` 0), shots on (`shot_enable`) and no clear results
  (`Runtime::clear.active`). letterbox_task sets scene_state 4 during cut-scenes, and 1/3 while the bars slide.
- No chapter card: kind 177 sets bit 0x20 in `evt_clrgfst_flags` while it is shown.
- No voice line: a kind 161 task (0x9B380 / 0x9B4C0) did not run this frame or the last one. `Runtime::run` records
  this in `voiceFrame`.
- At least one player is in play (state 5).

It is a boss fight when the boss life bar is up (0x543530, life_bar_init). The Chariot, Hangedman, Hermit and Magician
all use it. Their intros are cut-scenes and run at the normal rate. Pause runs no ticks at all.

**Sound and music**: effects play when the logic sends them. The music and the arcade sound board are rendered by the
audio device in real time, so they are not sped up. `--audio-wav` still writes 1/60 s per tick.

**Online co-op**: the host's preset goes to the client in the settings packet (protocol 4, one byte after the routes).
The client prints "Sonic Mode <preset>" when it gets it.

**Scores**: each preset keeps its own top 10: `ranking.bin` (Off) and `ranking_hasty.bin`, `ranking_frenzy.bin`,
`ranking_nightmare.bin`. The menu's High Scores screen shows the table for the chosen preset.

**Tracing**: `HOTD_TRACE_SONIC=1` prints each phase change: headless runs with scene_state, voice and card, window runs
with real time. Window runs with a preset print "sonic timing" at exit, with ticks per second by phase. Headless runs
print "sonic so far / at the end" lines with ticks by phase and display frames.

### Test results (2026-10-19)

- `--frames 40000 --autofire --mute --lives-kept 1`, arcade and `--models pc`: "chapter 1 ended" at 16695.
- `--input-test`: 865 checks, 0 failed.
- Headless, `--frames 200000 --autofire --mute --lives-kept 1 --continue-chapters`: Off, Hasty, Frenzy and Nightmare
  all play Chapters 1-4 and the ending without a crash. The same ticks for every preset: the chapter ends at 16695 /
  37190 / 62394 / 86358, and the ending is over at 93880. Display time for the whole run (95082 ticks: 25866 normal,
  52685 play, 16531 boss):

  | Preset | Display frames | Time at 60 Hz |
  |---|---|---|
  | Off | 95082 | 1584.7 s |
  | Hasty | 83546 | 1392.4 s |
  | Frenzy | 72010 | 1200.2 s |
  | Nightmare | 70436 | 1173.9 s |

  For Chapter 1 alone: Frenzy 12706 frames (211.8 s) against 16696 (278.3 s).
- Hidden-window run to frame 14000 (`--no-launcher --auto-start --autofire --stop-at 14000`). Off: 60.00 ticks/s in
  233.3 s. Frenzy: 173.6 s in total. Normal-rate ticks ran at 60.01/s, play at 90.01/s and boss at 89.94/s. Two
  cut-scenes measured at their real length:
  - The Chapter 1 cut-scene at frames 2892-3585 took 11.56 s (693 ticks at 60/s = 11.55 s).
  - The Chariot's intro at frames 12451-13416 took 16.08 s (965 ticks = 16.08 s).
  - The play stretch at frames 3585-5604 ran at 90.1 ticks/s.
- `tools/netplay_test.ps1 -HostArgs "--sonic-mode frenzy"` (and nightmare): the client got the preset. Hashes are
  equal for all 40000 frames (24f26acba90d219d on both), with no desync.
- `hotd_nettest`: all passed. The settings round trip now includes the preset.

### Known limits

- Everything in play is uniformly faster, including the player's own timings: the reload animation, hostage voice
  lines (these stay normal while the line plays), and the time to aim. Phase 2 below separates the dials.
- A voice line during play brings the rate back to normal for its length, by design ("dialogue at normal speed").
  Short lines in play (a hostage's "thank you") therefore slow play down briefly. If that feels wrong, the next step is
  to limit this to voice lines with a subtitle box or during scripted waits.
- The boss factor follows the life bar. The Hangedman's fly-bys in Chapter 3, before his fight, are play-rate.

## Phase 2 (plan, not started): separate dials through hooks

The goal is separate settings in place of a global tick rate. Each runs inside the deterministic logic, so it must come
from the settings (online: the host's packet), never from the clock. Tick scaling (phase 1) stays as the "overall"
dial. The others act per tick:

1. **Camera / rail speed**: the camera drive (camera.cpp, CAMDR modes; `camera_frames_left` 0x51F648, the spline step
   per frame). Scale the step, or run the camera task n times per tick with a fractional accumulator kept in game memory
   (snapshotted for rollback). Event waits (`evt_wait_frames`) tied to the camera's arrival follow on their own.
2. **Normal enemies' attack timers**: the attack slots (attack_slots.cpp, 0x5207xx) and the per-kind attack waits in
   enemy_ai / enemy_actions (the +0x2EC-style counters and motion frame steps). Hook the countdown decrement: subtract
   `factor` in 1/20 units with an accumulator per object (in the object's spare bytes or a side table keyed by serial
   that is snapshotted).
3. **Bosses**: per boss, the pattern tables (Magician 0xC69C0 `pattern`, `swoops` speed factors 0xC6A60, `shot_speeds`
   0xC6AA0; Chariot / Hangedman / Hermit state timers). Scale speeds and waits by reading the tables through a hook (the
   ROM tables untouched) or running the boss task twice on the accumulator's beat. Magician first (the user's ask).
4. **Projectiles**: thrown axes / barrels (held_* tables 0xC9C10..), the Magician's fireballs and barrage, the Hermit's
   spiders. Scale velocities at launch (the launch tables) rather than ticks, so hit boxes stay consistent.

Rules for every dial: Off = the exact arcade code path (proofs unchanged); the multiplier is in the netplay settings
packet; any per-object accumulator lives where rollback snapshots it; a headless `--frames` run with the dial on must
still be reproducible (the same frames twice); the score tables per preset (a dial setting counts as a preset "Custom").
Later: optionally the dynamic difficulty locked at its top, a score multiplier per preset, Boss Mode / PC Mode / 4P
inherit `cfg.sonic` because they start through the same `startGame` and window loop.
