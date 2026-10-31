// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): kinds 33-39, the
// Murrer (enemy_molb.cpp).
    // --- the Murrer (enemy_molb.cpp) ---
    void attack_queue_shift(int32_t i);                                        // 0x64C70
    void attack_queue_push_front(uint32_t obj);                                // 0x64CD0
    void kind34_molb_group(uint32_t obj);                                      // 0x64BB0
    void kind34_molb_group_idle(uint32_t obj);                                 // 0x64C30
    float molb_floor_level(uint32_t obj, float floor);                         // the floor modes' level
    float molb_floor_y(uint32_t obj, float ground);                            // inline (the type's floor height)
    void kind033_murrer_reset(uint32_t obj);                                   // 0x64C40
    void kind34_molb_member(uint32_t obj);                                     // 0x64E10
    void kind34_molb_member_wait(uint32_t obj);                                // 0x64F00
    void kind34_molb_member_wake(uint32_t obj);                                // 0x64FC0
    void kind34_molb_member_delay(uint32_t obj);                               // 0x65010
    void kind35_molb_crawl_start(uint32_t obj);                                // 0x65050
    void kind35_molb_follow_path(uint32_t obj);                                // 0x65230
    void molb_lunge_setup(uint32_t obj);                                       // 0x652A0's body (inline in 0x658E0, 0x66000)
    void kind35_molb_lunge_start(uint32_t obj);                                // 0x652A0
    void kind35_molb_lunge_wait(uint32_t obj);                                 // 0x65330
    void kind35_molb_leap_decide(uint32_t obj);                                // 0x65360
    void kind35_molb_leap(uint32_t obj);                                       // 0x65410
    void kind35_molb_leap_hit(uint32_t obj);                                   // 0x65450
    void kind35_molb_leap_hold(uint32_t obj);                                  // 0x65490
    void kind35_molb_drop_start(uint32_t obj);                                 // 0x654D0
    void kind35_molb_drop(uint32_t obj);                                       // 0x656A0
    void molb_leap_spot_free(uint32_t obj);                                    // inline (5 places)
    void kind35_molb_landed(uint32_t obj);                                     // 0x656D0
    void kind35_molb_menace(uint32_t obj);                                     // 0x65740
    void kind35_molb_regroup(uint32_t obj);                                    // 0x65770
    void kind35_molb_regroup_turn(uint32_t obj);                               // 0x657D0
    void kind35_molb_regroup_crawl(uint32_t obj);                              // 0x65810
    void kind35_molb_regroup_crawl_wait(uint32_t obj);                         // 0x65860
    void kind35_molb_face_camera(uint32_t obj);                                // 0x658A0
    void kind35_molb_face_turn(uint32_t obj);                                  // 0x658E0
    void kind35_molb_killed(uint32_t obj);                                     // 0x659A0
    void kind35_molb_fall(uint32_t obj);                                       // 0x65A70
    void kind35_molb_dead_wait(uint32_t obj);                                  // 0x65AD0
    void kind35_molb_remove(uint32_t obj);                                     // 0x65B60
    void kind35_molb_frame(uint32_t obj);                                      // 0x65BC0
    void kind35_molb_turn_start(uint32_t obj);                                 // 0x65C80
    void kind35_molb_murrer(uint32_t obj);                                     // 0x65D10
    void kind35_molb_wait_stop(uint32_t obj);                                  // 0x65E10
    void kind35_molb_delay(uint32_t obj);                                      // 0x65F10
    void kind36_molb_murrer_attack(uint32_t obj);                              // 0x66000
    void kind39_molb_murrer_jump(uint32_t obj);                                // 0x66150
    void kind39_jump_wait(uint32_t obj);                                       // 0x66240
    void kind39_crawl_floor(uint32_t obj);                                     // 0x66290
    void kind39_jump_start(uint32_t obj);                                      // 0x662F0
    void kind39_jump(uint32_t obj);                                            // 0x66310
    void kind39_molb_dying(uint32_t obj);                                      // 0x664A0
    void kind39_molb_frame(uint32_t obj);                                      // 0x66540
    void molb_leap_target_pick(uint32_t obj, uint32_t out);                    // 0x66680
