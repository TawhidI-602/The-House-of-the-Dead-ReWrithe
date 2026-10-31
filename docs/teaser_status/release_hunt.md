# Release hunt 2 (worker "hunt2", 2026-10-20, main at 2e781f4)

Branch `claude/hunt2`. The same hunt as below, repeated after ~40 merges (PC Mode rules / continues, Boss Mode name
entry, 4P cut-scenes, online 4P, cheats, Game Speed Auto), with the newer modes added. Every run headless
(`--frames 400000`, `--mute`, `HOTD_PREF_DIR` a scratch folder), 6 at a time; `--root` the main checkout. A run is
about 1-3 s (a whole game in ~2 s). Logs: the worktree's `build\hunt\logs\` (not committed).

## Runs (final build)

| sweep | runs | flags | result |
|---|---|---|---|
| A chapter starts | 48 | `--bot S --chapter 1..4 --models arcade/pc --bot-players 1/2 --lives 9 --bot-gameovers 3`, a third `--bot-skill 0.95,0.95` | 8 clear (on through the ending), 40 at the 3rd game over; 0 problems |
| B full game, lives kept | 12 | `--chapter 1 --continue-chapters --lives-kept 1`, 1P / 2P, both sources, plain / `--strict` / `--strict --default-route` | **12 / 12 through the ending**; 0 problems |
| C full game, real lives | 12 | `--continue-chapters --bot-gameovers 6 --bot-skill 0.85,0.85`, 1P / 2P, both sources | 1 through the ending, 11 at the 6th game over; 0 problems |
| D Sonic Mode | 24 | frenzy / overkill / nightmare x sources x 1P / 2P: full game lives kept + a chapter start with 5 lives | 12 / 12 lives-kept through the ending; the others clear / game over; 0 problems |
| E 3-4 players | 44 | 32 x `--autofire --players 3/4 --seat-bot 2..4:1/3` (CPU teammates, levels easy / hard), chapters 1-4, lives kept; 12 x `--bot --players 3/4 --bot-skill` (strong / weak / mixed) | **32 / 32 CPU-teammate runs through the ending**; bots 5 clear, 7 game over; 0 problems (after the watchdog fix) |
| F PC Mode | 36 | `--pc-char 1,0 / 5,6 / 2,4 / 6,0`, chapters 1-4, both sources; 4 full games lives kept | 4 / 4 full games through the ending; 1 more clear, 31 game over (PC Mode continues run out: weak bots); 0 problems |
| G Boss Mode | 20 | `--boss-mode chariot / hangedman / hermit / magician / all`, both sources, 1P / 2P | 11 cleared (BOSS MODE result=cleared, a rank), 9 game over; 0 problems |
| H routes | 10 | `--autofire --lives-kept 1 --route 1:1=1 / 2:1=1 / 3:2=1 / 4:1=1 / 1:3=1,2:4=1,3:5=1`, both sources | 10 / 10 through the ending |
| I flows | 16 | `--lives-out` + `--start-at` (continue / join after a death), difficulty 0 / 4, `--game-speed 60`, `--name-entry --name-keys ABC#` from Chapter 4 | all ended as expected (game over -> restart; name entry and ranking run); 0 problems |
| J determinism | 10 | 5 seeds run twice (2P arcade; 4P PC models) | **5 / 5 pairs identical logs** |
| K continue / lives-out | 32 | `--bot --bot-players 2 --lives-out F --lives-out F+7000` (F 600 / 5000 / 15000) and autofire continue + join at 3000 / 12000, chapters 1-4, both sources | 5 clear, 19 game over, 8 autofire runs to their game over; **4 soft-locks before the fix** (below), 0 after |
| R random mix | 300 | random chapter / source / 1-4 players / skills / lives / difficulty, plus Sonic, PC Mode, `--lives-out`, `--strict`, `--lives-kept` | 139 clear, 160 game over, 1 flagged (R_140, below: a bot artefact) |
| rendered | 10 | `--render --shot-at`: Chapter 1 both sources, CONTINUE / GAME OVER / route map both sources, Chapter 4 + ending + name entry both sources, 4P CPU teammates, PC Mode (Sophie + Researcher), Boss Mode Hermit, Sonic Frenzy | **0 GL errors**; 35 shots: HUD, CONTINUE, GAME OVER, route map, credits all drawn, no black frames. Draw mean 0.4 ms (PC) / 3-6 ms (arcade) |

Total: 564 bot / headless runs (24.1 M game frames) + 10 rendered runs. All exited 0; 0 crashes, 0 asserts, 0 hangs,
0 `bad_access`, 0 `nan_draws` (after the fixes), max tasks 165, max draw list 276.

## Fixed (commit on claude/hunt2)

1. **PC Mode: the last continue was lost** (src/port/game_flow.cpp, game_continue_countdown's PC Mode branch). Ours
   ended the game at once whenever the credit pool was 0 after the frame, also when a player had just used the last
   credit to continue (that player was back in, state 1 / 5). THOTD.EXE's game_continue_countdown 0x4588A0 ends it at
   once only for a player **still in state 4** with no credit; ours now does the same. Seen as NaN draws: the game
   over then had no player in state 6, so the route map's walker (game_over_walkers_start, index -1) got a NaN place
   (12 of 36 PC Mode runs, 2000-10000 NaN draws each).
2. **4P game over with players 1 / 2 never in the game** (src/port/game_over.cpp game_over_walkers_start): seats 3 / 4
   ended the game, players 1 / 2 in state 9: the arcade's walker index -1 (unreachable on the arcade) read its tables
   out of range (NaN walker, 18 4P runs). 4P / PC Mode only: no walker then. The arcade path is unchanged.
3. **Watchdog (testing only, src/game/bot.cpp):** the progress check looked at players 1 / 2 only, so a 4P Magician
   fight carried by player 3 counted as a soft-lock (R_205). It now hashes every seat.
4. **`--lives-out` (testing only, src/main.cpp):** a lives-out still pending at a game over's restart fired at frame 1
   of the restarted chapter (camera_status bit 0 still the old chapter's), inside the opening cut-scene: the
   "impossible state" its own comment describes (player 2 joining into state 10, event script waiting for ever).
   4 of 24 K runs. A restart now drops it.

Checks: `--input-test` 1480 checks, 0 failed; `--frames 40000 --autofire --mute --lives-kept 1` both sources: chapter
1 ended at 16695; `--chapter 4 --strict --autofire --lives-kept 1 --frames 60000 --default-route` both sources: the
ending is over at 34967.

## Open / notes

| # | severity | what | notes |
|---|---|---|---|
| 1 | info (bot) | R_140 (`--bot 1140 --chapter 2 --models pc --players 3 --bot-skill 0.2,0.1,0.4,0.8 --lives 3 --difficulty 3 --bot-gameovers 1 --bot-players 2`): player 2 alone, one zombie circles him ~20 units away on screen and never attacks, the event waits (evt_WAIT_ENALV) | Arcade logic, not a stall for a person: the zombie finished its attack (state 8 -> 9) while everyone was out (sub-mode 4), when attack slots 0 / 1 sit at the camera, so its retreat point is the player's own place and it orbits inside the 25-unit "far enough" circle (enemy_ai state 9, enemy_ai_motion). A player shoots it; the bot (skill 0.1, "left") never does. Not changed (ported routine, no capture proving it wrong) |
| 2 | low | Chapter 4 + ending render, arcade models: one 1.9 s frame at 31153 (3 renders in parallel) | As open #7 below (32443 then): contention, not seen serially before; not re-run |
| 3 | info | The open items of the first hunt below: #1 (4P soft-lock with players 1 / 2 out) was not seen in 44 + ~150 3P / 4P runs now; #4 / #5 unchanged (match the PC files) | |
| 4 | info | Computer teammates are seeded from the clock (`SDL_GetTicksNS`), so runs with `--seat-bot` are not repeatable | testing only; the `--bot` runs are (J) |

# Release hunt (worker "hunt", 2026-10-20)

Branch `claude/hunt` (build folder `build` in the worktree). Crashes, soft-locks, hangs, bad memory access, NaNs and
visible glitches across the whole game, both model sources. Shots and logs under the main checkout's
`private\hunt\` (not committed): `vis_c<N>_<source>\render_f<F>.png` (a shot every 450 frames of each chapter's default
route), `sheet_*.png` (arcade | PC side by side), `issue_*.png`, `bot_logs\` (the 4P runs).

Every run: headless (`--frames`), `--mute`, `HOTD_PREF_DIR` = a scratch folder, 4-5 at a time.

## Runs

| sweep | runs | flags | result |
|---|---|---|---|
| chapter starts | 48 | `--bot S --chapter 1..4 --models arcade/pc --bot-players 1/2 [--players 2] --lives 9 --bot-gameovers 3`, a third with `--bot-skill 0.95,0.95` | 16 clear (2P and skilled bots go on through the following chapters and the ending: chapters continue by default now), 32 stopped at the 3rd game over; **0 problems** |
| full game | 16 | `--chapter 1 --continue-chapters --bot-gameovers 8`, 1P / 2P, both sources, 4 seeds (one `--strict`) | 2 through the ending, 14 at the 8th game over (weak 1P bots); 0 problems |
| full game, lives kept | 12 | `--continue-chapters --lives-kept 1`, 1P / 2P, both sources, 4 `--strict` | 12 / 12 through the ending (81k-116k frames); 0 problems |
| Sonic Mode | 24 | frenzy / overkill / nightmare x arcade / pc x 1P / 2P: full game with `--lives-kept 1`, plus a chapter start with real lives | 12 / 12 lives-kept runs through the ending; the others clear or game over; 0 problems |
| game speed 60 | 8 | `--game-speed 60`, chapters 1-4, both sources, 2P | 8 / 8 clear through the ending; 0 problems (headless, the speed only changes the audio tick) |
| 4P | 16 | `--players 4 --bot-skill` 0.9x4 / 0.9,0.9,0.3,0.3 / 0.3x4 / 0.5,0.8,0.2,0.95, chapters 1 and 3, both sources | **6 soft-locks** (below), the rest clear / game over |
| rendered | 8 + 6 | `--render --autofire --lives-kept 1 --default-route`, every chapter + the ending, both sources, 8 runs with 37-78 shots each; 6 short debug renders | **0 GL errors, 0 draw-call spikes**; draw mean 0.3-0.5 ms (PC) / 2.5-3.3 ms (arcade) |

Total: 132 bot runs (~10.1 M game frames) + 14 rendered runs. All exited 0; 0 crashes, 0 asserts, 0 hangs (wall
timeout), 0 `bad_access`, 0 `nan_draws`, 0 runaway task rings / draw lists in 1P / 2P. Max tasks 111, max draw list 256.

## Fixed

Nothing in the game code this session (the 1P / 2P sweeps found nothing to fix; the finds below are in other workers'
areas or need more than the time left). Reported to the coordinator at once:
- **Start in fullscreen did nothing** (src/main.cpp: `toggleBorderlessFullscreen` called twice in a row, the second
  undoing the first; from 6e1c542): fixed on main by the coordinator (5261616).
- **(was open #2) Chapter 3 security card invisible with arcade models** (glitch, branch claude/glitch): the card
  0x16CE `sekom01a` (flat, so a decal item) lies 0.1-0.24 units **under** the console's top 0x16D0 `st3_card01a`, not
  above it (a ray from the camera hits the console first). The Model 2 sorts polygons (MAME model2_v.cpp: the console's
  big polygon sorts by its own min / max z, the small card's is nearer) so MAME draws it; our depth test hid it whole
  (drawn with the depth test off it showed in place). Fix (renderer.cpp vertex shader): an arcade decal away from the
  floor gets the floor decals' 0.3-unit dip along its own polygon normal (uFloorPull 2, uShading 1 only: PC untouched).
  private\glitch\c3_security_card_before_after_mame.png (before / after / MAME 0351).
- **(was open #3) Chapter 1 canal: the bright checker layer on the water** (glitch): world_draw_task's Chapter 1
  special case (decompilation.md "World drawing") draws, with placements 0x1C-0x1E (the canal water hashi01_08a-10a),
  also 0x916 / 0x91A, 0x917 / 0x91B, 0x918 / 0x91C (hashi02 / hashi03 _08a-10a: checker polygons, 0x916 in the sky's
  palette entry 8: the tan clouds) through uv_scroll_model (v scrolled frame_counter % 1024 / 8 texels). Never drawn
  until now. view.cpp adds them after their placement (arcade stage only; HOTD_NO_CANAL_LAYERS=1 = before) and sets
  the renderer's UV scroll for 0x915-0x91C. private\glitch\c1_canal_water_before_after_mame.png (before / after /
  MAME 0060), c1_after_f5000.png. 0x917-0x91C not seen on screen in the checked frames (same code path).
  Checks: `--input-test` 911 / 0 failed; `--frames 40000 --autofire --mute --lives-kept 1` (and `--models pc`):
  chapter 1 ended at 16695.

## Open

| # | severity | what | repro | notes |
|---|---|---|---|---|
| 1 | **high (4P)** | 4P soft-lock: when **players 1 and 2 are both out** (state 9) while player 3 or 4 still plays, the game goes to game mode 4 sub-mode 2 and the event script waits for ever at `evt_WAIT_ENALV` / `evt_WAIT_ENNUM` (no enemies come). 6 of 16 4P runs, both sources, Chapter 1 events 4 / 8 / 9 / 11 and Chapter 3 event 2. Also seen: player 2's lives going **negative** (-1, -2, -3) in that state | `hotd --root <repo> --mute --frames 250000 --bot 5037 --chapter 1 --continue-chapters --models arcade --players 4 --bot-skill 0.3,0.3,0.3,0.3 --lives 9 --bot-gameovers 3` -> `ROBUST: result=problem` at frame 13266. Also seeds 5038 (`--chapter 3`), 5041 / 5043 / 5045 / 5047 (`--models pc --chapter 1`, skills as in private\hunt\bot_logs) | coop4b's area: the everyone-out test looks at P1 / P2 only. Reported to the coordinator. The watchdog's dump prints P1 / P2 only (the 4P seats line at the end has all four) |
| 4 | low (PC models) | Chapter 2 event 1 rooftop (frame ~1500): **no moon** behind the bats with PC models (the arcade models and MAME snap 0186 have it); the boss fight's moon later shows with both | `--chapter 2 --models pc --shot-at 1500`; issue_c2_moon_pc.png | **Matches the PC (pcsky, 2026-10-20), no change.** That moon is model **0x11AD** (arcade name `moon`, at 300 / 140 / 0), drawn by prop_default (0x3B160; PC 0x47C604) from frame 1410; the falling leaves 0x11A8 (kokonoha, prop023 0x3C770) likewise. Neither ID is in any PC model file (CREAT1 / CREAT2 / WORLD1-6.RBH searched for the `id, 0xFFFF` table entry), and THOTD.EXE draws by ID through geo_draw_model (0x43D7F8: nothing when DAT_00d53080[id] is 0), so the real PC draws no moon and no leaves there; ours logs both as PC model gaps. The boss fight's moon is a different model, 0x14AC (billboard_model, PC 0x46B928), which CREAT2 has. Not seen in a live PC capture (the harness runs stop in Chapter 1) |
| 5 | low | Chapter 4 opening sky (frame ~28050 = the ending's sky shot with `--continue-chapters`): PC models show dark clouds on white (looks like a negative) where the arcade has a blue sky | sheet_c4_b.png | **Drawn as the PC files have it (pcsky, 2026-10-20), no change; not seen live.** Both sources draw the same sky models there (`--chapter 4 --default-route`, frame 28050, HOLD YOUR FIRE: 0x1D20 skyuvst4a + 0x608), and THOTD.EXE's sky_task / sky_task_ending (0x471224 / 0x47134C) draw the same table (0x1D20 for Chapter 4). The PC's 0x1D20 (CREAT2.RBH) textures all 30 of its polygons from BIGGER.VMC page 588 (Chapter 2's blue 0x1238: page 590); `HOTD_PC_NO_PAL=1` (the raw 16-bit texels, no STAGE4.TBL / PAL) gives the same dark clouds on white, and STAGE4.PAL maps sky blues to blues, so it is the PC texture itself, not our palette step. Open: a real-PC capture of Chapter 4 (the harness would need a chapter start) to be sure the PC driver shows page 588 that way |
| 6 | info | "RELOAD" and the researcher-shot "MISS SHOT / LIFE -1" overlap (Chapter 1 ~8250); "PRESS START BUTTON" next to the cut-scene captions | sheet_c1b / c1c | Same places in MAME: faithful |
| 7 | info | Arcade Chapter 4 + ending render: one 2.5 s frame at 32443, p99 12 ms (other runs: max 70-88 ms) | vis_c4_arcade.log | 2 renders + 3 bot runs in parallel: contention, as robust-2 saw; not reproduced serially (not re-run) |

## Not done

- Smooth motion (interpolated in-between pictures vs `--smooth off`): `--interp-dump` needs the window loop (a hidden
  window on a 1-display machine works, but there was no time left to compare); not checked this session.
- The visual sweep looked at the default route only, one shot every 450 frames (~200 places); Chapter 4's first half
  (sheet_c4_a.png) was made but not studied.
