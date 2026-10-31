// Boundary overrides of ProofBase, included inside its body (proof/main.cpp): the boundaries of the event script
// interpreter (event.cpp).
    uint32_t is_demo() override { if (P::pc || recorded_next(F::is_demo)) return boundary(F::is_demo, {}); return hotd::port::Game<P>::is_demo(); }
    void shadow_light_reset(uint32_t a0, uint32_t a1) override { if (P::pc || recorded_next(F::shadow_light_reset)) boundary(F::shadow_light_reset, {a0, a1}); else hotd::port::Game<P>::shadow_light_reset(a0, a1); }
    void color_row_set(int32_t a0, int32_t a1) override { boundary(F::color_row_set, {a0, a1}); }
    void texture_area_load(uint32_t a0, int32_t a1) override { boundary(F::texture_area_load, {a0, a1}); }
    void rand_seed(uint32_t a0) override { boundary(F::rand_seed, {a0}); }
    void sound_queue_reset() override { boundary(F::sound_queue_reset, {}); }
    void sound_send3(uint32_t a0) override { boundary(F::sound_send3, {a0}); }
    void pc_chapter_files_load(int32_t a0, int32_t a1) override { boundary(F::pc_chapter_files_load, {a0, a1}); }
    void pc_music_fade_ending() override { boundary(F::pc_music_fade_ending, {}); }
    void pc_music_fade_chapter() override { boundary(F::pc_music_fade_chapter, {}); }
