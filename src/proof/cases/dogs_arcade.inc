// Cases of runRoutine (the arcade: routine address -> the port), included inside its switch (proof/main.cpp): kind
// 97, the Kenfis dogs (enemy_dogs.cpp).
    case 0x7C9A0: a.kind97_ddog_kenfis(regs[0]); return true;
    case 0x7CB90: a.dog_set0_pack(regs[0]); return true;
    case 0x7CBA0: a.dog_set1_run_in(regs[0]); return true;
    case 0x7CC00: a.dog_set2_jump_in(regs[0]); return true;
    case 0x7CC60: a.dog_set3(regs[0]); return true;
    case 0x7CCC0: a.dog_set4_look_around(regs[0]); return true;
    case 0x7CE20: a.dog_motion_by_state(regs[0]); return true;
    case 0x7D2B0: a.dog_set1_frame(regs[0]); return true;
    case 0x7D330: a.dog_set2_frame(regs[0]); return true;
    case 0x7EA80: a.dog_frame(regs[0]); return true;
    case 0x7EBB0: a.dog_dust(regs[0]); return true;
    case 0x7D960: a.dog_hits(regs[0]); return true;
    case 0x7D460: a.dog_set3_frame(regs[0]); return true;
    case 0x7DCB0: a.dog_st0_bite(regs[0]); return true;
    case 0x7DED0: a.dog_st2_leap(regs[0]); return true;
    case 0x7E270: a.dog_st1(regs[0]); return true;
    case 0x7E620: a.dog_st3_circle(regs[0]); return true;
    case 0x7D780: a.dog_set4_frame(regs[0]); return true;
    case 0x7CD10: a.dog_pack_list_build(); return true;
    case 0x7D080: a.dog_motion_set(regs[0], int32_t(regs[1]), int32_t(regs[2]), int32_t(regs[3])); return true;
    case 0x7D190: a.dog_set_motion_step(regs[0], int32_t(regs[1]), int32_t(regs[2]), int32_t(regs[3]), int32_t(regs[4])); return true;
    case 0x69880: a.dog_knockback(regs[0], f(1)); return true;
    case 0x1CD60: a.dog_st2_move(regs[0], f(1), f(2), f(3)); return true;
    case 0x1D9A0: a.dog_angle_to_target(regs[0], f(1), f(2), f(3)); return true;
    case 0x6A0B0: a.dog_st3_steer(regs[0], f(1), f(2), f(3)); return true;
    case 0x1D710: a.dog_set3_fall(f(0), f(1), f(2), f(3), f(4), f(5), regs[6]); return true;
    case 0x1C820: a.dog_set4_path(regs[0], f(1), f(2), f(3), f(4)); return true;
