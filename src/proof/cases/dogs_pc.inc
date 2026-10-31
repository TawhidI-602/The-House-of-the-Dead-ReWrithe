// Cases of runRoutinePc (the PC: its F:: address -> the port), included inside its switch (proof/main.cpp): kind 97,
// the Kenfis dogs (enemy_dogs.cpp).
    case F::kind97_ddog_kenfis: a.kind97_ddog_kenfis(regs[0]); return true;
    case F::dog_set0_pack: a.dog_set0_pack(regs[0]); return true;
    case F::dog_set1_run_in: a.dog_set1_run_in(regs[0]); return true;
    case F::dog_set2_jump_in: a.dog_set2_jump_in(regs[0]); return true;
    case F::dog_set3: a.dog_set3(regs[0]); return true;
    case F::dog_set4_look_around: a.dog_set4_look_around(regs[0]); return true;
    case F::dog_motion_by_state: a.dog_motion_by_state(regs[0]); return true;
    case F::dog_set1_frame: a.dog_set1_frame(regs[0]); return true;
    case F::dog_set2_frame: a.dog_set2_frame(regs[0]); return true;
    case F::dog_frame: a.dog_frame(regs[0]); return true;
    case F::dog_dust: a.dog_dust(regs[0]); return true;
    case F::dog_hits: a.dog_hits(regs[0]); return true;
    case F::dog_set3_frame: a.dog_set3_frame(regs[0]); return true;
    case F::dog_st0_bite: a.dog_st0_bite(regs[0]); return true;
    case F::dog_st2_leap: a.dog_st2_leap(regs[0]); return true;
    case F::dog_st1: a.dog_st1(regs[0]); return true;
    case F::dog_st3_circle: a.dog_st3_circle(regs[0]); return true;
    case F::dog_set4_frame: a.dog_set4_frame(regs[0]); return true;
    case F::dog_pack_list_build: a.dog_pack_list_build(); return true;
    case F::dog_motion_set: a.dog_motion_set(regs[0], int32_t(regs[2]), int32_t(regs[3]), int32_t(regs[1])); return true;
    case F::dog_knockback: a.dog_knockback(regs[0], hotd::port::ffrom(a.mem.u32(regs[4] + 4))); return true;
    case F::dog_st2_move:
        a.dog_st2_move(regs[0], hotd::port::ffrom(a.mem.u32(regs[4] + 4)), hotd::port::ffrom(a.mem.u32(regs[4] + 8)),
                       hotd::port::ffrom(a.mem.u32(regs[4] + 12)));
        return true;
    case F::dog_angle_to_target:
        a.dog_angle_to_target(regs[0], hotd::port::ffrom(a.mem.u32(regs[4] + 4)), hotd::port::ffrom(a.mem.u32(regs[4] + 8)),
                              hotd::port::ffrom(a.mem.u32(regs[4] + 12)));
        return true;
    case F::dog_st3_steer:
        a.dog_st3_steer(regs[0], hotd::port::ffrom(a.mem.u32(regs[4] + 4)), hotd::port::ffrom(a.mem.u32(regs[4] + 8)),
                        hotd::port::ffrom(a.mem.u32(regs[4] + 12)));
        return true;
    case F::dog_set3_fall: {
        float v[6];
        for (int k = 0; k < 6; ++k)
            v[k] = hotd::port::ffrom(a.mem.u32(regs[4] + 4 + 4 * k));
        a.dog_set3_fall(v[0], v[1], v[2], v[3], v[4], v[5], a.mem.u32(regs[4] + 28));
        return true;
    }
    case F::dog_set4_path:
        a.dog_set4_path(regs[0], hotd::port::ffrom(a.mem.u32(regs[4] + 4)), hotd::port::ffrom(a.mem.u32(regs[4] + 8)),
                        hotd::port::ffrom(a.mem.u32(regs[4] + 12)), hotd::port::ffrom(a.mem.u32(regs[4] + 16)));
        return true;
