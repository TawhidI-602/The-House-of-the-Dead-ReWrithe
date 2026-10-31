// Boundary overrides of ProofBase, included inside its body (proof/main.cpp): the boundaries only kinds 33-39, the
// Murrer (enemy_molb.cpp) calls.
    void attack_queue_role(uint32_t a0) override { if (P::pc || recorded_next(F::attack_queue_role)) boundary(F::attack_queue_role, {a0}); else hotd::port::Game<P>::attack_queue_role(a0); }
    void velocity_for_arc(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) override { if (P::pc || recorded_next(F::velocity_for_arc)) boundary(F::velocity_for_arc, {a0, a1, a2, bitsf(a3), bitsf(a4)}); else hotd::port::Game<P>::velocity_for_arc(a0, a1, a2, a3, a4); }
    void obj_velocity_clear(uint32_t a0) override { if (P::pc || recorded_next(F::obj_velocity_clear)) boundary(F::obj_velocity_clear, {a0}); else hotd::port::Game<P>::obj_velocity_clear(a0); }
    void body_part_set_model(uint32_t a0, int a1, uint32_t a2) override { if (P::pc || recorded_next(F::body_part_set_model)) boundary(F::body_part_set_model, {a0, a1, a2}); else hotd::port::Game<P>::body_part_set_model(a0, a1, a2); }
    void area_heading_pick(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3) override { if (P::pc || recorded_next(F::area_heading_pick)) boundary(F::area_heading_pick, {a0, a1, bitsf(a2), a3}); else hotd::port::Game<P>::area_heading_pick(a0, a1, a2, a3); }
    void pc_matrix_translate4(float a0, float a1, float a2) override { boundary(F::pc_matrix_translate4, {a0, a1, a2}); }
