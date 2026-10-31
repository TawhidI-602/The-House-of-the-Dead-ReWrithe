// Game<Pc> instantiations (port/game.cpp) of Chapter 2's kinds (decl/ch2_kinds.h). NOT PROVEN, vtable only: the
// virtual members are instantiated because Game<Pc>'s vtable needs them; their PC bodies do nothing and ProofPc replays
// them as boundaries.
template void Game<Pc>::enemy_fall_death_check(uint32_t);
template void Game<Pc>::prop_hit_drop(uint32_t);
template void Game<Pc>::prop_lid_fly(uint32_t);
template void Game<Pc>::obj_velocity_clear(uint32_t);
template void Game<Pc>::area_heading_pick(uint32_t, uint32_t, uint32_t, uint32_t);
template void Game<Pc>::frog_splash_spawn(uint32_t);
template void Game<Pc>::point_clip_to_depth(uint32_t, uint32_t, float);
template void Game<Pc>::prop_bookshelf_collapse(uint32_t);
template void Game<Pc>::prop_bookshelf_remains(uint32_t);
