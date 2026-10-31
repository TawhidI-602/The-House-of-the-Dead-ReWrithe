// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): kind 97, the
// Kenfis dogs (enemy_dogs.cpp).
// The virtual ones were boundaries in older captures: the proof replays them where a capture recorded them so.
    // --- kind 97, the Kenfis dogs (enemy_dogs.cpp) ---
    uint32_t dog_rec(uint32_t obj);                                            // inline: the pack record
    void dog_call(uint32_t routine, uint32_t obj);                             // callx through dog_setups / dog_states
    void kind97_ddog_kenfis(uint32_t obj);                                     // 0x7C9A0
    void dog_set0_pack(uint32_t obj);                                          // 0x7CB90
    void dog_set_in(uint32_t obj, int32_t m, uint32_t frame, bool step0);     // set-ups 1-3
    void dog_set1_run_in(uint32_t obj);                                        // 0x7CBA0
    void dog_set2_jump_in(uint32_t obj);                                       // 0x7CC00
    void dog_set3(uint32_t obj);                                               // 0x7CC60
    void dog_set4_look_around(uint32_t obj);                                   // 0x7CCC0
    int32_t dog_pack_list_build();                                             // 0x7CD10
    uint32_t dog_motion_of(int32_t s, int32_t i);                              // inline
    void dog_motion_by_state(uint32_t obj);                                    // 0x7CE20
    void dog_motion_set(uint32_t obj, int32_t s, int32_t i, int32_t frames);   // 0x7D080
    void dog_set_motion_step(uint32_t obj, int32_t s, int32_t i, int32_t a, int32_t b);   // 0x7D190
    void dog_set1_frame(uint32_t obj);                                         // 0x7D2B0
    void dog_set2_frame(uint32_t obj);                                         // 0x7D330
    void dog_frame(uint32_t obj);                                              // 0x7EA80
    void dog_dust(uint32_t obj);                                               // 0x7EBB0
    void dog_hits(uint32_t obj);                                               // 0x7D960
    float dog_trig(uint32_t angle, bool cos);                                  // inline (A 0x1B / 0x1C; PC pc_trig_table)
    void dog_set3_frame(uint32_t obj);                                         // 0x7D460
    void dog_set4_done(uint32_t obj);                                          // inline
    void dog_set4_frame(uint32_t obj);                                         // 0x7D780
    uint32_t crand();                                                          // 0xA4E18 (A; PC: pc_crand)
    void dog_take_front_slot(uint32_t obj);                                    // inline (bite / leap)
    void dog_after_attack(uint32_t obj, bool retarget);                        // inline (bite / leap)
    void dog_st0_bite(uint32_t obj);                                           // 0x7DCB0
    void dog_st2_leap(uint32_t obj);                                           // 0x7DED0
    bool dog_pack_ready();                                                     // inline (PC 0x4DD9A4)
    void dog_st1_turn(uint32_t obj, uint32_t step);                            // inline
    void dog_st1(uint32_t obj);                                                // 0x7E270
    float dog_camera_distance(uint32_t obj);                                   // inline
    void dog_st3_turn(uint32_t obj, uint32_t step);                            // inline
    void dog_st3_circle(uint32_t obj);                                         // 0x7E620
    virtual void dog_knockback(uint32_t obj, float step);                      // 0x69880
    virtual void dog_st2_move(uint32_t obj, float side, float within, float speed);    // 0x1CD60
    virtual uint32_t dog_angle_to_target(uint32_t obj, float x, float y, float z);     // 0x1D9A0
    virtual uint32_t dog_st3_steer(uint32_t obj, float x, float z, float speed);       // 0x6A0B0 (1 = there)
    virtual float dog_set3_fall(float x, float y, float z, float tx, float ty, float tz, uint32_t angle);   // 0x1D710
    virtual uint32_t dog_set4_path(uint32_t obj, float x, float y, float z, float step);   // 0x1C820 (1 = there)
    float dog_log_sq(float d);                                                 // inline (dog_set4_path, dog_st3_steer)
    uint32_t dog_copro_angle(const uint32_t* v);                               // inline (A: coprocessor 0x0F + the fold)
    // the PC copies in the shared helpers file (0x445xxx) and the 0x4C1xxx file: same code as the generic routines
    void pc_matrix_identity11();                                               // PC 0x445CA4 (PC only)
    void pc_matrix_rotate_y11(uint32_t angle);                                 // PC 0x445654 (PC only)
    void pc_matrix_transform11(float x, float y, float z, float& ox, float& oy, float& oz);   // PC 0x445250 (PC only)
    void pc_matrix_identity12();                                               // PC 0x4C25B4 (PC only)
    void pc_matrix_rotate_y12(uint32_t angle);                                 // PC 0x4C1F64 (PC only)
    void pc_matrix_transform12(float x, float y, float z, float& ox, float& oy, float& oz);   // PC 0x4C1B60 (PC only)
    void pc_vec_angles12(float x, float y, float z, uint32_t& yaw, uint32_t& pitch);   // PC 0x4C1C48 (PC only)
    float pc_fast_sqrt12(float x);                                             // PC 0x4C239C (PC only)
    virtual void pc_matrix_translate11(float x, float y, float z) { pmx_translate(x, y, z); }           // PC 0x445434 (PC only)
    virtual void pc_matrix_rotate_z11(uint32_t angle) { pmx_rotate_z(angle); }                       // PC 0x44575C (PC only)
    virtual void pc_matrix_rotate_x11(uint32_t angle) { pmx_rotate_x(angle); }                       // PC 0x4458E8 (PC only)
    virtual void pc_matrix_translate12(float x, float y, float z) { pmx_translate(x, y, z); }           // PC 0x4C1D44 (PC only)
