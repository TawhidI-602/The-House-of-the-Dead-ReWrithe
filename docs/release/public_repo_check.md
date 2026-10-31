# Public repository check (2026-10-16, branch claude/release-docs from main 6a3933b)

Report only: nothing outside docs/release and README.md was changed. Method: `git ls-files` (358 files), the full
history file list (`git log --all --name-only`), content greps. Re-run before publishing.

## Result: no blocker found

| Check | Result |
|---|---|
| Anything from `private\` tracked, now or anywhere in history | **None.** `/private/` is in .gitignore; no path starting with `private/` in any commit. |
| Game data: ROM / EXE / DLL / zip / bin / dumps / textures / models / sounds / images (zip exe dll bin rom dmp vmc rbh ogg wav png jpg bmp 7z) | **None** tracked, none in history. Tracked extensions are only source and text (inc, cpp, py, h, md, ps1, lua, txt, java, tsv, bat, ...). |
| Large blobs | Largest tracked file is tools/ghidra/names.tsv (344 KB, text); largest blob in all history is 341 KB. |
| Personal paths / emails | No e-mail address, user name or `C:\Users` path in any tracked file. Two hard-coded **`C:/Games/HOTD`** fallbacks: src/main.cpp:442 and src/viewer/main.cpp:1133 (the author's PC game folder; harmless but personal, worth replacing by "not found" -> launcher; src is other workers' tonight, so not touched). |
| Author identity | Commits carry the git author name "Tawhid" (as allowed). |

## Suspicious files / things to decide before going public

1. **src/port/** (81 files, e.g. enemy_agile.cpp 124 KB, prop_kinds.cpp, enemy_kit_*.inc) and
   **tools/ghidra/names.tsv, types.tsv, tools/state_map.tsv**: a routine-by-routine transcription of Sega's program
   logic and our naming of its functions / addresses (addresses, offsets, constants, comments with arcade and PC
   addresses). Not game *data*, but a derived work of copyrighted code. Legal call for the user (the usual position
   for decompilation projects is publishing under that risk). Not changed.
2. **src/game/pc_manifest.inc** (12 KB): file names, sizes and SHA-1 of the official PC release files; and the
   official-files check's hash tables for the ROMs. Hashes only, no data. Fine; keep.
3. **src/assets/sprite_map_pc.inc, src/game/cue_map.inc, src/proof/cases/*.inc** (37 files), **src/proof/bounds/*.inc**:
   generated tables (sprite id maps, sound code maps, routine addresses and register-argument cases). No ROM bytes
   (spot-checked: addresses and indices only; no file has long byte arrays).
4. **docs/decompilation.md, formats.md, the session history, the working journal** (150 KB, 83 KB, 83 KB, 163 KB):
   internal notes. Contain game structure descriptions, offsets, hashes of reference files and the user's workflow
   notes; one Ghidra-style identifier hit in decompilation.md (no pasted decompiler output found beyond that).
   the working journal is a working journal (rules, "user" remarks) and not meant for readers: consider leaving it
   out of the public repo or moving it to a private branch.
5. **tools/mame_*.lua, run_mame_dump.ps1, run_capture.ps1, tools/ghidra/\*.java**: scripts that read the user's local
   ROMs / MAME. Code only. They point at `private\` folders that do not exist for a reader (document in tools/README.md).
6. **the viewer batch file** (root) builds the older developer viewer (`hotd_viewer`), which needs a MAME snapshot / dumps
   from `private\`: not useful to a player; consider moving it to tools\ or naming it as a developer helper.
7. **No LICENSE file.** Without one the code is all-rights-reserved by default. Choose a licence for our own code; the
   MAME-derived knowledge (ROM layout, BSD-3-Clause) should be credited (ROADMAP mentions it).
8. **No root README** existed; a draft README.md was added in this branch.
9. **.gitattributes** present (fine). `build/`, `build-*/`, `out/`, `.vs/` are ignored.

## Before publishing: commands to re-run

```
git ls-files | grep -i -E "^private/|\.(zip|exe|dll|bin|rom|dmp|vmc|rbh|ogg|wav|png|jpg|bmp|7z)$"   # expect nothing
git grep -n -I -i -E "iqbal|gmail|C:/Users|C:\\Users"                                          # expect nothing
git log --all --name-only --format= | sort -u | grep -i "^private/"                              # expect nothing
git status --ignored                                                                           # private/ only as ignored
```

Also decide whether the full history goes public (it is clean by the checks above, but commit messages and
the session history describe the private workflow) or a squashed first commit is used.

## Resolved (2026-10-16, after the user's go-ahead "most of these you can do yourself")

- **Licence:** MIT for the project's own code ([LICENSE](../../LICENSE)); [NOTICE.md](../../NOTICE.md) states that it
  grants nothing for the original games, credits MAME and the third-party libraries, and covers point 1's derived
  descriptions (the user's risk call stays: publishing the transcription is the usual position for such projects).
- **Journal and history (points 4, 8, "history"):** `tools\export_public.ps1 -Out <folder>` makes the public copy as a
  new repository with one commit (no history), leaving out the working journal, the history notes,
  docs/the session history and the viewer batch file (point 6), and checks the copy (no private\ file, no binary / game-data
  type, nothing over 1 MB, mentions of the left-out files). First run: 410 files, checks clean.
- **Hard-coded `C:/Games/HOTD`** in src/main.cpp and src/viewer/main.cpp: removed (the launcher's PC folder, else the
  developer copy in private\). The tools keep it only as a documented default of developer scripts.
