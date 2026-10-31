# tools

Command-line tools for working with the game data (format crackers, exporters, comparison loggers).

Python 3 with numpy (and Pillow for vmc_tool.py). Run from this folder so the tools can import each other.

| Tool | What it does |
|------|--------------|
| `vmc_tool.py` | PC `.VMC` texture files: export pages to PNG, import back (byte-identical round trip) |
| `pc_rbh.py` | PC world files (`WORLDn.RBH`): object list, export to .OBJ |
| `m2_roms.py` | Arcade zip: verify against MAME's hashes, rebuild the memory regions |
| `m2_models.py` | Arcade polygon ROM: find every model, export models to .OBJ |
| `compare_world.py` | Match a PC world's objects to arcade models (translation only), export both in PC coordinates |
| `blender_overlay.py` | Load the comparison into Blender and render overlay images (`blender -b --python ... -- <folder> [arcade obj] [prefix]`) |
| `m2_level.py` | Arcade level from arcade data alone (model table + placement tables): list tables, export .OBJ, compare with a PC world |
| `m2_textures.py` | Arcade texture sheets; export a placed level as textured .OBJ (greyscale; one image per texture) |
| `blender_textured.py` | Render a textured level .OBJ, with cameras given in game coordinates |
| `run_mame_dump.ps1` + `mame_dump.lua` | Run the arcade game in MAME headless and dump color tables, work RAM, texture RAM and screenshots |
| `mame_probe.lua` | List MAME's input ports for the running machine |
| `mame_fifo_log.lua` | Dump buffer RAM (the geometry DSP's display lists) at a chosen moment |
| `ghidra\` | Ghidra additions and headless scripts (`FixNoReturn.java` undoes wrong no return flags, e.g. rand; `FixSplits.java` folds split functions back into their parent; `ApplyTypes.java` applies the structures and parameter types of `types.tsv`): `install_specs.ps1` (Watcom compiler spec + our i960 SLEIGH module), `run_pc.ps1` / `run_arcade.ps1` (import, find functions, apply `names.tsv`, export decompiled C to `private\decomp\`; `-Reexport`) |
| `decomp_query.py` | Search the decompiled exports: show a function, grep, callers / callees, strings, call tree |
| `name_routines.py` | Structural names for arcade routines hanging off hand-named code (state tables, installed per-frame routines, switch cases) -> generated block of `ghidra\names.tsv`; hand names always win |
| `decomp_summary.py` | Digest of decompiled arcade functions: motions set (from the machine code, with MO_ names), sounds, state writes, calls |
| `names_add.py` | Add / update hand names in `ghidra\names.tsv` from a small batch file (PC address from the matcher) |
| `replay_enemy_life.py` | Tracks the standard enemies (routine enemy_main / enemy_dying) through a run's work RAM dumps: life +0x70 / record hit points +0x72 |
| `check_pairs.py` | Lists hand pairs of names.tsv whose PC and arcade sizes differ a lot (wrong-pair candidates) |
| `gen_param_types.py` | Finds the functions whose parameters are game objects / player tasks and writes `param` lines into `ghidra\types.tsv` (generated block); rerun after each re-export |
| `unnamed_clusters.py` | Unnamed arcade functions grouped under their topmost unnamed referrer (what to name next) |
| `match_functions.py` | Pair PC and arcade functions (seed tables, call / code-pointer alignment, paired globals, shared constants) -> `private\decomp\match_*.tsv` |
| `diff_pairs.py` | Paired functions whose constants differ (candidates for gameplay changes) |
| `make_portable.ps1` | Release build + every file the viewer reads, copied into `private\portable\HOTD Viewer` (own the viewer's own batch file, relative paths; personal use only, never shared) |
| `play_hotd.bat` / `play_hotd.ps1` | The user plays: mouse as light gun, infinite lives, automatic coins/start, inputs recorded |
| `replay_hotd.ps1` | Replay a recording headless and dump everything |
| `play_hotd_chapter.bat` | Like play_hotd.bat, but asks for a chapter and starts there |
| `calibrate_gun.ps1` + `mame_calib.lua` + `apply_gun_calibration.py` | DISABLED (made aiming worse): cursor-vs-gun measurement and in-game calibration; kept for reference |
| `mame_testmenu.lua` / `run_testmenu.ps1` | Drive the operator test menu headless (TEST, SERVICE, waits, gun shots), screenshots per step |
| `coverage.py` / `coverage_views.py` | Which placed models a run showed on screen; maps and views of the missed areas |
| `render_views.ps1` | Per-view comparison images, each with that moment's own texture RAM and colors |
| `mame_camera.py` | Read the arcade camera from work RAM dumps into views.json |
| `blender_views.py` | Render a level from those cameras (solid color or textured) |
| `compose_views.py` | Screenshot + renders side by side per view (renders mirrored: the game world is left-handed) |
| `m2_displaylist.py` | Parse geometry DSP display lists from buffer RAM dumps: objects drawn, light vectors, lighting parameters |
| `m2_palette_fx.py` | Arcade palette animation (maincpu 0x25310) in Python; `check` compares it with MAME dumps |
| `arcade_states.py` | Alternate-state placements (same position): where the replay drew each one -> `private\arcade_states.txt` |
| `arcade_props.py` | Event objects (doors ...) from the replay: world transforms + camera positions -> `private\arcade_props.txt` |
| `texset_seen.py` | Appends to a texsets file every set the replay drew each model in (fixes models missing a valid set) |
| `evt_dis.py` | PC event scripts (EVTALL.RBH): disassembler with the opcode names from THOTD.EXE (formats.md §29) |
| `m2_anim.py` | Arcade creature skeletons and motions from the ROM: list types/motions, part matrices of a frame, check against a work RAM dump |
| `texset_assign.py` | Arcade texture sets per chapter from a full-game replay: sets, per-model set, camera zones, lighting -> `private\arcade_texsets.txt` |
| `viewer_compare.py` | hotd_viewer render (arcade and PC) next to MAME's screenshot of the same moment |
| `viewer_shots.txt` | Example shot list for `hotd_viewer --shots` (same view in both sources, per-model mixing) |
| `make_neutral_icon.py` | Writes `src/res/hotd.ico`, the neutral exe icon (drawn from scratch, no game art) |
| `package_hashes.ps1` | SHA-256 of the release zip and every package file, for the release notes |
| `soak.ps1` | Crash / hang soak: hundreds of headless muted hotd.exe runs across modes and options with timeouts, a summary table (docs/soak.md) |

MAME dump run (headless, no key presses; about 2 minutes):

```bash
.\run_mame_dump.ps1 -Mame "..\private\mame0289" -RomPath "..\private" -Out "..\private\mame_dumps\ch1_run2" -Start 40 -Every 4 -Seconds 360
```

Typical run (quotes needed: the path has spaces):

```bash
py m2_roms.py extract "..\private\hotd.zip" "..\private\arcade_exports\regions"
```

```bash
py m2_models.py scan "..\private\arcade_exports\regions\polygons.bin" "..\private\arcade_exports\models.csv"
```

```bash
py compare_world.py "C:\Games\HOTD\ART\STAGE1\WORLD\WORLD1.RBH" "..\private\arcade_exports\regions\polygons.bin" "..\private\arcade_exports\models.csv" "..\private\compare\ch1"
```

```bash
& "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" -b --factory-startup --python blender_overlay.py -- "..\private\compare\ch1"
```

Rules for every tool:
- Read PC files from the user's install (for example `C:\Games\HOTD`) and never write into it.
- Write anything extracted from the arcade dump only to `private\arcade_exports\`.
- Quote every path: the project path contains spaces.
