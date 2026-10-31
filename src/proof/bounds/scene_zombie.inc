// Boundary overrides of ProofBase, included inside its body (proof/main.cpp): the boundaries only kind 65, the scene
// axe zombie (enemy_scene_zombie.cpp) calls.
    void enemy_kill_score(uint32_t a0, int a1) override { if (P::pc || recorded_next(F::enemy_kill_score)) boundary(F::enemy_kill_score, {a0, a1}); else hotd::port::Game<P>::enemy_kill_score(a0, a1); }
    uint32_t attack_slot_nearest_object(uint32_t a0) override { if (P::pc || recorded_next(F::attack_slot_nearest_object)) { return boundary(F::attack_slot_nearest_object, {a0}); } return hotd::port::Game<P>::attack_slot_nearest_object(a0); }
