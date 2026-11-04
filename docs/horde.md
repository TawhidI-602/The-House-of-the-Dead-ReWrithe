# Horde Mode (ours, 2026-11-03)

The arcade game as it is (the chapters, the story, the rescues, the bosses, the routes) with many more creatures at
once, as HOTD1 Remake's Horde mode. Code: `src/game/horde.h` / `horde.cpp`. (The wave / arena mode that was called
Horde until this date is now Survival Mode: [survival.md](survival.md).)

## How to play / test

- Menu: **More Modes > Horde** opens Chapter Select ("Horde - Chapter Select"; the players are asked after the chapter,
  1-2 or more as in the arcade mode). **Horde Size: Low / Normal / High** below it (click: the next size; Normal by
  default). Sonic Mode's preset applies as to every game.
- Command line: `--horde-mode [low|normal|high]` (Normal without a word) or `--horde-size low|normal|high`, with the
  arcade game's own options (`--chapter N`, `--players`, ...). Local games only (online co-op starts without it).
  (`--horde <arena>` / `--horde-test` are Survival Mode's old names and still start Survival.)
- High Scores: the **Horde** table, the arcade's own top 10 in its own file (`ranking_horde.bin`, with Sonic Mode
  `ranking_horde_<preset>.bin`): the game's name entry after a game writes there, never into the arcade's table.
- Headless runs print `HORDE: size=.. originals=.. copies=.. skipped=.. peak=.. bodies=.. bad_access=.. frame=..`.

## How it works

The spawn opcodes ENTB / ENTB3 / ENTBE tell the runtime of each object they make (`Game::evt_spawned`, before it first
runs). For a **standard walker** (record kind 0, `kind000_enemy_spawn`, or kind 9, `kind009_enemy_spawn_b`), outside
a cut-scene (shots on, no letterbox), Horde makes 1 / 2 / 3 copies (Low / Normal / High: about 2x / 3x / 4x the
creatures): a new task of the same routine in the same task group, its work area (+0x20..+0x3C8) copied from the
creature as the opcode set it (kind, flags, type, action, hit points, position, angles, start delay and sub-state),
then moved to its side (34 units one way, 34 the other, 68 for the third; a few units' jitter) and its start delay
(+0x320) 1-3 steps later, so they come staggered and do not stack.

Each copy runs the standard set-up and counts itself (`enemy_counts_add`), so the script's creature waits
(WAIT_ENNUM / WAIT_ENALV, the camera stops' kinds 0-2) wait for the copies as for the originals: the kills-to-proceed,
the doors and the chapter ends are the game's own. Every other kind is left as the script has it: the bosses (their
own kinds), hostages and their scripted attackers (kinds 96-101 ..), the scene and event creatures (kinds 32, 70, 162
...), props.

**Limits:** the extra body slots of Survival Mode (`Game::extraBodySlots`: slots 16.. in a table outside the original
map, 0x7E1000 here, the three skin buffers grown at each chapter's heap reset): 26 more than the original's 14. A copy
is not made when 24 creatures would be alive (`enemy_count`) or fewer than 4 body slots would be left for the script's
own creatures (`skipped` in the line). The other tables (attack places 14, attackers 13, depth list 256, shot
candidates 256, the heap) as docs/survival.md "Limits": the extra creatures queue for attack places as the original's
15th does.

## Checked (headless, autofire, `--strict --default-route --lives-kept 1`)

| Run | Copies (originals) | Peak alive / bodies | Result |
|---|---|---|---|
| Chapter 1, Normal | 142 (71) | 12 / 12 | chapter 1 ended at 19739 (plain: 16350), chapter 2 at 38450, chapter 3 at 69459 |
| Chapter 2, Normal | 100 (50) | 11 / 11 | chapter 2 ended 19160, chapter 3 52554 |
| Chapter 3, Normal | 68 (34) | 12 / 12 | chapter 3 ended 30954, chapter 4 53639, the ending is over 60523 |
| Chapter 4, Normal | 14 (7) | 6 / 6 | chapter 4 ended 24961, the ending is over 31845 |
| Chapters 1-4, High | 207 / 150 / 102 / 21 | up to 15 / 15 | every chapter ends; the ending is over (ch 3, 4) |
| Chapters 1-4, Low, 2 players | 76 / 52 / 35 / 7 | up to 9 | every chapter ends |
| Chapter 1, Normal, PC models | 142 | 12 | as the arcade models |

bad_access=0 in every run. Time: 8000 rendered frames of Chapter 1, plain 6.4 s, High 6.8 s (+6 %). Pictures:
scratch `workers\survival\shots\hordeB.png`. The plain arcade game is unchanged (the input test, Chapters 1-4 end at
the same frames).

## Open

- Chapter 4 has few standard walkers (7 kind 0 records): its creatures are mostly kinds 32 (the kit zombies), 70 (the
  agile ones) and the bosses, not copied yet; copying kinds 32 / 70 needs a check of each (their kits and records).
- The copies go where their original goes from beside it; in a narrow place one may start inside a wall edge (seen
  in no picture so far, not checked in a window).
- Online co-op: not with Horde (its state is the game's own, but the menu's Horde starts a local game).
