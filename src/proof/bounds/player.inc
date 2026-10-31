// Boundary overrides of ProofBase, included inside its body (proof/main.cpp): the player block (player.cpp). Arcade:
// player_hit runs in the port (replayed where an older capture recorded it as a boundary); PC: no PC form yet,
// replayed.
    void player_hit(int a0, int a1, int a2) override { if (P::pc || recorded_next(F::player_hit)) boundary(F::player_hit, {a0, a1, a2}); else hotd::port::Game<P>::player_hit(a0, a1, a2); }
    void hud_ammo_draw(uint32_t a0) override { if (P::pc || recorded_next(F::hud_ammo_draw)) boundary(F::hud_ammo_draw, {a0}); else hotd::port::Game<P>::hud_ammo_draw(a0); }
    void hud_player_full_draw(uint32_t a0) override { if (P::pc || recorded_next(F::hud_player_full_draw)) boundary(F::hud_player_full_draw, {a0}); else hotd::port::Game<P>::hud_player_full_draw(a0); }
    void hud_sprite107_draw(uint32_t a0) override { if (P::pc || recorded_next(F::hud_sprite107_draw)) boundary(F::hud_sprite107_draw, {a0}); else hotd::port::Game<P>::hud_sprite107_draw(a0); }
    void hud_player_start_draw(uint32_t a0) override { if (P::pc || recorded_next(F::hud_player_start_draw)) boundary(F::hud_player_start_draw, {a0}); else hotd::port::Game<P>::hud_player_start_draw(a0); }
    void hud_lives_draw(uint32_t a0) override { if (P::pc || recorded_next(F::hud_lives_draw)) boundary(F::hud_lives_draw, {a0}); else hotd::port::Game<P>::hud_lives_draw(a0); }
    void hud_sprite107_blink(uint32_t a0) override { if (P::pc || recorded_next(F::hud_sprite107_blink)) boundary(F::hud_sprite107_blink, {a0}); else hotd::port::Game<P>::hud_sprite107_blink(a0); }
    void hud_popup_update(uint32_t a0) override { if (P::pc || recorded_next(F::hud_popup_update)) boundary(F::hud_popup_update, {a0}); else hotd::port::Game<P>::hud_popup_update(a0); }
    void player_waiting_frame(uint32_t a0) override { if (P::pc || recorded_next(F::player_waiting_frame)) boundary(F::player_waiting_frame, {a0}); else hotd::port::Game<P>::player_waiting_frame(a0); }
