# T1: the boundary report (the work queue)

Every call into something the runtime does not really implement, from headless default-route play-throughs of
Chapter 1 (`hotd --frames 70000 --autofire --mute --lives-kept 1 --boundary-report <file> ...`; all runs:
`tools\t1_playthroughs.ps1`). Sorted by calls; per 1,000 frames of the run; first frame = the game frame of the
first call. Kinds:

- **stub**: a generated boundary stub of the port (src/port/decl/*.h through tools/gen_runtime_bounds.py): does
  nothing, returns 0. The address comes from the stub's comment or names.tsv.
- **unported**: a routine address the runtime's dispatch does not know (an object's routine, a table call): a no-op.
- **partial**: an unported part of a runtime routine (`missing("...")` with a description).
- **stand-in**: a runtime replacement of a system that is not ported (the mouse as the light gun, shadows not drawn).
- **guard**: the teaser cuts' event-script guard passing a wait (none left on the strict runs).

The HUD / 2D sprite entries (hud_*, sprite_*, tile_text_draw, voice_message*, life_bar*) belong to the "hud" worker;
sky_task / world_draw_task are drawn by the runtime's own view; render items (draw_fade_begin, geo_send_words,
shadow_*, color_row_set, the body hooks) to render. Regenerated 2026-10-16 ~02:20 (free route by default; branch paths: route_*.md runs of tools\t1_branches.ps1) (branch
claude/t1-integration).

### hotd --frames 70000 --autofire --mute --lives-kept 1 --strict --default-route

17099 frames, 13 entries.

| # | routine | address | kind | calls | per 1,000 frames | first frame |
|---|---|---|---|---|---|---|
| 1 | shadow_setup (not drawn) | 0x6C760 | stand-in | 36428 | 2130.4 | 0 |
| 2 | player_input_update (the mouse as the light gun) | 0x5DF0 | stand-in | 17098 | 999.9 | 0 |
| 3 | world_draw_task | 0x4B300 | unported | 17098 | 999.9 | 0 |
| 4 | sophie_skin_draw | 0x21C30 | stub | 4669 | 273.1 | 3071 |
| 5 | shadow_update | 0x6C9E0 | stub | 2672 | 156.3 | 4586 |
| 6 | kind182_grey_scene | 0x9B600 | unported | 1478 | 86.4 | 15620 |
| 7 | colour_blend32 | 0x23CC0 | stub | 250 | 14.6 | 6961 |
| 8 | shadow_light_reset | 0x6C2A0 | stub | 9 | 0.5 | 1 |
| 9 | player out of lives (kept at 1: --lives-kept) | - | partial | 6 | 0.4 | 2050 |
| 10 | eyeball_pop_spawn | 0x43B90 | stub | 2 | 0.1 | 8267 |
| 11 | part_blow_off | 0x69550 | stub | 2 | 0.1 | 8267 |
| 12 | palette_grey_build | 0x1A400 | stub | 1 | 0.1 | 182 |
| 13 | vblank_callback_add | 0x2780 | stub | 1 | 0.1 | 242 |

### hotd --frames 70000 --autofire --mute --lives-kept 1 --start-at 2:2600

17056 frames, 13 entries.

| # | routine | address | kind | calls | per 1,000 frames | first frame |
|---|---|---|---|---|---|---|
| 1 | shadow_setup (not drawn) | 0x6C760 | stand-in | 36676 | 2150.3 | 0 |
| 2 | player_input_update (the mouse as the light gun) | 0x5DF0 | stand-in | 17055 | 999.9 | 0 |
| 3 | world_draw_task | 0x4B300 | unported | 17055 | 999.9 | 0 |
| 4 | sophie_skin_draw | 0x21C30 | stub | 4678 | 274.3 | 2891 |
| 5 | shadow_update | 0x6C9E0 | stub | 2733 | 160.2 | 4423 |
| 6 | kind182_grey_scene | 0x9B600 | unported | 1478 | 86.7 | 15577 |
| 7 | colour_blend32 | 0x23CC0 | stub | 250 | 14.7 | 5731 |
| 8 | shadow_light_reset | 0x6C2A0 | stub | 9 | 0.5 | 1 |
| 9 | player out of lives (kept at 1: --lives-kept) | - | partial | 6 | 0.4 | 1987 |
| 10 | eyeball_pop_spawn | 0x43B90 | stub | 2 | 0.1 | 8117 |
| 11 | part_blow_off | 0x69550 | stub | 2 | 0.1 | 8117 |
| 12 | palette_grey_build | 0x1A400 | stub | 1 | 0.1 | 182 |
| 13 | vblank_callback_add | 0x2780 | stub | 1 | 0.1 | 242 |

### hotd --frames 70000 --autofire --mute --lives-kept 1 --route 1:3,1:9,1:11 --strict --default-route

18498 frames, 13 entries.

| # | routine | address | kind | calls | per 1,000 frames | first frame |
|---|---|---|---|---|---|---|
| 1 | shadow_setup (not drawn) | 0x6C760 | stand-in | 42159 | 2279.1 | 0 |
| 2 | player_input_update (the mouse as the light gun) | 0x5DF0 | stand-in | 18497 | 999.9 | 0 |
| 3 | world_draw_task | 0x4B300 | unported | 18497 | 999.9 | 0 |
| 4 | sophie_skin_draw | 0x21C30 | stub | 4505 | 243.5 | 3071 |
| 5 | kind182_grey_scene | 0x9B600 | unported | 1478 | 79.9 | 17019 |
| 6 | shadow_update | 0x6C9E0 | stub | 1162 | 62.8 | 5505 |
| 7 | colour_blend32 | 0x23CC0 | stub | 297 | 16.1 | 4636 |
| 8 | shadow_light_reset | 0x6C2A0 | stub | 10 | 0.5 | 1 |
| 9 | player out of lives (kept at 1: --lives-kept) | - | partial | 6 | 0.3 | 2050 |
| 10 | prop_bookshelf_collapse | 0x40BB0 | stub | 2 | 0.1 | 11870 |
| 11 | prop_bookshelf_remains | 0x40F50 | stub | 2 | 0.1 | 11870 |
| 12 | palette_grey_build | 0x1A400 | stub | 1 | 0.1 | 182 |
| 13 | vblank_callback_add | 0x2780 | stub | 1 | 0.1 | 242 |

