// Cases of runRoutine (the arcade: routine address -> the port), included inside its switch (proof/main.cpp): kind
// 65, the scene axe zombie (enemy_scene_zombie.cpp).
    case 0x8A400: a.kind65_zombie_scene(regs[0]); return true;
    case 0x8A4F0: a.kind65_frame(regs[0]); return true;
    case 0x8A570: a.kind65_hit_check(regs[0]); return true;
    case 0x8A680: a.kind65_st2_damage(regs[0]); return true;
    case 0x8A830: a.kind65_st3_die(regs[0]); return true;
    case 0x8AA00: a.kind65_st4_walk(regs[0]); return true;
    case 0x8AB60: a.kind65_st5_approach(regs[0]); return true;
    case 0x8AD90: a.kind65_st6_pick_target(regs[0]); return true;
    case 0x8AE20: a.kind65_st7_attack(regs[0]); return true;
    case 0x8B040: a.kind65_scene0_walk_chop(regs[0]); return true;
    case 0x8B2E0: a.kind65_scene1_stance_chop(regs[0]); return true;
    case 0x8B470: a.kind65_scene2_pull_out(regs[0]); return true;
    case 0x8B8F0: a.kind65_scene3_dash_chop(regs[0]); return true;
    case 0x8BCC0: a.kind65_scene4_pull_out(regs[0]); return true;
