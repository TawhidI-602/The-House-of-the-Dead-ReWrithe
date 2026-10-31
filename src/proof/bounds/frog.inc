// Boundary overrides of ProofBase, included inside its body (proof/main.cpp): the boundaries only kind 69, the frog
// (enemy_frog.cpp) calls.
    void pc_matrix_translate5(float a0, float a1, float a2) override { boundary(F::pc_matrix_translate5, {a0, a1, a2}); }
    void pc_matrix_rotate_b2(uint32_t a0) override { boundary(F::pc_matrix_rotate_b2, {a0}); }
    void pc_matrix_rotate_x(uint32_t a0) override { boundary(F::pc_matrix_rotate_x, {a0}); }
    void jump_arc_set(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6, int a7) override { if (P::pc || recorded_next(F::jump_arc_set)) { boundary(F::jump_arc_set, {a0, bitsf(a1), bitsf(a2), bitsf(a3), bitsf(a4), bitsf(a5), bitsf(a6), a7}); } else hotd::port::Game<P>::jump_arc_set(a0, a1, a2, a3, a4, a5, a6, a7); }
    uint32_t jump_arc_step_b(uint32_t a0) override { if (P::pc || recorded_next(F::jump_arc_step_b)) { return boundary(F::jump_arc_step_b, {a0}); } return hotd::port::Game<P>::jump_arc_step_b(a0); }
    void hop_dust_spawn(float a0, float a1, float a2) override { if (P::pc || recorded_next(F::hop_dust_spawn)) boundary(F::hop_dust_spawn, {a0, a1, a2}); else hotd::port::Game<P>::hop_dust_spawn(a0, a1, a2); }
    void frog_splash_spawn(uint32_t a0) override { if (P::pc || recorded_next(F::frog_splash_spawn)) boundary(F::frog_splash_spawn, {a0}); else hotd::port::Game<P>::frog_splash_spawn(a0); }
    uint32_t jump_arc_step(uint32_t a0) override { if (P::pc || recorded_next(F::jump_arc_step)) { return boundary(F::jump_arc_step, {a0}); } return hotd::port::Game<P>::jump_arc_step(a0); }
