// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): Chapter 4's own
// kinds (ch4_kinds.cpp). Arcade forms only; proven on A (ch4p_full / ch4p_size / vf_c4c) but where marked UNVERIFIED (teaser) in ch4_kinds.cpp.
    void kind191_route_by_rescues(uint32_t obj);                              // 0x4D50 (kind 191)
    void event_model_fade(uint32_t obj);                                      // 0x31390 (kind 22)
    void kind154_prop_group(uint32_t obj);                                    // 0x3B020 (kind 154)
    void kind25_kyurianb_curien(uint32_t obj);                                // 0x87F10 (kind 25)
    void kyurianb_draw(uint32_t obj, bool step);                              // inline (the states' common end)
    void kyurianb_wait_scene(uint32_t obj);                                   // 0x87F80
    void kyurianb_wait_point(uint32_t obj);                                   // 0x87FB0
    void kyurianb_pointing(uint32_t obj);                                     // 0x88000
    void kyurianb_wait_struck(uint32_t obj);                                  // 0x88050
    void kyurianb_struck(uint32_t obj);                                       // 0x88090
    void kyurianb_collapse_a(uint32_t obj);                                   // 0x880D0
    void kyurianb_collapse_b(uint32_t obj);                                   // 0x88120
    void kyurianb_collapse_c(uint32_t obj);                                   // 0x88180
    void kyurianb_wait_end(uint32_t obj);                                     // 0x881C0
    // the player's Chapter 4 cut-scene actions
    void player_scene17_dash(uint32_t task);                                  // 0x4DF50
    void player_scene17_dash_p1(uint32_t task);                               // 0x4E040
    void player_scene_nop(uint32_t task);                                     // 0x4E0A0 / 0x4E150 / 0x4E270 (ret)
    void player_scene17_dash_p2(uint32_t task);                               // 0x4E0B0
    void player_scene17_dash_p2_p(uint32_t task);                             // 0x4E100
    void player_body_at_scene_pos(uint32_t task);                             // 0x4E160
    void player_body_at_scene_pos_wait(uint32_t task);                        // 0x4E220
