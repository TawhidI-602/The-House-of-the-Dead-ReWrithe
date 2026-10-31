// ---- kind 32, kit 4: the iron-ball swinger (arcade cluster 0x46F60..0x47B00) ----
// Included by enemy_kit_zombie.cpp. Read from the arcade and PC disassembly. PC differences: the view position and
// the mode-3 walk step through the software matrix stack (the file's helper copies), the reach distance by its table
// square root, iron_ball_params packed (+0 / +4 / +8 for the arcade's +0 / +8 / +0xC), sounds 0x9B / 0x9A.

// iron_ball_params word i (0 walk step, 1 hold after a throw, 2 hold after the hit pose)
template <class P> uint32_t Game<P>::iron_ball_param(int i)
{
    return mem.u32(G::iron_ball_params + uint32_t(P::pc ? i * 4 : (i == 0 ? 0 : 4 + i * 4)));
}

// a motion's last-but-one frame reached (motion_frame_counts)
template <class P> bool Game<P>::kind32_motion_near_end(uint32_t obj)
{
    return int32_t(mem.u32(G::motion_frame_counts + uint32_t(int32_t(of<int16_t>(obj, 0xD0))) * 4)) - 2 ==
           int32_t(of<int16_t>(obj, 0xF4));
}

// 0x46F60 (kit 4 start): +0x320 / +0x324 / +0x328 0; its view position (stored matrix 5); the ball's task
// (iron_ball_task); by record +1: 0 by record +3 / +0xC (rising swing, swinging, walking or the stance), 1 the hit
// pose, 2 walking, 3 swinging with +0x320 bit 1
template <class P> void Game<P>::kind32_act4_iron_ball(uint32_t obj)
{
    const uint32_t r = of<uint32_t>(obj, 0x368);
    of<int32_t>(obj, 0x328) = 0;
    of<int32_t>(obj, 0x324) = 0;
    of<int32_t>(obj, 0x320) = 0;
    if (pc_path()) {
        // PC: stored matrix 5 on the software matrix stack, the file's pc_matrix_transform copy
        pc_matrix_push();
        pcm_load_stored5();
        float p[3];
        pc_matrix_transform(of<float>(obj, 0x20), of<float>(obj, 0x24), of<float>(obj, 0x28), p[0], p[1], p[2]);
        of<float>(obj, 0x98) = p[0];
        of<float>(obj, 0x9C) = p[1];
        of<float>(obj, 0xA0) = p[2];
        pcm_pop();
    } else {
        Tgp::Out o, none;
        const uint32_t slot[1] = {5};
        copro(0x74, slot, 1, none);
        const uint32_t v[3] = {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)};
        copro(0x1A, v, 3, o);
        of<uint32_t>(obj, 0x98) = o.w[0];
        of<uint32_t>(obj, 0x9C) = o.w[1];
        of<uint32_t>(obj, 0xA0) = o.w[2];
        copro(0x06, nullptr, 0, none);
    }
    task_create(F::iron_ball_task, uint32_t(sizeof(Obj)));
    const int32_t d = mem.at<int16_t>(r + 6);
    const int32_t k = mem.at<int8_t>(r + 1);
    if (k == 0) {
        if ((mem.at<uint8_t>(r + 3) & 0xE) == 8) {
            iron_ball_swing_start(obj, 1, 0, d);
            return;
        }
        const uint32_t m = mem.at<uint16_t>(r + 0xC) & 7;
        if (m == 2)
            kind32_act4_swing_set(obj, 1, 0, d);
        else if (m == 1 || m == 3)
            kind32_walk_start(obj, 1, 0x206, d);
        else
            kind32_stance_wait(obj, 1, 0x1F1, d);
        return;
    }
    if (k == 2) {
        kind32_walk_start(obj, 1, 0x206, d);
    } else if (k == 1) {
        iron_ball_hit_start(obj);
    } else if (k == 3) {
        of<uint32_t>(obj, 0x320) = of<uint32_t>(obj, 0x320) | 2;
        kind32_act4_swing_set(obj, 1, 0, d);
    }
}

// 0x470C0 (object, how, motion, delay): FixMode, second motion off; how 1 the motion, 2.. blended (second motion 8);
// +0x2EC the delay, then the count
template <class P> void Game<P>::kind32_stance_wait(uint32_t obj, int how, uint32_t motion, int32_t delay)
{
    of<uint8_t>(obj, 0x3C6) = of<uint8_t>(obj, 0x3C6) | 1;
    body_second_motion(obj, 0);
    if (how == 1) {
        set_motion(obj, motion);
    } else if (how != 0) {
        set_motion_blend(obj, motion, 0, 8);
        body_second_motion(obj, 8);
    }
    of<int32_t>(obj, 0x2EC) = delay;
    of<uint32_t>(obj, 0x0) = F::kind32_stance_wait_count;
    of<int16_t>(obj, 0x66) = 0;
    kind32_stance_wait_count(obj);
}

// 0x47130
template <class P> void Game<P>::kind32_stance_wait_count(uint32_t obj)
{
    if (of<int16_t>(obj, 0x66) == 0) {
        const int32_t t = of<int32_t>(obj, 0x2EC) - 1;
        of<int32_t>(obj, 0x2EC) = t;
        if (t != 0) {
            kind32_hits(obj);
            return;
        }
    }
    of<int16_t>(obj, 0x66) = 0;
    of<uint32_t>(obj, 0x0) = F::kind32_stance_wait_swing;
    kind32_stance_wait_swing(obj);
}

// 0x47170: MO_z_tekk_hajime (blended), at its end swinging; sound 0xA01363 at frame 75
template <class P> void Game<P>::kind32_stance_wait_swing(uint32_t obj)
{
    const int16_t step = of<int16_t>(obj, 0x66);
    if (step == 0) {
        of<uint32_t>(obj, 0x328) = of<uint32_t>(obj, 0x328) | 1;
        set_motion_blend(obj, 0x277, 0, 4);
        of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
    }
    if (step == 0 || step == 1) {
        if (kind32_motion_near_end(obj)) {
            kind32_act4_swing_set(obj, 1, 0, 1);
            return;
        }
    }
    if (of<int16_t>(obj, 0xF4) == 0x4B)
        sound_cue(P::pc ? 0x9B : 0xA01363);
    kind32_hits(obj);
}

// 0x47200 (object, how): second motion off; MO_z_tekk_hajime (how 1 at once, 2.. blended), then its frame routine
template <class P> void Game<P>::kind32_act4_swing_again(uint32_t obj, int how)
{
    body_second_motion(obj, 0);
    if (how == 1)
        set_motion(obj, 0x277);
    else if (how != 0)
        set_motion_blend(obj, 0x277, 0, 8);
    of<uint32_t>(obj, 0x0) = F::kind32_act4_swing_start_frame;
    of<int16_t>(obj, 0x66) = 0;
    kind32_act4_swing_start_frame(obj);
}

// 0x47260: at the start motion's end MO_z_tekk_mawasi and the walk (record +6); sound at frame 75; the hits
template <class P> void Game<P>::kind32_act4_swing_start_frame(uint32_t obj)
{
    const uint32_t r = of<uint32_t>(obj, 0x368);
    if (kind32_motion_near_end(obj)) {
        set_motion(obj, 0x27A);
        kind32_walk_start(obj, 2, 0x206, mem.at<int16_t>(r + 6));
    }
    if (of<int16_t>(obj, 0xF4) == 0x4B)
        sound_cue(P::pc ? 0x9B : 0xA01363);
    kind32_hits(obj);
}

// 0x472D0 (object, how, motion, delay): FixMode off, +0x328 bit 0; the walk motion with MO_z_tekk_mawasi as the second
// layer (how 1: the second layer first); +0x2EC the delay; kind32_walk
template <class P> void Game<P>::kind32_walk_start(uint32_t obj, int how, uint32_t motion, int32_t delay)
{
    of<uint8_t>(obj, 0x3C6) = of<uint8_t>(obj, 0x3C6) & 0xFE;
    of<uint32_t>(obj, 0x328) = of<uint32_t>(obj, 0x328) | 1;
    if (how == 1) {
        set_motion2(obj, 0x27A);
        set_motion(obj, motion);
    } else if (how != 0) {
        set_motion(obj, motion);
        set_motion2(obj, 0x27A);
    }
    of<int32_t>(obj, 0x2EC) = delay;
    of<uint32_t>(obj, 0x0) = F::kind32_walk;
    of<int16_t>(obj, 0x66) = 0;
    kind32_walk(obj);
}

// 0x47350: record +0xC bit 8: closer than record +0x14 to the camera (its view position's floor distance) -> turns;
// record +0xC mode 3 walks forward (iron_ball_params +0, coprocessor 0x21 / 0x15 / 0x1A); sound at the second
// motion's frame 20; the hits
template <class P> void Game<P>::kind32_walk(uint32_t obj)
{
    const uint32_t r = of<uint32_t>(obj, 0x368);
    if ((mem.at<uint16_t>(r + 0xC) >> 8) & 1) {
        float d;
        if (pc_path()) {
            const float dx = 0.0f - of<float>(obj, 0x98), dz = 0.0f - of<float>(obj, 0xA0);
            d = pc_fast_sqrt_table(dx * dx + dz * dz);   // the file's copy 0x48AE6C
        } else {
            const uint32_t in[6] = {of<uint32_t>(obj, 0x98), 0, of<uint32_t>(obj, 0xA0), 0, 0, 0};
            Tgp::Out o;
            copro(0x1F, in, 6, o);
            d = ffrom(o.w[0]);
        }
        if (!(d >= mem.f32(r + 0x14))) {
            of<int16_t>(obj, 0x66) = 0;
            of<uint32_t>(obj, 0x0) = F::kind32_walk_turn;
            kind32_walk_turn(obj);
            return;
        }
    }
    if ((mem.at<uint16_t>(r + 0xC) & 7) != 3) {
    } else if (pc_path()) {
        // PC: the identity on the software matrix stack, turned by its heading, the step through it
        pc_matrix_push();
        pc_matrix_identity3();
        pc_matrix_rotate_y8(of<uint32_t>(obj, 0x30));
        float p[3];
        pc_matrix_transform(0.0f, 0.0f, ffrom(iron_ball_param(0)), p[0], p[1], p[2]);
        of<float>(obj, 0x20) = of<float>(obj, 0x20) + p[0];
        pcm_pop();
        of<float>(obj, 0x24) = of<float>(obj, 0x24) + p[1];
        of<float>(obj, 0x28) = of<float>(obj, 0x28) + p[2];
    } else {
        Tgp::Out o, none;
        copro(0x21, nullptr, 0, none);
        const uint32_t ry[1] = {of<uint32_t>(obj, 0x30)};
        copro(0x15, ry, 1, none);
        const uint32_t v[3] = {0, 0, iron_ball_param(0)};
        copro(0x1A, v, 3, o);
        of<float>(obj, 0x28) = of<float>(obj, 0x28) + ffrom(o.w[2]);
        of<float>(obj, 0x20) = of<float>(obj, 0x20) + ffrom(o.w[0]);
        of<float>(obj, 0x24) = of<float>(obj, 0x24) + ffrom(o.w[1]);
        copro(0x06, nullptr, 0, none);
    }
    if (of<int16_t>(obj, 0x108) == 20)
        sound_cue(P::pc ? 0x9B : 0xA01363);
    kind32_hits(obj);
}

// 0x474B0: the second layer blended out over 8 frames (FixMode); at the motion's end swinging; sound at frame 20
template <class P> void Game<P>::kind32_walk_turn(uint32_t obj)
{
    if (of<int16_t>(obj, 0x66) == 0) {
        of<uint8_t>(obj, 0x3C6) = of<uint8_t>(obj, 0x3C6) | 1;
        set_motion2_blend(obj, 8);
        of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
    }
    if ((of<uint8_t>(obj, 0x3C5) >> 4) & 1) {
        kind32_act4_swing_set(obj, 0, 0, 2);
        return;
    }
    const int16_t f = ((of<uint8_t>(obj, 0x3C6) >> 5) & 1) ? of<int16_t>(obj, 0x108) : of<int16_t>(obj, 0xF4);
    if (f == 20)
        sound_cue(P::pc ? 0x9B : 0xA01363);
    kind32_hits(obj);
}

// 0x47530 (object, how, start, delay): MO_z_tekk_mawasi (how 1: second motion off, at once, anim_time = start; 2..
// blended from `start` over 16); +0x2EC the delay, +0x328 bit 0; the swing wait
template <class P> void Game<P>::kind32_act4_swing_set(uint32_t obj, int how, int start, int32_t delay)
{
    if (how == 1) {
        body_second_motion(obj, 0);
        set_motion(obj, 0x27A);
        of<int32_t>(obj, 0xF8) = start;
    } else if (how != 0) {
        set_motion_blend(obj, 0x27A, start, 0x10);
    }
    of<int32_t>(obj, 0x2EC) = delay;
    of<uint32_t>(obj, 0x0) = F::iron_ball_swing_wait;
    of<int16_t>(obj, 0x66) = 0;
    of<uint32_t>(obj, 0x328) = of<uint32_t>(obj, 0x328) | 1;
    iron_ball_swing_wait(obj);
}

// 0x475A0: counts down (step 0), then at a motion end attacks when in combat, in reach and the front positions free
template <class P> void Game<P>::iron_ball_swing_wait(uint32_t obj)
{
    bool check = true;
    if (of<int16_t>(obj, 0x66) == 0) {
        const int32_t t = of<int32_t>(obj, 0x2EC) - 1;
        of<int32_t>(obj, 0x2EC) = t;
        if (t != 0)
            check = false;
        else
            of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
    }
    if (check && ((of<uint8_t>(obj, 0x3C5) >> 4) & 1) && combat_active() != 0 && iron_ball_in_reach(obj) != 0 &&
        attack_front_busy(obj) == 0) {
        of<uint32_t>(obj, 0x0) = F::iron_ball_attack;
        of<int16_t>(obj, 0x66) = 0;
        iron_ball_attack(obj);
        return;
    }
    if (of<int16_t>(obj, 0xF4) == 20)
        sound_cue(P::pc ? 0x9B : 0xA01363);
    kind32_hits(obj);
}

// 0x47630 (object, how, start, delay): as kind32_act4_swing_set, then rising (kind32_rise_steps by record +2), not
// shootable meanwhile
template <class P> void Game<P>::iron_ball_swing_start(uint32_t obj, int how, int start, int32_t delay)
{
    const int32_t row = mem.at<int8_t>(of<uint32_t>(obj, 0x368) + 2);
    if (how == 1) {
        body_second_motion(obj, 0);
        set_motion(obj, 0x27A);
        of<int32_t>(obj, 0xF8) = start;
    } else if (how != 0) {
        set_motion_blend(obj, 0x27A, start, 0x10);
    }
    of<int32_t>(obj, 0x2EC) = delay;
    of<int16_t>(obj, 0x66) = 0;
    of<uint32_t>(obj, 0x328) = of<uint32_t>(obj, 0x328) | 1;
    const float frames = mem.f32(G::kind32_rise_steps + 4 + uint32_t(row) * 0x18);
    of<uint32_t>(obj, 0x0) = F::iron_ball_swing;
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xFFFFFEF8u;
    of<int32_t>(obj, 0x50) = int32_t(frames);
    iron_ball_swing(obj);
}

// 0x476D0: rises by the step; at 0 shootable and the swing wait; sound at frame 20
template <class P> void Game<P>::iron_ball_swing(uint32_t obj)
{
    const float y = of<float>(obj, 0x24);
    const float step = mem.f32(G::kind32_rise_steps + uint32_t(int32_t(mem.at<int8_t>(of<uint32_t>(obj, 0x368) + 2))) * 0x18);
    const int32_t t = of<int32_t>(obj, 0x50) - 1;
    of<int32_t>(obj, 0x50) = t;
    of<float>(obj, 0x24) = step + y;
    if (t == 0) {
        of<uint32_t>(obj, 0x0) = F::iron_ball_swing_wait;
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | 1;
        iron_ball_swing_wait(obj);
        return;
    }
    if (of<int16_t>(obj, 0xF4) == 20)
        sound_cue(P::pc ? 0x9B : 0xA01363);
    kind32_hits(obj);
}

// 0x47750: MO_z_tekk_hiki (pulling the ball back); at its end swinging again (record +8)
template <class P> void Game<P>::iron_ball_pull_back(uint32_t obj)
{
    const uint32_t r = of<uint32_t>(obj, 0x368);
    const int16_t step = of<int16_t>(obj, 0x66);
    if (step == 0) {
        set_motion(obj, 0x278);
        of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
    }
    if (step == 0 || step == 1) {
        if (!kind32_motion_near_end(obj)) {
            kind32_hits(obj);
            return;
        }
        of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
    }
    kind32_act4_swing_set(obj, 1, 0, mem.at<int16_t>(r + 8));
}

// 0x477D0: the throw: MO_z_tekk_atk (blended 4 / 3, FixMode off); near its end MO_z_tekk_hit and sound 0xA01362; once
// the ball reports back (+0x324 bit 0) iron_ball_params +8 frames, then pulls back (+0x320 bit 1 off)
template <class P> void Game<P>::iron_ball_attack(uint32_t obj)
{
    int16_t step = of<int16_t>(obj, 0x66);
    if (step == 0) {
        of<uint32_t>(obj, 0x328) = of<uint32_t>(obj, 0x328) & ~1u;
        of<uint8_t>(obj, 0x3C6) = of<uint8_t>(obj, 0x3C6) & 0xFE;
        set_motion_blend(obj, 0x276, 4, 3);
        of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
        step = 1;
    }
    if (step == 1) {
        if (!kind32_motion_near_end(obj)) {
            kind32_hits(obj);
            return;
        }
        set_motion(obj, 0x279);
        sound_cue(P::pc ? 0x9A : 0xA01362);
        of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
        step = 2;
    }
    if (step == 2) {
        if ((of<uint8_t>(obj, 0x324) & 1) == 0) {
            kind32_hits(obj);
            return;
        }
        of<uint32_t>(obj, 0x324) = of<uint32_t>(obj, 0x324) & ~1u;
        of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
        of<uint32_t>(obj, 0x2EC) = iron_ball_param(1);
    }
    const int32_t t = of<int32_t>(obj, 0x2EC) - 1;
    of<int32_t>(obj, 0x2EC) = t;
    if (t != 0) {
        kind32_hits(obj);
        return;
    }
    of<uint32_t>(obj, 0x0) = F::iron_ball_pull_back;
    of<int16_t>(obj, 0x66) = 0;
    of<uint32_t>(obj, 0x320) = of<uint32_t>(obj, 0x320) & ~2u;
    iron_ball_pull_back(obj);
}

// 0x478E0 (kit 4 after a hit): FixMode, +0x320 bit 1 off; the right hand not as the model set has it: bit 0 (the ball
// lost); the after-hit frame
template <class P> void Game<P>::kind32_act4_after_hit(uint32_t obj)
{
    of<uint8_t>(obj, 0x3C6) = of<uint8_t>(obj, 0x3C6) | 1;
    of<uint32_t>(obj, 0x320) = of<uint32_t>(obj, 0x320) & ~2u;
    const uint32_t hdr = mem.u32(mem.u32(G::creature_type_headers + uint32_t(int32_t(of<int16_t>(obj, 0xCC))) * 4) + 0x1C);
    const uint32_t p = mem.u32(mem.u32(mem.u32(hdr + 0x20) + 0x1C) + 0x1C);
    if (mem.u32(p) != body_node_model(obj, 5))
        of<uint32_t>(obj, 0x320) = of<uint32_t>(obj, 0x320) | 1;
    of<uint32_t>(obj, 0x0) = F::kind32_act4_after_hit_frame;
    kind32_act4_after_hit_frame(obj);
}

// 0x47950: the ball lost: deactivate (record +0xC bits 6-7 0) or stand down; else back to the second motion's
// motion (when one runs) and by record mode: 1 / 3 swinging again, else the stance (32 frames)
template <class P> void Game<P>::kind32_act4_recover(uint32_t obj)
{
    const uint32_t r = of<uint32_t>(obj, 0x368);
    if (of<uint8_t>(obj, 0x320) & 1) {
        of<uint32_t>(obj, 0x320) = of<uint32_t>(obj, 0x320) | 1;
        if (((uint32_t(int32_t(mem.at<int16_t>(r + 0xC))) >> 6) & 3) == 0)
            kind32_deactivate(obj);
        else
            kind32_after_throw_stance(obj);
        return;
    }
    if ((of<uint8_t>(obj, 0x3C6) >> 5) & 1)
        of<int16_t>(obj, 0xD0) = of<int16_t>(obj, 0xFE);
    const uint32_t m = mem.at<uint16_t>(r + 0xC) & 7;
    if (m == 1 || m == 3)
        kind32_act4_swing_again(obj, 2);
    else
        kind32_stance_wait(obj, 2, 0x1F1, 32);
}

// 0x479D0
template <class P> void Game<P>::kind32_act4_after_hit_frame(uint32_t obj)
{
    if (kind32_hit_reaction_done(obj, F::kind32_act4_recover) != 0)
        return;
    kind32_hits(obj);
}

// 0x47A00 (record +1 = 1): +0x324 0, +0x320 bit 1; MO_z_tekk_hit; the hit wait
template <class P> void Game<P>::iron_ball_hit_start(uint32_t obj)
{
    of<int32_t>(obj, 0x324) = 0;
    of<uint32_t>(obj, 0x320) = of<uint32_t>(obj, 0x320) | 2;
    set_motion(obj, 0x279);
    of<uint32_t>(obj, 0x0) = F::iron_ball_hit_wait;
    of<int16_t>(obj, 0x66) = 0;
    iron_ball_hit_wait(obj);
}

// 0x47A40: waits for the ball (+0x324 bit 0; until then event flag 0xE once the count runs out), then
// iron_ball_params +0xC frames and iron_ball_hit_wait_end
template <class P> void Game<P>::iron_ball_hit_wait(uint32_t obj)
{
    int16_t step = of<int16_t>(obj, 0x66);
    if (step == 0) {
        of<int32_t>(obj, 0x2EC) = 1;
        of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
        step = 1;
    }
    if (step == 1) {
        if (of<int32_t>(obj, 0x324) == 0) {
            const int32_t t = of<int32_t>(obj, 0x2EC) - 1;
            of<int32_t>(obj, 0x2EC) = t;
            if (t == 0)
                mem.at<uint8_t>(G::event_flags + 0xE) = 1;
            kind32_hits(obj);
            return;
        }
        of<uint32_t>(obj, 0x324) = of<uint32_t>(obj, 0x324) & ~1u;
        of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
        of<uint32_t>(obj, 0x2EC) = iron_ball_param(2);
        step = 2;
    }
    if (step == 2) {
        const int32_t t = of<int32_t>(obj, 0x2EC) - 1;
        of<int32_t>(obj, 0x2EC) = t;
        if (t == 0) {
            of<int16_t>(obj, 0x66) = 0;
            of<uint32_t>(obj, 0x0) = F::iron_ball_hit_wait_end;
            call_routine(F::iron_ball_hit_wait_end, obj);
            return;
        }
    }
    kind32_hits(obj);
}

// 0x47B00: +0x320 bit 1 off; record +0xC & 7: 1 / 3 walking, else swinging (the record's +6 delay)
template <class P> void Game<P>::iron_ball_hit_wait_end(uint32_t obj)
{
    const uint32_t r = of<uint32_t>(obj, 0x368);
    of<uint32_t>(obj, 0x320) = of<uint32_t>(obj, 0x320) & ~2u;
    const int32_t d = mem.at<int16_t>(r + 6);
    const uint32_t m = mem.at<uint16_t>(r + 0xC) & 7;
    if (m == 1 || m == 3) {
        kind32_walk_start(obj, 1, 0x206, d);
        return;
    }
    kind32_act4_swing_set(obj, 1, 0, d);
}

// 0x47B60 (the ball's task, made by kind32_act4_iron_ball; +0x18 its owner): +0x37C.. cleared; +0x35C from
// iron_ball_ptr_tree; the owner's record; a 0x14-byte block (+0x37C: the owner's type, +8 0, +0xA -1, +0xC 0); at the
// owner's node 0x1F in motion 0x279 (iron_ball_hand_pos); shootable, radius 4; the chain (iron_chain_start); then
// iron_ball_follow. PC: pc_wide_slot_kind 3 first.
template <class P> void Game<P>::iron_ball_task(uint32_t obj)
{
    LocalScope ls(*this);
    const uint32_t owner = of<uint32_t>(obj, 0x18);
    obj_clear_37c(obj);
    uint32_t t = mem.u32(G::iron_ball_ptr_tree);
    t = mem.u32(t + 0x1C);
    t = mem.u32(t + 0x20);
    t = mem.u32(t + 0x1C);
    t = mem.u32(t + 0x1C);
    t = mem.u32(t + 0x1C);
    of<uint32_t>(obj, 0x35C) = t;
    of<uint32_t>(obj, 0x368) = of<uint32_t>(owner, 0x368);
    if constexpr (P::pc)
        mem.set32(G::pc_wide_slot_kind, 3);   // PC only
    const uint32_t blk = mem_alloc(0x14);
    of<uint32_t>(obj, 0x37C) = blk;
    mem.at<int16_t>(blk) = of<int16_t>(owner, 0xCC);
    const uint32_t pos = local_alloc(16);   // fp + 0x40
    iron_ball_hand_pos(owner, 0x1F, 0x279, 0, pos);
    relocate(P::pc ? 5 : 4, pos, 12);
    of<uint32_t>(obj, 0x20) = mem.u32(pos);
    of<uint32_t>(obj, 0x24) = mem.u32(pos + 4);
    of<uint32_t>(obj, 0x28) = mem.u32(pos + 8);
    of<int32_t>(obj, 0x58) = 1;
    of<float>(obj, 0x90) = 4.0f;
    of<int32_t>(obj, 0xC8) = 0;
    of<int32_t>(obj, 0x31C) = 0;
    iron_chain_start(owner, obj);
    mem.at<int16_t>(blk + 8) = 0;
    mem.at<int32_t>(blk + 0xC) = 0;
    mem.at<int16_t>(blk + 0xA) = -1;
    of<uint32_t>(obj, 0x0) = F::iron_ball_follow;
    of<int16_t>(obj, 0x66) = 0;
    call_routine(F::iron_ball_follow, obj);
}
