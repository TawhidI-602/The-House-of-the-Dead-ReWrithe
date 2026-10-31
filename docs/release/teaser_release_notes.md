# The House of the Dead: ReWrithe - teaser (Chapters 1-4 and the ending), release notes

*A C++ rewrite of the arcade game and its PC port, combined.*

**DRAFT, NOT PUBLISHED (feature list refreshed 2026-10-20 by the docsync worker with pcmode2-4, bossname, coop4f / g and net4p; the package notes and hashes below date from the firstrun worker's package, built before those merges; before: 2026-10-19, relsheet). Not final until
the user has read it. Nothing has been uploaded. The package and hashes below come from a fresh Release build of branch
claude/firstrun (main merged in) configured with `-DHOTD_LOCAL_ICON=OFF` (neutral icon); re-check tests and hashes on the
day of release if anything is merged after it.**

## Quick start

1. Download `hotd-teaser-<date>-win64.zip` and unzip it anywhere (any drive, any folder name, also with spaces or accents).
2. Put your own arcade ROM zip (MAME's `hotd` set) in `data\arcade`.
3. Put your own 1998 PC game (its installed folder, or a `.zip` of it) in `data\pc`; the CD music (`TrackNN.ogg`) in `data\music`.
4. Double-click `Play.bat`: the files are checked once, then the main menu comes up. The mouse is the gun.
5. Something missing? The launcher says what and where it goes; details in `settings\hotd.log`.

## New since the 2026-10-19 draft

- **Online co-op (2 players)**: Co-op > Online Co-op: host or join over the internet (share code, UPnP) or the LAN
  (lobby list), optional password, join / rejoin a running game, Play Alone; rollback with adaptive delay; encrypted (docs/netplay.md).
- **Gamepads and light guns**: 4 pad control styles (classic sticks, gyro, gyro + stick, touchpad),
  vibration, DualSense adaptive trigger and light bar; light guns that act as an absolute mouse with F9 calibration.
- **Experimental 4-player mode** (Players 3 / 4): P3 / P4 with their own lamps, crosshairs and colours, boss hit points
  scaled for 3 / 4 players, a 4P top 10; **CPU teammates** for P2-P4 (Easy / Normal / Hard). The rare 4P soft-lock is fixed.
- **Boss Mode name entry** (bossname): a new best time opens the PC's own name-entry screen (screen 15) with a letter
  grid for mouse, pad and light gun, or the keyboard; the name goes into the best-times table.
- **PC Mode takes the PC's rules** (pcmode2-3): kill credit, the life gift, the enemy's turn, shots that pass scenery, no
  shot flash, the PC's auto reload, the hit rule (confirmed equal) and the 9-task bleeding cap. Plan and inventory: docs/pcmode.md.
- **PC Mode continues and Game Speed Auto** (pcmode4): Options -> Continues (0-9, default 5) is the PC's shared credit
  pool. Game Speed has a new default, Auto: arcade 57.52 Hz, PC Mode 60 Hz (an old `gamespeed=0` reads as Auto).
- **4P cut-scene bodies** (coop4f / g): players 3 / 4 stand in the cut-scenes as Rogan 2 / G 2 (scenes 00 / 02 / 06 / 07 /
  09 / 12; 10 / 11 still show two); draw side only.
- **Online 4 players** (net4p, protocol 7): two PCs with two seats each (host seats 1 + 3, friend 2 + 4); rollback,
  state hashes, join and rejoin cover four seats; 2-player online is unchanged. Four separate PCs are not done yet.
- **Sonic Mode** (Frenzy x1.2 / Overkill x1.5 / Nightmare): faster play, cut-scenes at normal speed, own rankings.
- **Arcade speed by default**: the game logic runs at the Model 2's 57.524 Hz (60 Hz option); frame-rate choices,
  vsync and **smooth motion** (frames blended between game ticks) for any monitor.
- **CRT filter**: Off / Arcade Monitor / Consumer TV / Sharp Scanlines / Custom; aiming follows the curvature.
- **Texture packs and custom models** (`data\mods`, Texture Packs page; docs/mods.md).
- **Starts in borderless fullscreen** (F11 / Alt+Enter toggles; the choice is remembered).
- **Look fixes**: the grey / sepia scenes built as the hardware does, hit flashes and death fades, the PC models'
  own ranking / name entry art, results HUD inside 4:3, end screen on black.
- **Proofs**: the Magician and Chapter 4's kinds proven against captures; every capture set re-checked.
- **First start and support** (firstrun): files in `data\arcade` / `data\pc` lead straight to the menu; the launcher
  welcomes a new player and, when a check fails, says what to do first (arcade zip and PC game swapped, .7z / .rar or a
  CD image, a zip inside a zip, nothing chosen) with only a short list of missing files; folders with non-ASCII
  names work; `settings\hotd.log` for support; a crash writes `settings\crash-<date>.txt` (stack trace) and shows a
  message; a missing OpenGL 3.3 driver is explained in a message box; a damaged settings file is set aside; the
  console window no longer stays open beside the game.
- **Release hunt**: 132 bot runs and 14 rendered runs found no 1P / 2P crash or soft-lock.
- **The PC version's secret codes**: on the menu hold Ctrl and type SKIDMARX (Cheat: any stage / scene, Free-Play;
  Edit Char Stats; in play Ctrl + 1 / 2 = infinite ammo and lives), then CREATURE (Creature Test; View in Gallery shows the creature in 3D).
  See docs/teaser_status/pc_cheats.md.

## Thanks

- RUNESA, for the tip about the PC version's SKIDMARX / CREATURE codes (YouTube comment, 2026-10-20).

## What it is

The House of the Dead: ReWrithe is an unofficial, fan-made C++ rewrite of Sega's The House of the Dead (arcade 1996 / PC 1998). The game logic is
rewritten routine by routine from the arcade (Sega Model 2C, Revision A) and the PC version; the models, textures,
sounds and music are read at run time from files **you** own. This teaser plays **Chapters 1-4 and the ending** (rough in places, see Known limits): from the opening car ride
through the Chariot, the Hangedman, the Hermit and the Magician to Rogan's closing scene, the staff roll and the end
screen, with the branch routes, arcade or PC models, and local co-op. Around it sit the arcade's title screen and attract loop (with the 1998.12.18 prologue), name entry and the
top-10 ranking.

Not affiliated with or endorsed by Sega.

## What you need (nothing is included)

The package contains the program only. No ROM, executable, texture, model, sound or music of the game is in it. You
need, legally obtained by yourself:

1. **The arcade ROM set**: MAME's `hotd` set (The House of the Dead, Revision A), as a zip or as loose files. File
   names do not matter; the files are recognised by content.
2. **The PC game**: the 1998 PC release, installed (the sound, music and PC models come from it): its folder, or a
   `.zip` of that folder; no need to unpack it (the game may sit one folder down inside the zip; music tracks in a zip
   are read into memory when played). RAR / 7z archives are not read: convert them to a `.zip` (or unpack them).

The game checks both against the official releases (CRC-32 / SHA-1) before it starts and tells you which file is
missing or different. Your files are only read, never changed.

System: Windows 10 / 11, 64-bit, a GPU with OpenGL 3.3.

## How to run

1. Build the portable folder: `powershell -ExecutionPolicy Bypass -File tools\make_portable_hotd.ps1` (a Release
   build, then one self-contained folder: `bin\hotd.exe`, `SDL3.dll`, the MSVC runtime, `Play.bat`, `README.txt`).
   By default it is written to `private\portable\HOTD Teaser`; `-Out <folder>` chooses another place.
   Exact commands: docs/release/build_release.md (dry run: 10 files, 5.3 MB, no game data).
   Distributed as a prebuilt zip (program only, no game data) on the GitHub Release page, with the source alongside.
2. Double-click `Play.bat` to open the launcher.
3. Choose your ROM zip / folder and your PC game folder / zip, then START. The files check runs first. Or put your
   own files in the package's `data\arcade` (a ROM zip) and `data\pc` (the PC game, a .zip or unpacked): the game (`Play.bat` or `bin\hotd.exe` directly, e.g. from Steam)
   then uses them by itself.

Settings and results are kept in the package's own `settings\` folder (paths inside the package are saved relative to
it, so a moved or copied package still works); a build run from the source keeps them in `%APPDATA%\hotd-rewrite\hotd\`.

Settings in the launcher (everything is also a command-line option, see the top of src/main.cpp):

- **Models:** arcade (default: the arcade's models and textures, read from the ROM) or PC (the 1998 PC release's).
  A choice saved in the launcher is kept; on the command line `--models pc` / `--models arcade`, arcade without it.
- **Music:** on / off, and separate **music** and **effects** volumes (0-100 %). Esc back to the launcher silences the
  music. Effects stay on with music off. Music files: .ogg or .mp3 (or a .zip of them) in `data\music`; **numbering**
  auto / PC CD / Saturn disc (`--music-numbering`): a Saturn-numbered set is recognised by itself and mapped to the
  PC's songs.
- **Chapter:** the launcher has a chapter line with all four chapters pickable (a chapter started from the select plays on
  into the next ones and the ending).
- **Title / attract:** on (default) plays the arcade's title and attract loop after START; off (`--no-title`) goes
  straight into Chapter 1. Key 1 / 2 or the trigger starts the game from the title.

Display settings:

- **Aspect:** widescreen 16:9 is the default (the same vertical view as the original, wider to the sides; the
  whole window is the game area). "Original 4:3" shows the original's screen with black bars at the sides; a shot
  into the bars reloads. Either way the game runs at 60 frames a second, also on 144 / 240 Hz displays.
- **Crosshair:** *auto* (follows the models), *arcade style* (the PC sight's shape recoloured in the arcade player colours:
  1P red, 2P blue; the arcade itself has no sight sprite), *PC* (the real PC's sight sprites: 1P orange, 2P blue), or
  *simple cross* (a plain 25 x 25 pixel plus on the aim point).
- **Auto reload** (off by default): an empty gun reloads by itself, as if you had shot off the screen (the same
  reload and sound). The originals have no such option: there you shoot off the screen (or press reload). The reload
  sound itself is a normal sound effect, always on.
- **Blood colour:** red (default), green, blue, purple, yellow or **custom (experimental)**: any colour (white by default; the
  launcher cycles a few presets, any `RRGGBB` value goes in `bloodrgb=` in launcher.ini). Experimental: with the
  arcade models the bursts and gore use it; with the PC models the blood textures are tinted. Some blood (the screen
  splash) stays red with every colour setting in this teaser.

### Controls

| | Player 1 | Player 2 |
|---|---|---|
| Aim | the mouse | a second mouse (its own crosshair), or the keyboard arrow keys |
| Trigger | left button | the second mouse's left button, or Space / Right Ctrl |
| Reload | right button (a shot off screen also reloads) | the second mouse's right button, or Enter / Right Shift |
| Start / join / continue | key **1** (or the middle button) | key **2** (or that mouse's middle button) |

- **Pause (as the 1998 PC game):** **Esc** or **P** in play opens PAUSED with Continue / Exit (arrows or the mouse pick,
  Enter / Space / a click choose; Esc or P again continues; Exit goes back to the main menu). The game also pauses when
  its window loses the focus and goes on when it gets it back. The music and sounds stop where they are and continue from there.

Other keys: **F11** or **Alt+Enter** fullscreen (borderless; works on every monitor, also
after maximizing; with two monitors the mouse stays inside the window), **F12** screenshot,
**F1** developer lines. Once a second mouse is used in the game, the game captures the mice (relative mode: no desktop cursor
until the game is left); with one mouse the desktop cursor stays. "Player 2 input: keyboard" never captures it.
The teaser always runs in free play (no coins).

## New in this build (2026-10-19)

- **Chapters 2, 3 and 4 and the ending are playable**, with arcade or PC models: chapter select lists all four. New bosses:
  the **Hangedman** (2), the **Hermit** (3) and the **Magician** (4); Chapter 4 opens with a Chariot rematch. After the
  Magician the ending plays (Rogan's scene, the staff roll, END / results card, name entry).
  A headless bot gate (32 seeds, 1 and 2 players, both model sources) found 0 crashes or soft-locks and 7 full clears.
- **Sound fixes:** the sound board mute is gone (game audio now plays from start to end), the pops/clicks are fixed, and
  the loudness is balanced.
- **PC music:** the PC CD tracks follow the 1998 patch-9 numbering; the music numbering can be set in Options -> Audio.
- **Menu music choice:** pick the main menu's song among the PC CD tracks (`--menu-music N`).
- **More crosshairs and your own PNGs:** extra built-in sights (ring + dot, corner brackets, gap cross), Crosshair Size and
  Thickness, and custom .png sights from `data\crosshairs` (see README.txt).
- **Options tabs:** the options are split over tabs (display, audio, crosshair, Add to Steam).
- **Widescreen fixes:** black sides on the chapter title cards, edge-anchored HUD, and the end screens drawn full width.

## What works

- Starts from your own ROMs and PC folder alone (no MAME snapshot, dump or other helper file); both may stay zipped.
- Chapters 1-4 and the ending from start to end (default route and branches), including the Chariot fight, Sophie's scene and
  the three new boss fights.
- **Title, attract and prologue:** the arcade's USA loop (FBI screen, the 1998.12.18 prologue with the Dr. Curien scene,
  title over the mansion, how-to-play, Chapter 1 demos, ranking, FREE PLAY / PRESS START). With the PC models there
  is no FBI screen and the prologue follows START.
- **Name entry and top-10 ranking** after GAME OVER and after the clear when the score qualifies; the ranking is saved
  in `ranking.bin` in the settings folder (above).
- **Widescreen 16:9** (default, the same vertical view as the original, per-player HUD at the window edges) or
  Original 4:3 with black bars; **60 Hz game logic on any monitor** (60 / 144 / 240 Hz give identical play).
- **Windows:** F11 / Alt+Enter on every monitor, the mouse kept inside the window on two monitors, an off-screen shot
  (also at the window edge or in the bars) reloads.
- **The 1998 icon and a desktop shortcut:** the window icon is read from your own THOTD.EXE; the launcher's
  [ DESKTOP SHORTCUT ] button (or `hotd.exe --create-shortcut`) makes a shortcut with that icon.
- **Add to Steam:** Options -> System -> Add to Steam (or the launcher's [ ADD TO STEAM ], `hotd.exe --add-to-steam`) adds
  the game to Steam as a non-Steam game, with artwork from your own PC files; close Steam first. Pressing it again
  updates the entry; [ REMOVE FROM STEAM ] takes it out. Steam's shortcut list is backed up first.
- Every branch route of the chapter (shots and rescues choose it, as in the arcade). Headless test play-throughs of
  the default route and the seven branch paths reach the chapter clear, with one and two players and arcade or PC
  models (docs/teaser_status/t1.md).
- Arcade models and textures (read from the ROM) or the PC game's models; the 2D layer (HUD, lives, ammo, continue
  screen, subtitles, results sprites) in both looks.
- Local co-op: a second player joins with key 2 or the second mouse.
- **Dark doorways are the original's:** the black rooms seen through open doors (e.g. the researcher room) match MAME on the same route.
- **Attract fixes:** the prologue's last scene no longer freezes (the next world loads in the background); the sewer demo's zombie and the how-to-play torches are drawn.
- **Polish:** Sophie's green pupils no longer poke out at the Chariot's end; far fewer flickering textures (depth ties between coplanar decals dropped from 23 to 5 sampled frames).
- Sound effects and music (music: see Known limits), with the launcher's music / effects volumes.
- The opening's sepia effect, the sky, rain, blood and shot effects; PC mode with the real PC's HUD torches, hit
  sparks (white puffs), continue screen, game-over route map and palette look; arcade mode with the Model 2
  microtexture (Chariot room), shadows, palette animation and the HOLD YOUR FIRE! blink.

## Known limits

From the status files' open lists (docs/teaser_status/):

- **Chapters 2-4 and the ending are new and rough.** They are read from the disassembly and only partly compared with MAME
  snapshots (see the checkpoint contact sheet tool, tools\checkpoints.ps1): timing, camera and small details can differ.
  The boss select is not in the teaser; the attract loop's Chapter 2 demo is skipped.
- **Hangedman (Chapter 2):** the wings were missing in the play-tested build (MAME shows big green bat wings).
- **End screens:** the "chapter clear" panel is drawn over the frozen name-entry picture (cosmetic); on the 16:9 results
  screen the 1P ammo / torches sit at the window edge.
- **Ending:** the palette colour-curve fade-ins and window lines are not drawn; with the PC models the staff-roll credit
  sheets and the results card use mapped pictures, and the PC game's own ending was not compared with the real game.
- **PC models:** the Chapter 1 door before the Chariot room draws without its room (the PC files have no polygons for it);
  a reported floating skull is not found yet; the attract ranking lacks the RANK cards; the PC models are unlit, so
  brighter than the arcade's dark rooms (MAME is as dark).
- **Chapter 4:** the Chariot rematch drop-in carries a soft-lock guard (a deviation from the arcade); bots never pick the
  Chapter 4 branch.
- **Remaining flicker:** the PC models' door halves can still tie in depth; the PC prologue and RESCUED-text timing are not exact.
- **Not ported yet (logged and skipped):** coin / credit prompts (the teaser is always free play); the results
  backdrop model is not drawn; the enemy fade-out is not drawn; some palette effects are incomplete.
- **PC mode details:** the prologue and the route-map walker are only partly the PC's own; the PC game's HUD and
  continue screens were compared by eye only.
- **Fallen bodies:** the researchers lying on the ground in the opening were adjusted to sit on the ground (a whole-body
  depth pull); checked on screenshots, not against MAME frame by frame.
- **Custom blood (experimental):** the red screen splash stays red with every colour, as in the originals.
- **Looks differ from the original:** the sky dome's mountains look taller than in MAME; the arcade's warm overall
  tint at the opening is not exact; in PC-model mode the few parts the PC itself does not draw are left out.
- **Not tested by a person:** two physical mice (on a system that reports all mice as one, player 2 is
  keyboard-only), the folder / zip dialogs on other machines, sound on other sound cards, the HUD at unusual window
  sizes. Tested from a fresh profile with a bare PATH (docs/release/build_release.md); one run on a second PC still advised.
- **Music** (a launcher setting: on / off, volumes) needs the stb_vorbis library at build time (OGG tracks of the PC game); a build without it plays no
  music. Confirmed 2026-10-16: a Release build configured with `-DHOTD_STB_DIR=<folder with stb_vorbis.c>` (or
  `-DHOTD_FETCH_STB=ON`) decodes and mixes the music from the packaged exe (docs/release/build_release.md).
- **Join details** (the arcade's character re-init on a mid-game join, play-time bookkeeping) are simplified.
- **Safety nets:** where an unported routine would make the game wait forever, the teaser has "guards" that end the
  wait after a few seconds, so those moments can differ from the arcade.

## UNVERIFIED count

The core systems (shooting, damage, the standard enemy, many enemy kinds) were proven against captures of the
arcade and PC games (docs/proof.md). Everything written for the teaser is read from the disassembly and checked by
play-throughs and screenshots only; it is marked `UNVERIFIED (teaser)` in the source. `py tools/unverified_list.py`
currently lists **45 routines** plus 50 file / section notes (98 markers) after the later proof rounds (the others were
checked against MAME captures with 0 differences). Expect small differences from the
original there.

## Reporting problems

Report bugs in the repository's GitHub Issues (link added when the repository is public). For graphics problems attach an F12 screenshot. Do not attach game files.

## First launch and checksums

The program is not code-signed (decided): Windows SmartScreen may say "Windows protected your PC / unknown publisher".
Click **More info**, then **Run anyway** (an unsigned program from a new publisher is flagged until Windows has seen it often). To check your download, compare it with the SHA-256 values published with the
release (zip and every file in it, printed by `tools\package_hashes.ps1`):

    Get-FileHash .\hotd-teaser-<date>-win64.zip -Algorithm SHA256

### SHA-256 values (package of 2026-10-20, rebuilt from main ec27f0c, DRAFT: regenerate if anything is rebuilt before publishing)

Printed by `make_portable_hotd.ps1` (tools\package_hashes.ps1) into `SHA256SUMS.txt` beside the zip. Package:
`private\release\2026-10-20 firstrun\HOTD ReWrithe` (16 files, 7.6 MB), built from branch claude/firstrun at 2ee4df6
(main merged in) with `-DHOTD_LOCAL_ICON=OFF` (the neutral icon). The zip is `Compress-Archive` of that folder (2.96 MB).

```
4619012ef69f4d1a84428dd6f11c29aa3eef4c1251b1ae56101575b7e423ff42  hotd-teaser-2026-10-20-win64.zip
04fee090b0ce7c51493a9f8469d71ae473bfa54778d925f4d44a21d682db969c  bin\hotd.exe
0f885b509a685d2bbfa652fed26b5fb31d88fbdab0a978c641d1c7b8aa460aa9  bin\msvcp140.dll
bfad5aef4c63a669e3c140655cdfdf395b6c979b400a447bd5dcb65ed8826c3d  bin\msvcp140_1.dll
3ea06f0ee098b4823cb79599df3780e7f23cce52c19aac31d2a0d47efe33a5e9  bin\msvcp140_2.dll
b026c17c1b924a7fe6019a1a8958b91d6e8e8cb65762347be8298235889160b4  bin\SDL3.dll
d5e4d9a3e835fa679450145d6a7d94e36573a509317111904d9b3712c30d9066  bin\vcruntime140.dll
1f2d41c4aa5db0bc33ebf7b66d72943a817d7ce6cbe880502a9403823633093f  bin\vcruntime140_1.dll
f7f49456957aa0e42a8d9123ac3162818cf5fc9fb8bdaf5c5c1188f76265d9be  data\arcade\put the arcade ROM zip here.txt
af923ecfa714ac460d50e92197b05cc31242d75e0885e83a59744da5b19c01ee  data\music\put the music here.txt
2ec4236e5e1ac8c8314e58a821f5298339088d8bce72b2d7dad74f642120d34d  data\pc\put the PC game here.txt
4b666e8a5f0fe1d16e1f419a239f0767dc2f0a877955b05bea6e20b8d2bab9eb  data\steam\Optional downloads (SteamGridDB).txt
6a1fceb08105ddc839d85c0522ae994bc1a02f03c09e88c45db09fed7e7a3305  hotd_package.txt
0f23b570671994ccc9a7eb520958953dbedcfd3c04efaa2245238432cf993a77  LICENSE.txt
ecb4e15b51496c554725897ce5560d8c1bb70f300a79e982cae3cd2f3dc6d32a  NOTICE.txt
58920ed31b3c97e111be88b39b8ad62492779ee23d4813b9f3baa40d71b0bde1  Play.bat
00732adb18d81291c4d15b6792ee704e2a84c3fa6ce1f98e807d9f289b50010a  README.txt
```

### Tests of this build

Run from a copy of the package's `bin\hotd.exe`, each with a fresh empty HOTD_PREF_DIR, the ROM zip and the PC files
given with `--arcade` / `--pc`: `--check-files`: arcade 29 / 29, PC 156 / 156 + 12 music tracks, exit 0;
`--input-test`: 922 checks, 0 failed; headless `--frames 40000 --autofire --mute --lives-kept 1` with arcade and with
PC models: exit 0, "chapter 1 ended" at runtime frame 16695. First-start cases (`--check-files` from a package with
nothing / only the ROM zip / only the PC zip / both zipped in data\, a non-ASCII folder, swapped, .7z / .iso, a zip
of the zip, a damaged launcher.ini): docs/teaser_status/firstrun.md. Not repeated: the music WAV check and a run on a
second PC (GPU drivers, no MSVC runtime installed).
