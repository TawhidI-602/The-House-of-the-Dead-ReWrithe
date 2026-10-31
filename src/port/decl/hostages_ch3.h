// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): Chapter 3's
// hostage scenarios 12-15 (hostages_ch3.cpp, worker "ch3own"). Arcade forms only. Proven on A (ch3own_full: the
// full-game recording, 0 differ) but where marked UNVERIFIED (teaser).
    void hostage_s12_cell(uint32_t obj);                                      // 0x57C80 (scenario 12)
    void hostage_s12_wait(uint32_t obj);                                      // 0x57CE0
    void hostage_s12_rescue(uint32_t obj);                                    // 0x57DB0
    void hostage_ch3_gift(uint32_t obj);                                      // inline (s13 / s15)
    void hostage_ch3_count(uint32_t obj);                                     // inline (s14 / s15)
    void hostage_s13_platform_gift(uint32_t obj);                             // 0x57EF0 (scenario 13)
    void hostage_s13_wait(uint32_t obj);                                      // 0x57F70
    void hostage_s13_rescue(uint32_t obj);                                    // 0x57FF0
    void hostage_s14_message_route(uint32_t obj);                             // 0x58320 (scenario 14)
    void hostage_s14_wait(uint32_t obj);                                      // 0x58380
    void hostage_s14_rescue(uint32_t obj);                                    // 0x58410 proven (vf_c3a)
    void hostage_s15_gift(uint32_t obj);                                      // 0x58650 (scenario 15) proven (vf_c3a)
    void hostage_s15_wait(uint32_t obj);                                      // 0x586B0 proven (vf_c3a)
    void hostage_s15_rescue(uint32_t obj);                                    // 0x58730 UNVERIFIED (teaser): not reached
