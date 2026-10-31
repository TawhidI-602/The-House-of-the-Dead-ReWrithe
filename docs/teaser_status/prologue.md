# prologue (2026-10-16, branch claude/prologue)

1. **Mode 5 backdrop black** (src/main.cpp, before the hud / view draw): the clear colour was the renderer's default
   dark navy (0.02, 0.02, 0.04) everywhere (RenderOptions::clear, never changed). Now black while game_mode == 5
   (fallen body, route map, GAME OVER), as MAME's life_scan snaps 0087-0094; other modes keep the navy (not compared
   with MAME). Both model sources (the clear does not depend on the source). Checked: `die` run, mode 5 3 +60 (PC
   models) is black. UNVERIFIED (teaser).

2. **The "1998.12.18" prologue** (not ported, findings only):
   - Arcade: the prologue is game mode 9 (mode9_tutorial 0x15C70 -> sub-table 0xA9A80: mode9_story_start 0x15C90,
     mode9_story 0x15E50). It is part of the **attract loop**, not the game start: next_mode table 0xA9630 (read from
     maincpu.bin) for region 1 (USA, ours) is mode 0->0, 1->9, 2->8, 3->6, 4->7, 5->7, 6->1, 7->1, 8->3, 9->2, so the
     attract runs 1 (title) -> 9 (prologue) -> 2 -> 8 -> 3 (demo) -> 6 -> 1. Export (region 2) boots straight into
     mode 9 (boot_start). Coin + start goes to the chapter card without it. The PC instead plays it before Chapter 1
     (PC 0x43BB28 = set_mode(9)), a PC-only change. No MAME capture of mode 9 checked (none found quickly; to capture:
     let MAME's attract run past the title, Lua reads game_mode).
   - Ours: boot.cpp jumps from the boot state to the chapter (no attract modes run), so there is no "matching mode"
     to play it in; playing it before Chapter 1 would follow the PC.
   - What it would take: mode9_story_start (chapter = 5, event 0 = the prologue event list, texture area 2, sky mode
     1, sprite banks 0x14-0x18 = 0x38-0x3C via mode9_sprites_load, player_char_models_set, task group 0xA9A90 =
     evt_chapter_start, hud_mode_init); mode9_story (geo frame, the group, frame counters 0x5123B4 / 0x51F9D4,
     FUN_00034AE0, next_mode on chapter_ended); sprite_scroll 0x15EE0 (the caption / date sprites, tables 0xA9AB0 /
     0xA9AD0, timed by 0x5123B4); the prologue object kinds (kind113 / 114 model_timed 0x161B0 / 0x16210 and the
     other prologue / letterbox / caption kinds named in session-history); chapter slot 5's event list loading
     through the existing chapter loader (check chapter_load for slot 5); a runtime flag or launcher option to run
     mode 9 then mode 4 (PC order). Note the port treats chapter index 5 as "chapter 6" in enemy_scripted.cpp /
     damage.cpp: check those branches against mode 9 before reusing chapter = 5. Estimate: one or two batches.
