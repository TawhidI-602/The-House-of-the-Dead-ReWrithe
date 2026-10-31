# PC harness: running the 1998 PC port under control

The differential checker compares game state tick by tick. For the arcade (A) MAME gives that for free; for the PC
port's two behaviours (PA = its Arcade Mode, PM = its PC Mode) the PC game itself has to be run repeatably, with
injected inputs, and its state written after every tick. This page describes how.

Rules it follows: it only ever runs a **copy** of the game (`private\pcharness\game`, copied from the user's
install, which is never modified); nothing of the game is committed; it must not take the keyboard focus, the mouse
or the screen from the person using the PC.

## How it hooks in

`tools/pcharness/hotd_hook.c` builds (with `tools/pcharness/build.ps1`, VS Build Tools, 32-bit) into a **proxy
`dinput.dll`**. The game imports `DirectInputCreateA` from `dinput.dll`, and Windows looks next to the executable
first, so placing the proxy beside the copied `THOTD.EXE` makes Windows load it with the game's other imports,
before any game code runs. The proxy forwards `DirectInputCreateA` to the system's real `dinput.dll`; its
`DllMain` installs everything else:
- **Import-table hooks** in THOTD.EXE (replace the resolved import slot): `timeGetTime`, `GetTickCount`,
  `VirtualAlloc`, `GlobalAlloc` / `Lock` / `Unlock` / `Size` / `Free`, `SetCursorPos`, `ClipCursor`,
  `GetCursorPos`, `GetActiveWindow`, `RegisterClassA`, `CreateWindowExA`, `DirectSoundCreate`.
- **An inline hook** on `frame_tick` (0x43B6E0, the 60 Hz game tick): its first 8 bytes (`push ebx; push ecx;
  push edx; mov eax, [abs]`, no relative operands) move to a trampoline; the hook calls the original tick, then
  logs and records the state, keeping every register and flag as the tick left them.
- `tools/pcharness/run_pc.ps1` prepares the copy (proxy in place, dgVoodoo set to windowed, CD music volume 0),
  clears a stale "already running" atom, starts the game and stops it after a wall-clock timeout.

## Several games at once

A second game used to exit at once (its log ends after the first `VirtualAlloc`): at start (0x418655) the game
looks up the global atom `TheHouseOfTheDead`, quits if it exists, else adds it (deleted at exit); WinMain (0x563475)
also gives up when `FindWindowA` finds a window of its class. The hook keeps both to the process (`GlobalFindAtomA`
/ `GlobalAddAtomA` / `GlobalDeleteAtom` answer with a fake atom for that name, `FindWindowA` finds nothing), so any
number of hooked games run side by side (each one slower; the clock is tick-based, so results don't change). The
fixed heap addresses (0x20000000 up) are per process and don't clash. The files the game writes (THOTD.CFG,
SCORES.CFG, CSTATS.CFG: read at start, written at exit) are opened in the run's folder (`HOTD_PC_CFG_DIR`;
`run_pc.ps1` copies the game folder's ones there first), so runs never share them and every run starts from the
same settings. `run_pc.ps1` swaps in a different hook build by renaming the loaded `dinput.dll` aside, and only
rewrites `dgVoodoo.conf` / `winmm.ini` when they change. From a git worktree it uses the main checkout's game copy
(and the worktree's own hook build, if it has one).

## Making runs repeat

- **Clock.** `pc_main_loop` (0x419168) normally runs ticks from `timeGetTime` (60 per second with catch-up). It also
  has a developer mode, "one tick per drawn frame" (flag 0x5CE688 = 0; toggled by a debug key), that never reads
  the clock. The hook sets that flag before the game starts and after every tick. `timeGetTime` / `GetTickCount`
  return base + ticks x 1000 / 60 (+1 per call since the last tick, so loops that wait for time still end), so
  timers the game keeps in milliseconds (PC Mode reload time, boss timer) advance with the ticks.
- **Heap addresses.** Objects are allocated from a memory pool that the game's library takes from one
  `GlobalAlloc` block (0x563050), and the C runtime grows its heap with `VirtualAlloc`. Windows places those
  blocks at different addresses each run, so every pointer in the game state differed between two runs. The hook
  serves those calls from consecutive 64 KB-aligned addresses starting at 0x20000000 (fixed `GlobalAlloc` blocks
  become committed, zero-filled `VirtualAlloc` blocks whose handle is the pointer). After that, **two runs give
  byte-identical state for every tick** (checked over 1200 ticks, 2026-10-13).
- **Random numbers.** The game seeds its own LCG (0xDA10F0) with 0 and the C library's `rand` with 123456: no
  wall-clock seed to replace.

## Staying out of the way

- The window is created off screen with `WS_EX_NOACTIVATE`; with dgVoodoo forced to windowed mode it stays
  0 x 0 pixels. The game's window procedure is wrapped so it always believes it is active (`WM_ACTIVATEAPP`,
  `WM_ACTIVATE`, `WM_KILLFOCUS`), and `GetActiveWindow` returns its window.
- `SetCursorPos` / `ClipCursor` do nothing and `GetCursorPos` returns the harness's cursor (later: the injected
  aim), so the user's mouse is never moved or confined.
- Sound: the game refuses to start without a DirectSound device (it shows an "Application Error" box), so the
  device stays real but every buffer is held at minimum volume (`IDirectSoundBuffer::SetVolume` / `Play` patched
  in the vtable); the CD music (ogg-winmm) volume is set to 0 in the copy's `winmm.ini`.
- On Windows 11's 32-bit `user32.dll` the functions don't start with the hot-patch prologue, so process-wide hooks
  of `ShowWindow` / `SetFocus` were skipped; the import-table hooks and `WS_EX_NOACTIVATE` cover the game's own calls.

## State record

The same `HOTDREC1` file as the arcade recorder (`tools/mame_state_rec.lua`), so `tools/state_diff.py` reads both:
per tick "FRAM", tick number, frame counter (0xDA0B08), game mode (u16 at 0xDA0CDC), rand state (0xDA10F0),
then the globals of `tools/state_map.tsv` at their PC addresses (lines without a PC address are left out), then
every task run that tick, in run order, dumped whole. Tasks are caught in `task_group_run` (0x43E304), whose loop
runs each task with `mov eax, edx; call [edx]; jmp 0x43E37D` at 0x43E386 (edx = the task): those 6 bytes become a
jump to a stub that notes the task and does the same. A task's size is the one given to `task_create`
(0x43E400), kept at task +0x10.

## Injected input

The game reads its devices in four ways; the hook replaces each with a virtual device that only the input script
changes, so nothing the user does on the PC reaches a run:
- **DirectInput keyboard** (menus, keys in play): `IDirectInputA::CreateDevice` is wrapped; every device's
  `Acquire` succeeds and `GetDeviceState` returns the script's key states (256 bytes, 0x80 = down).
- **DirectInput mouse** (aim and trigger in play): `GetDeviceState` with a DIMOUSESTATE returns the script's
  relative motion (consumed once) and buttons; the game accumulates the motion itself (0x12FDB90..) and maps
  button bits through its control table (0xD5B234..).
- **GetCursorPos / SetCursorPos / GetAsyncKeyState** (menus' relative mouse, a few keys): a virtual cursor and key
  table.
- **Joystick** (`joyGetPosEx` / `joyGetPos`): always "unplugged".

Script (`run_pc.ps1 -InputScript <file>`): one event per line, `<poll> <command> <arguments>`: `dik <code> 0|1`,
`mouse <dx> <dy>`, `mbtn <n> 0|1`, `vk <code> 0|1`, `cursor <x> <y>`. The time base is the keyboard poll count,
counted from the first game tick (the attract's start): the game polls the keyboard once per drawn frame both in
the menus (which run without game ticks) and in play (one tick per frame), while the boot screens before the
attract take a varying number of frames. `pc.log` lists ticks with their poll numbers.
**Auto-aim** (`<poll> autoaim 1` / `0`, 2026-10-14): from that poll on, player 1's virtual mouse is steered at the
nearest shootable object drawn in the last tick (task +0x60 bit 0; its camera-space position +0x40 projected with
the shot ray's focal length 496, the aim being in those units; the mouse moves the aim's y the other way), with a
click when within 6 units and a right click (reload) when the rounds are out. The input then depends only on game
state, so runs still repeat (two 6000-tick auto-aim games byte-identical). `scripts/game_aim.txt` (Arcade Mode) and
`scripts/game_pm_aim.txt` (PC Mode) are the menus of game2 / game_pm followed by auto-aim; they kill enemies and get
much further into chapter 1 than the sweeping scripts, which is what the PC proofs' coverage needs.
The front end: Enter in the attract opens the main menu (front-end state 0); the menu has a software cursor
starting at (5, 5), moved by the arrow keys with acceleration (per-poll steps 1, 1, 2, 3, 5, 7, then 11); Enter /
Space / the mouse button activates the item under it. Down for 24 polls and Right for 33 polls reach the first
item (cursor (321, 222)), which starts an Arcade Mode game (front-end state 2, game mode 4). The main menu's
items are 32 pixels apart: 1 Arcade Mode start (0x420F70), 2 the PC Mode screen (0x4259A4: Ghidra shows it as a
thunk because it starts with a call to 0x42792C and falls into the screen builder 0x4259AC; screen number 16),
3 boss mode (0x422B90), ... On screen 16 the start button (label 0x4F, y 416..448) runs 0x425990:
`frontend_state = 5` = **PC Mode**. Front-end states: 0 menus, 2 Arcade Mode game, 4 boss mode, 5 PC Mode game,
6 attract, 7 back to the menus. Screen number: 0x5CE8A8 (pc.log shows it).

## The PC attract loop (seen through the harness)

One cycle is 5575 ticks (93 s): game mode 0 for 2377 ticks, then mode 1 (low half switching 0 -> 1), during which
the demos run (the random state starts again from seed 0: first value 0x3039, as on the arcade). Front-end state
stays 6 throughout. 9000 ticks take about 17 s of wall time.

## Findings made with the harness

- **x87 precision (2026-10-13):** the game's tick runs with the FPU control word **0x107F**: precision control
  24-bit (single), round to nearest, all exceptions masked; it stays so from the attract through play (the hook
  logs it when it changes). So every x87 operation in the PC's game maths rounds its result to a float32 mantissa
  (only the exponent range is wider than float32): PA / PM maths can be reproduced with float32 operations in the
  PC code's order (watch only for overflow / underflow and values stored to memory as doubles). Seen with the
  dgVoodoo 2.87.5 DirectDraw / Direct3D wrapper; Direct3D itself also switches the FPU to single precision when a
  device is created without FPU_PRESERVE, so 1998 hardware most likely ran the same way.
- The PC game mode is the u16 at 0xDA0CDC; front-end states 0 / 2 / 4 / 5 / 6 / 7 (see above).

## Milestones

| | Milestone | State |
|---|-----------|-------|
| a | the copy runs under the harness and logs its tick | done 2026-10-13 |
| b | game state after every tick | done (globals and task bodies) |
| c | two runs of the PC attract / demo byte-identical, PA then PM | **done**: the attract (6000 ticks, the whole cycle with its demos) is the same for both modes (front-end state 6); PM itself is checked through (d) |
| d | injected inputs for a short scripted game | **done**: `scripts/game2.txt` (Arcade Mode) and `scripts/game_pm.txt` (PC Mode): menu -> start -> ~3000 ticks of play with mouse shots, kills, reloads, lost lives; two runs of each byte-identical |
| e | differential run: our runtime fed the PC's own gun, state compared frame by frame | **first version** (pccap, 2026-10-27): "Differential run" below |

## Boss Mode scripts (pccap, 2026-10-27)

`scripts/game_boss_all.txt`, `game_boss_chariot.txt`, `_hangedman`, `_hermit`, `_magician`: the main menu's third item
(Boss Mode, screen 7: Down 24, Right 33, Down 10 polls put the cursor at (321, 285)), then the picture (All Bosses:
the centre, under the cursor already; the others: 24 polls left / right and 23 up or 15 down), Enter, auto-aim from
poll 800. With `-InfLives` the fights run to the win; after it the PC goes to the name entry (screen 15), where the
auto-aim's Enter (game mode 7 only) does not reach, so the run idles there: its record is complete by then (a run
stuck there was stopped by its own PID). Seen: the pick sets boss_mode 0xC723B8 = 2..6, the PC's walk-up takes 490
ticks (Chariot), the clock (0x5CD37C start time, 0xC723D0 the ms) starts 180 ticks after the entrance (0x5CD380 = 1).

## The extra state map (`-Map`)

`run_pc.ps1 -Map tools\pcharness\pccap_map.tsv`: `tools/state_map.tsv` plus the gun records (0xD9F5E0, 0x60: aim
x / y at +4 in the shot's units), Boss Mode's state (0x5CD360..0x5CD387: pc_boss_next, the clock's start time and
shown flag; 0xC723B8..0xC723D3: boss_mode, index, result, the clock in ms), the mouse buttons (0x12FDB80 / 0x12FDB84)
and camera_pos (0xD9F038). Note: `state_map.tsv`'s PC enemy_count 0xDA0CCC is a guess; THOTD.EXE's is 0xDA0CCE
(globals.h), inside the same recorded 4 bytes.

## Differential run (pccap, 2026-10-27)

1. Capture: `run_pc.ps1 -Out <run> -Ticks N -InputScript scripts\<script> -InfLives -Map tools\pcharness\pccap_map.tsv`.
2. `py tools/pc_diff.py trace <run>\state.rec pc.txt` and `py tools/pc_diff.py gun <run>\state.rec gun.txt`:
   the PC's state per game frame, and player 1's gun (a shot = one round less, a reload = rounds up, a pull that fired
   nothing = the mouse button's press edge, aim from the gun record).
3. Ours: `HOTD_GUN_REPLAY=gun.txt HOTD_STATE_TRACE=ours.txt hotd.exe --frames N --lives 11 --lives-kept 1 --mute`
   with `--chapter 1 --pc-mode` or `--boss-mode <boss>` (`--lives 11` = 5 lives / max 5, the harness copy's settings).
4. `py tools/pc_diff.py diff pc.txt ours.txt [--tol 0.2] [--from F]`: the first frame each field differs (mode,
   chapter, event, rand state, floor_y, enemies alive / count, players 1 / 2, Boss Mode's clock, the shootable
   objects: state, hit points, position within --tol).

More digits (pmbody): `pc_diff.py trace <rec> <out> --digits 9` and `HOTD_STATE_DIGITS=9` on ours, then `--tol 1e-10`.
`HOTD_STATE_CAM=<file>` writes ours per traced frame: the PC-maths stored matrix 1, camera record 0 (eye, angles,
look-at) and camera_pos (with `HOTD_STATE_CAM_HOMES=1` each shootable object's home, motion, frame, flags, state and
direction; `HOTD_STATE_CAM_ALL=1` every task); `tools/pcharness/pmbody_map.tsv` adds the PC's camera records and stored
matrices to a capture to compare them with.

With `-InfLives` (pmrest2): add `HOTD_LIVES_MIN=3` on ours (player 1's lives raised to 3 before every frame, as the
hook does before every tick: the bosses' aggression level follows the lives) and `--inf-lives` on the diff (lives not
compared). The PC Mode captures need `--difficulty 0` on ours. pc_diff.py leaves out effect tasks whose +0x58 / state
words are the heap's leftovers (list in the script) and the objects of a chapter-change tick.

Frame numbers: game frame 0 = the first frame in game mode 4; the PC's game_new ticks (mode 4, player 1 still in
state 0: the start of every game, each next boss's too) are left out, as ours runs that step inside its first frame;
Boss Mode's walk-up ticks count on both sides (ours runs them inside one call). Checked: floor_y's first change, the
boss entrance, the Chariot's first frame and every clock start / stop land on the same frame.

