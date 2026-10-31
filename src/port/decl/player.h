// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): the player block
// (player.cpp, teaser L3). Arcade forms only so far.
    // --- the player block (player.cpp) ---
    virtual void player_hit(int player, int shake, int side);                 // 0x6410 (the runtime still overrides it)
    void player_hit_lose_life(int player, int shake, int side, int force, int frames);   // 0x6520 (proofs-6: p6_chlives, hostage shots)
    uint32_t player_life_add(int player);                                     // 0x6960
    void player_state_set(int state, int setRoutine, uint32_t task);          // 0x6190
    void player_camdr_run(uint32_t task);                                     // 0x6220
    void player_in_game_frame(uint32_t task);                                 // 0x50B0
    // helpers without an original routine of their own (inline in player_hit / player_hit_lose_life)
    int32_t player_hit_check(int player);
    void player_hit_take(int player, int shake, int side);
    void player_hit_keep_life(int player);
    // boundaries only the player block calls (the HUD: drawn by the engine)
    virtual void hud_ammo_draw(uint32_t task);                                 // 0x949F0 (hud_state.cpp)
    virtual void hud_player_full_draw(uint32_t task);                          // 0x94CC0 (hud_state.cpp)
    virtual void hud_sprite107_draw(uint32_t task);                            // 0x94FD0 (hud_state.cpp)
    virtual void hud_player_start_draw(uint32_t task);                         // 0x94670 (hud_state.cpp)
    virtual void hud_lives_draw(uint32_t task);                                // 0x94740 (hud_state.cpp)
    virtual void hud_sprite107_blink(uint32_t task);                           // 0x94F40 (hud_state.cpp)
    virtual void hud_popup_update(uint32_t task);                              // 0x94490 (hud_state.cpp)
