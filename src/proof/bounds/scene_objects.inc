// Boundary overrides of ProofBase, included inside its body (proof/main.cpp): the boundaries only the hostages
// (hostages.cpp) and the scene objects (scene_objects.cpp) call; kinds 18 / 17 (hangedman_flyby.cpp) have none of
// their own.
    void turn_motion_set(uint32_t a0, uint32_t a1, int a2, int a3) override { if (P::pc || recorded_next(F::turn_motion_set)) boundary(F::turn_motion_set, {a0, a1, a2, a3}); else hotd::port::Game<P>::turn_motion_set(a0, a1, a2, a3); }
    uint32_t task_create_child_alloc(uint32_t a0, uint32_t a1, uint32_t a2) override { if (P::pc || recorded_next(F::task_create_child_alloc)) return boundary(F::task_create_child_alloc, {a0, a1, a2}); return hotd::port::Game<P>::task_create_child_alloc(a0, a1, a2); }
    void sprite_draw_anim(uint32_t a0, int a1, int a2, int a3, int a4, uint32_t a5) override { boundary(F::sprite_draw_anim, {a0, a1, a2, a3, a4, a5}); }
    void sprite_draw_clipped(uint32_t a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, uint32_t a9) override { boundary(F::sprite_draw_clipped, {a0, a1, a2, a3, a4, a5, a6, a7, a8, a9}); }
    void tile_text_draw(int a0, int a1, uint32_t a2) override { boundary(F::tile_text_draw, {a0, a1, a2}); }
    void voice_message_frame_sub(int a0, int a1, uint32_t a2) override { boundary(F::voice_message_frame_sub, {a0, a1, a2}); }
    void gun_flash_on(uint32_t a0) override { boundary(F::gun_flash_on, {a0}); }
    void gun_flash_off(uint32_t a0) override { boundary(F::gun_flash_off, {a0}); }
    void palette_grey_build() override { if (P::pc || recorded_next(F::palette_grey_build)) boundary(F::palette_grey_build, {}); else hotd::port::Game<P>::palette_grey_build(); }
    void vblank_callback_add(uint32_t a0) override { if (P::pc || recorded_next(F::vblank_callback_add)) boundary(F::vblank_callback_add, {a0}); else hotd::port::Game<P>::vblank_callback_add(a0); }
    void mem_free(uint32_t a0) override { if (P::pc || recorded_next(F::mem_free)) boundary(F::mem_free, {a0}); else hotd::port::Game<P>::mem_free(a0); }
