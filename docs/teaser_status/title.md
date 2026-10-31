# title: the arcade's title screen and attract loop (2026-10-16, branch claude/title)

Build folder `build-title`. Everything new is **UNVERIFIED (teaser)**: ported from the typed decompilation
(private/decomp/arcade) and the disassembly, compared with a MAME attract capture (frames and the mode timeline), no
capture proof. Code: src/game/attract.cpp (Runtime members), small hooks in flow.cpp, runtime.cpp, stand_ins.cpp,
chapter.cpp, view.cpp, arcade_loader.cpp, launcher.*, main.cpp.

## What plays now

After the launcher's START (launcher option "Title / attract", on by default; `--title` / `--no-title`), the boot's
first mode runs (USA: 1) and the arcade's attract loop plays in Chapter 1's data, as the next-mode table 0xA9630 orders
it: 1 -> 9 -> 2 -> 8 -> 3 -> 6 -> 1. FREE PLAY and the blinking PRESS START BUTTON are coin_update's free-play prompts.
A start (key 1 / 2, a mouse's middle button, or a trigger pull in the attract) goes through the original's
player_start_check -> set_mode(4) -> game_new: Chapter 1 from event 0 with the players who pressed start.

| mode | what | frames (ours / MAME 57.5 Hz) | notes |
|---|---|---|---|
| 1 | WINNERS DON'T USE DRUGS (sprite 10) | 361 / 6.3 s | arcade models only (the PC has no such screen: skipped with PC models) |
| 9 | the "1998.12.18" prologue (chapter slot 5 = Chapter 1's world, texture area 2) | 2378 / 41.6 s | sprite_scroll, kinds 104 / 107 / 108 / 109 / 113 / 114, scenarios 16 / 17 / 20 / 23 / 24; the last scene (kind108 -> Chapter 4's world) is drawn without the world; the white fade (kind104) and prop_r4_mechanism are not ported; kind 109's double door behind Sophie at (+-15, 0, -73) as the disassembly has it (2026-10-31 attdoor: the typed decompilation lost the translate, the doors were drawn behind the camera) |
| 2 | the title: the mansion on camera path 0x69 (title zone list 0xC0C20), the logo from frame 184 | 478 / 8.3 s | title code (L / R starts) ported |
| 8 | how to play (chapter slot 4: the courtyard), the lesson zombie (kind 130), the lesson player (kind 129), recorded input 0xC5BC0 | 1625 / 28.2 s | whole lesson script; the torches' lamps (clipped base) drawn (sewer.md) |
| 3 | attract demo: Chapter 1 event 5 (1092 frames), then event 15 (1023) on the next loop, recorded input | as listed / 19 s | demo 2 (Chapter 2 event 9) skipped (Chapter 1 data only) |
| 6 | ranking: the backup RAM's default top 10, the room model 0x123B | 480 / 8.3 s | the rising camera's view differs from MAME's (not checked); no name entry after a game |

PC models (`--models pc`): no FBI screen; the start plays the prologue first, then Chapter 1 (the PC's order, PC
0x43BB28 = set_mode(9)); a start during it skips it; a start during the attract's own prologue goes straight to the
chapter. The prologue's world in PC models shows grey (PC textures for area 2 not loaded).

Automated / developer runs are unchanged: `--frames`, `--no-launcher`, `--checkpoints`, `--auto-start`,
`--input-test` go straight into Chapter 1 unless `--title` is given. Checked: the default route headless to the chapter
clear (no unported routine); `--frames 7000 --title` loops the whole attract twice.

## Checks (window screenshots, second display, no focus)

`hotd --root <repo> --no-launcher --title --models arcade --mute --shot-at F ...`: frame 100 (FBI), 405-2590 (the
prologue's rooms: desk / phone, the lab, the sewer, Rogan, Curien), 600 / 2950 (the title), 3261-4641 (how to play:
the lesson, RELOAD, SHOOT OUTSIDE OF THE SCREEN), 1300 / 4900 (the demo with PRESS START BUTTON), 6050 / 6100 (the
ranking), start at 1200 -> the chapter card -> Chapter 1. Side by side with MAME's attract
(`tools\run_mame_attract.ps1 -Out private\captures\attract_us -Seconds 260`: snapshots every second, modes.txt =
the mode / sub-mode timeline; compressed).

## Open

- The white fade of kind104 (palette fade; the runtime has none), prop_r4_mechanism and Chapter 4's world in the
  prologue's last scene; Tom's body_parts_mark_role (his shot's parts).
- Demo 2 (Chapter 2 event 9) once Chapter 2 loads; mode 0 (the Japanese warning) is not reached in USA.
- After a game over the host restarts at the title (mode 1); the original goes 5 -> 7 (ranking) -> 1, with the name
  entry first when a player ranks (not ported).
- The ranking room's camera (camera_matrices_update from camera_globals_reset's record) looks out of the room late in
  the second page; MAME shows the wall throughout.
- Sounds: the attract's cues go through the PC cue table as the game's do (not listened to).
