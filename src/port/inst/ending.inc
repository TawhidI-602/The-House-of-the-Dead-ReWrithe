// The Game<Pc> forms of the ending (ending.cpp), included by game.cpp: only its virtual members (vtable; arcade forms
// only, the PC forms are not ported).
template void Game<Pc>::game_ending_finish();
template int32_t Game<Pc>::ending_body_type(uint32_t);
template int32_t Game<Pc>::score_rank_get(int, int);
template void Game<Pc>::palette_curve_fade_in_start();
