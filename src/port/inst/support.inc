// Game<Pc> instantiations (port/game.cpp) of the support routines (decl/support.h). NOT PROVEN, vtable only: the
// virtual members are instantiated because Game<Pc>'s vtable needs them; their PC bodies do nothing and ProofPc replays
// them as boundaries.
template void Game<Pc>::attack_slot_take_waiting(uint32_t);
template void Game<Pc>::attack_slot_take_wide(uint32_t);
template void Game<Pc>::attack_slot_front_take(uint32_t);
template uint32_t Game<Pc>::attack_slot_nearest_object(uint32_t);
template void Game<Pc>::enemy_scripted_hit(uint32_t);
template void Game<Pc>::enemy_wait_camera_stop(uint32_t);
template uint32_t Game<Pc>::floor_probe(uint32_t, uint32_t, uint32_t, uint32_t);
template float Game<Pc>::ground_height(uint32_t);
template void Game<Pc>::jump_arc_set(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, int);
template uint32_t Game<Pc>::jump_arc_step_b(uint32_t);
template uint32_t Game<Pc>::jump_arc_step(uint32_t);
template uint32_t Game<Pc>::jump_arc_velocity(uint32_t);
template uint32_t Game<Pc>::jump_arc_velocity_b(uint32_t);
template uint32_t Game<Pc>::enemy_attack_slot_take(uint32_t);
template void Game<Pc>::enemy_attack_slot_free(uint32_t);
