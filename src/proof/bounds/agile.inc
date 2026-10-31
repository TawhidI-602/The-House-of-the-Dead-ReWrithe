// Boundary overrides of ProofBase, included inside its body (proof/main.cpp): the boundaries only kind 70, the
// agile creatures (enemy_agile.cpp) calls.
    uint32_t jump_arc_velocity(uint32_t a0) override { if (P::pc || recorded_next(F::jump_arc_velocity)) { return boundary(F::jump_arc_velocity, {a0}); } return hotd::port::Game<P>::jump_arc_velocity(a0); }
    uint32_t jump_arc_velocity_b(uint32_t a0) override { if (P::pc || recorded_next(F::jump_arc_velocity_b)) { return boundary(F::jump_arc_velocity_b, {a0}); } return hotd::port::Game<P>::jump_arc_velocity_b(a0); }
    uint32_t room_bounds_bounce(uint32_t a0) override { if (P::pc || recorded_next(F::room_bounds_bounce)) return boundary(F::room_bounds_bounce, {a0}); return hotd::port::Game<P>::room_bounds_bounce(a0); }
    uint32_t room_ceiling_bounce(uint32_t a0) override { if (P::pc || recorded_next(F::room_ceiling_bounce)) return boundary(F::room_ceiling_bounce, {a0}); return hotd::port::Game<P>::room_ceiling_bounce(a0); }
    void floor_splat_spawn(uint32_t a0) override { if (P::pc || recorded_next(F::floor_splat_spawn)) boundary(F::floor_splat_spawn, {a0}); else hotd::port::Game<P>::floor_splat_spawn(a0); }
    void impact_effect_spawn(float a0, float a1, float a2, int a3) override { if (P::pc || recorded_next(F::impact_effect_spawn)) boundary(F::impact_effect_spawn, {a0, a1, a2, a3}); else hotd::port::Game<P>::impact_effect_spawn(a0, a1, a2, a3); }
    void pc_matrix_translate10(float a0, float a1, float a2) override { boundary(F::pc_matrix_translate10, {a0, a1, a2}); }
    void pc_matrix_rotate_y10(uint32_t a0) override { boundary(F::pc_matrix_rotate_y10, {a0}); }
    void pc_matrix_rotate_b10(uint32_t a0) override { boundary(F::pc_matrix_rotate_b10, {a0}); }
    void pc_matrix_rotate_x10(uint32_t a0) override { boundary(F::pc_matrix_rotate_x10, {a0}); }
    void pc_matrix_mul_stored10(uint32_t a0) override { boundary(F::pc_matrix_mul_stored10, {a0}); }
    // ported routines that older captures (k70_full1 - full3) recorded as boundaries: replayed there
    void kind70_spawn0_jump_in(uint32_t a0) override { if (recorded_next(F::kind70_spawn0_jump_in)) boundary(F::kind70_spawn0_jump_in, {a0}); else hotd::port::Game<P>::kind70_spawn0_jump_in(a0); }
    void kind70_spawn6(uint32_t a0) override { if (recorded_next(F::kind70_spawn6)) boundary(F::kind70_spawn6, {a0}); else hotd::port::Game<P>::kind70_spawn6(a0); }
    void kind70_spawn5_waiting(uint32_t a0) override { if (recorded_next(F::kind70_spawn5_waiting)) boundary(F::kind70_spawn5_waiting, {a0}); else hotd::port::Game<P>::kind70_spawn5_waiting(a0); }
    void kind70_spawn2_eating(uint32_t a0) override { if (recorded_next(F::kind70_spawn2_eating)) boundary(F::kind70_spawn2_eating, {a0}); else hotd::port::Game<P>::kind70_spawn2_eating(a0); }
    void kind70_spawn8_wall_attack(uint32_t a0) override { if (recorded_next(F::kind70_spawn8_wall_attack)) boundary(F::kind70_spawn8_wall_attack, {a0}); else hotd::port::Game<P>::kind70_spawn8_wall_attack(a0); }
    void kind70_spawn10_stance_bite(uint32_t a0) override { if (recorded_next(F::kind70_spawn10_stance_bite)) boundary(F::kind70_spawn10_stance_bite, {a0}); else hotd::port::Game<P>::kind70_spawn10_stance_bite(a0); }
    void kind70_spawn11_hanging(uint32_t a0) override { if (recorded_next(F::kind70_spawn11_hanging)) boundary(F::kind70_spawn11_hanging, {a0}); else hotd::port::Game<P>::kind70_spawn11_hanging(a0); }
    void kind70_think_rubin(uint32_t a0) override { if (recorded_next(F::kind70_think_rubin)) boundary(F::kind70_think_rubin, {a0}); else hotd::port::Game<P>::kind70_think_rubin(a0); }
    void kind70_think_moody(uint32_t a0) override { if (recorded_next(F::kind70_think_moody)) boundary(F::kind70_think_moody, {a0}); else hotd::port::Game<P>::kind70_think_moody(a0); }
    void kind70_think_drake(uint32_t a0) override { if (recorded_next(F::kind70_think_drake)) boundary(F::kind70_think_drake, {a0}); else hotd::port::Game<P>::kind70_think_drake(a0); }
    void kind70_think_kageta(uint32_t a0) override { if (recorded_next(F::kind70_think_kageta)) boundary(F::kind70_think_kageta, {a0}); else hotd::port::Game<P>::kind70_think_kageta(a0); }
    void kind70_spawn9_fade_in(uint32_t a0) override { if (recorded_next(F::kind70_spawn9_fade_in)) boundary(F::kind70_spawn9_fade_in, {a0}); else hotd::port::Game<P>::kind70_spawn9_fade_in(a0); }
    void agile_hand_model_restore(uint32_t a0) override { if (recorded_next(F::agile_hand_model_restore)) boundary(F::agile_hand_model_restore, {a0}); else hotd::port::Game<P>::agile_hand_model_restore(a0); }
