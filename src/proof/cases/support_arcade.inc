// Cases of runRoutine (the arcade: routine address -> the port), included inside its switch (proof/main.cpp): the
// support routines (decl/support.h: attack positions, scripted hits and deaths, floors, jump arcs).
    case 0x31E60: a.attack_slot_take_waiting(regs[0]); return true;
    case 0x31D80: a.attack_slot_take_wide(regs[0]); return true;
    case 0x32010: a.attack_slot_front_take(regs[0]); return true;
    case 0x2FF90: a.attack_slot_nearest_object(regs[0]); return true;
    case 0x32970: a.attack_slots_sort(); return true;
    case 0x32440: a.attack_slots_update(); return true;
    case 0x535F0: a.enemy_scripted_hit(regs[0]); return true;
    case 0x1C670: a.enemy_scripted_dead(regs[0]); return true;
    case 0x1C780: a.enemy_scripted_dead_fall(regs[0]); return true;
    case 0x77350: a.enemy_wait_camera_stop(regs[0]); return true;
    case 0x78FC0: a.Zombie_Dead(regs[0]); return true;
    case 0x76E50: a.Zombie_Dead_frame(regs[0]); return true;
    case 0x45FE0: a.floor_probe(regs[0], regs[1], regs[2], regs[3]); return true;
    case 0x461D0: a.ground_height(regs[0]); return true;
    case 0x2F2F0: a.jump_arc_set(regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], int32_t(regs[7])); return true;
    case 0x2F330: a.jump_arc_step_b(regs[0]); return true;
    case 0x2F400: a.jump_arc_step(regs[0]); return true;
    case 0x2F560: a.jump_arc_velocity(regs[0]); return true;
    case 0x2F610: a.jump_arc_velocity_b(regs[0]); return true;
    case 0x6A8A0: a.obj_move_heading(regs[0]); return true;
    case 0x32160: a.enemy_attack_slot_take(regs[0]); return true;
    case 0x32C50: a.enemy_attack_slot_free(regs[0]); return true;
