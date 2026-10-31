// Chapter 2's prop sub-types (kind 128, table 0xAF960; worker "ch2", docs/chapter2_3_plan.md). Read from the arcade
// decompilation / disassembly; arcade forms only. Sub-type record 0xAC2C0 + 0x4C * type (props.cpp). Proven on A
// (ch2proof, docs/teaser_status/ch2proof.md) but where marked UNVERIFIED (teaser).
#include "port/game.h"

namespace hotd::port {

namespace p2 {
inline uint32_t rec(int32_t type) { return 0xAC2C0 + uint32_t(type) * 0x4C; }
} // namespace p2

// 0x3C6B0 prop015_fuuu (sub-type 15, kind 133's grid): shot once (flags bit 28): the model frames 0x12C2..0x1324
// played 5 units higher at a random y angle, drawn only while they play
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop015_fuuu(uint32_t obj)
{
    prop_schedule_update(obj);
    const uint32_t f = of<uint32_t>(obj, 0x58);
    if ((f & 1) && (f & 6) && !((f >> 28) & 1)) {
        of<uint32_t>(obj, 0x58) = f | 0x10000000;
        of<uint32_t>(obj, 0x54) = 0x12C2;
        of<uint32_t>(obj, 0x30) = rand() & 0xFFFF;
    }
    if (of<uint32_t>(obj, 0x2F0) != 0 && ((of<uint32_t>(obj, 0x58) >> 28) & 1)) {
        of<int32_t>(obj, 0x54) += 1;
        of<float>(obj, 0x24) = of<float>(obj, 0x24) + 5.0f;
        prop_draw(obj);
        const int32_t m = of<int32_t>(obj, 0x54);
        of<float>(obj, 0x24) = of<float>(obj, 0x24) - 5.0f;
        if (m > 0x1324)
            of<uint32_t>(obj, 0x58) &= 0xEFFFFFFFu;
    }
    prop_shootable(obj);
}

// 0x3E5F0 prop064_hit_drop (sub-type 64, no model of its own): shot: its sound and prop_hit_drop
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop064_hit_drop(uint32_t obj)
{
    prop_schedule_update(obj);
    if (of<uint32_t>(obj, 0x58) & 6) {
        sound_cue(mem.u32(p2::rec(of<int16_t>(obj, 0xCC)) + 0x64));
        prop_hit_drop(obj);
    }
    prop_shootable(obj);
}

// 0x3EF60 prop067_r2_lid (sub-type 67, Chapter 2 event 0's floor lid): damage steps with impacts; at step 3 its two
// halves 0x517 / 0x518 fly off (prop_lid_fly), the model becomes 1 and route_choice = 1 from then on (+0x2F8)
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop067_r2_lid(uint32_t obj)
{
    prop_schedule_update(obj);
    if (of<uint32_t>(obj, 0x2F8) == 1)
        mem.at<uint8_t>(G::route_choice) = 1;
    const uint32_t f = of<uint32_t>(obj, 0x58);
    if ((f & 1) && (f & 6) &&
        mem.u32(p2::rec(of<int16_t>(obj, 0xCC)) + 0x24 + 4 * of<uint32_t>(obj, 0x32C)) != 0xFFFFFFFFu) {
        sound_cue(mem.u32(p2::rec(of<int16_t>(obj, 0xCC)) + 0x64));
        if (of<int16_t>(obj, 0xCC) != 0x2B) {
            const float y = mem.f32(p2::rec(of<int16_t>(obj, 0xCC)) + 0x50 + 4 * of<uint32_t>(obj, 0x32C)) +
                            of<float>(obj, 0x24);
            prop_view_pos(obj, of<uint32_t>(obj, 0x20), fbits(y), of<uint32_t>(obj, 0x28));
            prop_bullet_impact(obj);
        }
        const uint32_t step = of<uint32_t>(obj, 0x32C) + 1;
        of<uint32_t>(obj, 0x32C) = step;
        const uint32_t r = p2::rec(of<int16_t>(obj, 0xCC));
        if (mem.u32(r + 0x20 + 4 * step) != 0)
            of<uint32_t>(obj, 0x54) = mem.u32(r + 0x20 + 4 * step);
        of<uint32_t>(obj, 0x90) = mem.u32(r + 0x34 + 4 * of<uint32_t>(obj, 0x32C));
        if (of<uint32_t>(obj, 0x32C) == 3) {
            of<uint32_t>(obj, 0x54) = 0x517;
            of<uint32_t>(obj, 0x30) = 0;
            prop_lid_fly(obj);
            of<uint32_t>(obj, 0x54) = 0x518;
            of<uint32_t>(obj, 0x30) = 0x8000;
            prop_lid_fly(obj);
            of<uint32_t>(obj, 0x2F8) = 1;
            of<uint32_t>(obj, 0x54) = 1;
        }
    }
    prop_draw(obj);
    prop_shootable(obj);
}

// 0x3BFA0 prop_flag_swap (sub-types 30 / 31 glass cases, 86 Chapter 3's s3_rf_kop): visible and not stacked: at damage
// step 0 its event flag (31: 0x51E945, 30: 0x51E946, 86 by its x: 505 / 515 / 532 / 549 / 557.9 -> 0x51E990..94)
// shatters it once (prop_glass_shatter) and shows step 1; drawn
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop_flag_swap(uint32_t obj)
{
    prop_schedule_update(obj);
    if (of<uint32_t>(obj, 0x2F0) == 0 || of<uint8_t>(obj, 0x2E6) != 0)
        return;
    if (of<uint32_t>(obj, 0x32C) == 0) {
        const int16_t type = of<int16_t>(obj, 0xCC);
        bool go = false;
        if (type == 31 && mem.at<uint8_t>(0x51E945) != 0)
            go = true;
        else if (type == 30 && mem.at<uint8_t>(0x51E946) != 0)
            go = true;
        else if (type == 0x56) {
            static constexpr uint32_t xs[5] = {0x43FC8000u, 0x4400C000u, 0x44050000u, 0x44094000u, 0x440B799Au};
            for (uint32_t k = 0; k < 5 && !go; ++k)
                if (of<float>(obj, 0x20) == ffrom(xs[k]) && mem.at<uint8_t>(0x51E990 + k) != 0)
                    go = true;
        }
        if (go) {
            prop_glass_shatter(obj);
            const uint32_t step = of<uint32_t>(obj, 0x32C) + 1;
            of<uint32_t>(obj, 0x32C) = step;
            of<uint32_t>(obj, 0x54) = mem.u32(p2::rec(of<int16_t>(obj, 0xCC)) + 0x20 + step * 4);
        }
    }
    prop_draw(obj);
}

// 0x40160 prop_glass_shatter: the splinters (prop_anim_once, casegara00a 0x13D0; sub-type 86: 0x17AE, 18 higher)
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop_glass_shatter(uint32_t obj)
{
    const uint32_t t = task_create(F::prop_anim_once, 0x3C8);
    obj_copy_pose(t, obj);
    of<uint32_t>(t, 0x54) = 0x13D0;
    of<uint32_t>(t, 0x50) = 0;
    of<uint16_t>(t, 0x6A) = 0;
    of<uint32_t>(t, 0x94) = of<uint32_t>(obj, 0x94);
    if (of<int16_t>(obj, 0xCC) == 0x56) {
        const float y = of<float>(t, 0x24) + 18.0f;
        of<uint32_t>(t, 0x54) = 0x17AE;
        of<float>(t, 0x24) = y;
    }
}

// 0x42CF0 prop_anim_once: model +0x54 + frame for 44 frames (T, Ry, S), then the end
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop_anim_once(uint32_t obj)
{
    const int32_t t = of<int32_t>(obj, 0x50) + 1;
    of<int32_t>(obj, 0x50) = t;
    if (t > 0x2B) {
        task_exit();
        return;
    }
    cam_copro(0x05, {});
    cam_copro(0x12, {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)});
    cam_copro(0x15, {of<uint32_t>(obj, 0x30)});
    prop_scale(of<uint32_t>(obj, 0x94));
    geo_draw_model(of<uint32_t>(obj, 0x54) + of<uint32_t>(obj, 0x50));
    cam_copro(0x06, {});
}

// 0x3C770 prop023_kokonoha_leaves (sub-type 23, kind 136): 32 leaves (0x53E4C0, 24 bytes: x, y, z, x / y / z angles;
// 0x53E7C0 a phase each) for 360 frames. Leaves 0-23 drift: the phase steps 0x400 + rand & 0x3FF, x by its sine, z by
// its cosine + 0.5, y by half the sine of the y angle (+0x300 a frame), z angle from that sine; glow (flags bit 26)
// while |z| < 12.5; x angle toward the step (coprocessor 0x0F) x 1.5. Leaves 24-31 lie on the floor. Each drawn
// with light 0 (T, Rx, Ry, Rz, S), the glow pass with bit 26
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop023_kokonoha_leaves(uint32_t obj)
{
    const int32_t t = of<int32_t>(obj, 0x50);
    of<int32_t>(obj, 0x50) = t + 1;
    if (t > 0x167) {
        task_exit();
        return;
    }
    for (uint32_t i = 0; i < 32; ++i) {
        const uint32_t r = 0x53E4C0 + i * 24;
        if (i <= 23) {
            of<uint32_t>(obj, 0x38) = mem.u32(r);
            of<uint32_t>(obj, 0x3C) = mem.u32(r + 4);
            of<uint32_t>(obj, 0x40) = mem.u32(r + 8);
            const uint32_t rnd = rand();
            const uint32_t ph = 0x53E7C0 + i * 4;
            mem.set32(ph, mem.u32(ph) + 0x400 + (rnd & 0x3FF));
            mem.set32(r + 0x10, mem.u32(r + 0x10) + 0x300);
            const float s = ffrom(cam_copro(0x1B, {mem.u32(ph)}).w[0]);
            mem.at<float>(r) = s + mem.f32(r);
            const float c = ffrom(cam_copro(0x1C, {mem.u32(ph)}).w[0]);
            mem.at<float>(r + 8) = (c + 0.5f) + mem.f32(r + 8);
            const float s2 = ffrom(cam_copro(0x1B, {mem.u32(r + 0x10)}).w[0]);
            const int32_t rz = int32_t(s2 * ffrom(0x43B60B61u));
            mem.at<float>(r + 4) = s2 / 2.0f + mem.f32(r + 4);
            mem.set32(r + 0x14, uint32_t(rz));
            const float az = std::fabs(mem.f32(r + 8));
            if (az < 12.5f)
                of<uint32_t>(obj, 0x58) |= 0x4000000;
            else
                of<uint32_t>(obj, 0x58) &= ~0x4000000u;
            const float dx = mem.f32(r) - of<float>(obj, 0x38);
            const float dy = mem.f32(r + 4) - of<float>(obj, 0x3C);
            const float dz = mem.f32(r + 8) - of<float>(obj, 0x40);
            const Tgp::Out o = cam_copro(0x0F, {fbits(dx), fbits(dy), fbits(dz)});
            uint32_t ax = o.w[1];
            const uint32_t v = ax & 0xFFFF;
            if (((v ^ (v << 1)) >> 15) & 1)
                ax = 0x8000 - ax;
            mem.set32(r + 0xC, uint32_t(int32_t(double(float(int32_t(ax))) * 1.5)));
        } else {
            of<uint32_t>(obj, 0x58) &= ~0x4000000u;
        }
        prop_light0();
        cam_copro(0x05, {});
        cam_copro(0x12, {mem.u32(r), mem.u32(r + 4), mem.u32(r + 8)});
        cam_copro(0x14, {mem.u32(r + 0xC)});
        cam_copro(0x15, {mem.u32(r + 0x10)});
        cam_copro(0x16, {mem.u32(r + 0x14)});
        prop_scale(of<uint32_t>(obj, 0x94));
        geo_draw_model(of<uint32_t>(obj, 0x54));
        if ((of<uint32_t>(obj, 0x58) >> 26) & 1)
            prop_glow(of<uint32_t>(obj, 0x54));
        cam_copro(0x06, {});
    }
}

// 0x3C4A0 prop014_jinmengyo_fish (sub-type 14, Chapter 2 event 4): swims a closed quadratic B-spline through the 4
// points at 0xAE800 (12 bytes; +0x2F8 the segment, +0x30C t, 0.01 a frame), turned along its path (coprocessor 0x0F
// of the step); visible and not stacked: the 40 model frames 0x1352.. and drawn
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop014_jinmengyo_fish(uint32_t obj)
{
    const float t = of<float>(obj, 0x30C);
    const uint32_t seg = of<uint32_t>(obj, 0x2F8);
    prop_schedule_update(obj);
    float b1 = (-t + 1.0f) * t;
    const float omt = 1.0f - t;
    float b2 = t * t;
    of<uint32_t>(obj, 0x38) = of<uint32_t>(obj, 0x20);
    of<uint32_t>(obj, 0x3C) = of<uint32_t>(obj, 0x24);
    of<uint32_t>(obj, 0x40) = of<uint32_t>(obj, 0x28);
    const uint32_t p0 = 0xAE800 + seg * 12, p1 = 0xAE80C + seg * 12, p2 = 0xAE818 + seg * 12;
    float b0 = omt * 0.5f;
    b2 = b2 * 0.5f;
    b1 = b1 + 0.5f;
    b0 = b0 * omt;
    of<float>(obj, 0x20) = (b0 * mem.f32(p0) + b1 * mem.f32(p1)) + b2 * mem.f32(p2);
    of<float>(obj, 0x24) = (b0 * mem.f32(p0 + 4) + b1 * mem.f32(p1 + 4)) + b2 * mem.f32(p2 + 4);
    const float z = (b0 * mem.f32(p0 + 8) + b1 * mem.f32(p1 + 8)) + b2 * mem.f32(p2 + 8);
    float nt = of<float>(obj, 0x30C) + ffrom(0x3C23D70Au);
    if (nt > 1.0f) {
        const uint32_t s = of<uint32_t>(obj, 0x2F8) + 1;
        of<uint32_t>(obj, 0x2F8) = s;
        nt = 0.0f;
        if (int32_t(s) > 3)
            of<uint32_t>(obj, 0x2F8) = 0;
    }
    const float dx = of<float>(obj, 0x20) - of<float>(obj, 0x38);
    const float dy = of<float>(obj, 0x24) - of<float>(obj, 0x3C);
    of<float>(obj, 0x30C) = nt;
    of<float>(obj, 0x28) = z;
    const float dz = z - of<float>(obj, 0x40);
    const Tgp::Out o = cam_copro(0x0F, {fbits(dx), fbits(dy), fbits(dz)});
    uint32_t a0 = o.w[0], a1 = o.w[1];
    const uint32_t v = a1 & 0xFFFF;
    if (((v ^ (v << 1)) >> 15) & 1) {
        a1 = 0x8000 - a1;
        a0 = a0 + 0x8000;
    }
    if (of<float>(obj, 0x30C) != ffrom(0x3C23D70Au)) {
        of<uint32_t>(obj, 0x2C) = a1;
        of<uint32_t>(obj, 0x30) = a0;
    }
    if (of<uint32_t>(obj, 0x2F0) != 0 && of<uint8_t>(obj, 0x2E6) == 0) {
        const int32_t f = of<int32_t>(obj, 0x50);
        of<int32_t>(obj, 0x50) = f + 1;
        of<uint32_t>(obj, 0x54) = uint32_t(f % 40 + 0x1352);
        prop_draw(obj);
    }
}

// 0x3E9B0 prop078_r2_door (sub-types 78 / 79, the double door r2do_01a / 01b): in event 1, flag 0x51E94E throws both
// halves open (sound 0xA01221, prop_door_fly; 0x51A to one side, the other to the other), drawn; elsewhere the
// prop's schedule and door_n4a (0x165) drawn at its pose, scale (0.9, 1.4, 1)
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop078_r2_door(uint32_t obj)
{
    if (mem.at<uint8_t>(G::event_index) == 1) {
        if (mem.at<uint8_t>(0x51E94E) != 0) {
            if (of<uint32_t>(obj, 0x54) == 0x51A) {
                of<uint32_t>(obj, 0x80) = 0x3E4CCCCDu;
                of<uint32_t>(obj, 0x8C) = 0x3D8F5C29u;
                of<uint32_t>(obj, 0x2EC) = 0x180;
                of<uint32_t>(obj, 0x2F0) = 0x100;
            } else {
                of<uint32_t>(obj, 0x80) = 0xBE99999Au;
                of<uint32_t>(obj, 0x8C) = 0xBD4CCCCDu;
                of<uint32_t>(obj, 0x2EC) = 0xFFFFFDC0u;
                of<uint32_t>(obj, 0x2F0) = 0xFFFFFF80u;
            }
            sound_cue(0xA01221);
            of<uint32_t>(obj, 0x7C) = 0x3DCCCCCDu;
            of<uint32_t>(obj, 0x78) = 0xBE19999Au;
            of<uint32_t>(obj, 0) = F::prop_door_fly;
        }
        prop_draw(obj);
        return;
    }
    prop_schedule_update(obj);
    prop_light0();
    cam_copro(0x05, {});
    prop_place_xyz(obj);
    cam_copro(0x13, {0x3F666666u, 0x3FB33333u, 0x3F800000u});
    geo_draw_model(0x165);
    cam_copro(0x06, {});
}

// 0x3EB90 prop_door_fly: a door half flying off (velocity; x slows 0.02, y falls 0.025, z accelerates by +0x8C; the
// y / z spins +0x2EC / +0x2F0 grow by a tenth a frame), drawn every other frame, ends below y -35
// UNVERIFIED (teaser)
template <class P> void Game<P>::prop_door_fly(uint32_t obj)
{
    if (of<float>(obj, 0x24) < -35.0f) {
        task_exit();
        return;
    }
    const float vx = of<float>(obj, 0x78), vy = of<float>(obj, 0x7C), vz = of<float>(obj, 0x80);
    const int32_t s1 = of<int32_t>(obj, 0x2EC), s2 = of<int32_t>(obj, 0x2F0);
    of<float>(obj, 0x24) = of<float>(obj, 0x24) + vy;
    of<int32_t>(obj, 0x30) = of<int32_t>(obj, 0x30) + s1;
    of<float>(obj, 0x28) = of<float>(obj, 0x28) + vz;
    of<float>(obj, 0x20) = of<float>(obj, 0x20) + vx;
    of<float>(obj, 0x78) = vx - ffrom(0x3CA3D70Au);
    of<float>(obj, 0x80) = vz + of<float>(obj, 0x8C);
    of<int32_t>(obj, 0x34) = of<int32_t>(obj, 0x34) + s2;
    of<float>(obj, 0x7C) = vy - ffrom(0x3CCCCCCDu);
    const int32_t t = of<int32_t>(obj, 0x50) + 1;
    of<int32_t>(obj, 0x50) = t;
    of<int32_t>(obj, 0x2EC) = s1 / 10 + s1;
    of<int32_t>(obj, 0x2F0) = s2 / 10 + s2;
    if (t % 2 != 0)
        prop_draw(obj);
}

// 0x3ED70 prop084_niku (sub-type 84, Chapter 2 event 12's meat): shot while at rest (0x11B1): its sound, the spurt,
// then niku_002..niku_120 once; drawn, shootable
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop084_niku(uint32_t obj)
{
    prop_schedule_update(obj);
    if ((of<uint8_t>(obj, 0x58) & 6) && of<uint32_t>(obj, 0x54) == 0x11B1) {
        sound_cue(mem.u32(p2::rec(of<int16_t>(obj, 0xCC)) + 0x64));
        prop_spurt_spawn(obj);
        of<uint32_t>(obj, 0x54) = 0x11B2;
    }
    uint32_t m = of<uint32_t>(obj, 0x54);
    if (m != 0x11B1) {
        m += 1;
        if (m == 0x1229)
            m = 0x11B1;
    }
    of<uint32_t>(obj, 0x54) = m;
    prop_draw(obj);
    prop_shootable(obj);
}

// 0x3EE00 prop083_kyochuu (sub-type 83, the giant insect): shot: its sound, the spurt, falls (prop083_kyochuu_fall,
// vy 0.5); wriggles: the model steps by +0x2F8 between 0x11A9 and 0x11AB, holding +0x338 frames at each end (rand
// 3..23 at 0x11A9, 5 at 0x11AB); drawn, shootable
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop083_kyochuu(uint32_t obj)
{
    prop_schedule_update(obj);
    if (of<uint8_t>(obj, 0x58) & 6) {
        sound_cue(mem.u32(p2::rec(of<int16_t>(obj, 0xCC)) + 0x64));
        prop_spurt_spawn(obj);
        of<uint32_t>(obj, 0x7C) = 0x3F000000u;
        of<uint32_t>(obj, 0) = F::prop083_kyochuu_fall;
    }
    const uint32_t m = of<uint32_t>(obj, 0x54);
    bool step = true;
    if (m == 0x11A9 || m == 0x11AB) {
        const int32_t t = of<int32_t>(obj, 0x50);
        of<int32_t>(obj, 0x50) = t + 1;
        if (t < of<int32_t>(obj, 0x338)) {
            step = false;
        } else {
            of<int32_t>(obj, 0x50) = 0;
            of<int32_t>(obj, 0x2F8) = 0 - of<int32_t>(obj, 0x2F8);
            if (m == 0x11A9)
                of<int32_t>(obj, 0x338) = int32_t(rand()) % 21 + 3;
            else
                of<int32_t>(obj, 0x338) = 5;
        }
    }
    if (step)
        of<uint32_t>(obj, 0x54) = of<uint32_t>(obj, 0x54) + of<uint32_t>(obj, 0x2F8);
    prop_draw(obj);
    prop_shootable(obj);
}

// 0x3EEE0 prop083_kyochuu_fall: falls (vy -0.04 a frame) drifting (x -0.2, z +0.2), turning, until below the floor
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop083_kyochuu_fall(uint32_t obj)
{
    if (of<float>(obj, 0x24) < mem.f32(G::floor_y)) {
        task_exit();
        return;
    }
    const float vy = of<float>(obj, 0x7C) - ffrom(0x3D23D70Au);
    of<uint32_t>(obj, 0x2C) = of<uint32_t>(obj, 0x2C) - 0x300;
    of<float>(obj, 0x20) = of<float>(obj, 0x20) - ffrom(0x3E4CCCCDu);
    of<float>(obj, 0x28) = of<float>(obj, 0x28) + ffrom(0x3E4CCCCDu);
    of<float>(obj, 0x7C) = vy;
    of<float>(obj, 0x24) = of<float>(obj, 0x24) + vy;
    prop_draw(obj);
}

// 0x3E880 prop077_r2_wall (sub-type 77, Chapter 2 event 12's wall): after camera frame 458 the damaged model (0x1237)
// and its pieces; ends after frame 723; drawn
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop077_r2_wall(uint32_t obj)
{
    if (mem.f32(G::camera_frame) > 458.0f && of<uint32_t>(obj, 0x54) == 0x1236) {
        of<uint32_t>(obj, 0x54) = 0x1237;
        prop077_wall_pieces(obj);
    }
    if (mem.f32(G::camera_frame) > 723.0f) {
        task_exit();
        return;
    }
    prop_draw(obj);
}

// 0x3AD60 prop077_wall_pieces: twelve pieces (prop077_wall_piece) from table 0xAF780 (28 bytes: model, offset from
// (405.706, -82.154, 33.678), angles), random fall / drift / spins; sound 0xA01235
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop077_wall_pieces(uint32_t obj)
{
    for (uint32_t i = 0; i <= 11; ++i) {
        const uint32_t t = task_create(F::prop077_wall_piece, 0x3C8);
        task_clear_work(t);
        of<uint32_t>(t, 0x58) |= 0x1401;
        of<uint16_t>(t, 0xCC) = of<uint16_t>(obj, 0xCC);
        of<uint16_t>(t, 0x70) = of<uint16_t>(obj, 0x70);
        const uint32_t e = 0xAF780 + i * 28;
        of<float>(t, 0x20) = mem.f32(e + 4) + ffrom(0x43CADA5Eu);
        of<float>(t, 0x24) = mem.f32(e + 8) - ffrom(0x42A44ED9u);
        of<float>(t, 0x28) = mem.f32(e + 12) + ffrom(0x4206B646u);
        of<uint32_t>(t, 0x54) = mem.u32(e);
        of<uint32_t>(t, 0x2C) = mem.u32(e + 16);
        of<uint32_t>(t, 0x30) = mem.u32(e + 20);
        of<uint32_t>(t, 0x7C) = 0;
        of<uint32_t>(t, 0x34) = mem.u32(e + 24);
        of<float>(t, 0x88) = ffrom(0xBBE56042u) - pdiv(float(int32_t(rand()) % 20), 1000.0f);
        of<float>(t, 0x80) = -(pdiv(float(int32_t(rand()) % 300), 1000.0f));
        of<float>(t, 0x8C) = ffrom(0xBCF5C28Fu) - pdiv(float(int32_t(rand()) % 100), 1000.0f);
        of<uint32_t>(t, 0x31C) = (rand() & 0x7FF) - 0x400;
        of<uint32_t>(t, 0x320) = (rand() & 0x7FF) - 0x400;
        of<uint16_t>(t, 0x6A) = 0;
        of<uint32_t>(t, 0x94) = 0x3F800000u;
    }
    sound_cue(0xA01235);
}

// 0x3E8F0 prop077_wall_piece: ends after camera frame 700; above the floor (+0.2): moves (z by vy, the original's
// own), gravity +0x88 on vy and vz, spins +0x31C / +0x320; on it: lies flat (x angle 0x4000); drawn
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop077_wall_piece(uint32_t obj)
{
    if (mem.f32(G::camera_frame) > 700.0f) {
        task_exit();
        return;
    }
    const float rest = mem.f32(G::floor_y) + ffrom(0x3E4CCCCDu);
    float y = of<float>(obj, 0x24);
    uint32_t ax = of<uint32_t>(obj, 0x2C);
    if (y > rest) {
        const float vz = of<float>(obj, 0x80), ay = of<float>(obj, 0x88), vy = of<float>(obj, 0x7C);
        const float z = of<float>(obj, 0x28) + vy;
        of<uint32_t>(obj, 0x34) = of<uint32_t>(obj, 0x34) + of<uint32_t>(obj, 0x320);
        ax += of<uint32_t>(obj, 0x31C);
        of<float>(obj, 0x80) = vz + ay;
        of<float>(obj, 0x28) = z;
        const float nvy = vy + ay;
        of<float>(obj, 0x7C) = nvy;
        y = y + nvy;
    } else {
        ax = 0x4000;
        y = rest;
    }
    of<uint32_t>(obj, 0x2C) = ax;
    of<float>(obj, 0x24) = y;
    prop_draw(obj);
}

// 0x3EC90 prop080_statue_piece (sub-type 80, kind 151's fragments): ends below y -130; shot: the impact, sound
// 0xA01259, knocked up (vy 0.3, vz 0.4); falls and spins; drawn with fade 0x96, shootable
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop080_statue_piece(uint32_t obj)
{
    if (of<float>(obj, 0x24) < -130.0f) {
        task_exit();
        return;
    }
    if (of<uint8_t>(obj, 0x58) & 6) {
        prop_bullet_impact(obj);
        sound_cue(0xA01259);
        of<uint32_t>(obj, 0x7C) = 0x3E99999Au;
        of<uint32_t>(obj, 0x80) = 0x3ECCCCCDu;
    }
    const float vy = of<float>(obj, 0x7C) + of<float>(obj, 0x88);
    const float z = of<float>(obj, 0x28) + of<float>(obj, 0x80);
    const float y = of<float>(obj, 0x24) + vy;
    of<uint32_t>(obj, 0x2C) = of<uint32_t>(obj, 0x2C) + of<uint32_t>(obj, 0x31C);
    of<uint32_t>(obj, 0x34) = of<uint32_t>(obj, 0x34) + of<uint32_t>(obj, 0x320);
    of<float>(obj, 0x7C) = vy;
    of<float>(obj, 0x28) = z;
    of<float>(obj, 0x24) = y;
    draw_fade_begin(0x96);
    prop_draw(obj);
    geo_send_words(0, 31, 0x7A0);
    prop_shootable(obj);
}

// 0x3BC40 prop085_fruit_scheduled (sub-type 85 and kind 152's props): prop041_tabul_scheduled's frame with the
// not-shootable window at camera frames 0..40 (exclusive); shot: 76 / 85 plate shards, 73 tumble copy and shards,
// else the break animation; drawn (light 0, T R S), sub-type 73 also its plate 0x1241 at twice the scale
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop085_fruit_scheduled(uint32_t obj)
{
    const float frame = mem.f32(G::camera_frame);
    if (frame > 0.0f && !(frame >= 40.0f))
        of<uint32_t>(obj, 0x58) |= 0x40000000;
    else if (of<uint16_t>(obj, 0xCC) != 4)
        of<uint32_t>(obj, 0x58) &= ~0x40000000u;
    const int32_t scene = mem.at<int8_t>(G::scene_number);
    if (scene != of<int32_t>(obj, 0x2EC)) {
        of<int32_t>(obj, 0x2EC) = scene;
        const int32_t n = of<int32_t>(obj, 0x330) + 1;
        of<int32_t>(obj, 0x330) = n;
        if (n > of<int16_t>(obj, 0x70)) {
            task_exit();
            return;
        }
    }
    const int32_t s = of<int16_t>(obj, 0x64);
    uint32_t pos = of<uint32_t>(obj, 0x2F4);
    const float at = float(int32_t(mem.at<int16_t>(0xAE840 + uint32_t(s << 4) + 2 * pos)));
    int32_t vis = of<int32_t>(obj, 0x2F0);
    if (at == mem.f32(G::camera_frame)) {
        vis = 1 - vis;
        pos += 1;
    }
    of<uint32_t>(obj, 0x2F4) = pos;
    of<int32_t>(obj, 0x2F0) = vis;
    if (vis == 0 && mem.at<uint16_t>(0xAE840 + ((uint32_t(s) << 16) >> 12) + 2 * of<uint32_t>(obj, 0x2F4)) == 0xFFFF) {
        task_exit();
        return;
    }
    if (of<uint32_t>(obj, 0x2F0) == 0 || of<uint8_t>(obj, 0x2E6) != 0)
        return;
    const uint32_t f = of<uint32_t>(obj, 0x58);
    if (!((f >> 30) & 1) && (f & 6)) {
        const uint32_t r = p2::rec(of<int16_t>(obj, 0xCC));
        if (mem.u32(r + 0x24 + 4 * of<uint32_t>(obj, 0x32C)) != 0xFFFFFFFFu) {
            sound_cue(mem.u32(r + 0x64));
            const float y = of<float>(obj, 0x24) + mem.f32(r + 0x50 + 4 * of<uint32_t>(obj, 0x32C));
            prop_view_pos(obj, of<uint32_t>(obj, 0x20), fbits(y), of<uint32_t>(obj, 0x28));
            prop_bullet_impact(obj);
            const int16_t type = of<int16_t>(obj, 0xCC);
            if (type == 0x4C || type == 0x55) {
                prop_plate_shards(obj);
            } else if (type == 0x49) {
                prop_tumble_copy(obj);
                prop_plate_shards(obj);
            } else {
                prop_break_anim_spawn(obj);
            }
            task_exit();
            return;
        }
    }
    prop_light0();
    cam_copro(0x05, {});
    prop_place_xyz(obj);
    prop_scale(of<uint32_t>(obj, 0x94));
    geo_draw_model(of<uint32_t>(obj, 0x54));
    if (of<uint16_t>(obj, 0xCC) == 0x49) {
        prop_scale(0x40000000u);
        geo_draw_model(0x1241);
    }
    cam_copro(0x06, {});
    if (!((of<uint32_t>(obj, 0x58) >> 30) & 1))
        prop_shootable(obj);
}

// 0x3E800 prop008_bookshelf_topple (sub-type 8 in Chapter 2 events 13 / 14, set by prop_object): from scene 5 counts
// 130 frames, then its sound, the books fly (prop_bookshelf_collapse, prop_bookshelf_remains) and it ends; drawn
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop008_bookshelf_topple(uint32_t obj)
{
    int32_t t = of<int32_t>(obj, 0x50);
    if (mem.at<uint8_t>(G::scene_number) == 5 && t == 0)
        t = 1;
    of<int32_t>(obj, 0x50) = t;
    if (t > 0) {
        const int32_t n = of<int32_t>(obj, 0x50);
        of<int32_t>(obj, 0x50) = n + 1;
        if (n > 0x81) {
            sound_cue(mem.u32(p2::rec(of<int16_t>(obj, 0xCC)) + 0x64));
            prop_bookshelf_collapse(obj);
            prop_bookshelf_remains(obj);
            task_exit();
            return;
        }
    }
    prop_draw(obj);
}

// 0x42140 prop_hit_drop: a shot prop drops a loose part (prop_hit_drop_piece, madodoco00a 0x3BD frames) at its pose,
// placed per sub-type: 13 z -1, half scale; 51 turned, 3 lower, 1.8; 64 turned, 0.6 lower, 0.3; 86 turned, 5 higher,
// 1.5; 94 at (-32, -12.5, 175) turned, 1.5; others not placed
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop_hit_drop(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;   // the PC form: not ported (vtable only; ProofPc replays it)
        return;
    }
    const uint32_t t = task_create(F::prop_hit_drop_piece, 0x3C8);
    obj_copy_pose(t, obj);
    of<uint32_t>(t, 0x54) = 0x3BD;
    of<uint32_t>(t, 0x50) = 0;
    of<uint16_t>(t, 0x6A) = 0;
    of<uint16_t>(t, 0xCC) = of<uint16_t>(obj, 0xCC);
    const int16_t type = of<int16_t>(obj, 0xCC);
    const float s = of<float>(obj, 0x94);
    switch (type) {
    case 0x40:
        of<float>(t, 0x24) = of<float>(t, 0x24) - ffrom(0x3F19999Au);
        of<uint32_t>(t, 0x30) += 0x8000;
        of<float>(t, 0x94) = s * ffrom(0x3E99999Au);
        break;
    case 13:
        of<float>(t, 0x28) = of<float>(t, 0x28) - 1.0f;
        of<float>(t, 0x94) = s * 0.5f;
        break;
    case 51:
        of<float>(t, 0x24) = of<float>(t, 0x24) - 3.0f;
        of<uint32_t>(t, 0x30) += 0x8000;
        of<float>(t, 0x94) = s * ffrom(0x3FE66666u);
        break;
    case 0x56:
        of<float>(t, 0x24) = of<float>(t, 0x24) + 5.0f;
        of<uint32_t>(t, 0x30) += 0x8000;
        of<float>(t, 0x94) = s * 1.5f;
        break;
    case 0x5E:
        of<uint32_t>(t, 0x20) = 0xC2000000u;
        of<uint32_t>(t, 0x24) = 0xC1480000u;
        of<uint32_t>(t, 0x28) = 0x432F0000u;
        of<uint32_t>(t, 0x30) = 0x8000;
        of<uint32_t>(t, 0x94) = 0x3FC00000u;
        break;
    default: break;
    }
}

// 0x41FF0 prop_hit_drop_piece: its model frames up to 0x3F3 at its pose (T, Rx, Ry, Rz), scaled (sub-type 94: 1.75
// wide, else 1.5 high); then the end
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop_hit_drop_piece(uint32_t obj)
{
    const uint32_t m = of<uint32_t>(obj, 0x54);
    of<uint32_t>(obj, 0x54) = m + 1;
    if (int32_t(m) > 0x3F2) {
        task_exit();
        return;
    }
    cam_copro(0x05, {});
    prop_place_xyz(obj);
    const float s = of<float>(obj, 0x94);
    if (of<int16_t>(obj, 0xCC) == 0x5E)
        cam_copro(0x13, {fbits(s * 1.75f), fbits(s), fbits(s)});
    else
        cam_copro(0x13, {fbits(s), fbits(s * 1.5f), fbits(s)});
    geo_draw_model(of<uint32_t>(obj, 0x54));
    cam_copro(0x06, {});
}

// 0x422B0 prop_lid_fly: a copy of the prop (pose, flags, model, scale) that falls spinning (prop_fall_spin)
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop_lid_fly(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;   // the PC form: not ported (vtable only; ProofPc replays it)
        return;
    }
    const uint32_t t = task_create(F::prop_fall_spin, 0x3C8);
    obj_copy_pose(t, obj);
    of<uint32_t>(t, 0x58) = of<uint32_t>(obj, 0x58);
    of<uint32_t>(t, 0x7C) = 0;
    of<uint32_t>(t, 0x54) = of<uint32_t>(obj, 0x54);
    of<uint32_t>(t, 0x50) = 0;
    of<uint16_t>(t, 0x6A) = 0;
    of<uint32_t>(t, 0x94) = of<uint32_t>(obj, 0x94);
}

// 0x42EC0 prop_fall_spin: falls (vy -0.03 a frame) turning (+0x34 += 0x300) for 121 frames; drawn
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop_fall_spin(uint32_t obj)
{
    const int32_t t = of<int32_t>(obj, 0x50);
    of<int32_t>(obj, 0x50) = t + 1;
    if (t > 0x78) {
        task_exit();
        return;
    }
    const float vy = of<float>(obj, 0x7C) - ffrom(0x3CF5C28Fu);
    const float y = of<float>(obj, 0x24) + vy;
    of<uint32_t>(obj, 0x34) = of<uint32_t>(obj, 0x34) + 0x300;
    of<float>(obj, 0x7C) = vy;
    of<float>(obj, 0x24) = y;
    prop_draw(obj);
}

// --- the bookshelves (sub-types 8 and 43, Chapters 1 / 2; worker "ch2play"), read from the disassembly ---

// 0x40BB0 prop_bookshelf_collapse: 18 prop_book_fly tasks: 12 books (table 0xAEF00, 7 words each: model, offset x / y /
// z, angles x / y / z; Chapter 1 swaps the book_tanab models 0x12BE / 0x12BF / 0x12C1 for 0xB6B / 0xB68 / 0xB6A), 6 paper
// scraps (kamikuzu 0x137A). Each piece's push (vx -= sin / 6.5, vz -= cos / 6.5) uses the heading computed at the
// end of the piece before it (the first one 0), as the original does.
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop_bookshelf_collapse(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;   // the PC form: not ported (vtable only; ProofPc replays it)
        return;
    }
    uint32_t heading = 0;
    for (uint32_t i = 0; i <= 17; ++i) {
        const uint32_t t = task_create(F::prop_book_fly, 0x3C8);
        task_clear_work(t);
        prop_piece_random(t, obj);
        int32_t a;
        if (i < 12) {
            const uint32_t e = 0xAEF00 + i * 28;
            const uint32_t m = mem.u32(e);
            of<uint32_t>(t, 0x54) = m;
            if (mem.at<uint8_t>(G::chapter) == 0) {
                if (m == 0x12BF)
                    of<uint32_t>(t, 0x54) = 0xB68;
                else if (m == 0x12C1)
                    of<uint32_t>(t, 0x54) = 0xB6A;
                else if (m == 0x12BE)
                    of<uint32_t>(t, 0x54) = 0xB6B;
            }
            of<uint32_t>(t, 0x58) |= 0x4000000;
            of<float>(t, 0x20) = of<float>(obj, 0x20) + mem.f32(e + 4);
            of<float>(t, 0x24) = (of<float>(obj, 0x24) + mem.f32(e + 8)) + 17.0f;
            of<float>(t, 0x28) = of<float>(obj, 0x28) + mem.f32(e + 12);
            of<uint32_t>(t, 0x2C) = of<uint32_t>(obj, 0x2C) + mem.u32(e + 16);
            of<uint32_t>(t, 0x30) = of<uint32_t>(obj, 0x30) + mem.u32(e + 20);
            of<uint32_t>(t, 0x34) = of<uint32_t>(obj, 0x34) + mem.u32(e + 24);
            const int32_t axis = int32_t(rand()) % 3;
            const int32_t spin = int32_t(rand()) % 0x8FF;
            const int32_t r = int32_t(rand());
            of<int32_t>(t, 0x31C + uint32_t(axis) * 4) = (r % 2 != 0) ? spin : -spin;
            const float s = ffrom(cam_copro(0x1B, {heading}).w[0]);
            of<float>(t, 0x78) = of<float>(t, 0x78) - s / 6.5f;
            of<float>(t, 0x7C) = pdiv(float(int32_t(rand()) % 10), 100.0f) + ffrom(0x3D4CCCCDu);
            of<float>(t, 0x88) = ffrom(0xBCA3D70Au) - pdiv(float(int32_t(rand()) % 10), 1000.0f);
            const float c = ffrom(cam_copro(0x1C, {heading}).w[0]);
            of<float>(t, 0x80) = of<float>(t, 0x80) - c / 6.5f;
            const uint32_t model = of<uint32_t>(t, 0x54);
            if (model == 0x12C1 || model == 0xB6A)
                of<float>(t, 0x94) = of<float>(obj, 0x94) * ffrom(0x3F4CCCCDu);
            a = int32_t(i) * 15 + int32_t(rand()) % 9 - 4;
        } else {
            of<float>(t, 0x20) = of<float>(obj, 0x20) + pdiv(float(int32_t(rand()) % 41 - 20), 10.0f);
            of<float>(t, 0x24) = of<float>(obj, 0x24) + 17.0f;
            of<uint32_t>(t, 0x2C) = 0;
            of<float>(t, 0x28) = of<float>(obj, 0x28) - 3.0f;
            const uint32_t r = rand();
            of<uint32_t>(t, 0x54) = 0x137A;
            of<uint32_t>(t, 0x34) = 0;
            of<uint32_t>(t, 0x7C) = 0x3E4CCCCDu;
            of<uint32_t>(t, 0x30) = r & 0xFFFF;
            of<float>(t, 0x94) = of<float>(obj, 0x94) * 3.0f;
            a = int32_t(rand()) % 9 + (18 - int32_t(i)) * 30 - 4;
        }
        const int32_t deg = int32_t(float(a) * ffrom(0x43360B61u));
        heading = of<uint32_t>(obj, 0x30) - uint32_t(deg - 0x4000);
        if (of<int32_t>(obj, 0x32C) == 1)
            of<float>(t, 0x24) = of<float>(t, 0x24) - 5.5f;
        of<uint32_t>(t, 0x354) = 0x3F4CCCCDu;
        of<uint32_t>(t, 0x358) = 0x3F19999Au;
    }
}

// 0x40F50 prop_bookshelf_remains: the emptied shelf (book_tanab_dam01a 0xB69; Chapter 2 book_tana_dam01a 0x12C0) at
// the shelf's pose and scale, blinking (prop_blink_remains)
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop_bookshelf_remains(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;   // the PC form: not ported (vtable only; ProofPc replays it)
        return;
    }
    const uint32_t t = task_create(F::prop_blink_remains, 0x3C8);
    task_clear_work(t);
    obj_copy_pose(t, obj);
    of<uint32_t>(t, 0x54) = 0xB69;
    if (mem.at<uint8_t>(G::chapter) == 1)
        of<uint32_t>(t, 0x54) = 0xB69 + 0x757;
    of<uint32_t>(t, 0x50) = 0;
    of<uint16_t>(t, 0x6A) = 0;
    of<uint32_t>(t, 0x94) = of<uint32_t>(obj, 0x94);
}

// 0x40980 prop_book_fly: a book flies 90 frames (pos += vel; it bounces once below the floor + 0.8, or below -68.4 for
// sub-type 59: vy = the bounce, bounce - 0.1; spins; vx / vz grow by 1/150), a paper scrap 180 (above the floor it
// flutters: a first push (vx / vz random, vy -0.4) when still rising faster than 0.1, then vy + 0.03, turning); after
// frame 59 drawn every other frame
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop_book_fly(uint32_t obj)
{
    const int32_t t0 = of<int32_t>(obj, 0x50);
    of<int32_t>(obj, 0x50) = t0 + 1;
    if ((t0 > 0x5A && of<uint32_t>(obj, 0x54) != 0x137A) || of<int32_t>(obj, 0x50) > 0xB4) {
        task_exit();
        return;
    }
    const float vy0 = of<float>(obj, 0x7C);
    of<float>(obj, 0x20) = of<float>(obj, 0x20) + of<float>(obj, 0x78);
    of<float>(obj, 0x24) = of<float>(obj, 0x24) + vy0;
    of<float>(obj, 0x28) = of<float>(obj, 0x28) + of<float>(obj, 0x80);
    if (of<uint32_t>(obj, 0x54) == 0x137A) {
        if (!(of<float>(obj, 0x24) < mem.f32(G::floor_y))) {
            if (vy0 > ffrom(0x3DCCCCCDu)) {
                const float vx = float(int32_t(rand()) % 20 - 10) / 20.0f;
                of<uint32_t>(obj, 0x7C) = 0xBECCCCCDu;
                of<float>(obj, 0x78) = vx;
                const float vz = float(int32_t(rand()) % 20 - 10) / 20.0f;
                of<uint32_t>(obj, 0x34) = 0x1555;
                of<float>(obj, 0x80) = vz;
            }
            of<float>(obj, 0x7C) = of<float>(obj, 0x7C) + ffrom(0x3CF5C28Fu);
            of<uint32_t>(obj, 0x34) = of<uint32_t>(obj, 0x34) - 0x38E;
        }
    } else {
        const int16_t type = of<int16_t>(obj, 0xCC);
        float vy = of<float>(obj, 0x7C);
        const float y = of<float>(obj, 0x24);
        const bool bounce = type != 0x3B ? y <= mem.f32(G::floor_y) + of<float>(obj, 0x354) : !(y > ffrom(0xC288CCCDu));
        if (bounce) {
            vy = of<float>(obj, 0x358);
            of<float>(obj, 0x358) = vy - ffrom(0x3DCCCCCDu);
        }
        const float vx = of<float>(obj, 0x78), vz = of<float>(obj, 0x80);
        const float dx = vx / 150.0f, dz = vz / 150.0f;
        of<uint32_t>(obj, 0x2C) = of<uint32_t>(obj, 0x2C) + of<uint32_t>(obj, 0x31C);
        of<float>(obj, 0x7C) = vy + of<float>(obj, 0x88);
        of<uint32_t>(obj, 0x30) = of<uint32_t>(obj, 0x30) + of<uint32_t>(obj, 0x320);
        of<uint32_t>(obj, 0x34) = of<uint32_t>(obj, 0x34) + of<uint32_t>(obj, 0x324);
        of<float>(obj, 0x78) = dx + vx;
        of<float>(obj, 0x80) = dz + vz;
    }
    const int32_t t = of<int32_t>(obj, 0x50);
    if (t > 0x3B && t % 2 == 0)
        return;
    prop_draw(obj);
}

// 0x42E70 prop_blink_remains: drawn every other frame for 81 frames
// Proven on A (ch2proof, docs/teaser_status/ch2proof.md)
template <class P> void Game<P>::prop_blink_remains(uint32_t obj)
{
    const int32_t t0 = of<int32_t>(obj, 0x50);
    of<int32_t>(obj, 0x50) = t0 + 1;
    if (t0 > 0x50) {
        task_exit();
        return;
    }
    if (of<int32_t>(obj, 0x50) % 2 != 0)
        prop_draw(obj);
}

} // namespace hotd::port
