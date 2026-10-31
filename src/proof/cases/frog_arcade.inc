// Cases of runRoutine (the arcade: routine address -> the port), included inside its switch (proof/main.cpp): kind
// 69, the frog (enemy_frog.cpp).
    case 0x88210: a.kind69_frog_bouere(regs[0]); return true;
    case 0x88230: a.kind69_frog_body_init(regs[0]); return true;
    case 0x88280: a.kind69_frog_frame(regs[0]); return true;
    case 0x88370: a.kind69_frog_st1_hit(regs[0]); return true;
    case 0x883D0: a.kind69_frog_st2_die(regs[0]); return true;
    case 0x885E0: a.kind69_frog_st3_hop(regs[0]); return true;
    case 0x88A50: a.kind69_frog_st4_hop_in_place(regs[0]); return true;
    case 0x88E30: a.kind69_frog_st5_leap_attack(regs[0]); return true;
    case 0x892B0: a.kind69_frog_setup0_swimmers(regs[0]); return true;
    case 0x89600: a.kind69_frog_setup1_pose(regs[0]); return true;
    case 0x89930: a.kind69_frog_setup2_leap_in(regs[0]); return true;
    case 0x89BC0: a.kind69_frog_setup3_swim_out(regs[0]); return true;
    case 0x89FE0: a.kind69_frog_setup4_swim_out_far(regs[0]); return true;
