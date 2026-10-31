// Declarations of class Game, included inside its body (port/game.h): the players' character bodies in the
// cut-scenes (player_body.cpp, R4). Arcade forms only. Proven 2026-10-16 (proofs-2, docs/proof.md "Support routines
// (2)") except the ones marked UNVERIFIED (not reached in the captures).
    // --- the players' character bodies (player_body.cpp) ---
    void player_char_models_set();                                            // 0x95530
    void player_body_at(uint32_t task, uint32_t offset);                      // the common part of 0x4B850 / 0x4B980
    void player_body_at_camera(uint32_t task);                                // 0x4B850
    void player_body_place(uint32_t task);                                    // 0x4B980 (proofs-10: p10_gover, a player game over in a 2-player game)
    void player_body_draw_camdr(uint32_t task);                               // 0x6240
    void player_body_camdr(uint32_t task);                                    // 0x62C0 (proofs-10: p10_gover)
    void player_body_hold(uint32_t task);                                     // 0x6330 (proofs-10: p10_gover)
    void player_scene00_kousya_run(uint32_t task);                            // 0x4BAB0
    void player_scene00_kousya_run_p(uint32_t task);                          // 0x4BAF0
    void player_scene00_kousya_run_p_p1(uint32_t task);                       // 0x4BBA0
    void player_scene00_kousya_run_p_p1_p(uint32_t task);                     // 0x4BBE0
    void player_scene00_turn(uint32_t task);                                  // 0x4BD00 (unnamed; proofs-11: p11_s00)
    void player_scene00_turn_end(uint32_t task);                              // 0x4BD70 (unnamed; proofs-11: p11_s00)
    void player_scene00_hold(uint32_t task);                                  // 0x4BDB0 (unnamed; proofs-11: p11_s00)
    void player_scene02_shoot_three(uint32_t task);                           // 0x4C0D0
    void player_scene02_shoot_three_p(uint32_t task);                         // 0x4C110
    void player_scene02_shoot_three_p_sub(uint32_t task);                     // 0x4BFB0
    void player_scene02_shoot_three_p_sub_p(uint32_t task);                   // 0x4C210
    void player_scene02_shoot_three_p_sub_p_p(uint32_t task);                 // 0x4C280
    void player_scene02_shoot_three_p_sub_p_p_p(uint32_t task);               // 0x4C320
    void player_scene08_finish_pose(uint32_t task);                           // 0x4CEE0
    void player_scene08_finish_pose_p(uint32_t task);                         // 0x4CFB0
    void player_scene08_finish_pose_p_p(uint32_t task);                       // 0x4CFF0
    void player_scene08_finish_pose_p_p_p(uint32_t task);                     // 0x4D050
    void player_scene08_finish_pose_p_p_p_p(uint32_t task);                   // 0x4D090
    uint32_t player_scene_start(uint32_t task, const uint32_t cam[6], uint32_t table);   // the scenes' common start (T1)
    void player_scene03_ladder(uint32_t task);                                // 0x4C3A0
    void player_scene03_ladder_p1(uint32_t task);                             // 0x4C500
    void player_scene04_fall_backside(uint32_t task);                         // 0x4C550
    void player_scene04_fall_backside_p2(uint32_t task);                      // 0x4C6C0
    void player_scene05_knocked_flying(uint32_t task);                        // 0x4C700
    void player_scene05_knocked_flying_p(uint32_t task);                      // 0x4C860
    void player_scene05_knocked_flying_p_p(uint32_t task);                    // 0x4C8E0 UNVERIFIED (teaser)
    void player_scene05_knocked_flying_p_p_p(uint32_t task);                  // 0x4C960 UNVERIFIED (teaser)
    void player_scene06_reload_shoot(uint32_t task);                          // 0x4C980
    void player_scene06_reload_shoot_p1(uint32_t task);                       // 0x4CAD0
    void player_scene06_reload_shoot_p2(uint32_t task);                       // 0x4CB70
    void player_scene06_reload_shoot_p2_p(uint32_t task);                     // 0x4CBB0
    void player_scene07_jump_down_a(uint32_t task);                           // 0x4CC00
    void player_scene07_jump_down_a_p1(uint32_t task);                        // 0x4CD60
    void player_scene07_jump_down_a_p1_p(uint32_t task);                      // 0x4CDA0
    void player_scene07_jump_down_a_p1_p_p(uint32_t task);                    // 0x4CDF0
    void player_scene07_jump_down_a_p1_p_p_p(uint32_t task);                  // 0x4CE70 UNVERIFIED (teaser)
    // Chapter 2's scenes 09-14 (ch2proof)
    void player_scene09_stance_shoot(uint32_t task);                          // 0x4D0B0
    void player_scene09_stance_shoot_p1(uint32_t task);                       // 0x4D1A0
    void player_scene09_stance_shoot_p1_p(uint32_t task);                     // 0x4D240
    void player_scene09_stance_shoot_p1_p_p(uint32_t task);                   // 0x4D280
    void player_scene09_stance_shoot_p1_p_p_p(uint32_t task);                 // 0x4D2C0
    void player_scene_jump_down(uint32_t task, const uint32_t cam[6], uint32_t table, uint32_t y, int32_t wait_facing);  // 10 / 11
    void player_scene10_jump_down_b(uint32_t task);                           // 0x4D2F0
    void player_scene11_jump_down_c(uint32_t task);                           // 0x4D450
    void player_scene12_reload_kick(uint32_t task);                           // 0x4D5C0
    void player_scene13_knocked_flying_b(uint32_t task);                      // 0x4D710
    void player_scene14_rattle_door(uint32_t task);                           // 0x4D870
    void player_scene14_rattle_door_p(uint32_t task);                         // 0x4D9B0
    void player_scene14_rattle_door_p_p(uint32_t task);                       // 0x4DA20
