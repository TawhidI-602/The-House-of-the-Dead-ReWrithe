// Declarations of class Game, included inside its body (port/game.h, docs/port.md "Adding a kind"): the camera
// (camera.cpp): the camera task and its CCTB job ring, the jobs, the CAMDR / CCTB 33 camera modes, the game camera at
// the stops, path evaluation and the view matrices.
    // --- the camera (camera.cpp) ---
    uint32_t cam_rec(int n) const { return G::camera_records + uint32_t(n) * 0x154; }   // camera record n
    void cam_setf(uint32_t a, float v) { mem.set32(a, fbits(v)); }
    Tgp::Out cam_copro(int n, std::initializer_list<uint32_t> in);             // one coprocessor call, inputs in order
    Tgp::Out cam_angles(float x, float y, float z);                              // 0x0F, or PC Mode's pc_vec_angles
    void cam_job_end(int slot);                                                // inline in every job: the job is done
    void camera_call(uint32_t routine, uint32_t arg);                          // callx through the camera's tables
    // writes to the geometrizer's ports (0x800000 + n x 0x10 commands, 0x804000 its FIFO: projection, light vector);
    // not work RAM: the engine's renderer takes them
    virtual void geo_write(uint32_t port, uint32_t value) {}
    void camera_matrices_update();                                             // 0x2ABD0
    void camera_chapter_reset();                                               // 0x2B040 UNVERIFIED (teaser)
    void camera_task(uint32_t task);                                           // 0x2B100
    void cam_next_job();                                                       // 0x2B180 UNVERIFIED (teaser)
    void camera_run_jobs_follow();                                             // 0x2B230
    void camera_run_jobs();                                                    // 0x2B2F0
    void camera_task_start();                                                  // 0x2B3A0
    void camera_slot2_follow();                                                // 0x2B470
    void camera_frame_mode4();                                                 // 0x2B740
    void camera_frame_mode6();                                                 // 0x2B890
    void game_cam_stop_update();                                               // 0x2B980
    void game_cam_nop();                                                       // 0x2BAA0 UNVERIFIED (teaser)
    void game_cam_clear();                                                     // 0x2BAC0
    void game_cam_clear_turn_back();                                           // 0x2BAE0
    void game_cam_clear_now();                                                 // 0x2BD90
    void game_cam_look_at_attackers();                                         // 0x2BDD0
    void game_cam_idle();                                                      // 0x2C050
    void game_cam_turn_to_point();                                             // 0x2C060 UNVERIFIED (teaser)
    void camera_frame_mode7();                                                 // 0x2C450
    void camera_frame_mode7_look();                                            // 0x2C490
    void game_cam_path_eye();                                                  // 0x2C530
    void game_cam_position_hold();                                             // 0x2C5C0
    void game_cam_path_look();                                                 // 0x2C5F0
    void game_cam_attackers_midpoint();                                        // 0x2C680
    void game_cam_stop_reset();                                                // 0x2C8A0
    void cam_slot_overhead(int slot);                                          // 0x2C930
    void cam_job20(int slot);                                                  // 0x2C990
    void cam_job20_wait(int slot);                                             // 0x2C9C0
    void cam_job20_nop(int slot);                                              // 0x2CA20
    void cam_job32_offset(int slot);                                           // 0x2CA30 UNVERIFIED (teaser)
    void cam_job64_path(int slot);                                             // 0x2CAD0
    void cam_job64_jump(int slot);                                             // 0x2CB20
    void cam_job64_defer(int slot);                                            // 0x2CBD0
    void cam_job64_play_start(int slot);                                       // 0x2CC90
    void cam_job64_play(int slot);                                             // 0x2CD70
    void cam_job17_resetup(int slot);                                          // 0x2CE50
    void cam_job33_mode(int slot);                                             // 0x2CEF0
    void cam_job18_player_scene(int slot);                                     // 0x2CF80
    void cam_job21_set(int slot);                                              // 0x2D000
    void cam_job16_player_flag(int slot);                                      // 0x2D060
    void cam_job19_preset(int slot);                                           // 0x2D100
    void camera_look_at(uint32_t rec, uint32_t point, uint32_t roll);          // 0x2D190
    void game_cam_look_at_attackers_sub();                                     // 0x2D240
    void camera_globals_reset();                                               // 0x2D350
    void camera_path_eval(uint32_t path, uint32_t frame, uint32_t eye, uint32_t look, uint32_t roll, uint32_t zoom);   // 0x2D4E0 (frame: float bits)
    void unused_2d5d0(uint32_t request);                                       // 0x2D5D0 UNVERIFIED (teaser)
    void unused_2d600();                                                       // 0x2D600 UNVERIFIED (teaser)
    void camera_records_clear();                                               // 0x2D610
    void camera_target_clear();                                                // 0x2D660
    void camera_mode_routine_clear();                                          // 0x2D670
    void camdr_mode1();                                                        // 0x2D690
    void camdr_mode2();                                                        // 0x2D6F0 UNVERIFIED (teaser)
    void camdr_mode3();                                                        // 0x2D770
    void cctb33_mode4();                                                       // 0x2D7E0
    void cctb33_mode5();                                                       // 0x2D860 UNVERIFIED (teaser)
    void cctb33_mode6();                                                       // 0x2D8E0
    void cctb33_mode7();                                                       // 0x2D960
    void ChgCamMode_undefined();                                               // 0x2DA00 UNVERIFIED (teaser)
    void matrix_identity_set(uint32_t pitch, uint32_t yaw, uint32_t out);      // 0x6C1F0
    void camera_reset();                                                       // 0x6CA60 (proofs-7: p7_chtasks)
    void camera_mode_run();                                                    // 0x6CAB0
    void camdr_mode3_frame();                                                  // 0x6CAD0
    void cctb33_mode4_start();                                                 // 0x6CAE0
    void cctb33_mode4_idle();                                                  // 0x6CB20
    void cctb33_mode5_start();                                                 // 0x6CB30 UNVERIFIED (teaser)
    void cctb33_mode5_frame();                                                 // 0x6CC20 UNVERIFIED (teaser)
    void cctb33_mode5_impulse();                                               // 0x6CC40 UNVERIFIED (teaser)
    void cctb33_mode6_start();                                                 // 0x6CE60
    void cctb33_mode6_walk();                                                  // 0x6CE90
    void cctb33_mode7_start();                                                 // 0x6CF90
    void cctb33_mode7_walk();                                                  // 0x6CFB0
    void camdr_mode3_view_pos(int slot);                                       // 0x6D2B0
    void camdr_mode1_frame();                                                  // 0x6D990
