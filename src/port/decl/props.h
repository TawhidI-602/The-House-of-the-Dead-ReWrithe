// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): the props and
// prop-like scene kinds (props.cpp, prop_kinds.cpp; teaser L6). Arcade forms only. UNVERIFIED (teaser) marks the
// routines no capture has proven yet (docs/teaser_status/l6.md).
    // --- kind 128 prop_object and its helpers (props.cpp) ---
    void prop_object(uint32_t obj);                                           // 0x39930 (kind 128)
    void prop_init(uint32_t obj);                                             // 0x42740
    void prop_shootable(uint32_t obj);                                        // 0x427E0
    void prop_draw(uint32_t obj);                                             // 0x42300
    void prop_schedule_update(uint32_t obj);                                  // 0x3FCB0
    void prop_default(uint32_t obj);                                          // 0x3B160
    void prop_bullet_impact(uint32_t obj);                                    // 0x42460
    void prop_bullet_impact_mark(uint32_t obj);                               // 0x42F20
    void obj_copy_pose(uint32_t a, uint32_t b);                               // 0x42860
    void prop_break_anim_spawn(uint32_t obj);                                 // 0x3FE90
    void prop_break_anim(uint32_t obj);                                       // 0x3FD80
    void prop_collapse_anim_spawn(uint32_t obj);                              // 0x40070
    void prop_collapse_anim(uint32_t obj);                                    // 0x3FF70
    void prop_skull_burst(uint32_t obj);                                      // 0x401D0
    void prop_piece_fall(uint32_t obj);                                       // 0x42D90
    void prop018_kkkdolam_break(uint32_t obj);                                // 0x41E20
    void prop_piece_random(uint32_t t, uint32_t obj);                         // 0x428A0
    void prop_break_piece(uint32_t obj);                                      // 0x403F0
    void prop_statue_break(uint32_t obj);                                     // 0x412C0
    void prop_tumble_copy(uint32_t obj);                                      // 0x416A0
    void prop_piece_bounce(uint32_t obj);                                     // 0x40520
    void prop_drop_life_item(uint32_t obj);                                   // 0x3D4F0
    void item_life_frame(uint32_t obj);                                       // 0x3D260
    void item_gift_spawn(uint32_t obj);                                       // 0x42420
    void prop_drop_gold_coin(uint32_t obj);                                   // 0x3D920
    void prop_coin_run(uint32_t obj);                                         // 0x3D680
    void prop_drop_gold_frog(uint32_t obj);                                   // 0x3DB30
    void prop_gold_frog_init(uint32_t obj);                                   // 0x429B0
    void prop_gold_frog_run(uint32_t obj);                                    // 0x3DA10
    void prop_gold_frog_leap(uint32_t obj);                                   // 0x3DBE0
    void item_sparkle_spawn(uint32_t x, uint32_t y, uint32_t z);              // 0x3FC70
    void item_sparkle(uint32_t obj);                                          // 0x42C20
    void prop_splash_spawn(uint32_t obj);                                     // 0x3E590
    void prop_splash(uint32_t obj);                                           // 0x42AB0
    void prop069_powa(uint32_t obj);                                          // 0x3E7B0
    void prop_break_pieces(uint32_t obj);                                     // 0x41900
    void prop_plate_shards(uint32_t obj);                                     // 0x40290
    void prop041_tabul_scheduled(uint32_t obj);                               // 0x3B910
    void prop005_tokei_clock(uint32_t obj);                                   // 0x3C0F0
    void prop005_clock_pieces(uint32_t obj);                                  // 0x40FB0
    void kind137_prop_group(uint32_t obj);                                    // 0x39AC0 (kind 137)
    void kind139_prop_group(uint32_t obj);                                    // 0x3A980 (kind 139)
    void kind147_prop_group(uint32_t obj);                                    // 0x3A400 (kind 147)
    void kind134_burst_pieces(uint32_t obj);                                  // 0x3A1B0 (kind 134)
    void kind134_piece(uint32_t obj);                                         // 0x3DFC0
    void kind134_dust_spawn(uint32_t x, uint32_t y, uint32_t z);              // 0x3FC30
    void kind134_dust(uint32_t obj);                                          // 0x42BA0
    // helpers without an original routine of their own (coprocessor sequences inline in the routines above)
    void prop_light0();
    void prop_place_xyz(uint32_t obj);
    void prop_scale(uint32_t s);
    void prop_glow(uint32_t model);
    void prop_billboard();
    void prop_view_pos(uint32_t obj, uint32_t x, uint32_t y, uint32_t z);
    float prop_mark_height(uint32_t obj);
    uint32_t prop_face_camera(uint32_t obj);
    int prop_shooter(uint32_t obj);
    void prop_piece_spin(uint32_t t, uint32_t limit);
    void prop_piece_throw(uint32_t t, uint32_t a, float div);
    uint32_t prop_glow_phase();
    // boundaries only the props call (not ported: sub-types not in Chapter 1, or other systems)
    // prop_bookshelf_collapse 0x40BB0 / prop_bookshelf_remains 0x40F50: ported in props_ch2.cpp (worker "ch2play")
    // prop_hit_drop 0x42140 / prop_lid_fly 0x422B0: ported in props_ch2.cpp (decl/props_ch2.h, worker "ch2")
    virtual uint32_t colour_blend32(uint32_t a, uint32_t b, uint32_t t);      // 0x23CC0 (palette.cpp; proofs-8: p8_sweep1, its result compared)
    // --- the other prop-like kinds (prop_kinds.cpp) ---
    void obj_move_velocity(uint32_t obj);                                     // 0x6A830
    void obj_model_draw_colour(uint32_t obj);                                 // 0x637B0
    void kind169_camera_window(uint32_t obj);                                 // 0x3390 (kind 169)
    void kind21_scene_mover(uint32_t obj);                                    // 0x311F0 (kind 21)
    void kind181_car(uint32_t obj);                                           // 0x4E280 (kind 181)
    void kind181_car_wait(uint32_t obj);                                      // 0x4E2A0
    void kind181_car_drive_in(uint32_t obj);                                  // 0x4E2F0
    void kind181_car_brake(uint32_t obj);                                     // 0x4E600
    void kind181_car_stop(uint32_t obj);                                      // 0x4E960
    void kind181_car_parked(uint32_t obj);                                    // 0x4EAF0
    void kind181_car_skipped(uint32_t obj);                                   // 0x4EC10
    void player_char_props_draw();                                            // 0x4ED20
    void kind175_rat(uint32_t obj);                                           // 0x4960 (kind 175)
    void kind175_rat_run(uint32_t obj);                                       // 0x49B0
    void kind175_rat_sniff(uint32_t obj);                                     // 0x4AF0
    void kind175_rat_shot(uint32_t obj);                                      // 0x4BF0
    void kind175_rat_draw(uint32_t obj);                                      // 0x4C90
    void kind38_barrel(uint32_t obj);                                         // 0x4360 (kind 38)
    void kind38_barrel_fall(uint32_t obj);                                    // 0x4430
    void kind38_barrel_rise_start(uint32_t obj);                              // 0x44D0
    void kind38_barrel_rise(uint32_t obj);                                    // 0x44F0
    void kind38_barrel_tumble_start(uint32_t obj);                            // 0x4540
    void kind38_barrel_tumble(uint32_t obj);                                  // 0x4560
    void kind38_barrel_settle_start(uint32_t obj);                            // 0x4620
    void kind38_barrel_settle(uint32_t obj);                                  // 0x4650
    void kind38_barrel_rest_start(uint32_t obj);                              // 0x46E0
    void kind38_barrel_rest(uint32_t obj);                                    // 0x4700
    void kind38_barrel_draw(uint32_t obj);                                    // 0x4770
    void kind38_barrel_break(uint32_t obj);                                   // 0x4810
    void kind77_lock(uint32_t obj);                                           // 0x70490 (kind 77)
    void kind77_lock_spark_spawn(uint32_t obj);                               // 0x707A0
    void kind77_lock_spark(uint32_t obj);                                     // 0x70A30
    void kind076_slider(uint32_t obj);                                        // 0x70630 (kind 76)
    void kind140_dropped_weapon(uint32_t obj);                                // 0x61DD0 (kind 140)
    void kind140_dropped_weapon_frame(uint32_t obj);                          // 0x61E30
    // helpers (inline in the routines above)
    void car_draw(uint32_t obj, bool steer, uint32_t bodyCmd);
    void car_parked_draw(uint32_t obj, bool door);
    void car_turn(uint32_t obj, int32_t div);
    void rat_check_shot(uint32_t obj);
    void barrel_spin(uint32_t obj);
    void barrel_tumble_step(uint32_t obj);
    void kind135_ebitb_drop(uint32_t obj);                                    // 0x606D0 (kind 135)
    void kind135_ebitb_frame(uint32_t obj);                                   // 0x60820
    void kind135_ebitb_dead_fall(uint32_t obj);                               // 0x60E40
    void kind135_ebitb_decoy_land(uint32_t obj);                              // 0x61000
    void kind135_ebitb_dead_glide(uint32_t obj);                              // 0x610E0
    void kind135_splash_spawn(uint32_t obj);                                  // 0x61F80
    void kind135_splash(uint32_t obj);                                        // 0x64B10
    void ebitb_uncount(uint32_t obj);                                         // inline in kind135_ebitb_frame
    virtual void body_drips_draw(uint32_t part);                              // 0x60430 (support_c.cpp)
    // branch-event kinds (prop_kinds.cpp)
    void kind145_breakable_wall(uint32_t obj);                                // 0x3AB50 (kind 145)
    void kind145_wall_piece(uint32_t obj);                                    // 0x3E030
    void kind145_route_hold_start();                                          // 0x3E290
    void kind145_route_hold(uint32_t obj);                                    // 0x42A90
    void kind145_wall_chips_spawn(uint32_t obj);                              // 0x414D0
    void kind148_prop_group(uint32_t obj);                                    // 0x39EA0 (kind 148)
    void prop025_haguuu(uint32_t obj);                                        // 0x3D190
    void kind144_prop_group(uint32_t obj);                                    // 0x3A0F0 (kind 144)
    void kind144_drip(uint32_t obj);                                          // 0x3CB50
    virtual void splash_9f1_spawn(uint32_t x, uint32_t y, uint32_t z);        // 0x44B10 (virtual: older captures recorded it as a boundary)
    void splash_9f1(uint32_t obj);                                            // 0x452C0
    void kind143_prop_group(uint32_t obj);                                    // 0x3A5D0 (kind 143)
    void prop062_kibako_stack(uint32_t obj);                                  // 0x3DCA0
    void kind149_prop_creature(uint32_t obj);                                 // 0x3AC90 (kind 149)
    void prop063_frog(uint32_t obj);                                          // 0x3E2B0
    void prop063_frog_fall(uint32_t obj);                                     // 0x3E4E0
    void prop119_creature(uint32_t obj);                                      // 0x3F8B0
    void prop119_creature_fall(uint32_t obj);                                 // 0x3FA20
    void kind146_prop_group(uint32_t obj);                                    // 0x39D80 (kind 146)
    void kind146_ball(uint32_t obj);                                          // 0x3CC90
    void kind141_slug(uint32_t obj);                                          // 0x61FD0 (kind 141)
    void kind141_slug_frame(uint32_t obj);                                    // 0x62290
    void kind141_slug_burst(uint32_t obj);                                    // 0x62EB0
    void kind141_slug_burst_draw(uint32_t obj);                               // 0x62F4C
    void kind142_slug_spitter(uint32_t obj);                                  // 0x62090 (kind 142)
    void kind142_slug_spitter_frame(uint32_t obj);                            // 0x62A20
    void kind142_slug_fall(uint32_t obj);                                     // 0x63120
    void kind142_spit_spawn(uint32_t obj);                                    // 0x635D0
    void kind142_spit(uint32_t obj);                                          // 0x63200
    void bspline_basis(uint32_t seg, float t, uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3);   // 0x647B0
    void slug_light(int32_t param);                                           // inline (a light record)
    void slug_frame_end(uint32_t obj);                                        // inline in the slug frames
    void slug_uncount(uint32_t obj);                                          // inline in kind141_slug_frame
