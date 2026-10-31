# The PC version's secret codes (SKIDMARX, CREATURE)

Tip: RUNESA (YouTube comment, 2026-10-20). Thanks!

The comment: in the 1998 PC version, hold **Ctrl** and type **skidmarx** on the title / main menu for a cheat menu and
stats; hold **Ctrl** and type **creature** for a creature test mode ("below the Exit option in the title menu").
**Confirmed in THOTD.EXE** (the PC harness copy, `private\pcharness\game\THOTD.EXE`), read from the decompilation
(`private\decomp\pc\thotd.c`) and the disassembly (`tools/x86dis.py`). Not checked by typing into the PC harness (the
codes need a real Ctrl + key sequence in the harness's DirectInput; not done in this session).

## The key check (frontend_input 0x41DA18, tail 0x41DC3C..0x41DD38)

- Strings: `"SKIDMARX"` 0x5A118C, `"00000000"` 0x5A1198 (the buffer's reset value), `"CREATURE"` 0x5A11A4 (plain ASCII,
  not encoded; file offsets 0x18858C / 0x1885A4 in this copy).
- While neither Ctrl is held (DirectInput state `[0x12FDBE0]` bytes 0x1D / 0x9D, bit 7) the 9-byte buffer 0x5CE89C is
  set to "00000000".
- With Ctrl held, for each key in the list 0x5CE868 (DIK codes of the letters, 0-terminated) with a press edge
  (`cur & (cur ^ prev) & 0x80`), the buffer shifts left one place and the key's character from the DIK -> ASCII table
  0x5CED9C (US layout, upper case) goes into place 7. Then `strcmp` (0x571C79) with "SKIDMARX": match -> buffer reset,
  **0x5CD374 ^= 1** (the debug flag), 0x42792C, `screen_main_menu` 0x4210FC (the main menu is rebuilt, from whatever
  screen was up). Else with "CREATURE": **0x5CE898 ^= 1**, the same reset / rebuild.
- Typing a code again switches it off. Neither flag is in `thotd.cfg` (config_load 0x421DF4 / its save) or anywhere
  else on disk: they last until the game is closed.
- In the same routine, with Ctrl held: F12 = screenshot (`screenshot_save`, not in 320 x 240); with the debug flag also
  **Ctrl + B** (DIK 0x30) = 0x4270A4 "About to generate blood color files, this will take a long time" (writes the
  blood colour files; not ported) and **Ctrl + T** (0x14) = 0x5CD378 ^= 1 (not identified; not ported).

## What the flags unlock

### The main menu (screen_main_menu 0x4210FC, 0x421342..0x42142C)

With 0x5CD374 set: `chapter_select_check` 0x422808 (counts the EVTALL event lists per stage, clamps the start stage /
scene 0xD5B350 / 0xD5B354), then three more items **under Quit** (the "Exit" of the comment), text widgets (not label
ids), white 24 font, centred: **Cheat** (Quit's y + 0x23; handler 0x422784 -> 0x4228CC), **Edit Char Stats** (+ 0x19;
0x426188 -> `screen_pc_mode_edit` 0x426190), and only when 0x5CE898 is set too, **Creature Test** (+ 0x19; 0x422230 ->
`screen_creature_test` 0x422324). So CREATURE alone shows nothing; it needs SKIDMARX as well. Also with the debug flag
the main menu goes to the attract after 10 s idle instead of 30 s (0x420A60).

### Cheat (screen 6, 0x4228CC; its tail jumps to 0x41FEF4)

Background sprite 0xD1, "Cheat" (white 32, centred) at y 0x40, then rows 0x5CE8D8 (32) apart, each a name at x 0x80
(0x180 wide, white 24 left) and its value (same rect, right-aligned, hit by rect 0x428294):

| Row | Value | Click |
|---|---|---|
| Stage: | `"%s : %d"` (0x5A1320): the stage name (0x5CE918, 10 bytes each: "Stage 01 ".."Stage 04 ", "Stage tt ", "Stage adv", "Stage end") and its index 0xD5B350 | 0x422790: next stage (wraps at the event lists' count 0xD5A978), scene 0 |
| Scene: | the scene name (lists 0x5CEBA4 per stage: Stage 1 "Scene 000".."Scene 16 " (20), Stage 2 (16), Stage 3 (8), Stage 4 "Scene 00 ".."Scene 03 " incl. "Scene 02A" (5), ...) and the index 0xD5B354 | 0x4227CC: next scene (wraps at the stage's event count 0xD5A920[stage]) |
| Free-Play | label 0x25 "On" / 0x26 "Off" from byte 0xD5B35C | 0x4228C0: 0xD5B35C ^= 1 |
| Start (2 rows on) | | 0x420F70 = Arcade's start: `game_new` 0x458588 takes chapter = 0xD5B350, event_index = 0xD5B354 |
| Exit (label 0x24, 2 rows on, Escape) | | 0x4210F4: the main menu |

Free-Play: `pc_continues_init` 0x4488E4 copies it into 0xDA0B8C, which the continue code reads (`!= 1`: the
continue count is used; `== 1`: free play, no continue limit). config_load clears 0xD5B350 / 0xD5B354 / 0xD5B358 / 0xD5B35C.

### Edit Char Stats (0x426190 `screen_pc_mode_edit`)

The PC Mode character editor: each character's HitPoints / NumRounds / Bul. Damage / Reload t (ms) / HitSize
(`cstats.cfg`). Not ported: ours opens the PC Mode screen (screen 16) instead.

### Creature Test (screen 5, 0x422324)

"Creature Test" (white 32) at y 0x20; a 3D view widget (0x60, 0x40, 0x1C4 x ..; draw 0x422774 -> 0x41D598 between
0x43D8D4 / 0x43D928) showing the body object 0xD5A944 with motion 0xD5A958; under it **Left / Right** (labels 0x32 /
0x33; 0x422274 / 0x422290: yaw 0x5CEBF8 += / -= 0xB4), **last BO / next BO** (0x4222DC / 0x4222AC: 0xD5A944 - / + 1,
wrapping 0..0x5D = 94 types, names `BO_...` table 0x5F10E0), **last MO / next MO** (handlers 0x42792C only: they do
nothing), the BO and MO names (table 0x5F1258, "MO_b1_atk_a" first) at x 0x140, y 0x60 / 0x80, Exit (0x24, 0x42223C:
0x43D900, sound 0x5685B0, the main menu). 0x425CD4 / 0x425D6C (thought to be
creature-viewer routines) are not called by screen 5: 0x425CD4 switches on a widget's +0x38 (0 / 1 / 2), a widget
routine, not identified.

### In play, with the debug flag (Ctrl held)

- `pc_main_loop` 0x419168: **Ctrl + 1 / 2** (DIK 2 / 3) toggle bits 1 / 2 of 0x5CCEF8 (0x412224); `pc_gun_input`
  0x4358CC then sets that player's rounds to 12 and lives to the maximum every frame (infinite ammo and lives).
  **Ctrl + 3 / 4** (DIK 4 / 5) set bits 4 / 8 (0x412200): `player_in_game_frame` 0x434C9C clears them and zeroes that
  player's +0x10 (kills the player once). **Ctrl + R** (0x13): 0x5CE680 ^= 1: both guns' input becomes mode 7 (random
  aim from `crand`, the trigger every frame: a random autofire). **Ctrl + V** (0x2F): 0x5CE688 ^= 1 (the frame pacing:
  one game frame per loop vs the timed catch-up).
- The HUD (0x418CC8): two lines at y 0x168, `"%d fps [%d:%d:%d]"` (0x5A0E98: fps 0x5CE664, the scene number
  0xDA0CF0 and two more bytes) at x 1 and `"[1] %d [2] %d"` (0x5A0EAC: the two players' scores 0xD9F064 / 0xD9F130)
  at x 0x1CC: the "stats" of the comment.

## What ReWrithe ports (src/game/frontend.cpp, src/main.cpp, src/game/runtime.cpp)

- **The codes, 1:1**: `Frontend::secretCodes` (the buffer, the reset while Ctrl is up, the letters as DIK positions:
  SDL scancodes A..Z, the two `strcmp`s, the toggles, the main menu rebuilt). Not saved, as in the original.
- **Main menu**: Cheat / Edit Char Stats / Creature Test under Quit at + 0x23 / + 0x19 / + 0x19 (ours: the ten items
  are 19 pixels apart while they show, 25 otherwise, so all fits in 480; the help line is off then).
- **Cheat**: the PC's screen (background, rows, values, On / Off labels, Start, Exit). Stages 1-4 only (ours has no
  "tt" / "adv" / "end" event lists to start); the scene counts are the PC's name lists (20 / 16 / 8 / 5). Start plays
  that chapter from that event (`chapter_start(chapter, event)`; log line "Cheat screen: chapter N from event E").
  Free-Play is shown and toggles, but the teaser has no coins / continue limit anyway.
- **Edit Char Stats**: opens the PC Mode screen (the editor 0x426190 is not ported).
- **Creature Test**: the screen with its buttons, the BO names (table 0x5F10E0), the MO name, the yaw; no 3D view on
  the page itself: ours adds **View in Gallery** (grey when the Gallery has no entry of that creature type), which opens
  the Gallery (game/gallery.h, merged 2026-10-20) at the entry whose arcade creature type = the BO index (BO 2
  "BO_tom" -> Thomas Rogan).
- **In play**: Ctrl + 1 / 2 (with SKIDMARX on, not in online co-op) toggle infinite rounds and lives for player 1 / 2
  (`Runtime::cheatInfinite`: rounds and lives kept at the most seen since switched on; SDL log line "cheat: player N
  ..."). Not ported: Ctrl + 3 / 4 / R / V / B / T and the fps / scores HUD line.
- Developer: `HOTD_MENU_TYPE="N:TEXT;M:X,Y"` types TEXT with Ctrl held at the menu's N-th frame and clicks (X, Y) at
  frame M (hidden window; with `HOTD_MENU_SHOT`): pictures of the screens in `private\screenshots`.

UNVERIFIED (teaser): read from the decompilation / disassembly and compared by eye with our captures (no PC harness
capture of these screens yet).

## Checks (2026-10-20)

`--input-test` 1478 checks, 0 failed (1480 / 0 after merging main with the Gallery); `--frames 40000 --autofire --mute --lives-kept 1` and `--models pc`: "chapter 1
ended" at 16695 (the cheats are off by default and touch nothing then); HOTD_MENU_TYPE runs: the main menu with the
three items, the Cheat screen, the Creature Test screen, its View in Gallery at BO 2 (Thomas Rogan in 3D), Start from Stage 02 / scene 3 -> "Cheat screen: chapter 2
from event 3".
