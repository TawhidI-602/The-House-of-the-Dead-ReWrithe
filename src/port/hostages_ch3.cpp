// Chapter 3's hostage scenarios 12-15 (kind 100, scenario table 0xC1580; worker "ch3own", docs/chapter2_3_plan.md).
// Reached through hostage_call's default (the runtime's dispatch). Read from the arcade decompilation / disassembly;
// arcade forms only. Proven on A (ch3own_full, vf_c3a; decl/hostages_ch3.h) but where marked UNVERIFIED (teaser).
#include "port/game.h"

namespace hotd::port {

// 0x57C80 hostage_s12_cell (scenario 12, rou_1, Chapter 3 event 0): in the cell (MO 0xFD, state 4); the help voice
// by sex
template <class P> void Game<P>::hostage_s12_cell(uint32_t obj)
{
    body_init(obj, mem.at<int32_t>(G::hit_type), 0xFD);
    of<int16_t>(obj, 0x5C) = 4;
    of<uint32_t>(obj, 0) = F::hostage_s12_wait;
    sound_cue(hostage_is_female(obj) == 0 ? 0xA0133B : 0xA01328);
}

// 0x57CE0: shot / struck; the captor gone (8 frames) or the padlock broken (0x51E9D8, 16 frames): turns to the
// camera (MO 0xFF), hostage_s12_rescue; the padlock not broken in time (0x51E971): off the attack slot,
// hostage_scene_end_frame; shootable once the lock is broken
template <class P> void Game<P>::hostage_s12_wait(uint32_t obj)
{
    hostage_shot(obj);
    hostage_struck(obj);
    int frames;
    if (hostage_captor_gone(obj) != 0) {
        frames = 8;
    } else if (mem.at<uint8_t>(0x51E9D8) != 0) {
        frames = 16;
    } else {
        if (mem.at<uint8_t>(0x51E971) != 0) {
            attack_slot_free(obj);
            of<int16_t>(obj, 0x60) = 0;
            of<uint32_t>(obj, 0) = F::hostage_scene_end_frame;
            return;
        }
        enemy_frame_end(obj);
        if (mem.at<uint8_t>(0x51E9D8) != 0)
            depth_list_add(obj);
        attacker_list_add(obj);
        return;
    }
    turn_motion_set_speed(obj, 0xFF, 0, frames, 0xFFFFF000u);
    of<int16_t>(obj, 0x60) = 0;
    of<uint32_t>(obj, 0) = F::hostage_s12_rescue;
    of<uint8_t>(obj, 0x3C4) = uint8_t(of<uint8_t>(obj, 0x3C4) | 0x80);
}

// 0x57DB0: the camera within 30 (on the floor): off the attack slot, thanks by sex, counted, 400 points (to player
// 1 when 0x51E973 is set, else player 2), step 5; at the motion's end hostage_s12_leave
template <class P> void Game<P>::hostage_s12_rescue(uint32_t obj)
{
    hostage_shot(obj);
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        const float d = floor_distance(of<float>(obj, 0x20), of<float>(obj, 0x28), mem.f32(G::camera_pos),
                                       mem.f32(G::camera_pos + 8));
        if (d < 30.0f) {
            attack_slot_free(obj);
            sound_cue(hostage_is_female(obj) == 0 ? 0xA01332 : 0xA0132A);
            mem.at<uint16_t>(G::hostages_rescued) = uint16_t(mem.at<uint16_t>(G::hostages_rescued) + 1);
            mem.at<uint16_t>(G::hostages_rescued_chapter) = uint16_t(mem.at<uint16_t>(G::hostages_rescued_chapter) + 1);
            add_score(mem.at<uint8_t>(0x51E973) == 0 ? 1 : 0, 400);
            of<int16_t>(obj, 0x60) = 5;
        }
    } else if (sub == 5 && ((of<uint8_t>(obj, 0x3C5) >> 4) & 1)) {
        of<int16_t>(obj, 0x60) = 0;
        of<uint32_t>(obj, 0) = F::hostage_s12_leave;
    }
    depth_list_add(obj);
    enemy_frame_end(obj);
}

// inline (s13 / s15): the gift (hostage_gift_task at the home position); the rescuer in play: 400 points, the gift's
// +0x2EB = the rescuer
template <class P> void Game<P>::hostage_ch3_gift(uint32_t obj)
{
    const uint32_t t = task_create(F::hostage_gift_task, 0x3C8);
    of<uint32_t>(t, 0x20) = of<uint32_t>(obj, 0x2D4);
    of<uint32_t>(t, 0x24) = of<uint32_t>(obj, 0x2D8);
    of<uint32_t>(t, 0x50) = 0;
    of<uint32_t>(t, 0x28) = of<uint32_t>(obj, 0x2DC);
    const int8_t p = of<int8_t>(obj, 0x2EB);
    if (mem.at<Player>(player_rec(p)).state == 5) {   // 4P: rescuers 3 / 4 too
        const int who = coop4_on() ? int(p) : (uint8_t(p) != 0) ? 1 : 0;
        of<uint8_t>(t, 0x2EB) = uint8_t(who);
        add_score(who, 400);
    }
}

// inline (s14 / s15): counted as rescued; the rescuer in play: 400 points
template <class P> void Game<P>::hostage_ch3_count(uint32_t obj)
{
    mem.at<uint16_t>(G::hostages_rescued) = uint16_t(mem.at<uint16_t>(G::hostages_rescued) + 1);
    mem.at<uint16_t>(G::hostages_rescued_chapter) = uint16_t(mem.at<uint16_t>(G::hostages_rescued_chapter) + 1);
    const int8_t p = of<int8_t>(obj, 0x2EB);
    if (mem.at<Player>(G::players, uint32_t(int32_t(p))).state == 5)
        add_score(uint8_t(p) != 0 ? 1 : 0, 400);
}

// 0x57EF0 hostage_s13_platform_gift (scenario 13, sno_k, Chapter 3 event 1): rides the object that made it (+0x18,
// its height; MO 0x100, state 19), NoBias; the help voice by sex
template <class P> void Game<P>::hostage_s13_platform_gift(uint32_t obj)
{
    body_init(obj, mem.at<int32_t>(G::hit_type), 0x100);
    of<int16_t>(obj, 0x5C) = 19;
    of<uint32_t>(obj, 0) = F::hostage_s13_wait;
    of<int32_t>(obj, 0x50) = 0;
    of<uint8_t>(obj, 0x3C4) = uint8_t(of<uint8_t>(obj, 0x3C4) | 0x40);
    of<uint8_t>(obj, 0x3C5) = uint8_t(of<uint8_t>(obj, 0x3C5) | 8);
    sound_cue(hostage_is_female(obj) == 0 ? 0xA0133B : 0xA01328);
}

// 0x57F70: shot / struck; the captor gone: MO 0x103 (8 frames), NoBias off, hostage_s13_rescue; else at its
// platform's height, with its shadow
template <class P> void Game<P>::hostage_s13_wait(uint32_t obj)
{
    const uint32_t plat = of<uint32_t>(obj, 0x18);
    hostage_shot(obj);
    hostage_struck(obj);
    if (hostage_captor_gone(obj) != 0) {
        set_motion_blend(obj, 0x103, 0, 8);
        of<int16_t>(obj, 0x60) = 0;
        of<uint32_t>(obj, 0) = F::hostage_s13_rescue;
        of<uint8_t>(obj, 0x3C5) = uint8_t(of<uint8_t>(obj, 0x3C5) & ~8);
        return;
    }
    of<uint32_t>(obj, 0x24) = of<uint32_t>(plat, 0x24);
    shadow_setup(of<float>(obj, 0x24));
    enemy_frame_end(obj);
    depth_list_add(obj);
    attacker_list_add(obj);
}

// 0x57FF0: 0 rides down (frame 70: scene_state 5); the platform below -338.2, no enemy left, the motion ended: turns
// round (MO 0xFF), 1; 1 the camera near: holds out the gift (MO 0xEB, model 0x313), 6; 6 frame 24: thanks voice, 2;
// 2 the motion ended: the gift, MO 0xEC, 3 (lips meanwhile); 3: MO 0x116, scene_state 6, counted, 4; 4 off the attack
// slot, MO 0xFF, 5; 5 hostage_scene_end_frame. Its shadow at its height
template <class P> void Game<P>::hostage_s13_rescue(uint32_t obj)
{
    const uint32_t plat = of<uint32_t>(obj, 0x18);
    const bool done = (of<uint8_t>(obj, 0x3C5) >> 4) & 1;
    switch (uint16_t(of<int16_t>(obj, 0x60))) {
    case 0: {
        if (of<int16_t>(obj, 0xF4) == 0x46) {
            mem.at<uint8_t>(G::scene_state) = 5;
            of<uint8_t>(obj, 0x3C4) = uint8_t(of<uint8_t>(obj, 0x3C4) | 0x80);
        }
        const uint8_t mf = of<uint8_t>(obj, 0x3C5);
        if (of<float>(plat, 0x24) < ffrom(0xC3A9199Au) && mem.at<uint16_t>(G::enemies_alive) == 0 && ((mf >> 4) & 1)) {
            set_motion(obj, 0xFF);
            of<int16_t>(obj, 0x60) = 1;
            of<uint32_t>(obj, 0x30) = of<uint32_t>(obj, 0x30) ^ 0x8000;
        } else if ((mf >> 4) & 1) {
            of<int32_t>(obj, 0xF8) = mem.at<int32_t>(G::motion_frame_counts + uint32_t(int32_t(of<int16_t>(obj, 0xD0))) * 4) - 1;
        }
        of<uint32_t>(obj, 0x24) = of<uint32_t>(plat, 0x24);
        break;
    }
    case 1:
        if (!((of<uint8_t>(obj, 0x3C6) >> 1) & 1) && hostage_camera_near(obj) != 0) {
            set_motion_blend(obj, 0xEB, 0, 2);
            body_role_model_set(obj, 6, 0x313);
            of<int16_t>(obj, 0x60) = 6;
        }
        break;
    case 6:
        if (of<int16_t>(obj, 0xF4) == 24) {
            sound_cue(0xA0132A);
            of<int16_t>(obj, 0x60) = 2;
        }
        break;
    case 2:
        if (done) {
            hostage_ch3_gift(obj);
            set_motion_blend(obj, 0xEC, 0, 2);
            body_role_model_set(obj, 6, 0x4DB);
            of<int16_t>(obj, 0x60) = 3;
        } else {
            hostage_lips32(obj, 0x4AC, 0x4BE);
        }
        break;
    case 3:
        if (done) {
            body_role_model_set(obj, 1, 0x4AB);
            set_motion_blend(obj, 0x116, 0, 4);
            mem.at<uint8_t>(G::scene_state) = 6;
            mem.at<uint16_t>(G::hostages_rescued) = uint16_t(mem.at<uint16_t>(G::hostages_rescued) + 1);
            mem.at<uint16_t>(G::hostages_rescued_chapter) = uint16_t(mem.at<uint16_t>(G::hostages_rescued_chapter) + 1);
            of<int16_t>(obj, 0x60) = 4;
        } else {
            body_role_model_set(obj, 1, (of<int32_t>(obj, 0x50) % 32) / 2 == 15 ? 0x4AC : 0x4AB);
            of<int32_t>(obj, 0x50) = of<int32_t>(obj, 0x50) + 1;
        }
        break;
    case 4:
        attack_slot_free(obj);
        if ((of<uint8_t>(obj, 0x3C5) >> 4) & 1) {
            set_motion_blend(obj, 0xFF, 0, 4);
            of<int16_t>(obj, 0x60) = 5;
        }
        break;
    case 5:
        if (done) {
            of<int16_t>(obj, 0x60) = 0;
            of<uint32_t>(obj, 0) = F::hostage_scene_end_frame;
        }
        break;
    default:
        break;
    }
    shadow_setup(of<float>(obj, 0x24));
    enemy_frame_end(obj);
}

// 0x58320 hostage_s14_message_route (scenario 14, Chapter 3 events 1 / 2): MO 0xD4, state 20, NoBias; the help voice
template <class P> void Game<P>::hostage_s14_message_route(uint32_t obj)
{
    body_init(obj, mem.at<int32_t>(G::hit_type), 0xD4);
    of<int16_t>(obj, 0x5C) = 20;
    of<uint32_t>(obj, 0) = F::hostage_s14_wait;
    of<uint8_t>(obj, 0x3C5) = uint8_t(of<uint8_t>(obj, 0x3C5) | 8);
    sound_cue(hostage_is_female(obj) == 0 ? 0xA0133B : 0xA01328);
}

// 0x58380: shot / struck; the captor gone: MO 0xFF (4 frames), counts, NoBias off, scene_state 5, hostage_s14_rescue
template <class P> void Game<P>::hostage_s14_wait(uint32_t obj)
{
    hostage_shot(obj);
    hostage_struck(obj);
    if (hostage_captor_gone(obj) != 0) {
        set_motion_blend(obj, 0xFF, 0, 4);
        of<uint8_t>(obj, 0x3C4) = uint8_t(of<uint8_t>(obj, 0x3C4) | 0x80);
        of<uint8_t>(obj, 0x3C5) = uint8_t(of<uint8_t>(obj, 0x3C5) & ~8);
        mem.at<uint8_t>(G::scene_state) = 5;
        of<int16_t>(obj, 0x60) = 0;
        of<uint32_t>(obj, 0) = F::hostage_s14_rescue;
        return;
    }
    enemy_frame_end(obj);
    depth_list_add(obj);
    attacker_list_add(obj);
}

// 0x58410: 0 the camera near: the voice message (voice_message_task, +0x70 36), MO 0xEB, model 0x71C, route_choice 1
// (the message names the other way), 2; 2 the motion ended: MO 0xEC, model 0x4DB, 3 (lips); 3: MO 0x116, counted,
// 400 points, scene_state 6, 4; 4 off the attack slot, MO 0xFF, 5; 5 hostage_scene_end_frame
// Proven on A (vf_c3a: 106 captures, 0 differ)
template <class P> void Game<P>::hostage_s14_rescue(uint32_t obj)
{
    const bool done = (of<uint8_t>(obj, 0x3C5) >> 4) & 1;
    switch (uint16_t(of<int16_t>(obj, 0x60))) {
    case 0:
        if (!((of<uint8_t>(obj, 0x3C6) >> 1) & 1) && hostage_camera_near(obj) != 0) {
            const uint32_t t = task_create(F::voice_message_task, 0x3C8);
            task_clear_work(t);
            of<int16_t>(t, 0x70) = 36;
            set_motion_blend(obj, 0xEB, 0, 2);
            body_role_model_set(obj, 6, 0x71C);
            mem.at<uint8_t>(G::route_choice) = 1;
            of<int16_t>(obj, 0x60) = 2;
        }
        attacker_list_add(obj);
        break;
    case 2:
        if (done) {
            set_motion_blend(obj, 0xEC, 0, 2);
            body_role_model_set(obj, 6, 0x4DB);
            of<int16_t>(obj, 0x60) = 3;
        } else {
            hostage_lips32(obj, 0x33E, 0x34F);
        }
        attacker_list_add(obj);
        break;
    case 3:
        if (done) {
            set_motion_blend(obj, 0x116, 0, 4);
            hostage_ch3_count(obj);
            mem.at<uint8_t>(G::scene_state) = 6;
            of<int16_t>(obj, 0x60) = 4;
        } else {
            hostage_lips32(obj, 0x33E, 0x34F);
        }
        attacker_list_add(obj);
        break;
    case 4:
        attack_slot_free(obj);
        if ((of<uint8_t>(obj, 0x3C5) >> 4) & 1) {
            set_motion_blend(obj, 0xFF, 0, 4);
            of<int16_t>(obj, 0x60) = 5;
        }
        break;
    case 5:
        if (done) {
            of<int16_t>(obj, 0x60) = 0;
            of<uint32_t>(obj, 0) = F::hostage_scene_end_frame;
        }
        break;
    default:
        break;
    }
    enemy_frame_end(obj);
}

// 0x58650 hostage_s15_gift (scenario 15, Chapter 3 event 2): MO 0xD4, state 4; the help voice
// Proven on A (vf_c3a: 1 captures, 0 differ)
template <class P> void Game<P>::hostage_s15_gift(uint32_t obj)
{
    body_init(obj, mem.at<int32_t>(G::hit_type), 0xD4);
    of<int16_t>(obj, 0x5C) = 4;
    of<uint32_t>(obj, 0) = F::hostage_s15_wait;
    sound_cue(hostage_is_female(obj) == 0 ? 0xA0133B : 0xA01328);
}

// 0x586B0: shot / struck; the captor gone: gets up toward the camera's side (MO 0xE6 / 0xE5), hostage_s15_rescue
// Proven on A (vf_c3a: 21 captures, 0 differ)
template <class P> void Game<P>::hostage_s15_wait(uint32_t obj)
{
    hostage_shot(obj);
    hostage_struck(obj);
    if (hostage_captor_gone(obj) != 0) {
        set_motion_blend(obj, int32_t(angle_to_camera(obj)) > 0x7FFF ? 0xE6 : 0xE5, 0, 2);
        of<int16_t>(obj, 0x60) = 0;
        of<uint32_t>(obj, 0) = F::hostage_s15_rescue;
        return;
    }
    enemy_frame_end(obj);
    depth_list_add(obj);
    attacker_list_add(obj);
}

// 0x58730: 0 up (frame 40 / 20 by side): MO 0xFF turning, scene_state 5, 1; 1 turns to the camera; near: holds out
// the gift (MO 0xEB, model 0x313), 6; 6 frame 24: thanks voice, 2; 2 the gift (400 points), MO 0xEC, 3; 3: MO 0x116,
// counted, 400 points, scene_state 6, 4; 4 off the attack slot, MO 0xFF, 5; 5 hostage_scene_end_frame
// UNVERIFIED (teaser)
template <class P> void Game<P>::hostage_s15_rescue(uint32_t obj)
{
    const bool done = (of<uint8_t>(obj, 0x3C5) >> 4) & 1;
    switch (uint16_t(of<int16_t>(obj, 0x60))) {
    case 0:
        if (int32_t(angle_to_camera(obj)) > 0x7FFF) {
            if (of<int16_t>(obj, 0xF4) <= 39)
                break;
        } else if (of<int16_t>(obj, 0xF4) <= 19) {
            break;
        }
        of<uint8_t>(obj, 0x3C4) = uint8_t(of<uint8_t>(obj, 0x3C4) | 0x80);
        turn_motion_set(obj, 0xFF, 0, 2);
        mem.at<uint8_t>(G::scene_state) = 5;
        of<int16_t>(obj, 0x60) = 1;
        break;
    case 1:
        turn_toward_camera(obj, 0x400);
        if (!((of<uint8_t>(obj, 0x3C6) >> 1) & 1) && hostage_camera_near(obj) != 0) {
            set_motion_blend(obj, 0xEB, 0, 2);
            body_role_model_set(obj, 6, 0x313);
            of<int16_t>(obj, 0x60) = 6;
        }
        break;
    case 6:
        if (of<int16_t>(obj, 0xF4) == 24) {
            sound_cue(0xA0132A);
            of<int16_t>(obj, 0x60) = 2;
        }
        break;
    case 2:
        if (done) {
            hostage_ch3_gift(obj);
            set_motion_blend(obj, 0xEC, 0, 2);
            body_role_model_set(obj, 6, 0x4DB);
            of<int16_t>(obj, 0x60) = 3;
        } else {
            hostage_lips32(obj, 0x4AC, 0x4BE);
        }
        break;
    case 3:
        if (done) {
            body_role_model_set(obj, 1, 0x4AB);
            set_motion_blend(obj, 0x116, 0, 4);
            hostage_ch3_count(obj);
            mem.at<uint8_t>(G::scene_state) = 6;
            of<int16_t>(obj, 0x60) = 4;
        } else {
            body_role_model_set(obj, 1, (of<int32_t>(obj, 0x50) % 32) / 2 == 15 ? 0x4AC : 0x4AB);
            of<int32_t>(obj, 0x50) = of<int32_t>(obj, 0x50) + 1;
        }
        break;
    case 4:
        attack_slot_free(obj);
        if ((of<uint8_t>(obj, 0x3C5) >> 4) & 1) {
            set_motion_blend(obj, 0xFF, 0, 4);
            of<int16_t>(obj, 0x60) = 5;
        }
        break;
    case 5:
        if (done) {
            of<int16_t>(obj, 0x60) = 0;
            of<uint32_t>(obj, 0) = F::hostage_scene_end_frame;
        }
        break;
    default:
        break;
    }
    enemy_frame_end(obj);
}

} // namespace hotd::port
