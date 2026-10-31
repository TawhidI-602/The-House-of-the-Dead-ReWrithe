// Cases of runRoutinePc (the PC: its F:: address -> the port), included inside its switch (proof/main.cpp): kind 65,
// the scene axe zombie (enemy_scene_zombie.cpp).
    case F::kind65_zombie_scene: a.kind65_zombie_scene(regs[0]); return true;
    case F::kind65_frame: a.kind65_frame(regs[0]); return true;
    case F::kind65_hit_check: a.kind65_hit_check(regs[0]); return true;
    case F::kind65_st2_damage: a.kind65_st2_damage(regs[0]); return true;
    case F::kind65_st3_die: a.kind65_st3_die(regs[0]); return true;
    case F::kind65_st4_walk: a.kind65_st4_walk(regs[0]); return true;
    case F::kind65_st5_approach: a.kind65_st5_approach(regs[0]); return true;
    case F::kind65_st6_pick_target: a.kind65_st6_pick_target(regs[0]); return true;
    case F::kind65_st7_attack: a.kind65_st7_attack(regs[0]); return true;
    case F::kind65_scene0_walk_chop: a.kind65_scene0_walk_chop(regs[0]); return true;
    case F::kind65_scene1_stance_chop: a.kind65_scene1_stance_chop(regs[0]); return true;
    case F::kind65_scene2_pull_out: a.kind65_scene2_pull_out(regs[0]); return true;
    case F::kind65_scene3_dash_chop: a.kind65_scene3_dash_chop(regs[0]); return true;
    case F::kind65_scene4_pull_out: a.kind65_scene4_pull_out(regs[0]); return true;
