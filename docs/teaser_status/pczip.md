# pczip: the PC files may stay zipped (user, 2026-10-17)

- `src/assets/pc_files.*` (`hotd::pcfs`): read-only layer over our zip reader. A path is a real file or
  `<x.zip>/<inside>`; names match any case / separator; the install may be one folder down (root found by `THOTD.EXE`
  or `ART/STAGE1/`). Stored entries stream in place (64 KB blocks, seekable `std::istream`), deflated ones inflate on
  demand (the last 3 / 160 MB kept: BIGGER.VMC is read by textures and sprites). Music in a zip: read into memory,
  `stb_vorbis_open_memory`.
- Users: pc_loader (RBH, VMC pages, TBL / PAL, TEXTURE.VMC size), sprites_pc, audio (cue table, bank, O_ files,
  music), files check (`checkPc`: a zip's entries by path relative to the install, else size + SHA-1; remembered by
  the zip's size + date + the entry's CRC), window icon (`peicon::largestImage(bytes)`). Launcher: PC `[folder] [zip]`.
  Play.bat (tools\make_portable_hotd.ps1): an unpacked install first, else `data\pc\*.zip`. RAR / 7z: a message.
- Not covered: the desktop shortcut's icon with a zipped PC game (the shortcut cannot point into a zip: the program's
  own icon); hotd_viewer's EVT / CAM readers (developer tool) still need a folder.

Tests (2026-10-17, build-pczip): the user's `data\pc\hotd pc.zip` (deflated, top folder `hotd pc/`, no music):
`--check-files` 156 / 156, headless `--models pc --render` Chapter 1 clear at frame 16695 (score 8280), silent music,
`--input-test` 518 / 0 failed, window icon found. Scratch zips of private\pcharness\game (THOTD.EXE.orig as THOTD.EXE):
deflated (.NET, `HOTD DUMP/`) and stored (bsdtar, `hotd pc/`): check 156 / 156 with 12 tracks; the same clear; the
`--render` shots at 3000 / 9000 and the `--audio-wav` mix (music included) byte-identical to folder mode. Folder mode:
check, clear, shots, wav, input test unchanged. Test zips deleted.
