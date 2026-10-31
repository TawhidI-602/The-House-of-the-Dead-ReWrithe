// Cases of runRoutine (the arcade: routine address -> the port), included inside its switch (proof/main.cpp): kinds
// 16, 78, 170 (sophie.cpp).
    case 0x95EE0: a.sophie(regs[0]); return true;
    case 0x95F50: a.sophie_wait_scene(regs[0]); return true;
    case 0x95FB0: a.sophie_stand_up(regs[0]); return true;
    case 0x96020: a.sophie_writhe(regs[0]); return true;
    case 0x960A0: a.sophie_wait_struck(regs[0]); return true;
    case 0x96110: a.sophie_fall(regs[0]); return true;
    case 0x96190: a.sophie_wait_lying(regs[0]); return true;
    case 0x961F0: a.sophie_lying_b(regs[0]); return true;
    case 0x96230: a.sophie_wait_lying_b(regs[0]); return true;
    case 0x96280: a.sophie_lying_c(regs[0]); return true;
    case 0x962E0: a.sophie_lying_d(regs[0]); return true;
    case 0x96350: a.sophie_wait_frame37(regs[0]); return true;
    case 0x963A0: a.sophie_wait_end(regs[0]); return true;
    case 0x310B0: a.kind78_delay(regs[0]); return true;
    case 0x3520: a.kind170_glass_break(regs[0]); return true;
