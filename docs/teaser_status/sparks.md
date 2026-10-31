# sparks: polish #7 (hit sparks) and #4 (researchers in the results) (2026-10-16)

Branch `claude/sparks`, build folder `build-sparks`. Not merged. UNVERIFIED (teaser): judged by headless `--render`
shots and one checkpoint shot against MAME fullgame_us snaps.

1. **#7 hit sparks** (sparks 1): the large white / orange star MAME shows at almost every shot is not a hit spark but
   the shot's impact flash, shot_effect_flash (0x95140, from shot_effect 0x95270 on every shot): a camera-facing star
   at depth 1, scale 0.1. It was spawned and drawn every shot (traced), but turned toward its off-axis point half of
   it lies nearer than depth 1 and the renderer's near plane (1.0) cut it to a sliver. The first fix (near 0.25) was
   held by the coordinator (depth precision) and is reverted (sparks 3). Now: **GL depth clamp** (no near clip; near
   stays 1, far 20000, 24-bit depth in both the window and the offscreen buffers), as the Model 2 does: MAME
   model2_v.cpp clips only to the four side planes through the eye, culls polygons wholly behind it (max z < 0) and
   far ones by master_z_clip; there is no near value. `HOTD_NO_DEPTH_CLAMP=1` switches it off.
   Blood-face check (render's method, frames 930-937, `--autofire --default-route`, clamp on / off): PC models
   pixel-identical on / off (the PC shot flash is not ported); arcade frames differ (the star, near-floor clipping)
   but the face blood is the same and stable over the frames, no layer stripes.
   The blood bursts (models 7..0x23 with the orange spikes) were already drawn per part hit (43 in 6000 frames of
   `--autofire`); our autofire shoots every 15 frames, MAME's player shot more often.
2. **#4 results researchers** (sparks 2): the nine researchers are one model, 0x65E, that kind176_results /
   _hold / _end (0x9B9F0 / 0x9BC00 / 0x9BDC0) draw every frame in camera space at (0, -10, 40) turned 0x8000 on y,
   independent of the rescue count (MAME shows nine with RESCUED x 3). It was left out; now drawn
   (src/game/clear.cpp kind176_backdrop_draw). c12 shot: the group fills the window like MAME, size and place close;
   ours is lit brighter / whiter than MAME's (open). No rescuing run was needed (the model ignores the rescues).

Other workers' files touched: src/render/renderer.cpp, gl.h (depth clamp), src/game/clear.cpp, runtime.h.
