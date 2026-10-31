// Cases of runRoutine (the arcade: routine address -> the port), included inside its switch: the players' character
// bodies (player_body.cpp, R4). The runtime's dispatch includes it (CMake list), hotd_proof too (proofs-2).
    case 0x95530: a.player_char_models_set(); return true;
    case 0x4B810:   // camdr_mode2_body
    case 0x4B820:   // camdr_mode3_body
    case 0x4B830:   // cctb33_mode4_body
    case 0x4B840:   // cctb33_body_follow
    case 0x4B850: a.player_body_at_camera(regs[0]); return true;
    case 0x4B980: a.player_body_place(regs[0]); return true;
    case 0x6240: a.player_body_draw_camdr(regs[0]); return true;
    case 0x62C0: a.player_body_camdr(regs[0]); return true;
    case 0x6330: a.player_body_hold(regs[0]); return true;
    case 0x4BAB0: a.player_scene00_kousya_run(regs[0]); return true;
    case 0x4BAF0: a.player_scene00_kousya_run_p(regs[0]); return true;
    case 0x4BBA0: a.player_scene00_kousya_run_p_p1(regs[0]); return true;
    case 0x4BBE0: a.player_scene00_kousya_run_p_p1_p(regs[0]); return true;
    case 0x4BD00: a.player_scene00_turn(regs[0]); return true;
    case 0x4BD70: a.player_scene00_turn_end(regs[0]); return true;
    case 0x4BDB0: a.player_scene00_hold(regs[0]); return true;
    case 0x4C0D0: a.player_scene02_shoot_three(regs[0]); return true;
    case 0x4C110: a.player_scene02_shoot_three_p(regs[0]); return true;
    case 0x4BFB0: a.player_scene02_shoot_three_p_sub(regs[0]); return true;
    case 0x4C210: a.player_scene02_shoot_three_p_sub_p(regs[0]); return true;
    case 0x4C280: a.player_scene02_shoot_three_p_sub_p_p(regs[0]); return true;
    case 0x4C320: a.player_scene02_shoot_three_p_sub_p_p_p(regs[0]); return true;
    case 0x4C390: return true;   // player_scene02_shoot_three_p_p: idle
    // T1: the branch events' scenes 03-07
    case 0x4C3A0: a.player_scene03_ladder(regs[0]); return true;
    case 0x4C500: a.player_scene03_ladder_p1(regs[0]); return true;
    case 0x4C540: return true;   // player_scene03_ladder_p2: idle
    case 0x4C550: a.player_scene04_fall_backside(regs[0]); return true;
    case 0x4C6B0: return true;   // player_scene04_fall_backside_p1: idle
    case 0x4C6C0: a.player_scene04_fall_backside_p2(regs[0]); return true;
    case 0x4C700: a.player_scene05_knocked_flying(regs[0]); return true;
    case 0x4C860: a.player_scene05_knocked_flying_p(regs[0]); return true;
    case 0x4C8E0: a.player_scene05_knocked_flying_p_p(regs[0]); return true;
    case 0x4C960: a.player_scene05_knocked_flying_p_p_p(regs[0]); return true;
    case 0x4C980: a.player_scene06_reload_shoot(regs[0]); return true;
    case 0x4CAD0: a.player_scene06_reload_shoot_p1(regs[0]); return true;
    case 0x4CB70: a.player_scene06_reload_shoot_p2(regs[0]); return true;
    case 0x4CBB0: a.player_scene06_reload_shoot_p2_p(regs[0]); return true;
    case 0x4CBF0: return true;   // player_scene_idle
    case 0x4CC00: a.player_scene07_jump_down_a(regs[0]); return true;
    case 0x4CD60: a.player_scene07_jump_down_a_p1(regs[0]); return true;
    case 0x4CDA0: a.player_scene07_jump_down_a_p1_p(regs[0]); return true;
    case 0x4CDF0: a.player_scene07_jump_down_a_p1_p_p(regs[0]); return true;
    case 0x4CE70: a.player_scene07_jump_down_a_p1_p_p_p(regs[0]); return true;
    case 0x4CED0: return true;   // player_scene07_jump_down_a_p2: idle
    case 0x4CEE0: a.player_scene08_finish_pose(regs[0]); return true;
    case 0x4CFB0: a.player_scene08_finish_pose_p(regs[0]); return true;
    case 0x4CFF0: a.player_scene08_finish_pose_p_p(regs[0]); return true;
    case 0x4D050: a.player_scene08_finish_pose_p_p_p(regs[0]); return true;
    case 0x4D090: a.player_scene08_finish_pose_p_p_p_p(regs[0]); return true;
    // Chapter 2's scenes 09-14 (UNVERIFIED)
    case 0x4D0B0: a.player_scene09_stance_shoot(regs[0]); return true;
    case 0x4D1A0: a.player_scene09_stance_shoot_p1(regs[0]); return true;
    case 0x4D240: a.player_scene09_stance_shoot_p1_p(regs[0]); return true;
    case 0x4D280: a.player_scene09_stance_shoot_p1_p_p(regs[0]); return true;
    case 0x4D2C0: a.player_scene09_stance_shoot_p1_p_p_p(regs[0]); return true;
    case 0x4D2E0: return true;   // player_scene09_stance_shoot_p2: idle
    case 0x4D2F0: a.player_scene10_jump_down_b(regs[0]); return true;
    case 0x4D450: a.player_scene11_jump_down_c(regs[0]); return true;
    case 0x4D5C0: a.player_scene12_reload_kick(regs[0]); return true;
    case 0x4D710: a.player_scene13_knocked_flying_b(regs[0]); return true;
    case 0x4D870: a.player_scene14_rattle_door(regs[0]); return true;
    case 0x4D9B0: a.player_scene14_rattle_door_p(regs[0]); return true;
    case 0x4DA20: a.player_scene14_rattle_door_p_p(regs[0]); return true;
