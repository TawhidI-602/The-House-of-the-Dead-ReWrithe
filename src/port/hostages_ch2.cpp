// Chapter 2's hostage scenarios 7-11 (kind 100, scenario table 0xC1580; worker "ch2", docs/chapter2_3_plan.md).
// Reached through hostage_call's default (the runtime's dispatch). Read from the arcade decompilation / disassembly;
// arcade forms only. Proven on A (ch2proof, docs/teaser_status/ch2proof.md).
#include "port/game.h"

namespace hotd::port {

// 0x56FE0 hostage_s07_crawl (scenario 7, hau, Chapter 2 event 2): crawling (MO 0xE4), state 0
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::hostage_s07_crawl(uint32_t obj)
{
    body_init(obj, mem.at<int32_t>(G::hit_type), 0xE4);
    of<int16_t>(obj, 0x5C) = 0;
    of<uint32_t>(obj, 0) = F::hostage_s07_wait;
}

// 0x57010: shot / struck; freed: gets up and runs (MO 0xFF, thanks voice 0xA01332), hostage_s02_thank
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::hostage_s07_wait(uint32_t obj)
{
    hostage_shot(obj);
    hostage_struck(obj);
    if (hostage_captor_gone(obj) != 0) {
        set_motion_blend(obj, 0xFF, 0, 2);
        of<uint8_t>(obj, 0x3C4) = uint8_t(of<uint8_t>(obj, 0x3C4) | 0x80);
        sound_cue(0xA01332);
        of<uint32_t>(obj, 0) = F::hostage_s02_thank;
        return;
    }
    enemy_frame_end(obj);
    depth_list_add(obj);
    attacker_list_add(obj);
}

// 0x57090 hostage_s08_curled (scenario 8, maru, Chapter 2 event 4): curled up (MO 0xF5, state 2), facing the camera,
// counts as an enemy
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::hostage_s08_curled(uint32_t obj)
{
    body_init(obj, mem.at<int32_t>(G::hit_type), 0xF5);
    of<int16_t>(obj, 0x5C) = 2;
    of<int32_t>(obj, 0x50) = 0;
    of<uint32_t>(obj, 0) = F::hostage_s08_wait;
    of<int32_t>(obj, 0x2EC) = mem.at<int8_t>(G::scene_number);
    of<uint8_t>(obj, 0x3C4) = uint8_t(of<uint8_t>(obj, 0x3C4) | 0x80);
    mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) + 1);
    mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) + 1);
}

// 0x57110: shot by a player; no player reached him (0x51E950 / 0x51E951) by camera frame 221: uncounted, off the
// attack slot, hostage_scene_end_frame; reached: stands up (marutati 0xF6), hostage_s08_rescue
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::hostage_s08_wait(uint32_t obj)
{
    researcher_shot_by_player(obj);
    if (mem.at<uint8_t>(0x51E950) == 0 && mem.at<uint8_t>(0x51E951) == 0) {
        if (mem.f32(G::camera_frame) >= 221.0f) {
            mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) - 1);
            mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) - 1);
            attack_slot_free(obj);
            of<uint32_t>(obj, 0) = F::hostage_scene_end_frame;
            return;
        }
        turn_toward_camera(obj, 0xC0);
        enemy_frame_end(obj);
        depth_list_add(obj);
        attacker_list_add(obj);
        return;
    }
    set_motion_blend(obj, 0xF6, 0, 2);
    of<int16_t>(obj, 0x60) = 0;
    of<uint32_t>(obj, 0) = F::hostage_s08_rescue;
    of<uint8_t>(obj, 0x3C4) = uint8_t(of<uint8_t>(obj, 0x3C4) | 0x80);
}

// 0x571F0: 0 standing up (shootable after frame 34), runs (MO 0xFF), scene_state 5; 1 near the camera: MO 0xEB, the
// gift 0x313 in hand; 2 frame 24 his voice; 3 the gift task (hostage_gift_task at home, to the player who reached him,
// 400 points), MO 0xEC, head 0x4DB (lips meanwhile); 4 counted, MO 0x115 (mouth closed meanwhile); 5 scene_state 6,
// uncounted, off the slot, MO 0xFF; 6 at the next scene hostage_scene_end_frame
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::hostage_s08_rescue(uint32_t obj)
{
    switch (of<int16_t>(obj, 0x60)) {
    case 0:
        if (of<int16_t>(obj, 0xF4) > 0x22) {
            researcher_shot_by_player(obj);
            depth_list_add(obj);
        }
        if (hostage_motion_done(obj)) {
            set_motion_blend(obj, 0xFF, 0, 2);
            mem.at<uint8_t>(G::scene_state) = 5;
            of<int16_t>(obj, 0x60) = 1;
        }
        break;
    case 1:
        turn_toward_camera(obj, 0x800);
        if (hostage_camera_near(obj) != 0) {
            set_motion_blend(obj, 0xEB, 0, 2);
            body_role_model_set(obj, 6, 0x313);
            of<int16_t>(obj, 0x60) = 2;
        }
        break;
    case 2:
        if (of<int16_t>(obj, 0xF4) == 0x18) {
            sound_cue(0xA0132A);
            of<int16_t>(obj, 0x60) = 3;
        }
        break;
    case 3:
        if (hostage_motion_done(obj)) {
            const uint32_t t = task_create(F::hostage_gift_task, 0x3C8);
            of<uint32_t>(t, 0x20) = of<uint32_t>(obj, 0x2D4);
            of<uint32_t>(t, 0x24) = of<uint32_t>(obj, 0x2D8);
            of<uint32_t>(t, 0x50) = 0;
            of<uint32_t>(t, 0x28) = of<uint32_t>(obj, 0x2DC);
            if (mem.at<Player>(G::players, 0).state == 5 && mem.at<uint8_t>(0x51E950) != 0) {
                of<uint8_t>(t, 0x2EB) = 0;
                add_score(0, 400);
            } else if (mem.at<Player>(G::players, 1).state == 5 && mem.at<uint8_t>(0x51E951) != 0) {
                of<uint8_t>(t, 0x2EB) = 1;
                add_score(1, 400);
            }
            set_motion(obj, 0xEC);
            body_role_model_set(obj, 6, 0x4DB);
            of<int16_t>(obj, 0x60) = 4;
        } else {
            hostage_lips32(obj, 0x4AC, 0x4BE);
        }
        break;
    case 4:
        if (hostage_motion_done(obj)) {
            of<uint8_t>(obj, 0x3C4) = uint8_t(of<uint8_t>(obj, 0x3C4) & 0x7F);
            body_role_model_set(obj, 1, 0x4AB);
            set_motion_blend(obj, 0x115, 0, 4);
            hostage_count_rescue();
            of<int16_t>(obj, 0x60) = 5;
        } else {
            const int32_t v = (of<int32_t>(obj, 0x50) % 32) / 2;
            body_role_model_set(obj, 1, v == 15 ? 0x4AC : 0x4AB);
            of<int32_t>(obj, 0x50) = of<int32_t>(obj, 0x50) + 1;
        }
        break;
    case 5:
        if (hostage_motion_done(obj)) {
            mem.at<uint8_t>(G::scene_state) = 6;
            mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) - 1);
            mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) - 1);
            attack_slot_free(obj);
            set_motion_blend(obj, 0xFF, 0, 4);
            of<int16_t>(obj, 0x60) = 6;
        }
        break;
    case 6:
        if (of<int32_t>(obj, 0x2EC) < int32_t(mem.at<int8_t>(G::scene_number))) {
            of<int16_t>(obj, 0x60) = 0;
            of<uint32_t>(obj, 0) = F::hostage_scene_end_frame;
        }
        break;
    default: break;
    }
    enemy_frame_end(obj);
}

// 0x57510 hostage_s09_flee_thank (scenario 9, tenonige, Chapter 2 event 5): MO 0x10F, state 5
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::hostage_s09_flee_thank(uint32_t obj)
{
    body_init(obj, mem.at<int32_t>(G::hit_type), 0x10F);
    of<int16_t>(obj, 0x5C) = 5;
    of<uint32_t>(obj, 0) = F::hostage_s09_wait;
}

// 0x57540: shot / struck; freed: MO 0xFF, hostage_s09_thank
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::hostage_s09_wait(uint32_t obj)
{
    hostage_shot(obj);
    hostage_struck(obj);
    if (hostage_captor_gone(obj) != 0) {
        set_motion_blend(obj, 0xFF, 0, 2);
        of<uint32_t>(obj, 0) = F::hostage_s09_thank;
        of<uint8_t>(obj, 0x3C4) = uint8_t(of<uint8_t>(obj, 0x3C4) | 0x80);
        return;
    }
    enemy_frame_end(obj);
    depth_list_add(obj);
    attacker_list_add(obj);
}

// 0x575B0: 0 counted, 400 points to the rescuer, thank_l (MO 0x110); 1 frame 30 the voice, at the end off the slot,
// MO 0xFF; 2 hostage_end
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::hostage_s09_thank(uint32_t obj)
{
    const int16_t s = of<int16_t>(obj, 0x60);
    if (s == 1) {
        if (of<int16_t>(obj, 0xF4) == 0x1E)
            sound_cue(0xA01332);
        if (hostage_motion_done(obj)) {
            attack_slot_free(obj);
            set_motion_blend(obj, 0xFF, 0, 2);
            of<int16_t>(obj, 0x60) = 2;
        }
    } else if (s == 0) {
        hostage_count_rescue();
        hostage_rescuer_score(obj, 400);
        set_motion_blend(obj, 0x110, 0, 4);
        of<int16_t>(obj, 0x60) = 1;
    } else if (s == 2) {
        hostage_end(obj);
    }
    enemy_frame_end(obj);
}

// 0x576B0 hostage_s10_walk_off (scenario 10, atozu, Chapter 2 event 9): MO 0xD4, state 4, a call for help (0xA0133B
// male / 0xA01328 female)
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::hostage_s10_walk_off(uint32_t obj)
{
    body_init(obj, mem.at<int32_t>(G::hit_type), 0xD4);
    of<int16_t>(obj, 0x5C) = 4;
    of<uint32_t>(obj, 0) = F::hostage_s10_wait;
    if (hostage_is_female(obj) == 0)
        sound_cue(0xA0133B);
    else
        sound_cue(0xA01328);
}

// 0x57710: shot / struck; freed: MO 0xFF, hostage_s10_rescue
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::hostage_s10_wait(uint32_t obj)
{
    hostage_shot(obj);
    hostage_struck(obj);
    if (hostage_captor_gone(obj) != 0) {
        set_motion_blend(obj, 0xFF, 0, 2);
        of<int16_t>(obj, 0x60) = 0;
        of<uint32_t>(obj, 0) = F::hostage_s10_rescue;
        of<uint8_t>(obj, 0x3C4) = uint8_t(of<uint8_t>(obj, 0x3C4) | 0x80);
        return;
    }
    enemy_frame_end(obj);
    depth_list_add(obj);
    attacker_list_add(obj);
}

// 0x57780: 0 turns to the camera; within 30 of it: thanks (0xA01332 male / 0xA0132A female), off the slot, MO 0x112,
// counted, 400 points; 4 at the end turns away (MO 0xFF); 5 walks to (1260, 190), then hostage_scene_end_frame
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::hostage_s10_rescue(uint32_t obj)
{
    const int16_t s = of<int16_t>(obj, 0x60);
    if (s == 0) {
        turn_toward_camera(obj, 0x800);
        const float d = floor_distance(of<float>(obj, 0x20), of<float>(obj, 0x28), mem.f32(G::camera_pos),
                                       mem.f32(G::camera_pos + 8));
        if (d < 30.0f) {
            sound_cue(hostage_is_female(obj) == 0 ? 0xA01332 : 0xA0132A);
            attack_slot_free(obj);
            set_motion_blend(obj, 0x112, 0, 4);
            hostage_count_rescue();
            hostage_rescuer_score(obj, 400);
            of<int16_t>(obj, 0x60) = 4;
        }
    } else if (s == 4) {
        if (hostage_motion_done(obj)) {
            turn_motion_set(obj, 0xFF, 0, 8);
            of<int16_t>(obj, 0x60) = 5;
        }
    } else if (s == 5) {
        if (walk_to(obj, 1260.0f, 190.0f, 0x200, 1.0f) != 0) {
            of<int16_t>(obj, 0x60) = 0;
            of<uint32_t>(obj, 0) = F::hostage_scene_end_frame;
        }
    }
    enemy_frame_end(obj);
}

// 0x57930 hostage_s11_reluctant_gift (scenario 11, iyaiya, Chapter 2 event 12): MO 0xF3, state 19, motion flag 3, a
// call for help (0xA0133B male / 0xA01328 female), the shadow at y -111
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::hostage_s11_reluctant_gift(uint32_t obj)
{
    body_init(obj, mem.at<int32_t>(G::hit_type), 0xF3);
    of<int16_t>(obj, 0x5C) = 19;
    of<uint32_t>(obj, 0) = F::hostage_s11_wait;
    of<uint8_t>(obj, 0x3C5) = uint8_t(of<uint8_t>(obj, 0x3C5) | 8);
    if (hostage_is_female(obj) == 0)
        sound_cue(0xA0133B);
    else
        sound_cue(0xA01328);
    shadow_setup(ffrom(0xC2DE0000u));
}

// 0x579A0: shot / struck; flags2 bit 1 cleared; freed: holds out the gift (MO 0xF1 over 60 frames, 0x313 in hand),
// hostage_s11_rescue, off the attack slot
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::hostage_s11_wait(uint32_t obj)
{
    hostage_shot(obj);
    hostage_struck(obj);
    of<uint16_t>(obj, 0x6C) = uint16_t(of<uint16_t>(obj, 0x6C) & ~2u);
    if (hostage_captor_gone(obj) != 0) {
        set_motion_blend(obj, 0xF1, 0, 60);
        body_role_model_set(obj, 6, 0x313);
        of<uint32_t>(obj, 0) = F::hostage_s11_rescue;
        of<int16_t>(obj, 0x60) = 0;
        attack_slot_free(obj);
        return;
    }
    enemy_frame_end(obj);
    depth_list_add(obj);
}

// 0x57A20: 0 (lips meanwhile) no enemy left and camera frame 40: counted as an enemy, facing the camera, scene_state
// 5; 1 (lips) at the motion's end the gift task to the rescuer (400 points), MO 0xF2, head 0x4DB, sound 0xA01419;
// 4 at its end scene_state 6, uncounted, counted as rescued, hostage_scene_end_frame (drawn once more)
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::hostage_s11_rescue(uint32_t obj)
{
    const int16_t s = of<int16_t>(obj, 0x60);
    if (s == 0) {
        if (mem.at<uint16_t>(G::enemies_alive) == 0 && !(mem.f32(G::camera_frame) < 40.0f)) {
            mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) + 1);
            mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) + 1);
            of<uint8_t>(obj, 0x3C4) = uint8_t(of<uint8_t>(obj, 0x3C4) | 0x80);
            mem.at<uint8_t>(G::scene_state) = 5;
            of<int16_t>(obj, 0x60) = 1;
        } else {
            hostage_lips32(obj, 0x4A9, 0x4AF);
        }
    } else if (s == 1) {
        if (hostage_motion_done(obj)) {
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
            set_motion_blend(obj, 0xF2, 0, 16);
            body_role_model_set(obj, 6, 0x4DB);
            sound_cue(0xA01419);
            of<int16_t>(obj, 0x60) = 4;
        } else {
            hostage_lips32(obj, 0x4A9, 0x4AF);
        }
    } else if (s == 4) {
        if (hostage_motion_done(obj)) {
            mem.at<uint8_t>(G::scene_state) = 6;
            mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) - 1);
            mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) - 1);
            of<uint32_t>(obj, 0) = F::hostage_scene_end_frame;
            of<int16_t>(obj, 0x60) = 0;
            of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
            hostage_count_rescue();
            object_draw(obj);
            return;
        }
    }
    enemy_frame_end(obj);
}

} // namespace hotd::port
