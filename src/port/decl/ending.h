// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): the ending after
// Chapter 4 (ending.cpp; worker "verify", moved from src/game/ending.cpp for its capture proof). Arcade forms only.
    void game_ending_start();                                                 // 0x35160 proven (vf_end: 1 capture; it differs only where a vblank interrupt in the tile copies wrote) (mode 4 sub-mode 7)
    void game_ending();                                                       // 0x352A0 proven (vf_end: 200) (sub-mode 8)
    virtual void game_ending_finish();                                        // game_ending's tail 0x35360 (the runtime: its ranking flow)
    void sky_task_ending_start();                                             // 0x34360 proven (vf_end2: 1)
    void sky_task_ending(uint32_t task);                                      // 0x34150 proven (vf_end: 200)
    void kind120_letterbox_open(uint32_t obj);                                // 0x353E0 proven (vf_end / vf_end2: 15)
    void kind120_letterbox_open_frame(uint32_t obj);                          // 0x361C0 proven (vf_end: 15)
    void kind121_ending_scene(uint32_t obj);                                  // 0x35F10 proven (vf_end / vf_end2: 14)
    void kind121_ending_scene_draw(uint32_t obj);                             // 0x35410 proven (vf_end: 175)
    void kind105_ending_caption(uint32_t obj);                                // 0x35F30 proven (vf_end2: 1, palette RAM recorded)
    void kind116_end_sprite(uint32_t obj);                                    // 0x35FF0 proven (vf_end / vf_end2: 2)
    void kind122_ending_score(uint32_t obj);                                  // 0x360D0 proven (vf_end / vf_end2 / vf_c4c: 3)
    void kind122_letterbox_close_frame(uint32_t obj);                         // 0x36030 proven (vf_end: 2)
    void kind110_letterbox(uint32_t obj);                                     // 0x1E630 proven (vf_end / vf_end2: 16)
    void kind110_wait(uint32_t obj);                                          // 0x1E580 proven (vf_end: 2)
    void kind111_texture_area(uint32_t obj);                                  // 0x1E7B0 proven (vf_end: 1)
    void kind179_chapter_results(uint32_t obj);                               // 0x9C020 proven (vf_end: 1, vf_c4c: 1 with two players)
    void kind179_chapter_results_frame(uint32_t obj);                         // 0x9C200 proven (vf_end: 8)
    void ending_p1_a(uint32_t obj);                                           // 0x58C40 proven (vf_end2: 1) (kind 103 set-ups)
    void ending_p1_b(uint32_t obj);                                           // 0x59090 proven (vf_c4c: 1, two players)
    void ending_p2_a(uint32_t obj);                                           // 0x59660 UNVERIFIED (teaser): not reached (player 2 alone)
    void ending_p2_b(uint32_t obj);                                           // 0x59A80 proven (vf_c4c: 1)
    void ending_p1_a_frame(uint32_t obj);                                     // 0x58CC0 proven (vf_end: 58)
    void ending_p1_b_frame(uint32_t obj);                                     // 0x59120 proven (vf_c4c: 100)
    void ending_p2_a_frame(uint32_t obj);                                     // 0x596F0 UNVERIFIED (teaser): not reached (player 2 alone)
    void ending_p2_b_frame(uint32_t obj);                                     // 0x59B10 proven (vf_c4c: 100)
    void player_st7_idle(uint32_t task);                                      // 0x55B0 proven (vf_end2: 1)
    // the set-ups' and frames' common pieces (inline in the original)
    void ending_body_setup(uint32_t obj, uint32_t table, uint32_t frame, uint32_t x0, bool lowAlways, int16_t lowType);
    virtual int32_t ending_body_type(uint32_t table);                         // the body type (table by 0x51F42C; the runtime: PC Mode's)
    void ending_body_head(uint32_t obj);
    void ending_body_hand(uint32_t obj, uint32_t last, uint32_t first);
    void ending_body_props();
    void ending_body_shift(uint32_t obj, bool shift);
    void ending_body_end(uint32_t obj);
    // boundaries (not ported: the runtime's own forms)
    virtual void sprite_init() {}                                             // 0x170C0
    virtual void text_layers_clear() {}                                       // 0x171B0
    virtual void rank_insert() {}                                             // 0x5E5D0
    virtual void palette_curve_fade_in_start();                               // 0x18B50 (ported, ch4proof: the colour curves' fade-in)
    void palette_curve_fade_in_task(uint32_t task);                           // 0x189C0 (ported, ch4proof)
    void palette_curve_fade_step(uint32_t task);                              // the step both run (inline in each)
    virtual void player_idle(uint32_t task) {}                                // 0x55D0 (ret)
    virtual int32_t score_rank_get(int p, int table);                         // 0x9BE20 (ported)
