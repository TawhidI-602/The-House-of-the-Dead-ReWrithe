// Boundary overrides of ProofBase, included inside its body (proof/main.cpp): the boundaries only kind 32, the
// zombies with a kit (enemy_kit_zombie.cpp and its .inc files) calls.
    void point_clip_to_depth(uint32_t a0, uint32_t a1, float a2) override { if (P::pc || recorded_next(F::point_clip_to_depth)) boundary(F::point_clip_to_depth, {a0, a1, a2}); else hotd::port::Game<P>::point_clip_to_depth(a0, a1, a2); }
    void spin_axis_from_velocity(uint32_t a0, uint32_t a1) override { if (P::pc || recorded_next(F::spin_axis_from_velocity)) boundary(F::spin_axis_from_velocity, {a0, a1}); else hotd::port::Game<P>::spin_axis_from_velocity(a0, a1); }
    void tumble_corners(uint32_t a0) override { if (P::pc || recorded_next(F::tumble_corners)) boundary(F::tumble_corners, {a0}); else hotd::port::Game<P>::tumble_corners(a0); }
    void tumble_update(uint32_t a0, uint32_t a1) override { if (P::pc || recorded_next(F::tumble_update)) boundary(F::tumble_update, {a0, a1}); else hotd::port::Game<P>::tumble_update(a0, a1); }
    void tumble_bounce(uint32_t a0, uint32_t a1) override { boundary(F::tumble_bounce, {a0, a1}); }
    void call_routine_f(uint32_t a0, uint32_t a1, float a2) override { boundary(a0, {a1, a2}); }
    void iron_ball_hand_pos(uint32_t a0, int a1, uint32_t a2, int a3, uint32_t a4) override
    {
        if constexpr (P::pc)
            boundary(F::iron_ball_hand_pos, {a0, a1, a2, a3, uint32_t(0x5D80D8), a4});
        else if (recorded_next(F::iron_ball_hand_pos))
            boundary(F::iron_ball_hand_pos, {a0, a1, a2, a3, a4});
        else
            hotd::port::Game<P>::iron_ball_hand_pos(a0, a1, a2, a3, a4);
    }
    void iron_chain_start(uint32_t a0, uint32_t a1) override { boundary(F::iron_chain_start, {a0, a1}); }
    uint32_t body_node_model(uint32_t a0, int a1) override { if (P::pc || recorded_next(F::body_node_model)) return boundary(F::body_node_model, {a0, a1}); return hotd::port::Game<P>::body_node_model(a0, a1); }
    uint32_t attack_front_busy(uint32_t a0) override { if (P::pc || recorded_next(F::attack_front_busy)) return boundary(F::attack_front_busy, {a0}); return hotd::port::Game<P>::attack_front_busy(a0); }
    void axe_hand_model_hide(uint32_t a0, int a1, uint32_t a2) override { if (P::pc || recorded_next(F::axe_hand_model_hide)) boundary(F::axe_hand_model_hide, {a0, a1, a2}); else hotd::port::Game<P>::axe_hand_model_hide(a0, a1, a2); }
    void pc_matrix_mul8(uint32_t a0) override { boundary(F::pc_matrix_mul8, {a0}); }
    void pc_matrix_translate8(float a0, float a1, float a2) override { boundary(F::pc_matrix_translate8, {a0, a1, a2}); }
    void pc_matrix_rotate_z8(uint32_t a0) override { boundary(F::pc_matrix_rotate_z8, {a0}); }
    void pc_matrix_rotate_x8(uint32_t a0) override { boundary(F::pc_matrix_rotate_x8, {a0}); }
    void pc_matrix_identity8() override { boundary(F::pc_matrix_identity8, {}); }
    void pc_matrix_angles8(uint32_t a0, uint32_t a1, uint32_t a2) override { boundary(F::pc_matrix_angles8, {a0, a1, a2}); }
    uint32_t iron_ball_in_reach(uint32_t a0) override { if (P::pc || recorded_next(F::iron_ball_in_reach)) return boundary(F::iron_ball_in_reach, {a0}); return hotd::port::Game<P>::iron_ball_in_reach(a0); }
    void pc_matrix_identity3() override { boundary(F::pc_matrix_identity3, {}); }
    void pc_matrix_rotate_y8(uint32_t a0) override { boundary(F::pc_matrix_rotate_y8, {a0}); }
    void pc_matrix_rotate_y7(uint32_t a0) override { boundary(F::pc_matrix_rotate_y7, {a0}); }
    float pc_view_test2(float a0, float a1, float a2, float a3) override { return boundary_float(F::pc_view_test2, {a0, a1, a2, a3}); }
    // ported after the PC runs all_pm / all_pa_r9 / all_pm_r9 (2026-10-14) recorded them as boundaries: replayed there
    void kind32_act4_iron_ball(uint32_t a0) override { if (recorded_next(F::kind32_act4_iron_ball)) boundary(F::kind32_act4_iron_ball, {a0}); else hotd::port::Game<P>::kind32_act4_iron_ball(a0); }
    void kind32_act4_recover(uint32_t a0) override { if (recorded_next(F::kind32_act4_recover)) boundary(F::kind32_act4_recover, {a0}); else hotd::port::Game<P>::kind32_act4_recover(a0); }
    void kind32_part_hits(uint32_t a0) override { if (recorded_next(F::kind32_part_hits)) boundary(F::kind32_part_hits, {a0}); else hotd::port::Game<P>::kind32_part_hits(a0); }
    void kind32_axe_release(uint32_t a0, uint32_t a1) override { if (recorded_next(F::kind32_axe_release)) boundary(F::kind32_axe_release, {a0, a1}); else hotd::port::Game<P>::kind32_axe_release(a0, a1); }
