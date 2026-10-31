# hud: the 2D sprite layer, status (2026-10-16 overnight)

Branch `claude/hud-sprites` (worktree `.claude/worktrees/agent-ae5b4650894ae5a92`, build folder `build-hud`). Not merged.
Everything new is **UNVERIFIED (teaser)**: read from the typed decompilation / disassembly, checked with window
screenshots, no capture proof. Formats: docs/formats.md §31; runtime: docs/runtime.md "HUD and the 2D layer".

## Done

- **Arcade 2D layer decoded and running**: the sprite table 0xA6590 (tiles / palettes / layouts), the System 24 tile
  chip's cells, character RAM, window masks, black = transparent. The original's routines in src/game/sprites.cpp
  (Runtime members, 40 routines: sprite banks, sprite_place / draw_anim / draw_clipped / draw, tilemap_clear_rect,
  tile_text_draw / voice_message_frame_sub, gun_flash_on / off, chapter_screen_reset, hud_sprites_load, hud_lives_draw,
  hud_ammo_draw, hud_player_full_draw, hud_sprite107_*, hud_player_start_draw, hud_popup_update, hud_start_prompt (free
  play), hud_continue_*, hud_game_over_*, continue_screen_draw, life_bar_init / update / draw). The flow (flow.cpp,
  players.cpp) now calls the original's HUD routines at the original's places (CONTINUE? and digits, the centre
  CONTINUE, GAME OVER 0x106 and the layer clears of game_over_start, PRESS START of a waiting player, the HUD at a join).
- **Chapter clear (kind 176)** with the original's sprites (metal frame 0xA7, the area picture 0xA6 low priority,
  RESCUED x n, LIFE BONUS x n, SCORE and the digits: kind176_score_draw); R4's text stays for the placeholder only.
  PC: 0xA6 has no PC counterpart (left out). Low-priority cells are composed under the priority ones (over the 3D).
- **PC sprites decoded**: SPR.RBH = descriptors over BIGGER.VMC pages (§31); the PC's sprite-ID layers mirrored
  (Runtime::pcCells); arcade -> PC sprite map for all 0x276 sprites by picture (tools/sprites_tool.py map ->
  src/assets/sprite_map_pc.inc); subtitles in the PC font (sprite = char - 0x1E). `--models pc` draws only PC sprites.
- tools/sprites_tool.py: contact sheets of arcade / PC sprites, arcade-PC pairs, the map.
- Debug: `HOTD_TRACE_SPRITES=1`, `HOTD_DUMP_2D=<frame>`.

## Checkpoints (window screenshots, private\screenshots)

Copies with names in private\screenshots\hud (arcade_* / pc_*): title_card (kind 172, frame 90), subtitle ("That
voice!!", `--event 19` frame 100), hud_play (arcade: torches, RELOAD, SHOOT OUTSIDE OF THE SCREEN, frame 1500),
chariot_title (Chariot TYPE 27 + WEAK POINT, `--event 19` frame 2300), lifebar (+ bullets; arcade also P2's PRESS
START BUTTON, frames 2500 / 2600), continue (`--lives-out 200`, frame 260), game_over (frame 1500), chapter_clear (arcade, `--event 19 --autofire` frame 4950).
Command: `build-hud\Release\hotd.exe --root <repo> --no-launcher --models arcade|pc --mute --autofire --size 1280x720
--event 19 --shot-at 2300`.

## Not done / for later

- Not compared with a MAME frame / the PC game's screen: scroll sign, exact colours (MAME's colour table + gamma),
  window-mask uses beyond the title, the PC placement scale (cell x 10 px of 640 x 480, height-fitted).
- Low-priority cells are drawn over the 3D (no hook between the 3D and the 2D); the shot flash (0x46D50), coin / credit prompts (free play only),
  the emblem model 0x65E of the results, the game over map (sprites 0x243..), the attract / title / ranking screens.
- The arcade -> PC map is by picture: sprites outside the HUD / boss / continue / cards set are unchecked
  (`py tools/sprites_tool.py pairs <png> <ids>` shows them side by side). Letterbox sprites 0x8E / 0x97 / 0xA0 have
  no PC match (R4's bars are drawn instead).
- G::region (0x51F791) picks the subtitle style: 0 = Japan (voice sprites), else the English text; a from-scratch boot
  (s1) must keep it non-zero.
- Japanese PC mode: the PC subtitle path draws ASCII only.

## Second batch (2026-10-16, after the merge with main and claude/t1-integration)

- **ROM boot (s1):** region 0x51F791 = 1 (USA) from boot.cpp; subtitles take the English path without the snapshot
  (headless trace: "Others are still inside. Save them." at frame 1058). No change needed.
- **Compared with MAME** (private\mame_dumps\fullgame_us snapshots against the runtime started from the same work RAM,
  `--snapshot t00620_workram.bin --no-chapter-start`, 496 x 384): the bullets, torches and 1P frame sit on the same
  pixels (no scroll / placement difference). Colours differed: MAME passes the palette through its colour table and a
  gamma `max(v - 64, 0) x 255 / 191` and copies black as transparent; now applied in Arcade2D (bullet 198,133,34 vs
  MAME 189,121,36; life bar fill 20,0,255 in MAME). Dark colours are therefore transparent too, as in MAME.
- **Shot flash** (padpass 2026-10-27: replaced by the host's white flash, Controls > Shot Flash; docs/teaser_status/pads.md): `--shot-flash` showed gun_flash's (0x46D50) picture: the window mask of layers 0 / 1 for 4 frames
  after a shot, showing the grey screen 0xA2 (checked: mean brightness 70 -> 181 on the flash frames). Off by default
  (a strobe on every shot); the runtime's gun stand-in keeps gun_flash_phase for the shot logic, so the flash keeps
  its own counter (a round fewer than the frame before).
- **Low-priority cells stay over the 3D.** Behind the 3D would need a depth trick, but the sky is drawn without depth
  writes (it would be covered), and in Chapter 1 the only low-priority sprite is kind 176's area picture inside the
  results frame, where the 3D world (still drawn by the view) would show through instead. Revisit with render.
- **Hit marks:** hud_player_notice_show 0x945D0 (t1's player_hit_notice): the slash / bite / claw / stab marks
  0x28..0x2B on layer 2 with their sound, taken off by hud_popup_update; also called from hit_screen_shake (the
  runtime's task order; the second call is ignored as in the original). Checked: `--event 19` frame 373, bite mark
  0x29 (private\screenshots\hotd_20261016_003135_f385.png). Hits passed with shake 0 show no mark, as in the original.
- Not done: the coin prompts (the teaser runs in free play, where the arcade's shared prompt is off in mode 4), the
  game over map (the route scroll, walker and camera tasks 0x5CB50..0x5D4F0: not cheap).

## User play-test 2026-10-16 (branch claude/hud2, build folder build-hud2)

The user: item / bonus pickup text (LIFE UP) missing; subtitles not like the arcade / PC.

- **The pickup "text" is 3D, not 2D**: LIFE UP! is model 0x3BB (P1) / 0x3BC (P2), `lifeupita`, drawn in camera space by
  hostage_gift_fly 0x1E040 (hostage scenes, the props' life item via item_gift_spawn) and gift_effect 0x1DE90. The gold
  coin (200) and gold frog (1000) show only the item sparkle (item_kira 0x2E0..0x311, item_sparkle 0x42C20); the
  original draws no score text for them (props.cpp from the decompilation; no 2D call on either path).
- **Fixed: researcher_gift_spawn 0x707E0 was an empty boundary stub** (core_bounds.h): a researcher rescued in a kind-64
  scene (kind64_st7 / st14 ...) gave neither the extra life nor the LIFE UP! item. Ported (arcade form: gift_effect at
  his home position, the life by 0x5207B0; the PC 0x4CF2BC gives it to pc_last_killer, vtable only) with gift_effect
  (hostage_gift_fly's body, shared as gift_fly_step).
- **Fixed: colour_blend32 0x23CC0 was a stub returning 0**: the item sparkle's / life item's glow colour (palette
  entry 985, 0x5206AC) stayed 0. Ported (palette.cpp).
- Checked (bot 1, frame 3760, offscreen): LIFE UP! over the bridge researcher as MAME fullgame_us snap 0054. Arcade
  models: dark red (MAME a brighter red; polygon colour palette entry 1020 = 30,0,18 through the colour tables; not
  chased). **PC models: magenta**: the PC model 0x3BB (CREAT2.RBH) uses texture page 589 and draws in the colours the
  PC's texture data gives (real letter detail, not the missing-page fill); left as is. UNVERIFIED: no real-PC capture
  of a LIFE UP.
- **Sweep** (HOTD_TRACE_SPRITES now also logs the item / gift tasks; headless `--bot 1/2/5` with `--route 1:0,1:3`,
  `--bot-players 2`, 12000-14000 frames): every sprite / tile routine the Chapter 1 code reaches is ported (from
  maincpu_index.tsv: the callers of sprite_place / sprite_draw(_anim / _clipped / _tinted) / hud_sprite_put /
  tile_text_draw; the rest are the test menu, attract, title, ranking, mode 8 lesson, ending, Chapters 2-4 bosses,
  name entry, and the coin prompts that free play does not show). Sprites seen: HUD 0x10 / 0x14 / 0x36 / 0x40 /
  0xB7-0xBE, HOLD YOUR FIRE 0x107, hit marks 0x28-0x2D, CONTINUE 0x108-0x113, GAME OVER 0x106, game over map
  0x243-0x275, results 0xA8-0xAF, letterbox. No 2D boundary stub is hit on these routes (stubs hit: shadow_update,
  sophie_skin_draw, point_clip_to_depth, part_blow_off, frog_splash_spawn, prop_bookshelf_*: all 3D).
- **Subtitles, arcade**: font, colour and position compared with MAME snaps 0034 ("Sophie!?") and 0041 ("Rogan, it's
  you!"): the same (no change).
- **Subtitles, PC: fixed.** They used the serif title-card font (sprite = char - 0x1E) at the arcade's place. The PC
  (tile_text_draw 0x41B1C0 keeps one string; 0x41B114 draws it each frame) uses the 640 x 480 subtitle font, glyph
  routine 0x428494 with flag 0x400000 = char - 0x20 + 0x2DB, centred on the screen (width 0x427F64: glyph + 1, space 8),
  top at y 360. Checked against the real PC capture pc_look fw2 t00952: the same white black-edged font, the same
  width (about 355 px) and x; y consistent with 360 (the capture's top is cut). One line only (the later, lower one),
  as the PC.

## hud3 (2026-10-16 evening, branch claude/hud3, build folder build-hud3)

- **PC torches** were the wrong sprites: the arcade -> PC picture map took the orange set 0x44D.. for 1P's lit torch
  and mixed 2P's frames. The PC's hud_lives_draw 0x50B5B0 draws 1P lit 0x46F + step (7), burning out 0x476 + step
  (8), 2P 0x460.. / 0x467.., the bowl 0x44C: now overrides in tools/sprites_tool.py and sprite_map_pc.inc. Compared
  with the real PC capture private\screenshots\pc_crosshair\pc_t01262.png (640 x 480 = the capture / 3): the same
  flames (white core, red rim), the bowl under the flame, the same cells (bullets 20 px apart, torches 40). Flicker
  timing: the same code as the arcade (/ 6 % 7, phases from a table); not timed on the PC.
- **Reload sound**: both originals play it in player_shoot's off-screen path (arcade 0xA01200, PC cue 0x1C) when
  shots are on. The input stand-in set gun_shot on screen only, so the reload went past player_shoot (sound only by
  chance, then every frame). Now every pull reaches player_shoot; the gun record's trigger is cleared each frame. Side
  effect, as the arcade: an off-screen pull in a cut-scene (shots off) is a skip request too.
- **Auto reload** (launcher, off by default, `autoreload=`, `--auto-reload on|off`): an empty gun pulls off the
  screen the next frame (only while shots are on).
- **Crosshair "simple cross"** (crosshair=3, `--crosshair cross`): the drawn "+".
- **More crosshairs** (crosshair=4..9, size `crosssize=`, thickness `crossthick=`; `src/game/sights.*`, `Hud::buildPixelSight`): ring + dot, corner brackets, gap cross (dotted ring and pointer removed 2026-10-19 by the user), drawn in window pixels (player colours, dark rim, scaled with the window height); 9 = custom PNGs from `dataosshairs` / settings `crosshairs` (`crosshair.png` tinted, or `p1.png` / `p2.png` as they are; own PNG reader `util/png_read`).
- `--input-test`: the second game waits for the shooting (frame 301 held) and checks the reload sound (right button,
  corner, P2's Enter after joining), the auto reload on / off: 494 checks (wide), 498 (4:3), 0 failed.
