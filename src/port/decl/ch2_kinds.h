// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): Chapter 2's own
// kinds (ch2_kinds.cpp, worker "ch2"). Arcade forms only; proven on A (ch2proof) but where marked UNVERIFIED (teaser).
    void kind183_shootable_sign(uint32_t obj);                                // 0x3A40 (kind 183)
    void kind168_double_door(uint32_t obj);                                   // 0x96EC0 (kind 168)
    void kind168_double_door_tail(uint32_t obj);                              // 0x96FDC (a Ghidra split: r4 = the object)
    void kind075_sound_repeat(uint32_t obj);                                  // 0x92BE0 (kind 75)
    void kind131_prop_group(uint32_t obj);                                    // 0x3AAA0 (kind 131)
    void kind133_prop_grid(uint32_t obj);                                     // 0x39BC0 (kind 133)
    void kind136_prop_group(uint32_t obj);                                    // 0x39F60 (kind 136)
    void kind132_ebitb_vomit(uint32_t obj);                                   // 0x5FDA0 (kind 132)
    void kind132_ebitb_vomit_frame(uint32_t obj);                             // 0x5FF60
    void kind132_ebitb_dead(uint32_t obj);                                    // 0x60330
    void kind132_drips_draw(uint32_t obj);                                    // 0x603E0
    void kind132_pick_target(uint32_t obj);                                   // inline (set-up and frame)
    virtual void enemy_fall_death_check(uint32_t obj);                        // 0x93340 (was a core boundary; the proof keeps its override)
    void enemy_fall_death_land(uint32_t obj);                                 // 0x934B0
    void kind187_fridge_door(uint32_t obj);                                   // 0x970B0 (kind 187)
    void kind184_lift(uint32_t obj);                                          // 0x3B90 (kind 184)
    void kind184_lift_frame(uint32_t obj);                                    // 0x3BE0
    void kind185_elevator(uint32_t obj);                                      // 0x3CA0 (kind 185)
    void kind185_elevator_frame(uint32_t obj);                                // 0x3CF0
    void kind151_prop_group(uint32_t obj);                                    // 0x3AEF0 (kind 151)
    void kind152_prop_group(uint32_t obj);                                    // 0x3A500 (kind 152)
    void kind150_ebitan_slide(uint32_t obj);                                  // 0x61240 (kind 150)
    void kind150_ebitan_slide_frame(uint32_t obj);                            // 0x613D0
    void kind150_ebitan_slide_dying(uint32_t obj);                            // 0x61C10
    void kind150_slope_shadow(uint32_t obj, uint32_t model);                  // inline (event 3)
    void kind150_uncount(uint32_t obj);                                       // inline
    // former core boundaries (worker "ch2play"; still virtual so the proof's boundary overrides stay), proven on A (ch2proof)
    virtual void obj_velocity_clear(uint32_t obj);                            // 0x2E870
    virtual void area_heading_pick(uint32_t rect, uint32_t pos, uint32_t step, uint32_t outAngle);   // 0x2E550
    virtual void frog_splash_spawn(uint32_t obj);                             // 0x308B0
    void frog_splash(uint32_t obj);                                           // 0x30820 (the splash task)
    virtual void point_clip_to_depth(uint32_t point, uint32_t from, float depth);   // 0x34920
