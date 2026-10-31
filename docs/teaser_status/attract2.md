# attract2: the prologue's timing and the sewer zombie (2026-10-17, branch claude/attract2)

Build folder `build-attract2`. Compared: the user's play-test video (2026-10-16 23-37-39.mp4, 0:15-1:30), MAME
(`tools\mame_flow_trace.lua`: mode / sub-mode / scene_number / event_index / evt_pc every frame, snapshots;
capture private\captures\attract2_flow) and ours headless (`HOTD_TRACE_FLOW=<file>`, the same line format).

## 1. "G held too long" (fixed, b50e592)

The event script runs in step with MAME (every scene and evt_pc change of mode 9 within a few frames; MAME's few
extra frames are lag frames). The extra time was a freeze: the prologue's last scene (kind108_zone_set slot 3)
loaded Chapter 4's world on its first frame, 1.1-1.3 s (arcade: markCoplanarLayers of every model; PC models: the PC
stage), so the picture stopped at the end of scene 9 (G in the dark suit) - the video shows 1.13 s without a new
frame at 0:52.2. Now GameView::prefetchExtra loads the arcade stage on the prologue's first frame (~80 ms, black)
and the slow rest on a worker thread; that frame went 1285 ms -> 26 ms (headless --render, arcade and PC models).

## 2. The sewer zombie (scene 6, open)

Spawns (event pc 0xFDC78..0xFDC80, ENTB3): kind 99 records 0xFE370 **twice** and 0xFE3D0; the ROM really lists
0xFE370 twice (read in MAME), 0xFE3A0 (a third record, type 24 at x -71 z 127) is referenced by nothing there. So
our spawns match the arcade's; the two type-24 zombies (action 1, motion 0x289) overlap in both.
The visible difference: MAME snap at 31.0 s (scene start + 1.9 s) shows the brown zombie's leg in the right
foreground; ours (video 0:39.15-0:39.4) no longer draws it there. Lead: a cull of that zombie near the camera / the
screen edge (enemy_frame_end's on-screen test or the view), not a spawn. **Resolved (sewer.md): drawn as in MAME; MAME snap 6 = our frame 1607, the earlier comparison was a few frames off.**

The white fade of kind 104 covers only a part of the screen in widescreen in the video (0:43, 0:53): the HUD's
white quad, not checked.
