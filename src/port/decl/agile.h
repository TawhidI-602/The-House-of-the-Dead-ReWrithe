// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): kind 70, the
// agile creatures (enemy_agile.cpp), and the boundary stubs only kind 70 calls.
// The virtual ones were boundaries in older captures: the proof replays them where a capture recorded them so.
    // --- kind 70, the agile creatures (enemy_agile.cpp) ---
    void kind70_call(uint32_t routine, uint32_t obj);                          // callx through kind70_setups / _states
    void obj_stop(uint32_t obj);                                               // 0x2FF70
    void kind70_saru_saruzou(uint32_t obj);                                    // 0x8BEC0
    void kind70_spawn_init(uint32_t obj);                                      // 0x8BF10
    void kind70_spawn12(uint32_t obj);                                         // 0x8BFC0
    void kind70_st00_none(uint32_t obj);                                       // 0x8C080
    void kind70_frame(uint32_t obj);                                           // 0x8C090
    void kind70_ch1_route_check(uint32_t obj);                                 // 0x8C1A0
    void kind70_hits(uint32_t obj);                                            // 0x8C230
    void kind70_hit_reaction(uint32_t obj);                                    // 0x8C420
    void kind70_after_hit_state(uint32_t obj);                                 // 0x92D10
    float pc_floor_distance10(float ax, float az, float bx, float bz);         // PC 0x4FBFB0 (PC only)
    void pc_vec_angles10(float x, float y, float z, uint32_t& yaw, uint32_t& pitch);   // PC 0x4FC018 (PC only)
    float kind70_camera_distance(uint32_t obj);                                // inline (coprocessor 0x1F / PC table)
    void kind70_face(uint32_t obj, float dx, float dy, float dz);              // inline (coprocessor 0x0F / PC)
    void kind70_st02_sub(uint32_t obj);                                        // 0x2F730
    void kind70_st02_approach(uint32_t obj);                                   // 0x8D190
    void kind70_st03_think(uint32_t obj);                                      // 0x8D310
    void kind70_think_monkey(uint32_t obj);                                    // 0x8D7A0
    void kind70_think_knife(uint32_t obj, bool kageta);                        // Drake / kageta
    void kind70_wall_move(uint32_t obj, bool swap, uint32_t motion);           // states 6 / 7
    void kind70_st06_wall_x(uint32_t obj);                                     // 0x8E260
    void kind70_st07_wall_z(uint32_t obj);                                     // 0x8E340
    void kind70_leap(uint32_t obj, bool cry, int16_t next);                    // states 8 / 27
    void kind70_st08_leap(uint32_t obj);                                       // 0x8E420
    void kind70_st09_turn(uint32_t obj);                                       // 0x8E530
    void kind70_st10_stance(uint32_t obj);                                 // 0x8E5A0
    void kind70_st04_wall(uint32_t obj);                                   // 0x8E790
    void kind70_wall_attack_target(uint32_t obj);                          // 0x92C40
    void kind70_face_camera(uint32_t obj);                                     // inline
    void kind70_st05_bite(uint32_t obj);                                   // 0x8E9C0
    void kind70_st17_bite_b(uint32_t obj);                                 // 0x8EEE0
    // room_ceiling_bounce 0x2FD30 / impact_effect_spawn 0x30C90: ported in ch3_kinds.cpp (decl/ch3_kinds.h, worker "ch3own")
    virtual void pc_matrix_translate10(float x, float y, float z) { pmx_translate(x, y, z); }   // PC 0x4FC114 (PC only, kind 69 / 70 file)
    virtual void pc_matrix_rotate_y10(uint32_t a) { pmx_rotate_y(a); }                   // PC 0x4FC334 (PC only)
    virtual void pc_matrix_rotate_b10(uint32_t a) { pmx_rotate_z(a); }                   // PC 0x4FC43C (PC only)
    virtual void pc_matrix_rotate_x10(uint32_t a) { pmx_rotate_x(a); }                   // PC 0x4FC5C8 (PC only)
    virtual void pc_matrix_mul_stored10(uint32_t m) { (void)m; }                 // PC 0x4FCB84 (PC only)
    float kind70_ground(uint32_t obj);                                         // inline (ground_height on a local)
    void kind70_fall_start(uint32_t obj, float depth, bool level);             // inline (death / airborne hit)
    void kind70_fall_wall(uint32_t obj);                                       // inline
    bool kind70_fall_arc(uint32_t obj);                                        // inline
    int kind70_fall_floor(uint32_t obj, float& ground);                        // inline
    void kind70_hit_reaction_air(uint32_t obj);                            // 0x8C620
    void kind70_st01_death(uint32_t obj);                                  // 0x8CA40
    void kind70_st11_last(uint32_t obj);                                   // 0x8C4C0
    virtual void kind70_spawn0_jump_in(uint32_t obj);                      // 0x8F090
    void kind70_aim_point(uint32_t obj, float sx, float sy, float depth, uint32_t oneVar);   // inline (the leap / throw targets)
    uint32_t moody_held_model(uint32_t obj);                                   // inline (Moody: its object model)
    void kind70_leave(uint32_t obj, uint32_t x);                               // set-ups 0 / 1, steps 10-13
    void kind70_st27_wait(uint32_t obj);                                   // 0x91DE0
    void kind70_st26_none(uint32_t obj);                                   // 0x91DD0
    virtual void kind70_spawn6(uint32_t obj);                              // 0x8CEB0
    virtual void kind70_spawn5_waiting(uint32_t obj);                      // 0x8FC30
    virtual void kind70_spawn2_eating(uint32_t obj);                       // 0x8F990
    virtual void kind70_spawn8_wall_attack(uint32_t obj);                  // 0x8FF00
    void kind70_spawn1_jump_across(uint32_t obj);                          // 0x8F780
    void kind70_spawn7(uint32_t obj);                                      // 0x8FD90
    virtual void kind70_spawn10_stance_bite(uint32_t obj);                 // 0x909F0
    virtual void kind70_spawn11_hanging(uint32_t obj);                     // 0x91960
    void kind70_spawn3(uint32_t obj);                                      // 0x902F0
    virtual void kind70_think_rubin(uint32_t obj);                         // 0x8DC50
    virtual void kind70_think_moody(uint32_t obj);                         // 0x8DD80
    virtual void kind70_think_drake(uint32_t obj);                         // 0x8D960
    virtual void kind70_think_kageta(uint32_t obj);                        // 0x8DF40
    void kind70_st23_knife_throw(uint32_t obj);                            // 0x90D30
    void kind70_st24_roll(uint32_t obj);                                   // 0x90F20
    void kind70_st28_moody_wall(uint32_t obj);                             // 0x91F00
    void kind70_st29_moody_ceiling(uint32_t obj);                          // 0x92210
    void kind70_knife_spawn(uint32_t obj);                                 // 0x915C0
    void kind70_knife_fly_sub(uint32_t obj);                               // 0x914F0
    void kind70_knife_fly(uint32_t obj);                                   // 0x90FF0
    virtual void kind70_spawn9_fade_in(uint32_t obj);                      // 0x906C0
    void moody_projectile(uint32_t obj);                                   // 0x92380
    void moody_projectile_target(uint32_t obj);                            // 0x92930
    virtual void agile_hand_model_restore(uint32_t obj);                   // 0x92A00
    void moody_throw(uint32_t obj);                                        // 0x92A90
