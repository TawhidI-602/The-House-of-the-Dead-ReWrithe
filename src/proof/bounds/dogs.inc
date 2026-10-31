// Boundary overrides of ProofBase, included inside its body (proof/main.cpp): the boundaries only kind 97, the Kenfis
// dogs (enemy_dogs.cpp) calls.
    uint32_t pc_crand() override { return boundary(F::crand, {}); }
    void pc_vec_angles9(float a0, float a1, float a2, uint32_t a3, uint32_t a4) override { boundary(F::pc_vec_angles9, {a0, a1, a2, a3, a4}); }
    uint32_t angle_to_camera(uint32_t a0) override { if (P::pc || recorded_next(F::angle_to_camera)) return boundary(F::angle_to_camera, {a0}); return hotd::port::Game<P>::angle_to_camera(a0); }
    void pc_matrix_translate9(float a0, float a1, float a2) override { boundary(F::pc_matrix_translate9, {a0, a1, a2}); }
    void pc_matrix_rotate_z9(uint32_t a0) override { boundary(F::pc_matrix_rotate_z9, {a0}); }
    void pc_matrix_rotate_y9(uint32_t a0) override { boundary(F::pc_matrix_rotate_y9, {a0}); }
    void pc_matrix_rotate_x9(uint32_t a0) override { boundary(F::pc_matrix_rotate_x9, {a0}); }
    void pc_matrix_translate11(float a0, float a1, float a2) override { boundary(F::pc_matrix_translate11, {a0, a1, a2}); }
    void pc_matrix_rotate_z11(uint32_t a0) override { boundary(F::pc_matrix_rotate_z11, {a0}); }
    void pc_matrix_rotate_x11(uint32_t a0) override { boundary(F::pc_matrix_rotate_x11, {a0}); }
    void pc_matrix_translate12(float a0, float a1, float a2) override { boundary(F::pc_matrix_translate12, {a0, a1, a2}); }
    // ported helpers that older captures (dog_skip, dog_r9b, k32dog_pa) recorded as boundaries: replayed there
    void dog_knockback(uint32_t a0, float a1) override { if (recorded_next(F::dog_knockback)) boundary(F::dog_knockback, {a0, a1}); else hotd::port::Game<P>::dog_knockback(a0, a1); }
    void dog_st2_move(uint32_t a0, float a1, float a2, float a3) override { if (recorded_next(F::dog_st2_move)) boundary(F::dog_st2_move, {a0, a1, a2, a3}); else hotd::port::Game<P>::dog_st2_move(a0, a1, a2, a3); }
    uint32_t dog_angle_to_target(uint32_t a0, float a1, float a2, float a3) override { return recorded_next(F::dog_angle_to_target) ? boundary(F::dog_angle_to_target, {a0, a1, a2, a3}) : hotd::port::Game<P>::dog_angle_to_target(a0, a1, a2, a3); }
    uint32_t dog_st3_steer(uint32_t a0, float a1, float a2, float a3) override { return recorded_next(F::dog_st3_steer) ? boundary(F::dog_st3_steer, {a0, a1, a2, a3}) : hotd::port::Game<P>::dog_st3_steer(a0, a1, a2, a3); }
    float dog_set3_fall(float a0, float a1, float a2, float a3, float a4, float a5, uint32_t a6) override { return recorded_next(F::dog_set3_fall) ? boundary_float(F::dog_set3_fall, {a0, a1, a2, a3, a4, a5, a6}) : hotd::port::Game<P>::dog_set3_fall(a0, a1, a2, a3, a4, a5, a6); }
    uint32_t dog_set4_path(uint32_t a0, float a1, float a2, float a3, float a4) override { return recorded_next(F::dog_set4_path) ? boundary(F::dog_set4_path, {a0, a1, a2, a3, a4}) : hotd::port::Game<P>::dog_set4_path(a0, a1, a2, a3, a4); }
