# Boss Mode (worker "bossmode", 2026-10-20)

THOTD.EXE's Boss Mode (front-end state 0x5CED98 = 4), read from the PC decompilation / disassembly and put on our
runtime (the arcade logic). Code: `src/game/bossmode.h/.cpp` (state, clock, tables), `src/game/frontend.*`
(`screenBossMode` = screen 7, `screenBossTimes` = screens 10-14), `src/game/hud.cpp` (`drawBossClock`),
`src/port/game_flow.cpp` (`bossNoContinue`), `src/main.cpp` (`--boss-mode`, the menu hook, the end).

## What the PC does

| Step | THOTD.EXE | What |
|------|-----------|------|
| Menu | Boss Mode 0x422B90 -> screen 7 | background 0xA8, "Boss Mode" (label 0x42), five pictures 0xE3..0xE7 (hovered 0xE8..0xEC): Chariot top left, Hangedman top right, Hermit bottom left, Magician bottom right, All Bosses centre; Exit (Escape) |
| Pick | 0x422EE4 / F24 / F64 / FA4 / FE8 | frontend_state 4, boss_mode 0xC723B8 = 2 / 3 / 4 / 5 / 6, boss_index 0xC723BC = 0..3 (all: 0), result 0xC723C0 = 0, pc_boss_next 0x5CD360 = 1 |
| Start | game_new 0x458588 | chapter / event / route from boss_starts 0x5CD384[index]: Chariot 1/19, Hangedman 2/15, Hermit 3/7, Magician 4/4 (route 0); a normal new game (lives, ammo, score as Arcade Mode) |
| Walk-up | pc_main_loop 0x419168, ENTB3 | while pc_boss_next: frames run undrawn, ENTB3 spawns nothing |
| Entrance | 0x412BC4 from chariot_title_card 0x517CC8, hangedman_intro 0x49A300, hermit_title_task 0x4EF3A4, Magician 0x4D24A8 | pc_boss_next = 0, clock shown (0x5CD380), boss song cue 12 (Magician 19) |
| Clock | 0x412B00 start (title card's end), 0x412B14 stop (death; pause), timeGetTime ms | HUD 0x418CC8: "%02d:%02d:%02d" (min, s, 1/100 s) at (500, 440), font 0x2000000, max 60:00:00 |
| Win | 0x412C08: result 1 | Chariot: PC-only chariot_frame branch, 200 frames after its death (0x515DBD); Magician: 180 frames (0x4D2430); Hangedman: end of the death fall (0x49A2D1); Hermit: death reaction (0x4EEA8A) |
| Death | game_continue_begin 0x458808 / _countdown 0x4588A0, game_over_scene 0x5130C4 | no CONTINUE: every player out -> game over at once; result 7 |
| After | 0x419168 | result 1, all four, index < 3: next boss (new game, clock kept); else time vs the mode's table (IsHighScore 0x425748: 10 lowest times) -> name entry / table; result 7: the table |
| Tables | scores.cfg modes 2-6; screens 10-14 (0x423A90 ..) | background 0xD2, "High Scores", "Chariot Boss" .. "All Bosses", ten rows name / time; arrows 0xD9 / 0xD8 cycle 8 (arcade) 9 (PC Mode) 10..14; defaults BART .. ROHAN (Chariot 24-42 s, Hangedman 74-92, Hermit 40-58, Magician 70-88, all 3:45-4:30) |

## Ported

- Boss select screen 7 (menu's Boss Mode entry via `fe.startBossMode` -> `Frontend::screenBossMode`), start of each
  boss at its boss_starts event, the all-four chain (a new game per boss, clock kept), no continue (game over at once),
  the clock (real time per tick: Sonic Mode's shorter ticks count as such; pause stops it) and its HUD, the five times
  tables (`bosstimes.cfg` in the settings folder, the PC's defaults) with their page view (screens 10-14 as ours 32-36,
  arrows step the five boss tables; the menu worker's High Scores keeps its own tables), the result line
  `BOSS MODE: result=cleared|gameover mode=.. time=.. ms=.. rank=..`.
- `--boss-mode chariot|hangedman|hermit|magician|all` (or 2-6; bare = all). Works with `--players 2`, 4P (the boss
  life bar task scales the hit points as in a normal game), `--sonic-mode`, `--models pc|arcade`.
- `--frontend-shot x --frontend-screen 7` (boss select) / `32..36` (a table).

## Differences / left to do

- **PC rules (pmrest, 2026-10-27).** The bosses now report THOTD.EXE's own Boss Mode calls (Game::boss_event,
  port/game.h), at the PC's points, found by scanning THOTD.EXE for every call of 0x412B00 / 0x412B14 / 0x412BC4 /
  0x412C08 (38 call sites; the boss ones below) and reading each one's PC routine:

  | Boss | Entrance 0x412BC4 | Clock start 0x412B00 | Clock stop 0x412B14 | Win 0x412C08 |
  |---|---|---|---|---|
  | Chariot | chariot_title_card at camera frame 390 (0x517CFB) | its end, 180 frames on (0x517DFC) | chariot_death step 0 (0x515D42) | Boss Mode only: at the death motion's end step 3 with +0x2F4 = 200, no count / bank change (0x515DBD); win when the count was 0 (0x515E73), each frame after |
  | Hangedman | hangedman_intro step 4 (0x49A471) | step 5's end (0x49A598) | hangedman_death_fall step 0 (0x49A060) | the fall's end (0x49A2D1); **270** blink frames in Boss Mode, 180 else (0x49A03C) |
  | Hermit | hermit_title_task frame 1 (0x4EF41D) | its 180th frame (0x4EF4D5) | hermit death step 0 (0x4EE96F) | Boss Mode only: motion end -> step 3, 200 (0x4EE9D9); 200 - 1 a frame (0x5CD364 = 1, never written), win at 0 (0x4EEA8A) |
  | Magician | magician_descend at flag 213 (0x4D2536) | its 180 frames' end (0x4D25C4) | magician_dying step 0 (0x4D1F63) | Boss Mode: after the part effects step 30 with 120 frames (0x4D1EE4: no flight home, no fall), win at 0 (0x4D2485); the normal end 0x4D2464 |

  BossMode::tick (bossmode.cpp) reads them: the entrance shows the clock and plays the boss song (cue 12, the
  Magician 19: arcade codes 0xA01004 / 0xA0100B), the clock runs between start and stop, the win ends the boss (all
  four: the next game). game_over_start 0x512FC8 stops and hides the clock.
- **The walk-up** (pc_boss_next 0x5CD360, set at the pick and for each next boss, cleared by the entrance): the PC's
  main loop runs `while (pc_boss_next) frame_tick();` before the frame it draws, so ours runs those frames at once,
  undrawn (Runtime::frame). While it is set, as the PC: sound_cue / sound_send3 play nothing (0x412000), tile texts
  are not drawn (tile_text_draw 0x41B1C0), ENTB3 spawns nothing (evt_ENTB3 0x46E7FC), a camera path starts
  5 frames before its end (cam_job64_play_start 0x461158; not in Chapter 4 after flag 213), and the Chariot lands at
  (260, 542) (0x51564C: the camera may skip frame 290).
- **Other state-4 rules**: no Sophie (sophie 0x50E318 exits at once), no start button (0x436430 reads none in state
  4: nobody joins a boss game; the players the game starts with play), no continue / CONTINUE? (game_continue_*,
  0x50BC24), no "Continues N" (0x41B304), result 7 at the game over scene's 90 (0x5130C4).
- Times shown in High Scores too (pmrest): after the arcade / Sonic tables and PC Mode, the five boss tables (the
  PC's arrows on screen 8 go 8, 9, 10..14).
- Not modelled: the clock is ours (the tick's real time, Sonic Mode's shorter ticks counted as such) where the PC
  reads timeGetTime; F12 / debug keys of 0x419168; the PC's window rebuild between bosses (ours: a new game).
- Name entry: ported (worker "bossname"): THOTD.EXE screen_15 0x424EA8 as our screen 15 (`Frontend::screenNameEntry`):
  background 0xD2, "Player 1", "Congratulations!", "You have a high score.", the time, "Please enter your name:", the
  field 0x424F7C (centred white 24, cursor sprite 0x15A blinking). Keys as 0x425188: A-Z, 0-9, space (10 at most),
  Backspace / Delete, Left / Right (keypad 4 / 6), Home / End; Enter / keypad Enter / Escape end it (empty: ANONYMOUS,
  0x5A14C8). Ours: a letter grid (A-Z, 0-9, Spc, Del, End) for the mouse, pads (d-pad moves the hand, South picks, East
  deletes) and light guns. Then the row is filed (bosstimes.cfg; spaces kept as '_') and the table opens with it lit.
  Testing: `HOTD_BOSS_NAME=<name>` files a new best time under that name without the screen (headless runs then write
  bosstimes.cfg in HOTD_PREF_DIR); `--frontend-shot x --frontend-screen 15` with `HOTD_FE_NAME_KEYS` ('<' Backspace,
  '[' Left, '#' Enter). Headless runs without it file ANONYMOUS and do not write the table.
- PC Mode's table (screen 9) and the arrows from the arcade table into the boss tables (the menu owns High Scores).
- **Compared with the PC harness (pccap, 2026-10-27)**: PC captures of each boss and All Bosses (tools/pcharness
  scripts game_boss_*.txt), our Boss Mode fed the same gun (docs/proof.md "differential run"). Same frames: the
  entrance (walk-up 490 ticks for the Chariot), the clock's start (entrance + 180) and stop, the win, the next boss's
  entrance; the shown time: the Chariot 00:23:40 both (PC 23401 ms = the harness clock's +1 ms per call; ours 23400 =
  1404 ticks). The Hermit's PC clock read 23306 ms for 1668 ticks: the harness's timeGetTime (tick time + 1 per call
  since the tick) had run 4483 ms ahead when the clock started, so its time is short; ours (27800) is the 1668 ticks
  as a real PC would count them. Fixed: Boss Mode now runs the PC's maths and PA rules (`pc_path()`), no
  grandfather clock (the PC has none: its two random numbers shifted the fight). Left: the bosses' own motion maths
  drift by 0.1-0.3 from the PC's (the arcade's), so a fight's shots start to land differently after a few hundred
  frames.
- **Online (bossnet, 2026-10-27, protocol 18; docs/netplay.md "Boss Mode online")**: the host picks the boss (Host page
  Game Mode: Boss Mode + Boss; `--boss-mode` with `--host`). BossMode::tick now runs at the end of `Runtime::frame`
  (after the walk-up), so the clock, the win, All Bosses' next boss and the end are part of the game step (rollback's
  re-runs, a rejoin's replay); `BossMode::tickMs` is the clock's step: a local game sets it before each frame as before
  (offline times / frames unchanged), online it is 1000 / the tick rate (game ticks). The BossMode struct and the
  logic's boss_mode / boss_next / boss_events / bossNoContinue / startAtSection0 are in rollback's snapshot and the
  desync hash (with PC Mode's maths fields: a boss game runs pc_path). The end online: settled first, the session ends,
  each PC files the time in its own bosstimes.cfg (its own name entry), as online PC Mode. Nobody joins a boss game with
  start (as the PC); a PC that drops can rejoin its seat.
