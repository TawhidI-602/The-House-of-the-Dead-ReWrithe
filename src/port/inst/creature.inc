// The proven Game<Pc> routines of the creature core (creature.cpp), included by game.cpp. None proven yet: no PC
// forms. NOT PROVEN, vtable only: the virtual members below are instantiated because Game<Pc>'s vtable needs them; the
// bodies are the arcade forms. ProofPc replays them as boundaries (proof/bounds/creature.inc), so nothing PC runs them.
template void Game<Pc>::body_init(uint32_t, int32_t, uint32_t);
template void Game<Pc>::creature_type_setup(uint32_t);
template void Game<Pc>::obj_clear_37c(uint32_t);
template void Game<Pc>::set_motion(uint32_t, uint32_t);
template void Game<Pc>::set_motion2(uint32_t, uint32_t);
template void Game<Pc>::set_motion_blend(uint32_t, uint32_t, int, int);
template void Game<Pc>::turn_motion_set_speed(uint32_t, uint32_t, int, int, uint32_t);
template void Game<Pc>::set_motion2_at_frame(uint32_t, uint32_t, int, int);
template void Game<Pc>::set_motion_blend_hold(uint32_t, uint32_t, int, int, int);
template void Game<Pc>::body_second_motion(uint32_t, int);
template void Game<Pc>::set_motion2_blend(uint32_t, int);
template void Game<Pc>::object_draw(uint32_t);
template void Game<Pc>::body_draw(uint32_t);
