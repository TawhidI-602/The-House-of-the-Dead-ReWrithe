// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): kind 65, the
// scene axe zombie (enemy_scene_zombie.cpp).
    // --- kind 65, the scene axe zombie (enemy_scene_zombie.cpp) ---
    uint32_t yaw_toward_a(float dx, float dy, float dz);                       // coprocessor 0x0F + fold (inline)
    void kind65_zombie_scene(uint32_t obj);                                    // 0x8A400
    void kind65_call(uint32_t routine, uint32_t obj);                          // callx through its tables
    void kind65_body(uint32_t obj, bool slotUnless14);                         // inline in the set-ups
    void kind65_frame(uint32_t obj);                                           // 0x8A4F0
    void kind65_hit_check(uint32_t obj);                                       // 0x8A570
    uint32_t kind65_hit_motion(int32_t hitClass);                              // inline
    void kind65_st2_damage(uint32_t obj);                                      // 0x8A680
    void kind65_st3_die(uint32_t obj);                                         // 0x8A830
    void kind65_walk_start(uint32_t obj);                                      // inline
    void kind65_st4_walk(uint32_t obj);                                        // 0x8AA00
    void kind65_st5_approach(uint32_t obj);                                    // 0x8AB60
    void kind65_st6_pick_target(uint32_t obj);                                 // 0x8AD90
    void kind65_st7_attack(uint32_t obj);                                      // 0x8AE20
    void kind65_to_enemy(uint32_t obj, int16_t action);                        // inline
    void kind65_chop_sound(uint32_t obj, uint32_t flag);                       // inline
    bool kind65_chase(uint32_t obj, float reach);                              // inline
    void kind65_scene0_walk_chop(uint32_t obj);                                // 0x8B040
    void kind65_scene1_stance_chop(uint32_t obj);                              // 0x8B2E0
    void kind65_scene2_pull_out(uint32_t obj);                                 // 0x8B470
    void kind65_scene3_dash_chop(uint32_t obj);                                // 0x8B8F0
    void kind65_scene4_pull_out(uint32_t obj);                                 // 0x8BCC0
