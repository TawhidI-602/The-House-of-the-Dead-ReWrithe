# title-2: name entry, ranking backup, the prologue's fade, the ranking camera (2026-10-16, branch claude/title-2)

Build folder `build-title2`. Everything new is **UNVERIFIED (teaser)**: ported from the typed decompilation
(private/decomp/arcade) and the disassembly (tools\i960dis.py on the program ROM), no capture proof. Follows
docs/teaser_status/title.md (the attract loop).

## 1. The name entry and the ranking's backup (src/game/name_entry.cpp)

What the arcade does (read from the code, table 0xA9630 for USA): a game over goes 5 -> 7 (the ranking) -> 1, **without**
a name entry: rank_insert (0x5E5D0) has one caller, game_ending (0x352A0, after the ending). A place in the top 10
there -> mode 4 sub-mode 9 / 10 (game_name_entry_start 0x5D500 / game_name_entry 0x5D6D0, task group 0xC5470), then
next_mode: 4 -> 7 (the ranking) -> 1.

The teaser (Chapter 1 only): the chapter clear stands in for the ending. game_chapter_end_wait -> game_ending_rank:
the players' rank cards (score_rank_get 0x9BE20, as kind179_chapter_results sets them), rank_insert; a place -> the
NAME ENTRY: texture area 2, the backdrop 0x11F, NAME ENTRY 0x124, YOUR RANK (1ST / 2ND / 3RD, 4TH..10TH), the 33
letter models (A-Z . - ? ! back forward END) under the guns (a 20 x 15 box, the highlight 0xAC9), the shot letter
spins and flies into its slot, back drops the last one, the countdown model 9..0 (20 s), banned names blanked
(list 0x5E520), the backup saved. Then:
- with the title (launcher "Title / attract" on, `--title`): the ranking (mode 7) with the new entry, then the attract;
- without it: the end screen (as before). Automated runs (`--frames`, `--checkpoints`, `--auto-start`,
  `--input-test`) keep the old end unless `--name-entry`.
A game over with the title now goes to the ranking (mode 7) and the attract instead of restarting the chapter.

Input: the gun (the players' state 8, player_shoot_only 0x5600: fired this frame = player +4 bit 1) or the keyboard
(the teaser's addition): A-Z, `.`, `-`, `/` = ?, Shift+1 = !, Backspace = back, Tab = forward, Enter = END; a key is a
shot at its letter by the first player with a place (one key every 8 frames).

Backup: the top 10 (backup RAM 0x1D00104, 12 bytes an entry: name, rank card, score) from the ROM's defaults
(0xAA4A0), then `%APPDATA%\hotd-rewrite\hotd\ranking.bin` (120 bytes, checked: printable names, card < 8); written
after a name entry. `--ranking-file <file>` another file; `--frames` runs use none (the defaults).

Checked: headless `--frames 30000 --title --start-at 0:200 --autofire --lives-kept 1 --name-keys "TAX<WZZ#"` with a
low table: clear at 16621, place 1, name "TAW", mode 7, attract again; game over with the title: 5 -> 7 -> 1 -> 9 ...;
window `--no-launcher --event 19 --autofire --name-entry --models arcade` checkpoint `mode 4 10 400`: the screen as
described (letters, 1ST, the underscores, the countdown). No MAME capture of the name entry (it needs the whole game).
Open: whether the arcade shows player 2's PRESS START BUTTON there (ours does: the waiting player's prompt).

## 2. The prologue: the white fade and the last scene

- kind 104 (every scene cut of the prologue, 8 times): kind104_fade_start / kind104_fade_task (0x19060 / 0x18E30), a fade
  **from** white over 33 frames (palette_blend_white 0x1A110: colour = white - (white - colour) x level / 32). The
  runtime keeps the level (Runtime::whiteFade); the HUD draws white with alpha (32 - level) / 32 over the 3D picture,
  under the 2D layer. MAME (`tools\run_mame_attract.ps1 -Out private\captures\attract_fade -Seconds 14.5 -Every 0.05`,
  snaps 0128-0132) shows the same: the desk scene from white, PRESS START / FREE PLAY not whitened.
- The last scene (Curien, kind107 area 12, kind108 slot 3 = Chapter 4's placement table 0xC0A90): GameView::ensureExtra
  loads Chapter 4's stage on first use (arcade: its placements, texture sets with area 12 added (arcade_loader.cpp
  romStageTags: area 12 is in no chapter's list), every model ID as the chapter's; PC models: the PC's Chapter 4
  stage), about 350 ms once per session (arcade), kept. Its draw zone's placements are drawn with area 12's set, and
  the game's models (Curien) come from that stage. Compared with MAME attract_us snaps 0045-0049: the monitor wall,
  Curien, the hall. Other workers' files touched: src/game/view.* (render), src/assets/arcade_loader.cpp (s1), the
  white layer in src/game/hud.cpp (hud) and the crosshair there in the name entry (players' state 8).

## Coordinator's extra requests

- (a) the Curien scene: done above (section 2).
- (b) the title logo looked transparent: Arcade2D::compose (src/assets/sprites_arcade.cpp, hud's) made every pen whose
  colour comes out black after MAME's gamma transparent; now only pen 0 is (as the System 24 chip): the logo's dark
  shading and the figure are opaque, as MAME attract_us snap 0058. Side effect, as in the arcade: covers like sprite
  0xA1 are opaque black now (the ranking's end, mode switches). PC title (PC sprites): already opaque, unchanged.
  Also fixed: after the Curien scene the game's models (the title's sky) were taken from Chapter 4's stage.

## 3. The ranking camera (mode 6)

Ours (checkpoints `mode 6 1 60 / 180 / 300 / 420 / 440`) against MAME attract_us snaps 0107-0113: the same view of the
room through both pages (the dark wall, then the window and the building, rising); no change needed with main's
camera code as of this branch. The black screen seen at the end of mode 6 is the cover 0xA1 (now opaque).

## Left

Demo 2 (Chapter 2 data, skipped as asked); the name entry not compared with MAME (needs the ending); P2's PRESS START
in the name entry; ~350 ms load at the Curien scene's first showing (could be preloaded at the prologue's start).
