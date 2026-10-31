// Cases of runRoutine (the arcade: routine address -> the port), included inside its switch (proof/main.cpp): the
// player block (player.cpp).
    case 0x6410: a.player_hit(int(regs[0]), int(regs[1]), int(regs[2])); return true;
    case 0x6520: a.player_hit_lose_life(int(regs[0]), int(regs[1]), int(regs[2]), int(regs[3]), int(regs[4])); return true;
    case 0x6960: a.player_life_add(int(regs[0])); return true;
    case 0x6190: a.player_state_set(int(regs[0]), int(regs[1]), regs[2]); return true;
    case 0x6220: a.player_camdr_run(regs[0]); return true;
    case 0x50B0: a.player_in_game_frame(regs[0]); return true;
