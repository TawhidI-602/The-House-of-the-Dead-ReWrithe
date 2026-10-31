// Boundary overrides of ProofBase, included inside its body (proof/main.cpp): kind 37 (the Chariot): the HUD layer.
    void sprite_bank_load(int a0, int a1) override { if (P::pc || recorded_next(F::sprite_bank_load)) boundary(F::sprite_bank_load, {a0, a1}); else hotd::port::Game<P>::sprite_bank_load(a0, a1); }
    void sprite_bank_free(int a0) override { if (P::pc || recorded_next(F::sprite_bank_free)) boundary(F::sprite_bank_free, {a0}); else hotd::port::Game<P>::sprite_bank_free(a0); }
    void sprite_bank_tiles_load(uint32_t a0, int a1) override { if (P::pc || recorded_next(F::sprite_bank_tiles_load)) boundary(F::sprite_bank_tiles_load, {a0, a1}); else hotd::port::Game<P>::sprite_bank_tiles_load(a0, a1); }
    void palette_queue_add(int32_t a0, int32_t a1) override { boundary(F::palette_queue_add, {a0, a1}); }
    void sprite_place(uint32_t a0, int a1, int a2, int a3, int a4, int a5) override { boundary(F::sprite_place, {a0, a1, a2, a3, a4, a5}); }
    void sprite_draw(uint32_t a0, int a1, int a2, int a3) override { boundary(F::sprite_draw, {a0, a1, a2, a3}); }
    void life_bar_init(int a0, int a1, int a2) override { if (P::pc || recorded_next(F::life_bar_init)) boundary(F::life_bar_init, {a0, a1, a2}); else hotd::port::Game<P>::life_bar_init(a0, a1, a2); }
    int32_t life_bar_update(int32_t a0, int a1) override { if (P::pc || recorded_next(F::life_bar_update)) return int32_t(boundary(F::life_bar_update, {a0, a1})); return hotd::port::Game<P>::life_bar_update(a0, a1); }
