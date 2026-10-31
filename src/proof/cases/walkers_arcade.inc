// Cases of runRoutine (the arcade: routine address -> the port), included inside its switch (proof/main.cpp): kinds
// 96, 98, 160 (enemy_walkers.cpp).
    case 0x7EDE0: a.kind096_zombie_walker(regs[0]); return true;
    case 0x7EC60: a.kind096_walk_frame(regs[0]); return true;
    case 0x7EC30: a.kind096_fall_frame(regs[0]); return true;
    case 0x55B00: a.enemy_guard_b(regs[0]); return true;
    case 0x3970: a.kind160_splash(regs[0]); return true;
    case 0x39A0: a.kind160_splash_anim(regs[0]); return true;
