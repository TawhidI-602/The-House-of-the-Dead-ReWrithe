// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): kinds 66-68, the
// bats (enemy_bats.cpp).
    // --- kinds 66-68, the bats (enemy_bats.cpp) ---
    void bat_call(uint32_t routine, uint32_t obj);                             // callx through their tables
    void kind66_bat(uint32_t obj);                                             // 0x83330
    void kind67_bat_swarm(uint32_t obj);                                       // 0x83350
    void kind68_bat(uint32_t obj);                                             // 0x83370
    void bat_hit_check(uint32_t obj);                                          // inline in both frames
    void bat_frame(uint32_t obj);                                              // 0x83390
    void bat_st1_hanging(uint32_t obj);                                        // 0x83590
    void bat_st2_take_off(uint32_t obj);                                       // 0x83660
    void bat_wing(uint32_t obj);                                               // inline
    void bat_bob(uint32_t obj);                                                // inline
    void bat_circle_step(uint32_t obj);                                        // inline in states 3 / 9
    void bat_st3_circle(uint32_t obj);                                         // 0x837C0
    void bat_dive_point(uint32_t obj);                                         // inline in states 4 / 10
    void bat_fly_at(uint32_t obj, float tx, float ty, float tz, const float* speed, float* d);   // inline
    float bat_distance(uint32_t obj, float x, float y, float z);              // inline (coprocessor 0x1F)
    void bat_st4_dive(uint32_t obj);                                           // 0x83A30
    void bat_st5_shot(uint32_t obj);                                           // 0x84180
    void bat_st6_fall(uint32_t obj);                                           // 0x843B0
    void bat_setup_fly(uint32_t obj, uint32_t rz);                             // the body of set-ups 0 / 4
    void bat_setup_flying(uint32_t obj);                                       // 0x844A0
    void bat_setup_group5();                                                   // 0x84590
    void bat_setup_flying_low(uint32_t obj);                                   // 0x846B0
    void bat_setup_hangedman_swarm();                                          // 0x84790
    void bat_swarm_frame(uint32_t obj);                                        // 0x84940
    void bat_st9_swarm_circle(uint32_t obj);                                   // 0x84A80
    void bat_st10_swarm_dive(uint32_t obj);                                    // 0x84CD0
    float pc_sin2(uint32_t angle);                                             // PC 0x4E7F1C (PC only)
    float pc_cos2(uint32_t angle);                                             // PC 0x4E7F3C (PC only)
    float pc_fast_sqrt8(float x);                                              // PC 0x4E7F5C (PC only)
    void pc_vec_angles7(float x, float y, float z, uint32_t& yaw, uint32_t& pitch);   // PC 0x4E7808 (PC only)
