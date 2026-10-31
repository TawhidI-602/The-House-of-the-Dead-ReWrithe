# menuart: the PC port's own front end, ported (2026-10-17, branch claude/menuart)

Build folder `build-menuart`. The user's decision: no new menu design. **THOTD.EXE's front end is ported routine by
routine**, as the game logic was, and drawn from the player's own PC files at run time (nothing is copied into the
repo or the package). It is the menu for everyone (every player has both the arcade and the PC files). The text
launcher (src/game/launcher.cpp) stays for two cases only: the very first start (before the files are picked and
checked) and as the fallback when the PC menu art cannot be read. Our extra options become extra items / pages in the
ported menu's style. Model source keeps its current default (arcade, `LaunchConfig::models = 0`, for a fresh profile;
a choice saved in launcher.ini still wins); no other default changes.

Everything here is **UNVERIFIED (teaser)**: compared by eye and by pixel difference with one PC harness capture.

## First screen: the main menu (done, waiting for the user's OK)

Side by side, ours left, the real THOTD.EXE right: `private\menuart_scratch\main_menu_side_by_side.png`
(ours alone: `private\menuart_scratch\ported\main_menu.png`; the harness captures: `private\menuart_scratch\pc_menu_cap\`,
the menu in pc_s0008 .. pc_s0028; it returns to the attract after the original's 30 s idle).
Mean absolute difference of the 640 x 480 pictures: 1.9 / 255; 0.17 % of pixels differ by more than 40 (the edges of
the scaled capture).

How to remake them:

```
hotd --root "<repo>" --pc "<PC folder or zip>" --frontend-shot <out.png> [--frontend-cursor 321,222] [--size 1280x720]
tools\pcharness\run_pc_shots.ps1 -Out <private\...> -Script menu_hover.txt -Ticks 99999 -Timeout 50 -EverySeconds 4
py tools/menuart_tool.py side <out.png> <ours.png> <real.png>
```

`--frontend-shot` is headless (no window): loads the sprites through MENU.TBL / MENU.PAL, builds screen 1, puts the
pointer at (321, 222) (the harness script's position: on "Arcade"), runs one front-end frame (hover walk, draw walk,
cursor) and writes the 640 x 480 picture scaled to 720 lines, pillarboxed. `run_pc_shots.ps1 -EverySeconds` (new)
captures by wall time, because the menus run without game ticks. `scripts/menu_hover.txt`: Enter in the attract,
the cursor onto Arcade, no activation.

## Step 1: the menu in the window (done)

- Flow: the text launcher only on the first start (no launcher.ini with both paths) or when the PC art cannot be read
  (then Start goes straight into the game as before). With a known setup the launcher presses its Start by itself
  (the files check, as before), then the **main menu** comes up (Screen::Menu in src/main.cpp). Arcade -> the game as
  the launcher's Start did; Options -> the text launcher's settings for now (its Start brings the menu back);
  Ranking does nothing yet (step 2); Quit / Escape (the widget tagged 1, frontend_input 0x41DA18) -> exit. Esc in
  the game and the end screen go back to the menu (frontend_run's start again: cue 1, cue 22, the main menu).
- PC Mode and Boss Mode: shown in the grey 24 font, no handlers (user, 2026-10-17: locked while the teaser runs the
  arcade logic; `Frontend::mainMenuEnabled`).
- Input (0x420A60 / 0x42095C): arrows / keypad 8 2 4 6 with the original's acceleration (16.16 step x1.5 a frame up
  to 11 px), the mouse through the picture's rectangle (the 640 x 480 picture fitted to the window's height,
  centred), Enter / keypad Enter / Space / the left button -> cue 28 + the bony hand's click frames -> the activation
  (a second press during the animation activates at once). The system cursor is hidden in the menu.
- Sound: `Frontend::onCue` -> `Audio::pcCue` (cue 1 command 2, cue 21 track 11 (the PC's cue 22 = track 12 is the Chapter 4 song; 2026-10-19), cue 28 sample 13); with
  HOTD_TRACE_SOUND=1 the log shows `[sound] front end: PC cue N ...`.
- Checks: input test 518 / 518 (updated: from the menu it moves the mouse onto Arcade and clicks 5 frames later; the
  texts say "main menu"); headless `--frames 40000 --autofire --mute --lives-kept 1` arcade and `--models pc`:
  "chapter 1 ended" at 16695; a window run (second display, --mute --shot-at 0, `HOTD_MENU_SHOT=60` = a screenshot
  at the menu's 60th frame, then exit) shows the menu.
- Open: windows narrower than 4:3 cut the picture's sides (it is fitted to the height). (The 30 s idle: menuboot.)

## Step 2 (part): the Options screen (done)

`Frontend::screenOptions` = THOTD.EXE 0x41E428 (screen 2), reached from the main menu's Options (0x421434). Side by
side with the harness capture (`tools\pcharness\scripts\menu_options.txt`):
`private\menuart_scratch\options_side_by_side.png` (ours: `hotd --frontend-screen 2 --frontend-cursor 322,345
--frontend-shot <png>`). Background 0xA7 (Sophie; its value boxes are drawn into the picture), title label 0xB in
white 32, panel (0x60, 0x60, 0x1C0), rows every 32 px: label (0, y, 0, height of 'A') flags 0x20002; values as
children of the row label, right-aligned (0x20004), hit test 0x428294 (the whole row); the 1P / 2P cells at
panel.w - 0x98 / - 0x48, 72 wide; sliders at panel.w - 10 x width(0xE0) (0x426E80: left arrow, 8 cells with the
knob, right arrow; sprites 0xDC-0xE2 from table 0x5CED60; click 0x426DC0: the arrows step, a cell sets). Exit
(label 0x24, centred, tag 1 = Escape) back to the main menu.
Ours behind it (`Frontend::options`, saved to launcher.ini at each change, the volumes applied at once):
Difficulty = our five operator levels (labels 0x17-0x1B; the original has Easy / Normal / Hard), Life 1P / 2P = our
one lives setting (both cells show its start lives; a click cycles the operator table), Continues and Resolution
grey (no limit / the window decides), Auto Reload 1P / 2P = our one switch, Blood Color = ours (the same order:
Red, Green, Blue, Purple, Yellow), Sound Effects / Music = our volumes (8 steps <-> 0-100 % in tens), Controller
Config -> the text launcher (players, input) for now.

## Step 3: our extras page (done)

"More Options" (ours, white 24 text item in the gap before Options' Exit) -> `Frontend::screenExtras` in the
Options screen's style on the hall picture 0xD1 (the Sophie pictures have the Options boxes drawn in): Chapter
(cycles the playable chapters only), Models (Arcade default for a fresh profile; launcher.ini wins), Crosshair,
Aspect, Music on / off, Music Numbering (Auto / PC CD / Saturn, main 82a9784), Files Check (-> the text launcher),
Desktop Shortcut (winshell::createShortcut; shows Created / Exists / Failed). Exit / Escape -> Options. Sample
picture: `hotd --frontend-screen 3 --frontend-shot <png>` -> `private\menuart_scratch\ported\more_options.png`.
Open: the locked chapters are skipped rather than shown grey; players / P2 input still in the text launcher.

Checks after steps 2-3: input test 518 / 518; arcade and `--models pc` headless: "chapter 1 ended" at 16695.

## Where it stands / next

- Steps 4 (widescreen: the picture pillarboxed, the pointer mapped through it) and 5 (the text launcher on a failed
  load) came with step 1.
- Ranking is grey for now (no dead item) until screen_high_scores 0x423038 is ported; then the Controller Config
  (0x41F75C), PC Mode (0x4259AC) and Boss Mode (0x422B90) screens, each with a harness side by side (scripts:
  menu_hover.txt / menu_options.txt as models; down N polls: 1+1+2+3+5+7 then 11 a poll).
- Open: windows narrower than 4:3 cut the picture's sides; the locked chapters on More
  Options are skipped rather than shown grey; players / P2 input still in the text launcher. (All three done by the
  overhaul below.)

## The overhaul (branch claude/menu, 2026-10-20)

User: keep the 1998 PC menu's charm (its art, hand, font, sounds, music, layout), reorganise the structure.

- **Main menu** (screen 1): nine items 28 px apart from y 0xC8 (the PC: six, 32 apart from 0xD0): Arcade (Chapter 1
  always: `menuChapter = 0`), Chapter Select, Boss Mode / PC Mode (grey "coming" until `Frontend::startBossMode` /
  `startPcMode` are set by their workers), Co-op, Sonic Mode: <preset> (click: next), High Scores, Options, Quit (tag 1).
- **Chapter Select** (screen 9, ours): the PC's boss-select portraits 0xE3-0xE6 (yellow 0xE8-0xEB on hover) for the
  chapters' bosses, "Chapter N" under each (grey when not `chapterPlayable`), the ending's line (after Chapter 4), Exit.
  A choice is saved as launcher.ini `chapter=` and starts that chapter.
- **High Scores** (screen 8): the PC's page arrows 0xD8 / 0xD9 (yellow 0xDA / 0xDB) turn 8 tables: the arcade's,
  Sonic Mode's three presets, then the same four for 4 players (`ranking*.bin` / `ranking*.bin4p`).
- **Options** (screen 2): the PC's rows; Controller Config opens Controls; the old "More Options" row is a strip of
  the categories (white 24, the current one yellow) Video / Audio / Controls / Gameplay / Mods / System.
- **Category pages** (screen 100, `src/game/menu_pages.cpp`): More Options' rows regrouped, the same values and
  launcher.ini keys; new: the volumes as rows, Title / Attract, FBI Screen, Guards (cuts), Blood Colour incl. Custom,
  4P Boss Hit Points (`boss4p=`, percent; `--4p-boss-step` still overrides). 12 rows a page (title 0x14, tabs 0x3A,
  rows from 0x5C, Exit at 0x19A with the page arrows beside it). Sub-pages (Controller P1-P4, Steam, CRT Settings,
  Online) have no tab strip; their Exit returns to their category.
- **Co-op** page: Local 2 Players / 4 Players (start the arcade with that many), Online Co-op, Players, Human / CPU.
- **Help line**: every widget may carry `help`; the hover walk keeps the innermost one under the hand; drawn centred at
  the bottom in the 6 x 10 font (0x37A + c; split in two lines when wide). The PC data's ')' (0x383) is a second '(':
  drawn as '(' mirrored.
- **Fit**: `Hud::drawPicture` fits the 640 x 480 picture inside the window (pillarbox or letterbox; was: fitted to the
  height, so portrait windows cut it); the mouse and the input test map through `Hud::pictureRect`.
- Shots: `--frontend-screen` 1, 2, 3 / 10-15 (categories), 4, 8 (`HOTD_FE_PAGE` = table), 9, 16 (Co-op); private\menu\
  <size>\ at 640x480, 1024x768, 1920x1080, 2560x1080, 600x900.
- Checks: input test 922 / 922 (main's 911 + 11 menu-walk steps: Options, the Video and Audio tabs, Escape back,
  Chapter Select, Co-op, High Scores); headless arcade and `--models pc`: "chapter 1 ended" at 16695.

## Boot order: the attract before the menu (branch claude/menuboot, 2026-10-17)

**THOTD.EXE's boot** (harness capture with no input, `tools\pcharness\scripts\boot_idle.txt`, a picture every 3 s:
`private\menuboot_cap\boot_idle\sheet.png`): black, the SEGA logo (~3-9 s), the start-up splash (the title art,
"(c) Sega Enterprises, Ltd., 1996, 1998", ~12 s), then the attract loop: the prologue (mode 9) and the demos with
"Press SPACE to skip Demo", the title (logo over the mansion + SEGA), how to play (mode 8), ... It never reaches the
menu by itself.
**Leaving it** (pc_main_loop 0x419168, 0x41932C..0x4193CE): in game modes 2 / 3 / 8 / 9 a mouse button (0x12FDB84
bits 0 / 1: left or right) or a press edge of Escape (DIK 0x01), Enter (0x1C), keypad Enter (0x9C) or Space (0x39)
-> `frontend_state` 7 -> the menus. Other keys do nothing there.
**Idle timer** (0x420A60's tail, 0x420ED0..0x420F2B): 0xD5A960 = timeGetTime at the last activity (the pointer moved
0x420EEC, an activation 0x420DCA, a screen builder: 0x421119 for screen 1); n = 30 (10 with the debug flag 0x5CD374),
limit `((n * 31 * 4 + n) * 8)` = **n x 1000 ms = 30 s**; only on screen 1 (0x5CE8A8 == 1) -> `frontend_state` 6, the
attract. Keys that do not move the pointer (e.g. an arrow at the edge) are no activity.

Ours (src/main.cpp `enterAttract`, `feAttract`; src/game/frontend.cpp `pointerFrame` = 0x420A60 with the timer,
`attractDue` / `clearAttract`), all UNVERIFIED (teaser):
- first start: the text launcher (files not picked) as before; then (and with a known setup at once) the files check
  -> the start-up splash (below)
  -> **the attract** (the arcade's title / attract loop, `Runtime::attract_begin`, whatever chapter is chosen) ->
  any key or mouse button (the original's set plus the rest: our attract would otherwise start a game on 1 / 2) ->
  the main menu (cue 1, cue 22). Arcade -> `chapter_start` directly (no second attract; also after a game over).
- 30 s without pointer motion / activation on the main menu -> the attract again -> a key -> the menu.
- The attract plays once at boot: Options / Files Check -> the text launcher -> its Start comes back to the menu.
- Off (straight to the menu, as before): `--no-title`, the launcher's "Title / attract" off, `--input-test`,
  `--auto-start`. Defaults unchanged (model source arcade, title on).
- Developer: `HOTD_ATTRACT_EXIT=N` presses Enter at the attract's N-th frame (window runs).

Checks: input test 518 / 518 (hidden, silent; it skips the attract as before); headless `--frames 20000 --autofire
--mute --lives-kept 1` arcade and `--models pc`: "chapter 1 ended" at 16695; window runs on the second display with a
scratch HOTD_PREF_DIR (a copy of launcher.ini), `--mute`: `--shot-at 600` = the attract
(`private\menuboot_cap\attract_f600.png`), `HOTD_ATTRACT_EXIT=700 HOTD_MENU_SHOT=60` = the menu after it
(`menu_after_attract.png`; both side by side: `attract_to_menu.png`); `HOTD_ATTRACT_EXIT=120`, 60 s: the log shows
attract -> menu -> "idle on the main menu -> the attract" -> attract.
**The start-up splash** (THOTD.EXE 0x419FDC, the start-up window's handler; `Frontend::splashStart` / `splashTick` /
the paint in `render`, UNVERIFIED (teaser)): two SPR.RBH sprites on BIGGER.VMC's 256 x 256 pages, read at run time
from the player's PC files like the menu art: **0xA1 the SEGA logo**, then **0xAA the title art** (the zombie,
"(c) Sega Enterprises, Ltd., 1996, 1998"). Message 0 (create): time into 0xD5A688, stage 0xD5A67C = 0 (1 when the flag
0x5CD350 is set). Message 0xD (tick, 0x41A140): input = a key edge from the list 0x5CE6BC (Esc, 1-0, Tab, Shift, Ctrl,
Alt, the keypad, the letters ...) or mouse button 0 / 1 (0x419F54 / 0x419FC0). Stage 0 (0xA1): input or > 3000 ms ->
stage 1. Stage 1 (0xAA): input or > 5000 ms -> 0x565350(window, 9, 1 / 0) = the window ends. Paint 0x419F18: the
sprite at (0, 0), **no fade**. Ours: in the Menu screen before the attract at boot (when the attract plays), music
silent until the menu. Side by side with the harness (`private\menuart_scratch\splash_side_by_side.png`: ours left,
the window run's `HOTD_MENU_SHOT=60` / `260`; THOTD right, boot_idle pc_s0006 / pc_s0012): the same pictures; the
SEGA logo's background is a neutral grey in ours (MENU.PAL) and slightly blue in THOTD's (another palette at boot:
open). Window run `HOTD_MENU_SHOT=640 HOTD_ATTRACT_EXIT=300`: splash -> attract -> menu
(`private\menuboot_cap\menu_after_splash_attract.png`).
Open: what the window's end with 1 (a key on the title art) changes vs 0; the first black second (THOTD's load).
Open: our attract is the arcade's (its "PRESS START BUTTON / FREE PLAY" texts, no "Press SPACE to skip Demo", no SEGA
logo); whether the PC's Arcade plays the prologue before Chapter 1 (ours: straight to the chapter, as before).

## What is ported (src/game/frontend.h / .cpp)

From the disassembly (tools/x86dis.py; Watcom register calls, which the Ghidra C mixes up):

| THOTD.EXE | ours | what |
|---|---|---|
| 0x427530 widget_new, 0x427694 widget_add_child (prepends), 0x427C84 widget_free_tree, 0x427CB8 widget_set_rect (eax w, edx x, ebx y, ecx w, stack h) | `widgetNew`, `addChild`, `freeTree`, `setRect` | the 0x44-byte widget tree |
| 0x4283A0 widget_set_label, 0x4283CC widget_set_text, 0x4283F8 widget_set_image, 0x428424 widget_set_handlers, 0x427CB0 (tag) | `setLabel`, `setText`, `setImage`, `setHandlers` | |
| 0x4275CC (push), rect stack 0xD5AD20 / 0xD5B1D0 | `push`, `stackReset`, `absRect` | 0 width / height = the parent's (minus x / y when pushed) |
| 0x427938 / 0x427998 | `hoverWalk` | flags 0x40000 on the hovered widget (the text turns yellow) |
| 0x427A84 / 0x427AE8 | `drawWalk` | draws, clears the hover mark; 0x4000 blinks with 0x5CED90 |
| 0x4277B0 / 0x427810 | `activate` / `activateWalk` | the widget under the pointer runs its handler |
| 0x427CDC widget_draw_image, 0x427FE8 widget_draw_text, 0x427F64 (measure), 0x428178 / 0x428294 (hit tests) | `drawImage`, `drawText`, `measure`, `hitText`, `hitRect` | alignment flags 0x04 / 0x20 (x), 0x10 / 0x40 (y); space = 8 px, glyph advance = width + 1, height of 'A' |
| 0x428494 | `glyph` | the font by flags (below) |
| 0x428448 + table 0x5CCD64 | `label` | the English strings (100 ids, read from THOTD.EXE's data; listed in the code) |
| 0x420904 / 0x420A10 | `drawCursor` | sprite 0xF9 + frame, top-left at the pointer |
| 0x4210FC screen 1 | `screenMainMenu` | background 0xD3 (flags 0x20000A), list at y 0xD0 (h 0x140), six labels 1 / 2 / 0x42 / 5 / 6 / 7 (Arcade, PC Mode, Boss Mode, Ranking, Options, Quit) every 32 px (0x5CE8D8), flags 0x20020 (white 24, centred), hit test 0x428178; Quit has tag 1 (Escape) |

Main menu handlers (not wired yet): Arcade 0x420F70 (frontend_state 2), PC Mode 0x4259A4 (screen 16), Boss Mode
0x422B90, Ranking 0x423028, Options 0x421434, Quit 0x420F80. The debug items (Cheat / Edit Char Stats / Creature
Test, only with 0x5CD374 set) are not ported.

## Inventory (PC files; SPRITE\SPR.RBH sprites on BIGGER.VMC pages, coloured through MENU.TBL / MENU.PAL)

Contact sheets of all 1640 sprites: `private\menuart_scratch\sheets\pc_*.png` (`py tools/menuart_tool.py sheet-pc`).

| sprite | size | what | menu use |
|---|---|---|---|
| 0x0D3 | 640 x 480 | THE HOUSE OF THE DEAD logo over the mansion's hall | screen 1 background (the original's) |
| 0x0D1, 0x0D2 | 640 x 480 | the hall without logo; a desk with a gun and a watch | other screens' backgrounds (to be found in their builders) |
| 0x0AA, 0x0D4 | 640 x 480, 645 x 340 | the title picture; the logo alone | title / splash |
| 0x0A1 (SEGA), 0x0D5 (SEGA 168 x 50), 0x0D6 / 0x0D7 (©SEGA 1996 / 1997) | | boot logos, copyright line | |
| 0x0A2 / 0x0A3 | 640 x 480 / 320 x 240 | parchment with a crosshair | gun calibration |
| 0x0A4 - 0x0A9 | 640 x 480 | doors; dark portraits (Sophie, a zombie) | PC Mode / boss mode screens (to be checked) |
| 0x0E3 - 0x0EC | 120 x 120 | the bosses (Chariot, Hangedman, Hermit, Magician, ...) | boss mode select |
| 0x0ED - 0x0F2 | 160 x 160 | character portraits (Thomas, G, ...) ; 0x0F3 - 0x0F8 24 x 24 small ones | PC Mode character select |
| **0x0F9 - 0x0FC** | 32 x 61 | **the bony-hand cursor** (frame 0 + 3 click frames, 25 ms each: 0x42095C) | the cursor |
| 0x0D8 - 0x0E2 | 16-32 x 24 | arrows (white / yellow), slider marks | option arrows / sliders |
| 0x0AB - 0x0D0 | | big digits / letters (score fonts) | high scores |
| 0x5CA - 0x5DB | ~160 x 32 | agent ranks (Rookie .. Top Agent, CLASSIFIED; red / blue) | ranking |
| 0x5A1 - 0x5C9 | 24 x 32 | carved initials font | name entry |

Fonts (0x428494, character c from ' '): white 24 = c - 0x20 + 0x19D (flags 0x20000), **yellow 24 = + 0x23C (0x20000 |
0x40000: the hover)**, grey 24 = + 2 (0x20000 | 0x80000), white 32 = + 0xFE (0x10000), digital 11 x 21 = + 0x2DB
(0x400000), small 6 x 10 = + 0x37A (0x800000); each has the Latin-1 accented letters after '~'. Every text our extra
items need can be set in these fonts (no gap found).

Sounds (cue table 0x5CBDA8): frontend_run 0x41DD3C plays cue 1 (type 5, command 2: every sample off) and **cue 22 =
music: CD track 12** (type 3; our audio plays it as `Music/Track12.ogg` from the PC folder: the user's zip has no
music folder, so the menu is silent there unless the player's install has the tracks); the click: **cue 28 =
GAMESNDS.RBH sample 13** (type 1), played by 0x420A60 when the button / Enter / Space is pressed. No hover sound was
found (the hover only turns the label yellow).

Cursor / input (0x420A60, docs/decompilation.md "Front-end input"): arrows with acceleration, mouse motion sets the
pointer, a press starts the click animation (state 0x5CE8E8 3 -> 4 -> 5) and the activation happens when it ends;
30 s idle on screen 1 starts the attract (frontend_state 6).

## Build plan (after the user's OK)

1. Input + frame loop: port 0x420A60 (pointer, keys, mouse, click animation 0x42095C, idle -> attract) and
   frontend_input 0x41DA18 (Escape / Y / N tags); run the front end in the window instead of the text launcher once
   the files check has passed; cue 22 / 28 through our audio. Wire the main menu handlers: Arcade -> our game start,
   Quit -> exit, the rest to their screens.
2. Port the other screens one builder at a time, each compared with a harness capture: Options (0x421434 ->
   0x427494 ...), video options 0x41E428, controls 0x41F75C, PC Mode 0x4259AC (character select / stats, start
   button 0x425990), Boss Mode 0x422B90, ranking / high scores 0x423038 / 0x4234E0, boss times 0x423A90.., screens
   15 / 17.
3. Our extra items in the same widgets / fonts: an extra page "ReWrithe" from Options (model source, crosshair,
   aspect, blood, music / effects volumes, files check, desktop shortcut) and chapter select (Chapters 1-3, the
   unplayable ones in the grey font as locked); the values in launcher.ini as now.
4. Widescreen: the 640 x 480 picture pillarboxed (as the shot), the pointer mapped through the same rectangle.
5. Fallback: any load error (SPR.RBH / BIGGER.VMC / MENU.PAL missing) -> the text launcher.

## PC attract text: "Press SPACE to skip Demo" (branch claude/pcattract, 2026-10-17)

THOTD.EXE `demo_skip_prompt` 0x41B3F8 (UNVERIFIED (teaser): routine from the decompilation, placement compared with a harness
capture): while an idle screen of mode 2 / 3 / 9 runs (0x419168 sets 0x5CE658 each frame without input; never in mode 8) the
line blinks every 500 ms (ours 30 ticks), centred (0x427F64 width: glyph width + 1, space 8) at y 0x168 of 640 x 480, drawn
by 0x41AA54 in the font of flag 0x20000 (sprite = character - 0x20 + 0x19D). Ours: `Hud::drawPc2D` (`--models pc` only);
`Runtime::coin_prompts` draws no PRESS START BUTTON / FREE PLAY in the PC attract. The arcade model source is unchanged.
Side by side (ours left, boot_idle pc_s0018 right; text identical in place and look):
`private\menuart_scratch\attract_text_side_by_side.png`. Open: the real blink phase (timeGetTime based), whether modes 1 / 6 / 7
show it (the original's condition lists 2 / 3 / 8 / 9 only). Pause screen comparison not done this round.

Menu music (2026-10-19): More Options -> "Menu Music" (launcher.ini `menumusic` = a PC CD track 2-12, `--menu-music N`; title, Chapter 1-4, boss, final boss, clear, game over, ending, name entry). Default 12 = Chapter 4 (PC cue 22, what the original PC menu played); 11 = name entry (cue 21). It goes through the music numbering, so Saturn rips play the right song.
