// Enemies: the enemy kinds (the object kinds of the spawn records that make creatures: kind table obj_kinds), A and PC
// (PA / PM). Drafted with tools/ghidra_to_cpp.py from the typed Ghidra C, fixed against the disassembly, proven with
// captures (docs/proof.md). Per-kind work words are reached by their arcade offsets (ob()): +0x2EC a counter,
// +0x320 / +0x324 values from the spawn record, +0x330 the walk variant.
#include "port/game.h"

namespace hotd::port {

// the creature counters (u16) of a new / leaving enemy
template <class P> void Game<P>::enemy_counts_add(int d)
{
    mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) + d);
    mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) + d);
}

// flags bit 22 (from the spawn record): cleared, and +0x304 (start the AI at once) set
template <class P> void Game<P>::enemy_take_ai_at_once(uint32_t obj)
{
    uint32_t f = mem.at<Enemy>(obj).flags;
    if ((f >> 22) & 1) {
        f &= 0xFFBFFFFFu;
        mem.set32(obj + ob(0x304), 1);
    }
    mem.at<Enemy>(obj).flags = f;
}

// the frame start of a fighting kind: hit type, hit class / damage cleared, part hits
template <class P> void Game<P>::kind_hits(uint32_t obj)
{
    mem.set32(G::hit_type, uint32_t(int32_t(mem.at<Enemy>(obj).type)));
    mem.at<uint8_t>(G::hit_class) = 0;
    mem.at<int16_t>(G::hit_damage) = 0;
    enemy_part_hits(obj);
}

// shadow on the floor, draw, anim_time + 1
template <class P> void Game<P>::kind_draw(uint32_t obj)
{
    shadow_setup(mem.f32(G::floor_y));
    object_draw(obj);
    Enemy& e = mem.at<Enemy>(obj);
    e.anim_time = e.anim_time + 1;
}

// the motion of enemy_motion_lists[type][action][sub]
template <class P> uint32_t Game<P>::kind_list_motion(uint32_t obj, int32_t sub)
{
    const Enemy& e = mem.at<Enemy>(obj);
    const uint32_t row = mem.u32(G::enemy_motion_lists + uint32_t(int32_t(e.type)) * 4);
    const uint32_t list = mem.u32(row + uint32_t(int32_t(e.action)) * 4);
    return mem.u32(list + uint32_t(sub) * 8);
}

// ---------------------------------------------------------------- kind 1: a zombie that wakes (its record's motion)

// 0x92D30 (kind 1): body (motion 0x21C), anim_time 25, AI-at-once flag, type set-up, state 0, counted; waits
template <class P> void Game<P>::kind1_zombie_wake(uint32_t obj)
{
    mem.set32(G::hit_type, uint32_t(int32_t(mem.at<Enemy>(obj).type)));
    body_init(obj, int32_t(mem.u32(G::hit_type)), 0x21C);
    mem.at<Enemy>(obj).anim_time = 0x19;
    enemy_take_ai_at_once(obj);
    creature_type_setup(obj);
    mem.at<Enemy>(obj).state = 0;
    enemy_counts_add(1);
    mem.at<Enemy>(obj).routine = F::kind1_zombie_wake_wait;
    kind1_zombie_wake_wait(obj);
}

// 0x92DC0: drawn; at motion frame 30 its event flag (event_flags + record +0x324) is set and it fights
template <class P> void Game<P>::kind1_zombie_wake_wait(uint32_t obj)
{
    kind_draw(obj);
    Enemy& e = mem.at<Enemy>(obj);
    if (e.frame == 0x1E) {
        mem.at<uint8_t>(G::event_flags + mem.u32(obj + ob(0x324))) = 1;
        e.routine = F::kind1_zombie_fight;
    }
}

// 0x92E10: hits, death check, drawn; while shootable at the end of a motion loop: a waiting attack position, the
// standard enemy walking (state 1, sub-state 2, walk variant 1, delay = record +0x320 x 5, blend 5); depth list
template <class P> void Game<P>::kind1_zombie_fight(uint32_t obj)
{
    kind_hits(obj);
    enemy_check_death(obj);
    kind_draw(obj);
    Enemy& e = mem.at<Enemy>(obj);
    if ((e.flags & 1) == 0)
        return;
    if ((e.motion_flags >> 4) & 1) {
        attack_slot_take_waiting(obj);
        const int32_t d = int32_t(mem.u32(obj + ob(0x320)));
        Enemy& f = mem.at<Enemy>(obj);
        f.state = 1;
        f.substate = 2;
        mem.set32(obj + ob(0x330), 1);
        mem.set32(obj + ob(0x2EC), uint32_t(d * 5));
        set_motion_blend(obj, kind_list_motion(obj, 2), 0, 5);
        mem.at<Enemy>(obj).routine = F::enemy_main;
        enemy_target_distance(obj);
    }
    depth_list_add(obj);
}

// ---------------------------------------------------------------- kind 2: a zombie that kicks (chapter 1 event 5)

// 0x92EE0 (kind 2): body (0x151), type set-up, state / walk variant / sub-state 0, counted, anim_time 32; waits
template <class P> void Game<P>::kind2_zombie_kick(uint32_t obj)
{
    mem.set32(G::hit_type, uint32_t(int32_t(mem.at<Enemy>(obj).type)));
    body_init(obj, int32_t(mem.u32(G::hit_type)), 0x151);
    creature_type_setup(obj);
    Enemy& e = mem.at<Enemy>(obj);
    e.state = 0;
    mem.set32(obj + ob(0x330), 0);
    enemy_counts_add(1);
    mem.at<Enemy>(obj).routine = F::kind2_zombie_kick_wait;
    mem.at<Enemy>(obj).substate = 0;
    mem.at<Enemy>(obj).anim_time = 0x20;
}

// 0x92F50: drawn; sub-state 0: anim_time counts to 48 (timer 0, sound 0xA01102), 1: timer to 51, 2: anim_time back
// down to 32, then MO 0x2A1 (the kick) and fighting
template <class P> void Game<P>::kind2_zombie_kick_wait(uint32_t obj)
{
    shadow_setup(mem.f32(G::floor_y));
    object_draw(obj);
    Enemy& e = mem.at<Enemy>(obj);
    const int16_t s = e.substate;
    if (s == 1) {
        const int32_t t = e.timer + 1;
        e.timer = t;
        if (t == 0x33)
            e.substate = int16_t(e.substate + 1);
        return;
    }
    if (s == 0) {
        const int32_t a = e.anim_time + 1;
        e.anim_time = a;
        if (a == 0x30) {
            e.timer = 0;
            e.substate = int16_t(e.substate + 1);
            sound_cue(P::pc ? 0x18 : 0xA01102);
        }
        return;
    }
    if (s != 2)
        return;
    const int32_t a = e.anim_time - 1;
    e.anim_time = a;
    if (a == 0x20) {
        e.substate = int16_t(e.substate + 1);
        set_motion(obj, 0x2A1);
        mem.at<Enemy>(obj).routine = F::kind2_zombie_fight;
    }
}

// 0x93030: as kind1_zombie_fight, with the sub-state from record +0x324 (2: walk variant 1)
template <class P> void Game<P>::kind2_zombie_fight(uint32_t obj)
{
    kind_hits(obj);
    enemy_check_death(obj);
    kind_draw(obj);
    Enemy& e = mem.at<Enemy>(obj);
    if ((e.flags & 1) == 0)
        return;
    if ((e.motion_flags >> 4) & 1) {
        attack_slot_take_waiting(obj);
        const int16_t sub = int16_t(mem.u32(obj + ob(0x324)));
        Enemy& f = mem.at<Enemy>(obj);
        f.state = 1;
        f.substate = sub;
        if (sub == 2)
            mem.set32(obj + ob(0x330), 1);
        set_motion_blend(obj, kind_list_motion(obj, mem.at<Enemy>(obj).substate), 0, 5);
        mem.at<Enemy>(obj).routine = F::enemy_main;
        enemy_target_distance(obj);
    }
    depth_list_add(obj);
}

// ---------------------------------------------------------------- kind 6: a zombie that stabs (chapter 2 event 10)

// 0x93930 (kind 6): body (0x204), type set-up, counted, a 30-frame wait (+0x2EC); stabs
template <class P> void Game<P>::kind6_zombie_stab(uint32_t obj)
{
    mem.set32(G::hit_type, uint32_t(int32_t(mem.at<Enemy>(obj).type)));
    body_init(obj, int32_t(mem.u32(G::hit_type)), 0x204);
    creature_type_setup(obj);
    mem.at<Enemy>(obj).state = 0;
    enemy_counts_add(1);
    mem.set32(obj + ob(0x2EC), 0x1E);
    mem.at<Enemy>(obj).routine = F::kind6_zombie_stab_frame;
    mem.at<Enemy>(obj).substate = 0;
    kind6_zombie_stab_frame(obj);
}

// 0x939A0: hits, kind6_death_check, drawn; while shootable the attack and the depth list
template <class P> void Game<P>::kind6_zombie_stab_frame(uint32_t obj)
{
    kind_hits(obj);
    kind6_death_check(obj);
    kind_draw(obj);
    if ((mem.at<Enemy>(obj).flags & 1) == 0)
        return;
    kind6_attack(obj);
    depth_list_add(obj);
}

// 0x93A10: sub-state 0 waits +0x2EC frames, then MO 0x205 (the stab, sound 0xA01313); 1 at motion frame 21 hits the
// player (the solo player, else player 1); 2 at the end of the loop MO 0x204; 3 one in 32 frames stabs again (no shake,
// players in the game)
template <class P> void Game<P>::kind6_attack(uint32_t obj)
{
    Enemy& e = mem.at<Enemy>(obj);
    const int16_t s = e.substate;
    auto stab = [&]() {
        set_motion_blend(obj, 0x205, 0, 5);
        sound_cue(P::pc ? 0x60 : 0xA01313);
    };
    auto hitFrame = [&]() {
        if (mem.at<Enemy>(obj).frame < 0x15)
            return false;
        int32_t p = 0;
        if (mem.at<uint16_t>(G::players_in_game) == 1)
            p = mem.at<int8_t>(G::solo_player_is_p2);
        player_hit(p, 1, 0);
        mem.at<Enemy>(obj).substate = int16_t(mem.at<Enemy>(obj).substate + 1);
        return true;
    };
    auto again = [&]() {
        const int32_t r = int32_t(rand());
        if ((r >> 4) % 0x20 == 1 && mem.u32(G::shake_timer) == 0 && mem.at<uint16_t>(G::players_in_game) != 0) {
            mem.at<Enemy>(obj).substate = 1;
            stab();
        }
    };
    auto loopEnd = [&]() {
        if (((mem.at<Enemy>(obj).motion_flags >> 4) & 1) == 0)
            return;
        set_motion_blend(obj, 0x204, 0, 5);
        mem.at<Enemy>(obj).substate = int16_t(mem.at<Enemy>(obj).substate + 1);
        again();
    };
    switch (s) {
    case 0:
        if (mem.u32(obj + ob(0x2EC)) != 0) {
            mem.set32(obj + ob(0x2EC), mem.u32(obj + ob(0x2EC)) - 1);
            return;
        }
        stab();
        mem.at<Enemy>(obj).substate = int16_t(mem.at<Enemy>(obj).substate + 1);
        if (hitFrame())
            loopEnd();
        return;
    case 1:
        if (hitFrame())
            loopEnd();
        return;
    case 2:
        loopEnd();
        return;
    case 3:
        again();
        return;
    default:
        return;
    }
}

// 0x93B40: dead with flags bit 8 (or the debug hook): 80 points to the killer, not shootable, MO 0x234, enemy_dying,
// flags bit 20 while others live, enemies_alive - 1 with draw_flags bit 3, sound 0xA01314
template <class P> void Game<P>::kind6_death_check(uint32_t obj)
{
    Enemy& e = mem.at<Enemy>(obj);
    if (((uint16_t(e.flags) >> 8) & 1) == 0 && return_false() == 0)
        return;
    if (uint8_t(mem.at<Enemy>(obj).killer) != 0xFF)
        add_score(mem.at<Enemy>(obj).killer, 0x50);
    mem.at<Enemy>(obj).flags &= ~1u;
    set_motion(obj, 0x234);
    Enemy& d = mem.at<Enemy>(obj);
    d.timer = 0;
    d.routine = F::enemy_dying;
    if (1 < int32_t(mem.at<int16_t>(G::enemies_alive)))
        d.flags |= 0x100000;
    if ((d.draw_flags >> 3) & 1)
        mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) - 1);
    sound_cue(P::pc ? 0x61 : 0xA01314);
}

// ---------------------------------------------------------------- kind 10: a zombie in a scene (chapter 2 event 4)

// 0x941E0 (kind 10): body with its list's first motion, anim_time rand() % 10, AI-at-once flag, type set-up,
// counted, walk variant 1; waits for its scene
template <class P> void Game<P>::kind10_zombie_scene(uint32_t obj)
{
    mem.set32(G::hit_type, uint32_t(int32_t(mem.at<Enemy>(obj).type)));
    const int32_t t = int32_t(mem.u32(G::hit_type));
    const uint32_t row = mem.u32(G::enemy_motion_lists + uint32_t(t) * 4);
    const uint32_t list = mem.u32(row + uint32_t(int32_t(mem.at<Enemy>(obj).action)) * 4);
    body_init(obj, t, uint32_t(int32_t(int16_t(mem.u32(list)))));
    const int32_t r = int32_t(rand());
    mem.at<Enemy>(obj).anim_time = r % 10;
    enemy_take_ai_at_once(obj);
    creature_type_setup(obj);
    mem.at<Enemy>(obj).state = 0;
    enemy_counts_add(1);
    mem.set32(obj + ob(0x330), 1);
    mem.at<Enemy>(obj).routine = F::kind10_zombie_scene_wait;
    mem.at<Enemy>(obj).substate = 0;
    kind10_zombie_scene_wait(obj);
}

// 0x94290: at event flag +0x12 the standard wait (enemy_wait_camera_stop); from camera frame 623 it leaves
template <class P> void Game<P>::kind10_zombie_scene_wait(uint32_t obj)
{
    if (mem.at<uint8_t>(G::event_flags + 0x12) != 0) {
        mem.at<Enemy>(obj).routine = F::enemy_wait_camera_stop;
        return;
    }
    if (pc_path()) {   // PC: the frame's float bits compared as an integer (0x441BC000 = 623.0)
        if (int32_t(mem.u32(G::camera_records + 0xEC)) >= 0x441BC000)
            kind10_remove(obj);
    } else if (623.0f <= mem.f32(G::camera_records + 0xEC)) {
        kind10_remove(obj);
    }
}


// ---------------------------------------------------------------- kind 3: a zombie jumping down (MO_z_tobiori)

// 0x93100 (kind 3): body (0x281), anim_time 15, AI-at-once flag, type set-up, state / walk variant 0, counted, a
// waiting attack position; falls
template <class P> void Game<P>::kind003_zombie_jump_down(uint32_t obj)
{
    mem.set32(G::hit_type, uint32_t(int32_t(mem.at<Enemy>(obj).type)));
    body_init(obj, int32_t(mem.u32(G::hit_type)), 0x281);
    mem.at<Enemy>(obj).anim_time = 0xF;
    enemy_take_ai_at_once(obj);
    creature_type_setup(obj);
    mem.at<Enemy>(obj).state = 0;
    mem.set32(obj + ob(0x330), 0);
    enemy_counts_add(1);
    attack_slot_take_waiting(obj);
    mem.at<Enemy>(obj).routine = F::kind003_zombie_jump_down_frame;
    mem.at<Enemy>(obj).substate = 0;
    kind003_zombie_jump_down_frame(obj);
}

// 0x931A0: hits, enemy_fall_death_check; the floor (flags bit 23: the camera eye y - 15); drawn, attacker list;
// while shootable: sub-state 0 starts the fall (speed 0, gravity -0.027222222), 1 falls to the floor (a splat, sound
// 0xA0125B), 2 waits until anim_time 65, 3: the standard enemy (sub-state from record +0x324, blend 5); depth list
template <class P> void Game<P>::kind003_zombie_jump_down_frame(uint32_t obj)
{
    kind_hits(obj);
    enemy_fall_death_check(obj);
    float fy = mem.f32(G::floor_y);
    if ((mem.at<Enemy>(obj).flags >> 23) & 1)
        fy = mem.f32(G::camera_records + 4) - 15.0f;
    shadow_setup(fy);
    object_draw(obj);
    if (mem.at<Enemy>(obj).attack_slot > 1)
        attacker_list_add(obj);
    Enemy& e = mem.at<Enemy>(obj);
    if ((e.flags & 1) == 0)
        return;
    auto finish = [&]() {
        Enemy& f = mem.at<Enemy>(obj);
        if ((f.flags >> 23) & 1)
            f.pos[1] = fy;
        f.state = 1;
        f.substate = int16_t(mem.u32(obj + ob(0x324)));
        if (int16_t(mem.u32(obj + ob(0x324))) == 2)
            mem.set32(obj + ob(0x330), 1);
        set_motion_blend(obj, kind_list_motion(obj, mem.at<Enemy>(obj).substate), 0, 5);
        mem.at<Enemy>(obj).routine = F::enemy_main;
        enemy_target_distance(obj);
    };
    auto landed = [&]() {   // sub-state 2: on the floor until anim_time 65
        Enemy& f = mem.at<Enemy>(obj);
        if ((f.flags >> 23) & 1)
            f.pos[1] = fy;
        const uint32_t a = uint32_t(f.anim_time) + 1;
        f.anim_time = int32_t(a);
        if (a >= 0x41)
            finish();
    };
    auto fall = [&]() {     // sub-state 1
        Enemy& f = mem.at<Enemy>(obj);
        const float v = f.accel[1] + f.vel[1];
        const float y = v + f.pos[1];
        f.vel[1] = v;
        f.pos[1] = y;
        if (fy < y)
            return;
        f.pos[1] = fy;
        floor_splat_spawn_b(obj);
        sound_cue(P::pc ? 0x55 : 0xA0125B);
        mem.at<Enemy>(obj).substate = int16_t(mem.at<Enemy>(obj).substate + 1);
        landed();
    };
    switch (e.substate) {
    case 0:
        e.vel[1] = 0.0f;
        e.accel[1] = -0.027222222f;
        e.substate = int16_t(e.substate + 1);
        fall();
        break;
    case 1: fall(); break;
    case 2: landed(); break;
    case 3: finish(); break;
    default: break;
    }
    depth_list_add(obj);
}

// ---------------------------------------------------------------- kind 4: a zombie turning and walking

// 0x935A0 (kind 4): body (0x292, MO_zonbi3_furitoaruki), anim_time 20, AI-at-once flag, type set-up, counted
template <class P> void Game<P>::kind004_zombie_turn_walk(uint32_t obj)
{
    mem.set32(G::hit_type, uint32_t(int32_t(mem.at<Enemy>(obj).type)));
    body_init(obj, int32_t(mem.u32(G::hit_type)), 0x292);
    mem.at<Enemy>(obj).anim_time = 0x14;
    enemy_take_ai_at_once(obj);
    creature_type_setup(obj);
    mem.at<Enemy>(obj).state = 0;
    mem.set32(obj + ob(0x330), 0);
    enemy_counts_add(1);
    mem.at<Enemy>(obj).routine = F::kind004_zombie_turn_walk_frame;
    mem.at<Enemy>(obj).substate = 0;
    kind004_zombie_turn_walk_frame(obj);
}

// 0x93630: hits, enemy_check_death, drawn; while shootable after anim_time 47: a waiting attack position and the
// standard enemy (sub-state from record +0x324: 2 = walking, delay record +0x320 x 5, else x 10; blend 10)
template <class P> void Game<P>::kind004_zombie_turn_walk_frame(uint32_t obj)
{
    kind_hits(obj);
    enemy_check_death(obj);
    kind_draw(obj);
    Enemy& e = mem.at<Enemy>(obj);
    if ((e.flags & 1) == 0)
        return;
    if (0x2Fu < uint32_t(e.anim_time)) {
        attack_slot_take_waiting(obj);
        Enemy& f = mem.at<Enemy>(obj);
        f.state = 1;
        const int16_t sub = int16_t(mem.u32(obj + ob(0x324)));
        f.substate = sub;
        const int32_t d = int32_t(mem.u32(obj + ob(0x320)));
        if (sub == 2) {
            mem.set32(obj + ob(0x2EC), uint32_t(d * 5));
            mem.set32(obj + ob(0x330), 1);
        } else {
            mem.set32(obj + ob(0x2EC), uint32_t(d * 10));
        }
        set_motion_blend(obj, kind_list_motion(obj, mem.at<Enemy>(obj).substate), 0, 10);
        mem.at<Enemy>(obj).routine = F::enemy_main;
        enemy_target_distance(obj);
    }
    depth_list_add(obj);
}

// ---------------------------------------------------------------- kind 0 / 9: the standard enemy's spawn

// the walk set-up shared by kinds 0 and 9: record +0x324 (the start sub-state) 7 = one of two walks, 6 = one of six
// (the kind's set 2 / set 6; walk variants 6-7 / 2-7), 8 = motion m8 (walk variant 8); false for any other value
template <class P> bool Game<P>::kind_spawn_walk(uint32_t obj, uint32_t set2, uint32_t set6, uint32_t m8)
{
    const int32_t t = int32_t(mem.u32(G::hit_type));
    const int32_t a = int32_t(mem.u32(obj + ob(0x324)));
    if (a == 7) {
        const int32_t r = (int32_t(rand()) >> 4) % 2;
        body_init(obj, t, uint32_t(int32_t(int16_t(mem.u32(set2 + uint32_t(r) * 4)))));
        mem.set32(obj + ob(0x330), uint32_t(r + 6));
        mem.set32(obj + ob(0x324), 1);
        return true;
    }
    if (a == 6) {
        const int32_t r = (int32_t(rand()) >> 4) % 6;
        body_init(obj, t, uint32_t(int32_t(int16_t(mem.u32(set6 + uint32_t(r) * 4)))));
        mem.set32(obj + ob(0x324), 1);
        mem.set32(obj + ob(0x330), uint32_t(r + 2));
        return true;
    }
    if (a == 8) {
        body_init(obj, t, m8);
        mem.set32(obj + ob(0x324), 1);
        mem.set32(obj + ob(0x330), 8);
        return true;
    }
    return false;
}

// 0x77110 (kind 0, the standard enemy): one-hit kill for types 41-44 and 46; the body by the start sub-state (record
// +0x324, kind_spawn_walk; else the list motion of that sub-state); AI-at-once flag, type set-up, counted; headless
// types 30 / 84 (flags bit 19) count up (a sound at the first); anim_time rand() % 10, sub-state = +0x324, delay =
// record +0x320 x 5 (sub-state 2, walking) or x 10; then enemy_wait_camera_stop
template <class P> void Game<P>::kind000_enemy_spawn(uint32_t obj)
{
    mem.set32(G::hit_type, uint32_t(int32_t(mem.at<Enemy>(obj).type)));
    const int32_t t = int32_t(mem.u32(G::hit_type));
    mem.set32(obj + ob(0x308), (t < 0x29 || (0x2C < t && t != 0x2E)) ? 0u : 1u);
    if (!kind_spawn_walk(obj, G::enemy_walk_set2, G::enemy_walk_set6, P::pc ? mem.u32(G::enemy_walk_set6 + 0x18) : 0x287u))
        body_init(obj, t, uint32_t(int32_t(int16_t(kind_list_motion(obj, int32_t(mem.u32(obj + ob(0x324))))))));
    enemy_take_ai_at_once(obj);
    creature_type_setup(obj);
    mem.at<Enemy>(obj).state = 0;
    enemy_counts_add(1);
    const uint32_t fl = mem.at<Enemy>(obj).flags;
    if ((t == 0x1E || t == 0x54) && ((fl >> 19) & 1)) {
        if (mem.u32(G::headless_countdown) == 0)
            sound_cue(t == 0x1E ? (P::pc ? 0x8F : 0xA01355) : (P::pc ? 0xFB : 0xA01465));
        mem.set32(G::headless_countdown, mem.u32(G::headless_countdown) + 1);
    }
    const int32_t r = int32_t(rand());
    Enemy& e = mem.at<Enemy>(obj);
    e.substate = int16_t(mem.u32(obj + ob(0x324)));
    e.anim_time = r % 10;
    const int32_t d = int32_t(mem.u32(obj + ob(0x320)));
    if (int16_t(mem.u32(obj + ob(0x324))) == 2) {
        mem.set32(obj + ob(0x2EC), uint32_t(d * 5));
        mem.set32(obj + ob(0x330), 1);
    } else {
        mem.set32(obj + ob(0x2EC), uint32_t(d * 10));
    }
    mem.at<Enemy>(obj).routine = F::enemy_wait_camera_stop;
    enemy_wait_camera_stop(obj);
}

// 0x94040 (kind 9): as kind 0 with its own walk sets and motion 0x268 for start sub-state 8; with record +0x320 set:
// motion 0x1FA (twitching), +0x320 cleared. anim_time rand() % 10, AI-at-once, type set-up, counted, 2 = walking
template <class P> void Game<P>::kind009_enemy_spawn_b(uint32_t obj)
{
    mem.set32(G::hit_type, uint32_t(int32_t(mem.at<Enemy>(obj).type)));
    const int32_t t = int32_t(mem.u32(G::hit_type));
    if (mem.u32(obj + ob(0x320)) == 0) {
        if (!kind_spawn_walk(obj, G::enemy_walk_set2_b, G::enemy_walk_set6_b, P::pc ? mem.u32(G::enemy_walk_set6_b + 0x18) : 0x268u))
            body_init(obj, t, uint32_t(int32_t(int16_t(kind_list_motion(obj, 0)))));
    } else {
        body_init(obj, t, 0x1FA);
        mem.set32(obj + ob(0x320), 0);
    }
    const int32_t r = int32_t(rand());
    mem.at<Enemy>(obj).anim_time = r % 10;
    enemy_take_ai_at_once(obj);
    creature_type_setup(obj);
    mem.at<Enemy>(obj).state = 0;
    enemy_counts_add(1);
    if (int16_t(mem.u32(obj + ob(0x324))) == 2)
        mem.set32(obj + ob(0x330), 1);
    mem.at<Enemy>(obj).routine = F::enemy_wait_camera_stop;
    mem.at<Enemy>(obj).substate = 0;
    enemy_wait_camera_stop(obj);
}

// ---------------------------------------------------------------- kind 8: an enemy rising into view

// 0x93EC0 (kind 8): type / action / +0x320 / +0x324 from bytes 0-3 of its record (+0x35C), the body with its list's
// first motion, anim_time rand() % 10, frames = record +0x10, AI-at-once, type set-up, counted; rises
template <class P> void Game<P>::kind008_enemy_rise(uint32_t obj)
{
    const uint32_t rec = mem.u32(obj + ob(0x35C));
    Enemy& e = mem.at<Enemy>(obj);
    e.type = int16_t(mem.at<uint8_t>(rec));
    mem.set32(G::hit_type, uint32_t(mem.at<uint8_t>(rec)));
    e.action = int16_t(mem.at<uint8_t>(rec + 1));
    mem.set32(obj + ob(0x320), mem.at<uint8_t>(rec + 2));
    mem.set32(obj + ob(0x324), mem.at<uint8_t>(rec + 3));
    body_init(obj, int32_t(mem.u32(G::hit_type)), uint32_t(int32_t(int16_t(kind_list_motion(obj, 0)))));
    const int32_t r = int32_t(rand());
    mem.at<Enemy>(obj).anim_time = r % 10;
    mem.set32(obj + ob(0x2EC), mem.u32(rec + 0x10));
    enemy_take_ai_at_once(obj);
    creature_type_setup(obj);
    mem.at<Enemy>(obj).state = 0;
    enemy_counts_add(1);
    mem.at<Enemy>(obj).routine = F::kind008_enemy_rise_frame;
    mem.at<Enemy>(obj).substate = 0;
    kind008_enemy_rise_frame(obj);
}

// 0x93FA0: shadow at its own y, drawn; while shootable rises by the record's float +8 each frame for +0x2EC frames,
// then the standard wait (delay record +0x320 x 5 walking / x 10)
template <class P> void Game<P>::kind008_enemy_rise_frame(uint32_t obj)
{
    const uint32_t rec = mem.u32(obj + ob(0x35C));
    shadow_setup(mem.at<Enemy>(obj).pos[1]);
    object_draw(obj);
    Enemy& e = mem.at<Enemy>(obj);
    if ((e.flags & 1) == 0)
        return;
    const float y = e.pos[1];
    const float dy = mem.f32(rec + 8);
    const int32_t n = int32_t(mem.u32(obj + ob(0x2EC))) - 1;
    e.anim_time = e.anim_time + 1;
    mem.set32(obj + ob(0x2EC), uint32_t(n));
    e.pos[1] = dy + y;
    if (n != 0)
        return;
    e.state = 0;
    const int32_t d = int32_t(mem.u32(obj + ob(0x320)));
    if (int16_t(mem.u32(obj + ob(0x324))) == 2) {
        mem.set32(obj + ob(0x2EC), uint32_t(d * 5));
        mem.set32(obj + ob(0x330), 1);
    } else {
        mem.set32(obj + ob(0x2EC), uint32_t(d * 10));
    }
    mem.at<Enemy>(obj).substate = 0;
    mem.at<Enemy>(obj).routine = F::enemy_wait_camera_stop;
}

// ---------------------------------------------------------------- kind 12: a zombie on a ledge above

// 0xA1900 (kind 12): body (0x252), anim_time rand() % 10, type set-up, the attack position ang[0] + 2 held, counted;
// ang[0] cleared, slot_wanted = that position - 2, target = ang[0], delay record +0x320 x 10
template <class P> void Game<P>::kind012_zombie_above(uint32_t obj)
{
    mem.set32(G::hit_type, uint32_t(int32_t(mem.at<Enemy>(obj).type)));
    body_init(obj, int32_t(mem.u32(G::hit_type)), 0x252);
    const int32_t r = int32_t(rand());
    mem.at<Enemy>(obj).anim_time = r % 10;
    creature_type_setup(obj);
    Enemy& e = mem.at<Enemy>(obj);
    e.state = 0;
    mem.set32(obj + ob(0x330), 0);
    const int8_t c = int8_t(e.ang[0]);
    const uint8_t b = uint8_t(c + 2);
    mem.at<uint8_t>(G::attack_slots + uint32_t(b) * 20) = 1;
    mem.set32(G::attack_slots + uint32_t(b) * 20 + 4, obj);
    enemy_counts_add(1);
    Enemy& f = mem.at<Enemy>(obj);
    f.routine = F::kind012_zombie_above_frame;
    f.ang[0] = 0;
    f.substate = 0;
    f.attack_slot = int8_t(b);
    mem.set32(obj + ob(0x338), uint32_t(b) - 2);
    f.target = c;
    mem.set32(obj + ob(0x2EC), uint32_t(int32_t(mem.u32(obj + ob(0x320))) * 10));
    kind012_zombie_above_frame(obj);
}

// 0xA19D0: hits, enemy_check_death, drawn; while shootable the attack and the depth list
template <class P> void Game<P>::kind012_zombie_above_frame(uint32_t obj)
{
    kind_hits(obj);
    enemy_check_death(obj);
    kind_draw(obj);
    if ((mem.at<Enemy>(obj).flags & 1) == 0)
        return;
    kind012_zombie_above_attack(obj);
    depth_list_add(obj);
}

// 0xA1A40: sub-state 0 waits its delay, then motion 0x253 (waiting above, start frame rand() % 10); from camera frame
// 300: with one player who is not its target 0x253 again (target released, sub-state 3), else 0x254 (the punch
// down); 2 after anim_time 25 hits its target (with one player only if it is the solo player); 3 at the end of the
// loop the standard enemy (list motion 0, blend 5)
template <class P> void Game<P>::kind012_zombie_above_attack(uint32_t obj)
{
    auto E = [&]() -> Enemy& { return mem.at<Enemy>(obj); };
    int stage = E().substate;   // where the original's switch enters; each stage falls into the next
    if (stage < 0 || stage > 3)
        return;
    if (stage == 0) {
        if (mem.u32(obj + ob(0x2EC)) != 0) {
            mem.set32(obj + ob(0x2EC), mem.u32(obj + ob(0x2EC)) - 1);
            return;
        }
        const int32_t r = int32_t(rand());
        set_motion_blend(obj, 0x253, r % 10, 5);
        E().substate = int16_t(E().substate + 1);
        stage = 1;
    }
    if (stage == 1) {
        if (!(300.0f <= mem.f32(G::camera_records + 0xEC)))
            return;
        if (mem.at<uint16_t>(G::players_in_game) == 1 && mem.at<int8_t>(G::solo_player_is_p2) != E().target) {
            set_motion_blend(obj, 0x253, 0, 10);
            E().target = -1;
            E().substate = 3;
        } else {
            set_motion_blend(obj, 0x254, 0, 5);
            E().substate = int16_t(E().substate + 1);
        }
        stage = 2;
    }
    if (stage == 2) {
        if (!(0x19u < uint32_t(E().anim_time)))
            return;
        const uint8_t tg = uint8_t(E().target);
        if (mem.at<uint16_t>(G::players_in_game) != 1 || int32_t(tg) == int32_t(mem.at<int8_t>(G::solo_player_is_p2)))
            player_hit(int8_t(tg), 1, 0);
        E().target = -1;
        E().substate = int16_t(E().substate + 1);
    }
    if (((E().motion_flags >> 4) & 1) == 0)
        return;
    E().state = 1;
    E().substate = int16_t(mem.u32(obj + ob(0x324)));
    if (int16_t(mem.u32(obj + ob(0x324))) == 2)
        mem.set32(obj + ob(0x330), 1);
    set_motion_blend(obj, kind_list_motion(obj, 0), 0, 5);
    E().routine = F::enemy_main;
    enemy_target_distance(obj);
}

// ---------------------------------------------------------------- kind 5: a zombie hidden as a model (chapter 3 event 0)

// 0x93720 (kind 5): body (0x203), AI-at-once flag, type set-up, state / walk variant / sub-state 0, the plain model of
// its type (kind5_hidden_models), waits
template <class P> void Game<P>::kind5_zombie_hidden(uint32_t obj)
{
    mem.set32(G::hit_type, uint32_t(int32_t(mem.at<Enemy>(obj).type)));
    body_init(obj, int32_t(mem.u32(G::hit_type)), 0x203);
    enemy_take_ai_at_once(obj);
    creature_type_setup(obj);
    mem.at<Enemy>(obj).state = 0;
    mem.set32(obj + ob(0x330), 0);
    const uint32_t model = mem.u32(G::kind5_hidden_models + mem.u32(G::hit_type) * 4);
    mem.at<Enemy>(obj).routine = F::kind5_zombie_hidden_wait;
    mem.at<Enemy>(obj).substate = 0;
    mem.at<Enemy>(obj).model = int32_t(model);
    kind5_zombie_hidden_wait(obj);
}

// 0x937A0: drawn as its plain model (a matrix pushed, moved to its position, turned by its yaw) or, without one, as
// its body; at its event flag (event_flags + record +0x320) counted as an enemy and fighting
template <class P> void Game<P>::kind5_zombie_hidden_wait(uint32_t obj)
{
    Enemy& e = mem.at<Enemy>(obj);
    if (e.model == 0) {
        shadow_setup(mem.f32(G::floor_y));
        object_draw(obj);
    } else if constexpr (P::pc) {
        // PC: not drawn at all with pc_no_extra_model set; else the software matrix stack
        if (mem.u32(G::pc_no_extra_model) == 0) {
            pc_matrix_push();
            pc_matrix_translate2(e.pos[0], e.pos[1], e.pos[2]);
            pc_matrix_rotate_y2(uint32_t(mem.at<Enemy>(obj).ang[1]));
            geo_draw_model(uint32_t(mem.at<Enemy>(obj).model));
            pcm_pop();
        }
    } else {
        Tgp::Out none;
        copro(0x05, nullptr, 0, none);
        uint32_t t[3] = {fbits(e.pos[0]), fbits(e.pos[1]), fbits(e.pos[2])};
        copro(0x12, t, 3, none);
        uint32_t a[1] = {uint32_t(e.ang[1])};
        copro(0x15, a, 1, none);
        geo_draw_model(uint32_t(mem.at<Enemy>(obj).model));
        copro(0x06, nullptr, 0, none);
    }
    if (mem.at<uint8_t>(G::event_flags + mem.u32(obj + ob(0x320))) != 0) {
        enemy_counts_add(1);
        mem.at<Enemy>(obj).routine = F::kind5_zombie_fight;
    }
}

// 0x93860: as kind2_zombie_fight with blend 10
template <class P> void Game<P>::kind5_zombie_fight(uint32_t obj)
{
    kind_hits(obj);
    enemy_check_death(obj);
    kind_draw(obj);
    Enemy& e = mem.at<Enemy>(obj);
    if ((e.flags & 1) == 0)
        return;
    if ((e.motion_flags >> 4) & 1) {
        attack_slot_take_waiting(obj);
        const int16_t sub = int16_t(mem.u32(obj + ob(0x324)));
        Enemy& f = mem.at<Enemy>(obj);
        f.state = 1;
        f.substate = sub;
        if (sub == 2)
            mem.set32(obj + ob(0x330), 1);
        set_motion_blend(obj, kind_list_motion(obj, mem.at<Enemy>(obj).substate), 0, 10);
        mem.at<Enemy>(obj).routine = F::enemy_main;
        enemy_target_distance(obj);
    }
    depth_list_add(obj);
}

// ---------------------------------------------------------------- kind 7: a zombie that falls in with a piece of the
// ceiling (chapter 3 event 1)

// 0x93BE0 (kind 7): type / action / +0x320 / +0x324 from the record's first bytes (record = +0x35C), body (0x270), the
// fall's frame count (record +0x10) in +0x2EC, AI-at-once flag, type set-up, the falling piece, counted, a waiting
// attack position; fall speed 0, gravity -0.0272 (+0x7C / +0x88), sub-state 0
template <class P> void Game<P>::kind7_zombie_drop(uint32_t obj)
{
    const uint32_t rec = mem.u32(obj + ob(0x35C));
    const uint8_t type = mem.at<uint8_t>(rec);
    mem.at<Enemy>(obj).type = int16_t(type);
    mem.set32(G::hit_type, uint32_t(type));
    mem.at<Enemy>(obj).action = int16_t(mem.at<uint8_t>(rec + 1));
    mem.set32(obj + ob(0x320), mem.at<uint8_t>(rec + 2));
    mem.set32(obj + ob(0x324), mem.at<uint8_t>(rec + 3));
    body_init(obj, int32_t(mem.u32(G::hit_type)), 0x270);
    mem.set32(obj + ob(0x2EC), mem.u32(rec + 0x10));
    enemy_take_ai_at_once(obj);
    creature_type_setup(obj);
    mem.at<Enemy>(obj).state = 0;
    mem.set32(obj + ob(0x330), 0);
    kind7_falling_object(obj);
    enemy_counts_add(1);
    attack_slot_take_waiting(obj);
    mem.set32(obj + ob(0x88), 0xBCDF0123u);   // -0.0272
    mem.set32(obj + ob(0x7C), 0);
    mem.at<Enemy>(obj).routine = F::kind7_zombie_drop_frame;
    mem.at<Enemy>(obj).substate = 0;
    kind7_zombie_drop_frame(obj);
}

// 0x93CC0: hits, enemy_fall_death_check, shadow and draw, the attacker list (slot > 1); while shootable: sub-state 0
// moves by the record's speed (+4 / +8 / +0xC) for +0x2EC frames (anim_time + 1), then the jump-down frame of kind 3
// with motion 0x281 (15 / 5); depth list
template <class P> void Game<P>::kind7_zombie_drop_frame(uint32_t obj)
{
    const uint32_t rec = mem.u32(obj + ob(0x35C));
    kind_hits(obj);
    enemy_fall_death_check(obj);
    shadow_setup(mem.f32(G::floor_y));
    object_draw(obj);
    if (mem.at<Enemy>(obj).attack_slot > 1)
        attacker_list_add(obj);
    if ((mem.at<Enemy>(obj).flags & 1) == 0)
        return;
    Enemy& e = mem.at<Enemy>(obj);
    if (e.substate == 0) {
        e.pos[0] = e.pos[0] + mem.f32(rec + 4);
        e.pos[1] = e.pos[1] + mem.f32(rec + 8);
        e.pos[2] = e.pos[2] + mem.f32(rec + 0xC);
        e.anim_time = e.anim_time + 1;
        const uint32_t n = mem.u32(obj + ob(0x2EC)) - 1;
        mem.set32(obj + ob(0x2EC), n);
        if (n == 0) {
            mem.at<Enemy>(obj).routine = F::kind003_zombie_jump_down_frame;
            mem.set32(obj + ob(0x2EC), 0);
            mem.at<Enemy>(obj).substate = 0;
            set_motion_blend(obj, 0x281, 15, 5);
        }
    }
    depth_list_add(obj);
}

// 0x93DB0: the piece of ceiling (model 0x1682, a new task kind7_falling_piece with the zombie's record), 22.5 above
// the zombie, falling for the record's count + 60 frames; its first frame at once (arcade: inline), then sound
template <class P> void Game<P>::kind7_falling_object(uint32_t obj)
{
    const uint32_t task = task_create(F::kind7_falling_piece, uint32_t(sizeof(Obj)));
    const uint32_t rec = mem.u32(obj + ob(0x35C));
    Obj& t = mem.at<Obj>(task);
    t.model = 0x1682;
    mem.set32(task + ob(0x35C), rec);
    mem.set32(task + ob(0x2EC), mem.u32(rec + 0x10) + 0x3C);
    const Obj& o = mem.at<Obj>(obj);
    t.pos[0] = o.pos[0];
    t.pos[1] = o.pos[1] + 22.5f;
    t.pos[2] = o.pos[2];
    kind7_falling_piece(task);
    sound_cue(P::pc ? 0x105 : 0xA01470);
}

// 0x942D0: drawn at its position (PC: not with pc_no_extra_model), moved by the record's speed; after +0x2EC frames
// the landing sound and the task ends
template <class P> void Game<P>::kind7_falling_piece(uint32_t obj)
{
    const uint32_t rec = mem.u32(obj + ob(0x35C));
    if constexpr (P::pc) {
        if (mem.u32(G::pc_no_extra_model) == 0) {
            pc_matrix_push();
            const Obj& o = mem.at<Obj>(obj);
            pc_matrix_translate2(o.pos[0], o.pos[1], o.pos[2]);
            geo_draw_model(uint32_t(mem.at<Obj>(obj).model));
            pcm_pop();
        }
    } else {
        Tgp::Out none;
        copro(0x05, nullptr, 0, none);
        uint32_t tr[3] = {mem.u32(obj + ob(0x20)), mem.u32(obj + ob(0x24)), mem.u32(obj + ob(0x28))};
        copro(0x12, tr, 3, none);
        geo_draw_model(uint32_t(mem.at<Obj>(obj).model));
        copro(0x06, nullptr, 0, none);
    }
    Obj& o = mem.at<Obj>(obj);
    o.pos[0] = o.pos[0] + mem.f32(rec + 4);
    o.pos[1] = o.pos[1] + mem.f32(rec + 8);
    o.pos[2] = o.pos[2] + mem.f32(rec + 0xC);
    const uint32_t n = mem.u32(obj + ob(0x2EC)) - 1;
    mem.set32(obj + ob(0x2EC), n);
    if (n == 0) {
        sound_cue(P::pc ? 0x106 : 0xA01471);
        task_exit();
        return;
    }
}

// PC 0x503B64: a copy of pc_matrix_rotate_y (kind file)
template <class P> void Game<P>::pc_matrix_rotate_y2(uint32_t angle) { pc_matrix_rotate_y(angle); }

// ---------------------------------------------------------------- kind 11: an Ebitan that leaps at a player

// 0xA13D0 (kind 11): body (0x23A), type set-up, counted; one player: front attack position 0 (or 1 when attack_side
// is set and 0 is taken), target the solo player; two: a random player not targeted yet (rand() % 2, else the
// other); players_seen = players_in_game; target_pos = the target player's body x / z
template <class P> void Game<P>::kind11_ebita_leap(uint32_t obj)
{
    mem.set32(G::hit_type, uint32_t(int32_t(mem.at<Enemy>(obj).type)));
    body_init(obj, int32_t(mem.u32(G::hit_type)), 0x23A);
    creature_type_setup(obj);
    mem.at<Enemy>(obj).state = 0;
    mem.set32(obj + ob(0x330), 0);
    enemy_counts_add(1);
    mem.at<Enemy>(obj).substate = 0;
    mem.at<Enemy>(obj).routine = F::kind11_ebita_leap_frame;
    auto take = [&](uint32_t slot) {
        mem.at<uint8_t>(G::attack_slots + slot * 20) = 1;
        mem.set32(G::attack_slots + slot * 20 + 4, obj);
    };
    if (mem.at<uint16_t>(G::players_in_game) == 1) {
        if (mem.at<uint8_t>(G::attack_side) == 0 || mem.at<uint8_t>(G::attack_slots) == 0) {
            take(0);
            mem.at<Enemy>(obj).attack_slot = 0;
            mem.set32(obj + ob(0x338), 0);
        } else {
            take(1);
            mem.at<Enemy>(obj).attack_slot = 1;
            mem.set32(obj + ob(0x338), 1);
        }
        const int8_t solo = mem.at<int8_t>(G::solo_player_is_p2);
        mem.at<Enemy>(obj).target = solo;
        mem.at<uint8_t>(G::player_targeted + uint32_t(int32_t(solo))) = 1;
    } else {
        uint32_t r = uint32_t(int32_t(rand()) % 2);
        if (mem.at<uint8_t>(G::player_targeted + r) != 0)
            r = ~r & 1;
        mem.set32(G::attack_slots + r * 20 + 4, obj);
        mem.at<uint8_t>(G::attack_slots + r * 20) = 1;
        mem.at<Enemy>(obj).attack_slot = int8_t(r);
        mem.set32(obj + ob(0x338), r);
        mem.at<Enemy>(obj).target = int8_t(r);
        mem.at<uint8_t>(G::player_targeted + r) = 1;
    }
    mem.set32(obj + ob(0x334), uint32_t(int32_t(mem.at<int16_t>(G::players_in_game))));
    const uint32_t body = mem.u32(G::players + uint32_t(int32_t(mem.at<Enemy>(obj).target)) * 0xCC + 0xBC);
    Enemy& e = mem.at<Enemy>(obj);
    const float x = mem.f32(body + ob(0x20));
    e.target_pos[1] = 0.0f;
    e.target_pos[0] = x;
    e.target_pos[2] = mem.f32(body + ob(0x28));
    kind11_ebita_leap_frame(obj);
}

// 0xA1550: hits, enemy_check_death, drawn; while shootable the leap and the depth list
template <class P> void Game<P>::kind11_ebita_leap_frame(uint32_t obj)
{
    kind_hits(obj);
    enemy_check_death(obj);
    kind_draw(obj);
    if ((mem.at<Enemy>(obj).flags & 1) == 0)
        return;
    kind11_leap(obj);
    depth_list_add(obj);
}

// 0xA15C0: the target update; sub-state 0 at a loop end MO 0x239 (the jump); 1 within 23 of the target (coprocessor
// distance) MO 0x23B (the attack jump); 2 at its end player_hit(target, 1, 1), target released, state 9 sub-state 3,
// list motion 3, target_pos = its attack position, the standard enemy; every frame the turn toward target_pos
template <class P> void Game<P>::kind11_leap(uint32_t obj)
{
    auto E = [&]() -> Enemy& { return mem.at<Enemy>(obj); };
    kind11_leap_move(obj);
    int stage = E().substate;
    if (stage == 0) {
        if ((E().motion_flags >> 4) & 1) {
            set_motion(obj, 0x239);
            E().substate = int16_t(E().substate + 1);
            stage = 1;
        }
    }
    if (stage == 1) {
        const float d = enemy_distance(E().pos[0], E().pos[2], E().target_pos[0], E().target_pos[2]);
        if (!(d > 23.0f)) {
            set_motion(obj, 0x23B);
            E().substate = int16_t(E().substate + 1);
            stage = 2;
        }
    }
    if (stage == 2 && ((E().motion_flags >> 4) & 1)) {
        player_hit(E().target, 1, 1);
        mem.at<uint8_t>(G::player_targeted + uint32_t(int32_t(E().target))) = 0;
        E().target = -1;
        E().state = 9;
        E().substate = 3;
        set_motion(obj, kind_list_motion(obj, 3));
        const uint32_t slot = G::attack_slots + uint32_t(int32_t(E().attack_slot)) * 20;
        const float x = mem.f32(slot + 8);
        E().target_pos[1] = 0.0f;
        E().target_pos[0] = x;
        E().routine = F::enemy_main;
        E().target_pos[2] = mem.f32(slot + 0x10);
    }
    if constexpr (P::pc)
        enemy_turn_pc(obj, false, false);
    else if (pc_path())   // PC Mode (docs/pcmode.md row 10): the PC's turn on plain values, no minimum slowdown
        E().ang[1] = pm::enemy_turn(E().ang[1], E().pos, E().target_pos, false, false);
    else
        enemy_turn_a(obj, false);
}

} // namespace hotd::port
