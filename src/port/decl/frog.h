// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): kind 69, the frog
// (enemy_frog.cpp).
    // --- kind 69, the frog (enemy_frog.cpp) ---
    void kind69_frog_bouere(uint32_t obj);                                     // 0x88210
    void kind69_call(uint32_t routine, uint32_t obj);                          // callx through its tables
    void kind69_frog_body_init(uint32_t obj);                                  // 0x88230
    void kind69_frog_frame(uint32_t obj);                                      // 0x88280
    void kind69_frog_st1_hit(uint32_t obj);                                    // 0x88370
    void kind69_frog_st2_die(uint32_t obj);                                    // 0x883D0
    void frog_hop_target(uint32_t obj, float dx, float dy, float dz);          // inline (double precision)
    void kind69_frog_st3_hop(uint32_t obj);                                    // 0x885E0
    void kind69_frog_st4_hop_in_place(uint32_t obj);                           // 0x88A50
    void kind69_frog_st5_leap_attack(uint32_t obj);                            // 0x88E30
    void frog_swim_draw(uint32_t obj);                                         // inline in set-ups 0 / 3 / 4
    void frog_swim_step(uint32_t obj, uint32_t speedOff, uint32_t posOff);     // inline
    uint32_t frog_task(uint32_t routine, uint32_t point, uint32_t index);      // inline
    void frog_body(uint32_t t);                                                // inline
    void frog_swim_out(uint32_t obj, bool far);                                // the bodies of set-ups 3 / 4
    void kind69_frog_setup0_swimmers(uint32_t obj);                            // 0x892B0
    void kind69_frog_setup1_pose(uint32_t obj);                                // 0x89600
    void kind69_frog_setup2_leap_in(uint32_t obj);                             // 0x89930
    void kind69_frog_setup3_swim_out(uint32_t obj);                            // 0x89BC0
    void kind69_frog_setup4_swim_out_far(uint32_t obj);                        // 0x89FE0
