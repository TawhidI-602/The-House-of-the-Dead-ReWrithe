// Boundary overrides of ProofBase, included inside its body (proof/main.cpp): the boundaries only the props
// (props.cpp, prop_kinds.cpp) call.
    void prop_bookshelf_collapse(uint32_t a0) override { if (P::pc || recorded_next(F::prop_bookshelf_collapse)) boundary(F::prop_bookshelf_collapse, {a0}); else hotd::port::Game<P>::prop_bookshelf_collapse(a0); }
    void splash_9f1_spawn(uint32_t a0, uint32_t a1, uint32_t a2) override { if (P::pc || recorded_next(F::splash_9f1_spawn)) boundary(F::splash_9f1_spawn, {a0, a1, a2}); else hotd::port::Game<P>::splash_9f1_spawn(a0, a1, a2); }
    void prop_bookshelf_remains(uint32_t a0) override { if (P::pc || recorded_next(F::prop_bookshelf_remains)) boundary(F::prop_bookshelf_remains, {a0}); else hotd::port::Game<P>::prop_bookshelf_remains(a0); }
    void prop_spurt_spawn(uint32_t a0) override { if (P::pc || recorded_next(F::prop_spurt_spawn)) boundary(F::prop_spurt_spawn, {a0}); else hotd::port::Game<P>::prop_spurt_spawn(a0); }
    void prop_hit_drop(uint32_t a0) override { if (P::pc || recorded_next(F::prop_hit_drop)) boundary(F::prop_hit_drop, {a0}); else hotd::port::Game<P>::prop_hit_drop(a0); }
    void prop_lid_fly(uint32_t a0) override { if (P::pc || recorded_next(F::prop_lid_fly)) boundary(F::prop_lid_fly, {a0}); else hotd::port::Game<P>::prop_lid_fly(a0); }
    uint32_t colour_blend32(uint32_t a0, uint32_t a1, uint32_t a2) override { if (P::pc || recorded_next(F::colour_blend32)) return boundary(F::colour_blend32, {a0, a1, a2}); return hotd::port::Game<P>::colour_blend32(a0, a1, a2); }
    void body_drips_draw(uint32_t a0) override { if (P::pc || recorded_next(F::body_drips_draw)) boundary(F::body_drips_draw, {a0}); else hotd::port::Game<P>::body_drips_draw(a0); }
