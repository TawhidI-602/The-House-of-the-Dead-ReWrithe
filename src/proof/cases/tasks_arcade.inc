// Cases of runRoutine (the arcade: routine address -> the port), included inside its switch (proof/main.cpp): the
// heap and the task creation (tasks.cpp, proofs-7).
    case 0x14640: a.heap_reset(); return true;
    case 0x35090: a.skin_buffers_alloc(); return true;
    case 0x14710: a.mem_alloc(regs[0]); return true;
    case 0x14810: a.heap_block_insert(regs[0], regs[1]); return true;
    case 0x149B0: a.mem_free(regs[0]); return true;
    case 0x14A50: a.heap_block_free(regs[0]); return true;
    case 0x14C10: a.task_group_create(regs[0]); return true;
    case 0x14EA0: a.task_create(regs[0], regs[1]); return true;
    case 0x14FA0: a.task_create_child(regs[0], regs[1]); return true;
    case 0x15080: a.task_clear_work(regs[0]); return true;
    // mode 4, the game (game_flow.cpp, proofs-7)
    case 0x25C20: a.mode4_game(); return true;
    case 0x25F40: a.game_play(); return true;
    case 0x262A0: a.game_frame(regs[0]); return true;
    case 0x25FF0: a.game_chapter_end(); return true;
    case 0x26020: a.game_chapter_end_wait(); return true;
    case 0x25D40: a.game_chapter_start(); return true;
    case 0x25DA0: a.chapter_load(regs[0]); return true;
    case 0x26080: a.game_continue_begin(); return true;
    case 0x26100: a.game_continue_countdown(); return true;
    case 0x5230: a.player_continue_begin(int32_t(regs[0])); return true;
    // the chapter's resets and per-frame tasks (chapter_tasks.cpp, proofs-7)
    case 0x453C0: a.shot_lists_clear(); return true;
    case 0x32290: a.chapter_reset_53e1b0(); return true;
    case 0x67480: a.chapter_counters_reset(); return true;
    case 0x31C00: a.attack_slots_task(regs[0]); return true;
    case 0x53480: a.depth_lists_task(regs[0]); return true;
    // the chapter's start table (chapter_tasks.cpp, chstart)
    case 0x34130: a.sky_task_start(); return true;
    case 0x5DA0: a.input_task_start(); return true;
    case 0x5D30: a.InitSetPlayerTask(); return true;
    case 0x5DD0: a.shake_task_start(); return true;
    case 0x4B650: a.world_init(); return true;
    case 0x31D40: a.attack_slots_task_start(); return true;
    case 0x44AF0: a.letterbox_task_start(); return true;
    case 0x53370: a.depth_lists_task_start(); return true;
    case 0x23CA0: a.demo_timer_start(); return true;
    case 0x5260: a.player_st4_out_of_lives(regs[0]); return true;
    case 0x52C0: a.player_continue_start(regs[0]); return true;
    case 0x5300: a.player_continue_frame(regs[0]); return true;
    // kind 176 (chapter_clear.cpp, proofs-7)
    case 0x9B690: a.kind176_score_draw(int32_t(regs[0]), int32_t(regs[1]), int32_t(regs[2]), int32_t(regs[3])); return true;
    case 0x9B880: a.kind176_chapter_clear(regs[0]); return true;
    case 0x9B9F0: a.kind176_results(regs[0]); return true;
    case 0x9BC00: a.kind176_results_hold(regs[0]); return true;
    case 0x9BDC0: a.kind176_results_end(regs[0]); return true;
    // small routines (support_c.cpp, proofs-7)
    case 0x15010: a.task_create_child_alloc(regs[0], regs[1], regs[2]); return true;
    case 0x33EC0: a.spin_axis_from_velocity(regs[0], regs[1]); return true;
    case 0x9B600: a.kind182_grey_scene(regs[0]); return true;
    case 0x9B620: a.kind182_wait(regs[0]); return true;
    case 0x9B660: a.kind182_grey_hold(regs[0]); return true;
    // the players' start states (player_join.cpp, proofs-7)
    case 0x94430: a.hud_popup_clear(regs[0]); return true;
    case 0x4F80: a.player_st1_continue(regs[0]); return true;
    case 0x4FE0: a.player_st2_rejoin(regs[0]); return true;
    case 0x4FF0: a.player_st3_join(regs[0]); return true;
    case 0x5870: a.PlStartInit(regs[0], int32_t(regs[1])); return true;
    case 0x37D80: a.object_draw_body(regs[0]); return true;
    case 0x37B80: a.body_part_draw_mat(regs[0]); return true;
    case 0x61C0: a.player_camdr_mode1(regs[0]); return true;
    case 0x61F0: a.player_camdr_mode4(regs[0]); return true;
    case 0x4B7D0: a.player_camdr_mode3(regs[0]); return true;
    case 0x4B7F0: a.player_camdr2_mode4(regs[0]); return true;
    case 0x6A970: a.tumble_corners(regs[0]); return true;
    case 0x6ADD0: a.tumble_update(regs[0], regs[1]); return true;
    case 0x6AF70: a.tumble_start(regs[0], regs[1]); return true;
    case 0x60430: a.body_drips_draw(regs[0]); return true;
    case 0xDE70: a.set_mode(int32_t(regs[0])); return true;
