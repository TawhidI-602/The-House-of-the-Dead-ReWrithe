// Enemies: kind 64, the researchers (kenkyu) of the scripted scenes (arcade cluster 0x6DAC0..0x70190), read from the
// typed decompilation and the arcade disassembly. Set-up by the record's action (+0x64, table kind64_setups 0xC6950):
// nine scenes (hanging, sno, hanging by a ladder, set-up 3, crawling, hands up, sno, in a cell, crawling); the set-ups
// are themselves states. kind64_frame runs the state routine (kind64_states 0xC68D0, by +0x5C) with sub-steps in
// +0x60. Words: +0x328 1 = a woman (by creature type: the scream), +0x32C the state to go back to after being struck
// (state 29), +0x338 the draw zone at the set-up; +0x324 bit 1 struck, bit 11 attack slot given up, bit 18 a
// researcher, bit 25 rescued (turns away), bit 26 hurt. Event flags (event_flags + n) start and end the scenes.
#include "port/game.h"

namespace hotd::port {

// 0x6DAC0 (kind 64): sub-step 0, the set-up of its action (kind64_setups)
template <class P> void Game<P>::kind64_kenkyu_researchers(uint32_t obj)
{
    const int32_t action = of<int16_t>(obj, 0x64);
    of<int16_t>(obj, 0x60) = 0;
    kind64_call(mem.u32(G::kind64_setups + uint32_t(action) * 4), obj);
}

// a call through kind64_setups / kind64_states: the ported routines directly, anything else as a boundary
template <class P> void Game<P>::kind64_call(uint32_t routine, uint32_t obj)
{
    switch (routine) {
    case F::kind64_st00_none: return;
    case F::kind64_st01_none: return;
    case F::kind64_set0_hanging: kind64_set0_hanging(obj); return;
    case F::kind64_st03_hang: kind64_st03_hang(obj); return;
    case F::kind64_st04_climb_up: kind64_st04_climb_up(obj); return;
    case F::kind64_st05_remove: kind64_st05_remove(obj); return;
    case F::kind64_set3: kind64_set3(obj); return;
    case F::kind64_st07_rescue: kind64_st07_rescue(obj); return;
    case F::kind64_st08_remove: kind64_st08_remove(obj); return;
    case F::kind64_set4_crawl: kind64_set4_crawl(obj); return;
    case F::kind64_st10_run: kind64_st10_run(obj); return;
    case F::kind64_st11_remove: kind64_st11_remove(obj); return;
    case F::kind64_set1_sno: kind64_set1_sno(obj); return;
    case F::kind64_st13_shot: kind64_st13_shot(obj); return;
    case F::kind64_st14_rescue: kind64_st14_rescue(obj); return;
    case F::kind64_st15_remove: kind64_st15_remove(obj); return;
    case F::kind64_set2_hanging_ladder: kind64_set2_hanging_ladder(obj); return;
    case F::kind64_st17_hang: kind64_st17_hang(obj); return;
    case F::kind64_st18_ladder: kind64_st18_ladder(obj); return;
    case F::kind64_st19_remove: kind64_st19_remove(obj); return;
    case F::kind64_set5_hold_up: kind64_set5_hold_up(obj); return;
    case F::kind64_st21_talk: kind64_st21_talk(obj); return;
    case F::kind64_st22_message: kind64_st22_message(obj); return;
    case F::kind64_set6_sno: kind64_set6_sno(obj); return;
    case F::kind64_st24_shot: kind64_st24_shot(obj); return;
    case F::kind64_st25_rescue: kind64_st25_rescue(obj); return;
    case F::kind64_st26_remove: kind64_st26_remove(obj); return;
    case F::kind64_set7_cell: kind64_set7_cell(obj); return;
    case F::kind64_set8_crawl: kind64_set8_crawl(obj); return;
    case F::kind64_st29_struck: kind64_st29_struck(obj); return;
    default: call_routine(routine, obj); return;
    }
}

// the set-ups' common start (inline in the arcade; PC 0x4CCE5C): body (MO 0xDA), type set-up, one more researcher,
// +0x324 bit 18, an enemy slot unless in event 14, kind64_frame, the draw zone; +0x328 by creature type: 17, 19-22
// a man (0), 23, 25-27 a woman (1), the others unchanged (the PC also has type 49 a man)
template <class P> void Game<P>::kind64_researcher_init(uint32_t obj)
{
    mem.set32(G::hit_type, uint32_t(int32_t(of<int16_t>(obj, 0xCC))));
    body_init(obj, int32_t(of<int16_t>(obj, 0xCC)), 0xDA);
    creature_type_setup(obj);
    mem.at<uint16_t>(G::hostages_appeared) = uint16_t(mem.at<uint16_t>(G::hostages_appeared) + 1);
    of<uint32_t>(obj, 0x324) = of<uint32_t>(obj, 0x324) | (1u << 18);
    if (mem.at<uint8_t>(G::event_index) != 14)
        enemy_slot_take(obj);
    of<int16_t>(obj, 0x68) = 0;
    of<uint32_t>(obj, 0x0) = F::kind64_frame;
    of<int32_t>(obj, 0x338) = int32_t(mem.at<int8_t>(G::draw_zone));
    const uint16_t type = of<uint16_t>(obj, 0xCC);
    if (pc_path()) {
        if (type == 0x11 || (type >= 0x13 && type <= 0x16) || type == 0x31)
            of<int32_t>(obj, 0x328) = 0;
        else if (type == 0x17 || (type >= 0x19 && type <= 0x1B))
            of<int32_t>(obj, 0x328) = 1;
    } else {
        const uint32_t k = uint32_t(int32_t(int16_t(uint16_t(type - 17))));
        if (k <= 10) {
            switch (k) {
            case 0: case 2: case 3: case 4: case 5: of<int32_t>(obj, 0x328) = 0; break;
            case 6: case 8: case 9: case 10: of<int32_t>(obj, 0x328) = 1; break;
            default: break;
            }
        }
    }
}

// 0x6DBD0: the state routine (kind64_states); drawn unless hidden (flags bit 25); anim_time; depth list; the
// attacker list unless its attack slot was given up (+0x324 bit 11)
template <class P> void Game<P>::kind64_frame(uint32_t obj)
{
    kind64_call(mem.u32(G::kind64_states + uint32_t(int32_t(of<int16_t>(obj, 0x5C))) * 4), obj);
    if (((of<uint32_t>(obj, 0x58) >> 25) & 1) == 0)
        object_draw(obj);
    of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) + 1;
    depth_list_add(obj);
    if (((of<uint16_t>(obj, 0x324) >> 11) & 1) == 0)
        attacker_list_add(obj);
}

// helpers (inline in the original)
template <class P> void Game<P>::kind64_slot_free(uint32_t obj)
{
    mem.at<uint8_t>(G::attack_slots + uint32_t(int32_t(of<int8_t>(obj, 0x2E7))) * 20) = 0;
}

template <class P> bool Game<P>::kind64_motion_done(uint32_t obj)
{
    return ((of<uint8_t>(obj, 0x3C5) >> 4) & 1) != 0;
}

template <class P> void Game<P>::kind64_to(uint32_t obj, int16_t state)
{
    of<int16_t>(obj, 0x60) = 0;
    of<int16_t>(obj, 0x5C) = state;
}

template <class P> void Game<P>::kind64_sub_next(uint32_t obj)
{
    of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
}

// the rescue counters: hostages_rescued (game), hostages_rescued_chapter
template <class P> void Game<P>::kind64_count_rescue()
{
    mem.at<uint16_t>(G::hostages_rescued) = uint16_t(mem.at<uint16_t>(G::hostages_rescued) + 1);
    mem.at<uint16_t>(G::hostages_rescued_chapter) = uint16_t(mem.at<uint16_t>(G::hostages_rescued_chapter) + 1);
}

// shot by a player (flags bit 8): researcher_hit_by_player, else (an event flag) the scream penalty
template <class P> void Game<P>::kind64_hurt(uint32_t obj, uint16_t flags)
{
    if (flags & 0x100)
        researcher_hit_by_player(obj);
    else
        researcher_scream_penalty(obj, of<uint8_t>(obj, 0x328), 0);
}

// coprocessor 0x0F: ang[1] = the yaw of (dx, dy, dz), turned half round when the pitch folds over
template <class P> void Game<P>::kind64_face(uint32_t obj, float dx, float dy, float dz)
{
    if (pc_path()) {
        uint32_t yaw = 0, pitch = 0;
        pc_vec_angles6(dx, dy, dz, yaw, pitch);
        of<int32_t>(obj, 0x30) = int32_t(yaw);
    } else {
        of<int32_t>(obj, 0x30) = int32_t(yaw_toward_a(dx, dy, dz));
    }
}

// PC 0x4CB010 (ax, az, bx, bz): the floor distance, (bx - ax)^2 + (bz - az)^2 through the table square root (PC only)
template <class P> float Game<P>::pc_floor_distance2(float ax, float az, float bx, float bz)
{
    const float dx = bx - ax, dz = bz - az;
    return pc_fast_sqrt_table(dx * dx + dz * dz);
}
template <class P> float Game<P>::pc_fast_sqrt7(float x) { return pc_fast_sqrt_table(x); }   // PC 0x4CB7CC
// PC 0x4CB078: the researchers file's copy of pc_vec_angles
template <class P> void Game<P>::pc_vec_angles6(float x, float y, float z, uint32_t& yaw, uint32_t& pitch)
{
    pc_vec_angles(x, y, z, yaw, pitch);
}

// 0x6DC30 (set-up 0, state 2): hanging (MO 0xDA); event flag 0x1A gives up its attack slot; shot (flags bit 8) or
// event flag 8: struck (+0x324 bit 1), state 3; no enemies alive: the slot again; no enemies left: state 4
template <class P> void Game<P>::kind64_set0_hanging(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        kind64_researcher_init(obj);
        of<int32_t>(obj, 0x30) = 0x8000;
        set_motion_blend(obj, 0xDA, 0, 8);
        of<int16_t>(obj, 0x5C) = 2;
        kind64_sub_next(obj);
    } else if (sub != 1) {
        return;
    }
    if (mem.at<uint8_t>(G::event_flags + 0x1A) == 1 && ((of<uint16_t>(obj, 0x324) >> 11) & 1) == 0) {
        kind64_slot_free(obj);
        of<int8_t>(obj, 0x2E7) = -1;
        of<uint32_t>(obj, 0x324) = of<uint32_t>(obj, 0x324) | (1u << 11);
    }
    const uint16_t f = of<uint16_t>(obj, 0x58);
    if (((f >> 8) & 1) != 0 || mem.at<uint8_t>(G::event_flags + 8) == 1) {
        of<uint32_t>(obj, 0x324) = of<uint32_t>(obj, 0x324) | 2;
        kind64_hurt(obj, f);
        kind64_to(obj, 3);
    }
    if (mem.at<int16_t>(G::enemies_alive) == 0 && ((of<uint16_t>(obj, 0x324) >> 11) & 1) != 0) {
        enemy_slot_take(obj);
        of<uint32_t>(obj, 0x324) = of<uint32_t>(obj, 0x324) & ~(1u << 11);
    }
    if (mem.at<int16_t>(G::enemy_count) == 0)
        kind64_to(obj, 4);
}

// the fall of a struck hanging researcher (states 3 / 17): gravity -0.027222222, z - 0.01 a frame; on the floor
// sound 0xA01223 and the fall motion (MO 0xD3, frame 70, turned by `turn`); at its end the next state
template <class P> void Game<P>::kind64_fall(uint32_t obj, uint32_t turn, int16_t next)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        of<uint32_t>(obj, 0x88) = 0xBCDF0123u;   // -0.027222222
        of<float>(obj, 0x7C) = 0.0f;
        of<int16_t>(obj, 0xF4) = 0x46;
        kind64_sub_next(obj);
    } else if (sub != 1) {
        if (sub != 2)
            return;
    }
    if (sub <= 1) {
        const float vy = of<float>(obj, 0x7C) + of<float>(obj, 0x88);
        const float y = of<float>(obj, 0x24) + vy;
        of<float>(obj, 0x28) = of<float>(obj, 0x28) - ffrom(0x3C23D70Au);
        of<float>(obj, 0x7C) = vy;
        of<float>(obj, 0x24) = y;
        of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
        if (y > mem.f32(G::floor_y))
            return;
        sound_cue(P::pc ? 0x30 : 0xA01223);
        of<float>(obj, 0x24) = mem.f32(G::floor_y);
        turn_motion_set_speed(obj, 0xD3, 0x46, 8, turn);
        kind64_sub_next(obj);
    }
    if (kind64_motion_done(obj)) {
        kind64_to(obj, next);
        of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
    }
}

// 0x6DE10 (state 3): the fall, then state 5
template <class P> void Game<P>::kind64_st03_hang(uint32_t obj)
{
    kind64_fall(obj, 0x8000, 5);
}

// 0x6DF10 (state 4): rescued: bonus, counted, not shootable, climbs up (MO 0x120), thanks (sound 0xA01332); after
// 60 frames event flag 7, its slot freed; at the motion's end state 5
template <class P> void Game<P>::kind64_st04_climb_up(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        researcher_rescue_bonus();
        kind64_count_rescue();
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
        of<uint16_t>(obj, 0x6C) = of<uint16_t>(obj, 0x6C) & 0xFFFD;
        set_motion_blend(obj, 0x120, 0, 8);
        of<int32_t>(obj, 0x30) = 0x8000;
        sound_cue(P::pc ? 0x79 : 0xA01332);
        of<int32_t>(obj, 0x50) = 0x3C;
        kind64_sub_next(obj);
    } else if (sub != 1 && sub != 2) {
        return;
    }
    if (sub <= 1) {
        const int32_t t = of<int32_t>(obj, 0x50) - 1;
        of<int32_t>(obj, 0x50) = t;
        if (t > 0)
            return;
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
        mem.at<uint8_t>(G::event_flags + 7) = 1;
        kind64_slot_free(obj);
        kind64_sub_next(obj);
    }
    if (kind64_motion_done(obj)) {
        kind64_to(obj, 5);
        of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
    }
}

// 0x6E020 (state 5): not shootable, its slot freed (if any), event flag 7, the camera hold off; ends at event flag
// 0x31
template <class P> void Game<P>::kind64_st05_remove(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
        if (of<int8_t>(obj, 0x2E7) != -1)
            kind64_slot_free(obj);
        mem.at<uint8_t>(G::event_flags + 7) = 1;
        mem.set32(G::camera_hold_hostage, 0);
        kind64_sub_next(obj);
    } else if (sub != 1) {
        return;
    }
    of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
    if (mem.at<uint8_t>(G::event_flags + 0x31) == 1) {
        body_free_parts(obj);
        task_exit();
    }
}

// 0x6E0A0 (set-up 1, state 12): sno (MO 0x108); shot or event flag 0xA: struck, state 13; no enemies alive and the
// motion done: state 14
template <class P> void Game<P>::kind64_set1_sno(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        kind64_researcher_init(obj);
        set_motion_blend(obj, 0x108, 0, 8);
        of<int16_t>(obj, 0x5C) = 0xC;
        kind64_sub_next(obj);
    } else if (sub != 1) {
        return;
    }
    if (((of<uint16_t>(obj, 0x58) >> 8) & 1) != 0 || mem.at<uint8_t>(G::event_flags + 0xA) == 1) {
        kind64_to(obj, 0xD);
        of<uint32_t>(obj, 0x324) = of<uint32_t>(obj, 0x324) | 2;
    }
    if (mem.at<int16_t>(G::enemies_alive) == 0 && kind64_motion_done(obj))
        kind64_to(obj, 0xE);
}

// 0x6E200 (state 13): struck: MO 0x107 and researcher_hit_by_player when shot, else MO 0x106 and the scream; at the
// motion's end state 15
template <class P> void Game<P>::kind64_st13_shot(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        if (((of<uint16_t>(obj, 0x58) >> 8) & 1) == 0) {
            set_motion_blend(obj, 0x106, 0, 8);
            researcher_scream_penalty(obj, of<uint8_t>(obj, 0x328), 0);
        } else {
            set_motion_blend(obj, 0x107, 0, 8);
            researcher_hit_by_player(obj);
        }
        kind64_sub_next(obj);
    } else if (sub != 1) {
        return;
    }
    if (kind64_motion_done(obj)) {
        kind64_to(obj, 0xF);
        of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
    }
}

// the gift scene of states 7 / 14 (from step `first`): runs to the camera, holds out the gift (MO 0xEB, hand model
// 0x313, the head's lip-sync frames from `head`), the gift; then MO 0xEC (hand 0x4DB), talking; MO 0x112
template <class P> bool Game<P>::kind64_gift(uint32_t obj, int16_t first, uint32_t sound, uint32_t head, uint32_t closed)
{
    int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == first) {
        turn_toward_camera(obj, 0x800);
        bool reached;
        if (pc_path()) {
            // PC: researcher_reached_camera inline: the second turn, the floor distance by the file's own copy
            turn_toward_camera(obj, 0xC0);
            reached = pc_floor_distance2(of<float>(obj, 0x20), of<float>(obj, 0x28), mem.f32(G::camera_pos),
                                         mem.f32(G::camera_pos + 8)) < 15.0f;
        } else {
            reached = researcher_reached_camera(obj) != 0;
        }
        if (!reached)
            return false;
        set_motion_blend(obj, 0xEB, 0, 2);
        body_role_model_set(obj, 6, 0x313);
        kind64_sub_next(obj);
        sub = int16_t(first + 1);
    }
    if (sub == first + 1) {
        if (of<int16_t>(obj, 0xF4) == 5)
            sound_cue(sound);
        const int32_t h = (of<int32_t>(obj, 0x50) % 0x20) / 2;
        if (h == 0xF)
            body_role_model_set(obj, 1, closed);
        else
            body_role_model_set(obj, 1, uint32_t(h) + head);
        of<int32_t>(obj, 0x50) = of<int32_t>(obj, 0x50) + 1;
        if (!kind64_motion_done(obj))
            return false;
        researcher_gift_spawn(obj);
        set_motion_blend(obj, 0xEC, 0, 2);
        body_role_model_set(obj, 6, 0x4DB);
        kind64_sub_next(obj);
        sub = int16_t(first + 2);
    }
    if (sub == first + 2) {
        body_role_model_set(obj, 1, uint32_t((of<int32_t>(obj, 0x50) % 0x1E) / 2) + head);
        of<int32_t>(obj, 0x50) = of<int32_t>(obj, 0x50) + 1;
        if (!kind64_motion_done(obj))
            return false;
        set_motion_blend(obj, 0x112, 0, 4);
        kind64_sub_next(obj);
        sub = int16_t(first + 3);
    }
    return sub == first + 3;
}

// 0x6E290 (state 14): rescued (scene_state 5, bonus, counted): MO 0x104; then runs to the camera (MO 0xFF, heading
// 0x4000, draw flag 7), the gift (sound 0xA0132A, head frames 0x4AF.., closed 0x4A9); after MO 0x112's frame 19: MO
// 0xFF, not shootable, its slot freed, turns to 0x5800 for 60 frames; then event flag 9, scene_state 1 (unless 2), ends
template <class P> void Game<P>::kind64_st14_rescue(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub < 0 || sub > 6)
        return;
    if (sub == 0) {
        mem.at<uint8_t>(G::scene_state) = 5;
        researcher_rescue_bonus();
        kind64_count_rescue();
        set_motion_blend(obj, 0x104, 0, 8);
        kind64_sub_next(obj);
    }
    if (sub <= 1) {
        if (!kind64_motion_done(obj))
            return;
        of<int32_t>(obj, 0x30) = 0x4000;
        set_motion(obj, 0xFF);
        of<uint8_t>(obj, 0x3C4) = of<uint8_t>(obj, 0x3C4) | 0x80;
        kind64_sub_next(obj);
    }
    if (sub <= 4 && !kind64_gift(obj, 2, P::pc ? 0x73 : 0xA0132A, 0x4AF, 0x4A9))
        return;
    if (sub <= 5) {
        if (of<int16_t>(obj, 0xF4) <= 0x13)
            return;
        set_motion_blend(obj, 0xFF, 0, 2);
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
        kind64_slot_free(obj);
        of<int32_t>(obj, 0x50) = 0x3C;
        kind64_sub_next(obj);
    }
    angle_turn_toward(obj, 0x5800, 0x800);
    const int32_t t = of<int32_t>(obj, 0x50) - 1;
    of<int32_t>(obj, 0x50) = t;
    if (t < 1) {
        mem.at<uint8_t>(G::event_flags + 9) = 1;
        if (mem.at<uint8_t>(G::scene_state) != 2)
            mem.at<uint8_t>(G::scene_state) = 1;
        body_free_parts(obj);
        task_exit();
    }
}

// the scene end shared by states 15 / 19 / 26: not shootable, the camera hold off, event flag `flag`, its slot freed,
// scene_state 1 (unless 2)
template <class P> void Game<P>::kind64_scene_end(uint32_t obj, uint32_t flag)
{
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
    const uint8_t s = mem.at<uint8_t>(G::scene_state);
    mem.set32(G::camera_hold_hostage, 0);
    mem.at<uint8_t>(G::event_flags + flag) = 1;
    kind64_slot_free(obj);
    if (s != 2)
        mem.at<uint8_t>(G::scene_state) = 1;
    kind64_sub_next(obj);
}

// 0x6E540 (state 15): the scene end (event flag 9); ends at event flag 0x36
template <class P> void Game<P>::kind64_st15_remove(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0)
        kind64_scene_end(obj, 9);
    else if (sub != 1)
        return;
    of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
    if (mem.at<uint8_t>(G::event_flags + 0x36) == 1) {
        body_free_parts(obj);
        task_exit();
    }
}

// 0x6E5D0 (set-up 2, state 16): hanging by a ladder (MO 0xDA, fade 0xE1); shot or event flag 0xC: struck, state 17;
// no enemies left: state 18
template <class P> void Game<P>::kind64_set2_hanging_ladder(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        kind64_researcher_init(obj);
        of<int32_t>(obj, 0x30) = 0x8000;
        set_motion(obj, 0xDA);
        of<int16_t>(obj, 0x2E0) = 0xE1;
        of<int16_t>(obj, 0x5C) = 0x10;
        kind64_sub_next(obj);
    } else if (sub != 1) {
        return;
    }
    const uint16_t f = of<uint16_t>(obj, 0x58);
    if (((f >> 8) & 1) != 0 || mem.at<uint8_t>(G::event_flags + 0xC) == 1) {
        of<uint32_t>(obj, 0x324) = of<uint32_t>(obj, 0x324) | 2;
        kind64_hurt(obj, f);
        kind64_to(obj, 0x11);
    }
    if (mem.at<int16_t>(G::enemy_count) == 0)
        kind64_to(obj, 0x12);
}

// 0x6E750 (state 17): the fall (turned the other way), then state 19
template <class P> void Game<P>::kind64_st17_hang(uint32_t obj)
{
    kind64_fall(obj, 0xFFFF8000u, 0x13);
}

// 0x6E850 (state 18): rescued (scene_state 5, bonus, counted, sound 0xA01332), not shootable, steps off the ladder
// (x - 1, z - 1), climbs (MO 0xE1), its slot freed; after 120 frames state 19
template <class P> void Game<P>::kind64_st18_ladder(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        mem.at<uint8_t>(G::scene_state) = 5;
        researcher_rescue_bonus();
        kind64_count_rescue();
        sound_cue(P::pc ? 0x79 : 0xA01332);
        const float z = of<float>(obj, 0x28);
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
        of<float>(obj, 0x20) = of<float>(obj, 0x20) + -1.0f;
        of<float>(obj, 0x28) = z + -1.0f;
        set_motion_blend(obj, 0xE1, 0, 4);
        kind64_slot_free(obj);
        of<int32_t>(obj, 0x50) = 0x78;
        kind64_sub_next(obj);
    } else if (sub != 1) {
        return;
    }
    const int32_t t = of<int32_t>(obj, 0x50) - 1;
    of<int32_t>(obj, 0x50) = t;
    if (t < 1)
        kind64_to(obj, 0x13);
}

// 0x6E930 (state 19): the scene end (event flag 0xB); ends once the draw zone has changed
template <class P> void Game<P>::kind64_st19_remove(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0)
        kind64_scene_end(obj, 0xB);
    else if (sub != 1)
        return;
    of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
    if (of<int32_t>(obj, 0x338) != int32_t(mem.at<int8_t>(G::draw_zone))) {
        body_free_parts(obj);
        task_exit();
    }
}

// 0x6E9C0 (set-up 3, state 6): MO 0xFD; struck: state 29 (back to 8); event flag 0x14: state 8; until event flag
// 0x12 not shootable by the shot test (flags bit 8 cleared); then z + 1, waits (kind64_hit_check) for event flag
// 0x15, MO 0xF7 and kind64_hit_check each frame
template <class P> void Game<P>::kind64_set3(uint32_t obj)
{
    int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        kind64_researcher_init(obj);
        set_motion(obj, 0xFD);
        of<int16_t>(obj, 0x5C) = 6;
        kind64_sub_next(obj);
        sub = 1;
    } else if (sub < 0 || sub > 3) {
        return;
    }
    if (sub == 1) {
        if (((of<uint8_t>(obj, 0x324) >> 1) & 1) != 0) {
            of<int32_t>(obj, 0x32C) = 8;
            kind64_to(obj, 0x1D);
        }
        if (mem.at<uint8_t>(G::event_flags + 0x14) == 1)
            kind64_to(obj, 8);
        if (mem.at<uint8_t>(G::event_flags + 0x12) != 1) {
            of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xFFFFFEFFu;
            return;
        }
        const float z = of<float>(obj, 0x28);
        kind64_sub_next(obj);
        of<float>(obj, 0x28) = z + 1.0f;
        sub = 2;
    }
    if (sub == 2) {
        kind64_hit_check(obj);
        of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
        if (mem.at<uint8_t>(G::event_flags + 0x15) != 1)
            return;
        set_motion_blend(obj, 0xF7, 0, 8);
        kind64_sub_next(obj);
    }
    kind64_hit_check(obj);
}

// 0x6EB90: shot or struck: state 29 (back to 8); no enemies alive and the motion done: state 7
template <class P> void Game<P>::kind64_hit_check(uint32_t obj)
{
    if (((of<uint16_t>(obj, 0x58) >> 8) & 1) != 0 || ((of<uint8_t>(obj, 0x324) >> 1) & 1) != 0) {
        of<int32_t>(obj, 0x32C) = 8;
        kind64_to(obj, 0x1D);
    }
    if (mem.at<int16_t>(G::enemies_alive) == 0 && kind64_motion_done(obj))
        kind64_to(obj, 7);
}

// 0x6EBE0 (state 7): rescued (scene_state 5, bonus, counted, +0x324 bit 25): runs to the camera (MO 0xFF, draw flag
// 7), the gift (sound 0xA01332, head frames 0x34F.., closed 0x33E); after MO 0x112's frame 19: MO 0xFF, state 8
template <class P> void Game<P>::kind64_st07_rescue(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub < 0 || sub > 4)
        return;
    if (sub == 0) {
        if (mem.at<uint8_t>(G::scene_state) != 5)
            mem.at<uint8_t>(G::scene_state) = 5;
        researcher_rescue_bonus();
        kind64_count_rescue();
        of<uint32_t>(obj, 0x324) = of<uint32_t>(obj, 0x324) | (1u << 25);
        set_motion_blend(obj, 0xFF, 0, 2);
        of<uint8_t>(obj, 0x3C4) = of<uint8_t>(obj, 0x3C4) | 0x80;
        kind64_sub_next(obj);
    }
    if (sub <= 3 && !kind64_gift(obj, 1, P::pc ? 0x79 : 0xA01332, 0x34F, 0x33E))
        return;
    if (of<int16_t>(obj, 0xF4) > 0x13) {
        set_motion_blend(obj, 0xFF, 0, 2);
        kind64_to(obj, 8);
    }
}

// 0x6EE00 (state 8): not shootable, event flags 0xE / 0x10, the camera hold off, its slot freed, scene_state 1
// (unless 2); rescued ones turn to 0x9800; ends at event flag 0x14 or once behind the camera (view z < 0)
template <class P> void Game<P>::kind64_st08_remove(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
        const uint8_t s = mem.at<uint8_t>(G::scene_state);
        mem.at<uint8_t>(G::event_flags + 0xE) = 1;
        mem.set32(G::camera_hold_hostage, 0);
        mem.at<uint8_t>(G::event_flags + 0x10) = 1;
        kind64_slot_free(obj);
        if (s != 2)
            mem.at<uint8_t>(G::scene_state) = 1;
        kind64_sub_next(obj);
    } else if (sub != 1) {
        return;
    }
    if (((of<uint32_t>(obj, 0x324) >> 25) & 1) == 0)
        of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
    else
        angle_turn_toward(obj, 0x9800, 0x800);
    if (mem.at<uint8_t>(G::event_flags + 0x14) != 1 && 0.0f <= of<float>(obj, 0x40))
        return;
    body_free_parts(obj);
    task_exit();
}

// 0x6EEC0 (set-up 4, state 9): crawls (MO 0xE4, FixMode); struck: state 29 (back to 11); event flag 0x14: state 11;
// until event flag 0x12 not shootable by the shot test; then frame 59 held until event flag 0x15, gets up (MO 0xDE);
// at its end (enemies left) cry 0xA01327 and MO 0xF8; kind64_set4_next each frame
template <class P> void Game<P>::kind64_set4_crawl(uint32_t obj)
{
    int16_t sub = of<int16_t>(obj, 0x60);
    if (sub < 0 || sub > 4)
        return;
    if (sub == 0) {
        kind64_researcher_init(obj);
        set_motion(obj, 0xE4);
        of<int16_t>(obj, 0x5C) = 9;
        of<uint8_t>(obj, 0x3C6) = of<uint8_t>(obj, 0x3C6) | 1;
        kind64_sub_next(obj);
        sub = 1;
    }
    if (sub == 1) {
        if (((of<uint8_t>(obj, 0x324) >> 1) & 1) != 0) {
            of<int32_t>(obj, 0x32C) = 0xB;
            kind64_to(obj, 0x1D);
        }
        if (mem.at<uint8_t>(G::event_flags + 0x14) == 1)
            kind64_to(obj, 0xB);
        if (mem.at<uint8_t>(G::event_flags + 0x12) != 1) {
            of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xFFFFFEFFu;
            return;
        }
        kind64_sub_next(obj);
        sub = 2;
    }
    if (sub == 2) {
        kind64_set4_next(obj);
        of<int16_t>(obj, 0xF4) = 0x3B;
        of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
        if (mem.at<uint8_t>(G::event_flags + 0x15) != 1)
            return;
        set_motion_blend(obj, 0xDE, 0, 4);
        kind64_sub_next(obj);
        sub = 3;
    }
    if (sub == 3) {
        kind64_set4_next(obj);
        if (!kind64_motion_done(obj))
            return;
        of<uint8_t>(obj, 0x3C6) = of<uint8_t>(obj, 0x3C6) & 0xFE;
        if (mem.at<int16_t>(G::enemy_count) != 0) {
            sound_cue(P::pc ? 0x70 : 0xA01327);
            set_motion(obj, 0xF8);
        }
        kind64_sub_next(obj);
    }
    if (mem.at<int16_t>(G::enemy_count) == 0)
        of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
    kind64_set4_next(obj);
}

// 0x6F100: shot or struck: state 29 (back to 11); no enemies left: state 10
template <class P> void Game<P>::kind64_set4_next(uint32_t obj)
{
    if (((of<uint16_t>(obj, 0x58) >> 8) & 1) != 0 || ((of<uint8_t>(obj, 0x324) >> 1) & 1) != 0) {
        of<int32_t>(obj, 0x32C) = 0xB;
        kind64_to(obj, 0x1D);
    }
    if (mem.at<int16_t>(G::enemy_count) == 0)
        kind64_to(obj, 0xA);
}

// 0x6F140 (state 10): rescued (scene_state 5, bonus, counted, +0x324 bit 25, sound 0xA0132A), runs (MO 0xFF), state 11
template <class P> void Game<P>::kind64_st10_run(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        if (mem.at<uint8_t>(G::scene_state) != 5)
            mem.at<uint8_t>(G::scene_state) = 5;
        researcher_rescue_bonus();
        kind64_count_rescue();
        of<uint32_t>(obj, 0x324) = of<uint32_t>(obj, 0x324) | (1u << 25);
        sound_cue(P::pc ? 0x73 : 0xA0132A);
        kind64_sub_next(obj);
    } else if (sub != 1) {
        return;
    }
    set_motion(obj, 0xFF);
    kind64_to(obj, 0xB);
}

// 0x6F1E0 (state 11): not shootable, event flags 0xE / 0x10, its slot freed, the camera hold off; hurt ones
// (+0x324 bit 26) hold the motion; ends at event flag 0x14 (scene_state 1 unless 2)
template <class P> void Game<P>::kind64_st11_remove(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
        mem.at<uint8_t>(G::event_flags + 0xE) = 1;
        mem.at<uint8_t>(G::event_flags + 0x10) = 1;
        kind64_slot_free(obj);
        mem.set32(G::camera_hold_hostage, 0);
        kind64_sub_next(obj);
    } else if (sub != 1) {
        return;
    }
    if (((of<uint32_t>(obj, 0x324) >> 26) & 1) != 0)
        of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
    if (mem.at<uint8_t>(G::event_flags + 0x14) == 1) {
        if (mem.at<uint8_t>(G::scene_state) != 2)
            mem.at<uint8_t>(G::scene_state) = 1;
        body_free_parts(obj);
        task_exit();
    }
}

// 0x6F280 (set-up 7, state 27): in a cell (MO 0xFD, not shootable); at event flag 0x3F points (MO 0xFE, hand 0x367),
// at its end MO 0xFD (hand 0x36B); at event flags 0x39 and 0x40: MO 0xF4 (hand 0x367), shootable, an enemy slot,
// state 6 step 1
template <class P> void Game<P>::kind64_set7_cell(uint32_t obj)
{
    int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        kind64_researcher_init(obj);
        set_motion(obj, 0xFD);
        of<int16_t>(obj, 0x5C) = 0x1B;
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
        kind64_sub_next(obj);
        sub = 1;
    } else if (sub < 0 || sub > 3) {
        return;
    }
    if (sub == 1) {
        if (mem.at<uint8_t>(G::event_flags + 0x3F) != 1)
            return;
        set_motion_blend(obj, 0xFE, 0, 4);
        body_role_model_set(obj, 6, 0x367);
        kind64_sub_next(obj);
        sub = 2;
    }
    if (sub == 2) {
        if (!kind64_motion_done(obj))
            return;
        set_motion_blend(obj, 0xFD, 0, 4);
        body_role_model_set(obj, 6, 0x36B);
        kind64_sub_next(obj);
    }
    if (mem.at<uint8_t>(G::event_flags + 0x39) == 1 && mem.at<uint8_t>(G::event_flags + 0x40) == 1) {
        set_motion_blend(obj, 0xF4, 0, 4);
        body_role_model_set(obj, 6, 0x367);
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | 1;
        enemy_slot_take(obj);
        of<int16_t>(obj, 0x5C) = 6;
        of<int16_t>(obj, 0x60) = 1;
    }
}

// 0x6F480 (set-up 8, state 28): crawling (MO 0xE4, FixMode, not shootable); at event flag 0x39 shootable, an enemy
// slot, state 9 step 1
template <class P> void Game<P>::kind64_set8_crawl(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        kind64_researcher_init(obj);
        set_motion(obj, 0xE4);
        of<int16_t>(obj, 0x5C) = 0x1C;
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
        of<uint8_t>(obj, 0x3C6) = of<uint8_t>(obj, 0x3C6) | 1;
        kind64_sub_next(obj);
    } else if (sub != 1) {
        return;
    }
    if (mem.at<uint8_t>(G::event_flags + 0x39) == 1) {
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | 1;
        enemy_slot_take(obj);
        of<int16_t>(obj, 0x5C) = 9;
        of<int16_t>(obj, 0x60) = 1;
    }
}

// 0x6F5D0 (set-up 5, state 20): flees (MO 0x10F, fade 0xE1; counted in scene_count_520794, not shootable, its slot
// freed); past x -133 cry 0xA01338 and shootable; faces kind64_hold_up_point (-180, -75, 334); within 1.5 of it
// (floor distance) hands up: heading -0x4000, MO 0xE7. Shot or struck: route_choice 1, state 29 (back to 22); event
// flag 0x38: state 22; no enemies left: route_choice 0, state 21
template <class P> void Game<P>::kind64_set5_hold_up(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        kind64_researcher_init(obj);
        mem.at<uint16_t>(G::scene_count_520794) = uint16_t(mem.at<uint16_t>(G::scene_count_520794) + 1);
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
        kind64_slot_free(obj);
        set_motion(obj, 0x10F);
        of<int16_t>(obj, 0x2E0) = 0xE1;
        of<int16_t>(obj, 0x5C) = 0x14;
        kind64_sub_next(obj);
    }
    if (sub == 0 || sub == 1) {
        if (of<float>(obj, 0x20) < -133.0f && (of<uint8_t>(obj, 0x58) & 1) == 0) {
            sound_cue(P::pc ? 0x7B : 0xA01338);
            of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | 1;
        }
        const float px = mem.f32(G::kind64_hold_up_point), py = mem.f32(G::kind64_hold_up_point + 4);
        const float pz = mem.f32(G::kind64_hold_up_point + 8);
        kind64_face(obj, px - of<float>(obj, 0x20), py - of<float>(obj, 0x24), pz - of<float>(obj, 0x28));
        float d;
        if (pc_path()) {
            const float dx = px - of<float>(obj, 0x20), dz = pz - of<float>(obj, 0x28);
            d = pc_fast_sqrt7(dx * dx + dz * dz);
        } else {
            d = floor_distance(of<float>(obj, 0x20), of<float>(obj, 0x28), px, pz);
        }
        if (!(d > 1.5f)) {
            of<int32_t>(obj, 0x30) = int32_t(0xFFFFC000u);
            set_motion_blend(obj, 0xE7, 0, 8);
            kind64_sub_next(obj);
        }
    }
    if (((of<uint16_t>(obj, 0x58) >> 8) & 1) != 0 || ((of<uint8_t>(obj, 0x324) >> 1) & 1) != 0) {
        mem.at<uint8_t>(G::route_choice) = 1;
        of<int16_t>(obj, 0x60) = 0;
        of<int32_t>(obj, 0x32C) = 0x16;
        of<int16_t>(obj, 0x5C) = 0x1D;
    }
    if (mem.at<uint8_t>(G::event_flags + 0x38) == 1)
        kind64_to(obj, 0x16);
    if (mem.at<int16_t>(G::enemy_count) == 0) {
        mem.at<uint8_t>(G::route_choice) = 0;
        kind64_to(obj, 0x15);
    }
}

// 0x6F8F0 (state 21): rescued (scene_state 5, bonus, counted, one less in scene_count_520794), talks (MO 0x10C); state
// 22
template <class P> void Game<P>::kind64_st21_talk(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        mem.at<uint8_t>(G::scene_state) = 5;
        researcher_rescue_bonus();
        kind64_count_rescue();
        mem.at<uint16_t>(G::scene_count_520794) = uint16_t(mem.at<uint16_t>(G::scene_count_520794) - 1);
        set_motion(obj, 0x10C);
    } else if (sub != 1) {
        return;
    }
    kind64_to(obj, 0x16);
}

// 0x6F970 (state 22): not shootable, event flag 0x37, its slot freed, the camera hold off; unhurt: the head's
// lip-sync frames (0x34F.., closed 0x33E), voice message 16 at camera frame 45, scene_state 1 (unless 2) at 80, faces
// the camera; ends at event flag 0x38
template <class P> void Game<P>::kind64_st22_message(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
        mem.at<uint8_t>(G::event_flags + 0x37) = 1;
        kind64_slot_free(obj);
        mem.set32(G::camera_hold_hostage, 0);
        kind64_sub_next(obj);
    } else if (sub != 1) {
        return;
    }
    if (((of<uint16_t>(obj, 0x58) >> 8) & 1) == 0 && ((of<uint8_t>(obj, 0x324) >> 1) & 1) == 0) {
        const int32_t h = (of<int32_t>(obj, 0x50) % 0x20) / 2;
        if (h == 0xF)
            body_role_model_set(obj, 1, 0x33E);
        else
            body_role_model_set(obj, 1, uint32_t(h) + 0x34F);
        // (the PC compares the camera frame's float bits as integers)
        const float frame = mem.f32(G::camera_records + 0xEC);
        of<int32_t>(obj, 0x50) = of<int32_t>(obj, 0x50) + 1;
        if (P::pc ? mem.u32(G::camera_records + 0xEC) == 0x42340000u : frame == 45.0f)
            voice_message(0x10);
        if ((P::pc ? mem.u32(G::camera_records + 0xEC) == 0x42A00000u : mem.f32(G::camera_records + 0xEC) == 80.0f) &&
            mem.at<uint8_t>(G::scene_state) != 2)
            mem.at<uint8_t>(G::scene_state) = 1;
        kind64_face(obj, mem.f32(G::camera_pos) - of<float>(obj, 0x20), mem.f32(G::camera_pos + 4) - of<float>(obj, 0x24),
                    mem.f32(G::camera_pos + 8) - of<float>(obj, 0x28));
    } else {
        of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
    }
    if (mem.at<uint8_t>(G::event_flags + 0x38) == 1) {
        body_free_parts(obj);
        task_exit();
    }
}

// 0x6FB40 (set-up 6, state 23): sno (MO 0x108, head 0x33C; counted in scene_count_520794); shot or struck: state 24;
// no enemies left: state 25
template <class P> void Game<P>::kind64_set6_sno(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        kind64_researcher_init(obj);
        body_role_model_set(obj, 1, 0x33C);
        mem.at<uint16_t>(G::scene_count_520794) = uint16_t(mem.at<uint16_t>(G::scene_count_520794) + 1);
        set_motion_blend(obj, 0x108, 0, 8);
        of<int16_t>(obj, 0x5C) = 0x17;
        kind64_sub_next(obj);
    } else if (sub != 1) {
        return;
    }
    if (((of<uint16_t>(obj, 0x58) >> 8) & 1) != 0 || ((of<uint8_t>(obj, 0x324) >> 1) & 1) != 0)
        kind64_to(obj, 0x18);
    if (mem.at<int16_t>(G::enemy_count) == 0)
        kind64_to(obj, 0x19);
}

// 0x6FCB0 (state 24): one less in scene_count_520794, hurt (+0x324 bit 26); as state 13; then state 26
template <class P> void Game<P>::kind64_st24_shot(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        const uint32_t f = of<uint32_t>(obj, 0x58);
        mem.at<uint16_t>(G::scene_count_520794) = uint16_t(mem.at<uint16_t>(G::scene_count_520794) - 1);
        of<uint32_t>(obj, 0x324) = of<uint32_t>(obj, 0x324) | (1u << 26);
        if (((f >> 8) & 1) == 0) {
            set_motion_blend(obj, 0x106, 0, 8);
            researcher_scream_penalty(obj, of<uint8_t>(obj, 0x328), 0);
        } else {
            set_motion_blend(obj, 0x107, 0, 8);
            researcher_hit_by_player(obj);
        }
        kind64_sub_next(obj);
    } else if (sub != 1) {
        return;
    }
    if (kind64_motion_done(obj)) {
        kind64_to(obj, 0x1A);
        of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
    }
}

// 0x6FD60 (state 25): rescued (scene_state 5, bonus, counted, one less in scene_count_520794, +0x324 bit 25): MO 0x101;
// at its end voice message 28, MO 0x114 facing the camera (heading 0x4000 first); after its frame 19 MO 0xFF, not
// shootable, event flag 7, its slot freed, scene_state 1 (unless 2); then turns to 0x7800; ends at event flag 8
template <class P> void Game<P>::kind64_st25_rescue(uint32_t obj)
{
    int16_t sub = of<int16_t>(obj, 0x60);
    if (sub < 0 || sub > 3)
        return;
    if (sub == 0) {
        mem.at<uint8_t>(G::scene_state) = 5;
        researcher_rescue_bonus();
        kind64_count_rescue();
        mem.at<uint16_t>(G::scene_count_520794) = uint16_t(mem.at<uint16_t>(G::scene_count_520794) - 1);
        of<uint32_t>(obj, 0x324) = of<uint32_t>(obj, 0x324) | (1u << 25);
        set_motion_blend(obj, 0x101, 0, 8);
        kind64_sub_next(obj);
        sub = 1;
    }
    if (sub == 1) {
        if (!kind64_motion_done(obj))
            return;
        voice_message(0x1C);
        of<int32_t>(obj, 0x30) = 0x4000;
        set_motion(obj, 0x114);
        kind64_face(obj, mem.f32(G::camera_pos) - of<float>(obj, 0x20), 0.0f,
                    mem.f32(G::camera_pos + 8) - of<float>(obj, 0x28));
        kind64_sub_next(obj);
        sub = 2;
    }
    if (sub == 2) {
        if (of<int16_t>(obj, 0xF4) <= 0x13)
            return;
        set_motion_blend(obj, 0xFF, 0, 2);
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
        const uint8_t s = mem.at<uint8_t>(G::scene_state);
        mem.at<uint8_t>(G::event_flags + 7) = 1;
        kind64_slot_free(obj);
        if (s != 2)
            mem.at<uint8_t>(G::scene_state) = 1;
        kind64_sub_next(obj);
    }
    angle_turn_toward(obj, 0x7800, 0x800);
    if (mem.at<uint8_t>(G::event_flags + 8) != 1)
        return;
    body_free_parts(obj);
    task_exit();
}

// 0x6FF50 (state 26): the scene end (event flag 7); ends at event flag 8
template <class P> void Game<P>::kind64_st26_remove(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0)
        kind64_scene_end(obj, 7);
    else if (sub != 1)
        return;
    of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
    if (mem.at<uint8_t>(G::event_flags + 8) == 1) {
        body_free_parts(obj);
        task_exit();
    }
}

// 0x6FFE0 (state 29): struck (in event 9 one less in scene_count_520794; hurt, +0x324 bit 26). By an enemy (flags
// bit 8 clear): a hit effect 4 above the body part at +0x1A8 (coprocessor 0x74 / 0x1A / 0x06), MO 0xD3 (in chapter 0
// events 6 / 14: a man MO 0xE9; a woman crawling, MO 0xF8, cut to 0xD3), the scream. Shot by a player: MO 0xD3 (cut
// when crawling), researcher_hit_by_player. At the motion's end back to the saved state (+0x32C)
template <class P> void Game<P>::kind64_st29_struck(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        if (mem.at<uint8_t>(G::event_index) == 9)
            mem.at<uint16_t>(G::scene_count_520794) = uint16_t(mem.at<uint16_t>(G::scene_count_520794) - 1);
        const uint16_t f = of<uint16_t>(obj, 0x58);
        of<uint32_t>(obj, 0x324) = of<uint32_t>(obj, 0x324) | (1u << 26);
        if (((f >> 8) & 1) != 0) {
            if (of<int16_t>(obj, 0xD0) == 0xF8)
                set_motion(obj, 0xD3);
            else
                set_motion_blend(obj, 0xD3, 0, 8);
            researcher_hit_by_player(obj);
        } else {
            kind64_struck_effect(obj);
            uint32_t m = 0xD3;
            bool blend = true;
            if (mem.at<uint8_t>(G::chapter) == 0 &&
                (mem.at<uint8_t>(G::event_index) == 6 || mem.at<uint8_t>(G::event_index) == 14)) {
                if (of<int32_t>(obj, 0x328) == 0)
                    m = 0xE9;
                else if (of<int16_t>(obj, 0xD0) == 0xF8)
                    blend = false;
            }
            if (blend)
                set_motion_blend(obj, m, 0, 8);
            else
                set_motion(obj, 0xD3);
            researcher_scream_penalty(obj, of<uint8_t>(obj, 0x328), 0);
        }
        kind64_sub_next(obj);
    } else if (sub != 1) {
        return;
    }
    if ((of<uint8_t>(obj, 0x3C5) & 0x10) != 0) {
        const int32_t t = of<int32_t>(obj, 0xF8);
        const int16_t back = of<int16_t>(obj, 0x32C);
        of<int16_t>(obj, 0x60) = 0;
        of<int32_t>(obj, 0xF8) = t - 1;
        of<int16_t>(obj, 0x5C) = back;
    }
}

// state 29's hit effect: the body part point +0x1A8 in world space (coprocessor 0x74 1 / 0x1A / 0x06), 4 higher in z
template <class P> void Game<P>::kind64_struck_effect(uint32_t obj)
{
    if (!pc_path()) {
        Tgp::Out o, none;
        const uint32_t slot[1] = {1};
        copro(0x74, slot, 1, none);
        const uint32_t v[3] = {of<uint32_t>(obj, 0x1A8), of<uint32_t>(obj, 0x1AC), of<uint32_t>(obj, 0x1B0)};
        copro(0x1A, v, 3, o);
        const float z = ffrom(o.w[2]) + 4.0f;
        copro(0x06, nullptr, 0, none);
        hit_effect_at(ffrom(o.w[0]), ffrom(o.w[1]), z);
    } else {
        // PC: stored matrix 1 (the camera) on the software stack, the point through it (the file's copy 0x4CAF90)
        pc_matrix_push();
        pcm_load_camera();
        float p[3];
        pc_matrix_transform(of<float>(obj, 0x1A8), of<float>(obj, 0x1AC), of<float>(obj, 0x1B0), p[0], p[1], p[2]);
        p[2] = p[2] + 4.0f;
        pcm_pop();
        hit_effect_at(p[0], p[1], p[2]);
    }
}

} // namespace hotd::port
