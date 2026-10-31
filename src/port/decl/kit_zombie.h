// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): kind 32, the
// zombies with a kit (enemy_kit_zombie.cpp and its .inc files).
    // --- kind 32, the zombies with a kit (enemy_kit_zombie.cpp) ---
    void kind32_call(uint32_t routine, uint32_t obj);                          // a per-frame routine set and called
    void kind32_zombie_kit(uint32_t obj);                                      // 0x7EEB0
    void kind32_act2_stance(uint32_t obj);                                     // 0x7EFC0
    void kind32_act2_rise(uint32_t obj);                                       // 0x7F040
    void kind32_act2_rise_frame(uint32_t obj);                                 // 0x7F0A0
    void kind32_act2_hidden(uint32_t obj);                                     // 0x7F100
    void kind32_act2_hidden_frame(uint32_t obj);                               // 0x7F170
    void kind32_act2_walk_in(uint32_t obj);                                    // 0x7F1A0
    void kind32_act2_walk_in_frame(uint32_t obj);                              // 0x7F310
    void kind32_act2_ready(uint32_t obj);                                      // 0x7F3B0
    void kind32_after_throw(uint32_t obj);                                     // inline (record +0xC bits 6-8)
    uint32_t kind32_bare_hands(uint32_t obj);                                  // inline
    void kind32_throw_wait(uint32_t obj);                                      // 0x7F400
    void kind32_throw_ready_start(uint32_t obj);                               // 0x7F560
    void kind32_throw_ready(uint32_t obj);                                     // 0x7F580
    void kind32_axe_throw(uint32_t obj);                                       // 0x7F5A0
    void kind32_axe_throw_frame(uint32_t obj);                                 // 0x7F5D0
    void kind32_walk_off_call(uint32_t obj);                                   // 0x7F700
    void kind32_leave(uint32_t obj);                                           // 0x7F710
    void kind32_walk_off(uint32_t obj);                                        // 0x7F770
    void kind32_walk_off_frame(uint32_t obj);                                  // 0x7F840
    void kind32_after_throw_walk(uint32_t obj);                                // 0x7F900
    void kind32_after_throw_walk_n(uint32_t obj);                              // 0x7F970
    void obj_velocity_zero(uint32_t obj);                                      // 0x7F9D0
    void kind32_act2_after_hit(uint32_t obj);                                  // 0x7F9F0
    void kind32_act2_hit_wait(uint32_t obj);                                   // 0x7FA10
    void kind32_hide_pose(uint32_t obj, int blend);                            // 0x7FA40
    void kind32_axe_throw_row(uint32_t obj, uint32_t out);                     // 0x7FAE0
    void kind32_act3_drum(uint32_t obj);                                       // 0x7FBA0
    void kind32_carry_wait_start(uint32_t obj);                                // 0x7FC70
    void kind32_carry_wait(uint32_t obj);                                      // 0x7FC90
    void kind32_throw_start(uint32_t obj);                                     // 0x7FCD0
    void kind32_throw(uint32_t obj);                                           // 0x7FD40
    uint32_t kind32_stance_motion();                                           // inline
    void kind32_throw_end(uint32_t obj);                                       // 0x7FD90
    void kind32_throw_end_wait(uint32_t obj);                                  // 0x7FDE0
    void kind32_stand_down(uint32_t obj);                                      // inline (3 places)
    void kind32_after_throw_stance(uint32_t obj);                              // 0x7FE20
    void kind32_after_throw_stance_frame(uint32_t obj);                        // 0x7FEB0
    void kind32_throw_done(uint32_t obj);                                      // 0x7FF30
    void kind32_act3_after_hit(uint32_t obj);                                  // 0x7FF40
    void kind32_act3_hit_wait(uint32_t obj);                                   // 0x7FFC0
    void kind32_act3_recover(uint32_t obj);                                    // 0x7FFF0
    void kind32_carry_resume(uint32_t obj);                                    // 0x80100
    void kind32_hit_react(uint32_t obj);                                       // 0x80120
    void kind32_hits(uint32_t obj);                                            // 0x80240
    uint32_t kind32_hit_reaction_done(uint32_t obj, uint32_t next);            // 0x80510
    void kind32_act2_rise_start(uint32_t obj);                                 // 0x83270
    void kind32_deactivate(uint32_t obj);                                      // 0x82F00
    float pc_fast_sqrt9(float x);                                              // PC 0x4E22A8 (PC only)
    bool kind32_motion_near_end(uint32_t obj);                                 // inline
    uint32_t iron_ball_param(int i);                                           // inline (the PC packs the table)
    virtual void kind32_act4_iron_ball(uint32_t obj);                          // 0x46F60 (virtual: older captures recorded it as a boundary)
    void kind32_stance_wait(uint32_t obj, int how, uint32_t motion, int32_t delay);   // 0x470C0
    void kind32_stance_wait_count(uint32_t obj);                               // 0x47130
    void kind32_stance_wait_swing(uint32_t obj);                               // 0x47170
    void kind32_act4_swing_again(uint32_t obj, int how);                       // 0x47200
    void kind32_act4_swing_start_frame(uint32_t obj);                          // 0x47260
    void kind32_walk_start(uint32_t obj, int how, uint32_t motion, int32_t delay);    // 0x472D0
    void kind32_walk(uint32_t obj);                                            // 0x47350
    void kind32_walk_turn(uint32_t obj);                                       // 0x474B0
    void kind32_act4_swing_set(uint32_t obj, int how, int start, int32_t delay);      // 0x47530
    void iron_ball_swing_wait(uint32_t obj);                                   // 0x475A0
    void iron_ball_swing_start(uint32_t obj, int how, int start, int32_t delay);      // 0x47630
    void iron_ball_swing(uint32_t obj);                                        // 0x476D0
    void iron_ball_pull_back(uint32_t obj);                                    // 0x47750
    void iron_ball_attack(uint32_t obj);                                       // 0x477D0
    void kind32_act4_after_hit(uint32_t obj);                                  // 0x478E0
    virtual void kind32_act4_recover(uint32_t obj);                            // 0x47950 (virtual: older captures recorded it as a boundary)
    void kind32_act4_after_hit_frame(uint32_t obj);                            // 0x479D0
    void iron_ball_hit_start(uint32_t obj);                                    // 0x47A00
    void iron_ball_hit_wait(uint32_t obj);                                     // 0x47A40
    void iron_ball_hit_wait_end(uint32_t obj);                                 // 0x47B00
    void iron_ball_task(uint32_t obj);                                         // 0x47B60
    virtual void kind32_part_hits(uint32_t obj);                               // 0x68630 (virtual: older captures recorded it as a boundary)
    virtual void kind32_axe_release(uint32_t obj, uint32_t row);               // 0x80900 (virtual: older captures recorded it as a boundary)
    // --- kind 32's held objects (enemy_kit_held.inc) ---
    void held_call(uint32_t routine, uint32_t obj);                             // callx through the kit tables
    void held_call_f(uint32_t routine, uint32_t obj, uint32_t v);               // callx (object, float)
    int32_t pc_atan2_8(float a, float b);                                      // PC 0x4E1AD8 (PC only)
    void pc_vec_angles8(float x, float y, float z, uint32_t& yaw, uint32_t& pitch);   // PC 0x4E1B54 (PC only)
    void held_vec_angles(uint32_t obj, uint32_t& a0, uint32_t& a1);           // inline
    void held_uncount();                                                       // inline
    int32_t held_hit(uint32_t obj, uint32_t kr, uint32_t box);                 // inline
    bool held_near(uint32_t obj, uint32_t a);                                  // inline
    void held_reach_camera(uint32_t obj, uint32_t kr, uint32_t point);         // inline
    void flying_axe_start(uint32_t obj);                                        // 0x83230
    void flying_axe_launch(uint32_t obj);                                       // 0x81300
    void held_object_launch_arc(uint32_t obj);                                  // 0x813D0
    void held_object_launch_random(uint32_t obj);                               // 0x814E0
    void held_object_launch_drop(uint32_t obj);                                 // 0x815E0
    void flying_axe_frame(uint32_t obj);                                        // 0x81670
    void flying_axe_shot(uint32_t obj);                                         // 0x81890
    void flying_axe_shot_fall(uint32_t obj);                                    // 0x81920
    void flying_axe_spin(uint32_t obj);                                         // 0x80840
    void held_object_flight_spin(uint32_t obj);                                 // 0x80C80
    void held_object_knocked_away(uint32_t obj);                                // 0x805A0
    void held_object_knocked_away_fall(uint32_t obj);                           // 0x80700
    void held_object_start(uint32_t obj);                                       // 0x80F30
    void held_object_frame(uint32_t obj);                                       // 0x81030
    void held_object_throw_at_camera(uint32_t obj);                             // 0x81990
    void held_object_throw_flight(uint32_t obj);                                // 0x81A10
    int32_t held_object_shot_check(uint32_t obj);                               // 0x81DB0
    void held_turn(uint32_t obj, bool aboutY, uint32_t angle);                 // inline
    void held_kit_drop(uint32_t obj);                                          // 0x820B0 = 0x82400 (the same code)
    void held_object_carried(uint32_t obj);                                     // 0x81F30
    void held_kit_drop1(uint32_t obj);                                          // 0x820B0
    void held_kit_drop2(uint32_t obj);                                          // 0x82400
    void held_kit_piece_bounce(uint32_t obj);                                   // 0x82320
    void held_object_release(uint32_t obj);                                     // 0x82670
    void held_object_flight(uint32_t obj);                                      // 0x82790
    void held_object_draw(uint32_t obj);                                        // 0x82C30
    void held_kit_barrel_drop(uint32_t obj);                                    // 0x82E30
    void held_kit_barrel_breakup(uint32_t obj);                                 // 0x82E80
    void hit_spark_spawn(uint32_t obj, uint32_t size);                          // 0x82FE0
