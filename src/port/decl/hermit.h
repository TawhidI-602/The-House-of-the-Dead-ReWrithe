// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): kind 40, the
// Hermit (Chapter 3's boss, event 7), with its spit ball, title card, life bar, drop and death effects, and kind 24,
// Dr. Curien's scene after the fight (boss_hermit.cpp, worker "hermit"). Arcade forms only. Proven on A (hermit_full:
// the full-game recording, 0 differ) but where marked UNVERIFIED (teaser).
    // --- kind 40, the Hermit (boss_hermit.cpp) ---
    void boss_hermit(uint32_t obj);                                            // 0x85430 (hermit_full)
    void hermit_entrance(uint32_t obj);                                        // 0x85550 (hermit_full)
    void hermit_frame_outer(uint32_t obj);                                     // 0x85640 (hermit_full)
    void hermit_frame(uint32_t obj);                                           // 0x85700 (hermit_full)
    void hermit_act1_pose(uint32_t obj);                                       // 0x85B30 (hermit_full)
    void hermit_act_wait_combat(uint32_t obj);                                 // 0x85BE0 (hermit_full), actions 33 / 34
    void hermit_act_turn(uint32_t obj);                                        // 0x85C80 (hermit_full), actions 16-19
    void hermit_act_step(uint32_t obj);                                        // 0x85D90 (hermit_full), actions 8-10
    void hermit_act_walk(uint32_t obj);                                        // 0x85E70 (hermit_full), actions 2-7
    void hermit_act22_guard(uint32_t obj);                                     // 0x86000 (hermit_full)
    void hermit_act23_guard_hold(uint32_t obj);                                // 0x86070 (hermit_full)
    void hermit_act24_guard_end(uint32_t obj);                                 // 0x86100 UNVERIFIED (teaser): action 24 not in the recording
    void hermit_act31_drop(uint32_t obj);                                      // 0x86160 (hermit_full)
    void hermit_drop_sound_start();                                            // 0x86410 (hermit_full)
    void hermit_act21_lunge(uint32_t obj);                                     // 0x86430 (hermit_full)
    void hermit_act_attack(uint32_t obj);                                      // 0x86510 (hermit_full), actions 11-15
    void hermit_act20_spit(uint32_t obj);                                      // 0x866A0 (hermit_full)
    void hermit_act32_hit_reaction(uint32_t obj);                              // 0x86990 (hermit_full)
    void hermit_hit_reaction_pick(uint32_t obj);                               // 0x86AB0 (hermit_full)
    void hermit_hits_and_draw(uint32_t obj);                                   // 0x86B20 (hermit_full)
    void hermit_spit_ball(uint32_t obj);                                       // 0x86CB0 (hermit_full)
    void hermit_spit_ball_shot(uint32_t obj);                                  // 0x87060 (hermit_full)
    void hermit_spit_ball_draw(uint32_t obj);                                  // 0x870C0 (hermit_full)
    void hermit_action_done(uint32_t obj);                                     // 0x871A0 (hermit_full)
    void hermit_attack_hit(int who);                                           // 0x871F0 (hermit_full)
    void hermit_part_draw_hook(uint32_t part);                                 // 0x87560 (hermit_full), body +0x11C
    void hermit_title_task(uint32_t t);                                        // 0x87770 (hermit_full)
    void hermit_life_bar_task(uint32_t t);                                     // 0x878E0 (hermit_full)
    void hermit_drop_dust_spawn();                                             // 0x879A0 (hermit_full)
    void hermit_death_sink(uint32_t obj);                                      // 0x879E0 (hermit_full)
    void hermit_drop_sound(uint32_t t);                                        // 0x87AD0 (hermit_full)
    void hermit_hit_sounds(uint32_t t);                                        // 0x87B10 (hermit_full)
    void hermit_drop_dust(uint32_t t);                                         // 0x87B50 (hermit_full)
    // --- kind 24, Dr. Curien after the fight (boss_hermit.cpp) ---
    void kind24_kyurian_curien(uint32_t obj);                                  // 0x87C50 (hermit_full)
    void kind24_kyurian_wait(uint32_t obj);                                    // 0x87D10 (hermit_full)
    void kind24_kyurian_rise(uint32_t obj);                                    // 0x87D80 (hermit_full)
    void kind24_kyurian_float(uint32_t obj);                                   // 0x87E20 (hermit_full)
    // helpers without an original routine of their own (inline in the routines above)
    bool hermit_motion_end(uint32_t obj);
    void hermit_view_point(uint32_t obj);
    void kyurian_draw(uint32_t obj);
