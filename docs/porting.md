# Porting: Linux, Steam Deck (SteamOS), macOS

Workers "linuxport" and "linux2", 2026-10-20. Goal: Linux and the Steam Deck (important), macOS (later), mobile maybe
one day. Windows behaviour is unchanged (all checks give the same numbers).

## Where it stands

- **The game runs on Linux** (worker "linux2", WSL Ubuntu 26.04, GCC 15.2, cmake 4.2.3 + Ninja, the system's SDL
  3.4.2): `hotd`, `hotd_viewer`, `hotd_nettest`, `hotd_proof` build and run. Headless on Linux, the same numbers as
  Windows:
  | check | Linux | Windows |
  |---|---|---|
  | `--input-test` | 1577 checks, 0 failed | 1577 checks, 0 failed |
  | Chapter 1, `--frames 40000 --autofire --mute --lives-kept 1` (arcade / `--models pc`) | ended at 16695 / 16695 | 16695 / 16695 |
  | Chapter 4, `--chapter 4 --strict --autofire --lives-kept 1 --mute --frames 60000 --default-route` (both models) | the ending at 34967 / 34967 | 34967 / 34967 |
  | `hotd_nettest` | all passed | all passed |
  | `hotd_proof pmmaths` | 0 differ (102 routines, 2,065,600 cases) | 0 differ |
  | `--render --shot-at 2500` (WSLg's OpenGL) | the picture, 0 GL errors, ~10 ms a frame | |
- **Online play on every system with one protocol (cross-play)**: protocol 10 uses Monocypher on Windows, Linux and
  macOS (section "Crypto"). A Linux host and a Windows joiner played Chapter 1 + 2 (40,000 frames, 2 players): both
  `result=ok`, 40,000 hashes compared, the same final hash as two Windows PCs.
- **A Linux package** (`tools/make_linux_package.sh`, section "SteamOS package"): `bin/hotd` with its own
  `libSDL3.so.0` (rpath `$ORIGIN`), `hotd.sh`. Checked: the packaged game loads the bundled SDL and plays Chapter 1
  to 16695.
- Earlier (worker "linuxport"): everything compiles with `-Wall -Wextra`; `hotd_proof` on the 4 MAME captures is
  identical on Linux and Windows; the C library maths the game uses are bit-identical (section "Cross-play").
- **Not tested yet:** a real Steam Deck / desktop Linux PC (the window, pads, gyro, light guns, sound devices), a
  build in the Steam Runtime (the WSL build needs glibc 2.43), UPnP on Linux, macOS.

## Inventory of the Windows-only parts

| File | What is Windows-only | Portable replacement | State | Effort left |
|---|---|---|---|---|
| `src/net/socket.cpp` | Winsock UDP | BSD sockets behind the same class | **done before** (both in the file) | none |
| `src/net/upnp.cpp` | UPnP port mapping through Windows' COM (`NATUPnP`) | our own SSDP + SOAP client (UDP multicast + a small HTTP POST), no library | Linux / macOS: "forward the port yourself" message | medium (1 session) |
| `src/net/crypto_win.cpp` (gone) | CNG: ECDH P-256, CPace, HMAC-SHA256, AES-256-GCM, SHA-256, random | Monocypher on every system (section "Crypto") | **done (linux2)**: `crypto.cpp`, protocol 10, cross-play checked | none |
| `src/util/crash_log.cpp` | SEH filter + DbgHelp stack walk, `MessageBox`, `_wfopen` | **POSIX signal handler on its own stack** (SIGSEGV / SIGBUS / SIGILL / SIGFPE), `backtrace` + `dladdr` (module + offset), SDL's message box | **done (this branch)** | none |
| `src/game/bot.cpp` | the robustness runs' SEH crash line | the same `ROBUST: result=crash` line from a signal handler | **done (this branch)** | none |
| `src/util/win_shell.cpp` | desktop / Start menu `.lnk` (COM `IShellLink`), `GetModuleFileName` | **Linux: a `.desktop` file** on the Desktop and in `~/.local/share/applications`, icon = your THOTD.EXE's icon as a PNG in `~/.local/share/icons`; **own path: `/proc/self/exe` (Linux), `_NSGetExecutablePath` (macOS)** | **done (this branch)**; macOS shortcut: says "not supported" | macOS: small |
| `src/util/steam_shortcut.cpp` | Steam folder from the registry; "is Steam running" via Toolhelp | **Linux: `~/.steam/steam`, `~/.local/share/Steam`, Flatpak, Snap; macOS: `~/Library/Application Support/Steam`; running: `/proc/*/comm` = `steam` (Linux), `libproc` `steam_osx` (macOS)**. The `shortcuts.vdf` / grid code was already portable | **done (this branch)** | none |
| `src/main.cpp` | raw input (`WM_INPUT`) to tell mice / light guns apart; process memory report | SDL3 already tells mice apart on Linux (X11 XInput2 / Wayland): the events decide; memory report says "unknown" | compiles; mice / guns untested on Linux | test on real hardware |
| `src/game/lightguns.cpp` | light guns through raw input (absolute mice) | Linux: guns are absolute evdev devices; SDL3 shows them as mice with absolute motion. Needs testing with a real gun (Sinden / GUN4IR) | Windows-only path kept | medium |
| `src/assets/archive.cpp`, `pc_files.cpp`, `game/files_check.cpp`, `proof/main.cpp` | `_fseeki64` / `_ftelli64` (MSVC) | `util/file64.h`: `fseeko` / `ftello` elsewhere | **done (this branch)** | none |
| `src/assets/pc_files.cpp` | file names match whatever their case on Windows | **Linux: a missing PC file path is looked up a part at a time ignoring case** (`ART/STAGE1/...` finds `art/stage1/...`) | **done (this branch)**, tested in WSL | none |
| `src/proof/main.cpp` | memory-mapped captures (`CreateFileMapping`) | `mmap` | **done (this branch)** | none |
| `src/main.cpp`, `src/viewer/main.cpp` | `localtime_s` | `localtime_r` | **done** | none |
| `src/game/audio.cpp` | (all builds) a build without the arcade sound board did not compile (`unique_ptr` of an incomplete type) | an empty stand-in class | **done** | none |
| `src/game/pads.cpp` | `std::max(uint64_t, unsigned long long)` (the same type on Windows only) | `std::max<uint64_t>` | **done** | none |
| `src/res/*.rc`, `hotd.ico`, manifest | the exe icon, version info | Linux: the `.desktop` file's icon; macOS: an `.app` bundle's `.icns` | CMake already adds the `.rc` on Windows only | macOS: small |
| `CMakeLists.txt` | `ws2_32 bcrypt ole32 shell32 uuid`, `/MAP`, the `.rc`, SDL3.dll copy | already inside `if(WIN32)` / `if(MSVC)`; **added: Threads, `${CMAKE_DL_LIBS}`, rpath `$ORIGIN` (`@loader_path` on macOS), `-Wno-unused-parameter`** | **done** | none |
| settings / cache folder | `SDL_GetPrefPath` | SDL already uses `~/.local/share/hotd-rewrite/hotd/` on Linux (XDG_DATA_HOME) and `~/Library/Application Support/hotd-rewrite/hotd/` on macOS; a portable package keeps them in its own `settings/` (unchanged) | nothing to do | none |
| file dialogs (setup "Folder... / Zip...") | `SDL_ShowOpenFolderDialog` | works on Linux desktops through xdg-desktop-portal or zenity; **not in the Steam Deck's Game Mode**: there the files go in the package's `data/` folders (the game finds them itself) | nothing to do | none |
| `tools/*.ps1`, `*.bat` (HOTDReWritheSetup.bat, `make_portable*.ps1`, `netplay*_test.ps1`, `package_hashes.ps1`, ...) | PowerShell | the developer tools stay Windows-only; a Linux package needs a `make_linux_package.sh` and `hotd.sh` launch script (section "SteamOS package") | **`tools/make_linux_package.sh` done (linux2)**; the netplay test scripts stay PowerShell | none |
| `HOTDLauncher.exe` / HOTDReWritheSetup.bat busy checks (Get-Process) | PowerShell | the game itself needs none on Linux | n/a | none |

## Cross-play: the game logic gives the same results on Linux

Online play is lockstep: both PCs must compute the same game. Checked on this branch:

- `hotd_proof` on 4 MAME captures: Linux (GCC 15, `-ffp-contract=off -fno-fast-math`) and Windows (MSVC, `/fp:precise`)
  print identical results (0 differ on both).
- The only C library maths the game logic uses: the sine / cosine and arctangent tables (`port/pm_maths.h`) and the
  dogs' `log` (`port/enemy_dogs.cpp`). A test program hashed the tables and `log` for **all 222,313,874 floats from
  0.001 to 100,000** (both forms the dogs use): MSVC's UCRT and Linux's glibc give **bit-identical** results.
- **Checked (linux2):** `hotd` on Linux ends Chapter 1 at 16695 (both models) and Chapter 4's run at 34967, the
  Windows numbers; a Linux host + Windows joiner game (40,000 frames, 2 players) never desynced and ended with the same
  hash as a Windows + Windows game (3ddee74c4007301f).
- **Cross-play over WSL, which direction:** the Linux host / Windows joiner direction was run. The other one (a
  Windows host, a WSL joiner) was not: WSL 2's NAT puts Linux behind a virtual switch, so the joiner's packets arrive at
  the Windows host from outside, and Windows Firewall has no rule for the worktree's new hotd.exe: binding a
  non-loopback UDP port would pop up the "allow access" dialog on the desktop (NotifyOnListen is on), which a test must
  not do, and adding a rule is a security setting. The handshake and the packets are the same in both directions (the
  host / joiner roles only swap who sends HELLO), so this direction adds little; on two real PCs both work alike.

## Crypto (online play)

**Done (linux2, protocol 10): Monocypher 4.0.3 on every system, Windows too** (`src/net/crypto.cpp`,
`src/third_party/monocypher/`). The design, the primitives and the tests: docs/netplay.md "Security". The CNG code
(`crypto_win.cpp`, `p256.cpp`) and the stand-in that refused online play (`crypto_none.cpp`) are gone.

The choice that was made (linuxport's comparison):

| Option | Primitives | Licence | Notes |
|---|---|---|---|
| **Monocypher 4.0.2** (recommended) | X25519, Elligator 2 (hash to curve: exactly what CPace on X25519 needs), XChaCha20-Poly1305 AEAD, BLAKE2b, SHA-512 / HMAC-SHA-512 | CC0 or BSD-2-Clause (your choice) | 2 files (`monocypher.c/.h`, about 3,000 lines), audited by Cure53 (2020), used widely; can be committed in `src/third_party/monocypher/` |
| libsodium 1.0.20 | X25519, XChaCha20-Poly1305, BLAKE2b, SHA-512 (+ AES-GCM only on CPUs with AES-NI) | ISC | bigger (a library to build per system), also well audited |
| keep P-256 + AES-GCM | needs a portable P-256 + AES-GCM library (e.g. BearSSL, MIT; Mbed TLS, Apache-2.0) | | bigger, more to get wrong; no reason to keep P-256 |

As built (all systems, Windows too): protocol 10; CPace-X25519 (generator = Elligator 2 map of BLAKE2b of the
password and the protocol's label; no session id: the nonces in the transcript make every session's keys fresh, and
the host makes a new key pair for every HELLO); keys = BLAKE2b-512 over a label, the shared secret and the transcript,
one half per direction; every packet XChaCha20-Poly1305 with the header as associated data and the same 64-bit
sequence numbers and replay window as before. Random bytes from the OS (Windows `BCryptGenRandom`, Linux `getrandom`,
macOS `arc4random_buf`). Passed: `hotd_nettest` (Windows and Linux, with the known-answer tests and the fuzz),
`netplay_test.ps1 -HostArgs "--players 2"`, `netplay4_test.ps1`, and a Linux <-> Windows game.

## Steam Deck (SteamOS)

- **Screen 1280 x 800 (16:10):** rendered on Windows at 1280 x 800: the main menu, Options, all six settings tabs, a
  Controller page, High Scores, Chapter Select, Co-op, and the HUD in a 2-player game (frames 2500 / 6000 / 8800).
  Everything fits: the menus are 4:3 with side bars, the 3D view simply shows a little more at the top and bottom,
  the HUD (bullets, lives, 1P / 2P) sits in the corners as at 16:9. Nothing needed fixing.
- **Controls:** the Deck's controls reach the game as an SDL gamepad (through Steam Input: an Xbox-style pad). The
  existing pad support applies (stick aim, fine aim, trigger = R2). Gyro: Steam Input passes the Deck's gyro to games
  as a mouse or a pad's motion sensor, depending on the controller layout; the game's gyro aim needs the motion
  sensor (Steam's "Gamepad with gyro" layout) - to test on a Deck. The trackpads work as a mouse.
- **Touch screen as a light gun:** already works through SDL's touch-to-mouse events (a tap = a shot where you touch;
  the code treats touches as absolute pointer positions). Reloading needs a shot off the screen, which a touch can't
  do: turn on Options -> Auto Reload (or use a back button mapped to the right mouse button in Steam Input).
- **Suspend / resume:** the game loop drops a long stall instead of speeding up afterwards (no fast-forward after
  waking). A netplay game will time out during a suspend (as with a network loss). Idea for later: on a long stall,
  clamp the pad aim's time step (today a stick held while suspending moves the aim once by stick speed x the gap;
  harmless, it is clamped to the screen).
- **Files:** in Game Mode there is no file dialog. A package finds the files in its `data/arcade` and `data/pc`
  folders by itself (as on Windows); PC file names in any case work (`art/stage1/...` too).
- **Add to Steam:** works with the Linux Steam folders, but Steam must not be running (it rewrites the list when it
  exits): on a Deck, switch to Desktop Mode, exit Steam, press Add to Steam, start Game Mode again. Adding it by hand
  in Desktop Mode (Steam -> Add a Non-Steam Game) works too.
- **Fullscreen:** the game starts borderless fullscreen (Gamescope gives it the whole screen).

### SteamOS package (`tools/make_linux_package.sh`, linux2)

```
HOTD ReWrithe Linux/
  hotd.sh, hotd_viewer.sh launch scripts: cd "$(dirname "$0")"; exec ./bin/hotd "$@"
  hotd_package.txt        (as the Windows package: the game keeps settings/ and finds data/ itself)
  bin/hotd, bin/hotd_viewer   built with rpath $ORIGIN (stripped)
  bin/libSDL3.so.0        the SDL3 the build linked (copied from where the loader finds it)
  data/arcade  data/pc  data/music  data/mods  data/steam/<kinds>   (empty, a note in each: the player's own files)
  README.txt (the Steam Deck steps), LICENSE.txt, NOTICE.txt, ONLINE HOWTO.txt
```

`tools/make_linux_package.sh [-b build-linux] [-o <folder>] [--no-build] [--no-tar]`: default output
`<main checkout>/private/portable/HOTD ReWrithe Linux` plus `hotd-teaser-<date>-linux-x86_64.tar.gz` and
`SHA256SUMS-linux.txt` beside it. Checks: no game data, the packaged `bin/hotd` loads the bundled `libSDL3.so.0`
(not the system's), no build paths in the binaries; prints the newest glibc the binaries need. Tested in WSL: the
package's `hotd.sh` loaded its own SDL and played Chapter 1 to 16695.

- The WSL build uses Ubuntu 26.04's SDL 3.4.2 and needs **glibc 2.43**: fine for a current desktop distribution, too
  new for SteamOS 3 (an older glibc) and older distributions. A release for the Deck must be built in the
  **Steam Runtime "sniper" SDK** (Debian 11, glibc 2.31) with SDL 3.2.10 built from source (needs that download:
  "What needs your approval").
- SDL's own Linux backends (X11, Wayland, PipeWire / PulseAudio / ALSA, evdev, HIDAPI for pads) load at run time;
  the bundled SDL avoids SteamOS's varying system version.
- OpenGL 3.3: SteamOS's Mesa (RADV / radeonsi) supports it.

## macOS (supplementary)

- Compiles in principle (the POSIX paths are shared with Linux; `_NSGetExecutablePath`, `libproc` for Steam, rpath
  `@loader_path`; random bytes from `arc4random_buf`); not compiled yet (needs a Mac or a cross toolchain).
- OpenGL on macOS stops at 4.1 core (deprecated but working): our renderer needs 3.3 core: fine.
- Missing: an `.app` bundle (Info.plist, `.icns`), code signing / notarisation for other people's Macs, the desktop
  shortcut (says "not supported").

## What needs your approval (downloads / installs)

Done (approved 2026-10-20): WSL got `build-essential cmake ninja-build libsdl3-dev` from Ubuntu's archive; Monocypher
4.0.3 was downloaded from monocypher.org (SHA-512 and BLAKE2b checked against the site) and is committed in
`src/third_party/monocypher/`. Still open:

1. **For a SteamOS release package:** the Steam Runtime sniper SDK (a container image from Valve,
   `registry.gitlab.steamos.cloud`) to build a binary that runs on every SteamOS, and SDL 3.2.10's build dependencies
   inside it.
2. A Mac (or a macOS cross toolchain) for the macOS build.
3. Optional, for the Windows-host direction of the WSL cross-play test: a Windows Firewall rule for the test exe
   (or WSL's mirrored networking). Two real PCs need neither.

## Linux build

```
D=<main checkout>/private/deps
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DHOTD_ARCADE_SOUND=ON \
      -DHOTD_DR_LIBS_DIR=$D/dr_libs -DHOTD_MUSASHI_DIR=$D/musashi -DHOTD_SCSP_DIR=$D/mame_scsp -DHOTD_STB_DIR=$D/stb
cmake --build build-linux
HOTD_PREF_DIR=<a test folder> ./build-linux/hotd --root <checkout> --arcade <hotd.zip> --pc <PC folder> \
      --frames 40000 --autofire --mute --lives-kept 1        # says: [runtime 16695] chapter 1 ended
./build-linux/hotd_nettest                                   # ALL PASSED (0 failed)
```

Headless runs need no display; `--render` and window runs in WSL use WSLg (Windows 11). A fresh `HOTD_PREF_DIR`
needs `--root`, `--arcade` and `--pc` (else the input test waits in the launcher). Cross-play test: host in WSL with
`--host <port> --code X --players 2`, join from Windows with `--join <WSL's eth0 address>:<port> --code X`.
