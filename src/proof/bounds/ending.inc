// Boundary overrides of ProofBase, included inside its body (proof/main.cpp): the ending (port/ending.cpp).
    void sprite_init() override { boundary(F::sprite_init, {}); }
    void text_layers_clear() override { boundary(F::text_layers_clear, {}); }
    void rank_insert() override { boundary(F::rank_insert, {}); }
    void palette_curve_fade_in_start() override { if (P::pc || recorded_next(F::palette_curve_fade_in_start)) { boundary(F::palette_curve_fade_in_start, {}); return; } hotd::port::Game<P>::palette_curve_fade_in_start(); }
    void player_idle(uint32_t a0) override { boundary(F::player_idle, {a0}); }
    int32_t score_rank_get(int a0, int a1) override { if (P::pc || recorded_next(F::score_rank_get)) return int32_t(boundary(F::score_rank_get, {a0, a1})); return hotd::port::Game<P>::score_rank_get(a0, a1); }
