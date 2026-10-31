// Game<Pc> instantiations (port/game.cpp) of Chapter 3's kinds (decl/ch3_kinds.h). NOT PROVEN, vtable only: the
// virtual members are instantiated because Game<Pc>'s vtable needs them; ProofPc replays them as boundaries.
template uint32_t Game<Pc>::iron_ball_in_reach(uint32_t);
template void Game<Pc>::iron_ball_hand_pos(uint32_t, int, uint32_t, int, uint32_t);
template void Game<Pc>::part_hit_class17(uint32_t);
template uint32_t Game<Pc>::room_ceiling_bounce(uint32_t);
template void Game<Pc>::impact_effect_spawn(float, float, float, int);
template void Game<Pc>::iron_chain_start(uint32_t, uint32_t);
