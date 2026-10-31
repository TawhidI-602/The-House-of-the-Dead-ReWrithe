// Cases of runRoutine (the arcade: routine address -> the port), included inside its switch (proof/main.cpp):
// Chapter 3's kind 128 prop sub-types (props_ch3.cpp).
    case 0x3F0D0: a.prop_ch3_machine(regs[0]); return true;
    case 0x3F3A0: a.prop094_r3_window(regs[0]); return true;
    case 0x3F3F0: a.prop096_koba(regs[0]); return true;
    case 0x3FA80: a.prop121_kikai_item(regs[0]); return true;
    case 0x3FAE0: a.prop121_kikai_item_run(regs[0]); return true;
