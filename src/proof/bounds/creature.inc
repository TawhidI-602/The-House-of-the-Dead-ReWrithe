// Boundary overrides of ProofBase, included inside its body (proof/main.cpp): the creature core (creature.cpp).
// Arcade: the port runs (A forms proven by captures), except where an older capture recorded the routine as a
// boundary (recorded_next: replayed); PC: no PC forms yet, so the PC proofs still replay them.
    void set_motion(uint32_t a0, uint32_t a1) override { if (P::pc || recorded_next(F::set_motion)) boundary(F::set_motion, {a0, a1}); else hotd::port::Game<P>::set_motion(a0, a1); }
    void set_motion_blend_hold(uint32_t a0, uint32_t a1, int a2, int a3, int a4) override { if (P::pc || recorded_next(F::set_motion_blend_hold)) boundary(F::set_motion_blend_hold, {a0, a1, a2, a3, a4}); else hotd::port::Game<P>::set_motion_blend_hold(a0, a1, a2, a3, a4); }
    void body_second_motion(uint32_t a0, int a1) override { if (P::pc || recorded_next(F::body_second_motion)) boundary(F::body_second_motion, {a0, a1}); else hotd::port::Game<P>::body_second_motion(a0, a1); }
    void body_init(uint32_t a0, int32_t a1, uint32_t a2) override { if (P::pc || recorded_next(F::body_init)) boundary(F::body_init, {a0, a1, a2}); else hotd::port::Game<P>::body_init(a0, a1, a2); }
    void creature_type_setup(uint32_t a0) override { if (P::pc || recorded_next(F::creature_type_setup)) boundary(F::creature_type_setup, {a0}); else hotd::port::Game<P>::creature_type_setup(a0); }
    void body_draw(uint32_t a0) override { if (P::pc || recorded_next(F::body_draw)) boundary(F::body_draw, {a0}); else hotd::port::Game<P>::body_draw(a0); }
    void turn_motion_set_speed(uint32_t a0, uint32_t a1, int a2, int a3, uint32_t a4) override { if (P::pc || recorded_next(F::turn_motion_set_speed)) boundary(F::turn_motion_set_speed, {a0, a1, a2, a3, a4}); else hotd::port::Game<P>::turn_motion_set_speed(a0, a1, a2, a3, a4); }
    void object_draw(uint32_t a0) override { if (P::pc || recorded_next(F::object_draw)) boundary(F::object_draw, {a0}); else hotd::port::Game<P>::object_draw(a0); }
    void set_motion_blend(uint32_t a0, uint32_t a1, int a2, int a3) override { if (P::pc || recorded_next(F::set_motion_blend)) boundary(F::set_motion_blend, {a0, a1, a2, a3}); else hotd::port::Game<P>::set_motion_blend(a0, a1, a2, a3); }
    void obj_clear_37c(uint32_t a0) override { if (P::pc || recorded_next(F::obj_clear_37c)) boundary(F::obj_clear_37c, {a0}); else hotd::port::Game<P>::obj_clear_37c(a0); }
    void set_motion2(uint32_t a0, uint32_t a1) override { if (P::pc || recorded_next(F::set_motion2)) boundary(F::set_motion2, {a0, a1}); else hotd::port::Game<P>::set_motion2(a0, a1); }
    void set_motion2_at_frame(uint32_t a0, uint32_t a1, int a2, int a3) override { if (P::pc || recorded_next(F::set_motion2_at_frame)) boundary(F::set_motion2_at_frame, {a0, a1, a2, a3}); else hotd::port::Game<P>::set_motion2_at_frame(a0, a1, a2, a3); }
    void set_motion2_blend(uint32_t a0, int a1) override { if (P::pc || recorded_next(F::set_motion2_blend)) boundary(F::set_motion2_blend, {a0, a1}); else hotd::port::Game<P>::set_motion2_blend(a0, a1); }
    void blood_drip_at(float a0, float a1, float a2) override { if (P::pc || recorded_next(F::blood_drip_at)) boundary(F::blood_drip_at, {a0, a1, a2}); else hotd::port::Game<P>::blood_drip_at(a0, a1, a2); }
    void skin_draw(uint32_t a0) override { boundary(F::skin_draw, {a0}); }
    void sophie_skin_draw(uint32_t a0) override { if (P::pc || recorded_next(F::sophie_skin_draw)) boundary(F::sophie_skin_draw, {a0}); else hotd::port::Game<P>::sophie_skin_draw(a0); }
