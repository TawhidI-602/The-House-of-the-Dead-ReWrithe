// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): kind 18 set-up 0,
// the Hangedman's fly-by in Chapter 1 event 3, and kind 17, the researcher it drops (hangedman_flyby.cpp, teaser L5).
// Arcade forms only.
    // --- kind 18 set-up 0 and kind 17 (hangedman_flyby.cpp) ---
    void boss_hangedman(uint32_t obj);                                         // 0x4EE70 (kind 18)
    void hangedman_idle(uint32_t obj);                                         // 0x4EEA0 (ret only)
    void hangedman_flyby_init(uint32_t obj);                                   // 0x4EEB0 (proofs-8: p8_sweep1)
    void hangedman_flyby_frame(uint32_t obj);                                  // 0x4F000
    void hangedman_flyby_start(uint32_t obj);                                  // 0x4F080 (proofs-8: p8_sweep1)
    void hangedman_flyby_move(uint32_t obj, const float d[3], float speed);    // inline in hangedman_flyby
    void hangedman_flyby(uint32_t obj);                                        // 0x4F0C0
    void kind17_kenkyu_researcher_collapse(uint32_t obj);                      // 0x95710 (kind 17)
    void kind17_researcher_frame(uint32_t obj);                                // 0x95770
    void kind17_researcher_st0_none(uint32_t obj);                             // 0x958C0 (ret only)
    void kind17_researcher_st1_appear(uint32_t obj);                           // 0x958D0
    void kind17_researcher_st2_run(uint32_t obj);                              // 0x95980
    void kind17_researcher_st3_collapse(uint32_t obj);                         // 0x95B50
    void kind17_researcher_model_draw(uint32_t obj);                           // 0x95D70
