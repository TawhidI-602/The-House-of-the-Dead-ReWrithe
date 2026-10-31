# PC Mode (PM): what it is in THOTD.EXE, and how ReWrithe runs it

PC Mode is the PC port's (1998) own way to play the arcade game: before the game each player picks a **character**
with its own hit points, rounds per magazine, bullet damage, reload time and hit size, and the game keeps its own
high-score table. Everything else (chapters, enemies, events, bosses) is the arcade game as the PC's Arcade Mode
plays it. The PC code tells the modes apart only by `frontend_state` (0x5CED98): **5 = PC Mode game** (2 Arcade
Mode, 4 boss mode, 6 attract, 7 back to the menus, 0 menus; docs/decompilation.md "The PC port's mode switch").

Sources: the PC decompilation (private\decomp\pc\thotd.c), `py tools/x86dis.py THOTD.EXE` for the parts Ghidra
gets wrong, THOTD.EXE's data read directly (the character table), names.tsv. Read 2026-10-20 (worker pcmode).

## The characters (table 0x5CE348, 7 x 0x34 bytes)

`cstats.cfg` (load 0x421DF4 / save 0x422194) overrides the table when it exists (the debug menu's Edit Char Stats,
`screen_pc_mode_edit` 0x426190, writes it); the player's install has none, so the defaults below are what everyone
plays. Layout (all little-endian):

| Offset | What |
|---|---|
| +0x00 | name, 0x10 bytes ("NULL PLAYER", "Rogan", ...) |
| +0x10 | creature type of the character's body (body_init: the endings, `player_char_models_set` 0x50C114) |
| +0x14 | model id (read by 0x496F2C, the player's prop / gun model in scenes) |
| +0x18 | portrait sprite (SPR.RBH, the character select screen) |
| +0x1C | second sprite (0xF3..0xF8; small portrait, not used by the select screen) |
| +0x20 | hit points = lives at the start (int) |
| +0x24 | rounds per magazine (int; the game copies the low byte) |
| +0x28 | bullet damage factor (float) |
| +0x2C | reload time, ms (int; shown on the select screen, see "Reload timer" for what the game really uses) |
| +0x30 | hit size factor (float: the shot-test radius) |

| # | Name | Type | Model | Portrait | HP | Rounds | Damage | Reload ms | Hit size |
|--:|------|-----:|------:|---------:|---:|-------:|-------:|----------:|---------:|
| 0 | NULL PLAYER (= "No Player") | 2 | 0x188 | 0xEF | 5 | 8 | 1.0 | 250 | 1.0 |
| 1 | Rogan (= the arcade) | 2 | 0x188 | 0xEF | 3 | 6 | 1.0 | 0 | 1.0 |
| 2 | G | 0x0D | 0x189 | 0xED | 3 | 2 | 4.0 | 250 | 1.2 |
| 3 | Rogan 2 | 0x0E | 0x1D38 | 0xF0 | 2 | 12 | 1.0 | 0 | 0.8 |
| 4 | G 2 | 0x0F | 0x1D39 | 0xEE | 5 | 6 | 0.5 | 250 | 1.2 |
| 5 | Sophie | 0x22 | 0x1D3A | 0xF1 | 4 | 12 | 0.5 | 500 | 0.8 |
| 6 | Researcher | 0x1A | 0x1D3B | 0xF2 | 2 | 10 | 1.5 | 500 | 1.0 |

Per player: `pc_mode_chars` 0x5CE4E4[p] (defaults 1 / 0: player 1 Rogan, player 2 No Player); 0x5CE4EC[p] is the
characters the other modes use for the same lookups (1 / 2: Rogan, G). `pc_reload_time` 0x5CE4DC[p] = the reload
timer (timeGetTime at the last reload).

## The character select screen (screen 16)

Main menu item 2 (`PC Mode`, 0x4259A4 -> builder 0x4259AC, screen id 16 in 0x5CE8A8):
- background sprite 0xA4 (flags 0x20000A, the whole screen);
- two panels (x 0x20 and 0x168, y 0x40, 0xF8 x 0x14A), one per player, built by 0x420080 (player in edx):
  - the character's portrait (+0x18) at the panel's top, height 100, centred (0x100020);
  - five rows, label left (0x20002) and value right (0x20004), at y 0xB8 / 0xD0 / 0xE8 / 0x100 / 0x118:
    `Hit Points:` (0x51) "%d" of +0x20; `Chamber:` (0x52) "%d" of +0x24; `Bullet Damage:` (0x53) Low / Medium /
    High (0x58 / 0x59 / 0x5A: <= 0.8, <= 1.2, more; thresholds 0x5CE4B4 / 0x5CE4B8); `Reload Time:` (0x54) Low /
    Medium / High by +0x2C (<= 200, <= 300 ms; 0x5CE4C0 / 0x5CE4C4); `Hit Size:` (0x55) Low / Medium / High by +0x30
    (<= 0.8, <= 1.2; 0x5CE4CC / 0x5CE4D0). (The labels Short / Long / Small / Large, 0x56 / 0x57 / 0x5B / 0x5C,
    exist but the screen does not use them.) For character 0 the portrait and rows are left out;
  - at y 0x130: the name (centred 0x20020; `No Player` 0x50 for character 0) between two arrows, sprite 0xD8
    (hover 0xDA, left, 0x100002) -> 0x425B5C (previous: below 0 wraps to 6; player 1 never gets 0 -> 6) and 0xD9
    (hover 0xDB, right, 0x100004) -> 0x425BAC (next: above 6 -> 1 for player 1, 0 for player 2); both rebuild the
    screen;
- title `PC Mode` (label 2, white 32 centred 0x10020) at y 0x10;
- `Start` (0x4F, white 24 centred) at y 0x1A0 -> 0x425990: `frontend_state = 5` (then the game starts as Arcade
  Mode's does); `Exit` (0x24) one line (0x5CE8D8 = 32) lower -> the main menu (0x4210F4), tag 1 (Escape).

## Gameplay rules that differ (every read of frontend_state == 5)

| Routine | PM rule |
|---|---|
| `PlStartInit` 0x43557C (start lives) | lives = lives_saved = the character's hit points (+0x20), instead of the Life setting (0xD5B1D8[p]); the gun's rounds = the character's rounds (+0x24) instead of 6 |
| 0x435518 (a PlStartInit set-up row) | rounds = the character's rounds |
| `pc_gun_input` 0x4358CC (auto reload) | with Auto Reload on (0xD5B1F4[p] == 1) and rounds 0: reload at once to the character's rounds, timer started |
| `pc_gun_input` (reload timer) | while the timer is set: if now - timer > **the dword at +0x24 (the rounds!)** the timer is cleared, else the trigger (GunInput +0x19) is dropped. The game compares with the rounds field, not +0x2C: the "reload time" is never used. With 60 ticks a second (16-17 ms a tick) and at most 12 rounds the timer always runs out by the next tick, so in practice there is no reload delay (read from the disassembly 0x4363A2..0x4363F5; not yet seen in a capture: the PM reload path did not run in the harness games, docs/proof.md) |
| `player_shoot` 0x4365C4 (reload) | rounds = the character's rounds; the timer starts (timeGetTime) |
| `hud_ammo_draw` 0x50B7E4 | the bullets are not drawn while the reload timer is set; up to 12 bullets (the loop runs to x 0x18) |
| `part_hit` 0x4C3D58 | each part hit's damage (table + difficulty bonus) x the shooter's damage factor (+0x28), truncated (pc_float_chop) |
| `ray_sign_test` 0x414AAC | the hit-sphere radius x the shooter's hit size (+0x30) |
| `player_waiting_frame` 0x436B60 (join) | a player who joins with character 0 (No Player) gets Rogan, or G when the other player is Rogan |
| 0x496F2C, `player_char_models_set` 0x50C114 | the players' models / bodies from the PM character (+0x14 / +0x10) instead of 0x5CE4EC |
| `ending_p1_a` .. `ending_p2_b` 0x4AA52C-0x4AB148 | the ending's character body = the PM character's type (+0x10) |
| `mode67_ranking_pc` 0x44D0C4 | the scores (and each player's character, 0x5CE4E4[p]) go into the PC Mode table (table 1 of 0x5CE320: 10 rows of 0x1C bytes) instead of Arcade Mode's (table 0); `screen_high_scores_pc_mode` 0x4234E0 (screen 9) shows it with the character names |

Not different in PM (the PA rules apply): continues (the Continues setting; the rewrite applies it to PM only, arcade games are free play), scoring (no PM-specific score code was
found), hostages, the life gift (to the last killer), kill credit, bleeding cap, the PC's turn rate, items. There are
no PC-only items or power-ups in PC Mode. Boss mode (state 4) is separate and never PM.

## What ReWrithe has (2026-10-20)

- **Game<Pc> (proof side):** the PM branches of `part_hit`, `ray_sign_test`, `player_shoot` (player_shoot_pc with
  the reload timer through `pc_time_ms`), `player_waiting_frame` are ported and proven on the PM harness captures
  (docs/proof.md rows Damage / Shooting: game_pm.txt, game_pm_aim.txt). Not ported for the PC: `PlStartInit`,
  `pc_gun_input`'s auto reload / timer test (a boundary), `hud_ammo_draw` (PC: stub), the endings, the ranking.
- **The runtime** runs the arcade logic (Game<A>), which has no PC Mode. This worker adds PM to it as a ReWrithe
  layer, like the 4P mode: `Game::pcmode` (set by the runtime, 0 in every arcade game and in the proofs) and the
  per-seat character `pm_char[4]`; the PM rules above are applied in the arcade routines only while it is set
  (`pm_on()`), with the characters from the table above (`pm_stats()`; THOTD.EXE's defaults, the same numbers the
  player's PC files give). The reload timer uses the game frame clock (a frame = 1000 / 60 ms, as the harness).
- Front end: `startPcMode()` in main.cpp starts a PM game with the chosen characters; `--pc-mode` and
  `--pc-char <p1>[,<p2>,<p3>,<p4>]` (0..6; 7..19 the Extra Characters, below) do it from the command line.

## Phased plan

| Phase | What | Size | State |
|---|---|---|---|
| 1 | This page | S | done |
| 2 | PM rules on the runtime: start lives / rounds, reload refill, reload timer (as the PC: the rounds threshold), damage factor, hit size, HUD bullets up to 12, the No Player join rule; `--pc-mode`, `--pc-char`, `startPcMode()` | M | done (pcmode branch) |
| 3 | Character select screen 16 in the front end (PC art: sprites 0xA4, portraits 0xED-0xF2, arrows 0xD8-0xDB; labels 0x4F-0x5A) | M | done: `Frontend::screenPcMode` (frontend_pcmode.cpp), main menu item PC Mode opened; `--frontend-screen 116` shoots it (HOTD_FE_PCCHARS=a,b); not compared with a harness capture yet. 4P panels: done (pcsel4, below) |
| 4 | Character bodies: the players' models in scenes and endings from the PM character (types 2 / 0xD / 0xE / 0xF / 0x22 / 0x1A; the arcade has types 0xE / 0xF as the "pair 3" bodies, coop4's kBodySetB) | M | **done** 2026-10-20 (pcmode5): row 15 |
| 5 | The PC Mode ranking: its own top 10 with the character per row (table 1), name entry into it, screen 9 | M | **done** 2026-10-20 (pcmode5): row 15 |
| 6 | PC proofs for the rest: `PlStartInit` PC, `pc_gun_input` (auto reload, timer), `hud_ammo_draw` PC, against game_pm / game_pm_aim captures | M | open (2026-10-20: `hotd_proof pmmaths` proves the runtime's character table, table square root, sin / cos and arctangent tables and the PM turn / shot maths against THOTD.EXE and Game<Pc>; the routines above still need Game<Pc> ports and **new PC harness captures**: the game_pm / game_pm_aim capture files of 2026-10-14 are no longer in private\pcharness\runs (only pc.log / recording.txt are left), and taking new ones means running THOTD.EXE in a window for about an hour, which pcmode6 did not do while the user was using the PC) |
| 7 | PA rules where PM inherits them: **decided (user, 2026-10-20): PC Mode = the 1998 PC port's PC Mode exactly**, so every PA difference applies when PM runs. Plan and inventory: "PC rules in PC Mode" below | L | rows 1-10, 12, 14, 15 done, 11 n/a, 13 already (row 10: distances, every enemy kind's and boss's turn / move / aim maths, the camera matrix and the shot ray / ray test on the PC's maths; the bosses' PC rules found on the way: pmboss) |

## PC rules in PC Mode (phase 7)

**Route.** Running `Game<Pc>` for PC Mode games is not possible: the runtime is built on the arcade address space
and Game<A>'s boundaries (renderer, copro, tasks, src/game overrides), while Game<Pc> works on the PC's address map
and still has stubs (hud_ammo_draw, researcher_gift_spawn, PlStartInit ...). So PM stays the arcade logic plus a
layer, and the layer is extended **routine by routine**: in the arcade branch of each routine where the PC differs,
`if (pm_on())` runs the PC's rule (as `if constexpr (P::pc)` does for Game<Pc>), on the runtime's own copies of PC
state where needed (`pm_last_killer` = pc_last_killer 0xDA0AE8). pm_on() is false in every arcade game and in every
proof, so the arcade is untouched. Where the PC rule needs PC-only maths (table square roots, the x87 at 24-bit
precision, the PC sin / cos table) the layer uses the PC model functions only when they work on plain values; the
bit-level match is checked later with the PC harness captures (docs/proof.md "PC capture": game_pm.txt,
game_pm_aim.txt) by a differential run (PM runtime vs the PC trace): first version built by pccap (2026-10-27,
docs/proof.md "PM-runtime vs PC-trace differential run": Chapter 1 identical to frame 424, first divergence a hit
on another body part at 425).

**Inventory** (A / PA / PM table in docs/decompilation.md; "where" = the runtime routine to change):

| # | PC rule in PM | Where | State |
|--:|---|---|---|
| 1 | Kill credit on a tie: the only player in state 5, else the previous killer; no random number | `enemy_part_hits` (damage.cpp), `pm_last_killer` | **done** 2026-10-20 (2 players; 4P keeps its own rule) |
| 2 | Researcher's life gift: the life to the last killer, no random number; the effect only when a life was added | `researcher_gift_spawn` (player_support.cpp) | **done** |
| 3 | Enemy turn: slowdown at least 4 (enemy_ai; kind 11's leap has no minimum) | `enemy_turn_a` (enemy_ai.cpp) | **done** (since pcmode6 the whole turn is the PC's: row 10) |
| 4 | Shots pass scenery: no scene-quad candidates (lists a / b, pairs) | `shot_resolve` (shooting.cpp) | **done** |
| 5 | No shot flash: the shot fires on the pull's frame, no 4-frame lockout / flash phases | the gun cycle (`gun_flash_phase`, runtime input) | **done** 2026-10-20: the runtime's gun stand-in already keeps `gun_flash_phase` at 2 (the shot on the pull's frame, no lockout: the arcade's cycle never runs in the runtime), so only the optional screen flash (`--shot-flash`, sprites.cpp) is now off in PM |
| 6 | Reload input: the PC's own reload button (GunInput +0x16) instead of shooting off screen; auto reload (Auto Reload setting) | `pc_gun_input` 0x4358CC -> runtime gun input | **done** 2026-10-20. Button: the PC reloads on its reload button *and* on a trigger with the gun outside the window (0x4358CC sets GunInput +0x1A in both cases), which the runtime's input already does (reload key / button = an off-screen pull). Auto reload: read from 0x4358CC's start: with Auto Reload on (0xD5B1F4[p] == 1) and rounds 0, the refill at once (character's rounds, empty cleared, the reload timer started), no sound, no game-mode / shot_enable test: PM does this in player_shoot (shooting_ray.cpp, `pm_auto_reload` = the launcher's setting) and main.cpp's own auto reload (an off-screen pull with its sound) is off in PM |
| 7 | Player hit: -100 directly (not through add_score), the PC's invulnerability variant 0x4370D4 | `player_hit` (player.cpp) | **done: no change needed** (2026-10-20, 0x436FE4 / 0x4370D4 read): the PC subtracts 100 inline and floors at 0, as add_score 0x69B0 does; it lowers hits_counter (PC 0xDA0CE8) too; 90 frames, shake, the keep-a-life rule all the same; 0x4370D4 (force in ecx, frames on the stack, -1 = keep) is the arcade's player_hit_lose_life 0x6520 line for line. PM = the arcade here |
| 8 | Continue: credits = the Continues setting, none without credits, countdown restarts at 9 on a change, "Continues N" | `game_continue_begin` / `_countdown` (game_over.cpp has the PC branches) | **done** 2026-10-20: the Options screen's Continues row is live (THOTD.EXE 0xD5B1E0: "0".."9", handler 0x421564 +1 wrapping past 9, config_load 0x421DF4 default 5; launcher.ini `continues=`). In PC Mode `pm_credits` (game.h) = Continues + 1 (pc_continues_init 0x4488E4), one pool for both players (0xDA0B98 = 0); the start (0x43BAD8) takes 1 from it for player 1 only (player 2's start takes from slot 1, not the pool: pmmisc 2026-10-27, before both were taken), each join / continue takes 1 (0xDA0B7C / 0xDA0B88, 0x448934). player_start_check uses can_continue 0x45B8F0 (credits >= 1) instead of free play; player_continue_start 0x434F94: none left -> state 6; game_continue_begin 0x458808: CONTINUE only with credits; game_continue_countdown 0x4588A0: none left for a player still out (state 4) -> game over at once (hunt2: not when the last credit was just used), restart at 9 when the credits change; "Continues N" shows the credits left. Arcade games stay free play (pcmode 0: none of it runs). **pmmisc** (2026-10-27): (1) the Cheat screen's **Free-Play** (0xD5B35C, kept in thotd.cfg: config_save writes 0xD5B1D4..0xD5B35F) is applied: pc_continues_init copies it to 0xDA0B8C, then can_continue is always 1 and 0x448934 takes nothing (the start's own -1 still runs); ours: `pm_free_play`, launcher.ini `freeplay=`, the Cheat row saves it, `--pc-free-play` for tests. (2) A player **out of lives while the other plays** (player_st4_out_of_lives 0x434F38 / player_continue_frame 0x435018): the countdown runs with or without credits, its snapshot / restart at 9 follow the PC's credits (pm_credits: another player's join or continue restarts it), and CONTINUE? is placed only when the player can_continue (hud_continue_sprite 0x50BC24): with no credits only the digit counts down; seats 3 / 4 the same. Check: continues=0 + --lives-out: game over at once (frame 1990), with --pc-free-play the countdown (mode 5 at 2280); 2P with the pool empty after P2's join, P2 out: only the digit 9 at frame 1400 |
| 9 | Bleeding: at most 9 live bleed + drip tasks | `part_bleed` / `blood_drip` (hit_effects.cpp; PC counter `pc_blood_task_count`) | **done** 2026-10-20: `pm_blood_tasks` (game.h) as 0x4864DC / 0x486520 / 0x486378 (a new task only while the count is below 9, +1) and the exits of 0x4863FC / 0x4862AC (-1); zeroed with the heap at every chapter load (PC: sprite_init 0x412660). A capped drip keeps the arcade's coprocessor calls (fx_to_world) |
| 10 | Distances and shot maths: table square root, x87 24-bit, 4096-step sin / cos | enemy distances, shot ray | **done** 2026-10-20 (pcmode5 distances, pcmode6 turn and shot). **pcmode6:** both tables are plain data in THOTD.EXE, nothing fills them at start-up: the sin / cos table 0x5697B0 sits in the code section and the arctangent table 0xC653B8 (1025 ints) in the initialised data (file offset 0x84C7B8; the earlier "filled at start-up" was wrong: its only references are pc_atan2's two reads). Rebuilt in `port/pm_maths.h` from formulas, nothing copied: sin / cos entry i = round(sin / cos(i 2 pi / 4096) x 65536) / 65536, and the one value that lands at 43562.4987 (entries 474 / 550 / 1498 / 1574 / 2522 / 2598 / 3546 / 3622, sin or cos, +-) is 43563 on the PC, so +-43562 -> +-43563 (found by comparing the formula with the EXE; the PC's builder must have rounded that value with less precision); arctangent entry i = round(atan(i / 1024) x 2048 / pi), no exceptions. On them `pm_maths.h` has the PC's pc_atan2, pc_vec_angles, pc_vec_cos, pc_angle_turn, pc_matrix_rotate_y / _transform on plain floats; in PM `enemy_ai`'s turn and `kind11_leap`'s (enemy_ai.cpp / enemy_kinds.cpp) are `pm::enemy_turn` (= enemy_turn_pc: minimum slowdown 4 in enemy_ai, none in the leap), `shot_ray_setup` is `pm::shot_ray` (the PC branch: table-sqrt normalise, pc_vec_angles, the table's sin / cos) on the arcade's stored camera matrix 1 (coprocessor B-RAM, turned into the PC's row layout: `pm::copro_matrix`), and `ray_sign_test` is the PC's sum with the table square root instead of coprocessor 0x1F. **Check** (`hotd_proof pmmaths <THOTD.EXE>`, 10 s): tables 8192 + 1025 entries and the identity matrix 0x5CE4F4: 0 differ; a differential run against Game<Pc>'s enemy_turn_pc / shot_ray_setup / ray_sign_test (the PC routines the 2026-10-14 PC captures proved, here on THOTD.EXE's own tables and matrix stack): 2,000,000 turns (yaw also outside 0..0xFFFF, zero / axis / tiny / far offsets, backwards, min 4 both ways), 300,000 shot rays (random camera rotations and translations, aim over and past the screen), 1,200,000 ray tests: **0 differ**; the matrix layout against the coprocessor's own transform: 1000 / 0 differ. PM-runtime vs PC-trace run: first version by pccap (docs/proof.md); (pcmode7 did the camera matrix and the other kinds, below). **pcmode7** (2026-10-20): (1) the **camera matrix**: in PM camera_matrices_update also builds the PC's own stored matrices from the same camera records (`pm::camera_matrix` / `camera_inverse`: the identity, translate the eye, turn y / x / z, or z / x / y by minus the angles and translate by minus the eye, with the PC's table sin / cos and x87 order): stored 1 (`pm_camera`, now the shot ray's camera), 0 (`pm_camera_inv`, also the bottom of PM's matrix stack, as the PC leaves it current), 5 / 6 (`pm_camera0_inv` / `pm_camera0`: record 0); the arcade's own matrices still draw the world. (2) The **other kinds**: Game<A> has the PC's software matrix stack (`pm_mstack`, game.h), so a PC branch that is the PC's maths and rules on the shared object layout runs in PM as it is: `if (pc_path())` (the PC, or PM) in place of `if constexpr (P::pc)`, its matrix memory through `pcm` / `pcm_set` / `pcm_pop` / `pcm_identity` / `pcm_load_camera` / `pcm_load_stored5` / `_6` (the same memory operations on the PC, so Game<Pc> and its proofs are unchanged), the PC-only helper copies (translate / rotate x, y, z, b / products, decl/*.h) on PM's stack, pc_atan2 / table square root / sin / cos on the rebuilt tables. Converted: the dogs (dog_trig, set 2 / 3 / 4 maths, leap / camera distances, knockback, st2 move, angle to target, st3 steer, set3 fall; their log), the bats (no bob on the PC, circle step, dive point, fly at, distance), the agile ones (kind 70: face, floor / camera distances, wall move, leap, knife / projectile aim and flight, fall start, aim point, bite; moody throw), the researchers (init, face, gift, hold up, struck effect), yaw_toward_a and the scene zombie's PC hit-reaction fall-through, walk_to_attack_position and enemy_act3's distance, enguard's hit effect, kind 10's frame test, the frogs (view position, hop, leap attack), the Murrers (drop start, view positions with stored 5, leap targets, float-bit compares, `pc_molb_height_offset` as a runtime copy: Chapter 1 event 4 scene 1's members stand 12 higher), the axe / kit zombies (walk in, walk-in distance, view position, carry rule, iron ball) and the thrown objects (vector angles, knocked away, axe spin, launches from stored 6, axe frame, throw at camera, frame, kit drop, release, flight hit). The enemy helpers' arcade forms take the PC's in PM (`pm_maths.h` pieces): move_along_heading, angle_to_camera, walk_to, turn_toward_camera, attacker_list_add's key, enemy_target_distance's screen side, velocity_for_arc / _speed, attack_slot_nearest_object (angle_turn_toward and camera_side_set: the arcade's form is the PC's, checked). **Check**: `hotd_proof pmmaths` runs THOTD.EXE's own routines in an x86 / x87 interpreter (docs/proof.md): 102 routines, 2,065,600 cases, 0 differ. Not done by pcmode7: the bosses, held_turn / the drum's tumble, the axe release, the dogs' crand, `pc_wide_slot_kind`, the view tests: all done by **pmboss**, below. Still not done: the drawing branches (the PC's renderer: bats / knives / dust / held objects / boss effects drawn with the PC matrix; PM keeps the arcade's drawing); a PM-runtime vs PC-trace run (first version: pccap, docs/proof.md). Online PC Mode: offered since onlinepm 2026-10-27 ("Online", below; its between-frame fields are in rollback's snapshot since pmmisc). Earlier (pcmode5): `enemy_distance` (enemy_ai.cpp: the standard enemy's 70 / 40 / 25 and every other caller) uses the PC's table square root in PM (`pm_fast_sqrt`, game.h: the 256-word table 0x5CCEFC rebuilt from a formula, word i = bits(sqrt((1 + (i & 127) / 128) x (i & 128 ? 2 : 1))) - 0x3F800000; `hotd_proof pmmaths <THOTD.EXE>`: all 2^32 float inputs bit-exact with the PC's pc_fast_sqrt). |
| 11 | Bullet damage applies outside game mode 4 too (demo) | part_hit | n/a in PM (PM is never the demo) |
| 12 | Enemy step forward: not in the PC's enemy_ai (the PC moves enemies elsewhere, to find) | enemy_ai | **done: no change needed** 2026-10-20 (pcmode5). The PC moves a standard enemy only by its motion's root step: body_draw 0x477B00 puts the body at the root's frame-to-frame offset (0x477C16..0x477E2D), as the arcade's body_draw 0x37720 does. The arcade's extra step at the end of enemy_ai reads the speed of the motion list entry (+4, table 0xC6B60) or of the attack record (+8, 0xC6F10): **every one of them is 0.0** in the arcade program ROM (and in THOTD.EXE's copy 0x5EF234), so the step returns at once (`speed == 0`) and never moves anything; the PC only dropped dead code. Side find (not PM): the PC's body_draw skips the root step for type 0x22 (Sophie) in chapter slot 5 (the prologue) |
| 13 | Sounds: PC cue numbers | sound layer | already: the runtime maps arcade codes to PC cues |
| 14 | Main loop: fixed 60 ticks / s (the arcade 57.52 Hz) | runtime rate | **done** 2026-10-20: Game Speed has a third value, Auto, the new default (launcher.ini `gamespeed=auto`, `--game-speed auto`): arcade games tick at the arcade's 57.52 Hz, PC Mode at the PC's 60 Hz (pc_main_loop 0x419168); Arcade / 60 Hz chosen by the player apply to both. The ini now stores auto / arcade / 60; an old `gamespeed=0` (the only value the old default wrote) reads as Auto, 1 stays 60 Hz |
| 15 | Ranking, endings, bodies, HUD (PM's own rules) | phases 4 / 5 | **done** 2026-10-20 (pcmode5). **Bodies** (`player_char_models_set`, player_body.cpp; PC 0x50C114): in PM the two cut-scene bodies get the seats' characters' types (+0x10: 2 Rogan, 0xD G, 0xE Rogan 2, 0xF G 2, 0x22 Sophie, 0x1A Researcher; No Player = Rogan's 2), not the title code's pair; 4P with only players 3 / 4 in (coop4's set B): their characters. The game-over scene and a join use the same types (0x51F438). **Car** (`player_char_props_draw`, prop_kinds.cpp; PC 0x496F2C): the seats' characters' models (+0x14: 0x188 / 0x189 / 0x1D38-0x1D3B); 0x1D38.. are PC-only models, so with arcade models (or headless) the arcade's 0x188 / 0x189 are drawn (`pm_model_drawable` -> GameView::hasModel). **Endings** (ending.cpp `ending_body_setup`; PC ending_p1_a .. ending_p2_b 0x4AA52C-0x4AB148): the seat's character's type; the -1.2 height for types 14 / 34 is the same code on both. **Ranking** (PC mode67_ranking_pc 0x44D0C4, FUN_0042593c, InsertScore 0x425798 / FUN_0042556c, screen_high_scores_pc_mode 0x4234E0): in PM the game's end (`game_ending_rank`) skips the arcade's 3D name entry; a game over or the end goes back to the menus (main.cpp `pcModeFinish`): per player a row (score, character, stage = the chapter 0..3, only while the player has lives, else the last game's: the PC's static 0xC72388), in when score >= 1 and >= the tenth of the table with the other players' scores sorted in; the name entry (screen 15) per player, the row over the tenth and the PC's swap sort (an equal score stays below), then High Scores on its PC Mode table (screen 9's four columns: name, stage, score, character with its small picture 0xF3..0xF8). The table: `game/pcmode_scores.{h,cpp}`, defaults = THOTD.EXE table 1 (BART 62000 Rogan Stage 4 .. ROHAN 4000 G 2 Stage 1), saved as pcmode_scores.txt in the settings folder (never the PC's SCORES.CFG). 4P: rows for all four seats (seats 3 / 4 named on the Player 1 / 2 entry screens). Headless: `PC MODE:` / `PC MODE RANK:` lines (HOTD_PM_NAME files the rows). **HUD**: already done (12 bullets, hidden while the reload timer runs; continues text, row 8). Left (pmmisc 2026-10-27, not PC Mode items): the PC's prologue Sophie root-step rule (row 12's side find: body_draw 0x477B00 skips the position write-back for type 0x22 in chapter slot 5) belongs to the attract's prologue, which PC Mode never plays (its start 0x43BAD8 goes straight to set_mode(4); only the attract's 0x43BB28 sets mode 9); `hangedman_death_fall` 0x49A03C only tests state 4 (boss mode: 270 blink frames), a Boss Mode / boss item (pmboss's area). **pmrest** (2026-10-27): the 270 frames are in (Boss Mode's PC rules, docs/teaser_status/bossmode.md); the prologue rule stays documented only: our attract has no PC-rules mode (the PC menu style changes only the menus; the attract, its prologue included, always runs the arcade logic with pcmode 0), so the PC's attract-only rule has nowhere to apply. For the record: THOTD.EXE body_draw 0x477B00 skips writing the root step back into the position for creature type 0x22 (Sophie) when the chapter slot is 5 (the "1998.12.18" prologue, mode 9 = 0x43BB28's attract start), so on the PC the prologue's Sophie stays where her scene puts her |

### The bosses and the rest of row 10 (pmboss, 2026-10-21)

The four bosses have only their arcade port (no Game<Pc> form), so their PC forms were read from THOTD.EXE (the
disassembly; Ghidra's names in that range are often shifted, so each PC routine was found by its state table or
its constants). In PC Mode (`pm_boss()`, never in arcade games or in Game<Pc>) each coprocessor use that decides
something takes the PC's form on local `pm::Matrix` values (the PC's stored matrices: `pm_m0` / `pm_m1` / `pm_m5` /
`pm_m6`), the PC's normalise `boss_unit` (1 / table square root of (x x + y y) + z z), its pc_vec_angles, its table
square root distances. Every boss file has its own copy of the helpers block; all were checked (docs/proof.md). PC
rules found on the way (the arcade differs):
- **Chariot** (file 0x513000; routines 0x515400..): walk / retreat step (rotate y, transform), facing (act 12 / 14:
  pc_vec_angles, its own fold), its camera-space point (stored 5) and the hit box's (stored 0), the death's view
  test (radius 20). The retreat's step is 0.0 on the PC (the word before its table; the arcade reads 2.2e-12). The
  hit reaction's push-back (0x5169CC) is the PC's own: x back to -40, z to 80 in Chapter 1 and to -100 in Chapter 4
  (the arcade: z to 80, or 60 after Chapter 1). The hit box is on the depth list (shootable) only once the Chariot is
  down (the PC's flag 0xDA67FC: 1 at set-up in Chapter 1, else set when act 2 lands; `pm_chariot_landed`). The
  armoured part hook's hit box point, the home point and the falling armour pieces (angles: pc_matrix_angles at
  +0x1418) use the part matrix of the arcade's body drawing in the PC's layout (`pm_copro_current`,
  `pm_copro_words_at`: PM draws with the arcade's renderer) times the PC's stored 1.
- **Hangedman** (file 0x497070; states 0x5E9B74): the claw's screen point times 0.0020161290 (1 / 496 as a float,
  also the Hermit's spit and the Magician's aims: the arcade divides by 496), through stored 1; the distance by the
  table square root; the claw follows the camera in the **odd** patterns 1, 3, 5, 7 (the arcade: 1, 3, 4, 6); the
  hover point (0x49CAD0) scales the path point by **2** (the arcade: 3), takes the odd patterns as its first group
  and adds 30 to x in its second (the rematch's (2 x + 40) + 30); the dive approach / dive on the PC's helpers; the
  PC's camera_path_eval 0x461980 chops the frame to an integer (`boss_path_frame`); the fly-by's moves; kind 17 (the
  scene's researcher) carried on the PC's helpers.
- **Hermit** (file 0x4EB224): its camera-space point (stored 5, the distance the script's actions 25 / 26 test); the
  spit's aim (stored 6) and the ball's clip point back through stored 6; the spit ball's speed 1.0 (the arcade reads
  a program word as a float); the mouth point (0, -2, -5) from the head part times stored 1; act 11-15 have no
  transform on the PC (the arcade's result is unused).
- **Magician** (file 0x4CF570; states 0x5EF084): fly-to / dying / attack / fireball / barrage normalises, the attack's
  target (stored 0 then 1) and distance, the hands' and parts' points through stored 1, the fireball aim and the
  barrage aim (x 0.0020161290), the intro object's angles, the barrage circle on the PC's helpers (path frame
  chopped).
- Not changed (drawing: PM keeps the arcade's renderer): the bosses' effects, sparks, dust, title cards, body draws.
- Round 2 (2026-10-27, merged with main): two more whole routines proven in `hotd_proof pmmaths` from the
  disassembly (Ghidra's decompiler hides the x87 code there): the Magician's fireball aim 0x4D3DEC (the PC's
  offsets per kind / count / players are the arcade's: +-124, +-82 / +-164, 32; only the 1 / 496 multiply differs)
  and the Hermit's spit 0x4EE4C0 (stored 6, the table square root unit vector, speed 1.0). 173 routines, 0 differ.
  What is still not proven as a whole routine (proven only through their helper copies and the restated pieces):
  the Hangedman's claw / dive / fly-by moves and the Magician's fly-to / barrage, whose PC routines are long state
  machines with task creation, motions and sound calls; their PC maths pieces are the helpers above.

The rest of pcmode7's list:
- **held_turn / the drum's tumble / the axe release** (kit file 0x4E1A6C): PM runs the PC branches (`pc_path()`) on its
  own stack: pc_matrix_angles8 = `pm::matrix_angles8`, tumble_corners 0x4C5924 = `pm_tumble_corners` (into the
  arcade's corner buffer that tumble_update reads), the PC's step rule (above 3 moves on). The axe release (0x4E53E8)
  multiplies stored 1 by the hand's node matrix (the arcade body drawing's, in the PC's layout). **pmmisc**
  (2026-10-27): the tumble's start and bounce too: in PM held_object_flight runs `pm_tumble_update` (PC 0x4C5B1C:
  the ground test 0x4C5A10 on the arcade's floor search, then `pm::tumble_start` 0x4C5B70 and `pm::tumble_bounce`
  0x4C5DC4 with the rock side 0x4C3570, the axis turn 0x4C1DD0 times the current matrix (Matrix::mul, 0x4C2610), the
  lowest corner / floor lift 0x4C5A64 and the matrix angles 0x4C2F78) instead of the arcade's coprocessor form and the
  runtime's stand-in for its unmodelled 0x85; `hotd_proof pmmaths`: those 4 routines, 20000 cases each, 0 differ (177
  routines, 3,546,100 cases in all). **pmrest** (2026-10-27): the kind 38 barrel prop. Its PC flow is the arcade's:
  fall 0x4323A4 (two bounces) -> 0x4324A0 (spins while rising; at vy <= 0 on to 0x432500) -> the tumble 0x432500 ->
  settle 0x432648 -> rest 0x4326D4, the same states and tests as the arcade's 0x4430 .. 0x4700 (the arcade's extra
  *_start routines are Watcom's inlined stubs on the PC). The "tumble while still rising" was Ghidra's FUN_00432448 /
  FUN_004324F8: dead code, no caller anywhere in THOTD.EXE (scanned: no pointer, no call). What differs is the maths:
  in PM `barrel_tumble_step` builds the box on PM's stack with the barrel file's own copies (identity 0x42FF74,
  translate 0x42F704, rotate z 0x42FA2C (+0x34), y 0x42F924 (+0x30), x 0x42FBB8 (+0x2C)), the PC's tumble_corners and
  tumble_update, and the rest's view test is 0x430BD8 (pc_view_test's copy, the model's radius). **The tumble's
  ground test** 0x4C5A10 calls the PC's ground_height 0x48A254, which is `fld dword [floor_y]; ret`: the PC has no
  floor search there, so a corner at or under floor_y starts the tumble (pm_tumble_update reads floor_y; the drums
  too). The PC calls 0x48A254 from 18 places (the Hangedman's fall, kind 70, the bats, the Molb, ...), where the
  arcade searches the floor quads; using floor_y for all of them in PM was tried and left Chapter 4 event 1 waiting
  for an enemy forever (evt_WAIT_ENNUM, `--pc-char 5,6`), so PM kept the arcade's floor search there.
  **pccap (2026-10-27): settled, all 18 on floor_y** (`ground_height`, floor_motion.cpp: `pc_path()` = floor_y).
  The 18 call sites (THOTD.EXE scan for `call 0x48A254`): hangedman_death_fall 0x49A0FA / 0x49A1CA, the hostages
  0x4AC4A5 (rescue: kept on the ground) / 0x4ACC83 (held: `floor_y = ground_height` = no change on the PC), the Molb
  0x4BD3AB / 0x4BD3E5 / 0x4BD6F3 / 0x4BD732 / 0x4BE3A7 / 0x4BE525, the tumble 0x4C5A3B, the bats' shadow 0x4E9777,
  kind 70 0x4FE675 / 0x4FE78C / 0x4FE9E6 / 0x4FEAF6, the knife's and the projectile's shadows 0x502180 / 0x5031D8.
  Evidence: (1) floor_y has no missing writer: our PM runtime's floor_y per chapter / event (HOTD_STATE_TRACE, a
  whole game) is the same sequence of values as the PC's in the harness capture `all_pm` in every event both played
  (Chapter 1 0 / -25 / -75 / 6.75 / 1, Chapter 2 -6.876 .. -615, Chapter 3 -57.774 .. -338.192, Chapter 4 0).
  (2) The stall was the Hangedman: the arcade's hangedman_hit 0x4FC40 asks, in Chapter 4, whether ground_height of
  his position equals floor_y (no floor quad under him: back to state 8 instead of dying); with ground_height =
  floor_y that is always true, so the Chapter 4 Hangedman never died. The PC's hangedman_hit 0x499B20 does not call
  ground_height there: it compares the constant -180.0 (0x5A32A4) with floor_y (0x499C54; floor_y is 0 in Chapter 4
  on the PC), so he dies; and his death fall in Chapter 4 lands on the constant -180.0 (0x49A0E5 / 0x49A1B9), else on
  ground_height. Both are in (`pc_path()`, boss_hangedman.cpp). Chapter 4 strict with `--pc-char 5,6` and with
  `--pc-mode`: the ending is over (34605 / 28847), no stall. `hotd_proof pmmaths`: the barrel file's five matrix
  copies, its view test, ground_height and the ground test added: 185 routines, 3,686,200 cases, 0 differ.
- **crand** (the dogs, 0x4DEC17 / 0x4DEFC1): THOTD.EXE 0x578A6B is Watcom's rand (seed x 0x41C64E6D + 0x3039, bits
  16-30); srand 0x578A8F is called only by the debug menu's random autofire and by an unreferenced start-up routine
  (0x43B5BC), so the seed is the library's 1 at start-up; PM: `pm::crand` on `pm_crand_seed` (1, never reseeded).
  Nothing else calls it in play (0x45BBE4, its other caller, is unreferenced).
- **pc_wide_slot_kind** 0x5CD2FC: written before 29 heap allocations (0x4122D0) and never read: an allocation tag
  for the PC's debugging, no effect; no runtime copy needed.
- **pc_view_test / pc_view_test2** (and the boss files' copies): `pm::view_test` (stored 0, the PC's projection 640 /
  -620 and viewport -320..320 x -240..240 read from the PC harness captures). **Where those numbers come from**
  (pmrest): 0x419E28 sets the projection at the game window's start: 496 x (screen width / 496) x s and -496 x (screen
  height / 384) x s (s = the viewport's fit, 1 for the full window), the viewport (0x5688F0) centred on the screen:
  640 x 480 gives 640 / -620 / +-320 / +-240; the PC's 320 x 240 mode gives exactly half of all four, and halving
  is exact in floats, so the test decides the same at both resolutions. PM keeps these logic numbers whatever our
  window's size or aspect (pm::view_test has no window input); only the drawing scales. PM now runs the PC branches that use
  them: the kit's thrown objects resting off screen, the axe thrower after its throw, ENGuardZombieRunAfterResearcher
  with no prey (the PC's own guard: leaves once out of view).
- Online PC Mode (offered since onlinepm, "Online" below): every PM runtime field that lives between frames is now in rollback's snapshot
  (net/rollback.h `RuntimeState::Pm`, pmmisc 2026-10-27): pcmode, pm_char, the PM matrix stack, pm_camera /
  _inv / camera0 / camera0_inv, pm_molb_height_bits, pm_crand_seed, pm_chariot_landed, pm_reload_at, pm_clock_ms,
  pm_continues / credits / credits_seen / free_play, pm_auto_reload, pm_blood_tasks, pm_last_killer.

Not checked against a PC capture yet: rows 1-6 and 9 are ported from the read PC code (docs/decompilation.md); the PC
captures game_pm*.txt are proof captures for single routines on Game<Pc>, which already prove the PC branches these
rows copy (enemy_part_hits, enemy_ai turn, shot_resolve: docs/proof.md). `pm_last_killer` starts at 0 for every
runtime game (the PC's 0xDA0AE8 is not reset between games: open whether that matters).

## The PC's body transform and an exact Chapter 1 (pmbody, 2026-10-27)

The differential run (docs/proof.md) now shows **PC Mode Chapter 1 the same as the PC from the first frame to the
end of the capture (17,289 frames)**: rand, events, enemies, players, every shootable object's state, hit points and
position (to 9 digits). Before, it was the same to frame 424 only. What it took, each found by the next first
difference and checked against THOTD.EXE (`hotd_proof pmmaths`):

| Cause | PC form now used in PC Mode / Boss Mode (`pm_body()` / `pc_path()`) |
|---|---|
| Body / part positions came from the coprocessor | body_draw 0x477B00, its part draw 0x476B70, object_draw 0x4C3B80 and the part hook 0x51D014's turns run the PC's transform on PM's own matrix stack, in step with the coprocessor's (the coprocessor still draws). Positions written back, the direction (0x475F38), the home point, view_pos and every part's hit-sphere centre are the PC's. The body file's and the hook file's helper blocks (0x474CE8, 0x51AF1C: translate, turns, transform, mul, mul_stored, invert 0x475B68, matrix angles 0x475F38) proven |
| Camera angles | camera_look_at 0x461A30: the PC's vec_angles (steps of 16) and roll 0 |
| Camera paths | camera_path_eval 0x461980: integer frames and the PC's Hermite form 0x461858 on the PC's own keys (CAM\ALLSTAGE\CAMALL.RBH, read from the PC files: its tangents are rounded, its values differ in the last digits from the ROM's; without the PC files the ROM's keys) |
| The game camera's look-at | 0x45FF68: x 1 / (n + 1); the PC reads last frame's look-at z for the angles (its own store order); the look weight 0x461690 on its table square root and 1600.0 |
| Camera turn-back, view position | the turn-back's angles on pc_vec_angles; camdr_mode3_frame 0x4CA680 and its heading 0x4C9D28 on the PC's matrices |
| Rain drops (rand timing) | 0x431678 on the PC's stored matrices 0 / 1 |
| A blend before the first drawing | the PC's body_init leaves frame_data at the motion table entry (&table[m], 0x478D4C), so such a blend starts from the table's pointer words: PC Mode reads them from THOTD.EXE in the PC files |
| floor_probe | the PC's quad test 0x489330 is an empty stub: no floor hit ever (a kind 11 death on water does not sink) |
| Murrer floor modes | 0x467B08 / 0x467B70 / 0x467BF8 add pc_molb_height_offset |
| Barrel break | 0x432804: height = floor_y + the frames left |
| Held hostage | 0x4ACA2B: the hands through the PC's stored 1 (the hook's anchors keep the PC's translation; the hands +0x214 / +0x244 the PC's whole matrix) |
| Murrer path, heading moves, jump arcs | spline_advance 0x4677FC, obj_move_heading 0x4C589C, jump arcs 0x46A984 / 0x46AA70 (reciprocal multiply, x87 24 bits) |
| Chapter 1 room ride | 0x4310B0's table sine |
| The Chariot's hooks | pm_copro_current is PM's own part matrix during the body drawing (hit box, home, kept part matrices) |

Left: the 3 x 3 of the head / skin anchors stays the coprocessor's (the arcade eye-beam code reads it); the PC's own
eye beam 0x51E258 is not ported. Two trace-only differences remain in the run: the PC capture keeps player 1's lives at
3 (`-InfLives`), and ReWrithe's bullet-impact mark task has a stale "shootable" bit (it is never added to the shot
list). Boss Mode: its fights now take the bosses' PC forms too (`pm_boss()` = PC Mode or Boss Mode: the PC runs its
own boss code in Boss Mode; pmboss had them for PC Mode only). Against the Boss Mode captures (9 digits): **the Chariot
and the Hangedman alone are the same as the PC over their whole captures** (were: a Chariot hit one shot later at 1703,
the Hangedman's fight different from its entrance); the Hermit has no rand / hit difference (positions 1.5e-5 apart
from 534: probably its draw mode, the PC's simpler "draw type 1" body routine 0x478094, not ported); the Magician no
rand difference (only which effects are listed as shootable, 1984 / 3626); All Bosses' first rand difference moved
from 4288 to 12481. (All of it closed by pmrest2, below.)

### pmrest2 (2026-10-27): every PC capture the same as the PC

All six PC captures are now **the same as the PC over every frame** (`pc_diff.py diff ... --tol 1e-10 --inf-lives`,
9 digits): PC Mode Chapter 1 (17,289 frames), Boss Mode Chariot (2,660), Hangedman (3,664), Hermit (2,446), Magician
(9,162) and All Bosses (18,726). What it took:

| Cause (first difference before) | PC form now used in PC Mode / Boss Mode |
|---|---|
| Hermit positions 1.5e-5 apart from 534 | not the draw type: the game camera's **look mode 2** (record +0xF0, the Hermit's fight looks up). THOTD.EXE 0x45FF68 has no quarter turn there (the arcade turns the view matrix by -/+0x4000 on the coprocessor): with mode 2 it sets yaw and roll to 0 and the pitch to -(pc_atan2(dy, table square root of dz dz + dx dx) x 16) of the eye minus the look-at (0x46009B); look mode 1 does nothing more on the PC. `pm::look2_pitch`, camera.cpp |
| (task 1) draw type 1 | THOTD.EXE 0x478094 and its part routine 0x477EC4 are the arcade's 0x37D80 / 0x37B80 on the PC's stack (the same flags, frame, root and home rules); PC Mode / Boss Mode now run them on PM's stack too (the home part through stored matrix 1). No capture difference was traced to it; not run as a whole routine in pmmaths (it draws models), its helpers are the body file's proven copies |
| (task 2) the eye beam | 0x51E258 / 0x51E4C8 (camera / target, 15 up): on the part's matrix turned about y by -0x4000, times stored 1, inverted, the point through it, the hook file's vec_angles (its own fold), angles kept to 16 bits, clamped to +-0x2000, eased 0xC0 a frame; the target beam does nothing without a target. `pm::look_turn` turns PM's head matrix too (before: only the coprocessor's), and the coprocessor gets the same angles |
| Chapter 1 rats at 5938, table props at 6303 | THOTD.EXE's kind table has `jmp task_exit` for kinds **131, 137, 146, 147, 152, 175** (the clock was known): the PC has no rats (175), no table / chairs / plates group (147), no Chapter 2 groups 131 / 152 and no balls (146). PC Mode / Boss Mode end those group tasks at once |
| Chapter 1 kind 134 pieces 1 ulp apart (5940) | the falling pieces' gravity: the PC multiplies (rand % 20) by 0.001 (0x47BA96), the arcade divides by 1000 |
| Magician fireball speed (4070), All Bosses rand (12481) | not a rule: the capture's `-InfLives` raises player 1's lives to 3 before every tick, and the bosses' aggression level (0x305D0: a life lost lowers it) sees that. The replay now does the same: `HOTD_LIVES_MIN=3` (main.cpp, with HOTD_GUN_REPLAY) |
| Shootable-effect lists (Magician 1984, All Bosses 6325 / 6809, Chapter 1 4970 / 9129 / 9565 / 10003 / 10109 / 15723) | trace only: effect tasks never write +0x58 / +0x5C / +0x70 and task_create does not clear them, so they hold the heap's last words (the PC's heap is another one). pc_diff.py ignores them (impact_effect, the bullet mark, prop_collapse_anim, severed_part_fall, shot_effect_flash, hermit_title_task, part_class17_anim) or compares them as state 0 (the kit zombies' held / thrown objects, a lid's falling copy); a PC record can list a task twice in its first tick (one record per task now); a chapter-change tick's PC record is taken after its heap reset (objects not compared there); `--inf-lives` leaves the lives out |

`hotd_proof pmmaths`: the look mode 2 piece and both beams added (324 routines, 0 differ). Chapters 2-4: below
("PC Mode Chapters 2-4").

### PC Mode Chapters 2-4 (pmrest2, 2026-10-27)

A new whole-game PC Mode capture (`run_pc.ps1 -Ticks 85000 -TimeoutSeconds 3000 -InputScript scripts\game_pm_aim.txt
-InfLives -Map tools\pcharness\pccap_map.tsv`, kept in the worktree's build\pcruns\pccap_pm_all; the watchdog stopped
it in Chapter 3 or 4) replayed as above (`--pc-mode --difficulty 0`, `HOTD_LIVES_MIN=3`). First divergence per
chapter (game frames: Chapter 2 starts at 15180, Chapter 3 at 32699):

| Chapter | Before this pass | Now |
|---|---|---|
| 1 | the same | the same (all fields, 9 digits) |
| 2 | objects at 17864 (props the PC has not), rand at 17873, a hostage penalty at 20113 | **the same to its end** (rand, events, enemies, players, every object within 1e-3; to 9 digits but one kind 70 position 1 ulp apart for one frame, 20478) |
| 3 | (not reached) | the iron ball zombie (kind 32 with the ball, Chapter 3 event 1): positions the same, one hit box fewer at 33633; rand from 35463 (pmch34: **the same to its end** but the dropped ball's hit boxes, below) |
| 4 | (not reached) | not compared: the run differs from Chapter 3 on, and a PC capture cannot start at Chapter 4 (pmch34: **the same to its end**, 69380 on both; the ending's last scene 222 frames longer, below) |

Causes found and fixed (all `pc_path()`: PC Mode and Boss Mode only):
- **prop_object 0x47B2C4** ends at once for sub-types 5, 10-14, 41, 59, 64, 70, 71, 82, 83, 91: the PC has none of those
  props (Chapter 2 event 12's extra props).
- **The held hostage 0x4AC89C / researcher_shot_by_player 0x4ACFB4** draw one more random number after the arcade's
  pick and clear a shooter bit by it (player 1's on an odd number, else player 2's): with one shooter the life is
  taken from the other seat half of the time (a PC quirk; hostage_shot 0x4ACD28 has no such draw). `pm_shot_bit_drop`.
- **x / 10, 100, 1000, 10000**: THOTD.EXE has no fdiv by these constants at all (82 fmul by 0.1 / 0.01 / 0.001 /
  0.0001), so every such division in the shared code is a multiply on the PC's path (`pdiv`, 49 sites: gravities,
  random scatter, speeds).
- **jump_arc_velocity(_b) 0x46AB60 / 0x46AC40**: x / z times 1 / f (the arcade divides).
- **kind133_prop_grid** (Chapter 2 event 4): the table sin / cos (0x479BAC / 0x479BCC).
- **The iron ball** (Chapter 3): iron_ball_pos_swing 0x48D46C (the hand matrix, three nodes, stored 1), the node place
  0x48EF20 (body_node_pos_at_frame), the drop's direction and view test, the hit boxes' camera-space point, all on the
  file's own helpers 0x48A630 (proven in pmmaths). Left: the hit boxes' count (33633), then the play.
- pc_diff.py: more heap-leftover effect tasks (part_spark_anim, prop_anim_once, hit_spark), the iron ball's hit boxes
  and second ball compared as state 0.

### PC Mode Chapters 3-4 (pmch34, 2026-10-27)

The whole-game capture re-recorded (`run_pc.ps1 -Ticks 90000 -TimeoutSeconds 3300 -InputScript
scripts\game_pm_aim.txt -InfLives -Map tools\pcharness\pccap_map.tsv`, build\pcruns\pccap_pm_all of worktree
wt-pmch34, not kept in private): this time the PC played the **whole game** (76,522 game frames, the ending, the
ranking; the old one had stopped in Chapter 3 or 4). Replayed as above (`--pc-mode --difficulty 0 --lives 11`,
`HOTD_LIVES_MIN=3`, `pc_diff.py diff --inf-lives`):

| | Before (pmrest2's code on this capture) | Now |
|---|---|---|
| Chapter ends (PC: 15180 / 32699 / 50862 / 69380) | 15180 / 32699, then the play differs (rand from 35463) | **15180 / 32699 / 50862 / 69380** |
| Chapter 3 | hit boxes at 33633, a door at 35032, rand at 35463 | the same (rand, events, enemies, players, every object to 1e-3) but the dropped ball's hit boxes (33633..) |
| Chapter 4 | (diverged) | the same to its end (1e-3; 9 digits but the kind 70 fall below) |
| The ending | (diverged) | the same to 76019; its last scene ends 222 frames later (76742, the PC 76520) |

Causes found and fixed (all `pc_path()`):
- **The dropped ball's hit boxes (33633): not a rule, the PC's heap.** iron_ball_draw 0x48E1BC tests its owner's
  +0x328 bit 0 through the ball's owner pointer, also after the owner's task has ended: on the PC the dead
  zombie's block is freed (task_exit, mem_free 0x412318 only clears the block's used bit) and taken again by the
  next task_create of that size (a held kit object 0x4E74EC, whose +0x328 is 0xA1: bit 0 set), so the falling ball
  makes hit boxes again; ours keeps the arcade's heap, where the block stays free. Seen in the capture: owner
  0x2043AF20 freed at 33606, reused at 33632. The PC's allocator (0x562C00: first fit from the heap's start over
  size words, bit 31 = used, free neighbours merged while searching, split when more than 3 bytes remain; blocks
  zeroed) is simple, but copying the result needs every PC allocation in the same order (tasks are the arcade's
  sizes + 8, but the bodies' part trees and buffers are the PC's own), so it is **not ported**; `pc_diff.py diff --no-ball-boxes` compares
  without the hit boxes for now (the play does not change: no shot hit one in the capture).
- **kind 162's move toward its point** (obj_kind162_turn 0x49F2C3, a Chapter 3 door): times 1 / the table square
  root of ((dx dx + dy dy) + dz dz), not the coprocessor's normalise (35032).
- **prop_break_pieces 0x482BF8** takes sub-type 121 (Chapter 3's machine item, prop121_kikai_item) into 116's case
  (0x482C49): model 0x15C0, lift and speeds, three more random numbers a piece (rand at 35463: 91 draws, ours 73).
- **The iron ball's throw / hit / pull** (0x48D740 / 0x48DA14 / 0x48DBA4): the throw's point (rand - 127.5) x 150
  (120) x 10 x 1 / 63240 through stored matrix 6, the direction times 1 / the table square root, the flight's view
  point through stored matrix 5 and back through 6; hit / pull: identity turned about y, (0, 0, 1) through it, the
  distance from the start by the table square root (`pm_ball_ahead` / `pm_ball_dist`, ch3_kinds.cpp; 44131).
- **hermit_death_sink 0x4EE948 step 2**: once event flag 0xFD is set the Hermit's body is freed and the task ends
  (the arcade draws it until the chapter ends; 50376).
- **hangedman_rematch_intro 0x49CA74** (Chapter 4): no 180-frame wait: the life bar, the aggression and state 4 at
  once (54230).
- **magician_dying 0x4D1EE4 steps 0x11-0x14**: the death flash and a music fade, then the last 180 frames at once
  (the arcade's palette dark / back waits are not there): Chapter 4 ends on the PC's frame (was 716 later).
- pc_diff.py: more tasks whose words are heap leftovers or that the PC has not (the iron ball's drop task, the blood
  marks 0x1DCD0 / 0x1DB30, kind7_falling_piece, the Chariot's hit box / spark / glow, the runtime's player tasks).

Left: (1) the dropped ball's hit boxes (the PC's heap, above). (2) kind 70's home height: +0x2D4's y is about
1.4e-3 off the PC's in some poses (20485: -31.281395 vs the PC's -31.279953), so its view point and the airborne
hit's fall target (kind70_fall_start) are 1e-3 apart (20486, 25669, 36712, under 1.1e-3), and one wall-attack arc
z is 1 ulp off for two frames (20478); the home comes from the body's home part (draw type 1), not found in the time
left. (3) The ending's last scene (scene_person 0x58AC0, src/game/attract.cpp, from 76020): the PC has no such object
there and ends 222 frames sooner. PC Mode end frames (autofire): Chapter 1 16289 (unchanged), Chapter 4 strict
`--pc-char 5,6` the ending over at 42008 (was 29416: the Hangedman's rematch starts 180 frames sooner, which changes the run).

## 4 players in PC Mode

**Select screen (pcsel4, 2026-10-20; ours, the PC has 2 players only).** With Players 3 or 4 (`cfg.players >= 3`, a local
game) screen 16 shows four quarter panels instead of the original's two (`Frontend::pcModeOpen` / `pcModePanel4`,
frontend_pcmode.cpp); with 1 or 2 players it is the original screen, pixel for pixel. Layout (640 x 480, so the
same in 4:3 and 16:9, where the menu is pillarboxed): the background 0xA4's two stones each hold two panels, 288 x
176 at x 0x10 / 0x150, y 0x30 (P1, P2) and 0xE6 (P3, P4); each panel: a 2-pixel frame in the player's colour (P1 red,
P2 blue, P3 green, P4 yellow), the portrait (+0x18) at half size (2 x 2 averaged) top left, "Player N", the name
between the arrows 0xD8 / 0xD9 (hover 0xDA / 0xDB), Ready (yellow "Ready!" while ready) or a grey "CPU", and the
five ratings of 0x420080 (same labels, same thresholds) in white 24, 18 pixels apart. No Start: a line at y 0x1A0
("Waiting for P1, P3") until every seat with a character is Ready, then `onPcModeStart` runs at once; Exit as before.
- **Seats.** The defaults for seats in play are Rogan / G / Rogan 2 / G 2 (a pick from an earlier visit is kept);
  seats out of play start at No Player (they may pick and join later; No Player joins by the join rule). Player 1
  never gets No Player (as the PC). A computer seat (p2bot.. p4bot) gets a random character 1-6 and is ready; only
  the mouse changes it. A Ready seat's arrows are locked until it takes Ready back. All four picks go to `gPcChars`.
- **Input** (main.cpp `pcSeatEvent`, before the menus' own pad / key handling): a pad assigned to a seat
  (`Pads::playerOf`): d-pad / left stick left / right = the arrows, South / Start = Ready, East / Back = take Ready
  back (not ready: the menu's Escape); the seat's keyboard keys (game/keybinds.h, so rebinding applies): Aim Left /
  Right = the arrows, Fire / Start = Ready, Reload = take it back (defaults: P1 1; P2 arrows, Space; P3 A / D, F, R;
  P4 J / L, H, Y). The mouse clicks any panel. Light guns use the pointer as a mouse. Escape = Exit.
- **Check:** `--frontend-screen 116` with `HOTD_FE_PCSEATS=players[,ready mask[,cpu mask]]` and `HOTD_FE_PCCHARS=a,b,c,d`
  shoots it; `--input-test` has 11 checks (4 players: P2 / P3 keys, P4's virtual pad pick, Ready by key / pad /
  mouse click, the locked pick, Ready taken back, the start with all four picks; 3 players with a computer P3;
  2 players: the original panels; Escape).

ROADMAP: 4P applies to PC Mode too. The 4P mode runs the arcade logic with seats 3 / 4 in the 4P block
(docs/coop4.md); PM needs only a character per seat: `pm_char[4]` covers all four seats and every PM rule is
per seat (lives, rounds, damage, hit size, the reload timer). The select screen gets four panels (two rows of two,
or the PC's two panels with a 1P-2P / 3P-4P page switch); characters may repeat (the PC lets both players be the
same). Bodies: seats 3 / 4 already use the "Rogan 2" / "G 2" bodies (types 14 / 15) in 4P; with PM each seat's body
should follow its character instead (phase 4). The No Player join rule generalises to "a seat joining with No Player
takes the first of Rogan / G that no other seat has, else Rogan".

## Online (onlinepm, 2026-10-27; protocol 16)

PC Mode plays online (ours: THOTD.EXE has no network play): two PCs, two PCs with two seats each, up to four PCs. The
host's Host page **Game Mode: PC Mode** (`--pc-mode`) and **Your Character**; each joiner picks its own character on
its PC (Join page **Your Character**, `--net-pc-char a[,b]`; Auto = the seat's default Rogan / G / Rogan 2 / G 2, never
No Player); the waiting rooms show every seat's character and the host's Start fixes them. The host's settings carry
PC Mode, the four characters, its Continues (0-9) and Free-Play; every PC starts the game from those (not its own
Options), with the host's Auto Reload, at the host's game speed (Auto = PC Mode's 60 Hz, for every PC). The desync
hash covers PC Mode's between-frame fields (the ones in rollback's `RuntimeState::Pm`). Not online: Boss Mode.
Details and tests: docs/netplay.md "PC Mode online".

Part 2 (onlinepm2, 2026-10-27; protocol 17): a joiner of a running game plays its own pick from its first start (the
start's input carries the character; applied to a waiting seat, state 9, on every PC). A game over or the end ends the
online session (as PC Mode's end goes to the menus); then each PC's own players enter their names on that PC and its
own PC Mode table (pcmode_scores.txt) is saved: `pcModeFinish` takes a seat mask (online: this PC's seats). The
host's Continues / Free-Play are the one shared credit pool (a continue or a join takes one, the countdown restarts
when it changes: the same on every PC). launcher.ini keeps the Host page's Game Mode (`netpcmode=`) and the characters
(`netpcchar=a,b`).

## Extra Characters (pmextra, 2026-10-28; ours, not THOTD.EXE's; protocol 20)

User: "you never added the extra characters to the PC Mode." The cut-scene viewer's other actors (docs/coop4.md
section 14) are PC Mode picks 7-19, after the PC's six (0-6 unchanged: PC Mode with them is the 1998 PC's exactly).
Each plays by one PC character's **rules** (hit points, rounds, damage, reload, hit size, its body type in the game
logic, its car model, its small picture): only the **drawn body** differs.

| # | Name | Body (creature type) | Plays as | PC models |
|--:|------|---------------------:|----------|-----------|
| 7-12 | Researcher (1) .. (6) | 17, 19, 20, 21, 22, 49 (the man's body) | Researcher (6) | own model |
| 13-16 | Researcher (woman) .. (woman, 4) | 23, 25, 26, 27 (the woman's body) | Sophie (5) | own model |
| 17 | Dr. Curien | 60 | Researcher (6) | own model |
| 18 | Dr. Curien (Dead) | 78 | Researcher (6) | own model |
| 19 | Sophie (zombie) | 67 | Sophie (5) | **none: Sophie's body** (the pick screen says so) |

- **Code:** `port::Game::pm_extra` / `pm_rules_char` (port/game.h; `kPmCharCount` 20): `pm_table(7..19)` is the base's
  row under the extra's name, so every PM rule (PlStartInit, the reload, part_hit, ray_sign_test, the join, the
  bodies' types in player_body / ending / coop4) runs the base's numbers. The body: `Runtime::actor_pm_sync` (each
  frame, the viewer closed) puts each seat's extra type in `actorSwap`, the viewer's actor-swap draw (`actor_swap`
  for players 1 / 2 incl. the ending's own body tasks, `actor_dress` for players 3 / 4: men's / women's jump motion,
  Rogan's coat only on Rogan, Curien's head / cane arm, a part tree that differs keeps the base's body);
  `actor_areas_update` gives Curien / the zombie Sophie their arcade texture area (looked at again every 30 frames:
  their type loads with the chapter / the ending). Draws only: nothing in the game state or the desync hash.
- **Not swapped:** a cut-scene's fixed talking-head models.
- **The opening car (carchars, 2026-10-28):** each seat shows its own character (P1 driving, P2 beside; the car has
  two seats, also in 4 players). A seat keeps a car model where its character has one in the model set: arcade
  models, Rogan driving / G beside (the arcade's pair); PC models, the PC's six (0x188 / 0x189 / 0x1D38-0x1D3B, as
  THOTD.EXE). Every other seat (the extras; Rogan 2 / G 2 / Sophie / Researcher with arcade models; G driving or
  Rogan beside with arcade models) is built from the character's own body (`Runtime::car_seats_dress`, creature.cpp):
  its part tree and part models, seated (MO_p_temoti frame 0 with the thighs / knees a quarter turn; the driver's
  elbows bent, hands on the wheel; the passenger leaning back a little, hands in the lap), scaled 0.9 and placed in
  the car model's draw so heads and hands sit where the car models' are (polish 2026-10-28: fitted with G's body
  over Rogan's car model and Rogan's over G's, `HOTD_CAR_OVERLAY=13,2 HOTD_CAR_NOBODY=1`), the car body hiding the
  legs; Rogan's arcade coat, the belly skin and Sophie's strip as the actors; an empty hand for the players' types
  (their own holds the gun); Dr. Curien's head as actor_dress, his cane arm in the driving pose (only the hand turned).
  The cut-scene viewer's actors for players 1 / 2 too. Draws only (`car_seat_mark` notes the draw). Pictures:
  private\changelog\img\carchars\sheet.png, sheet_v2.png (the polish; first column the arcade / PC Rogan / G car models).
- **Pick screen** (frontend_pcmode.cpp): the arrows run 1..19 (P1) / 0..19 and wrap; under an extra's name
  "Extra Characters" (yellow), "Plays as Researcher / Sophie", the PC-model note (Classic 2 panels: under the name,
  the PC's portrait fills the room above the rows; Remake: under the portrait); 4 panels: "Player N - Extra". The
  portrait / small picture: the base's (the PC has none of the extras). A name too wide for the Classic arrows / the
  Classic score column: the PC's small font; still too wide there (namefit): a short name (`shortPcName`:
  "Researcher W2", "Curien (Dead)", else the words before " (" cut short). Remake pick: a long name drawn smaller
  between the arrows (`drawRLabel` with `fitW`). Names and lines through the lang system (en, ko / ja / ar / he
  machine-translated). The CPU's random pick stays 1-6.
- **Scores:** pcmode_scores.txt keeps 0-19; High Scores > PC Mode shows the name (`HOTD_FE_PMSCORES=<file>` with
  `--frontend-screen 109` shoots a table from a file).
- **Online (protocol 20):** characters 1-19 in the settings, CONFIRM and ROSTER; an input's start carries the
  character in flags bits 3-7 (was 4-6). `--net-pc-char` 1-19; launcher.ini `netpcchar=` 0-19; the Host / Join
  pages' Your Character cycles Auto, the six, the extras.
- **Tests:** `--pc-char 0..6` end frames the same as main's build (Chapter 1: 1,0 16933; 2,0 16289; 3,4 17098; 5,6
  16185, also `--models pc`; 0 16933; 4P 5,0,3,6 22093; Chapter 4 strict 5,6 42008; 2P 6,6 28468). Every extra 1P
  Chapter 1 (both models for 9 / 15 / 19): the run log identical to its base's (6: 15975; 5 / 13-16 / 19 the same
  lines); 4P 7,13,17,19 = 6,5,6,5 (22533), 12,16,18,1 `--models pc` = 6,5,6,1 (22278); Chapter 4 strict 2P 7,17 =
  6,6 (28468), `--models pc` 8,19 = 6,5 (30476), 4P 18,14,11,19 the ending at 37787. Renders: private\changelog\img\pmextra.
- **pmfix (2026-10-28):** 18 renamed "Dr. Curien (Dead)" (his eyes are closed; key `curien2` kept). A woman / man
  extra on Rogan's body (and every actor in the viewer) gets its own belly skin: Rogan's type has none (skin number
  -1, his arcade coat covers the waist), so Sophie / the women researchers had a gap between chest and hips
  (`Runtime::actor_needs_belly`, drawn from the chest / hips draws; players 3 / 4's copies too). Arcade textures of
  Curien / the zombie Sophie (user: grey / white patches in the Chapter 1 opening): not reproduced here (headless
  renders of 17 / 18 / 19 in the Chapter 1 opening, Chapter 2 through the prepared-ahead view, and the viewer's
  1-07 / 2-12 / 3-15 / 4-17 / 4-19 all draw them in their area 11 / 12 / 2 sets); the view now logs once per model
  "actor models of area N drawn from chapter 4's set S" or "actor model X (area N) has no texture set" so the next
  play test's hotd.log says which path ran. The zombie Sophie's belly skin used area 6 (galfix3 moved her parts to
  area 2): now area 2. Check: `tools/actor_check.sh` (every actor x role, 5 scenes, both models, HOTD_ACTOR_CHECK).
- **pmfix2 (2026-10-28): the textures' cause.** `SceneView::applyCast` runs every window loop; with the viewer
  closed it set `actorSwap` to -1 and emptied `actorAreas` every loop, and the next tick's `actor_pm_sync` put them
  back. Above the tick rate (Display Refresh on a fast monitor) the loops between ticks drew the extras in the
  chapter's texture set (the user's f630 / f658). Headless and 60 Hz runs tick every loop, so they never showed it.
  applyCast now leaves the runtime alone while the viewer is closed. Reproduced through the menu at 240 fps
  (`HOTD_MENU_PCMODE=19,0,6000` starts PC Mode from the menu, `HOTD_SHOT_LATE=1` shoots one loop after the tick).
- **Portraits (pmfix2):** each extra has its own pick-screen portrait (160 x 160) and small picture (24 x 24) in the
  PC's style, made from its model with the Gallery's renderer (`Gallery::makePortrait`, its own framebuffer) at the
  first start in the menu (one character a frame, ~0.8 s), from the player's files: PC models where the PC has them,
  else the arcade's (zombie Sophie, Dr. Curien (Dead)); head and shoulders turned ~10 degrees, a dark ground with a
  faint glow, the body fading out, the PC's grading; the small one the face on the PC's small pictures' ground
  colours. Kept in the settings folder (`pmportraits\`, `version.txt`; never shipped), shown through
  `PcSprites::addImage` as sprites 0x700 / 0x740 + character (`Frontend::pcPortraitId`): pick screens (Classic /
  Remake, 2 / 4 panels) and the High Scores tables. Testing: `HOTD_PM_PORTRAIT_SHEET=<png>` (the PC's six and the
  extras side by side), `HOTD_FE_PMPORTRAITS=<folder>` for `--frontend-shot`, `HOTD_PM_PORTRAIT_CAM`,
  `HOTD_NO_PM_PORTRAITS`. Sheet: `private\changelog\img\pmfix\portraits.png`.
- **Portraits (portraits2):** redone to match the PC Researcher (0xF2, drawn from type 0x1A): `Gallery::PortraitHead`
  aims the camera at the neck joint (distance in body units, a 30 degree lens), turns the head 25 degrees left, drops
  the shoulders 18 degrees (standing motion `MO_p_l_tomari`), lights from the front left; her grading and soft edge.
  Each extra its own flat ground colour (Dr. Curien grey, Dr. Curien (Dead) maroon); both Dr. Curiens
  further back (whole head and some shoulders), zombie Sophie a little (x1.2, portraits3). The small picture is the big render's head square, 4 to 1. Testing:
  `HOTD_PM_PORTRAIT_HEAD="fov,dist,panx,pany,yaw,pitch,headyaw,headpitch,headroll,frame,lightdeg,lightup,armdrop,armdropx"`,
  `HOTD_PM_PORTRAIT_ADJ="type,distmul,dpanx,dpany,headyaw,cropx,cropy;..."` (type 0: the men),
  `HOTD_PM_PORTRAIT_LOG`. Sheet: `private\changelog\img\portraits2\portraits_proposal.png`.
  `hotd_proof pmmaths` 0 differ; hotd_nettest all passed (new: inputs / settings / roster with 13, 17, 19; 20 refused);
  `--input-test` 0 failed (new: the arrows into / out of the extras, the pick reaching the game).
- **Portraits (pmpick, user 2026-10-30):** the big pick portraits on the PC portraits' black ground (sampled from the
  PC Researcher's 0xF2 corner; the coloured grounds only on the small High Scores pictures). Made and kept per model
  set: Video > Models Arcade renders the extras from the arcade models / textures (`pmportraits_arcade\`), PC from the
  PC models where the PC files have them (`pmportraits_pc\`; zombie Sophie from the arcade's); version
  `pmportraits 4`; a set switch loads / makes the other. An extra without its picture yet shows none (never the PC
  character it plays as); `Frontend::pcPortraitsChanged` builds the open pick / High Scores again when they arrive.
  Testing: `HOTD_MENU_PICK=a,b[,c,d]` opens the live menu's pick (with `HOTD_MENU_SHOT` / `HOTD_MENU_SHOT_PNG`).
  Remake tombstones (0xA4): near the mask's edge the PC backdrop's warm pixels (blue under red / green) are keyed out.
  Classic High Scores: the character column at 0x200 (was 0x1F0), its names never nearer than 0x20 to the score.

## Checks (2026-10-20, after `git merge main`)

Arcade unchanged (PM off): `--input-test` 1478 / 1478 (after merging boss mode, padfeel, firstrun); `--frames 40000 --autofire --mute --lives-kept 1` chapter 1
ended at 16695 (arcade and `--models pc`); `--strict --default-route` Chapter 2 17270, Chapter 3 27279, Chapter 4
28083, ending 34967. PM on: `--pc-char 2,0` (G) chapter 1 ends at 15809 (15975 after phase 7 rows 1-4, 2026-10-20; arcade checks unchanged: input test 1480 / 1480, 16695, 17270 / 27279 / 28083 / 34967); `--players 4 --pc-char 5,0,3,6` at 17895.
The PC proofs are untouched (no Game<Pc> routine changed: the new code sits in the arcade branches).

pcmode5 (2026-10-20, rows 10 / 12 / 15): arcade unchanged: `--input-test` 1480 / 1480; Chapter 1 ended at 16695
(arcade and `--models pc`); `--chapter 4 --strict --default-route` Chapter 4 28083, ending 34967 (both models). PM:
`--pc-char 2,0` Chapter 1 15975; `--chapter 4 ... --pc-char 5,6` (Sophie / Researcher) ending over at 38656 (renders:
Sophie's body in the ending scenes); `--pc-char 6,4 --models pc` 32897 (the Researcher's body); `--players 4 --pc-char
5,0,3,6` Chapter 1 17088; Chapter 1's car with `--pc-char 3,4 --models pc`: model 0x1D38 (Rogan 2) drawn; ranking:
a table of low scores + HOTD_PM_NAME=TESTER: `PC MODE RANK: P1 name=TESTER score=2580 char=5 stage=4 place=1`, the
file rewritten; `--frontend-screen 109`: the PC Mode table with the PC's defaults. `hotd_proof pmmaths`: characters
7 of 7, pm_fast_sqrt 0 of 2^32 differ.

pcmode6 (2026-10-20, row 10: PC turn / shot maths; after `git merge main`): arcade unchanged: `--input-test` 1562
checks, 0 failed; Chapter 1 ended at 16695 (arcade and `--models pc`); `--chapter 4 --strict --default-route`
Chapter 4 28083, ending 34967 (both models). PM (changed, as expected: the PC's turn and shot maths): `--pc-char 2,0`
Chapter 1 16128 (was 15975); `--players 4 --pc-char 5,0,3,6` Chapter 1 19579 (was 17088); `--chapter 4 ... --pc-char
5,6` ending over at 31590 (was 38656). `hotd_proof pmmaths`: tables and the differential run 0 differ (proof.md);
`hotd_proof pccapture runs/k97aim_pm`: 244, 0 differ (Game<Pc> untouched).

pcmode7 (2026-10-20, row 10: the other kinds and the camera matrix; after `git merge main`): arcade unchanged:
`--input-test` 1562 checks, 0 failed; Chapter 1 ended at 16695 (arcade and `--models pc`); `--chapter 4 --strict
--default-route` Chapter 4 28083, ending 34967 (both models). PM (changed, as expected): `--pc-char 2,0` Chapter 1
16080 (was 16128); `--players 4 --pc-char 5,0,3,6` Chapter 1 16608 (was 19579); `--chapter 2 ... --pc-char 1,2`
Chapter 2 16970, Chapter 3 41679; `--chapter 3 ... --pc-char 3,4` Chapter 3 25434, Chapter 4 51286; `--chapter 4 ...
--pc-char 5,6` Chapter 4 22203, ending over at 29087 (was 31590). `hotd_proof pmmaths`: 102 THOTD.EXE routines, 0
differ (proof.md). Game<Pc> (its PC branches now use the pcm_* accessors, the same memory operations):
`hotd_proof pccapture` all_pa_r9 18276 / all_pm 16665 / all_pm_r9 18427 checked, 0 differ.

pmrest (2026-10-27; Boss Mode's PC rules, the barrel, after `git merge main`): arcade unchanged: `--input-test` 2071
checks, 0 failed; Chapter 1 ended at 16695 (arcade and `--models pc`); `--chapter 4 --strict --default-route` the
ending over at 34967 (both models); 4P Chapter 1 22201. PM: `--pc-char 2,0` Chapter 1 ends at 15563 (as main);
`--chapter 4 ... --pc-char 5,6` ending over at 41359 (as main). `hotd_proof pmmaths`: 185 routines, 0 differ.
`tools/soak.ps1 -Only "boss|pcmode"`: 24 runs, 0 failed.
pccap (2026-10-27; PC Mode ground, Boss Mode on the PC's maths, after `git merge main`): `--input-test` 2071 / 0 failed;
Chapter 1 16695 (both models); Chapter 4 strict 34967 (both models); 4P Chapter 1 22201; PM Chapter 4 strict
`--pc-char 5,6` the ending at 34605 (stalled at event 1 with floor_y for every caller before); `hotd_proof pmmaths`
0 differ; soak "boss|pcmode" 24 / 0 failed.
pmrest2 (2026-10-27; after `git merge main`): `--input-test` 2071 / 0 failed; Chapter 1 ended at 16350 (arcade and
`--models pc`); 4P Chapter 1 22283; `--chapter 4 --strict --default-route` the ending over at 34967 (both models).
PM: `--pc-char 2,0` Chapter 1 ends at 16289 (was 16649: no rats / table props, the PC's props and rounding);
`--chapter 4 ... --pc-char 5,6` ending over at 29416 (unchanged). Boss Mode autofire (unchanged): Chariot 2740,
Hangedman 4731, Hermit 2762, Magician 9582, All Bosses 19293. `hotd_proof pmmaths` 340 routines, 0 differ;
`tools/soak.ps1 -Only "boss|pcmode"` 24 runs, 0 failed; hotd_nettest all passed.
