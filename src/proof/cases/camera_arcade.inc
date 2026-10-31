// Cases of runRoutine (the arcade: routine address -> the port), included inside its switch (proof/main.cpp): the
// camera (camera.cpp).
    case 0x2ABD0: a.camera_matrices_update(); return true;
    case 0x2B040: a.camera_chapter_reset(); return true;
    // proofs-8: the camera routines no capture reached through camera_task (reached by the tables only)
    case 0x2B180: a.cam_next_job(); return true;
    case 0x2BAA0: a.game_cam_nop(); return true;
    case 0x2C060: a.game_cam_turn_to_point(); return true;
    case 0x2CA30: a.cam_job32_offset(int(regs[0])); return true;
    case 0x2D6F0: a.camdr_mode2(); return true;
    case 0x2D860: a.cctb33_mode5(); return true;
    case 0x2DA00: a.ChgCamMode_undefined(); return true;
    case 0x6CB30: a.cctb33_mode5_start(); return true;
    case 0x6CC20: a.cctb33_mode5_frame(); return true;
    case 0x6CC40: a.cctb33_mode5_impulse(); return true;
    case 0x2B100: a.camera_task(regs[0]); return true;
    case 0x2B3A0: a.camera_task_start(); return true;
    case 0x2D350: a.camera_globals_reset(); return true;
    case 0x6CA60: a.camera_reset(); return true;
    case 0x6CAB0: a.camera_mode_run(); return true;
