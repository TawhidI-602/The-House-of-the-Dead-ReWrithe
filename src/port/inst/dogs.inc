// The PC instantiations (included by port/game.cpp; a routine is listed once its PC captures match): kind 97, the
// Kenfis dogs (enemy_dogs.cpp).
template void Game<Pc>::kind97_ddog_kenfis(uint32_t);
template void Game<Pc>::dog_set0_pack(uint32_t);
template void Game<Pc>::dog_set1_run_in(uint32_t);
template void Game<Pc>::dog_set2_jump_in(uint32_t);
template void Game<Pc>::dog_set3(uint32_t);
template void Game<Pc>::dog_set4_look_around(uint32_t);
template void Game<Pc>::dog_motion_by_state(uint32_t);
template void Game<Pc>::dog_set1_frame(uint32_t);
template void Game<Pc>::dog_set2_frame(uint32_t);
template void Game<Pc>::dog_frame(uint32_t);
template void Game<Pc>::dog_dust(uint32_t);
template void Game<Pc>::dog_hits(uint32_t);
template void Game<Pc>::dog_set3_frame(uint32_t);
template void Game<Pc>::dog_st0_bite(uint32_t);
template void Game<Pc>::dog_st2_leap(uint32_t);
template void Game<Pc>::dog_st1(uint32_t);
template void Game<Pc>::dog_st3_circle(uint32_t);
template void Game<Pc>::dog_set4_frame(uint32_t);
template int32_t Game<Pc>::dog_pack_list_build();
template void Game<Pc>::dog_motion_set(uint32_t, int32_t, int32_t, int32_t);
template void Game<Pc>::dog_set_motion_step(uint32_t, int32_t, int32_t, int32_t, int32_t);
template void Game<Pc>::dog_knockback(uint32_t, float);
template void Game<Pc>::dog_st2_move(uint32_t, float, float, float);
template uint32_t Game<Pc>::dog_angle_to_target(uint32_t, float, float, float);
template uint32_t Game<Pc>::dog_st3_steer(uint32_t, float, float, float);
template float Game<Pc>::dog_set3_fall(float, float, float, float, float, float, uint32_t);
template uint32_t Game<Pc>::dog_set4_path(uint32_t, float, float, float, float);
