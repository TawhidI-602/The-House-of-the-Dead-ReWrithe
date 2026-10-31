// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): kind 18 set-ups 1
// and 2, the Hangedman's fight (Chapter 2 event 15) and rematch (Chapter 4), its captives, intro, life bar and death
// effects, and kind 19, the billboard of the same event (boss_hangedman.cpp, worker "hanged"). Arcade forms only.
// Proven on A where marked (hanged_full: the full-game recording, 0 differ); UNVERIFIED (teaser) where marked.
    // --- kind 18 set-ups 1 / 2 (boss_hangedman.cpp) ---
    void hangedman_fight_init(uint32_t obj);                                   // 0x4F960 (ch2proof)
    void hangedman_fight_frame(uint32_t obj);                                  // 0x4FAA0 (hanged_full)
    void hangedman_hit(uint32_t obj);                                          // 0x4FC40 (hanged_full)
    void hangedman_hit_reaction(uint32_t obj);                                 // 0x4FDF0 (hanged_full)
    void hangedman_vuln_clear(uint32_t obj);                                   // 0x500E0 (hanged_full)
    void hangedman_death_fall(uint32_t obj);                                   // 0x50110 (hanged_full), state 13
    void hangedman_intro(uint32_t obj);                                        // 0x503E0 (hanged_full), state 3
    void hangedman_flap(uint32_t obj);                                         // 0x50700 (hanged_full), state 4
    void hangedman_rejoin(uint32_t obj);                                       // 0x50930 (hanged_full), state 5
    void hangedman_hover(uint32_t obj);                                        // 0x509F0 (hanged_full), state 6
    void hangedman_claw_attack(uint32_t obj);                                  // 0x50C10 (hanged_full), state 7
    void hangedman_return(uint32_t obj);                                       // 0x51020 (hanged_full), state 8
    void hangedman_flee(uint32_t obj);                                         // 0x51290 (hanged_full), state 9
    void hangedman_wait_reappear(uint32_t obj);                                // 0x515B0 (hanged_full), state 10
    void hangedman_dive_approach(uint32_t obj);                                // 0x51B40 (hanged_full), state 11
    void hangedman_dive_attack(uint32_t obj);                                  // 0x51FA0 (hanged_full), state 12
    void hangedman_rematch_init(uint32_t obj);                                 // 0x527A0 (ch2proof)
    void hangedman_rematch_intro(uint32_t obj);                                // 0x528D0 (ch2proof), state 14
    void hangedman_hover_point(uint32_t obj, uint32_t path, uint32_t frame, uint32_t out);   // 0x52980 (hanged_full)
    void boss_life_bar_task(uint32_t t);                                       // 0x52C30 (hanged_full)
    void hangedman_death_effect(uint32_t obj);                                 // 0x52CE0 (hanged_full)
    void hangedman_turn_toward(uint32_t obj, uint32_t angle, int32_t step);    // 0x52D20 (hanged_full)
    void hangedman_explosion(uint32_t t);                                      // 0x52DA0 (hanged_full)
    void camera_set_direct(uint32_t ex, uint32_t ey, uint32_t ez, uint32_t lx, uint32_t ly, uint32_t lz);   // 0x301E0 (hanged_full), floats as bits
    void boss_aggression_init(uint32_t obj);                                   // 0x305A0 (hanged_full)
    void boss_aggression_update(uint32_t obj);                                 // 0x305D0 (hanged_full)
    void billboard_model(uint32_t obj);                                        // 0x30690 (hanged_full), kind 19
    void hangedman_intro_effect_spawn(int n);                                  // 0x31120 (hanged_full)
    void hangedman_intro_effect(uint32_t t);                                   // 0x31160 (hanged_full)
    void hangedman_captive_frame(uint32_t obj);                                // 0x70190 (hanged_full)
    void hangedman_captives();                                                 // 0x703B0 (ch2proof: inside hangedman_fight_init's captures)
    // helpers without an original routine of their own (inline in the routines above)
    void hangedman_face_camera(uint32_t obj);
    void hangedman_angles(float dx, float dy, float dz, uint32_t& yaw, uint32_t& pitch);
    int32_t hangedman_share(uint32_t obj, uint32_t byteOff);
    void hangedman_slot_free(uint32_t obj);
