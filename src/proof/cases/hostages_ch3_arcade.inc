// Cases of runRoutine (the arcade: routine address -> the port), included inside its switch (proof/main.cpp):
// Chapter 3's hostage scenarios (hostages_ch3.cpp).
    case 0x57C80: a.hostage_s12_cell(regs[0]); return true;
    case 0x57CE0: a.hostage_s12_wait(regs[0]); return true;
    case 0x57DB0: a.hostage_s12_rescue(regs[0]); return true;
    case 0x57EF0: a.hostage_s13_platform_gift(regs[0]); return true;
    case 0x57F70: a.hostage_s13_wait(regs[0]); return true;
    case 0x57FF0: a.hostage_s13_rescue(regs[0]); return true;
    case 0x58320: a.hostage_s14_message_route(regs[0]); return true;
    case 0x58380: a.hostage_s14_wait(regs[0]); return true;
    case 0x58410: a.hostage_s14_rescue(regs[0]); return true;
    case 0x58650: a.hostage_s15_gift(regs[0]); return true;
    case 0x586B0: a.hostage_s15_wait(regs[0]); return true;
    case 0x58730: a.hostage_s15_rescue(regs[0]); return true;
