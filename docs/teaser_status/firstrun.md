# firstrun: a new player's first start and everyday robustness (2026-10-20)

Branch `claude/firstrun` (worktree wt-firstrun, main merged in). Every run below used its own empty HOTD_PREF_DIR;
packages were made with `make_portable_hotd.ps1 -Out <scratch>`. No window was shown (headless / hidden runs and
`--check-files` only); the message boxes were not seen on screen (see "Not checked by a person").

## Found and fixed

| # | problem | fix |
|---|---|---|
| 1 | **Every start was windowed**: the start-fullscreen toggle was written twice (on, then off again) | one toggle (main fixed it too, 5261616; merged) |
| 2 | **Folders with non-ASCII names failed** (`C:\Users\Jörg\...`, a ROM zip in `Jörg 名前 files`): "not found", because the C runtime / std::filesystem read the program's UTF-8 strings in the ANSI code page | `activeCodePage UTF-8` in src/res/hotd.manifest (Windows 10 1903+): the same zip passes the check, the settings folder may have any name |
| 3 | A wrong folder filled the launcher with up to 185 MISSING lines, so the summary and "Cannot start" were off the screen; long lines were cut at 110 characters | the verdict first (CANNOT START / starts without sound), each check's summary, one `->` line saying what to do, 3 example files and "... and N more (hotd.log)"; status lines wrapped |
| 4 | Common mistakes got no advice | `placementHint` (src/game/files_check.cpp): nothing chosen, path gone, .7z / .rar, CD images (.iso / .bin / .cue ...), not a zip, a zip inside a zip, PC game chosen as the ROMs and the other way round, ROMs of another game, no THOTD.EXE. Also printed by `--check-files` (`arcade hint:` / `PC hint:`) |
| 5 | Empty paths showed as `'' not found` | "not chosen yet" |
| 6 | The first start with nothing chosen showed only the settings list | WELCOME lines: what is needed, where to click, or data\arcade / data\pc |
| 7 | Files placed in data\arcade / data\pc still needed START on the first run (no launcher.ini yet) | found in data = go straight on to the check and the menu; loose ROM files in data\arcade count too (the package's `.txt` notes do not) |
| 8 | No log for support (console only, and the console window stayed open beside the game) | `settings\hotd.log` (every SDL_Log line with its time; the previous run as hotd.prev.log); a console created for the game alone (Explorer, Play.bat, Steam) is closed; a terminal's console stays |
| 9 | A crash closed the game silently (the crash handler existed only for the bot) | src/util/crash_log.cpp: unhandled exception / abort / std::terminate write `crash-<date>-<time>.txt` (exception, stack as module + offset, the last 60 log lines; on its own thread, so a stack overflow too) and, in a player's run, a message box saying where the report is; tests print `CRASH: ...` and exit 3. The build writes `build\Release\hotd.map` (not shipped) to turn offsets into functions. Test: `HOTD_CRASH_TEST=1` |
| 10 | No OpenGL 3.3 / no window / SDL video failure: the window just closed | a message box with what to do (update the driver; remote desktop / some VMs cannot run it) |
| 11 | launcher.ini written in place (a crash or full disk mid-write leaves half a file); a damaged one was read as it was | written to launcher.ini.new, then renamed; a file with NUL / control bytes or no key=value line is kept as launcher.ini.damaged and the defaults are used (tested) |
| 12 | A window could be dragged to almost nothing | minimum 320 x 240 |
| 13 | Package: no zip, no hashes, empty data folders vanish in a zip | make_portable_hotd.ps1 now checks the exe (VERSIONINFO, asInvoker, no build paths, HOTD_LOCAL_ICON=OFF warning, no game data) and writes `hotd-teaser-<date>-win64.zip` + `SHA256SUMS.txt` next to the folder (`-NoZip` skips); a "put ... here.txt" note in data\arcade, data\pc, data\music |
| 14 | README.txt / README.md had no short start | "Quick start" in 5 lines, "Something went wrong?", the new options (online co-op, texture packs, Sonic Mode, CRT, 4P); README.md's stale "Chapter 1 only" fixed |

## Checked, no change needed

- Defaults of a new launcher.ini: fullscreen on, arcade speed (57.524 Hz), smooth motion on, widescreen, crosshair auto,
  volumes 100 %, Sonic Mode off, 1 player, CRT off.
- First-start cases with the package (`--check-files`): nothing; only arcade (PC hint); only PC (arcade hint); arcade
  zip + PC as a zip (both OK, 156 / 156 + 12 tracks, package folder "HOTD ReWrithe" with a space); swapped; .7z / .iso;
  a zip of the ROM zip; a missing path; a non-ASCII folder (after fix 2).
- DPI: SDL 3 declares per-monitor v2 awareness itself. Fullscreen uses the display the window is on (new windows open
  on the primary display).
- Audio device missing / failing: `audio.init` fails -> "no sound: ..." in the log, the game runs silent; SDL 3 follows
  the default device when it changes.
- Pads unplugged: the game pauses with a message until the pad is back (pads.cpp, not changed). Alt+Tab / minimise in
  play: the PC's pause on focus loss (main.cpp), menu music held.

## Not checked by a person (needs the user or a second PC)

- The message boxes (crash, OpenGL) and the launcher's new WELCOME / CANNOT START text on screen (all automated runs
  are hidden; the texts were checked in the log / `--check-files` output).
- A real PC without OpenGL 3.3, a real audio device failure, multi-monitor with different DPI.
- The console closing: Play.bat / Explorer start (a flash of the console window may remain for a moment).

## Tests (final build)

From the 2026-10-20 package: `--check-files` OK; `--input-test` 922 checks 0 failed; `--frames 40000 --autofire
--mute --lives-kept 1` arcade and `--models pc`: "chapter 1 ended" at runtime frame 16695.
