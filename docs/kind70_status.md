# Kind 70 (the agile creatures): port status

**Status (2026-10-15, session on branch kind70; written for the morning merge)**

- **Ported (A and PC): all 52 routines of kind 70** in `src/port/enemy_agile.cpp` (one member per original routine,
  `Game<P>`, `if constexpr (P::pc)` where the PC differs): the spawn and set-ups 0-12, `kind70_frame`, the Chapter 1
  route check, the hits, both hit reactions (ground / airborne), the death, states 0-29 (approach, think with the five
  per-type think routines, wall, bite / leap, wall moves, leap, turn, stance, the last state, Drake's knife throw and
  roll, Moody's wall / ceiling states), the shared helpers (`obj_stop`, `kind70_st02_sub`, `kind70_after_hit_state`,
  the wall-attack / knife / projectile target points, the hand-model restore, Moody's throw, the knife spawn) and the
  two projectile tasks (`kind70_knife_fly`, `moody_projectile`).
- **Proven on A** (arcade, full-game recording `play_us_20261007_2238`, 1530 s):
  - `k70_full1`: 973 captures, 0 differ (spawn 42, frame 400, hits 400, ground hit reaction 131).
  - `k70_full2`: 2,822 captures, 0 differ (adds the airborne hit reaction 400, death 400, states 2 125, 3 400, 4 15,
    5 282, 6 5, 7 4, 8 23, 9 195).
  - `k70_full3`: 6,334 captures, 0 differ (adds set-ups 0 / 2 / 5 / 6 / 8 / 10 / 11 as states, the Moody / kageta
    think, states 23 / 24 / 27 / 28 / 29, the hand-model restore, Moody's throw).
  - `k70_full4`: 1,317 captures, 0 differ (the knife task 393, Moody's projectile 294, the fade-in set-up 9 400, the
    monkey / Drake / Rubin think, the knife spawn, the wall-attack target).
  - `k70_skip` (fresh auto-aim game, `-AimSkip 8c090`: the creatures are never shot): 5,878 captures, 0 differ (adds
    the stance state 10 400; the bite 400, leap 400).
  - `k70_r3` / `k70_r6` (fresh games, `-AimSkip 8c090`, `-Route "1:3"` / `"1:5=0"`): 1,247 + 1,547 captures,
    0 differ (set-up 3 45 captures, stance / bite / death again).
  - **Total on A: 20,118 captures, 0 differences.**
  - Branch coverage of the kind 70 routines over the full / skip runs: 561 branches, 197 both ways, 213 one way, 151 never.
  - **Not reached on A yet:** states 11 (`kind70_st11_last`) and 17 (`kind70_st17_bite_b`), set-ups 1 / 7 / 12
    (Drake's), `kind70_ch1_route_check`'s route branch.
- **Proven on PA / PM (2026-10-15 ~7:50):** k70_pa 3,788 and k70_pm 3,666 captures, 0 differ (auto-aim games).
- PC differences found (kept in the code): division by 3 / 4 / 2 done as multiplications (x 0x3EAAAAAB, x 0.25,
  x 0.5); the coprocessor's normalise (0x0C) as 1 / the table square root; the angle / distance functions of the kind
  69 / 70 file (`pc_vec_angles10`, `pc_floor_distance10`, `pc_matrix_transform10`); the camera-matrix transforms on the
  PC's software matrix stack (stored matrix 1); the draw of the knife / projectile / fade-in model with the file's
  matrix helpers (boundaries `pc_matrix_translate10`, `_rotate_y10` / `_x10` / `_b10`, `_mul_stored10`), the scale
  applied to the matrix inline and the model only with the option `pc_draw_bats` (the fade-in: unless
  `pc_no_extra_model`); task sizes 8 bytes larger; the knife / projectile scale +0x90 computed as +0x94 x 1.5 (the
  arcade stores the constant).

## Pairs fixed in tools/ghidra/names.tsv (from the PC's own calls / tables)

- The five think routines were rotated by one (state 3's own calls): monkey 0x4FF1BC, Drake 0x4FF30C, Rubin 0x4FF59C,
  Moody 0x4FF6E0, kageta 0x4FF870.
- `kind70_st26_none` 0x5028F0 (the PC's state table); `kind70_knife_fly` 0x501E38 (knife_spawn's task_create),
  `kind70_knife_fly_sub` 0x502298, `moody_projectile` 0x502E14 (moody_throw's task_create), `moody_projectile_target`
  0x503314, `impact_effect_spawn` 0x46BDC0.
- `floor_splat_spawn` / `floor_splat` are PC 0x487004 / 0x486FB4 (they were paired with kind135_splash_spawn /
  kind135_splash, whose arcade code puts the splash 2 lower and rotates / scales it; those two are unpaired now).
- New data names: `kind70_setups`, `kind70_states`, `kind70_slot_state`, `kind70_slot_matrix`, `kind70_attack_turn`,
  the room box `room_x0` .. `room_z1`, the ride position `ride_pos_*` / `ride_prev_*` (the PC keeps these six floats in
  another order), `players_in_game_b` (meaning unverified).

## Open items

- PA / PM captures (arguments below).
- Coverage of the rarely reached paths (see the A results); routes that reach the Drake / Moody / kageta set-ups
  (chapters 3-4) beyond the full-game recording.
- `kind70_st02_approach` passes its own y (not the difference) to the heading function, as the original does.

## PC run arguments (for later; one PC game at a time)

PA (Arcade Mode) and PM (PC Mode) with the auto-aim scripts, all of kind 70 captured:

```
tools\pcharness\run_pc.ps1 -Out private\pcharness\runs\k70_pa -Ticks 90000 -InfLives -InputScript tools\pcharness\scripts\game_aim.txt -CapMax 400 <the -CapFuncs / -CapPorted / -CapBoundary lists below>
tools\pcharness\run_pc.ps1 -Out private\pcharness\runs\k70_pm -Ticks 90000 -InfLives -InputScript tools\pcharness\scripts\game_pm_aim.txt -CapMax 400 <the same lists>
```

then `build\Release\hotd_proof.exe pccapture private\pcharness\runs\k70_pa` (and `k70_pm`). A run with the
creatures left alive: add `-AimSkip 4fdf68` (kind70_frame on the PC).

The lists (PC addresses; -CapPorted = every ported routine incl. the ported helpers, -CapBoundary = k32dog_pa's
boundary list plus kind 70's unported callees):

```
-CapFuncs 4fde44,4fdf68,4fe118,4fe320,4fe53c,4fe8b8,4fe3f4,4fee84,4fef90,4ff1bc,4ff59c,4ff6e0,4ff30c,4ff870,4ffadc,4ffb88,4ffc34,4ffd38,4ffdc4,4fffa0,500158,500628,5035f4,500800,500cd4,500e68,5014f8,501000,4fec58,5010d0,501194,5017a0,5019d4,502600,501ba0,501d80,5028f4,502a18,502ca4,5033f4,503454,502378,501e38,502e14
-CapPorted 4fde44,4fdf68,4fe118,4fe320,4fe53c,4fe8b8,4fe3f4,4fee84,4fef90,4ff1bc,4ff59c,4ff6e0,4ff30c,4ff870,4ffadc,4ffb88,4ffc34,4ffd38,4ffdc4,4fffa0,500158,500628,5035f4,500800,500cd4,500e68,5014f8,501000,4fec58,5010d0,501194,5017a0,5019d4,502600,501ba0,501d80,5028f4,502a18,502ca4,5033f4,503454,502378,501e38,502e14,4fdea4,4fdf4c,4fdf64,4fe098,5028f0,46ada0,46b468,502298,503314,4fc018,4fbfb0,4fc76c,4fbf30,569600,4c4168,4c3d58,471828,45b9d0,437344,458c1c
-CapBoundary 478b64,4c3ad0,46ea30,4c56c8,478f08,4784d4,4122d0,45b9c4,4c55cc,46f28c,47165c,4677fc,4c589c,4c582c,46f2d4,478e54,43e5f0,4182e4,478f50,4788cc,43e400,478fa8,505cc0,4c86d0,4c8504,4c3b80,489c18,4d99a0,4e1e70,4e3124,47901c,48c14c,4e7678,48b084,48aa34,412000,46eee0,46ad04,478998,46b7b4,46ae0c,46ec08,436fe4,46b4a8,4d900c,4867b4,486950,486660,486520,4864dc,486d60,4c4c60,4791c8,4c4648,4c4774,4c4994,4c4a24,4c4af0,4c4b34,4c4b48,4c4bac,4c4bdc,4c4c0c,4c4c34,4c4c5c,46f2b8,48ef20,48e528,48d18c,4e251c,4e1c50,4e1f78,4e2104,4e2e84,4e24c0,4c5674,4e2c14,471750,4371b8,4c576c,43d7f8,4c5924,4c5b1c,477b00,4c521c,4c4f5c,4c5048,4db798,447c34,4472ec,44782c,447698,578a6b,4477a4,447e60,4c53d8,46ee90,46a91c,46a984,46ab60,46ac40,46af9c,46b27c,48a254,487004,46ae7c,46bab4,447e1c,43e5cc,4fc114,4fc334,4fc43c,4fc5c8,4fcb84,42ce70,43d870,46bdc0
```
