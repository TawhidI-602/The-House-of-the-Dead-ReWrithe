# The House of the Dead: ReWrithe v1.0.0

> A fan-made rewrite, **not affiliated with, endorsed by or sponsored by SEGA**. "The House of the Dead" is SEGA's
> trademark, used here only to say which game this rewrite works with. **No game data is included in any download.**

## What it is

A C++ rewrite of The House of the Dead (SEGA, arcade 1996 / PC 1998). It runs the whole game, Chapters 1-4 and the
ending, from your own copies of the original games: the arcade game's logic (checked frame by frame against MAME) with
the arcade's or the PC's models, plus the 1998 PC version's own modes. New: widescreen, any monitor's refresh rate,
local co-op up to 4 players, online co-op up to 4 PCs, gamepads with gyro / touchpad aim, light guns, mods, Sonic Mode,
a CRT filter, translations.

## What you need (nothing is included)

Your own, legally obtained copies of:

1. **The arcade ROM set**: a **Revision A** dump only, MAME's `hotd` set (The House of the Dead, Revision A), as a `.zip` or loose files. Other revisions (the original `hotdo`, prototypes) are not supported: the game checks every file against Revision A.
2. **The 1998 PC game**, installed: its folder, or a `.zip` of that folder (sound and the PC models come from it).
   Optional: the PC CD's music as `Track02.ogg` ... `Track12.ogg` (or `.mp3` with the track number at the end).

How to point the game at them (either way):

- put the ROM zip in `data\arcade`, the PC game in `data\pc`, the music in `data\music` of the unzipped folder; the
  game finds them by itself; or
- start the game: the first-start screen (WELCOME) has **[zip]** / **[folder]** buttons next to "Arcade ROMs" and
  "PC game folder"; pick them, then **START**.

The files are checked once against the official releases (CRC-32 / SHA-1); the screen says what is missing or wrong
and where it goes. Your files are only read, never changed.

## Downloads

| File | System |
|---|---|
| `hotd-rewrithe-v1.0.0-win64.zip` | Windows 10 / 11, 64-bit, OpenGL 3.3 |
| `hotd-rewrithe-v1.0.0-linux-x86_64.tar.gz` | Linux x86-64 and SteamOS / Steam Deck (built in the Steam Runtime "sniper") |
| `hotd-rewrithe-v1.0.0-macos-universal.zip` | macOS 11+, Apple Silicon and Intel (**never run by a person: experimental**) |

Windows: unzip anywhere (not into Program Files), double-click `HOTDReWritheSetup.bat` once (it then makes `The House of the Dead ReWrithe.exe` with your PC game's icon:
start the game with that from then on; the first start shows
the setup window, laid out like the 1998 PC CD's Auto Run window: pick your arcade ROMs and PC game, then Play; later
starts go straight to the menu, every setting is in Options). Linux / Deck: unpack, `./hotd.sh` (the
package's README.txt has the Steam Deck steps). macOS: unzip, then see "Unsigned" below, then open `HOTD ReWrithe.app`
(or `hotd.command`) in the unzipped folder.

## Features

- Arcade mode, Chapters 1-4 and the ending, 1-4 players; arcade (1996) or PC (1998) models.
- PC Mode and Boss Mode with the 1998 PC's rules; Chapter Select; High Scores (top 10 with name entry).
- Two menu styles: the 1998 PC's menu and a widescreen "Remake" menu.
- Online co-op up to 4 PCs (LAN or internet, a share code, end-to-end encrypted; Windows and Linux play together).
- Mouse, keyboard, gamepads (DualSense / DualShock gyro and touchpad, Xbox, Switch Pro, Joy-Con, Steam Deck), light guns
  (Sinden, Gun4IR, AimTrak, Wii Remote + DolphinBar), touch screens.
- Widescreen 16:9 / 16:10 or 4:3, fullscreen on any monitor, the arcade's 57.5 Hz or 60 Hz.
- CRT filter, crosshairs (built-in or your own PNGs), blood colour, Sonic Mode (faster play), mods (texture packs,
  custom models, a Ragdoll mod), Add to Steam, a Model Viewer.
- Languages: English, Korean, Japanese, Arabic, Hebrew (see "Translations").

## Tested: by the user and by AI

*Tested by the user* = played by a person on real hardware. *AI-tested* = only automated checks (headless
play-throughs, input tests, network tests on one PC, rendered pictures). The full table is in the README ("Tested").
In short:

- **Played by the user:** Arcade mode start to finish (Surface Pro 7), both menu styles, 4 players locally, the first
  DualSense aim, the Gallery / Model Viewer's first version, 16:10 aspect, the language samples, the Ragdoll mod.
- **AI-tested only:** online co-op (2-4 copies on one PC; never two real PCs over the internet), light guns
  (simulated), other gamepads (virtual pads), PC Mode / Boss Mode (equal to 1998 PC recordings), Linux (WSL only),
  the Steam Deck (pictures at 1280 x 800 only), macOS (CI build only).
- **Every build's automated checks:** the input test (0 failed), Chapter 1 (arcade and PC models) and Chapter 4 to the
  ending headless, the netplay tests.

## Known issues

- **Expect bugs.** This is the first public release: not every mode, chapter and setting was played through by a
  person. Please report anything that looks or plays wrong (see Reporting problems at the end).

- Some routines are still not proven against the originals (marked `UNVERIFIED` in the source); timing, camera and
  small details in Chapters 2-4 can differ.
- PC models: unlit (brighter than the arcade's dark rooms); a Chapter 1 door draws without its room.
- Custom blood colours: the red screen splash stays red.
- Online play over the internet needs a forwarded UDP port or UPnP (Windows only); not tried between two real PCs yet.
- Two physical mice, real light guns, other gamepads, sound cards other than the developer's: not tried by a person.
- Linux: UPnP is not used (forward the port by hand). macOS: never run by a person; no desktop shortcut; may need
  fixes (see docs/release/build_release.md "CI").
- Translations are machine-made (see below).

## Updates

The game checks GitHub for a newer release at start-up (Windows) and offers to open the download page; nothing is
downloaded by itself. Off: Options > System > Check for Updates.

## Translations

The Korean, Japanese, Arabic and Hebrew texts are **AI translations** (machine-made, every line), not checked by native
speakers yet. Corrections are welcome: docs/translating.md.

## Unsigned program (SmartScreen / Gatekeeper)

The program is not code-signed. **Windows** SmartScreen may say "Windows protected your PC": **More info -> Run
anyway**. **macOS**: the app is unsigned (ad-hoc only), so Gatekeeper blocks it; after unzipping, run once in Terminal
`xattr -dr com.apple.quarantine "<the unzipped HOTD ReWrithe macOS folder>"`, or right-click the app -> Open. Compare
the download's SHA-256 with the values below.

| File | SHA-256 |
|---|---|
| hotd-rewrithe-v1.0.0-win64.zip | (fill in) |
| hotd-rewrithe-v1.0.0-linux-x86_64.tar.gz | (fill in) |
| hotd-rewrithe-v1.0.0-macos-universal.zip | (fill in) |

## Health and content

- **Photosensitivity:** flashing lights (gunfire, hit flashes; the CRT-gun Shot Flash is off unless you choose the
  CRT Gun type). A few people may have seizures from flashing lights; stop if you feel unwell.
- **Mature content:** an unrated fan project with horror violence and blood (Blood Color can make it green, blue, purple, yellow or a custom colour; there is no "off").

## Made with AI

Anthropic's Claude assisted with the reverse engineering and with rewriting and combining the existing code; the
project's author (Tawhid) directed and play-tested it. No AI-generated images are included.

## Credits

- The original game: SEGA (AM1), arcade 1996, PC port 1998.
- MAME and its team (the Model 2 knowledge; the SCSP sound core, BSD-3-Clause); Karl Stenerud's Musashi (MIT);
  SDL3 (zlib); stb_vorbis (public domain); dr_mp3 (public domain); Monocypher (CC0).
- Noto Sans KR / JP / Arabic / Hebrew (SIL Open Font License 1.1); Xelu's Free Controller & Key Prompts and Kenney's
  Input Prompts (CC0).
- RUNESA for the secret-codes tip. Everyone who reports bugs.
- Every licence text: NOTICE.txt in the package (NOTICE.md in the repository); a thank-you list: CREDITS.txt.

## No game data

No ROM, executable, music, texture, model, font or logo of the original games is in the repository or in any download.
Everything of the game that you see or hear is read at run time from your own files.

## Reporting problems

Please report bugs: they are expected in a first release. Use this repository's GitHub Issues (the bug
template), or tell the author directly. Attach `settings\hotd.log` and any `settings\crash-<date>.txt`; never attach game
files or ROMs.
