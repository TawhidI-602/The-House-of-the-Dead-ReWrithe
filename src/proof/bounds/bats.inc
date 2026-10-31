// Boundary overrides of ProofBase, included inside its body (proof/main.cpp): the boundaries only kinds 66-68, the
// bats (enemy_bats.cpp) calls.
    void geo_matrix_position(uint32_t a0, uint32_t a1) override { boundary(F::geo_matrix_position, {a0, a1}); }
    void pick_target_player_any(uint32_t a0) override { if (P::pc || recorded_next(F::pick_target_player_any)) boundary(F::pick_target_player_any, {a0}); else hotd::port::Game<P>::pick_target_player_any(a0); }
    void pc_draw_state(int a0) override { boundary(F::pc_draw_state, {a0}); }
    void pc_matrix_position_draw(uint32_t a0, uint32_t a1) override { boundary(F::pc_matrix_position_draw, {a0, a1}); }
    void pc_matrix_translate6(float a0, float a1, float a2) override { boundary(F::pc_matrix_translate6, {a0, a1, a2}); }
    void pc_matrix_rotate_y6(uint32_t a0) override { boundary(F::pc_matrix_rotate_y6, {a0}); }
    void pc_matrix_rotate_b3(uint32_t a0) override { boundary(F::pc_matrix_rotate_b3, {a0}); }
    void pc_matrix_rotate_x2(uint32_t a0) override { boundary(F::pc_matrix_rotate_x2, {a0}); }
    void pc_matrix_mul_stored(uint32_t a0) override { boundary(F::pc_matrix_mul_stored, {a0}); }
