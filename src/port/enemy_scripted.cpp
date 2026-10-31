// The scripted enemies (guard actions) being hit and dying, the walk-in wait for the camera, and the scripted zombie
// fall (Zombie_Dead). First written as runtime overrides (T1, src/game/t1_bounds.cpp), read from the disassembly.
// Arcade forms (decl/support.h).
#include "port/game.h"

#include <cstring>

namespace hotd::port {

namespace {
constexpr uint32_t kEnemySlotFlags = 0x51FB68;   // the enemy slots' flag bytes (0x2C each, by obj +0x2E6) (arcade)
constexpr uint32_t kHitMotions = 0xC14A0;        // the hit reactions' motions by hit class % 9 (arcade)
} // namespace

// 0x535F0 enemy_scripted_hit: hit this frame (flags bit 8, not action 12): a hit reaction (motion kHitMotions[hit_class
// % 9], held 0x14 / 10, blend 8), slot flag bit 2, a voice; otherwise the reaction's end (flags bit 28 and +0x3C4 word
// bits 15 / 22) clears them. Damage off +0x70; at 0 (or chapter 6's class-1 hits) a death motion by the angle to the
// camera (0x237 / 0x236 / 0x233; action 7 always 0x233), enemy_scripted_dead, enemies_alive - 1
template <class P> void Game<P>::enemy_scripted_hit(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;   // the PC form: not ported (vtable only; ProofPc replays it)
    } else {
        const int8_t hitClass = mem.at<int8_t>(G::hit_class);
        const uint32_t slotFlag = kEnemySlotFlags + uint32_t(int32_t(mem.at<int8_t>(obj + 0x2E6))) * 0x2C;
        const bool chapter6 = mem.at<int8_t>(G::chapter) == 5;
        if (hitClass == 0 || !((mem.u32(obj + 0x58) >> 8) & 1) || mem.at<int16_t>(obj + 0x64) == 0xC) {
            if (((mem.u32(obj + 0x58) >> 28) & 1) && (mem.u32(obj + 0x3C4) & 0x408000u) == 0x408000u) {
                mem.set32(obj + 0x58, mem.u32(obj + 0x58) & 0xEFFFFFFFu);
                mem.at<uint8_t>(slotFlag) &= 0xFB;
            }
        } else {
            mem.set32(obj + 0x58, mem.u32(obj + 0x58) | 0x10000000u);
            set_motion_blend_hold(obj, mem.u32(kHitMotions + uint32_t(int32_t(int8_t(hitClass % 9))) * 4), 0x14, 10, 8);
            mem.at<uint8_t>(slotFlag) |= 4;
            if (!chapter6)
                enemy_voice(obj, 0);
        }
        mem.set32(obj + 0x58, mem.u32(obj + 0x58) & 0xFFFFFEFFu);
        const int16_t life = int16_t(mem.at<uint16_t>(obj + 0x70) - mem.at<uint16_t>(G::hit_damage));
        mem.at<int16_t>(obj + 0x70) = life;
        if (life > 0 && return_false() == 0) {
            if (!chapter6 || hitClass != 1)
                return;
        }
        mem.set32(obj + 0x58, mem.u32(obj + 0x58) & 0xFFFFFFFEu);
        if ((mem.at<uint8_t>(obj + 0x3C6) >> 5) & 1)
            body_second_motion(obj, 0);
        const int32_t a = int32_t(angle_to_camera(obj));
        uint32_t m = 0x233;
        if (mem.at<int16_t>(obj + 0x64) != 7) {
            if (a > 0x97FF && a < 0xE800)
                m = 0x237;
            else if (a > 0x17FF && a < 0x6800)
                m = 0x236;
        }
        set_motion_blend(obj, m, 0, 6);
        mem.set32(obj + 0x50, 0);
        mem.set32(obj, F::enemy_scripted_dead);
        if (mem.at<int16_t>(G::enemies_alive) > 1)
            mem.set32(obj + 0x58, mem.u32(obj + 0x58) | 0x100000u);
        mem.at<int16_t>(G::enemies_alive) = int16_t(mem.at<int16_t>(G::enemies_alive) - 1);
        enemy_voice(obj, hitClass != 1 ? 1 : 2);
    }
}

// 0x1C670 enemy_scripted_dead: no longer shootable; drawn; at the death motion's middle frame 0x50 points to the killer
// and its attack position freed; at the motion's end enemy_scripted_dead_fall (frame = count - 2)
template <class P> void Game<P>::enemy_scripted_dead(uint32_t obj)
{
    const int8_t s = mem.at<int8_t>(obj + 0x2E7);
    mem.set32(obj + 0x58, mem.u32(obj + 0x58) & 0xFFFFFFFEu);
    if ((mem.at<uint8_t>(obj + 0x3C4) >> 6) & 1)
        mem.set32(obj + 0x24, mem.u32(mem.u32(obj + 0x18) + 0x24));
    shadow_setup(mem.f32(G::floor_y));
    object_draw(obj);
    if ((mem.at<uint8_t>(obj + 0x3C6) >> 5) & 1)
        mem.set32(obj + 0x10C, mem.u32(obj + 0x10C) + 1);
    const int32_t count = int32_t(mem.u32(G::motion_frame_counts + uint32_t(int32_t(mem.at<int16_t>(obj + 0xD0))) * 4));
    if (!((mem.at<uint8_t>(obj + 0x3C5) >> 4) & 1)) {
        if (mem.at<int16_t>(obj + 0xF4) == count / 2 && !((mem.at<uint8_t>(obj + 0x3C6) >> 1) & 1)) {
            const int8_t killer = mem.at<int8_t>(obj + 0x2EB);
            if (killer == 0)
                add_score(0, 0x50);
            if (killer == 1)
                add_score(1, 0x50);
            if (coop4_on() && (killer == 2 || killer == 3))   // 4P: players 3 / 4
                add_score(killer, 0x50);
            if (s != -1)
                mem.at<uint8_t>(G::attack_slots + uint32_t(int32_t(s)) * 0x14) = 0;
        }
        mem.set32(obj + 0xF8, mem.u32(obj + 0xF8) + 1);
        return;
    }
    const uint16_t f = uint16_t(count - 2);
    mem.set32(obj, F::enemy_scripted_dead_fall);
    mem.at<int16_t>(obj + 0xF4) = int16_t(f);
    mem.set32(obj + 0xF8, uint32_t(int32_t(int16_t(f))));
    mem.at<uint8_t>(obj + 0x3C5) |= 8;
}

// 0x1C780 enemy_scripted_dead_fall: follows its group's height; drawn every other frame (always in chapter 6); gone
// after 40 frames (not in chapter 6), enemy_count - 1
template <class P> void Game<P>::enemy_scripted_dead_fall(uint32_t obj)
{
    if ((mem.at<uint8_t>(obj + 0x3C4) >> 6) & 1)
        mem.set32(obj + 0x24, mem.u32(mem.u32(obj + 0x18) + 0x24));
    const int32_t t = int32_t(mem.u32(obj + 0x50));
    mem.set32(obj + 0x50, uint32_t(t + 1));
    const bool chapter6 = mem.at<int8_t>(G::chapter) == 5;
    if ((t % 2) != 0 || chapter6) {
        shadow_setup(mem.f32(G::floor_y));
        object_draw(obj);
    }
    if (int32_t(mem.u32(obj + 0x50)) > 0x27 && !chapter6) {
        body_free_parts(obj);
        mem.at<int16_t>(G::enemy_count) = int16_t(mem.at<int16_t>(G::enemy_count) - 1);
        task_exit();
    }
}

// 0x77350 enemy_wait_camera_stop: walks (footsteps) and is drawn until the camera stops (camera_stopped 2); then state
// 1, the walk motion (the type's list, or a random walk set when +0x330 >= 2; start frame rand % 10, blend 10),
// enemy_main, a waiting attack position, the target
template <class P> void Game<P>::enemy_wait_camera_stop(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;   // the PC form: not ported (vtable only; ProofPc replays it)
    } else {
        enemy_footsteps(obj);
        shadow_setup(mem.f32(G::floor_y));
        object_draw(obj);
        mem.set32(obj + 0xF8, mem.u32(obj + 0xF8) + 1);
        if (mem.u32(G::camera_stopped) != 2)
            return;
        mem.at<int16_t>(obj + 0x5C) = 1;
        if (mem.at<int16_t>(obj + 0x60) == 0) {
            const int32_t variant = int32_t(mem.u32(obj + 0x330));
            mem.at<int16_t>(obj + 0x60) = int16_t(variant == 1 ? 2 : 1);
            uint32_t motion;
            int32_t r;
            if (variant < 2) {
                const int16_t sub = mem.at<int16_t>(obj + 0x60);
                const uint32_t row = mem.u32(G::enemy_motion_lists + uint32_t(int32_t(mem.at<int16_t>(obj + 0xCC))) * 4);
                const uint32_t list = mem.u32(row + uint32_t(int32_t(mem.at<int16_t>(obj + 0x64))) * 4);
                r = int32_t(rand());
                motion = mem.u32(list + uint32_t(int32_t(sub)) * 8);
            } else {
                r = int32_t(rand());
                motion = mem.u32(G::enemy_walk_motions_b + uint32_t(variant) * 4);
            }
            set_motion_blend(obj, motion, r % 10, 10);
        }
        mem.set32(obj, F::enemy_main);
        attack_slot_take_waiting(obj);
        enemy_target_distance(obj);
    }
}

// 0x78FC0 Zombie_Dead -> 0x76E50 Zombie_Dead_frame: the scripted fall of a zombie from its spawn record (branch
// events): 0 vertical speed -0.5, the record's heading (+0xE); 1 the death motion to its second-last frame; 2 the
// height above the floor (record +0x2C, a float) falls away at 0.0272 a frame (double precision); 3 blinks for 40
// frames; then removed (enemy_count - 1 with draw_flags bit 3)
template <class P> void Game<P>::Zombie_Dead(uint32_t obj)
{
    mem.at<int16_t>(obj + 0x60) = 0;
    mem.set32(obj, F::Zombie_Dead_frame);
    Zombie_Dead_frame(obj);
}

// 0x76E50 Zombie_Dead_frame (proven past its first frame on route 1:3,1:9: docs/proof.md "Support routines (2)")
template <class P> void Game<P>::Zombie_Dead_frame(uint32_t obj)
{
    const uint32_t rec = mem.u32(obj + 0x368);
    int16_t sub = mem.at<int16_t>(obj + 0x60);
    bool draw = true;
    auto remove = [&]() {
        if ((mem.at<uint8_t>(obj + 0x3C4) >> 3) & 1)
            mem.at<int16_t>(G::enemy_count) = int16_t(mem.at<int16_t>(G::enemy_count) - 1);
        body_free_parts(obj);
        enemy_free_extras(obj);
        task_exit();
    };
    if (sub < 0 || sub > 3) {
        remove();
        return;
    }
    if (sub == 0) {
        mem.set32(obj + 0x31C, 1);
        mem.set32(obj + 0x7C, fbits(-0.5f));
        mem.at<int16_t>(obj + 0x60) = sub = 1;
        mem.set32(obj + 0x30, uint32_t(int32_t(mem.at<int16_t>(rec + 0xE))));
    }
    if (sub == 1) {
        const int32_t last = int32_t(mem.u32(G::motion_frame_counts + uint32_t(int32_t(mem.at<int16_t>(obj + 0xD0))) * 4)) - 2;
        if (mem.at<int16_t>(obj + 0xF4) != last) {
            sub = -1;   // to the draw
        } else {
            mem.set32(obj + 0x31C, 0);
            mem.at<int16_t>(obj + 0x60) = sub = 2;
            mem.set32(obj + 0x33C, mem.u32(rec + 0x2C));
        }
    }
    if (sub == 2) {
        const float vy = float(double(mem.f32(obj + 0x7C)) - ffrom64(0x3F9BDA5119CE075Full));
        const float h = mem.f32(obj + 0x33C) + vy;
        mem.set32(obj + 0x7C, fbits(vy));
        mem.set32(obj + 0x24, fbits(mem.f32(obj + 0x24) + vy));
        mem.set32(obj + 0x33C, fbits(h));
        if (!(h > 0.0f)) {
            mem.set32(obj + 0x50, 0);
            mem.at<uint8_t>(obj + 0x3C4) |= 4;
            mem.at<int16_t>(obj + 0x60) = sub = 3;
        } else {
            sub = -1;
        }
    }
    if (sub == 3) {
        const int32_t t = int32_t(mem.u32(obj + 0x50)) + 1;
        mem.set32(obj + 0x50, uint32_t(t));
        if (t > 0x27) {
            remove();
            return;
        }
    }
    if (draw && (!((mem.at<uint8_t>(obj + 0x3C4) >> 2) & 1) || (mem.at<uint8_t>(G::blink_phase) & 1)))
        object_draw(obj);
    if (mem.u32(obj + 0x31C) != 0)
        mem.set32(obj + 0xF8, mem.u32(obj + 0xF8) + 1);
}

} // namespace hotd::port
