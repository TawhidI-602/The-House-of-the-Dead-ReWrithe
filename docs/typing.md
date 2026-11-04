# The Typing of the Dead 0

ReWrithe's own mode (worker totd0, 2026-11-03; reworked by totd1 the same day after the user's first play: "decent
but pretty buggy", and by totd2 after the second: bats / projectiles, kill grades, rescues, bosses, word lengths): The House of the Dead played by typing, after SEGA's The Typing of the Dead (2000, on HOTD2),
with a few ideas from Typing of the Dead 3 and Typing of the Dead: Overkill (2013). The target is TotD 2000's feel.
Code: `src/game/typing.h` / `typing.cpp` (the layer), `src/game/typing_hud.*` (its HUD), data: `data/typing/`.

## How it works

The arcade logic runs as it is; the layer turns typed letters into player 1's light-gun input (`Runtime::gun[0]`),
between ticks, from the game's own state, so it is deterministic and normal play is untouched when the mode is off.
Three small hooks in `src/port` (all null outside a Typing of the Dead 0 tick, so the arcade / PC Mode / the proofs
never see them): `port::gAttackHold` (an attack waits) and `port::gHitHold` (a hit waits) and `port::gPreyHold` (a captor's bite on a researcher waits), see "Attacks" and
"Rescues".

| What | Rule | From |
|---|---|---|
| Prompts | every enemy the CPU teammates would take as a target (`Bot::targetKind` / `targetThreat`: on the screen, damageable, in the fight) gets a prompt in a box over it | all three |
| Word mix | most enemies one **word** (from all of `word_lists`, by length); tougher ones (hit points >= `tough_hp`, 200, not near) a **short phrase** (`tough_letters`); the toughest (>= `two_prompts_hp`, 400, not near) two prompts in a row; bosses **sentences** (`boss_lists`, `boss_letters`); bats, Murrers, frogs, dogs (`small_names`) **1-3 letters** (`small_letters`), thrown axes / barrels / knives 1-2 (`projectile_letters`) | TotD (Overkill: two prompts) |
| Length by threat (totd2) | a word's letters by threat band: **near** (spawned within `near_depth` 30 of the camera, or at you within `near_seconds` 2 at the speed it comes closer: one jumping out next to you), **far** (beyond `far_depth` 70, later than `far_seconds` 5, no attack place taken: a slow walker far off), **mid** the rest; `word_letters_near / mid / far` per difficulty (Normal 3-4 / 4-5 / 5-7). The depth: the camera-space z at first sight; the speed: the CPU teammates' tracking of the enemy (`Bot::approach`, 8 frames before the prompt). Captors and enemies in a rescue scene (a researcher on the screen) take the near band's shortest | user 2026-11-03 |
| Difficulty | Options > Difficulty picks the lengths and lists (one value per difficulty). Very Easy: 3-5 letter words, 14-20 letter boss sentences; Normal: 3-7 letter words, 7-11 letter phrases, 16-25 letter sentences; Very Hard: 4-10 letter words, phrases and sentences with capitals and punctuation, typed exactly as shown (`case_from_difficulty`, 3) | TotD |
| Chapter | from `longer_from_chapter` (5: never, totd2) enemies take the next difficulty's lists | TotD (longer later) |
| First letters | no two prompts held (on the screen or briefly off it) start with the same letter; one coming back into view with a taken letter gets another word | TotD |
| Lock-on | the first letter locks onto the prompt starting with it; the box turns yellow | all three |
| Letters | each correct letter is one shot at the enemy (hit, blood, part damage, points); its hit points are held up (30000) from the first letter so the letters cannot kill; the last letter's shot kills (hit points 1 first) | TotD (each key a shot) |
| Interrupts (totd2) | bats, small creatures and projectiles are urgent: a letter that is not the locked word's next one but starts an urgent prompt breaks in; that one is typed, then the lock goes back to the word, its letters kept. An urgent prompt never starts with a letter still to type in the locked word | TotD |
| Thrown things (totd2) | while a thrown axe / barrel / knife carries a prompt it flies at `projectile_speed` of its speed (Normal 0.4: velocity x k, gravity x k^2: the same arc, k times slower) | TotD (thrown things drift) |
| Kill grade (totd2) | each kill gets S / A / B / C / D / F from its accuracy and speed over all its prompts, timed from when it could be typed (shown, or the last prompt done if later): `kill_grade_S..D` = accuracy %, WPM (S 100 % 50 WPM, A 100 % 36, B 85 % 27, C 70 % 19, D 50 % 11). Shown over the enemy for 1.25 s (gold S / A, white B / C, grey D / F; a boss sentence's under its panel), counted in the results' "Kills" row | TotD |
| Small targets | bats / Murrers / frogs / dogs and projectiles get no letter shots (a bat dies at any hit: its first letter killed it); the last letter fires once and waits 10 frames for the hit to show (its flags' hit bit); still not hit after 6 tries, the hit is given (flags bit 8 + player 1's bit, as a landed shot) so the word always kills or knocks the thing away | TotD |
| Misses | a wrong letter (or one that matches nothing) is a miss: accuracy, combo broken; the lock stays; nothing is entered, so there is nothing to Backspace | all three |
| Drop the lock | `drop_lock_keys`: 0 nothing, 1 Escape (default; Escape without a lock is the pause), 2 Escape and Backspace. Backspace alone does nothing (totd0 dropped the word on it: a typo's habitual Backspace threw progress away) | ours |
| Items | pickups by routine name (`item_names`: the gold frog ...) get a word; totd2: also the locks / route spots / signs (`route_names`), props with an item inside (flags bits 5-7), and in a rescue scene with no creature about the props by the researcher (Chapter 2's crate) | all three |
| Civilians | researchers / hostages / scene people never get a prompt; no shot is fired where one covers the aim point | all three |
| Off the screen | a prompt is drawn only while its enemy is a target on the screen; it keeps its word 5 s off the screen (a duck, a pillar) | ours |
| Combo | prompts in a row without a miss; the score gets `combo_bonus` x the combo (up to `combo_cap`) per prompt | Overkill |
| Reload | none: the gun is refilled every tick in play | TotD |
| Punctuation | typed as shown; `auto_skip_punctuation = 1` passes it over; spaces are always typed; case ignored below `case_from_difficulty` | Overkill / TotD |
| Long prompts | wrap onto two lines at the space nearest the middle | Overkill |
| Results | per chapter, over the clear's results: letters, misses, accuracy, WPM, prompts, best combo, the fastest prompt, a grade S-E | all three |
| Sonic Mode | the ticks run faster; windows and timers are in ticks: less real time to type; its own top 10s | ours |
| High scores | the game's own ranking (name entry as Arcade) in `ranking_totd.bin`, `ranking_totd_<preset>.bin` | ours |

## Attacks (totd1: "enemies attack too fast, even on Normal")

TotD: a zombie walks up, then gives a clear window before it hits; its attack waits on a timer you can see.

- **Timer.** A creature "arrives" when it takes an attack place (+0x2E7), comes within 45 units, or (bats, the agile
  ones) once its prompt has been up `attack_wait` seconds. It may attack `attack_wait` + `attack_per_letter` x its
  prompt's letters later (small ones: half the wait). The bar under its word shows the time left (green, yellow, red).
- **Gap.** No attack on player 1 within `attack_gap` seconds of the last one (an attack beginning: player 1's
  "targeted" flag set; a hit; a throw). After an attack the creatures whose timer had run out wait `attack_wait` again.
- **Defaults** (very easy .. very hard): wait 6 / 5 / 3.5 / 2.5 / 1.5 s, per letter 0.35 / 0.3 / 0.25 / 0.15 / 0.1 s,
  gap 4 / 3 / 2 / 1.2 / 0.6 s.
- **Where it is asked** (`port::gAttackHold(obj)`): the standard enemy's attack choice (`enemy_ai` states 5 / 6: no
  free player for it), `pick_target_player` (bats, the agile kind 70, frogs, scene zombies, the Hangedman's helpers),
  the dogs' bite (a held bite snaps at the air), and from the layer the kind 32 throwers' countdowns (+0x2EC held at 2).
  A creature without a prompt (off the screen) waits too. Murrers' leaps are not held (their own queue).
- **Bosses** (`port::gHitHold(player)` in `player_hit`): in a boss fight a hit on player 1 lands only after a sentence's
  window ran out (TotD: the boss strikes when its sentence was not typed in time); a finished sentence holds them again.
  Boss sentence windows: `boss_window_seconds` 5 / 4 / 3 / 3 / 2 + `boss_window_per_letter` 0.6 / 0.5 / 0.4 / 0.3 /
  0.25 s a letter (Normal: 30 WPM makes it). A sentence's attack that did no damage (the boss turning, in its own hit
  reaction) goes on with a shot every `boss_shot_gap` until one does (3 s at most).

Why the hit points are held: a shot's damage lands a frame or two after the trigger and HOTD's zombies need several
hits, so "one key = one shot" would kill most enemies half-way through a word, or never at its end. Holding the hit
points until the word is done gives the SEGA games' rule (the word kills) while every letter still visibly hits. A
prompt dropped (Escape, the enemy gone for 2 s, a cut-scene) puts the real hit points back; a killing shot that never
lands (40 tries, the body parts tried in turn: `partAim`) also puts them back and gives a new prompt.

Why bosses take `boss_damage_x`: a boss staggers at the first hit and takes no more for a while (the Chariot: 40 a
hit), so a sentence is about one hit; x3 makes the Chariot about six sentences. The extra is taken from the hit points
after the boss's own hit routine ran, never below 1, so the boss's own checks (armour off, death) come with its own
hits.

## Rescues (totd2: "the trapped / grabbed researchers can't be saved")

Root causes: a researcher in front of the captor's aim point stopped every shot (`Bot::hostageCovers`), so the
killing shot never fired and the prompt was given up; the captor's bite came at the game's own pace, before a word
could be typed; the locked doors' padlocks had no prompt. Now:

- The killing shot tries the captor's damageable body parts (`partAim`) for one the researcher does not cover.
- **Rescue timer** (`port::gPreyHold(obj)`, in the run-after / attack-researcher routines' arrival tests in
  `enemy_actions.cpp`): a creature that reached its researcher bites only after `rescue_wait` (Normal 4 s) +
  `attack_per_letter` x its prompt's letters from the prompt shown (one with no prompt yet: `rescue_wait`). While held
  it is put back where it stood after the tick (walking on into him pushed Chapter 1's first researcher off his path,
  and the event never went on). Typed in time, the game's own rescue follows its death (thanks, item, bonus); too
  late, the game's own bite.
- Locked-door researchers: the padlocks and route props get a word (Items above).

## Bosses (totd2: "bosses don't work")

- One sentence at a time: a boss of several parts (the Chariot's armour and body) had a sentence each, drawn over each
  other in the one boss panel. The part shown is the one locked or begun, else the most dangerous; the others wait
  (their windows held).
- The Magician's barrage (homing shots, routine 0x74150, 500-700 a fight) gets no prompts; its hits are held by the
  boss rule anyway.
- Hits outside a boss fight (`gHitHold`): only while a creature whose attack timer ran out is there (the agile kind 70,
  the vomit, the taran never ask `gAttackHold`; the self-test's 40 WPM typist took 16-18 hits a chapter from them).
- Chapter 4 ends in the ending, not a clear: its results are taken there (`Typing::flush`).

## Text

The prompts are drawn in the game's own 8 x 8 text font: the arcade's SCORE-line font (the boot's debug font, 128
ASCII glyphs at ROM 0x2EF3870, 4-bit pens; palette 0 at ROM 0x168A0 for the pens' brightness, through MAME's gamma
as `sprites_arcade.cpp`). It has every printable ASCII character (letters, digits, punctuation); anything else falls
back to SDL's 8 x 8 font. No new art. Typed letters red, the next one yellow and underlined, the rest white.
`HOTD_TOTD_FONT=<chars>` (developer) prints which glyphs the game has and draws the given ones as text.

## Input

totd2, the root cause of "bats and projectiles still don't work in real play" (totd1's self-test: 19/19): its typist
called `Typing::typed` itself and always finished its locked word first, so it never typed a thrown axe while a word
was locked; in real play the axe's letters were misses against the locked word, and the axe hit 0.6 s after the throw.
A frog / bat still falling after its word also got a new prompt. Now the self-test's keys go through SDL's event queue
(a key-down, Shift held for a capital, the text event, a key-up) into the window's own handler (`totdEvent` in
main.cpp); the typist reads a prompt a reaction time after it appears (0.38 s at 70 WPM, 0.48 s at 40), types urgent
ones as soon as read, slips onto a neighbouring key, and presses Escape when its locked prompt is gone for a second.

SDL text events while player 1 is in play (`Typing::inPlay`: play, shots on, no cut-scene / card / results / pause);
every printable key is kept from the key bindings then (a letter never also fires or aims). Ctrl / Alt chords,
F-keys, Enter and Escape without a lock reach the game as usual. One typist: player 1. Players 2-4 can play along with
the mouse / pads / CPU teammates. Offline only (an online game turns the mode off).

## Data (`data/typing/`, mods can replace any file)

- `typing.ini`: the tuning above (each data folder's in turn, a later one overriding). Values "a, b, c, d, e" are per
  difficulty (fewer: the last repeats).
- `<dictionary>/<list>.txt`: one prompt per line, `#` comments; the lists named in typing.ini. The default set
  (`default/`): `short` (1-3 letters), `easy` (3-5), `words` (5-8), `long` (8-13), `phrases` (everyday, lower case),
  `hard_phrases` (capitals, punctuation), `boss_easy`, `boss`, `boss_hard`. Plain everyday English, written for
  ReWrithe, a little spooky flavour; no song, book or SEGA text. A list missing from a dictionary borrows the largest
  one it has. `dictionary = <name>` (or `--totd-dict <name>`) picks the folder: a player's own word lists.
- HUD words go through the language system (`lang::tr`); prompts are not translated (a language's own dictionary
  folder is the way).

## Running it

- Window: main menu > More Modes > Typing of the Dead 0, or `hotd --totd` (`--difficulty 0-4` for a quick try).
- Headless self-test: `hotd --totd-test [--difficulty 0-4] [--totd-typist wpm,accuracy,seed] [--sonic-mode nightmare]
  [--chapter N]`: a scripted typist (70 WPM, 95 %; capitals with Shift from Hard; its keys as SDL events) plays the chapter to its end and
  prints `TOTD chapter N: ...`, `TOTD lists (...)` (prompts and mean letters per list), `TOTD attacks: ...` (attacks
  begun, hits taken, the closest two), `TOTD small targets / projectiles: ...` (typed, gone at the last letter, kill
  shots that never landed, letter kills) and `TOTD TEST: PASS|FAIL` (fails on attacks closer than `attack_gap`, small
  targets not killed, an enemy killed by a letter). totd2: `TOTD REPORT chapter N` (bats / projectiles / small ones
  shown / typed / killed as the game saw it afterwards, rescued of those who appeared, the boss, the grade spread,
  hits) and `TOTD LENGTHS chapter N` (mean letters by enemy class); fails on typed-but-not-killed (< 90 %) and on a
  boss not beaten; the attack gap is reported only. `--totd-verbose` a line per prompt; `HOTD_TOTD_TRACE=1`
  prompts given / dropped, shots, attacks, player 1's targeted flag; `HOTD_TOTD_BOSSHP=1`, `HOTD_TOTD_OBJ=<hex>`.
- Pictures: `--totd-test --render --shot-at F --shot-dir D` draws the prompts too.

Results (totd1, 2026-11-03, Chapter 1 unless said; player hits include the boss):

| Run | Lists (mean letters) | Hits | Small typed / killed |
|---|---|---|---|
| totd0, Normal, 70 WPM | words / phrases / sentences mixed | 19 | bats died at the first letter, frogs never |
| Very Easy, 70 WPM | easy 4.4, short 1.0, boss_easy 20 | 4 | 20 / 20 |
| Normal, 70 WPM | words 6.4, phrases 11, short 2.8, boss 37 | 4 | 19 / 19 |
| Normal, 30 WPM 90 % | the same | 16 (chapter cleared) | 19 / 19 |
| Very Hard, 70 WPM | long 9.3, hard_phrases 18, short 3.0, boss_hard 42 | 11 | 18 / 18 |
| Chapter 2, Normal, 70 WPM | | 4 (totd0: 44) | 20 / 20 |
| Sonic Nightmare, 100 WPM | | 4 | 20 / 20 |

Results (totd2, 2026-11-03, Normal, typist keys through SDL events; 70 WPM 95 % / 40 WPM 92 %):

| Chapter | Bats typed / killed | Projectiles typed / killed | Rescued | Boss | Grades S A B C D F | Hits |
|---|---|---|---|---|---|---|
| 1 at 70 / 40 | 1/1, 1/1 (+ small 16/16, 13/13) | 3/3, 2/2 | 6 of 6, 5 of 6 | beaten, beaten | 28 25 17 7 3 0 / 1 6 42 21 5 0 | 0 / 1 |
| 2 | 34/34, 31/31 | 4/4, 2/2 | 5 of 5, 6 of 6 | beaten, beaten | 29 40 23 11 7 0 / 1 12 59 32 11 4 | 1 / 6 |
| 3 | - | 20/20, 25/25 | 3 of 3, 3 of 3 | beaten, beaten | 51 36 18 11 3 0 / 6 19 50 32 9 1 | 2 / 3 |
| 4 | 20/20, 18/18 | 28/28, 25/25 | (none) | beaten (the ending), beaten | 30 25 27 6 5 0 / 2 11 37 30 7 1 | 1 / 3 |

Mean letters (70 WPM, all chapters): zombie near 3.5, mid 4.3, far 6.2, captor 3.0, tough 9.8, item 4.3, bat 2.0,
projectile 1.4, boss 19.9 (totd1 Normal: words 6.4, phrases 11, boss 37).

## Open

- (totd2) Murrers' leaps are held (they regroup); the Magician's / Hermit's own attack timing not looked at (their hits
  are held by the boss rule). Some projectiles are seen but never typed (Chapter 4: 45 shown, 28 typed): they reach the
  player or leave the screen before the typist reads them; their hits are not counted as a failure.
- Boss "quiz" segments (TotD's answer prompts).
- A dictionary chooser in the menu; a per-language dictionary picked with the language.
- Box frames / typewriter art from the user (the font is now the game's own).
