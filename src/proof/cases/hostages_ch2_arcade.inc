// Cases of runRoutine (the arcade: routine address -> the port), included inside its switch (proof/main.cpp):
// Chapter 2's hostage scenarios (hostages_ch2.cpp).
    case 0x56FE0: a.hostage_s07_crawl(regs[0]); return true;
    case 0x57010: a.hostage_s07_wait(regs[0]); return true;
    case 0x57090: a.hostage_s08_curled(regs[0]); return true;
    case 0x57110: a.hostage_s08_wait(regs[0]); return true;
    case 0x571F0: a.hostage_s08_rescue(regs[0]); return true;
    case 0x57510: a.hostage_s09_flee_thank(regs[0]); return true;
    case 0x57540: a.hostage_s09_wait(regs[0]); return true;
    case 0x575B0: a.hostage_s09_thank(regs[0]); return true;
    case 0x576B0: a.hostage_s10_walk_off(regs[0]); return true;
    case 0x57710: a.hostage_s10_wait(regs[0]); return true;
    case 0x57780: a.hostage_s10_rescue(regs[0]); return true;
    case 0x57930: a.hostage_s11_reluctant_gift(regs[0]); return true;
    case 0x579A0: a.hostage_s11_wait(regs[0]); return true;
    case 0x57A20: a.hostage_s11_rescue(regs[0]); return true;
