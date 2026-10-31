// Enemies: kind 65, the axe zombie of the scripted researcher scenes (arcade cluster 0x8A3F0..0x8BDE8), read from the
// arcade disassembly. Set-up by the record's action (+0x64, table kind65_setups 0xCA4B0): five scenes (walk and
// chop, stance and chop, pull the axe out and chase a researcher, dash and chop, pull out); its own frame
// (kind65_frame) runs a state routine (kind65_states 0xCA470, by +0x5C) with sub-steps in +0x60; after the scene it
// becomes a standard enemy (enemy_main). Flags +0x58: bit 0 shootable, 24 / 27 / 28 hit reaction running, 25 hidden
// (blinking while dying), 31 attacking. Words: +0x50 timer, +0x334 / +0x338 hit reaction, +0x330 the attack position it
// walked to, +0x35C the researcher it chases, +0x3AC.. its position before an attack.
#include "port/game.h"

#include <cmath>

namespace hotd::port {

// coprocessor 0x0F: the yaw of (dx, dy, dz), turned half round when the pitch folds over (as enemy_turn_a)
// (PC: pc_vec_angles' yaw; the kind file's copy 0x4F8AA8)
template <class P> uint32_t Game<P>::yaw_toward_a(float dx, float dy, float dz)
{
    if (pc_path()) {
        uint32_t yaw = 0, pitch = 0;
        pc_vec_angles(dx, dy, dz, yaw, pitch);
        return yaw;
    } else {
        Tgp::Out o;
        const uint32_t in[3] = {fbits(dx), fbits(dy), fbits(dz)};
        copro(0x0F, in, 3, o);
        uint32_t yaw = o.w[0];
        const uint32_t s = uint32_t(int32_t(int16_t(o.w[1] & 0xFFFF)));
        if (((s << 1) ^ s) & 0x8000)
            yaw += 0x8000;
        return yaw;
    }
}

// 0x8A400 (kind 65): sub-step 0, the set-up of its action (kind65_setups)
template <class P> void Game<P>::kind65_zombie_scene(uint32_t obj)
{
    const int32_t action = of<int16_t>(obj, 0x64);
    of<int16_t>(obj, 0x60) = 0;
    kind65_call(mem.u32(G::kind65_setups + uint32_t(action) * 4), obj);
}

// a call through kind65_setups / kind65_states: the ported routines directly, anything else as a boundary
template <class P> void Game<P>::kind65_call(uint32_t routine, uint32_t obj)
{
    switch (routine) {
    case F::kind65_st0_none: return;
    case F::kind65_st1_none: return;
    case F::kind65_st2_damage: kind65_st2_damage(obj); return;
    case F::kind65_st3_die: kind65_st3_die(obj); return;
    case F::kind65_st4_walk: kind65_st4_walk(obj); return;
    case F::kind65_st5_approach: kind65_st5_approach(obj); return;
    case F::kind65_st6_pick_target: kind65_st6_pick_target(obj); return;
    case F::kind65_st7_attack: kind65_st7_attack(obj); return;
    case F::kind65_scene0_walk_chop: kind65_scene0_walk_chop(obj); return;
    case F::kind65_scene1_stance_chop: kind65_scene1_stance_chop(obj); return;
    case F::kind65_scene2_pull_out: kind65_scene2_pull_out(obj); return;
    case F::kind65_scene3_dash_chop: kind65_scene3_dash_chop(obj); return;
    case F::kind65_scene4_pull_out: kind65_scene4_pull_out(obj); return;
    default: call_routine(routine, obj); return;
    }
}

// the scenes' common start: body (MO 0x150), type set-up; counted and an enemy slot unless in event 14
template <class P> void Game<P>::kind65_body(uint32_t obj, bool slotUnless14)
{
    body_init(obj, int32_t(of<int16_t>(obj, 0xCC)), 0x150);
    mem.set32(G::hit_type, uint32_t(int32_t(of<int16_t>(obj, 0xCC))));
    creature_type_setup(obj);
    if (!slotUnless14 || mem.at<uint8_t>(G::event_index) != 14) {
        mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) + 1);
        mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) + 1);
        if (slotUnless14)
            enemy_slot_take(obj);
    }
}

// 0x8A4F0: hits; a hit reaction (flags bit 28); the state routine unless one is running (bit 27); drawn unless
// hidden (bit 25); anim_time and +0x10C count; depth and attacker lists
template <class P> void Game<P>::kind65_frame(uint32_t obj)
{
    kind65_hit_check(obj);
    if ((of<uint32_t>(obj, 0x58) >> 28) & 1)
        kind65_st2_damage(obj);
    if (((of<uint32_t>(obj, 0x58) >> 27) & 1) == 0)
        kind65_call(mem.u32(G::kind65_states + uint32_t(int32_t(of<int16_t>(obj, 0x5C))) * 4), obj);
    if (((of<uint32_t>(obj, 0x58) >> 25) & 1) == 0)
        object_draw(obj);
    of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) + 1;
    of<int32_t>(obj, 0x10C) = of<int32_t>(obj, 0x10C) + 1;
    depth_list_add(obj);
    attacker_list_add(obj);
}

// 0x8A570: part hits; a hit that does damage (flags bit 8, damage and life > 0) or return_false: hit reaction
// (bits 24 / 28), life - damage; at 0 (or return_false) it dies (state 3, the hit class % 9 in +0x334, one less
// alive); else the hit voice
template <class P> void Game<P>::kind65_hit_check(uint32_t obj)
{
    mem.at<uint8_t>(G::hit_class) = 0;
    mem.at<int16_t>(G::hit_damage) = 0;
    mem.set32(G::hit_type, uint32_t(int32_t(of<int16_t>(obj, 0xCC))));
    enemy_part_hits(obj);
    bool hit = false;
    if (((of<uint16_t>(obj, 0x58) >> 8) & 1) && mem.at<int16_t>(G::hit_damage) > 0 && of<int16_t>(obj, 0x70) > 0)
        hit = true;
    else if (return_false() != 0)
        hit = true;
    if (!hit)
        return;
    uint32_t f = of<uint32_t>(obj, 0x58);
    f &= ~(1u << 8);
    f |= 1u << 28;
    f |= 1u << 24;
    of<uint32_t>(obj, 0x58) = f;
    const int16_t life = int16_t(of<uint16_t>(obj, 0x70) - mem.at<uint16_t>(G::hit_damage));
    of<int16_t>(obj, 0x70) = life;
    if (life > 0 && return_false() == 0) {
        enemy_voice(obj, 0);
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xFFFFFFF9u;
        return;
    }
    mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) - 1);
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xE7FFFFF9u;
    const int32_t c = int32_t(mem.at<int8_t>(G::hit_class)) % 9;
    of<int16_t>(obj, 0x60) = 0;
    of<int16_t>(obj, 0x5C) = 3;
    of<int32_t>(obj, 0x334) = int8_t(c);
}

// the hit motion of class c (hit class % 9 and / 9) from the type's table 0x14B900
template <class P> uint32_t Game<P>::kind65_hit_motion(int32_t hc)
{
    const int32_t r = hc % 9, q = hc / 9;
    const uint32_t t = mem.u32(G::hit_motions_by_type + mem.u32(G::hit_type) * 4);
    return mem.u32(t + uint32_t(int32_t(int8_t(q)) * 4 + int32_t(int8_t(r)) * 8));
}

// 0x8A680: the hit reaction. Step 0 (+0x338), not while attacking (flags < 0): by the hit class % 9: 2 / 5 hit
// voice 4 and MO 0x1E2 (keeping the motion in +0xD2), 1 / 3 / 4 the class's motion held (blend hold 0, 1, 8),
// 6 / 7 / 8 the class's motion; then step 1: when the reaction is done (+0xF6 > 0) back to the saved motion
// (+0xD2, blend 0 / 2), else the bits 24 / 28 cleared once the motion allows (+0x3C6 bit 5 clear)
template <class P> void Game<P>::kind65_st2_damage(uint32_t obj)
{
    const uint32_t step = of<uint32_t>(obj, 0x338);
    if (step != 0 && step != 1)
        return;
    if (step == 0) {
        if (of<int32_t>(obj, 0x58) < 0)
            return;
        const int32_t hc = mem.at<int8_t>(G::hit_class);
        const uint32_t k = uint32_t(int32_t(int8_t(hc % 9)));
        bool mark = false;
        if (k <= 8) {
            switch (k) {
            case 2:
            case 5:
                enemy_voice(obj, 4);
                if (((of<uint32_t>(obj, 0x58) >> 27) & 1) == 0) {
                    of<uint16_t>(obj, 0xD2) = of<uint16_t>(obj, 0xD0);
                    set_motion(obj, 0x1E2);
                }
                mark = true;
                break;
            case 1:
            case 3:
            case 4:
                set_motion_blend_hold(obj, kind65_hit_motion(hc), 0, 1, 8);
                if (!pc_path()) {
                    of<uint32_t>(obj, 0x338) = 1;
                    break;
                }
                // PC: no break here: on into the 6-8 case (the motion is set again, bit 27 set)
                [[fallthrough]];
            case 6:
            case 7:
            case 8:
                if (((of<uint32_t>(obj, 0x58) >> 27) & 1) == 0)
                    of<uint16_t>(obj, 0xD2) = of<uint16_t>(obj, 0xD0);
                set_motion(obj, kind65_hit_motion(hc));
                mark = true;
                break;
            default:
                break;
            }
        }
        if (mark) {
            of<uint32_t>(obj, 0x338) = 1;
            of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | (1u << 27);
        }
    }
    if ((of<uint32_t>(obj, 0x58) >> 27) & 1) {
        if (of<int16_t>(obj, 0xF6) > 0) {
            of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xE6FFFFFFu;
            set_motion_blend(obj, uint32_t(int32_t(of<int16_t>(obj, 0xD2))), 0, 2);
        }
        return;
    }
    if (((of<uint8_t>(obj, 0x3C6) >> 5) & 1) == 0)
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xEEFFFFFFu;
}

// 0x8A830: dying. Step 0: the kill score (80), a death voice and motion by +0x334 (1: voice 2, MO 0x201 or 0x1E9 by
// rand; 2-4: voice 1, MO 0x202 or 0x1E9; else voice 1, MO 0x202); 1: at the motion's end 80 frames; 2: blinking
// (hidden on even counts, anim_time held); 3: one enemy less, not shootable, its attack position freed, body freed,
// the task ends
template <class P> void Game<P>::kind65_st3_die(uint32_t obj)
{
    int32_t sub = of<int16_t>(obj, 0x60);
    if (sub < 0 || sub > 3)
        return;
    if (sub == 0) {
        enemy_kill_score(obj, 0x50);
        const uint32_t k = of<uint32_t>(obj, 0x334);
        auto odd = [&]() {
            const int32_t r = int32_t(rand());
            return (r % 2) != 0;
        };
        uint32_t m;
        if (k == 1) {
            enemy_voice(obj, 2);
            m = odd() ? 0x1E9 : 0x201;
        } else if (k >= 2 && k <= 4) {
            enemy_voice(obj, 1);
            m = odd() ? 0x1E9 : 0x202;
        } else {
            enemy_voice(obj, 1);
            m = 0x202;
        }
        set_motion(obj, m);
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
        sub = 1;
    }
    if (sub == 1) {
        if (((of<uint8_t>(obj, 0x3C5) >> 4) & 1) == 0)
            return;
        const int16_t s = of<int16_t>(obj, 0x60);
        of<uint32_t>(obj, 0x50) = 0x50;
        of<int16_t>(obj, 0x60) = int16_t(s + 1);
        sub = 2;
    }
    if (sub == 2) {
        const int32_t t = of<int32_t>(obj, 0x50);
        of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
        uint32_t f = of<uint32_t>(obj, 0x58);
        if (t % 2 == 0)
            f |= 1u << 25;
        else
            f &= ~(1u << 25);
        of<uint32_t>(obj, 0x58) = f;
        of<int32_t>(obj, 0x50) = t - 1;
        if (t - 1 > 0)
            return;
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
    }
    mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) - 1);
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
    mem.at<uint8_t>(G::attack_slots + uint32_t(int32_t(of<int8_t>(obj, 0x2E7))) * 20) = 0;
    body_free_parts(obj);
    task_exit();
}

// the walk start of states 4 / 5 / scene 0: which side of the camera, walk flags cleared, MO 0x207 (0 / 8)
template <class P> void Game<P>::kind65_walk_start(uint32_t obj)
{
    camera_side_set(obj);
    of<uint8_t>(obj, 0x3C6) = uint8_t(of<uint8_t>(obj, 0x3C6) & ~1u);
    of<uint8_t>(obj, 0x3C5) = uint8_t(of<uint8_t>(obj, 0x3C5) & ~8u);
    set_motion_blend(obj, 0x207, 0, 8);
    of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
}

// 0x8AA00: walks toward the camera (facing it) until within 20 of it on the floor, then state 6
template <class P> void Game<P>::kind65_st4_walk(uint32_t obj)
{
    int32_t sub = of<int16_t>(obj, 0x60);
    if (sub != 0 && sub != 1)
        return;
    if (sub == 0)
        kind65_walk_start(obj);
    const float cx = mem.f32(G::camera_pos), cy = mem.f32(G::camera_pos + 4), cz = mem.f32(G::camera_pos + 8);
    of<uint32_t>(obj, 0x30) = yaw_toward_a(cx - of<float>(obj, 0x20), cy - of<float>(obj, 0x24), cz - of<float>(obj, 0x28));
    const float d = enemy_distance(of<float>(obj, 0x20), of<float>(obj, 0x28), cx, cz);
    if (d >= 20.0f)
        return;
    of<int16_t>(obj, 0x60) = 0;
    of<int16_t>(obj, 0x5C) = 6;
}

// 0x8AB60: walks to its attack position (facing it); within 1.5 on both axes it stands on it (MO 0x1E7) and
// remembers it (+0x330); then from a front position (< 4) within 41 of the camera: state 6; if its position
// changed meanwhile it walks again
template <class P> void Game<P>::kind65_st5_approach(uint32_t obj)
{
    int32_t sub = of<int16_t>(obj, 0x60);
    if (sub < 0 || sub > 2)
        return;
    if (sub == 0) {
        kind65_walk_start(obj);
        sub = 1;
    }
    if (sub == 1) {
        const uint32_t s = G::attack_slots + uint32_t(int32_t(of<int8_t>(obj, 0x2E7))) * 20;
        const float dx = mem.f32(s + 8) - of<float>(obj, 0x20);
        const float dz = mem.f32(s + 0x10) - of<float>(obj, 0x28);
        of<uint32_t>(obj, 0x30) = yaw_toward_a(dx, 0.0f, dz);
        if (std::fabs(dx) > 1.5f || std::fabs(dz) > 1.5f)
            return;
        of<uint32_t>(obj, 0x20) = mem.u32(s + 8);
        of<uint32_t>(obj, 0x28) = mem.u32(s + 0x10);
        set_motion_blend(obj, 0x1E7, 0, 8);
        const int16_t st = of<int16_t>(obj, 0x60);
        of<int32_t>(obj, 0x330) = of<int8_t>(obj, 0x2E7);
        of<int16_t>(obj, 0x60) = int16_t(st + 1);
    }
    if (of<int8_t>(obj, 0x2E7) <= 3) {
        const float d = enemy_distance(of<float>(obj, 0x20), of<float>(obj, 0x28), mem.f32(G::camera_pos),
                                       mem.f32(G::camera_pos + 8));
        if (d < 41.0f) {
            of<int16_t>(obj, 0x60) = 0;
            of<int16_t>(obj, 0x5C) = 6;
            return;
        }
    }
    if (of<int32_t>(obj, 0x330) != int32_t(of<int8_t>(obj, 0x2E7)))
        of<int16_t>(obj, 0x60) = 0;
}

// 0x8AD90: stands (MO 0x1E7); picks a target player; from a waiting position takes a front one; at a front position
// (<= 1) marks the player targeted and attacks (state 7, flags bit 31); without a target +0x2E8 = -1
template <class P> void Game<P>::kind65_st6_pick_target(uint32_t obj)
{
    int32_t sub = of<int16_t>(obj, 0x60);
    if (sub != 0 && sub != 1)
        return;
    if (sub == 0) {
        set_motion_blend(obj, 0x1E7, 0, 8);
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
    }
    pick_target_player(obj);
    if (of<uint8_t>(obj, 0x2E8) == 0xFF || of<int8_t>(obj, 0x2E7) > 1) {
        of<int8_t>(obj, 0x2E8) = -1;
        return;
    }
    attack_slot_front_take(obj);
    const int32_t slot = of<int8_t>(obj, 0x2E7);
    if (slot > 1)
        return;
    mem.at<uint8_t>(G::player_targeted + uint32_t(slot)) = 1;
    of<int16_t>(obj, 0x60) = 0;
    of<int16_t>(obj, 0x5C) = 7;
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | (1u << 31);
}

// 0x8AE20: the attack: step 0 saves its position, faces the target player's body, MO 0x1DF; 1 at frame 60 hits the
// player (player_hit(target, 1, 0)), at the end MO 0x1DC (not attacking); 2 back within 2 of the saved position:
// put there, MO 0x1E7, the player no longer targeted, 60 frames; 3 then state 6
template <class P> void Game<P>::kind65_st7_attack(uint32_t obj)
{
    int32_t sub = of<int16_t>(obj, 0x60);
    if (sub < 0 || sub > 3)
        return;
    if (sub == 0) {
        const float x = of<float>(obj, 0x20), z = of<float>(obj, 0x28);
        const int32_t t = of<int8_t>(obj, 0x2E8);
        const uint32_t y = of<uint32_t>(obj, 0x24);
        of<float>(obj, 0x3AC) = x;
        of<float>(obj, 0x3B4) = z;
        of<uint32_t>(obj, 0x3B0) = y;
        const uint32_t body = mem.u32(G::players + uint32_t(t) * 0xCC + 0xBC);
        of<uint32_t>(obj, 0x30) = yaw_toward_a(mem.f32(body + ob(0x20)) - x, 0.0f, mem.f32(body + ob(0x28)) - z);
        set_motion_blend(obj, 0x1DF, 0, 8);
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
        sub = 1;
    }
    if (sub == 1) {
        if (((of<uint8_t>(obj, 0x3C5) >> 4) & 1) == 0) {
            if (int32_t(of<int16_t>(obj, 0xF4)) == int32_t(mem.u32(G::kind65_setups + (P::pc ? 0x18 : 0x24))))
                player_hit(of<int8_t>(obj, 0x2E8), 1, 0);
            return;
        }
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~(1u << 31);
        set_motion_blend(obj, 0x1DC, 0, 8);
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
        sub = 2;
    }
    if (sub == 2) {
        const float d = enemy_distance(of<float>(obj, 0x20), of<float>(obj, 0x28), of<float>(obj, 0x3AC), of<float>(obj, 0x3B4));
        if (d > 2.0f)
            return;
        of<uint32_t>(obj, 0x28) = of<uint32_t>(obj, 0x3B4);
        of<uint32_t>(obj, 0x20) = of<uint32_t>(obj, 0x3AC);
        set_motion_blend(obj, 0x1E7, 0, 8);
        mem.at<uint8_t>(G::player_targeted + uint32_t(int32_t(of<int8_t>(obj, 0x2E7)))) = 0;
        of<uint32_t>(obj, 0x50) = 60;
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
    }
    const int32_t n = of<int32_t>(obj, 0x50) - 1;
    of<int32_t>(obj, 0x50) = n;
    if (n > 0)
        return;
    of<int16_t>(obj, 0x60) = 0;
    of<int16_t>(obj, 0x5C) = 6;
}

// the end of a scene: it becomes a standard enemy (enemy_main, AI at once, +0x330 = 1, state 1 sub-state 2, the
// given action)
template <class P> void Game<P>::kind65_to_enemy(uint32_t obj, int16_t action)
{
    of<uint32_t>(obj, 0x330) = 1;
    mem.at<Obj>(obj).routine = F::enemy_main;
    of<int16_t>(obj, 0x64) = action;
    of<int16_t>(obj, 0x5C) = 1;
    of<int16_t>(obj, 0x60) = 2;
    enemy_target_distance(obj);
}

// the chop's frame: at motion frame 30 the axe sound and the flag (or the researcher's +0x324 bit 1)
template <class P> void Game<P>::kind65_chop_sound(uint32_t obj, uint32_t flag)
{
    if (of<uint16_t>(obj, 0xF4) != 30)
        return;
    sound_cue(P::pc ? 0x4E : 0xA0124E);
    if (flag)
        mem.at<uint8_t>(flag) = 1;
    else {
        const uint32_t r = of<uint32_t>(obj, 0x35C);
        of<uint32_t>(r, 0x324) = of<uint32_t>(r, 0x324) | 2;
    }
}

// 0x8B040 (scene 0): walks to its scene point (kind65_points by action, facing it) until within 10, chops (MO 0x1DD:
// sound and flag (G::event_flags + 0x0A) at frame 30), then MO 0x1E3 and the standard enemy (action 0)
template <class P> void Game<P>::kind65_scene0_walk_chop(uint32_t obj)
{
    uint32_t sub = uint32_t(int32_t(of<int16_t>(obj, 0x60)));
    if (sub > 4)
        return;
    if (sub == 0) {
        kind65_body(obj, true);
        mem.at<Obj>(obj).routine = F::kind65_frame;
        of<int16_t>(obj, 0x5C) = 8;
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
        sub = 1;
    }
    if (sub == 1) {
        camera_side_set(obj);
        set_motion_blend(obj, 0x207, 0, 8);
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
        sub = 2;
    }
    if (sub == 2) {
        const uint32_t pt = G::kind65_points + uint32_t(int32_t(of<int16_t>(obj, 0x64))) * 12;
        of<uint32_t>(obj, 0x30) = yaw_toward_a(mem.f32(pt) - of<float>(obj, 0x20), mem.f32(pt + 4) - of<float>(obj, 0x24),
                                               mem.f32(pt + 8) - of<float>(obj, 0x28));
        const float d = enemy_distance(of<float>(obj, 0x20), of<float>(obj, 0x28), mem.f32(pt), mem.f32(pt + 8));
        if (d > 10.0f)
            return;
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
        sub = 3;
    }
    if (sub == 3) {
        set_motion_blend(obj, 0x1DD, 0, 8);
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
    }
    kind65_chop_sound(obj, G::event_flags + 0x0A);
    if (((of<uint8_t>(obj, 0x3C5) >> 4) & 1) == 0)
        return;
    set_motion_blend(obj, 0x1E3, 0, 8);
    of<uint32_t>(obj, 0x304) = 1;
    kind65_to_enemy(obj, 0);
}

// 0x8B2E0 (scene 1): axe raised (MO 0x1F1, not shootable) until flag (G::event_flags + 0x0D), then shootable, 180 frames, the
// chop (MO 0x1DD: sound and flag (G::event_flags + 0x0C) at frame 30), then it stays (step 4; +0x50 counts down once)
template <class P> void Game<P>::kind65_scene1_stance_chop(uint32_t obj)
{
    uint32_t sub = uint32_t(int32_t(of<int16_t>(obj, 0x60)));
    if (sub > 5)
        return;
    if (sub == 0) {
        kind65_body(obj, true);
        mem.at<Obj>(obj).routine = F::kind65_frame;
        camera_side_set(obj);
        set_motion_blend(obj, 0x1F1, 0, 8);
        const uint32_t f = of<uint32_t>(obj, 0x58);
        const int16_t s = of<int16_t>(obj, 0x60);
        of<int16_t>(obj, 0x5C) = 9;
        of<uint32_t>(obj, 0x58) = f & ~1u;
        of<int16_t>(obj, 0x60) = int16_t(s + 1);
        sub = 1;
    }
    if (sub == 1) {
        if (mem.at<uint8_t>((G::event_flags + 0x0D)) != 1)
            return;
        of<uint32_t>(obj, 0x50) = 180;
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | 1;
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
        sub = 2;
    }
    if (sub == 2) {
        const int32_t n = of<int32_t>(obj, 0x50) - 1;
        of<int32_t>(obj, 0x50) = n;
        if (n > 0)
            return;
        set_motion_blend(obj, 0x1DD, 0, 8);
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
        sub = 3;
    }
    if (sub == 3) {
        if (((of<uint8_t>(obj, 0x3C5) >> 4) & 1) == 0) {
            kind65_chop_sound(obj, G::event_flags + 0x0C);
            return;
        }
        const int16_t s = of<int16_t>(obj, 0x60);
        of<uint32_t>(obj, 0x50) = 120;
        of<int16_t>(obj, 0x60) = int16_t(s + 1);
        sub = 5;   // (falls into step 5's code once)
    }
    if (sub == 5)
        of<int32_t>(obj, 0x50) = of<int32_t>(obj, 0x50) - 1;
}

// scenes 2 / 3: chase the researcher (+0x35C) facing it until within `reach`, then the chop (MO 0x1DD); returns
// true when the chop starts
template <class P> bool Game<P>::kind65_chase(uint32_t obj, float reach)
{
    const uint32_t r = of<uint32_t>(obj, 0x35C);
    const float x = of<float>(obj, 0x20), z = of<float>(obj, 0x28);
    of<uint32_t>(obj, 0x30) = yaw_toward_a(of<float>(r, 0x20) - x, of<float>(r, 0x24) - of<float>(obj, 0x24),
                                           of<float>(r, 0x28) - z);
    const float d = enemy_distance(of<float>(obj, 0x20), of<float>(obj, 0x28), of<float>(r, 0x20), of<float>(r, 0x28));
    if (d > reach)
        return false;
    set_motion_blend(obj, 0x1DD, 0, 8);
    of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
    return true;
}

// 0x8B470 (scene 2): shootable once flag (G::event_flags + 0x15) is set. Pulls the axe out (MO 0x1EF), flag (G::event_flags + 0x13), MO 0x1E3;
// finds the researcher (attack_slot_nearest_object); chases it while the camera is 15 or more away (chops within 15,
// the researcher's +0x324 bit 1 at frame 30, again after the chop); near the camera or without a researcher: with
// (G::event_flags + 0x15) the standard enemy (action 1), else it waits (MO 0x1F1) up to 180 frames (flag (G::event_flags + 0x16)), then leaves
// (one enemy less, its position freed) and ends at flag (G::event_flags + 0x14)
template <class P> void Game<P>::kind65_scene2_pull_out(uint32_t obj)
{
    if (mem.at<uint8_t>((G::event_flags + 0x15)) == 1)
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | 1;
    uint32_t sub = uint32_t(int32_t(of<int16_t>(obj, 0x60)));
    if (sub > 9)
        return;
    auto next = [&]() { of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1); };
    switch (sub) {
    case 0: {
        kind65_body(obj, true);
        mem.at<Obj>(obj).routine = F::kind65_frame;
        set_motion(obj, 0x1EF);
        const uint32_t f = of<uint32_t>(obj, 0x58);
        const int16_t s = of<int16_t>(obj, 0x60);
        of<int16_t>(obj, 0x5C) = 10;
        of<uint32_t>(obj, 0x58) = f & ~1u;
        of<int16_t>(obj, 0x60) = int16_t(s + 1);
    }
        [[fallthrough]];
    case 1:
        if (((of<uint8_t>(obj, 0x3C5) >> 4) & 1) == 0)
            return;
        mem.at<uint8_t>((G::event_flags + 0x13)) = 1;
        set_motion_blend(obj, 0x1E3, 0, 8);
        next();
        [[fallthrough]];
    case 2:
        of<uint32_t>(obj, 0x35C) = attack_slot_nearest_object(obj);
        next();
        [[fallthrough]];
    case 3: {
        bool chase = false;
        if (mem.at<uint8_t>((G::event_flags + 0x16)) != 1 && of<uint32_t>(obj, 0x35C) != 0) {
            const float d = enemy_distance(of<float>(obj, 0x20), of<float>(obj, 0x28), mem.f32(G::camera_pos),
                                           mem.f32(G::camera_pos + 8));
            chase = d >= 15.0f;
        }
        if (!chase) {
            if (mem.at<uint8_t>((G::event_flags + 0x15)) == 1) {
                of<int16_t>(obj, 0x60) = 6;
            } else {
                of<int16_t>(obj, 0x60) = 7;
                mem.at<uint8_t>((G::event_flags + 0x16)) = 1;
            }
            return;
        }
        if (!kind65_chase(obj, 15.0f))
            return;
    }
        [[fallthrough]];
    case 4:
        if (((of<uint8_t>(obj, 0x3C5) >> 4) & 1) == 0) {
            kind65_chop_sound(obj, 0);
            return;
        }
        of<uint32_t>(obj, 0x50) = 0;
        next();
        [[fallthrough]];
    case 5: {
        const int32_t n = of<int32_t>(obj, 0x50) - 1;
        of<int32_t>(obj, 0x50) = n;
        if (n > 0)
            return;
        set_motion_blend(obj, 0x1E3, 0, 8);
        of<int16_t>(obj, 0x60) = 2;
        return;
    }
    case 6:
        kind65_to_enemy(obj, 1);
        return;
    case 7:
        set_motion_blend(obj, 0x1F1, 0, 8);
        of<uint32_t>(obj, 0x50) = 0;
        next();
        [[fallthrough]];
    case 8: {
        const int32_t n = of<int32_t>(obj, 0x50) + 1;
        of<int32_t>(obj, 0x50) = n;
        if (n > 180) {
            const uint32_t f = of<uint32_t>(obj, 0x58);
            mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) - 1);
            mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) - 1);
            of<uint32_t>(obj, 0x58) = f & ~1u;
            mem.at<uint8_t>(G::attack_slots + uint32_t(int32_t(of<int8_t>(obj, 0x2E7))) * 20) = 0;
            of<int16_t>(obj, 0x60) = 9;
            return;
        }
        if (mem.at<uint8_t>((G::event_flags + 0x15)) == 1) {
            set_motion_blend(obj, 0x1E3, 0, 8);
            of<int16_t>(obj, 0x60) = 3;
        }
        return;
    }
    case 9:
        if (mem.at<uint8_t>((G::event_flags + 0x14)) == 1) {
            body_free_parts(obj);
            task_exit();
        }
        return;
    }
}

// 0x8B8F0 (scene 3): runs in (MO 0x1E3, no attack position, not shootable) until x <= -133, then shootable with an
// enemy slot; finds the researcher, chases and chops it while the camera is 15 or more away (reach 10); then MO
// 0x1E3 (blend 4) until x <= -140 and the standard enemy (action 1)
template <class P> void Game<P>::kind65_scene3_dash_chop(uint32_t obj)
{
    uint32_t sub = uint32_t(int32_t(of<int16_t>(obj, 0x60)));
    if (sub > 7)
        return;
    auto next = [&]() { of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1); };
    switch (sub) {
    case 0: {
        kind65_body(obj, false);
        mem.at<Obj>(obj).routine = F::kind65_frame;
        set_motion(obj, 0x1E3);
        of<int8_t>(obj, 0x2E7) = -1;
        const uint32_t f = of<uint32_t>(obj, 0x58);
        const int16_t s = of<int16_t>(obj, 0x60);
        of<int16_t>(obj, 0x5C) = 11;
        of<uint32_t>(obj, 0x58) = f & ~1u;
        of<int16_t>(obj, 0x60) = int16_t(s + 1);
        return;
    }
    case 1:
        if (of<float>(obj, 0x20) > -133.0f)
            return;
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | 1;
        enemy_slot_take(obj);
        next();
        [[fallthrough]];
    case 2: {
        const uint32_t r = attack_slot_nearest_object(obj);
        of<uint32_t>(obj, 0x35C) = r;
        if (mem.at<uint16_t>(G::scene_count_520794) != 0 && r == 0)
            return;
        set_motion_blend(obj, 0x1E3, 0, 8);
        next();
    }
        [[fallthrough]];
    case 3: {
        bool chase = false;
        if (mem.at<uint16_t>(G::scene_count_520794) != 0 || of<uint32_t>(obj, 0x35C) != 0) {
            const float d = enemy_distance(of<float>(obj, 0x20), of<float>(obj, 0x28), mem.f32(G::camera_pos),
                                           mem.f32(G::camera_pos + 8));
            chase = d >= 15.0f;
        }
        if (!chase) {
            of<int16_t>(obj, 0x60) = 6;
            return;
        }
        if (!kind65_chase(obj, 10.0f))
            return;
    }
        [[fallthrough]];
    case 4:
        if (((of<uint8_t>(obj, 0x3C5) >> 4) & 1) == 0) {
            kind65_chop_sound(obj, 0);
            return;
        }
        of<uint32_t>(obj, 0x50) = 0;
        next();
        [[fallthrough]];
    case 5: {
        const int32_t n = of<int32_t>(obj, 0x50) - 1;
        of<int32_t>(obj, 0x50) = n;
        if (n > 0)
            return;
        of<int16_t>(obj, 0x60) = 2;
        return;
    }
    case 6:
        if ((of<uint8_t>(obj, 0x58) & 1) == 0) {
            of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | 1;
            enemy_slot_take(obj);
        }
        set_motion_blend(obj, 0x1E3, 0, 4);
        next();
        [[fallthrough]];
    case 7:
        if (of<float>(obj, 0x20) > -140.0f)
            return;
        kind65_to_enemy(obj, 1);
        return;
    }
}

// 0x8BCC0 (scene 4): shootable once flag (G::event_flags + 0x15) is set; pulls the axe out (MO 0x1EF, not shootable, state 12);
// then (anim_time held) unless flag (G::event_flags + 0x39) is clear in event 14: counted with an enemy slot in event 14, state 10
// step 1 (scene 2's routine)
template <class P> void Game<P>::kind65_scene4_pull_out(uint32_t obj)
{
    if (mem.at<uint8_t>((G::event_flags + 0x15)) == 1)
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | 1;
    const int32_t sub = of<int16_t>(obj, 0x60);
    if (sub != 0 && sub != 1)
        return;
    if (sub == 0) {
        kind65_body(obj, true);
        mem.at<Obj>(obj).routine = F::kind65_frame;
        set_motion(obj, 0x1EF);
        const uint32_t f = of<uint32_t>(obj, 0x58);
        const int16_t s = of<int16_t>(obj, 0x60);
        of<int16_t>(obj, 0x5C) = 12;
        of<uint32_t>(obj, 0x58) = f & ~1u;
        of<int16_t>(obj, 0x60) = int16_t(s + 1);
    }
    of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
    const uint8_t go = mem.at<uint8_t>((G::event_flags + 0x39));
    const uint8_t ev = mem.at<uint8_t>(G::event_index);
    if (go == 0 && ev == 14)
        return;
    if (ev == 14) {
        mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) + 1);
        mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) + 1);
        enemy_slot_take(obj);
    }
    of<int16_t>(obj, 0x5C) = 10;
    of<int16_t>(obj, 0x60) = 1;
}

} // namespace hotd::port
