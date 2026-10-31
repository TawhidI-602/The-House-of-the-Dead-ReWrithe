// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): kind 20, the
// Magician (Chapter 4's boss, event 4: set-up 0 the intro with Dr. Curien, set-up 1 the fight), his fireballs,
// barrage, afterimages, trackers and death effects, the palette fades and the body-part searches they use
// (boss_magician.cpp, worker "hermit"). Arcade forms, read from the arcade disassembly; proven on A by routine (marks).
    // --- kind 20, the Magician (boss_magician.cpp) ---
    void boss_magician(uint32_t obj);                                          // 0x70AB0 proven (ch4p_full)
    void magician_idle(uint32_t obj);                                          // 0x70AD0 UNVERIFIED (teaser), state 0
    void magician_intro(uint32_t obj);                                         // 0x70AE0 proven (ch4p_full), set-up 0
    void magician_fight_init(uint32_t obj);                                    // 0x70E80 proven (ch4p_full), set-up 1
    void magician_frame(uint32_t obj);                                         // 0x71010 proven (ch4p_full)
    void magician_hit(uint32_t obj);                                           // 0x71110 proven (ch4p_full)
    void magician_hit_spark(uint32_t obj);                                     // 0x71260 proven (ch4p_full)
    void magician_hit_reaction(uint32_t obj);                                  // 0x712F0 proven (ch4p_full)
    void magician_vuln_clear(uint32_t obj);                                    // 0x713D0 proven (ch4p_full)
    void magician_hit_check_a(uint32_t obj);                                   // 0x71400 proven (ch4p_full)
    void magician_hit_check_b(uint32_t obj);                                   // 0x71490 proven (ch4p_full)
    void magician_hit_check_c(uint32_t obj);                                   // 0x71590 proven (ch4p_full)
    void magician_dying(uint32_t obj);                                         // 0x71620 proven (ch4p_full), state 1
    void magician_descend(uint32_t obj);                                       // 0x71C80 proven (ch4p_full), state 2
    void magician_next_pattern(uint32_t obj);                                  // 0x71F40 proven (ch4p_full), state 3
    void magician_fireballs(uint32_t obj);                                     // 0x72210 proven (ch4p_full), state 4
    void magician_attack(uint32_t obj);                                        // 0x72680 proven (ch4p_full), state 5
    void magician_barrage(uint32_t obj);                                       // 0x72DE0 proven (ch4p_full), state 6
    void magician_fireball(uint32_t obj);                                      // 0x73010 proven (ch4p_full)
    void magician_fireball_aim(uint32_t obj);                                  // 0x736C0 proven (ch4p_full)
    void magician_fireball_hit(uint32_t obj);                                  // 0x73820 UNVERIFIED (teaser)
    void magician_fireball_spawn(uint32_t obj, int hand);                      // 0x738D0 proven (ch4p_full)
    void magician_intro_object(uint32_t obj);                                  // 0x739D0 proven (ch4p_full)
    void magician_intro_object_spawn(uint32_t obj);                            // 0x73E60 proven (ch4p_full)
    void fireball_trail(uint32_t obj);                                         // 0x73EE0 proven (ch4p_full)
    void fireball_trail_copy(uint32_t obj);                                    // 0x74030 proven (ch4p_full)
    void magician_afterimage_spawn(uint32_t obj);                              // 0x740D0 proven (ch4p_full)
    void magician_barrage_shot_frame(uint32_t obj);                            // 0x74150 proven (ch4p_size) (not in the index)
    void magician_barrage_shot(uint32_t obj, uint32_t part);                   // 0x749F0 proven (ch4p_full)
    void magician_fireball_frame(uint32_t obj);                                // 0x74AD0 proven (ch4p_full)
    void magician_barrage_projectile(uint32_t obj);                            // 0x74C70 proven (ch4p_size) (not in the index)
    void magician_barrage_spawn(uint32_t obj);                                 // 0x74EC0 proven (ch4p_full)
    void magician_tracker_spawn(uint32_t obj);                                 // 0x74F20 proven (ch4p_full)
    void magician_part_effect_task(uint32_t obj);                              // 0x74FC0 proven (ch4p_full)
    void magician_part_effect(uint32_t part);                                  // 0x750D0 proven (ch4p_full)
    void magician_death_flash(uint32_t obj);                                   // 0x75200 proven (ch4p_full)
    void magician_death_flash_spawn(uint32_t obj);                             // 0x753B0 proven (ch4p_full)
    void magician_afterimage(uint32_t obj);                                    // 0x75410 proven (ch4p_full)
    void magician_tracker(uint32_t obj);                                       // 0x75530 proven (ch4p_full)
    // shared helpers the Magician reaches
    void palette_fade_in_task(uint32_t t);                                     // 0x18F00 proven (ch4p_full)
    void palette_fade_in_start(uint32_t frames);                               // 0x19130 proven (ch4p_full), float bits
    void palette_fade_out_task(uint32_t t);                                    // 0x19860 proven (ch4p_full)
    void palette_fade_out_start(uint32_t frames);                              // 0x199C0 proven (ch4p_full), float bits
    void palette_blend_white(int level, uint32_t src, uint32_t dst);           // 0x1A110 proven (ch4p_full)
    uint32_t any_player_in_play();                                             // 0x2F780 proven (ch4p_full), 1 or -1
    uint32_t body_find_part_rec(uint32_t part, uint32_t id);                   // 0x30260 proven (ch4p_full)
    uint32_t body_find_part(uint32_t obj, uint32_t id);                        // 0x302B0 proven (ch4p_full)
    uint32_t body_hit_part_find_rec(uint32_t part);                            // 0x30330 proven (ch4p_full)
    uint32_t body_hit_part_find(uint32_t obj);                                 // 0x30380 proven (ch4p_full)
    // helpers without an original routine of their own (inline in the routines above)
    void magician_face_camera(uint32_t obj);
    bool magician_fly_to(uint32_t obj, uint32_t target, float speed);
    Tgp::Out magician_from_camera(uint32_t mag, uint32_t obj);
    void magician_effect_matrix(uint32_t obj);
