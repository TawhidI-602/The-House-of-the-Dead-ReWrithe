// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): the support
// routines the play-through calls most (attack positions, the scripted enemies' hits and deaths, floors and jump arcs),
// first written as runtime overrides (src/game/t1_bounds.cpp, T1) and moved here for their proofs (docs/proof.md
// "Support routines"). Arcade forms only: the virtual ones are instantiated for Game<Pc> for its vtable alone (their
// PC bodies do nothing; ProofPc replays them as boundaries).
    // --- attack positions (attack_slots.cpp) ---
    virtual void attack_slot_take_waiting(uint32_t obj);                      // 0x31E60
    virtual void attack_slot_take_wide(uint32_t obj);                         // 0x31D80
    virtual void attack_slot_front_take(uint32_t obj);                        // 0x32010
    virtual uint32_t attack_slot_nearest_object(uint32_t obj);                // 0x2FF90
    virtual uint32_t enemy_attack_slot_take(uint32_t obj);                    // 0x32160 (1 = taken)
    virtual void enemy_attack_slot_free(uint32_t obj);                        // 0x32C50
    void attack_slots_sort();                                                 // 0x32970
    void attack_slots_update();                                               // 0x32440
    // --- the scripted enemies' hits and deaths (enemy_scripted.cpp) ---
    virtual void enemy_scripted_hit(uint32_t obj);                            // 0x535F0
    void enemy_scripted_dead(uint32_t obj);                                   // 0x1C670
    void enemy_scripted_dead_fall(uint32_t obj);                              // 0x1C780
    virtual void enemy_wait_camera_stop(uint32_t obj);                        // 0x77350
    void Zombie_Dead(uint32_t obj);                                           // 0x78FC0
    void Zombie_Dead_frame(uint32_t obj);                                     // 0x76E50
    // --- floors, jump arcs, moving along the heading (floor_motion.cpp) ---
    virtual uint32_t floor_probe(uint32_t point, uint32_t outA, uint32_t outB, uint32_t outC);   // 0x45FE0 (1 = found)
    virtual float ground_height(uint32_t point);                              // 0x461D0
    virtual void jump_arc_set(uint32_t obj, uint32_t fx, uint32_t fy, uint32_t fz, uint32_t tx, uint32_t ty, uint32_t tz, int frames);   // 0x2F2F0 (floats as bits)
    virtual uint32_t jump_arc_step_b(uint32_t obj);                           // 0x2F330 (1 = landed)
    virtual uint32_t jump_arc_step(uint32_t obj);                             // 0x2F400 (1 = landed)
    virtual uint32_t jump_arc_velocity(uint32_t obj);                         // 0x2F560 (1 = the jump is over)
    virtual uint32_t jump_arc_velocity_b(uint32_t obj);                       // 0x2F610 (1 = the jump is over)
    void obj_move_heading(uint32_t obj);                                      // 0x6A8A0
