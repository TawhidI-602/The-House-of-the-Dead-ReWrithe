// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): kind 64, the
// researchers (enemy_researchers.cpp).
    // --- kind 64, the researchers (enemy_researchers.cpp) ---
    void kind64_kenkyu_researchers(uint32_t obj);                              // 0x6DAC0
    void kind64_call(uint32_t routine, uint32_t obj);                          // callx through its tables
    void kind64_researcher_init(uint32_t obj);                                 // PC 0x4CCE5C (arcade: inline)
    void kind64_frame(uint32_t obj);                                           // 0x6DBD0
    void kind64_slot_free(uint32_t obj);                                       // inline
    bool kind64_motion_done(uint32_t obj);                                     // inline
    void kind64_to(uint32_t obj, int16_t state);                               // inline
    void kind64_sub_next(uint32_t obj);                                        // inline
    void kind64_count_rescue();                                                // inline
    void kind64_hurt(uint32_t obj, uint16_t flags);                            // inline
    void kind64_face(uint32_t obj, float dx, float dy, float dz);              // inline (coprocessor 0x0F)
    void kind64_fall(uint32_t obj, uint32_t turn, int16_t next);               // the bodies of states 3 / 17
    bool kind64_gift(uint32_t obj, int16_t first, uint32_t sound, uint32_t head, uint32_t closed);   // inline in 7 / 14
    void kind64_scene_end(uint32_t obj, uint32_t flag);                        // inline in 15 / 19 / 26
    void kind64_struck_effect(uint32_t obj);                                   // inline in state 29
    float pc_floor_distance2(float ax, float az, float bx, float bz);          // PC 0x4CB010 (PC only)
    float pc_fast_sqrt7(float x);                                              // PC 0x4CB7CC (PC only)
    void pc_vec_angles6(float x, float y, float z, uint32_t& yaw, uint32_t& pitch);   // PC 0x4CB078 (PC only)
    void kind64_set0_hanging(uint32_t obj);                                    // 0x6DC30
    void kind64_st03_hang(uint32_t obj);                                       // 0x6DE10
    void kind64_st04_climb_up(uint32_t obj);                                   // 0x6DF10
    void kind64_st05_remove(uint32_t obj);                                     // 0x6E020
    void kind64_set1_sno(uint32_t obj);                                        // 0x6E0A0
    void kind64_st13_shot(uint32_t obj);                                       // 0x6E200
    void kind64_st14_rescue(uint32_t obj);                                     // 0x6E290
    void kind64_st15_remove(uint32_t obj);                                     // 0x6E540
    void kind64_set2_hanging_ladder(uint32_t obj);                             // 0x6E5D0
    void kind64_st17_hang(uint32_t obj);                                       // 0x6E750
    void kind64_st18_ladder(uint32_t obj);                                     // 0x6E850
    void kind64_st19_remove(uint32_t obj);                                     // 0x6E930
    void kind64_set3(uint32_t obj);                                            // 0x6E9C0
    void kind64_hit_check(uint32_t obj);                                       // 0x6EB90
    void kind64_st07_rescue(uint32_t obj);                                     // 0x6EBE0
    void kind64_st08_remove(uint32_t obj);                                     // 0x6EE00
    void kind64_set4_crawl(uint32_t obj);                                      // 0x6EEC0
    void kind64_set4_next(uint32_t obj);                                       // 0x6F100
    void kind64_st10_run(uint32_t obj);                                        // 0x6F140
    void kind64_st11_remove(uint32_t obj);                                     // 0x6F1E0
    void kind64_set7_cell(uint32_t obj);                                       // 0x6F280
    void kind64_set8_crawl(uint32_t obj);                                      // 0x6F480
    void kind64_set5_hold_up(uint32_t obj);                                    // 0x6F5D0
    void kind64_st21_talk(uint32_t obj);                                       // 0x6F8F0
    void kind64_st22_message(uint32_t obj);                                    // 0x6F970
    void kind64_set6_sno(uint32_t obj);                                        // 0x6FB40
    void kind64_st24_shot(uint32_t obj);                                       // 0x6FCB0
    void kind64_st25_rescue(uint32_t obj);                                     // 0x6FD60
    void kind64_st26_remove(uint32_t obj);                                     // 0x6FF50
    void kind64_st29_struck(uint32_t obj);                                     // 0x6FFE0
