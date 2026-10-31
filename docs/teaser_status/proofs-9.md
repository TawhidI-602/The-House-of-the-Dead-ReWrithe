# Proofs worker 9 (weekend round, 2026-10-16 evening)

Branch `claude/proofs-9`, build folder `build-proofs9` (configured with `-DHOTD_FETCH_SDL3=ON
-DFETCHCONTENT_SOURCE_DIR_SDL3=<main checkout>/build/_deps/sdl3-src`: no download). Results and command lines in
[proof.md](../proof.md) ("Player join (proofs-9)").

## player_st3_join (proofs-8's 1 differing capture)

Not a port bug: the capture's size. The arcade index gives 0x4FF0 12 bytes (Ghidra split the routine at 0x5008,
`player_body_reinit`, which is its debug-mode tail), so capture_lists.py's next-index-start size is 0x18 and holds no
ret: mame_capture.lua kept capturing past the real ret (0x50A0) into the next game frame (game_frame's 0x7D focal-length
call = the "0x7D call" proofs-8 could not find). Fix: `4ff0:b4` in -Funcs **and** in -Ported (else play_time_start's
call site at 0x5094, inside 0x5008's index range, is not a boundary and the capture misses its record).
p9_join2 (2 players, route 1:4 / 1:7, 1100 s): 1 capture, 0 differ. UNVERIFIED 65 -> 64.
Not reached: the debug-mode body rebuild (0x51F42C != 0).

## player_camdr_mode4 / player_camdr2_mode4

PlStartInit row 3's camera-drive set-ups (0x61F0, 0x4B7F0): not captured as -Funcs inside 4FF0's capture (nested),
so a separate run p9_camdr (`-Funcs 61f0,4b7f0`, the same 2-player game): 1 capture each, 0 differ. UNVERIFIED 64 -> 62.

Captures kept (compressed): private\proof\p9_join2, private\proof\p9_camdr (about 2.3 GB each before compression);
the exploratory p9_join (4ff0:b4 in -Funcs only: play_time_start's record missing) deleted.
