// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): kind 37, the
// Chariot (boss_chariot.cpp, teaser L7). Arcade forms only. UNVERIFIED (teaser) where marked: not reached by a capture.
    // --- kind 37, the Chariot (boss_chariot.cpp) ---
    void boss_chariot(uint32_t obj);                                          // 0x97880 (kind 37)
    void chariot_entrance(uint32_t obj);                                      // 0x97A90
    void chariot_start_direct(uint32_t obj);                                  // 0x97C80 (proofs-6: p6_cstart2)
    void chariot_combat_start(uint32_t obj);                                  // 0x97CA0
    void chariot_frame(uint32_t obj);                                         // 0x97D20
    void chariot_armour_break(uint32_t obj);                                  // 0x97FE0
    void chariot_death(uint32_t obj);                                         // 0x98130
    void chariot_death_draw(uint32_t obj);                                    // 0x98298 (chariot_death's tail)
    void chariot_armoured_hits(uint32_t obj);                                 // 0x982B0
    void chariot_unarmoured_hits(uint32_t obj);                               // 0x98450
    void chariot_act_walk(uint32_t obj);                                      // 0x98560
    void chariot_act5_armour(uint32_t obj);                                   // 0x98720
    void chariot_act7_retreat(uint32_t obj);                                  // 0x987C0
    void chariot_act_attack(uint32_t obj);                                    // 0x98950
    void chariot_attack_a(uint32_t obj);                                      // 0x98A20
    void chariot_attack_b(uint32_t obj);                                      // 0x98B60
    void chariot_attack_r(uint32_t obj);                                      // 0x98C80 (proofs-6: p6_def / p6_r5 / p6_2p)
    void chariot_act14_face_players(uint32_t obj);                            // 0x98D10
    void chariot_act12_turn(uint32_t obj);                                    // 0x98DB0 (proofs-6: p6_r5, p6_cstart)
    void chariot_act2_land(uint32_t obj);                                     // 0x98E70 (proofs-6: p6_def / p6_r5 / p6_2p)
    void chariot_act_hit_reaction(uint32_t obj);                              // 0x98FA0
    void chariot_hit_reaction_end(uint32_t obj);                              // 0x99210
    void chariot_act17_wait_combat(uint32_t obj);                             // 0x99260 UNVERIFIED (teaser): not reached in the l7 captures
    void chariot_draw(uint32_t obj);                                          // 0x992D0
    void chariot_hitbox_task(uint32_t t);                                     // 0x993A0
    void chariot_armour_off(uint32_t obj, uint32_t motion, uint32_t frame);   // 0x99450
    void chariot_plate_state_set(uint32_t part);                              // 0x994B0
    void chariot_plates_off(uint32_t obj);                                    // 0x99500
    void chariot_armour_piece_fade(uint32_t t);                               // 0x99570
    void chariot_armour_pieces_spawn(uint32_t part);                          // 0x99660
    void chariot_armour_break_parts(uint32_t obj);                            // 0x997C0
    void chariot_part_index_rec(uint32_t part);                               // 0x997F0
    void chariot_part_index_build(uint32_t obj);                              // 0x99830
    void chariot_armour_hit_find(uint32_t part);                              // 0x99870
    void chariot_armour_hit_test(uint32_t obj);                               // 0x99940
    void chariot_part_damage_walk(uint32_t part);                             // 0x99A30
    void chariot_part_damage(uint32_t obj);                                   // 0x99B30
    void chariot_part_draw_hook(uint32_t part);                               // 0x9A040 (armoured body +0x11C)
    void chariot_part_draw_dispatch(uint32_t part);                           // 0x9A280 (unarmoured body +0x11C)
    void chariot_part_draw_plate(uint32_t part);                              // 0x9A2A0
    void chariot_part_draw_chest(uint32_t part);                              // 0x9A2E0
    void chariot_part_draw_waist(uint32_t part);                              // 0x9A450
    void chariot_part_draw_pair(uint32_t part);                               // 0x9A550
    void chariot_part_draw(uint32_t part);                                    // 0x9A660
    void chariot_skin_flag(uint32_t obj);                                     // 0x9A890
    void chariot_title_card(uint32_t t);                                      // 0x9A8C0
    void chariot_life_bar_task(uint32_t t);                                   // 0x9AA30
    void chariot_spark(uint32_t point);                                       // 0x9AB00
    void chariot_hit_glow(uint32_t t);                                        // 0x9AB90
    void chariot_hit_glow_spawn(uint32_t hb);                                 // 0x9AD60
    void chariot_landing_dust(uint32_t obj);                                  // 0x9AD90
    void chariot_armour_piece_reset(uint32_t t);                              // 0x9ADD0
    void chariot_spark_anim(uint32_t t);                                      // 0x9AE20
    void chariot_landing_dust_anim(uint32_t t);                               // 0x9AF10
    // helpers without an original routine of their own (inline in the routines above)
    bool chariot_motion_end(uint32_t obj);
    void chariot_footstep(uint32_t obj, uint32_t frames, uint32_t cue);
    void chariot_attack_hit(uint32_t obj);
    uint32_t chariot_heading(uint32_t obj, uint32_t x, uint32_t y, uint32_t z);
    void chariot_heading_step(uint32_t obj, uint32_t step, float v[3]);
    void chariot_share_counters(uint32_t counter, uint32_t part);
    void chariot_even_pair(uint32_t parts, uint32_t counter);
    int32_t chariot_role_class(uint32_t part);
    void chariot_hand_matrix(int32_t cls);
    void chariot_home_point(int32_t cls);
    void chariot_plate_model(uint32_t part, uint32_t k);
    // boundaries the Chariot calls (the HUD layer: sprites and the boss life bar; R4 draws them). If another kind
    // declares the same ones, they move to core_bounds.h at the merge (one declaration)
    virtual void sprite_bank_load(int bank, int sheet);                   // 0x17D50 (sprite_banks.cpp)
    virtual void sprite_bank_free(int bank);                              // 0x17BD0 (sprite_banks.cpp)
    virtual void sprite_place(uint32_t layer, int a, int b, int x, int y, int sprite) {}   // 0x17D80 (the title: sprites 0xDC / 0xDF)
    virtual void sprite_draw(uint32_t layer, int x, int y, int sprite) {}  // 0x180B0
    virtual void life_bar_init(int x, int y, int maximum);                // 0x6BA90 (hud_state.cpp)
    virtual int32_t life_bar_update(int32_t value, int finish);           // 0x6BC40 (-1 when done; hud_state.cpp)
