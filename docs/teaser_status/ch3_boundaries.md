Chapter 3 boundary reports after worker "ch3own" (2026-10-19; the default route, the chapter ends at frame 27279; the
earlier report of 73 entries is in the git history). Nothing unported, no stub, no guard is left.

### hotd --mute --autofire --lives-kept 1 --chapter 3 --frames 90000 --default-route

27280 frames, 2 entries.

| # | routine | address | kind | calls | per 1,000 frames | first frame |
|---|---|---|---|---|---|---|
| 1 | player_input_update (the mouse as the light gun) | 0x5DF0 | stand-in | 27279 | 1000.0 | 0 |
| 2 | player out of lives (kept at 1: --lives-kept) | - | partial | 42 | 1.5 | 2696 |

### hotd --mute --autofire --lives-kept 1 --chapter 3 --frames 90000 --default-route --models pc

27280 frames, 2 entries.

| # | routine | address | kind | calls | per 1,000 frames | first frame |
|---|---|---|---|---|---|---|
| 1 | player_input_update (the mouse as the light gun) | 0x5DF0 | stand-in | 27279 | 1000.0 | 0 |
| 2 | player out of lives (kept at 1: --lives-kept) | - | partial | 42 | 1.5 | 2696 |
