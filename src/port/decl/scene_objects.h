// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): Chapter 1's
// scene objects: kinds 161 (voice lines), 162-164 / 166 / 167 (doors), 173, 180, 172, 177, 178 (scene_objects.cpp,
// teaser L5). Arcade forms only.
    // --- kind 161, the voice lines (scene_objects.cpp) ---
    void voice_message_task(uint32_t task);                                    // 0x9B380 (kind 161)
    void voice_message_frame(uint32_t task);                                   // 0x9B4C0
    // --- doors 162-164, 166, 167 ---
    void kind162_door(uint32_t obj);                                           // 0x52E50 (kind 162)
    void kind162_door_frame(uint32_t obj);                                     // 0x52EA0
    void kind162_idle(uint32_t obj);                                           // 0x52E40 (ret only)
    void kind162_end(uint32_t obj);                                            // 0x53360 (proofs-8: p8_sweep1, its task_exit end captured)
    void obj_kind162_turn(uint32_t obj);                                       // 0x530A0
    void kind163_door(uint32_t obj);                                           // 0x963F0 (kind 163)
    void kind163_door_wait(uint32_t obj);                                      // 0x96440
    void kind163_door_open(uint32_t obj);                                      // 0x96490
    void kind163_door_open_wait(uint32_t obj);                                 // 0x964D0
    void kind163_door_open_now(uint32_t obj);                                  // 0x96510
    void kind163_door_draw(uint32_t obj);                                      // 0x96540
    void kind164_door_burst(uint32_t obj);                                     // 0x96660 (kind 164) (proofs-8: p8_sweep1, its task_exit end captured)
    void kind164_wing(uint32_t obj);                                           // 0x96760
    void kind164_wing_wait(uint32_t obj);                                      // 0x967B0
    void kind164_wing_fly(uint32_t obj);                                       // 0x969D0
    void kind164_door_model_draw(uint32_t obj);                                // 0x96A80
    void kind164_debris(uint32_t obj);                                         // 0x96B40
    void kind166_door(uint32_t obj);                                           // 0x96C80 (kind 166)
    void kind167_door(uint32_t obj);                                           // 0x96DC0 (kind 167)
    // --- kinds 173, 180 ---
    void kind173_list(uint32_t obj);                                           // 0x35E0 (kind 173)
    void kind173_list_frame(uint32_t obj);                                     // 0x3650
    void kind180_room_object(uint32_t obj);                                    // 0x2F50 (kind 180)
    void kind180_room_object_frame(uint32_t obj);                              // 0x2FC0
    void obj_kind180_draw(uint32_t obj);                                       // 0x32B0
    // --- title 172, chapter card 177, scene skip 178 ---
    void kind172_opening_title(uint32_t obj);                                  // 0x9AFB0 (kind 172)
    void kind172_title_wait(uint32_t obj);                                     // 0x9B040
    void kind177_chapter_card(uint32_t obj);                                   // 0x9B0E0 (kind 177)
    void kind177_chapter_card_frame(uint32_t obj);                             // 0x9B1C0
    void kind178_scene_skip(uint32_t obj);                                     // 0x9C230 (kind 178)
    void kind178_scene_skip_frame(uint32_t obj);                               // 0x9C250 (proofs-8: p8_sweep2p, captured with its size 0x68: the index stops before its last ret)
    void kind178_scene_skip_end(uint32_t obj);                                 // 0x9C2C0 (proofs-8: p8_sweep1, its task_exit end captured)
    // boundaries only the scene objects call (the 2D layer: drawn by the engine)
    virtual void sprite_draw_anim(uint32_t layer, int a, int b, int x, int y, uint32_t sprite) {}   // 0x17E40
    virtual void sprite_draw_clipped(uint32_t layer, int a, int b, int c, int d, int e, int x0, int y0, int y1, uint32_t sprite) {}   // 0x17F80
    virtual void tile_text_draw(int x, int y, uint32_t text) {}                // 0x9C450
    virtual void voice_message_frame_sub(int x, int y, uint32_t text) {}       // 0x9C610
    virtual void gun_flash_on(uint32_t layer) {}                               // 0x18600
    virtual void gun_flash_off(uint32_t layer) {}                              // 0x18630
    virtual void mem_free(uint32_t p);                                         // 0x149B0 (tasks.cpp)
    void pm_sin_1b(uint32_t angle, Tgp::Out& o);                               // 0x1B or the PC's table sine
