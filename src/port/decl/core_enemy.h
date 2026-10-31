// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): the core enemy
// systems (enemy_ai, enemy_actions, enemy_guard) and PC helpers.
    // an object field by its arcade offset (ob() gives the version's)
    template <class T> T& of(uint32_t obj, uint32_t a) { return mem.at<T>(obj + ob(a)); }
    void pc_matrix_rotate_y2(uint32_t angle);                                  // PC 0x503B64 (PC only)              // floor distance (A copro, PC table sqrt)
    int32_t pc_angle_turn(int32_t angle, uint32_t target, float k, float slow);   // PC 0x4D8D7C (PC only)
    float pc_vec_cos(float a0, float a1, float a2, float b0, float b1, float b2);  // PC 0x4D7A0C (PC only)
    void pc_matrix_identity();                                                 // PC 0x4D7B4C (PC only)
    void pc_matrix_rotate_y(uint32_t angle);                                   // PC 0x4D74FC (PC only)
    void pc_matrix_transform2(float x, float y, float z, float& ox, float& oy, float& oz);   // PC 0x4D70F8
    void pc_vec_angles2(float x, float y, float z, uint32_t& yaw, uint32_t& pitch);       // PC 0x4D71E0
    void enemy_footsteps(uint32_t obj);                                        // 0x78F50
    void enemy_hit_reaction(uint32_t obj);                                     // 0x777F0
    void enemy_check_death(uint32_t obj);                                      // 0x778C0
    void enemy_voice(uint32_t obj, int kind);                                  // 0x6BF30
    uint32_t return_false();                                                   // 0x27500
    void enemy_ai_motion(uint32_t obj);                                        // 0x78B70
    void enemy_ai(uint32_t obj);                                               // 0x77B20
    void enemy_guard(uint32_t obj);                                            // 0x550A0
    void enemy_frame_begin(uint32_t obj);                                      // 0x1D820
    void enemy_frame_end(uint32_t obj);                                        // 0x1D850
    void enemy_act1_scenery(uint32_t obj);                                     // 0x55FA0
    void enemy_act2_approach_target(uint32_t obj);                             // 0x55F40
    void enemy_act3_start_variant1(uint32_t obj);                              // 0x55CF0
    void enemy_act4_inert(uint32_t obj);                                       // 0x560E0
    void enemy_act5_wait_stop(uint32_t obj);                                   // 0x55DE0
    void enemy_act12_walk_to_spot(uint32_t obj);                               // 0x55EC0
    void ZombieWalk2Tom(uint32_t obj);                                         // 0x56020
    void ENGuardOpeningDummy(uint32_t obj);                                    // 0x56250
    void ENGuardZombieRunAfterResearcher(uint32_t obj);                        // 0x53EA0
    void ENGuardZombieRunAfterResearcher2(uint32_t obj);                       // 0x54090
    void ENGuardZombieRunAfterResearcher_St3_00(uint32_t obj);                 // 0x54290
    void enemy_act16_frame(uint32_t obj);                                      // 0x541A0
    void enguard_scripted_frame(uint32_t obj);                                 // 0x53910
    void ENGuardZombieBiteAgent(uint32_t obj);                                 // 0x53AA0
    void enemy_act12_frame(uint32_t obj);                                      // 0x55E50
    void enemy_frame_end_ch3_ev0(uint32_t obj);                                // 0x560F0
    void enemy_act16_frame_b(uint32_t obj);                                    // 0x56120
    void opening_dummy_walk(uint32_t obj);                                     // 0x561A0
    void enemy_act15_attack_researcher(uint32_t obj);                          // 0x54D60
    void enemy_act6_attack_researcher(uint32_t obj);                           // 0x543B0
    void enemy_act14_attack_researcher(uint32_t obj);                          // 0x54960
    void enemy_act17_attack_researcher(uint32_t obj);                          // 0x54B40
    void enemy_act7_chainsaw_attack(uint32_t obj);                             // 0x54720
    void elevator_ride(uint32_t obj);                                          // 0x53810
    void enemy_act7_frame(uint32_t obj);                                       // 0x54540
    void enemy_act7_frame_b(uint32_t obj);                                     // 0x54FE0
