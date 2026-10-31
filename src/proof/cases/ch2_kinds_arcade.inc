// Cases of runRoutine (the arcade: routine address -> the port), included inside its switch (proof/main.cpp):
// Chapter 2's own kinds (ch2_kinds.cpp).
    case 0x3A40: a.kind183_shootable_sign(regs[0]); return true;
    case 0x96EC0: a.kind168_double_door(regs[0]); return true;
    case 0x92BE0: a.kind075_sound_repeat(regs[0]); return true;
    case 0x3AAA0: a.kind131_prop_group(regs[0]); return true;
    case 0x39BC0: a.kind133_prop_grid(regs[0]); return true;
    case 0x39F60: a.kind136_prop_group(regs[0]); return true;
    case 0x5FDA0: a.kind132_ebitb_vomit(regs[0]); return true;
    case 0x5FF60: a.kind132_ebitb_vomit_frame(regs[0]); return true;
    case 0x60330: a.kind132_ebitb_dead(regs[0]); return true;
    case 0x603E0: a.kind132_drips_draw(regs[0]); return true;
    case 0x93340: a.enemy_fall_death_check(regs[0]); return true;
    case 0x934B0: a.enemy_fall_death_land(regs[0]); return true;
    case 0x970B0: a.kind187_fridge_door(regs[0]); return true;
    case 0x3B90: a.kind184_lift(regs[0]); return true;
    case 0x3BE0: a.kind184_lift_frame(regs[0]); return true;
    case 0x3CA0: a.kind185_elevator(regs[0]); return true;
    case 0x3CF0: a.kind185_elevator_frame(regs[0]); return true;
    case 0x3AEF0: a.kind151_prop_group(regs[0]); return true;
    case 0x3A500: a.kind152_prop_group(regs[0]); return true;
    case 0x61240: a.kind150_ebitan_slide(regs[0]); return true;
    case 0x613D0: a.kind150_ebitan_slide_frame(regs[0]); return true;
    case 0x61C10: a.kind150_ebitan_slide_dying(regs[0]); return true;
    case 0x30820: a.frog_splash(regs[0]); return true;
    case 0x2E870: a.obj_velocity_clear(regs[0]); return true;
    case 0x2E550: a.area_heading_pick(regs[0], regs[1], regs[2], regs[3]); return true;
    case 0x308B0: a.frog_splash_spawn(regs[0]); return true;
    case 0x34920: { float d; std::memcpy(&d, &regs[2], 4); a.point_clip_to_depth(regs[0], regs[1], d); return true; }
