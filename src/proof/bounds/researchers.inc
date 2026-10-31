// Boundary overrides of ProofBase, included inside its body (proof/main.cpp): the boundaries only kind 64, the
// researchers (enemy_researchers.cpp) calls.
    void researcher_scream_penalty(uint32_t a0, uint32_t a1, uint32_t a2) override { if (P::pc || recorded_next(F::researcher_scream_penalty)) boundary(F::researcher_scream_penalty, {a0, a1, a2}); else hotd::port::Game<P>::researcher_scream_penalty(a0, a1, a2); }
    void researcher_hit_by_player(uint32_t a0) override { if (P::pc || recorded_next(F::researcher_hit_by_player)) boundary(F::researcher_hit_by_player, {a0}); else hotd::port::Game<P>::researcher_hit_by_player(a0); }
    void researcher_rescue_bonus() override { if (P::pc || recorded_next(F::researcher_rescue_bonus)) boundary(F::researcher_rescue_bonus, {}); else hotd::port::Game<P>::researcher_rescue_bonus(); }
    void researcher_gift_spawn(uint32_t a0) override { if (P::pc || recorded_next(F::researcher_gift_spawn)) boundary(F::researcher_gift_spawn, {a0}); else hotd::port::Game<P>::researcher_gift_spawn(a0); }
    uint32_t researcher_reached_camera(uint32_t a0) override { if (P::pc || recorded_next(F::researcher_reached_camera)) return boundary(F::researcher_reached_camera, {a0}); return hotd::port::Game<P>::researcher_reached_camera(a0); }
    void body_role_model_set(uint32_t a0, int a1, uint32_t a2) override { if (P::pc || recorded_next(F::body_role_model_set)) boundary(F::body_role_model_set, {a0, a1, a2}); else hotd::port::Game<P>::body_role_model_set(a0, a1, a2); }
    void voice_message(int a0) override { if (P::pc || recorded_next(F::voice_message)) boundary(F::voice_message, {a0}); else hotd::port::Game<P>::voice_message(a0); }
