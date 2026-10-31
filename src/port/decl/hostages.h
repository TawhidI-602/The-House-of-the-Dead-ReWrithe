// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): kind 100, the
// hostages, scenarios 0-6 (hostages.cpp, teaser L5). Arcade forms only.
    // --- kind 100, the hostages (hostages.cpp) ---
    void hostage_slot_free_inline(uint32_t obj);                               // inline (the body of attack_slot_free)
    bool hostage_motion_done(uint32_t obj);                                    // inline
    void hostage_count_rescue();                                               // inline
    void hostage_rescuer_score(uint32_t obj, int32_t points);                  // inline
    void hostage_lips32(uint32_t obj, uint32_t closed, uint32_t first);        // inline
    void hostage_shot_penalty(uint32_t obj, bool held = false);                // inline in hostage_shot / s03_held
    void pm_shot_bit_drop(uint32_t obj);                                      // PC 0x4AC932 / 0x4AD064 (PC Mode, Boss Mode)
    void hostage_fall_step(uint32_t obj);                                      // inline
    void hostage_call(uint32_t routine, uint32_t obj);                         // callx (scenarios, end routines)
    void hostage(uint32_t obj);                                                // 0x56460 (kind 100)
    void hostage_b(uint32_t obj);                                              // 0x56510 (kind 103)
    void hostage_s00_bystander(uint32_t obj);                                  // 0x56540
    void hostage_s00_frame(uint32_t obj);                                      // 0x565B0
    void hostage_s01_hands_up(uint32_t obj);                                   // 0x56630
    void hostage_s01_wait(uint32_t obj);                                       // 0x56660
    void hostage_s01_run(uint32_t obj);                                        // 0x566E0 (proofs-10: p10_s01)
    void hostage_s02_flee(uint32_t obj);                                       // 0x56810
    void hostage_s02_wait(uint32_t obj);                                       // 0x56840
    void hostage_s02_thank(uint32_t obj);                                      // 0x568B0
    void hostage_s03_carried(uint32_t obj);                                    // 0x568D0
    void hostage_s04_message(uint32_t obj);                                    // 0x56900
    void hostage_s04_wait(uint32_t obj);                                       // 0x56960
    void hostage_s04_rescue(uint32_t obj);                                     // 0x569F0
    void hostage_s05_held_walk(uint32_t obj);                                  // 0x56C90
    void hostage_s05_frame(uint32_t obj);                                      // 0x56D00
    void hostage_s06_hanging(uint32_t obj);                                    // 0x56EF0 (proofs-11: p11_s06)
    void hostage_s06_wait(uint32_t obj);                                       // 0x56F30 (proofs-11: p11_s06)
    void hostage_s06_thank(uint32_t obj);                                      // 0x56FC0 (proofs-11: p11_s06)
    uint32_t hostage_is_female(uint32_t obj);                                  // 0x5A020
    uint32_t hostage_list_humans();                                            // 0x5A050
    void hostage_s03_thank_motion(uint32_t obj);                               // 0x5A130
    void attack_slot_free(uint32_t obj);                                       // 0x5A1E0
    uint32_t hostage_camera_near(uint32_t obj);                                // 0x5A210
    void hostage_s12_leave(uint32_t obj);                                      // 0x5A290
    void hostage_thank_and_leave(uint32_t obj);                                // 0x5A2E0
    void hostage_s01_run_steps(uint32_t obj);                                  // 0x5A4B0 (proofs-10: p10_s01)
    void hostage_struck_mark(uint32_t task);                                   // 0x5A770
    void hostage_struck_fall(uint32_t obj);                                    // 0x5A860
    void hostage_scene_end_frame(uint32_t obj);                                // 0x5A970
    void hostage_s01_run_task(uint32_t obj);                                   // 0x5A9F0
    void hostage_s03_thrown(uint32_t obj);                                     // 0x5AAB0 UNVERIFIED (teaser): not reached in the l5 captures
    void hostage_s03_rescue(uint32_t obj);                                     // 0x5AC90
    void hostage_s03_start(uint32_t obj);                                      // 0x5AFD0
    void hostage_s03_held(uint32_t obj);                                       // 0x5AFE0
    void hostage_end_wait(uint32_t obj);                                       // 0x5B4A0
    void hostage_end_countdown(uint32_t obj);                                  // 0x5B4C0
    void hostage_end_remove(uint32_t obj);                                     // 0x5B500
    void hostage_end(uint32_t obj);                                            // 0x5B540
    void hostage_shot(uint32_t obj);                                           // 0x5B550
    void researcher_shot_by_player(uint32_t obj);                              // 0x5B7D0
    void hostage_struck(uint32_t obj);                                         // 0x5BA40
    uint32_t hostage_captor_gone(uint32_t obj);                                // 0x5BD30
    void hostage_struck_wait(uint32_t obj);                                    // 0x5C3A0
    void hostage_s03_dead(uint32_t obj);                                       // 0x5C460 (proofs-6: p6_def / p6_r5 / p6_2p)
    void hostage_s03_dropped(uint32_t obj);                                    // 0x5C4B0
    void hostage_thank_leave_step(uint32_t obj);                               // 0x5C770 (proofs-11: p11_s06)
    void hostage_shot_slide(uint32_t obj);                                     // 0x5C7C0 UNVERIFIED (teaser): not reached in the l5 captures
    void hostage_shot_fall(uint32_t obj);                                      // 0x5C870 UNVERIFIED (teaser): not reached in the l5 captures
    void hostage_gift_task(uint32_t task);                                     // 0x1E200
    void hostage_gift_fly(uint32_t task);                                      // 0x1E040
    void gift_effect(uint32_t task);                                           // 0x1DE90 (proofs-10: p10_gift)
    void gift_fly_step(uint32_t task, uint32_t model);                         // 0x1E040 / 0x1DE90 body
    // boundaries only the hostages call
    virtual uint32_t task_create_child_alloc(uint32_t routine, uint32_t size, uint32_t parent);   // 0x15010 (support_c.cpp)
