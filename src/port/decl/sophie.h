// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): kinds 16 (Sophie's
// scene), 78, 170 (sophie.cpp, teaser L7). Arcade forms only.
    // --- kinds 16, 78, 170 (sophie.cpp) ---
    void sophie(uint32_t obj);                                                // 0x95EE0 (kind 16)
    void sophie_wait_scene(uint32_t obj);                                     // 0x95F50
    void sophie_stand_up(uint32_t obj);                                       // 0x95FB0
    void sophie_writhe(uint32_t obj);                                         // 0x96020
    void sophie_wait_struck(uint32_t obj);                                    // 0x960A0
    void sophie_fall(uint32_t obj);                                           // 0x96110
    void sophie_wait_lying(uint32_t obj);                                     // 0x96190
    void sophie_lying_b(uint32_t obj);                                        // 0x961F0
    void sophie_wait_lying_b(uint32_t obj);                                   // 0x96230
    void sophie_lying_c(uint32_t obj);                                        // 0x96280
    void sophie_lying_d(uint32_t obj);                                        // 0x962E0 (proofs-8: p8_sweep1)
    void sophie_wait_frame37(uint32_t obj);                                   // 0x96350 (proofs-8: p8_sweep1)
    void sophie_wait_end(uint32_t obj);                                       // 0x963A0 (proofs-6: fresh auto-aim games, p6_def / p6_r5 / p6_2p)
    void kind78_delay(uint32_t obj);                                          // 0x310B0 (kind 78)
    void kind170_glass_break(uint32_t obj);                                   // 0x3520 (kind 170)
    // helper without an original routine of its own (the routines' common tail)
    void sophie_draw(uint32_t obj, bool count);
