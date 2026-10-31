// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): Chapter 3's own
// kinds (ch3_kinds.cpp, worker "ch3own"). Arcade forms only. Proven on A (ch3own_full: the
// full-game recording, 0 differ) but where marked UNVERIFIED (teaser).
    // former core boundaries (still virtual so the proof's boundary overrides stay)
    virtual uint32_t iron_ball_in_reach(uint32_t obj);                        // 0x49DA0
    virtual void iron_ball_hand_pos(uint32_t obj, int node, uint32_t motion, int frame, uint32_t out);   // 0x49B60 (PC: + the table 0x5D80D8; vtable only)
    // the iron ball (kind 32 action 4)
    void iron_ball_pos_idle(uint32_t obj, int mode, int step);                // 0x47C50
    void iron_ball_follow(uint32_t obj);                                      // 0x47C70
    void iron_ball_pos_swing(uint32_t obj, uint32_t motion);                  // 0x47E70
    void iron_ball_pos_attack(uint32_t obj);                                  // 0x48170 proven (vf_c3a)
    void iron_ball_pos_hit(uint32_t obj);                                     // 0x484D0 UNVERIFIED (teaser): not reached
    void pm_ball_ahead(uint32_t angle, Tgp::Out& v);                          // PC Mode: the iron ball's PC helpers (pmch34)
    float pm_ball_dist(uint32_t obj);
    void iron_ball_pos_pull_sub();                                            // 0x469F0
    void iron_ball_pos_pull(uint32_t obj);                                    // 0x48690
    void iron_ball_pos_6(uint32_t obj);                                       // 0x48920 proven (vf_c3a)
    void iron_ball_pos_7(uint32_t obj);                                       // 0x489F0
    void iron_ball_drop(uint32_t obj);                                        // 0x48A50 proven (vf_c3a)
    void iron_ball_hitbox(uint32_t obj);                                      // 0x48C50 proven (vf_c3a)
    void iron_ball_draw(uint32_t obj);                                        // 0x48CF0
    void iron_ball_node_pos(uint32_t obj, uint32_t motion, int frame, uint32_t out);   // 0x49B80
    void body_node_pos_at_frame(uint32_t obj, int type, uint32_t motion, int frame, uint32_t chain, uint32_t out);   // 0x49BB0
    // kind 106
    void kind106_padlock(uint32_t obj);                                       // 0x5C0C0 (kind 106)
    void kind106_lock_spark(uint32_t obj);                                    // 0x5C8B0
    // kind 189
    void kind189_lock_task(uint32_t obj);                                     // 0x40B0
    void kind189_locked_door(uint32_t obj);                                   // 0x4240 (kind 189)
    // kind 153
    void kind153_taran(uint32_t obj);                                         // 0x638D0 (kind 153)
    void kind153_taran_frame(uint32_t obj);                                   // 0x63AC0 (+ its Ghidra split kind153_taran_reach_check 0x64224)
    void kind153_taran_leave(uint32_t obj);                                   // inline (the frame's common end)
    void kind153_taran_hit_player(uint32_t obj);                              // inline
    void kind153_taran_fall_off(uint32_t obj);                                // 0x64650
    // kinds 186 / 188
    void kind186_moving_object(uint32_t obj);                                 // 0x3D80 (kind 186)
    void kind186_moving_object_frame(uint32_t obj);                           // 0x3DD0
    void kind188_route_spot(uint32_t obj);                                    // 0x3E90 (kind 188)
    void kind188_spark(uint32_t obj);                                         // 0x4E30 proven (vf_c3a)
    void kind188_spark_anim(uint32_t obj);                                    // 0x4D90 proven (vf_c3a)
    // the player's Chapter 3 cut-scene actions
    void player_scene15_jump_down_kb(uint32_t task);                          // 0x4DA40
    void player_scene15_jump_down_kb_p1(uint32_t task);                       // 0x4DB30
    void player_scene15_jump_down_kb_p1_p(uint32_t task);                     // 0x4DB90
    void player_scene15_jump_down_kb_p2(uint32_t task);                       // 0x4DBB0 proven (vf_c3b)
    void player_scene15_jump_down_kb_p2_p(uint32_t task);                     // 0x4DC00 proven (vf_c3b)
    void player_scene15_jump_down_kb_p2_p_p(uint32_t task);                   // 0x4DC60 proven (vf_c3b)
    void player_scene16_run_jump(uint32_t task);                              // 0x4DC80
    void player_scene16_run_jump_p(uint32_t task);                            // 0x4DE00
    void player_scene16_run_jump_p_p(uint32_t task);                          // 0x4DE50
    void player_scene16_run_jump_p_p_p(uint32_t task);                        // 0x4DEA0
    void player_scene16_run_jump_p2(uint32_t task);                           // 0x4DED0 proven (vf_c3b)
    void player_scene16_run_jump_p2_p(uint32_t task);                         // 0x4DF20 proven (vf_c3b)
    virtual void part_hit_class17(uint32_t part);                             // 0x43860 (was a core boundary; the proof keeps its override)
    void part_class17_anim(uint32_t t);                                       // 0x450B0
    virtual uint32_t room_ceiling_bounce(uint32_t obj);                     // 0x2FD30 (was an agile.h boundary; 1 = hit, else -1)
    void impact_effect(uint32_t t);                                           // 0x30C00
    virtual void impact_effect_spawn(float x, float y, float z, int surface);   // 0x30C90 (was an agile.h boundary)
    // the iron ball's chain
    virtual void iron_chain_start(uint32_t owner, uint32_t ball);             // 0x490E0 (was a core boundary; the proof keeps its override) proven (ch4p_full)
    void iron_chain_init(uint32_t t, float hx, float hy, float hz, float bx, float by, float bz, float len, float link);   // 0x491F0 proven (ch4p_full)
    void iron_chain_task(uint32_t t);                                         // 0x48F20 proven (ch4p_full)
    void iron_chain_slack(uint32_t t);                                        // 0x49050 proven (ch4p_full)
    void iron_chain_constrain_fwd(uint32_t t, int32_t from, int32_t to);      // 0x49980 proven (ch4p_full)
    void iron_chain_constrain_back(uint32_t t, int32_t from, int32_t to);     // 0x49A70 proven (ch4p_full)
    void iron_chain_simulate(uint32_t t);                                     // 0x495C0 proven (ch4p_full)
