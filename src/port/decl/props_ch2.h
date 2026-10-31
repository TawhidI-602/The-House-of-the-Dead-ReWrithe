// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): Chapter 2's prop
// sub-types (props_ch2.cpp, worker "ch2"). Arcade forms only; proven on A (ch2proof) but where marked UNVERIFIED (teaser).
    void prop015_fuuu(uint32_t obj);                                          // 0x3C6B0 (sub-type 15)
    void prop064_hit_drop(uint32_t obj);                                      // 0x3E5F0 (sub-type 64)
    void prop067_r2_lid(uint32_t obj);                                        // 0x3EF60 (sub-type 67)
    void prop_flag_swap(uint32_t obj);                                        // 0x3BFA0 (sub-types 30 / 31 / 43 / 86)
    void prop_glass_shatter(uint32_t obj);                                    // 0x40160
    void prop_anim_once(uint32_t obj);                                        // 0x42CF0
    void prop023_kokonoha_leaves(uint32_t obj);                               // 0x3C770 (sub-type 23)
    void prop014_jinmengyo_fish(uint32_t obj);                                // 0x3C4A0 (sub-type 14)
    void prop078_r2_door(uint32_t obj);                                       // 0x3E9B0 (sub-types 78 / 79)
    void prop_door_fly(uint32_t obj);                                         // 0x3EB90 UNVERIFIED (teaser): flag 14 is never set
    void prop084_niku(uint32_t obj);                                          // 0x3ED70 (sub-type 84)
    void prop083_kyochuu(uint32_t obj);                                       // 0x3EE00 (sub-type 83)
    void prop083_kyochuu_fall(uint32_t obj);                                  // 0x3EEE0
    void prop077_r2_wall(uint32_t obj);                                       // 0x3E880 (sub-type 77)
    void prop077_wall_pieces(uint32_t obj);                                   // 0x3AD60
    void prop077_wall_piece(uint32_t obj);                                    // 0x3E8F0
    void prop080_statue_piece(uint32_t obj);                                  // 0x3EC90 (sub-type 80)
    void prop085_fruit_scheduled(uint32_t obj);                               // 0x3BC40 (sub-type 85)
    void prop008_bookshelf_topple(uint32_t obj);                              // 0x3E800 (sub-type 8, Chapter 2 events 13 / 14)
    virtual void prop_hit_drop(uint32_t obj);                                 // 0x42140 (was a props.h boundary; the proof keeps its override)
    void prop_hit_drop_piece(uint32_t obj);                                   // 0x41FF0
    virtual void prop_lid_fly(uint32_t obj);                                  // 0x422B0 (was a props.h boundary; the proof keeps its override)
    void prop_fall_spin(uint32_t obj);                                        // 0x42EC0
    // the bookshelves (Chapters 1 / 2; worker "ch2play"), proven on A (ch2proof)
    virtual void prop_bookshelf_collapse(uint32_t obj);                       // 0x40BB0 (was a props.h boundary)
    virtual void prop_bookshelf_remains(uint32_t obj);                        // 0x40F50 (was a props.h boundary)
    void prop_book_fly(uint32_t obj);                                         // 0x40980 (task)
    void prop_blink_remains(uint32_t obj);                                    // 0x42E70 (task)
