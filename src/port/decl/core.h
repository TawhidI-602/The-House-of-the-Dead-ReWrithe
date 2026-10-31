// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): the core systems
// (shooting, damage, enemy, enemy_kinds).
    // --- shooting (shooting.cpp) ---
    void player_shoot(uint32_t task);                                          // 0x5A80
    void shot_ray_setup(int player, float x, float y, float focal);            // 0x32CF0
    void player_shoot_pc(uint32_t task);                                       // PC 0x4365C4: player_shoot's PC form
    int ray_sign_test(int player, float x, float y, float z, float radius);   // 0x33190
    void shot_resolve();                                                       // 0x454E0
    void shot_test_object(uint32_t index, int player);                         // 0x45620
    void shot_test_pairs(int player);                                          // 0x45700
    void shot_test_part(uint32_t part, int player);                            // 0x45730
    void shot_test_body(uint32_t obj, int player);                             // 0x45810
    void shot_test_list_a(int player);                                         // 0x45850
    void shot_test_list_b(int player);                                         // 0x45B60
    void shot_hit_nearest(int player);                                         // 0x45E70
    void shot_targets_sort();                                                  // 0x466D0
    float pc_fast_sqrt(float x);                                               // PC 0x4FC76C (PC only)
    float pc_fast_sqrt2(float x);                                              // PC 0x41347C (PC only)
    float pc_fast_sqrt3(float x);                                              // PC 0x4D7934 (PC only)
    int32_t pc_atan2(float a, float b);                                        // PC 0x568F10 (PC only)
    void pc_vec_angles(float x, float y, float z, uint32_t& yaw, uint32_t& pitch);   // PC 0x412D28 (PC only)
    void pc_matrix_push();                                                     // PC 0x569600 (PC only)
    void pc_matrix_transform(float x, float y, float z, float& ox, float& oy, float& oz);   // PC 0x412C40 (PC only)

    // --- damage (damage.cpp) ---
    uint32_t rand();                                                           // 0x34A50
    void add_score(int player, int32_t points);                                // 0x69B0
    int32_t difficulty_get();                                                  // 0x264E0
    void enemy_part_hits(uint32_t obj);                                        // 0x684B0
    void part_hit(uint32_t part);                                              // 0x68010

    // --- enemies (enemy.cpp) ---
    void enemy_main(uint32_t obj);                                             // 0x77410
    void enemy_leave(uint32_t obj);                                            // PC 0x4D9694 (arcade: inline)
    void kind10_remove(uint32_t obj);                                  // PC 0x4D95F8 (arcade: inline)
    float enemy_distance(float ax, float az, float bx, float bz);
    void enemy_turn_a(uint32_t obj, bool backwards, bool atLeast4 = false);
    void enemy_turn_pc(uint32_t obj, bool backwards, bool atLeast4);          // inline in the PC's enemy_ai / kind11_leap                           // inline in enemy_ai / kind11_leap (A)
    void kind11_leap_move(uint32_t obj);                                       // 0x76AE0 (the enemy target update)
    // --- enemy kinds (enemy_kinds.cpp) ---
    void enemy_counts_add(int d);                                              // helpers (inline in the original)
    void enemy_take_ai_at_once(uint32_t obj);
    void kind_hits(uint32_t obj);
    void kind_draw(uint32_t obj);
    uint32_t kind_list_motion(uint32_t obj, int32_t sub);
    void kind1_zombie_wake(uint32_t obj);                                      // 0x92D30
    void kind1_zombie_wake_wait(uint32_t obj);                                 // 0x92DC0
    void kind1_zombie_fight(uint32_t obj);                                     // 0x92E10
    void kind2_zombie_kick(uint32_t obj);                                      // 0x92EE0
    void kind2_zombie_kick_wait(uint32_t obj);                                 // 0x92F50
    void kind2_zombie_fight(uint32_t obj);                                     // 0x93030
    void kind6_zombie_stab(uint32_t obj);                                      // 0x93930
    void kind6_zombie_stab_frame(uint32_t obj);                                // 0x939A0
    void kind6_attack(uint32_t obj);                                           // 0x93A10
    void kind6_death_check(uint32_t obj);                                      // 0x93B40
    void kind10_zombie_scene(uint32_t obj);                                    // 0x941E0
    void kind10_zombie_scene_wait(uint32_t obj);                               // 0x94290
    void kind003_zombie_jump_down(uint32_t obj);                               // 0x93100
    void kind003_zombie_jump_down_frame(uint32_t obj);                         // 0x931A0
    void kind004_zombie_turn_walk(uint32_t obj);                               // 0x935A0
    void kind004_zombie_turn_walk_frame(uint32_t obj);                         // 0x93630
    bool kind_spawn_walk(uint32_t obj, uint32_t set2, uint32_t set6, uint32_t m8);   // inline in kinds 0 / 9
    void kind000_enemy_spawn(uint32_t obj);                                    // 0x77110
    void kind009_enemy_spawn_b(uint32_t obj);                                  // 0x94040
    void kind008_enemy_rise(uint32_t obj);                                     // 0x93EC0
    void kind008_enemy_rise_frame(uint32_t obj);                               // 0x93FA0
    void kind012_zombie_above(uint32_t obj);                                   // 0xA1900
    void kind012_zombie_above_frame(uint32_t obj);                             // 0xA19D0
    void kind012_zombie_above_attack(uint32_t obj);                            // 0xA1A40
    void kind5_zombie_hidden(uint32_t obj);                                    // 0x93720
    void kind5_zombie_hidden_wait(uint32_t obj);                               // 0x937A0
    void kind5_zombie_fight(uint32_t obj);                                     // 0x93860
    void kind7_zombie_drop(uint32_t obj);                                      // 0x93BE0
    void kind7_zombie_drop_frame(uint32_t obj);                                // 0x93CC0
    void kind7_falling_object(uint32_t obj);                                   // 0x93DB0
    void kind7_falling_piece(uint32_t obj);                                    // 0x942D0
    void kind11_ebita_leap(uint32_t obj);                                      // 0xA13D0
    void kind11_ebita_leap_frame(uint32_t obj);                                // 0xA1550
    void kind11_leap(uint32_t obj);                                            // 0xA15C0
