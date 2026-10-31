// Boundary overrides of ProofBase, included inside its body (proof/main.cpp): the boundaries the core or several
// kinds call.
    void shot_flash_spawn(float a0, float a1, float a2, int a3, int a4) override { if (P::pc || recorded_next(F::shot_flash_spawn)) boundary(F::shot_flash_spawn, {a0, a1, a2, a3, a4}); else hotd::port::Game<P>::shot_flash_spawn(a0, a1, a2, a3, a4); }
    void sound_cue(uint32_t a0) override { boundary(F::sound_cue, {a0}); }
    void hud_player_notice_show(uint32_t a0, int a1) override { if (P::pc || recorded_next(F::hud_player_notice_show)) boundary(F::hud_player_notice_show, {a0, a1}); else hotd::port::Game<P>::hud_player_notice_show(a0, a1); }
    void hud_start_prompt(int a0, int a1, int a2) override { if (P::pc || recorded_next(F::hud_start_prompt)) boundary(F::hud_start_prompt, {a0, a1, a2}); else hotd::port::Game<P>::hud_start_prompt(a0, a1, a2); }
    void hud_prompt_anim(uint32_t a0, int a1, int a2, int a3, int a4) override { boundary(a0, {a1, a2, a3, a4}); }
    void set_mode(int a0) override { if (P::pc || recorded_next(F::set_mode)) boundary(F::set_mode, {a0}); else hotd::port::Game<P>::set_mode(a0); }
    void muzzle_flash_spawn(int a0) override { if (P::pc || recorded_next(F::muzzle_flash_spawn)) boundary(F::muzzle_flash_spawn, {a0}); else hotd::port::Game<P>::muzzle_flash_spawn(a0); }
    void player_demo_start(uint32_t a0) override { boundary(F::player_demo_start, {a0}); }
    void player_lesson_start(uint32_t a0) override { boundary(F::player_lesson_start, {a0}); }
    uint32_t player_start_check(uint32_t a0, int a1) override { if (P::pc || recorded_next(F::player_start_check)) return boundary(F::player_start_check, {a0, a1}); return hotd::port::Game<P>::player_start_check(a0, a1); }
    void shot_effect(uint32_t a0) override { if (P::pc || recorded_next(F::shot_effect)) boundary(F::shot_effect, {a0}); else hotd::port::Game<P>::shot_effect(a0); }
    void enemy_fall_death_check(uint32_t a0) override { if (P::pc || recorded_next(F::enemy_fall_death_check)) boundary(F::enemy_fall_death_check, {a0}); else hotd::port::Game<P>::enemy_fall_death_check(a0); }
    void floor_splat_spawn_b(uint32_t a0) override { if (P::pc || recorded_next(F::floor_splat_spawn_b)) boundary(F::floor_splat_spawn_b, {a0}); else hotd::port::Game<P>::floor_splat_spawn_b(a0); }
    void pc_matrix_translate2(float a0, float a1, float a2) override { boundary(F::pc_matrix_translate2, {a0, a1, a2}); }
    void attack_slot_take_waiting(uint32_t a0) override { if (P::pc || recorded_next(F::attack_slot_take_waiting)) { boundary(F::attack_slot_take_waiting, {a0}); } else hotd::port::Game<P>::attack_slot_take_waiting(a0); }
    void enemy_wait_camera_stop(uint32_t a0) override { if (P::pc || recorded_next(F::enemy_wait_camera_stop)) { boundary(F::enemy_wait_camera_stop, {a0}); } else hotd::port::Game<P>::enemy_wait_camera_stop(a0); }
    void pc_gun_input(uint32_t a0, uint32_t a1) override { boundary(F::pc_gun_input, {a0, a1}); }
    void pc_matrix_rotate_b(uint32_t a0) override { boundary(F::pc_matrix_rotate_b, {a0}); }
    void pc_debug_print() override { boundary_noargs(F::pc_debug_print); }
    float pc_view_test(float a0, float a1, float a2, float a3) override { return boundary_float(F::pc_view_test, {a0, a1, a2, a3}); }
    void pc_matrix_translate(float a0, float a1, float a2) override { boundary(F::pc_matrix_translate, {a0, a1, a2}); }
    uint32_t pc_time_ms() override { return boundary(kTimeGetTime, {}); }
    void body_free_cut_parts(uint32_t a0) override { if (P::pc || recorded_next(F::body_free_cut_parts)) boundary(F::body_free_cut_parts, {a0}); else hotd::port::Game<P>::body_free_cut_parts(a0); }
    void part_damage_callback(uint32_t a0, uint32_t a1) override { boundary(a0, {a1}); }
    void eyeball_pop_spawn(int a0, float a1, float a2, float a3) override { if (P::pc || recorded_next(F::eyeball_pop_spawn)) boundary(F::eyeball_pop_spawn, {a0, a1, a2, a3}); else hotd::port::Game<P>::eyeball_pop_spawn(a0, a1, a2, a3); }
    void part_blow_off(int a0, uint32_t a1) override { if (P::pc || recorded_next(F::part_blow_off)) boundary(F::part_blow_off, {a0, a1}); else hotd::port::Game<P>::part_blow_off(a0, a1); }
    void part_hit_spark(uint32_t a0) override { if (P::pc || recorded_next(F::part_hit_spark)) boundary(F::part_hit_spark, {a0}); else hotd::port::Game<P>::part_hit_spark(a0); }
    void part_hit_class17(uint32_t a0) override { if (P::pc || recorded_next(F::part_hit_class17)) boundary(F::part_hit_class17, {a0}); else hotd::port::Game<P>::part_hit_class17(a0); }
    void blood_burst_spawn(uint32_t a0) override { if (P::pc || recorded_next(F::blood_burst_spawn)) boundary(F::blood_burst_spawn, {a0}); else hotd::port::Game<P>::blood_burst_spawn(a0); }
    void part_bleed_start(uint32_t a0) override { if (P::pc || recorded_next(F::part_bleed_start)) boundary(F::part_bleed_start, {a0}); else hotd::port::Game<P>::part_bleed_start(a0); }
    void part_bleed_spawn(uint32_t a0) override { if (P::pc || recorded_next(F::part_bleed_spawn)) boundary(F::part_bleed_spawn, {a0}); else hotd::port::Game<P>::part_bleed_spawn(a0); }
    void enemy_attack_slot_free(uint32_t a0) override { if (P::pc || recorded_next(F::enemy_attack_slot_free)) { boundary(F::enemy_attack_slot_free, {a0}); } else hotd::port::Game<P>::enemy_attack_slot_free(a0); }
    void body_free_parts(uint32_t a0) override { if (P::pc || recorded_next(F::body_free_parts)) boundary(F::body_free_parts, {a0}); else hotd::port::Game<P>::body_free_parts(a0); }
    void enemy_free_extras(uint32_t a0) override { if (P::pc || recorded_next(F::enemy_free_extras)) boundary(F::enemy_free_extras, {a0}); else hotd::port::Game<P>::enemy_free_extras(a0); }
    void task_exit() override { boundary(F::task_exit, {}); }
    uint32_t enemy_attack_slot_take(uint32_t a0) override { if (P::pc || recorded_next(F::enemy_attack_slot_take)) { return boundary(F::enemy_attack_slot_take, {a0}); } return hotd::port::Game<P>::enemy_attack_slot_take(a0); }
    void enemy_target_distance(uint32_t a0) override { if (P::pc || recorded_next(F::enemy_target_distance)) boundary(F::enemy_target_distance, {a0}); else hotd::port::Game<P>::enemy_target_distance(a0); }
    void enemy_slot_take(uint32_t a0) override { if (P::pc || recorded_next(F::enemy_slot_take)) boundary(F::enemy_slot_take, {a0}); else hotd::port::Game<P>::enemy_slot_take(a0); }
    uint32_t task_create_child(uint32_t a0, uint32_t a1) override { if (P::pc || recorded_next(F::task_create_child)) return boundary(F::task_create_child, {a0, a1}); return hotd::port::Game<P>::task_create_child(a0, a1); }
    uint32_t task_create(uint32_t a0, uint32_t a1) override { if (P::pc || recorded_next(F::task_create)) return boundary(F::task_create, {a0, a1}); return hotd::port::Game<P>::task_create(a0, a1); }
    void call_routine(uint32_t a0, uint32_t a1) override { boundary(a0, {a1}); }
    void kill_score_shooter(uint32_t a0, int a1) override { if (P::pc || recorded_next(F::kill_score_shooter)) boundary(F::kill_score_shooter, {a0, a1}); else hotd::port::Game<P>::kill_score_shooter(a0, a1); }
    void shadow_update(uint32_t a0) override { if (P::pc || recorded_next(F::shadow_update)) boundary(F::shadow_update, {a0}); else hotd::port::Game<P>::shadow_update(a0); }
    void spline_segment_init(uint32_t a0, uint32_t a1) override { if (P::pc || recorded_next(F::spline_segment_init)) boundary(F::spline_segment_init, {a0, a1}); else hotd::port::Game<P>::spline_segment_init(a0, a1); }
    void spline_advance(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5) override
    {
        if (P::pc || recorded_next(F::spline_advance))
            boundary(F::spline_advance, {a0, a1, bitsf(a2), bitsf(a3), a4, a5});   // the PC passes the floats on the stack
        else
            hotd::port::Game<P>::spline_advance(a0, a1, a2, a3, a4, a5);
    }
    float ground_height(uint32_t a0) override { if (P::pc || recorded_next(F::ground_height)) { return boundary_float(F::ground_height, {a0}); } return hotd::port::Game<P>::ground_height(a0); }
    uint32_t mem_alloc(uint32_t a0) override { if (P::pc || recorded_next(F::mem_alloc)) return boundary(F::mem_alloc, {a0}); return hotd::port::Game<P>::mem_alloc(a0); }
    void attack_slot_take_wide(uint32_t a0) override { if (P::pc || recorded_next(F::attack_slot_take_wide)) { boundary(F::attack_slot_take_wide, {a0}); } else hotd::port::Game<P>::attack_slot_take_wide(a0); }
    void pick_target_player(uint32_t a0) override { if (P::pc || recorded_next(F::pick_target_player)) boundary(F::pick_target_player, {a0}); else hotd::port::Game<P>::pick_target_player(a0); }
    void pick_target_player_rand(uint32_t a0) override { if (P::pc || recorded_next(F::pick_target_player_rand)) boundary(F::pick_target_player_rand, {a0}); else hotd::port::Game<P>::pick_target_player_rand(a0); }
    void camera_side_set(uint32_t a0) override { if (P::pc || recorded_next(F::camera_side_set)) boundary(F::camera_side_set, {a0}); else hotd::port::Game<P>::camera_side_set(a0); }
    void attack_slot_front_take(uint32_t a0) override { if (P::pc || recorded_next(F::attack_slot_front_take)) { boundary(F::attack_slot_front_take, {a0}); } else hotd::port::Game<P>::attack_slot_front_take(a0); }
    uint32_t combat_active() override { if (P::pc || recorded_next(F::combat_active)) return boundary(F::combat_active, {}); return hotd::port::Game<P>::combat_active(); }
    uint32_t projectile_hit_player(uint32_t a0, int a1, int a2) override { if (P::pc || recorded_next(F::projectile_hit_player)) return boundary(F::projectile_hit_player, {a0, a1, a2}); return hotd::port::Game<P>::projectile_hit_player(a0, a1, a2); }
    uint32_t floor_probe(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3) override { if (P::pc || recorded_next(F::floor_probe)) { return boundary(F::floor_probe, {a0, a1, a2, a3}); } return hotd::port::Game<P>::floor_probe(a0, a1, a2, a3); }
    void draw_fade_begin(int a0) override { boundary(F::draw_fade_begin, {a0}); }
    void player_nop() override { boundary(F::player_nop, {}); }
    void player_continue_nop(uint32_t a0) override { if (recorded_next(F::player_continue_nop)) boundary(F::player_continue_nop, {a0}); else hotd::port::Game<P>::player_continue_nop(a0); }
    void walk_to_attack_position(uint32_t a0, float a1) override { if (recorded_next(F::walk_to_attack_position)) boundary(F::walk_to_attack_position, {a0, a1}); else hotd::port::Game<P>::walk_to_attack_position(a0, a1); }
    void heap_reset() override { if (P::pc || recorded_next(F::heap_reset)) boundary(F::heap_reset, {}); else hotd::port::Game<P>::heap_reset(); }
    uint32_t task_group_create(uint32_t a0) override { if (P::pc || recorded_next(F::task_group_create)) return boundary(F::task_group_create, {a0}); return hotd::port::Game<P>::task_group_create(a0); }
    void task_group_run(uint32_t a0) override { boundary(F::task_group_run, {a0}); }
    void geo_frame_begin() override { boundary(F::geo_frame_begin, {}); }
    void geo_view_setup() override { boundary(F::geo_view_setup, {}); }
    void palette_fade_finish() override { boundary(F::palette_fade_finish, {}); }
    void next_mode() override { boundary(F::next_mode, {}); }
    void bookkeeping_game_end() override { boundary(F::bookkeeping_game_end, {}); }
    void hud_game_over_sprite(int a0) override { if (P::pc || recorded_next(F::hud_game_over_sprite)) boundary(F::hud_game_over_sprite, {a0}); else hotd::port::Game<P>::hud_game_over_sprite(a0); }
    void hud_game_over_sprite_final(int a0) override { if (P::pc || recorded_next(F::hud_game_over_sprite_final)) boundary(F::hud_game_over_sprite_final, {a0}); else hotd::port::Game<P>::hud_game_over_sprite_final(a0); }
    void bookkeeping_play_end(int a0) override { boundary(F::bookkeeping_play_end, {a0}); }
    void play_time_start(int a0) override { boundary(F::play_time_start, {a0}); }
    void play_time_continue(int a0) override { boundary(F::play_time_continue, {a0}); }
    void game_over_timer_tick() override { if (P::pc || recorded_next(F::game_over_timer_tick)) boundary(F::game_over_timer_tick, {}); else hotd::port::Game<P>::game_over_timer_tick(); }
    void tilemap_clear_rect(uint32_t a0, int a1, int a2, int a3, int a4) override { boundary(F::tilemap_clear_rect, {a0, a1, a2, a3, a4}); }
    void geo_send_words(uint32_t a0, uint32_t a1, uint32_t a2) override { boundary(F::geo_send_words, {a0, a1, a2}); }
    void velocity_for_arc_speed(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5) override { if (P::pc || recorded_next(F::velocity_for_arc_speed)) boundary(F::velocity_for_arc_speed, {a0, a1, a2, bitsf(a3), bitsf(a4), a5}); else hotd::port::Game<P>::velocity_for_arc_speed(a0, a1, a2, a3, a4, a5); }
    void task_clear_work(uint32_t a0) override { if (P::pc || recorded_next(F::task_clear_work)) boundary(F::task_clear_work, {a0}); else hotd::port::Game<P>::task_clear_work(a0); }
    void heap_block_insert(uint32_t a0, uint32_t a1) override { if (P::pc || recorded_next(F::heap_block_insert)) boundary(F::heap_block_insert, {a0, a1}); else hotd::port::Game<P>::heap_block_insert(a0, a1); }
    void heap_block_free(uint32_t a0) override { if (P::pc || recorded_next(F::heap_block_free)) boundary(F::heap_block_free, {a0}); else hotd::port::Game<P>::heap_block_free(a0); }
    void text_cursor(int a0, int a1) override { boundary(F::text_cursor, {a0, a1}); }
    void text_printf(uint32_t a0, int32_t) override { boundary(F::text_printf, {a0}); }   // (the argument: not compared)
    uint32_t walk_to(uint32_t a0, float a1, float a2, uint32_t a3, float a4) override { if (P::pc || recorded_next(F::walk_to)) return boundary(F::walk_to, {a0, a1, a2, a3, a4}); return hotd::port::Game<P>::walk_to(a0, a1, a2, a3, a4); }
    void angle_turn_toward(uint32_t a0, uint32_t a1, uint32_t a2) override { if (P::pc || recorded_next(F::angle_turn_toward)) boundary(F::angle_turn_toward, {a0, a1, a2}); else hotd::port::Game<P>::angle_turn_toward(a0, a1, a2); }
    void enemy_scripted_hit(uint32_t a0) override { if (P::pc || recorded_next(F::enemy_scripted_hit)) { boundary(F::enemy_scripted_hit, {a0}); } else hotd::port::Game<P>::enemy_scripted_hit(a0); }
    void move_along_heading(uint32_t a0, float a1) override { if (P::pc || recorded_next(F::move_along_heading)) boundary(F::move_along_heading, {a0, a1}); else hotd::port::Game<P>::move_along_heading(a0, a1); }
    void turn_toward_camera(uint32_t a0, uint32_t a1) override { if (P::pc || recorded_next(F::turn_toward_camera)) boundary(F::turn_toward_camera, {a0, a1}); else hotd::port::Game<P>::turn_toward_camera(a0, a1); }
    void enemy_scripted_to_ai(uint32_t a0) override { if (P::pc || recorded_next(F::enemy_scripted_to_ai)) boundary(F::enemy_scripted_to_ai, {a0}); else hotd::port::Game<P>::enemy_scripted_to_ai(a0); }
    void hit_effect_at(float a0, float a1, float a2) override { if (P::pc || recorded_next(F::hit_effect_at)) boundary(F::hit_effect_at, {a0, a1, a2}); else hotd::port::Game<P>::hit_effect_at(a0, a1, a2); }
    void debug_halt() override
    {
        if (error.empty())
            error = "the port reached the original's endless loop (debug_halt)";
    }
    void shadow_setup(float a0) override { if (P::pc || recorded_next(F::shadow_setup)) boundary(F::shadow_setup, {a0}); else hotd::port::Game<P>::shadow_setup(a0); }
    void geo_draw_model(uint32_t a0) override { boundary(F::geo_draw_model, {a0}); }
    void attacker_list_add(uint32_t a0) override { if (P::pc || recorded_next(F::attacker_list_add)) boundary(F::attacker_list_add, {a0}); else hotd::port::Game<P>::attacker_list_add(a0); }
    void depth_list_add(uint32_t a0) override { if (P::pc || recorded_next(F::depth_list_add)) boundary(F::depth_list_add, {a0}); else hotd::port::Game<P>::depth_list_add(a0); }
    void palette_animation_step(int a0, int a1) override { boundary(F::palette_animation_step, {a0, a1}); }
    void chapter_screen_reset() override { boundary(F::chapter_screen_reset, {}); }
    void hud_mode_init() override { boundary(F::hud_mode_init, {}); }
    void chapter_load(uint32_t a0) override { if (P::pc || recorded_next(F::chapter_load)) boundary(F::chapter_load, {a0}); else hotd::port::Game<P>::chapter_load(a0); }
