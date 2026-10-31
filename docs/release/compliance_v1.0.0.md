# Release compliance review, v1.0.0 (relprep, 2026-10-27)

**This is a checklist, not legal advice.** It was written by an AI worker from the repository and the test package. For
anything marked "needs you" or anything you are unsure about (above all trademark and the reverse-engineered code),
ask a lawyer who knows games / IP law where you live before publishing.

Status words: **OK** (checked, nothing to do), **fixed** (changed in this branch), **needs you** (a decision or an
action only you can take).

## 1. Legal / intellectual property

| Item | Status | Notes |
|---|---|---|
| No SEGA ROMs, PC game files, music, textures, models, fonts or logos in the repository | **OK** | `git ls-files` (1,433 files) and the whole history: no zip / bin / rom / ogg / mp3 / wav / rbh / vmc / exe / dll / jpg / bmp; the only images are the CC0 prompt icons and our wordmark; no long byte arrays in the sources; nothing from `private\`. |
| No game data in the release package | **OK** | The package (761 files, 21.8 MB, zip 13.5 MB): our exes, SDL3.dll, the MSVC runtime, data\ui (prompt icons + licences, wordmark), data\fonts (Noto + OFL), data\lang, data\mods (two mod.ini files), text files. `make_portable_hotd.ps1` refuses game-data file types. |
| The menu wordmark (`data/ui/rewrithe_wordmark.png`) | **OK** | Our own: the word "ReWrithe" set in the free Special Elite font (Apache 2.0; the font itself is not shipped). Not taken from the game. |
| SEGA logos / title art | **OK** | None shipped. The title screen, menu art and icons on screen are read at run time from the player's own files; the exes carry our neutral icon (`-DHOTD_LOCAL_ICON=OFF` for every release build). |
| The game uses only the player's own files | **OK** | It reads the ROM zip / PC folder the player chooses, checks them against the official releases, never changes them, never downloads anything. |
| Trademark: "The House of the Dead" only descriptively | **fixed** | README now opens with a boxed disclaimer ("a fan-made rewrite, not affiliated with, endorsed by or sponsored by SEGA ... used only to say which game this rewrite works with"); the same at the top of the release notes; NOTICE.md had it already. The program's name "The House of the Dead: ReWrithe" itself contains SEGA's trademark: **needs you** (see below). |
| Reverse-engineered game logic in `src/port/` and `tools/ghidra/` | **needs you** | A routine-by-routine rewrite of SEGA's program logic (with addresses and names). Not game data, but derived from copyrighted code. Fan projects publish this under some risk (cf. decompilation projects); the law differs by country. Ask a lawyer if unsure. |
| Our own code's licence | **OK** | MIT (`LICENSE`), with NOTICE.md saying it covers only our code and grants nothing in the original games. Consistent with every third-party licence below (all permissive). |

**Needs you, trademark in the name:** using "The House of the Dead" in the project's name is the most visible
trademark use. It is common for fan projects, but a shorter name ("HOTD ReWrithe") or "ReWrithe (a rewrite of The House
of the Dead)" lowers the risk. Your call.

## 2. Licences of third-party parts

| Part | Licence | Status |
|---|---|---|
| SDL3 3.2.10 (shipped as SDL3.dll / libSDL3) | zlib | **fixed**: full licence text added to NOTICE.md (the package's NOTICE.txt) |
| stb_vorbis (compiled in) | public domain / MIT | **OK** (public domain chosen, credited) |
| dr_mp3 (compiled in) | public domain / MIT-0 | **OK** (credited) |
| Monocypher 4.0.3 (compiled in) | CC0 / BSD-2-Clause | **OK** (CC0 chosen, LICENCE.md in the repo) |
| Musashi 68000 core (compiled in) | MIT | **fixed**: MIT requires its notice in copies: full text added to NOTICE.md. Its m68kfpu.c / m68kmmu.h (other licences) are not used (our stand-ins); the CI workflow deletes them after fetching. |
| MAME SCSP sound core (compiled in) | BSD-3-Clause | **fixed**: BSD-3 requires the notice in binary distributions: full text added to NOTICE.md |
| Noto Sans KR / JP / Arabic / Hebrew | SIL OFL 1.1 | **fixed**: `data/fonts/OFL.txt` now starts with each font's copyright line (OFL condition 2); Hebrew is "2024", corrected in NOTICE.md. Reserved Font Names: the fonts are shipped **unmodified under their own names**, which the OFL allows (the RFN rule only limits renamed / modified versions). |
| Xelu's prompts, Kenney's prompts | CC0 | **OK** (licences in data/ui/prompts, credited in README / NOTICE) |
| Visual C++ runtime DLLs (Windows package) | Microsoft redistributable | **OK** (Visual Studio allows shipping these files with a program built with it; noted in NOTICE.md) |
| Console / platform names and button icons (PlayStation, Xbox, Nintendo Switch, Steam Deck ...) | trademarks of their owners | **OK, with a caveat**: names are used only to say which controller a setting is for; the icons are the generic CC0 packs (button shapes and letters; not checked icon by icon for brand logos, e.g. a PlayStation / Xbox home-button logo). Trademark rights are not covered by CC0; descriptive use like this is normal. |
| MAME (the Model 2 hardware knowledge) | BSD-3-Clause | **OK** (credited in NOTICE.md; no MAME code except the SCSP core above) |

## 3. Safety

| Item | Status | Notes |
|---|---|---|
| Photosensitivity | **fixed** | Shot Flash (whole screen white on each shot) is "Auto" = on only for the CRT Gun type, off otherwise; the README already warned at that setting. Added: a health warning in the README's top box, the release notes, and at the top of the package's README.txt. (2026-10-31: the line on the first-start setup window was removed at the user's request when the setup became the small Auto Run-style window.) |
| Online play: encryption, no accounts | **OK** | CPace password exchange + XChaCha20-Poly1305 per packet (Monocypher); no accounts, no servers of ours. |
| Online play: the host's IP address | **fixed** | ONLINE HOWTO now says the share code holds the host's IP address and every player sees the others' addresses (direct connection, no server). |
| No telemetry / phone-home | **OK** | No HTTP / analytics code: no URL is opened by the program (`https://` appears only in comments); the only network use is the online co-op UDP and UPnP on the local router (Windows), both only when the player hosts / joins. |
| Writes only where expected | **OK** | The settings folder (the package's `settings\`, else `%APPDATA%\hotd-rewrite\hotd\`): launcher.ini, the checked-files cache, logs, crash reports, scores; screenshots in the package's `screenshots\`. Only when the player asks: a desktop / Start-menu shortcut, Steam's shortcut list (after a backup). Never the game files. |
| Administrator rights | **OK** | `asInvoker` manifest: never asks for admin. |
| Unsigned program | **OK, noted** | Release notes and README explain SmartScreen ("More info -> Run anyway") and macOS Gatekeeper, and give SHA-256 sums. **Needs you**: fill the sums in from the CI artifacts. |

## 4. Privacy

| Item | Status | Notes |
|---|---|---|
| Personal data | **OK** | Nothing is collected or sent. launcher.ini holds the settings, the paths of your game files and your online player name (only sent to the PCs you play with). |
| Logs, crash reports, screenshots | **OK** | Stay on the PC; the bug template asks for the log and crash report and says never to attach game files. The log can contain your folder paths (e.g. your Windows user name): **needs you**: consider one line in the bug template, "check the log for anything private before attaching". |

## 5. Ethics and content

| Item | Status | Notes |
|---|---|---|
| AI-assisted development disclosed | **fixed** | README box and release notes: code and documents by AI (Claude) with the author; translations machine-made (each language file says so in its header); no AI-generated images shipped. |
| Mature content | **fixed** | README / release notes: unrated fan project, horror violence and blood, for adults. Blood Color offers green, blue, purple, yellow or custom; there is **no "off"** setting (the request mentioned one; it does not exist). |

## 6. Mods

| Item | Status | Notes |
|---|---|---|
| Mods run no code | **OK** | A mod is data only: mod.ini (names, colour rules, settings), PNG textures, OBJ / MTL models. No scripts, no DLLs, nothing executed. |
| Path traversal | **fixed** | Mod folders are found by listing `data\mods`, and textures / models by listing their own folders, so those could not escape. But an OBJ's `mtllib` and an MTL's `map_Kd` named any path (`..\..\` or `C:\...`): now refused unless it is a plain relative path inside the mod's folder (`src/assets/mods.cpp` insideDir). The worst case before was reading another image file as a texture, not running anything. |

## 7. Security of the file and packet parsers

| Item | Status | Notes |
|---|---|---|
| Untrusted network packets | **OK** | `hotd_nettest` fuzzes the settings decoder with random bytes and feeds 20,000 malformed / replayed / flipped packets into a running session; ALL PASSED on this branch (0 failed). |
| PNG reader | **OK** | Chunk lengths checked against the file size, 4096 x 4096 limit, no interlace, output buffer sized from the header. |
| OBJ reader | **OK** | Face indices checked against the vertex count. |
| ini / lang files | **OK** | Line-based `key = value` parsing with std::string, numbers clamped to their ranges. |

## Summary: what needs you

1. Decide whether to keep "The House of the Dead" in the program's name (trademark risk is lowest without it).
2. Accept (or get advice on) publishing the reverse-engineered logic in `src/port/`.
3. Fill in the SHA-256 sums in docs/release/notes_v1.0.0.md from the CI artifacts.
4. Optional: one line in the bug template about checking logs for private folder names.
