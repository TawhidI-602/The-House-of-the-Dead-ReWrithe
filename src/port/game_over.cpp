// The game over (proofs-5): the players' game-over state, the game-over scene's camera, the route map's walker and
// route tasks. First written in the runtime (src/game/flow.cpp: gameover, gameover-2, fix-world-2), moved here for
// their capture proofs (docs/proof.md "Game over (proofs-5)"). Arcade forms only (the PC forms are replayed).
// Declarations: decl/support_b.h.
#include "port/game.h"

namespace hotd::port {

// 0x5470 player_st6_game_over (task): state 6 (the countdown ran out): the body routine player_body_camdr (0x62C0),
// the character's collapse motion (table 0xA86E8; player 2 turned 0x2000), the body placed at the camera, 120 frames,
// the start prompt off, then player_game_over_wait
template <class P> void Game<P>::player_st6_game_over(uint32_t task)
{
    if constexpr (P::pc) {
        (void)task;   // the PC form: not ported (vtable only; ProofPc replays it)
    } else {
        const int32_t p = mem.template at<int8_t>(task + 0x20);
        const uint32_t rec = G::players + uint32_t(p) * 0xCC;
        const uint32_t body = mem.u32(rec + 0xBC);
        player_nop();
        if (mem.template at<int16_t>(G::game_mode) != 5)
            hud_game_over_sprite(p);
        mem.set32(rec + 0xAC, 0x62C0);
        set_motion(body, uint32_t(int32_t(int16_t(mem.u32(0xA86E8 + uint32_t(p) * 4)))));
        if (p != 0)
            mem.set32(body + 0x30, mem.u32(body + 0x30) + 0x2000);
        mem.set32(rec + 0xC4, 0);
        player_body_place(task);
        mem.set32(rec + 0x60, 0x78);
        hud_start_prompt(int32_t(mem.template at<int8_t>(rec)), 1, 0);
        mem.template at<uint16_t>(0x51F4E0 + uint32_t(p) * 0x1C) &= 0xFFFE;
        mem.set32(task, 0x5540);
        player_game_over_wait(task);
    }
}

// 0x5540 player_game_over_wait (task): in mode 5 only the camera drive (the body routine); else 120 frames, then the
// GAME OVER sprite at the corner, the play time booked, state 9
template <class P> void Game<P>::player_game_over_wait(uint32_t task)
{
    if constexpr (P::pc) {
        (void)task;
    } else {
        if (mem.template at<int16_t>(G::game_mode) == 5) {
            player_camdr_run(task);
            return;
        }
        const int32_t p = mem.template at<int8_t>(task + 0x20);
        const uint32_t rec = G::players + uint32_t(p) * 0xCC;
        const int32_t t = int32_t(mem.u32(rec + 0x60)) - 1;
        mem.set32(rec + 0x60, uint32_t(t));
        if (t == 0) {
            mem.set32(0x51F560, 0);
            hud_game_over_sprite_final(p);
            bookkeeping_play_end(p);
            player_state_set(9, 1, task);
        }
    }
}

// game_over_camera's coprocessor sequence (0x880210 push / identity, 0x88 camera_pos, 0x15 camera_heading, 0x1A the
// point, 0x880060 pop): the point (x, y, z) in the camera's frame -> world
template <class P> Tgp::Out Game<P>::game_over_cam_point(float x, float y, float z)
{
    cam_copro(0x21, {});
    cam_copro(0x88, {mem.u32(G::camera_pos), mem.u32(G::camera_pos + 4), mem.u32(G::camera_pos + 8)});
    cam_copro(0x15, {mem.u32(G::camera_heading)});
    const Tgp::Out o = cam_copro(0x1A, {fbits(x), fbits(y), fbits(z)});
    cam_copro(0x06, {});
    return o;
}

// 0x97490 game_over_camera (task; disassembly: the decompiler lost the float constants): on its first frame the eye
// at (10, 5, 20) and the look-at point at (0, 14, 6) from the camera; from timer 0x520700 < 276 the look-at height
// eases to (0, 0, 6)'s (x 0.9756098 + 0.024390243 a frame); camera_look_at(record 0, its look-at, roll 0),
// camera_matrices_update every frame
template <class P> void Game<P>::game_over_camera(uint32_t task)
{
    if constexpr (P::pc) {
        (void)task;
    } else {
        const uint32_t eye = G::camera_records, at = G::camera_records + 0x18;
        uint32_t st = mem.u32(task + 0x50);
        if (st == 0) {
            const Tgp::Out e = game_over_cam_point(10.0f, 5.0f, 20.0f);
            const Tgp::Out a = game_over_cam_point(0.0f, 14.0f, 6.0f);
            for (uint32_t k = 0; k < 3; ++k) {
                mem.set32(eye + 4 * k, e.w[k]);
                mem.set32(at + 4 * k, a.w[k]);
            }
            st = 1;
            mem.set32(task + 0x50, st);
        }
        if (st == 1 && int32_t(mem.u32(0x520700)) < 0x114) {
            const Tgp::Out o = game_over_cam_point(0.0f, 0.0f, 6.0f);
            mem.set32(at + 4, fbits(ffrom(o.w[1]) * 0.024390243f + mem.f32(at + 4) * 0.9756098f));
        }
        camera_look_at(eye, at, 0);
        camera_matrices_update();
    }
}

// 0x5CCD0 game_over_walker_task (obj; disassembly): the body at the map's scroll point (0x543400 / 0x543402) through
// stored matrix 1 (coprocessor 0x74: ((x - 0xF8) x 150 / 496, (0xC0 - y) x 150 / 496, 150)), x moved by table 0xC5460,
// z 150; step 0: body_init (the player's type 0x51F438, motion 0x1CB); step 1: turned by 0x543404 (x 0x4000), drawn,
// its clock on; when the route ends (0x543405 = 1) the collapse motion (0xA86E8); step 2: drawn until the motion ends
template <class P> void Game<P>::game_over_walker_task(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;
    } else {
        const float sx = float(int32_t(mem.template at<int16_t>(0x543400)) - 0xF8) * 150.0f / 496.0f;
        const float sy = float(0xC0 - int32_t(mem.template at<int16_t>(0x543402))) * 150.0f / 496.0f;
        cam_copro(0x74, {1});
        const Tgp::Out o = cam_copro(0x1A, {fbits(sx), fbits(sy), fbits(150.0f)});
        cam_copro(0x06, {});
        mem.set32(0x5433F0, o.w[0]);
        mem.set32(0x5433F4, o.w[1]);
        mem.set32(0x5433F8, o.w[2]);
        mem.set32(obj + 0x28, fbits(150.0f));
        mem.set32(obj + 0x24, o.w[1]);
        mem.set32(obj + 0x20, fbits(ffrom(o.w[0]) + mem.f32(0xC5460 + uint32_t(int32_t(mem.template at<int16_t>(obj + 0xCE))) * 4)));
        int16_t step = mem.template at<int16_t>(obj + 0x66);
        if (step == 2) {
            body_draw(obj);
            if (mem.template at<uint8_t>(obj + 0x3C5) & 0x10)
                mem.template at<int16_t>(obj + 0x66) = int16_t(step + 1);
            else
                mem.set32(obj + 0xF8, mem.u32(obj + 0xF8) + 1);
            return;
        }
        if (step != 0 && step != 1) {
            body_draw(obj);
            return;
        }
        const int32_t dir = int32_t(mem.template at<int8_t>(0x543404));
        if (step == 0) {
            body_init(obj, int32_t(mem.u32(0x51F438 + uint32_t(int32_t(mem.template at<int16_t>(obj + 0x70))) * 4)), 0x1CB);
            mem.set32(obj + 0x2C, 0);
            mem.set32(obj + 0x34, 0);
            mem.template at<int16_t>(obj + 0x66) = int16_t(step + 1);
            mem.set32(obj + 0x30, uint32_t(dir << 14));
        }
        mem.set32(obj + 0x30, uint32_t(dir << 14));
        body_draw(obj);
        mem.set32(obj + 0xF8, mem.u32(obj + 0xF8) + 1);
        if (mem.template at<uint8_t>(0x543405) == 1) {
            set_motion(obj, mem.u32(0xA86E8 + uint32_t(int32_t(mem.template at<int16_t>(obj + 0x70))) * 4));
            mem.template at<int16_t>(obj + 0x66) = int16_t(mem.template at<int16_t>(obj + 0x66) + 1);
        }
    }
}

// 0x5CF50 game_over_route_task (obj): loads the map sprites, then follows the route the players took segment by
// segment (records of 6 shorts at 0xC1810 + chapter * 0xF00 + scene * 0xC0: x, y, steps, dx, dy, sprite; x = -1 ends
// a scene), drawing the trail on layer 1 (0x1002000, window mask 0x100C000 all set), scrolling layers 1 / 2
// (0x51E8FE / 0x51E900) and revealing the map's lower parts (records of 12 bytes from 0xC1790, pointer 0x5433E8); at
// the end of the chapter reached the walkers fall (0x543405) and the 180-frame timer runs. The task: +0x58 step,
// +0x5A the scroll position, +0x5C chapter, +0x5E scene index, +0x60 scene, +0x64 steps left, +0x68 the record,
// +0x50 the frame counter
template <class P> void Game<P>::game_over_route_task(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;
    } else {
        auto s16 = [&](uint32_t a) { return mem.template at<int16_t>(a); };
        const int16_t step = s16(obj + 0x58);
        if (step == 0) {
            sprite_bank_load(0x4A, 0x243);
            sprite_bank_load(0x4B, 0x247);
            sprite_draw_anim(0x1004000, 0, 0, 0, 0x10, 0x243);
            mem.set32(0x5433E8, 0xC1790);
            sprite_draw_clipped(0x1004000, 0, 0, 0, -0x20, 0, 0x3D, 0, 0xF, 0x244);
            mem.set32(0x5433E8, mem.u32(0x5433E8) + 0xC);
            tilemap_clear_rect(0x1002000, 0, 0, 0x3E, 0x40);
            if (mem.mapped(0x100C000))   // (the window layer: not in a capture's memory)
                for (uint32_t i = 0; i < 0xC00; i += 4)
                    mem.set32(0x100C000 + i, 0xFFFFFFFFu);
            mem.template at<int16_t>(obj + 0x5C) = 0;
            mem.template at<int16_t>(obj + 0x5E) = 0;
            const int32_t sc = mem.template at<int8_t>(G::scene_history);
            mem.template at<int16_t>(obj + 0x60) = int16_t(sc);
            const int32_t off = sc * 0xC0 + int32_t(s16(obj + 0x5C)) * 0xF00;
            mem.set32(obj + 0x68, uint32_t(0xC1810 + off));
            const int16_t steps = s16(uint32_t(0xC1814 + off));
            mem.template at<int16_t>(obj + 0x5A) = 0x480;
            mem.template at<int16_t>(obj + 0x64) = steps;
            mem.template at<uint16_t>(0x51E8FE) = 0x80;
            mem.template at<uint16_t>(0x51E900) = 0x80;
            mem.set32(obj + 0x50, 0);
            mem.template at<int16_t>(obj + 0x58) = int16_t(step + 1);
        } else if (step != 1) {
            game_over_timer_tick();
            return;
        }
        const int32_t timer = int32_t(mem.u32(obj + 0x50)) + 1;
        mem.set32(obj + 0x50, uint32_t(timer));
        uint32_t r = mem.u32(obj + 0x68);
        if ((timer & 1) == 0) {
            if (s16(r + 10) != 0) {
                const int32_t k = int32_t(uint32_t(uint16_t(s16(r + 4)))) - int32_t(s16(obj + 0x64));
                sprite_draw_anim(0x1002000, 0, 0, s16(r + 6) * k + s16(r), (s16(r + 8) * k + s16(r + 2)) % 0x40,
                                 uint32_t(int32_t(s16(r + 10))));
            }
        } else {
            if (s16(r + 10) != 0) {
                const int32_t k = int32_t(uint32_t(uint16_t(s16(r + 4)))) - int32_t(s16(obj + 0x64));
                sprite_draw_anim(0x1002000, 0, 0, s16(r + 6) * k + s16(r), (s16(r + 8) * k + s16(r + 2)) % 0x40,
                                 mem.u32(0xC4AA0 + uint32_t(uint16_t(s16(r + 10))) * 4));
            }
            r = mem.u32(obj + 0x68);
            if (s16(r + 8) == -1)
                mem.template at<uint8_t>(0x543404) = 0;
            else if (s16(r + 6) == -1)
                mem.template at<uint8_t>(0x543404) = 1;
            else if (s16(r + 6) == 1)
                mem.template at<uint8_t>(0x543404) = 3;
        }
        r = mem.u32(obj + 0x68);
        const int16_t pos = s16(obj + 0x5A);
        if (s16(r + 2) < 0xA9 && pos > 0) {
            mem.template at<int16_t>(obj + 0x5A) = int16_t(s16(r + 8) * 4 + pos);
        } else {
            if (pos < 1)
                mem.template at<int16_t>(obj + 0x5A) = 0;
            mem.template at<int16_t>(0x543402) = int16_t(s16(r + 8) * 4 + s16(0x543402));
        }
        mem.template at<int16_t>(0x543400) = int16_t(s16(r + 6) * 4 + s16(0x543400));
        const uint16_t scroll = uint16_t(mem.template at<uint16_t>(obj + 0x5A) & 0x1FF);
        mem.template at<uint16_t>(0x51E8FE) = scroll;
        mem.template at<uint16_t>(0x51E900) = scroll;
        if (timer & 1)
            return;
        const int16_t left = s16(obj + 0x64);
        mem.template at<int16_t>(obj + 0x64) = int16_t(left - 1);
        if (left == 1) {
            r = mem.u32(obj + 0x68) + 0xC;
            mem.set32(obj + 0x68, r);
            if (s16(r) == -1) {
                const int16_t i2 = int16_t(s16(obj + 0x5E) + 1);
                mem.template at<int16_t>(obj + 0x5E) = i2;
                const int8_t sc = mem.template at<int8_t>(G::scene_history + uint32_t(int32_t(i2) + int32_t(s16(obj + 0x5C)) * 0x10));
                mem.template at<int16_t>(obj + 0x60) = sc;
                if (sc == -1) {
                    if (int16_t(mem.template at<int8_t>(G::chapter)) == s16(obj + 0x5C)) {
                        mem.template at<uint8_t>(0x543405) = uint8_t(mem.template at<uint8_t>(0x543405) + 1);
                        mem.template at<int16_t>(obj + 0x58) = int16_t(s16(obj + 0x58) + 1);
                    } else {
                        const uint16_t ch = uint16_t(s16(obj + 0x5C) + 1);
                        mem.template at<int16_t>(obj + 0x5E) = 0;
                        mem.template at<int16_t>(obj + 0x5C) = int16_t(ch);
                        mem.template at<int16_t>(obj + 0x60) =
                            mem.template at<int8_t>(G::scene_history + uint32_t(int32_t(uint32_t(ch) << 16) >> 12));
                        mem.set32(obj + 0x68, uint32_t(0xC1810 + int32_t(s16(obj + 0x5E)) * 0xC0 + int32_t(s16(obj + 0x5C)) * 0xF00));
                    }
                } else {
                    mem.set32(obj + 0x68, uint32_t(0xC1810 + int32_t(s16(obj + 0x60)) * 0xC0 + int32_t(s16(obj + 0x5C)) * 0xF00));
                }
            }
            mem.template at<int16_t>(obj + 0x64) = s16(mem.u32(obj + 0x68) + 4);
        }
        const int32_t v = s16(obj + 0x5A);
        const uint32_t rv = mem.u32(0x5433E8);
        if (int32_t(mem.u32(rv)) == v) {
            sprite_draw_clipped(0x1004000, 0, 0, 0, int32_t(int16_t(mem.u32(rv + 4))), 0, 0x3D, s16(rv + 6),
                                int32_t(int16_t(mem.u32(rv + 8))), mem.template at<uint16_t>(rv + 10));
            tilemap_clear_rect(0x1002000, 0, s16(rv + 6), 0x3E, 0x10);
            mem.set32(0x5433E8, rv + 0xC);
        }
    }
}

// --- the task groups' start entries (task_group_create runs each once) ---

// 0x97740 game_over_state0_camera (group 0xCA790 entry 0): the scene's camera task (0x97490), the camera globals
// reset, its state 0 and its first frame
template <class P> void Game<P>::game_over_state0_camera()
{
    if constexpr (!P::pc) {
        const uint32_t t = task_create(0x97490, 0x58);
        camera_globals_reset();
        mem.set32(t + 0x50, 0);
        game_over_camera(t);
    }
}

// 0x5CCA0 game_over_camera_start (group 0xC5440 entry 0): the camera task 0x5D4F0, the camera globals reset, camera
// record 0's eye and first angle at 0
template <class P> void Game<P>::game_over_camera_start()
{
    if constexpr (!P::pc) {
        task_create(0x5D4F0, 0x20);
        camera_globals_reset();
        for (uint32_t i = 0; i < 0x10; i += 4)
            mem.set32(G::camera_records + i, 0);
    }
}

// 0x5D4F0 game_over_camera_task: camera_matrices_update every frame
template <class P> void Game<P>::game_over_camera_task()
{
    if constexpr (!P::pc)
        camera_matrices_update();
}

// 0x5D4D0 game_over_route_start (entry 1): the route task 0x5CF50, its step 0
template <class P> void Game<P>::game_over_route_start()
{
    if constexpr (!P::pc)
        mem.template at<int16_t>(task_create(0x5CF50, 0x6C) + 0x58) = 0;
}

// 0x5CE70 game_over_walkers_start (entry 2): a walker (0x5CCD0, a 0x3C8-byte body object) per player still in
// state 6: both (player 0 at table 0xC5460 entry 1 with +0xCC 2, player 1 entry 2 with 0xD), or the one (+0xCC from
// table 0xC5450; the index -1 + 2 x player 2 = 1 for player 2 alone, -1 with nobody)
template <class P> void Game<P>::game_over_walkers_start()
{
    if constexpr (!P::pc) {
        int32_t n = mem.u32(G::players) != 6 ? -1 : 0;
        if (mem.u32(G::players + 0xCC) == 6)
            n += 2;
        if (n == 2) {
            uint32_t t = task_create(0x5CCD0, 0x3C8);
            mem.template at<int16_t>(t + 0x70) = 0;
            mem.template at<int16_t>(t + 0x66) = 0;
            mem.template at<int16_t>(t + 0xCE) = 1;
            mem.template at<int16_t>(t + 0xCC) = 2;
            t = task_create(0x5CCD0, 0x3C8);
            mem.template at<int16_t>(t + 0x70) = 1;
            mem.template at<int16_t>(t + 0x66) = 0;
            mem.template at<int16_t>(t + 0xCE) = 2;
            mem.template at<int16_t>(t + 0xCC) = 0xD;
            return;
        }
        // 4P / PC Mode (hunt2, 2026-10-20): a game over with neither player 1 nor 2 in state 6 (seats 3 / 4 ended it,
        // players 1 / 2 never joined: state 9). The arcade never gets here with n = -1 (its walker then reads the
        // tables at index -1: a NaN position, nothing drawn); no walker instead
        if (n < 0 && (coop4_on() || pm_on()))
            return;
        const uint32_t t = task_create(0x5CCD0, 0x3C8);
        mem.template at<int16_t>(t + 0x70) = int16_t(n);
        const int16_t cc = mem.template at<int16_t>(uint32_t(int32_t(0xC5450) + n * 4));
        mem.template at<int16_t>(t + 0xCE) = 0;
        mem.template at<int16_t>(t + 0x66) = 0;
        mem.template at<int16_t>(t + 0xCC) = cc;
    }
}

// --- mode 5's sub-modes (table 0xCA770) ---
// The original's busy wait after palette_fade_finish (until the vblank clears 0x51F706) is not here (the
// runtime has no vblank; 0x51F706 is 0 without a fade).

// 0x97200 game_over_start (sub-mode 0): layers 0 and 2 cleared, a running fade finished, the slots marked, the heap
// reset, the characters' bodies, the camera at the origin, players out of lives (state 4) to state 6, the task group
// 0xCA790 (the scene's camera, input, the players' tasks, shake) in 0x543640, the 360-frame timer 0x520700, camera
// status bit 1 off, the music off and the game over cue
template <class P> void Game<P>::game_over_start()
{
    if constexpr (!P::pc) {
        tilemap_clear_rect(0x1000000, 0, 0, 0x3E, 0x30);
        tilemap_clear_rect(0x1004000, 0, 0, 0x3E, 0x30);
        if (mem.template at<uint16_t>(0x51F702) != 0)
            palette_fade_finish();
        sprite_slots_mark();
        heap_reset();
        player_char_models_set();
        for (uint32_t i = 0; i < 0x10; i += 4)
            mem.set32(G::camera_pos + i, 0);
        mem.set32(G::camera_heading, 0);
        for (uint32_t p = 0; p < 2; ++p)
            if (mem.u32(G::players + p * 0xCC) == 4)
                player_state_set(6, 1, mem.u32(G::players + p * 0xCC + 0xA4));
        mem.set32(0x543640, task_group_create(0xCA790));
        mem.set32(0x520700, 0x168);
        mem.set32(G::camera_status, mem.u32(G::camera_status) & 0xFFFFFFFDu);
        sound_queue_reset();
        sound_send3(0xA00002);
        sound_send3(0xBE5000);
        sound_cue(0xA01003);
        mem.set32(G::game_submode, mem.u32(G::game_submode) + 1);
    }
}

// 0x97340 game_over_scene (sub-mode 1): the frame, the group (the camera, the fallen bodies); after 270 frames
// (timer below 90) the next sub-mode
template <class P> void Game<P>::game_over_scene()
{
    if constexpr (!P::pc) {
        Tgp::Out none;
        copro(0x08, nullptr, 0, none);   // 0x880080 = 0x808: the matrix stack reset
        geo_frame_begin();
        geo_view_setup();
        task_group_run(mem.u32(0x543640));
        const int32_t t = int32_t(mem.u32(0x520700)) - 1;
        mem.set32(0x520700, uint32_t(t));
        if (t < 0x5A)
            mem.set32(G::game_submode, mem.u32(G::game_submode) + 1);
    }
}

// 0x5CF30 map_scroll_set (pos): both map scroll shadows 0x51E8FE / 0x51E900
template <class P> void Game<P>::map_scroll_set(uint32_t pos)
{
    mem.template at<uint16_t>(0x51E8FE) = uint16_t(pos & 0x1FF);
    mem.template at<uint16_t>(0x51E900) = uint16_t(pos & 0x1FF);
}

// 0x5CB50 game_over_map_start (sub-mode 2): the route map: a running fade finished, the slots marked, the scroll at
// 0x480, the heap reset, the task group 0xC5440 (camera, route, walkers) in 0x5433E0, the walkers' scroll point
// (0x100, 0x150), the 180-frame timer 0x5433E4, the walkers' direction / fall flags, floor_y 0
template <class P> void Game<P>::game_over_map_start()
{
    if constexpr (!P::pc) {
        if (mem.template at<uint16_t>(0x51F702) != 0)
            palette_fade_finish();
        sprite_slots_mark();
        map_scroll_set(0x480);
        heap_reset();
        mem.set32(0x5433E0, task_group_create(0xC5440));
        mem.template at<int16_t>(0x543400) = 0x100;
        mem.template at<int16_t>(0x543402) = 0x150;
        mem.set32(0x5433E4, 0xB4);
        mem.template at<uint8_t>(0x543404) = 0;
        mem.template at<uint8_t>(0x543405) = 0;
        mem.set32(G::floor_y, 0);
        mem.set32(G::game_submode, mem.u32(G::game_submode) + 1);
    }
}

// 0x5CBF0 game_over_map (sub-mode 3): the frame, the group; when the timer has run out the window layer 0x100C000
// cleared and the next sub-mode
template <class P> void Game<P>::game_over_map()
{
    if constexpr (!P::pc) {
        Tgp::Out none;
        copro(0x08, nullptr, 0, none);   // 0x880080 = 0x808: the matrix stack reset
        geo_frame_begin();
        geo_view_setup();
        task_group_run(mem.u32(0x5433E0));
        if (mem.u32(0x5433E4) == 0) {
            if (mem.mapped(0x100C000))
                for (uint32_t i = 0; i < 0xC00; i += 4)
                    mem.set32(0x100C000 + i, 0);
            mem.set32(G::game_submode, mem.u32(G::game_submode) + 1);
        }
    }
}

// 0x973A0 game_over_text (sub-mode 4): a running fade finished, the map's scroll back to 0, the full-screen GAME OVER
// (0x106) on layer 2, the task group 0xCA7A8 (camera_globals_reset) in 0x543640, the 180-frame timer
template <class P> void Game<P>::game_over_text()
{
    if constexpr (!P::pc) {
        if (mem.template at<uint16_t>(0x51F702) != 0)
            palette_fade_finish();
        mem.template at<uint16_t>(0x51E8FE) = 0;
        mem.template at<uint16_t>(0x51E900) = 0;
        sprite_bank_load(0x10, 0x106);
        sprite_draw_anim(0x1004000, 1, 0, 0, 0, 0x106);
        mem.set32(0x543640, task_group_create(0xCA7A8));
        mem.set32(0x520700, 0xB4);
        mem.set32(G::game_submode, mem.u32(G::game_submode) + 1);
    }
}

// 0x97430 game_over_wait (sub-mode 5): the timer; at 0 the play time of the players still in state 6 booked, the
// debug mode 0x51F42C cleared, the game booked, next_mode
template <class P> void Game<P>::game_over_wait()
{
    if constexpr (!P::pc) {
        const int32_t t = int32_t(mem.u32(0x520700)) - 1;
        mem.set32(0x520700, uint32_t(t));
        if (t == 0) {
            if (mem.u32(G::players) == 6)
                bookkeeping_play_end(0);
            if (mem.u32(G::players + 0xCC) == 6)
                bookkeeping_play_end(1);
            mem.set32(0x51F42C, 0);
            bookkeeping_game_end();
            next_mode();
        }
    }
}

} // namespace hotd::port
