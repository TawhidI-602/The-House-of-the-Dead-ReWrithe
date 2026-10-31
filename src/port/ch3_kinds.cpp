// Chapter 3's own kinds (worker "ch3own", docs/chapter2_3_plan.md): the Taran 153, the padlock 106, the route objects
// 186 / 188 / 189 and the Chapter 3 parts of shared kinds. Read from the arcade decompilation / disassembly; arcade
// forms only. UNVERIFIED (teaser) unless marked proven below.
#include "port/game.h"

namespace hotd::port {

// --- the iron-ball zombie (kind 32 action 4, Chapter 3 event 0) ---

// 0x49DA0 iron_ball_in_reach: always 1 (8 bytes)
template <class P> uint32_t Game<P>::iron_ball_in_reach(uint32_t obj)
{
    (void)obj;
    return 1;
}

// --- the ball (iron_ball_task, kit_zombie, runs these) ---

namespace c3 {
constexpr uint32_t ball_vel_scale = 0xAFD2C;   // float: the thrown / pulled ball's speed
constexpr uint32_t ball_pull_turn = 0xAFD30;   // int: added to the owner's heading for the pull
constexpr uint32_t ball_pull_reach = 0xAFD34;  // float: pull distance from the start point
constexpr uint32_t ball_pull_slow = 0xAFD38;   // float: x / z speed factor once pulled out
constexpr uint32_t ball_spin_step = 0xAFD20;   // int: +0x31C step per frame (a hit box every 0x10000)
constexpr uint32_t ball_hitbox_life = 0xAFD24; // int: frames a hit box lives
constexpr uint32_t ball_spark_size = 0xAFD28;  // word: hit_spark_spawn size
constexpr uint32_t ball_models = 0xAFCF0;      // short per texture area
constexpr uint32_t hand_chain = 0xAFDA0;       // node chains (body_node_pos_at_frame)
constexpr uint32_t ball_chain = 0xAFDC0;
constexpr uint32_t drop_heading = 0x51EAC0;    // int angle (the drop's direction)
constexpr uint32_t ball_pulled = 0x51E980;     // byte, set when the ball is pulled out
} // namespace c3

// 0x47C50 iron_ball_pos_idle (ball, mode, step): a new mode (extra +8): the step restarts at `step`
template <class P> void Game<P>::iron_ball_pos_idle(uint32_t obj, int mode, int step)
{
    const uint32_t x = of<uint32_t>(obj, 0x37C);
    if (mem.at<int16_t>(x + 8) != mode) {
        mem.at<int16_t>(x + 8) = int16_t(mode);
        of<int16_t>(obj, 0x66) = int16_t(step);
    }
}

// 0x47C70 iron_ball_follow: the owner's motion and frame (by its second-motion flags) into extra +2 / +4; the owner
// dead (life < 1: +0x320 bit 0) -> iron_ball_drop; else the ball's place by the owner's motion
template <class P> void Game<P>::iron_ball_follow(uint32_t obj)
{
    const uint32_t own = of<uint32_t>(obj, 0x18);
    const uint32_t rec = of<uint32_t>(obj, 0x368);
    const uint32_t x = of<uint32_t>(obj, 0x37C);
    const uint8_t f2 = of<uint8_t>(own, 0x3C6);
    uint32_t sel = (f2 >> 5) & 1;
    if ((f2 >> 1) & 1)
        sel |= 2;
    if (sel == 0) {
        mem.at<int16_t>(x + 2) = of<int16_t>(own, 0xD0);
        mem.at<int16_t>(x + 4) = of<int16_t>(own, 0xF4);
    } else if (sel == 1) {
        mem.at<int16_t>(x + 2) = of<int16_t>(own, 0xFE);
        mem.at<int16_t>(x + 4) = of<int16_t>(own, 0x108);
    } else if (sel == 2) {
        mem.at<int16_t>(x + 2) = of<int16_t>(own, 0xD2);
        mem.at<int16_t>(x + 4) = of<int16_t>(own, 0xF4);
    } else {
        mem.at<int16_t>(x + 2) = of<int16_t>(own, 0x100);
        mem.at<int16_t>(x + 4) = of<int16_t>(own, 0x108);
    }
    if (of<int16_t>(own, 0x70) < 1)
        of<uint32_t>(own, 0x320) = of<uint32_t>(own, 0x320) | 1;
    if (of<uint8_t>(own, 0x320) & 1) {
        of<int16_t>(obj, 0x66) = 0;
        of<uint32_t>(obj, 0) = F::iron_ball_drop;
        iron_ball_drop(obj);
        return;
    }
    switch (mem.at<int16_t>(x + 2)) {
    case 0x1D8: case 0x1DA: case 0x1EE: case 0x1F1: case 0x1F4: case 0x1F6: case 0x1F8: case 0x1F9:
        iron_ball_pos_idle(obj, 3, 0x60);
        iron_ball_pos_7(obj);
        return;
    case 0x276:
        iron_ball_pos_idle(obj, 0, 0);
        iron_ball_pos_swing(obj, 0x1C8);
        return;
    case 0x277:
        iron_ball_pos_idle(obj, 0, 0);
        iron_ball_pos_swing(obj, 0x1C9);
        return;
    case 0x278:
        iron_ball_pos_idle(obj, 2, 0x40);
        iron_ball_pos_6(obj);
        return;
    case 0x279:
        iron_ball_pos_idle(obj, 1, 0x20);
        if ((of<uint8_t>(own, 0x320) >> 1) & 1) {
            if (mem.at<int8_t>(rec + 1) == 3)
                iron_ball_pos_pull(obj);
            else
                iron_ball_pos_hit(obj);
            return;
        }
        iron_ball_pos_attack(obj);
        return;
    case 0x27A:
        iron_ball_pos_idle(obj, 0, 0);
        iron_ball_pos_swing(obj, 0x1CA);
        return;
    default:
        return;
    }
}

// 0x47E70 iron_ball_pos_swing (ball, motion): the ball at the owner's hand: the owner's matrix (+0x244), the nodes of
// the +0x35C chain posed by `motion` at the owner's frame (frame data 0x8C bytes each: joints at +0x30 / +0x36 /
// +0x3C), the point 1 ahead scaled by 1 / its distance; first frame: the chain's +0x2A4 / +0x2A6 cleared
template <class P> void Game<P>::iron_ball_pos_swing(uint32_t obj, uint32_t motion)
{
    const uint32_t x = of<uint32_t>(obj, 0x37C);
    const uint32_t n1 = of<uint32_t>(obj, 0x35C);
    const uint32_t n2 = mem.u32(n1 + 0x1C);
    const uint32_t n3 = mem.u32(n2 + 0x1C);
    const uint32_t fd = mem.u32(0xC7540 + motion * 4) + uint32_t(int32_t(mem.at<int16_t>(x + 4))) * 0x8C;
    const uint32_t own = of<uint32_t>(obj, 0x18);
    if (of<int16_t>(obj, 0x66) == 0) {
        const uint32_t ch = mem.u32(x + 0x10);
        mem.at<int16_t>(ch + 0x2A6) = 0;
        mem.at<int16_t>(ch + 0x2A4) = 0;
        of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
    }
    uint32_t m[12];
    for (uint32_t i = 0; i < 12; ++i)
        m[i] = of<uint32_t>(own, 0x244 + 4 * i);
    Tgp::Out none;
    copro(0x7F, m, 12, none);
    const uint32_t nodes[3] = {n1, n2, n3};
    const uint32_t joints[3] = {fd + 0x30, fd + 0x36, fd + 0x3C};
    for (int i = 0; i < 3; ++i) {
        cam_copro(0x12, {mem.u32(nodes[i] + 4), mem.u32(nodes[i] + 8), mem.u32(nodes[i] + 0xC) ^ 0x80000000u});
        cam_copro(0x86, {0u - uint32_t(mem.at<uint16_t>(joints[i] + 4)), uint32_t(mem.at<uint16_t>(joints[i] + 2)),
                         uint32_t(mem.at<uint16_t>(joints[i]))});
    }
    cam_copro(0x05, {});
    cam_copro(0x77, {1});
    cam_copro(0x05, {});
    cam_copro(0x88, {0, 0, 0});
    const Tgp::Out p = cam_copro(0x1A, {0, 0, fbits(1.0f)});
    cam_copro(0x06, {});
    const Tgp::Out d = cam_copro(0x1F, {p.w[0], p.w[1], p.w[2], 0, 0, 0});
    const uint32_t s = fbits(1.0f / ffrom(d.w[0]));
    cam_copro(0x13, {s, s, s});
    const Tgp::Out t = cam_copro(0x20, {});
    of<uint32_t>(obj, 0x20) = t.w[0];
    of<uint32_t>(obj, 0x24) = t.w[1];
    cam_copro(0x06, {});
    cam_copro(0x06, {});
    of<uint32_t>(obj, 0x28) = t.w[2];
    if (pc_path()) {
        // PC Mode / Boss Mode: THOTD.EXE 0x48D46C on its file's helpers (0x48A630): the owner's hand matrix (the PC's
        // own, kept by the part hook), the three nodes' offsets (x, y, -z) and turns z / y / x, times stored matrix 1;
        // the place is its translation (the 1 / distance scale only changes the drawing; pmrest2)
        pm::Matrix w = pm_copro_words_at(own + 0x244);
        for (int i = 0; i < 3; ++i)
            pm::draw1_part(w, mem.f32(nodes[i] + 4), mem.f32(nodes[i] + 8), mem.f32(nodes[i] + 0xC),
                           0u - uint32_t(mem.at<uint16_t>(joints[i] + 4)), uint32_t(mem.at<uint16_t>(joints[i] + 2)),
                           uint32_t(mem.at<uint16_t>(joints[i])));
        w.mul_stored(pm_camera);
        for (uint32_t k = 0; k < 3; ++k)
            of<float>(obj, 0x20 + 4 * k) = w.e[9 + k];
    }
    iron_ball_draw(obj);
}

// 0x48170 iron_ball_pos_attack: the throw. Step 0x20: aimed at a random point 10 ahead of the camera (stored matrix
// 6), speed 0xAFD2C, +0xA0 32768; then it flies; within 10 of the camera (stored matrix 5): clipped to depth 10,
// back in the world, projectile_hit_player, the owner told (+0x324 bit 0), step 0x3F
// Proven on A (vf_c3a: 150 captures, 0 differ)
template <class P> void Game<P>::iron_ball_pos_attack(uint32_t obj)
{
    LocalScope ls(*this);
    const uint32_t own = of<uint32_t>(obj, 0x18);
    const int16_t step = of<int16_t>(obj, 0x66);
    if (step == 0x20) {
        cam_copro(0x74, {6});
        const float k = ffrom(0x47770800u);
        const uint32_t rand_a = rand() & 0xFF;
        float a = float(int32_t(rand_a));
        a = (a - 127.5f) * 150.0f * 10.0f / k;
        const uint32_t rand_b = rand() & 0xFF;
        float b = float(int32_t(rand_b));
        b = (b - 127.5f) * 120.0f * 10.0f / k;
        const Tgp::Out q = cam_copro(0x1A, {fbits(a), fbits(b), fbits(10.0f)});
        const Tgp::Out v = cam_copro(0x0C, {fbits(ffrom(q.w[0]) - of<float>(obj, 0x20)),
                                            fbits(ffrom(q.w[1]) - of<float>(obj, 0x24)),
                                            fbits(ffrom(q.w[2]) - of<float>(obj, 0x28))});
        of<uint32_t>(obj, 0x78) = v.w[0];
        of<uint32_t>(obj, 0x7C) = v.w[1];
        of<uint32_t>(obj, 0x80) = v.w[2];
        if (pc_path()) {
            // PC Mode / Boss Mode: THOTD.EXE 0x48D771: the same two random numbers times 1 / 63240 (0x5A3154), the
            // point (a, b, 10) through stored matrix 6 and the file's helper 0x48A630, the direction times 1 / the
            // table square root (0x48AE6C; x87, 24 bits; pmch34)
            const float pa = ((float(int32_t(rand_a)) + -127.5f) * 150.0f) * 10.0f * ffrom(0x3784A5AEu);
            const float pb = ((float(int32_t(rand_b)) + -127.5f) * 120.0f) * 10.0f * ffrom(0x3784A5AEu);
            float pq[3];
            pm_m6().transform(pa, pb, 10.0f, pq[0], pq[1], pq[2]);
            const float dx = pq[0] - of<float>(obj, 0x20), dy = pq[1] - of<float>(obj, 0x24), dz = pq[2] - of<float>(obj, 0x28);
            const float r = 1.0f / pm_fast_sqrt((dx * dx + dy * dy) + dz * dz);
            of<float>(obj, 0x78) = dx * r;
            of<float>(obj, 0x7C) = dy * r;
            of<float>(obj, 0x80) = dz * r;
        }
        of<float>(obj, 0x78) = of<float>(obj, 0x78) * mem.f32(c3::ball_vel_scale);
        of<float>(obj, 0x7C) = of<float>(obj, 0x7C) * mem.f32(c3::ball_vel_scale);
        of<float>(obj, 0x80) = of<float>(obj, 0x80) * mem.f32(c3::ball_vel_scale);
        cam_copro(0x06, {});
        of<float>(obj, 0xA0) = 32768.0f;
        of<int16_t>(obj, 0x66) = int16_t(step + 1);
    } else if (step != 0x21) {
        iron_ball_draw(obj);
        return;
    }
    of<float>(obj, 0x20) = of<float>(obj, 0x20) + of<float>(obj, 0x78);
    of<float>(obj, 0x24) = of<float>(obj, 0x24) + of<float>(obj, 0x7C);
    of<float>(obj, 0x28) = of<float>(obj, 0x28) + of<float>(obj, 0x80);
    const uint32_t from = local_alloc(16);   // fp + 0x50: the view point of the last frame
    mem.set32(from, of<uint32_t>(obj, 0x98));
    mem.set32(from + 4, of<uint32_t>(obj, 0x9C));
    mem.set32(from + 8, of<uint32_t>(obj, 0xA0));
    cam_copro(0x74, {5});
    const Tgp::Out c = cam_copro(0x1A, {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)});
    of<uint32_t>(obj, 0x98) = c.w[0];
    of<uint32_t>(obj, 0x9C) = c.w[1];
    of<uint32_t>(obj, 0xA0) = c.w[2];
    if (pc_path())   // PC Mode / Boss Mode: the view point through stored matrix 5 (0x48D8E8, the file's helper; pmch34)
        pm_m5().transform(of<float>(obj, 0x20), of<float>(obj, 0x24), of<float>(obj, 0x28), of<float>(obj, 0x98),
                          of<float>(obj, 0x9C), of<float>(obj, 0xA0));
    cam_copro(0x06, {});
    if (of<float>(obj, 0xA0) > 10.0f) {
        iron_ball_draw(obj);
        return;
    }
    point_clip_to_depth(obj + 0x98, from, 10.0f);
    cam_copro(0x74, {6});
    const Tgp::Out w = cam_copro(0x1A, {of<uint32_t>(obj, 0x98), of<uint32_t>(obj, 0x9C), of<uint32_t>(obj, 0xA0)});
    of<uint32_t>(obj, 0x20) = w.w[0];
    of<uint32_t>(obj, 0x24) = w.w[1];
    cam_copro(0x06, {});
    of<uint32_t>(obj, 0x28) = w.w[2];
    if (pc_path())   // PC Mode / Boss Mode: back in the world through stored matrix 6 (pmch34)
        pm_m6().transform(of<float>(obj, 0x98), of<float>(obj, 0x9C), of<float>(obj, 0xA0), of<float>(obj, 0x20),
                          of<float>(obj, 0x24), of<float>(obj, 0x28));
    projectile_hit_player(obj + 0x98, 1, 4);
    of<uint32_t>(own, 0x324) = of<uint32_t>(own, 0x324) | 1;
    of<int16_t>(obj, 0x66) = 0x3F;
    iron_ball_draw(obj);
}

// PC Mode / Boss Mode (pmch34): the iron ball's PC helpers. pm_ball_ahead: identity turned about y by `angle`
// (0x48AA34), (0, 0, 1) through it (0x48A630); pm_ball_dist: (start - place) squared, y + x first, then z, its table
// square root (0x48AE6C), as THOTD.EXE 0x48DA14 / 0x48DBA4 compute it
template <class P> void Game<P>::pm_ball_ahead(uint32_t angle, Tgp::Out& v)
{
    pm::Matrix m;
    m.rotate_y(angle);
    float o[3];
    m.transform(0.0f, 0.0f, 1.0f, o[0], o[1], o[2]);
    for (int k = 0; k < 3; ++k)
        v.w[k] = fbits(o[k]);
}
template <class P> float Game<P>::pm_ball_dist(uint32_t obj)
{
    const float dy = of<float>(obj, 0x398) - of<float>(obj, 0x24);
    const float dx = of<float>(obj, 0x394) - of<float>(obj, 0x20);
    const float dz = of<float>(obj, 0x39C) - of<float>(obj, 0x28);
    return pm_fast_sqrt((dy * dy + dx * dx) + dz * dz);
}

// 0x484D0 iron_ball_pos_hit: step 0x20: start point (+0x394), straight ahead of the owner (its heading) at speed
// 0xAFD2C; then it flies; 50 from the start: the owner told (+0x324 bit 0), step 0x3F
// UNVERIFIED (teaser)
template <class P> void Game<P>::iron_ball_pos_hit(uint32_t obj)
{
    const uint32_t own = of<uint32_t>(obj, 0x18);
    const int16_t step = of<int16_t>(obj, 0x66);
    if (step == 0x20) {
        of<uint32_t>(obj, 0x394) = of<uint32_t>(obj, 0x20);
        of<uint32_t>(obj, 0x398) = of<uint32_t>(obj, 0x24);
        of<uint32_t>(obj, 0x39C) = of<uint32_t>(obj, 0x28);
        cam_copro(0x21, {});
        cam_copro(0x15, {of<uint32_t>(own, 0x30)});
        Tgp::Out v = cam_copro(0x1A, {0, 0, fbits(1.0f)});
        if (pc_path())   // PC Mode / Boss Mode: THOTD.EXE 0x48DA14: identity turned about y, (0, 0, 1) through it (pmch34)
            pm_ball_ahead(of<uint32_t>(own, 0x30), v);
        of<uint32_t>(obj, 0x78) = v.w[0];
        of<uint32_t>(obj, 0x7C) = v.w[1];
        of<uint32_t>(obj, 0x80) = v.w[2];
        of<float>(obj, 0x78) = of<float>(obj, 0x78) * mem.f32(c3::ball_vel_scale);
        of<float>(obj, 0x7C) = of<float>(obj, 0x7C) * mem.f32(c3::ball_vel_scale);
        of<float>(obj, 0x80) = of<float>(obj, 0x80) * mem.f32(c3::ball_vel_scale);
        cam_copro(0x06, {});
        of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
    } else if (step != 0x21) {
        iron_ball_draw(obj);
        return;
    }
    const float px = of<float>(obj, 0x20) + of<float>(obj, 0x78);
    const float py = of<float>(obj, 0x24) + of<float>(obj, 0x7C);
    const float pz = of<float>(obj, 0x28) + of<float>(obj, 0x80);
    of<float>(obj, 0x20) = px;
    of<float>(obj, 0x24) = py;
    of<float>(obj, 0x28) = pz;
    Tgp::Out d = cam_copro(0x1F, {fbits(px), fbits(py), fbits(pz), of<uint32_t>(obj, 0x394), of<uint32_t>(obj, 0x398),
                                  of<uint32_t>(obj, 0x39C)});
    if (pc_path())   // PC Mode / Boss Mode: the table square root of the distance from the start (pmch34)
        d.w[0] = fbits(pm_ball_dist(obj));
    if (!(ffrom(d.w[0]) < 50.0f)) {
        of<uint32_t>(own, 0x324) = of<uint32_t>(own, 0x324) | 1;
        of<int16_t>(obj, 0x66) = 0x3F;
    }
    iron_ball_draw(obj);
}

// 0x469F0 iron_ball_pos_pull_sub: the entries 0x2F leave the list shot_list_a (count: scene_quads +0x60)
template <class P> void Game<P>::iron_ball_pos_pull_sub()
{
    const uint32_t q = mem.u32(G::scene_quads);
    uint32_t dst = G::shot_list_a, src = G::shot_list_a;
    uint32_t n = 0;
    for (uint32_t i = 0; i < mem.at<uint8_t>(q + 0x60); ++i) {
        const uint32_t v = mem.u32(src);
        src += 4;
        if (v != 0x2F) {
            mem.set32(dst, v);
            dst += 4;
            ++n;
        }
    }
    mem.at<uint8_t>(q + 0x60) = uint8_t(n);
}

// 0x48690 iron_ball_pos_pull: step 0x20: start point, along the owner's heading + 0xAFD30 at speed 0xAFD2C; step
// 0x21: flies; 0xAFD34 from the start: 0x51E980 = 1, pull_sub, then falls (gravity -0.0272, x / z slowed by 0xAFD38);
// step 0x22: falls (y twice by its speed) to the floor: the owner told (+0x324 bit 0), step 0x3F
template <class P> void Game<P>::iron_ball_pos_pull(uint32_t obj)
{
    const uint32_t own = of<uint32_t>(obj, 0x18);
    const int16_t step = of<int16_t>(obj, 0x66);
    if (step == 0x20) {
        of<uint32_t>(obj, 0x394) = of<uint32_t>(obj, 0x20);
        of<uint32_t>(obj, 0x398) = of<uint32_t>(obj, 0x24);
        of<uint32_t>(obj, 0x39C) = of<uint32_t>(obj, 0x28);
        cam_copro(0x21, {});
        cam_copro(0x15, {of<uint32_t>(own, 0x30) + mem.u32(c3::ball_pull_turn)});
        Tgp::Out v = cam_copro(0x1A, {0, 0, fbits(1.0f)});
        if (pc_path())   // PC Mode / Boss Mode: THOTD.EXE 0x48DBA4, as iron_ball_pos_hit's (pmch34)
            pm_ball_ahead(of<uint32_t>(own, 0x30) + mem.u32(c3::ball_pull_turn), v);
        of<uint32_t>(obj, 0x78) = v.w[0];
        of<uint32_t>(obj, 0x7C) = v.w[1];
        of<uint32_t>(obj, 0x80) = v.w[2];
        of<float>(obj, 0x78) = of<float>(obj, 0x78) * mem.f32(c3::ball_vel_scale);
        of<float>(obj, 0x7C) = of<float>(obj, 0x7C) * mem.f32(c3::ball_vel_scale);
        of<float>(obj, 0x80) = of<float>(obj, 0x80) * mem.f32(c3::ball_vel_scale);
        cam_copro(0x06, {});
        of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
    }
    if (step == 0x20 || step == 0x21) {
        const float px = of<float>(obj, 0x20) + of<float>(obj, 0x78);
        const float py = of<float>(obj, 0x24) + of<float>(obj, 0x7C);
        const float pz = of<float>(obj, 0x28) + of<float>(obj, 0x80);
        of<float>(obj, 0x20) = px;
        of<float>(obj, 0x24) = py;
        of<float>(obj, 0x28) = pz;
        Tgp::Out d = cam_copro(0x1F, {fbits(px), fbits(py), fbits(pz), of<uint32_t>(obj, 0x394),
                                      of<uint32_t>(obj, 0x398), of<uint32_t>(obj, 0x39C)});
        if (pc_path())   // PC Mode / Boss Mode: the table square root of the distance from the start (pmch34)
            d.w[0] = fbits(pm_ball_dist(obj));
        if (ffrom(d.w[0]) < mem.f32(c3::ball_pull_reach)) {
            iron_ball_draw(obj);
            return;
        }
        mem.at<uint8_t>(c3::ball_pulled) = 1;
        iron_ball_pos_pull_sub();
        of<float>(obj, 0x7C) = 0.0f;
        of<uint32_t>(obj, 0x88) = 0xBCDF0123u;
        of<float>(obj, 0x78) = of<float>(obj, 0x78) * mem.f32(c3::ball_pull_slow);
        of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
        of<float>(obj, 0x80) = of<float>(obj, 0x80) * mem.f32(c3::ball_pull_slow);
    } else if (step != 0x22) {
        iron_ball_draw(obj);
        return;
    }
    const float vy = of<float>(obj, 0x7C) + of<float>(obj, 0x88);
    const float py = (of<float>(obj, 0x24) + vy) + vy;
    of<float>(obj, 0x20) = of<float>(obj, 0x20) + of<float>(obj, 0x78);
    of<float>(obj, 0x28) = of<float>(obj, 0x28) + of<float>(obj, 0x80);
    of<float>(obj, 0x7C) = vy;
    of<float>(obj, 0x24) = py;
    if (!(py > mem.f32(G::floor_y))) {
        of<uint32_t>(own, 0x324) = of<uint32_t>(own, 0x324) | 1;
        of<int16_t>(obj, 0x66) = 0x3F;
    }
    iron_ball_draw(obj);
}

// 0x48920 iron_ball_pos_6: step 0x40: the target (+0x3A0) = node chain 0xAFDC0 posed by motion 0x1CA, frame 0;
// +0x30C = the owner's motion's frame count; then each frame the ball eases to the target: (pos * n + target) /
// (n + 1), n counting down to 0
// Proven on A (vf_c3a: 150 captures, 0 differ)
template <class P> void Game<P>::iron_ball_pos_6(uint32_t obj)
{
    LocalScope ls(*this);
    const uint32_t own = of<uint32_t>(obj, 0x18);
    if (of<int16_t>(obj, 0x66) == 0x40) {
        const uint32_t out = local_alloc(16);
        iron_ball_node_pos(own, 0x1CA, 0, out);
        of<uint32_t>(obj, 0x3A0) = mem.u32(out);
        of<uint32_t>(obj, 0x3A4) = mem.u32(out + 4);
        of<uint32_t>(obj, 0x3A8) = mem.u32(out + 8);
        const float n = float(mem.at<int32_t>(G::motion_frame_counts + uint32_t(int32_t(of<int16_t>(own, 0xD0))) * 4));
        of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
        of<float>(obj, 0x30C) = n;
    }
    float n = of<float>(obj, 0x30C) - 1.0f;
    if (n < 0.0f)
        n = 0.0f;
    const float d = n + 1.0f;
    const float px = (of<float>(obj, 0x20) * n + of<float>(obj, 0x3A0)) / d;
    const float py = (of<float>(obj, 0x24) * n + of<float>(obj, 0x3A4)) / d;
    const float pz = (of<float>(obj, 0x28) * n + of<float>(obj, 0x3A8)) / d;
    of<float>(obj, 0x30C) = n;
    of<float>(obj, 0x20) = px;
    of<float>(obj, 0x24) = py;
    of<float>(obj, 0x28) = pz;
    iron_ball_draw(obj);
}

// 0x489F0 iron_ball_pos_7: steps 0x60 / 0x61: the ball at node chain 0xAFDC0 posed by motion 0x1C9, frame 0
template <class P> void Game<P>::iron_ball_pos_7(uint32_t obj)
{
    LocalScope ls(*this);
    const int16_t step = of<int16_t>(obj, 0x66);
    const uint32_t own = of<uint32_t>(obj, 0x18);
    if (step == 0x60)
        of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
    if (step == 0x60 || step == 0x61) {
        const uint32_t out = local_alloc(16);
        iron_ball_node_pos(own, 0x1C9, 0, out);
        of<uint32_t>(obj, 0x20) = mem.u32(out);
        of<uint32_t>(obj, 0x24) = mem.u32(out + 4);
        of<uint32_t>(obj, 0x28) = mem.u32(out + 8);
    }
    iron_ball_draw(obj);
}

// 0x48A50 iron_ball_drop: the owner dead: step 0: thrown up along 0x51EAC0 (0, 1, 0.5), gravity -0.2, two bounces
// (half the speed back at the floor); step 2: out of view (coprocessor 0x7B, radius 20): the chain told (+0x2AA), ends
// Proven on A (vf_c3a: 150 captures, 0 differ)
template <class P> void Game<P>::iron_ball_drop(uint32_t obj)
{
    const int16_t step = of<int16_t>(obj, 0x66);
    const uint32_t x = of<uint32_t>(obj, 0x37C);
    if (step == 0) {
        cam_copro(0x21, {});
        cam_copro(0x15, {mem.u32(c3::drop_heading)});
        Tgp::Out v = cam_copro(0x1A, {0, fbits(1.0f), fbits(0.5f)});
        if (pc_path()) {   // PC Mode / Boss Mode: the PC's helpers (identity turned about y, (0, 1, 0.5) through it)
            pm::Matrix m;
            m.rotate_y(mem.u32(c3::drop_heading));
            float o[3];
            m.transform(0.0f, 1.0f, 0.5f, o[0], o[1], o[2]);
            for (int k = 0; k < 3; ++k)
                v.w[k] = fbits(o[k]);
        }
        of<uint32_t>(obj, 0x78) = v.w[0];
        of<uint32_t>(obj, 0x7C) = v.w[1];
        cam_copro(0x06, {});
        of<float>(obj, 0x84) = 0.0f;
        of<float>(obj, 0x8C) = 0.0f;
        of<uint32_t>(obj, 0x88) = 0xBE4CCCCDu;
        of<int32_t>(obj, 0x2EC) = 2;
        of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
        of<uint32_t>(obj, 0x80) = v.w[2];
    }
    if (step == 0 || step == 1) {
        const float y0 = of<float>(obj, 0x24);
        const float vx = of<float>(obj, 0x78) + of<float>(obj, 0x84);
        float vy = of<float>(obj, 0x7C) + of<float>(obj, 0x88);
        const float vz = of<float>(obj, 0x80) + of<float>(obj, 0x8C);
        of<float>(obj, 0x78) = vx;
        of<float>(obj, 0x80) = vz;
        of<float>(obj, 0x20) = of<float>(obj, 0x20) + vx;
        of<float>(obj, 0x24) = y0 + vy;
        of<float>(obj, 0x28) = of<float>(obj, 0x28) + vz;
        bool next = false;
        if (!(of<float>(obj, 0x24) > mem.f32(G::floor_y))) {
            vy = vy * -0.5f;
            of<float>(obj, 0x24) = vy + y0;
            of<int32_t>(obj, 0x2EC) = of<int32_t>(obj, 0x2EC) - 1;
            next = of<int32_t>(obj, 0x2EC) == 0;
        }
        of<float>(obj, 0x7C) = vy;
        if (!next) {
            iron_ball_draw(obj);
            return;
        }
        of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
    } else if (step != 2) {
        iron_ball_draw(obj);
        return;
    }
    const Tgp::Out s = cam_copro(0x7B, {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28),
                                        fbits(20.0f)});
    const float vis = pc_path() ? pm_view_test(of<float>(obj, 0x20), of<float>(obj, 0x24), of<float>(obj, 0x28), 20.0f)
                                : ffrom(s.w[0]);   // PC Mode / Boss Mode: the PC's view test on its stored matrix 0
    if (vis >= 0.0f) {
        iron_ball_draw(obj);
        return;
    }
    mem.at<int16_t>(mem.u32(x + 0x10) + 0x2AA) = 1;
    task_exit();
}

// 0x48C50 iron_ball_hitbox: a shootable trail point of the swinging ball (iron_ball_draw makes them): shot: spark,
// sound 0xA0120B; lives +0x2EC frames; its camera-space point
// Proven on A (vf_c3a: 150 captures, 0 differ)
template <class P> void Game<P>::iron_ball_hitbox(uint32_t obj)
{
    if ((of<uint16_t>(obj, 0x58) >> 8) & 1) {
        hit_spark_spawn(obj, mem.u32(c3::ball_spark_size));
        sound_cue(0xA0120B);
    }
    of<int32_t>(obj, 0x2EC) = of<int32_t>(obj, 0x2EC) - 1;
    if (of<int32_t>(obj, 0x2EC) == 0) {
        task_exit();
        return;
    }
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xFFFFFEF9u;
    depth_list_add(obj);
    Tgp::Out v = cam_copro(0x1A, {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)});
    if (pc_path()) {   // PC Mode / Boss Mode: its camera-space point through the PC's stored matrix 0 (pmrest2)
        float o[3];
        pm_m0().transform(of<float>(obj, 0x20), of<float>(obj, 0x24), of<float>(obj, 0x28), o[0], o[1], o[2]);
        for (int k = 0; k < 3; ++k)
            v.w[k] = fbits(o[k]);
    }
    of<uint32_t>(obj, 0x38) = v.w[0];
    of<uint32_t>(obj, 0x3C) = v.w[1];
    of<uint32_t>(obj, 0x40) = v.w[2];
}

// 0x48CF0 iron_ball_draw: extra +0xA = the mode; +0x31C steps by 0xAFD20: the owner swinging (+0x328 bit 0), a hit
// box task (iron_ball_hitbox) each 0x10000; shot: spark, sound 0xA0120B; shootable; the ball model (0xAFCF0 by
// texture area) at its place; its camera-space point (stored matrix 5) at +0x98
template <class P> void Game<P>::iron_ball_draw(uint32_t obj)
{
    const uint32_t x = of<uint32_t>(obj, 0x37C);
    const uint32_t own = of<uint32_t>(obj, 0x18);
    mem.at<int16_t>(x + 0xA) = mem.at<int16_t>(x + 8);
    const uint32_t a = of<uint32_t>(obj, 0x31C);
    const uint32_t b = a + mem.u32(c3::ball_spin_step);
    of<uint32_t>(obj, 0x31C) = b;
    if ((of<uint8_t>(own, 0x328) & 1) && ((a ^ b) & 0xFFFF0000u) != 0) {
        const uint32_t t = task_create(F::iron_ball_hitbox, 0x3C8);
        of<int32_t>(t, 0x58) = 1;
        of<int32_t>(t, 0xC8) = 0;
        of<uint32_t>(t, 0x90) = of<uint32_t>(obj, 0x90);
        of<uint32_t>(t, 0x2EC) = mem.u32(c3::ball_hitbox_life);
        for (uint32_t o = 0x20; o < 0x38; o += 4)
            of<uint32_t>(t, o) = of<uint32_t>(obj, o);
        const Tgp::Out v = cam_copro(0x1A, {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)});
        of<uint32_t>(t, 0x38) = v.w[0];
        of<uint32_t>(t, 0x3C) = v.w[1];
        of<uint32_t>(t, 0x40) = v.w[2];
    }
    if ((of<uint16_t>(obj, 0x58) >> 8) & 1) {
        hit_spark_spawn(obj, mem.u32(c3::ball_spark_size));
        sound_cue(0xA0120B);
    }
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xFFFFFEF9u;
    depth_list_add(obj);
    const uint32_t model = uint32_t(int32_t(mem.at<int16_t>(c3::ball_models + mem.u32(G::texture_area) * 2)));
    cam_copro(0x05, {});
    cam_copro(0x12, {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)});
    geo_matrix_position(model, obj + 0x38);
    cam_copro(0x77, {3});
    draw_fade_begin(0);
    geo_draw_model(model);
    geo_send_words(0, 31, 0x7A0);
    cam_copro(0x06, {});
    cam_copro(0x74, {5});
    const Tgp::Out c = cam_copro(0x1A, {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)});
    of<uint32_t>(obj, 0x98) = c.w[0];
    of<uint32_t>(obj, 0x9C) = c.w[1];
    cam_copro(0x06, {});
    of<uint32_t>(obj, 0xA0) = c.w[2];
}

// 0x49B60 iron_ball_hand_pos (body, type, motion, frame, out): body_node_pos_at_frame through node chain 0xAFDA0
// (PC: its own node matrix code; vtable only here)
template <class P> void Game<P>::iron_ball_hand_pos(uint32_t obj, int node, uint32_t motion, int frame, uint32_t out)
{
    if constexpr (!P::pc)
        body_node_pos_at_frame(obj, node, motion, frame, c3::hand_chain, out);
}

// 0x49B80 iron_ball_node_pos (body, motion, frame, out): body_node_pos_at_frame of type 0x36 through chain 0xAFDC0
template <class P> void Game<P>::iron_ball_node_pos(uint32_t obj, uint32_t motion, int frame, uint32_t out)
{
    body_node_pos_at_frame(obj, 0x36, motion, frame, c3::ball_chain, out);
}

// 0x49BB0 body_node_pos_at_frame (body, type, motion, frame, chain, out): a node's world position at a motion frame:
// the body's place, angles and scale, the frame's root offset and angles, then per chain entry (node index, joint
// index; -1 ends) that node's offset (type node table 0xC7240 +0x1C, walked) and the joint's angles
template <class P> void Game<P>::body_node_pos_at_frame(uint32_t obj, int type, uint32_t motion, int frame, uint32_t chain,
                                                       uint32_t out)
{
    const uint32_t joints = mem.u32(0xC73C0 + uint32_t(type) * 4);
    const uint32_t mdata = mem.u32(0xC7540 + motion * 4);
    uint32_t node = mem.u32(G::creature_type_headers + uint32_t(type) * 4);
    cam_copro(0x21, {});
    cam_copro(0x12, {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)});
    const uint32_t size = (joints * 48 + 0x7F) >> 5;
    cam_copro(0x86, {of<uint32_t>(obj, 0x34), of<uint32_t>(obj, 0x30), of<uint32_t>(obj, 0x2C)});
    const uint32_t s = of<uint32_t>(obj, 0x94);
    cam_copro(0x13, {s, s, s});
    const uint32_t fd = uint32_t(frame) * 4 * size + mdata;
    cam_copro(0x12, {mem.u32(fd), mem.u32(fd + 4), mem.u32(fd + 8) ^ 0x80000000u});
    cam_copro(0x86, {0u - uint32_t(mem.at<uint16_t>(fd + 0x10)), uint32_t(mem.at<uint16_t>(fd + 0xE)),
                     uint32_t(mem.at<uint16_t>(fd + 0xC))});
    for (uint32_t c = chain; mem.at<int16_t>(c) != -1; c += 4) {
        node = mem.u32(node + 0x1C + uint32_t(int32_t(mem.at<int16_t>(c))) * 4);
        cam_copro(0x12, {mem.u32(node + 4), mem.u32(node + 8), mem.u32(node + 0xC) ^ 0x80000000u});
        const uint32_t j = fd + uint32_t(int32_t(mem.at<int16_t>(c + 2))) * 6;
        cam_copro(0x86, {0u - uint32_t(mem.at<uint16_t>(j + 0x10)), uint32_t(mem.at<uint16_t>(j + 0xE)),
                         uint32_t(mem.at<uint16_t>(j + 0xC))});
    }
    const Tgp::Out p = cam_copro(0x20, {});
    mem.set32(out, p.w[0]);
    mem.set32(out + 4, p.w[1]);
    cam_copro(0x06, {});
    mem.set32(out + 8, p.w[2]);
    if (pc_path()) {
        // PC Mode / Boss Mode: THOTD.EXE 0x48EF20 on the iron ball file's helpers (0x48A630): identity, the body's place
        // and turns z / y / x, the 3 x 3 scaled, the frame's root and turns, each chain node's offset and turns; the
        // translation (pmrest2)
        pm::Matrix m;
        m.translate(of<float>(obj, 0x20), of<float>(obj, 0x24), of<float>(obj, 0x28));
        m.rotate_z(of<uint32_t>(obj, 0x34));
        m.rotate_y(of<uint32_t>(obj, 0x30));
        m.rotate_x(of<uint32_t>(obj, 0x2C));
        const float k = of<float>(obj, 0x94);
        for (int i = 0; i < 9; ++i)
            m.e[i] = m.e[i] * k;
        pm::draw1_part(m, mem.f32(fd), mem.f32(fd + 4), mem.f32(fd + 8), 0u - uint32_t(mem.at<uint16_t>(fd + 0x10)),
                       uint32_t(mem.at<uint16_t>(fd + 0xE)), uint32_t(mem.at<uint16_t>(fd + 0xC)));
        uint32_t nd = mem.u32(G::creature_type_headers + uint32_t(type) * 4);
        for (uint32_t c = chain; mem.at<int16_t>(c) != -1; c += 4) {
            nd = mem.u32(nd + 0x1C + uint32_t(int32_t(mem.at<int16_t>(c))) * 4);
            const uint32_t j = fd + uint32_t(int32_t(mem.at<int16_t>(c + 2))) * 6;
            pm::draw1_part(m, mem.f32(nd + 4), mem.f32(nd + 8), mem.f32(nd + 0xC), 0u - uint32_t(mem.at<uint16_t>(j + 0x10)),
                           uint32_t(mem.at<uint16_t>(j + 0xE)), uint32_t(mem.at<uint16_t>(j + 0xC)));
        }
        for (uint32_t i = 0; i < 3; ++i)
            mem.set32(out + 4 * i, fbits(m.e[9 + i]));
    }
}

// --- kind 106 (Chapter 3 event 0): the padlock of the hostage's cell ---

// 0x5C0C0 kind106_padlock: set-up: one hit, model 0x314 (kagii), scale 0.75, radius 2.1; waits for 0x51E970, then
// shootable for 360 frames: a shot: a spark (kind106_lock_spark, model 0x37), sound 0xA0124A, a hit less; broken:
// 0x51E9D8 and 0x51E973 (player 2) / 0x51E974 (player 1), ends; not in time: 0x51E971, waits for 0x51E972, ends.
// Drawn at its place turned by its heading
template <class P> void Game<P>::kind106_padlock(uint32_t obj)
{
    switch (of<int16_t>(obj, 0x60)) {
    case 0:
        of<int16_t>(obj, 0x70) = 1;
        of<int32_t>(obj, 0x54) = 0x314;
        of<uint32_t>(obj, 0x94) = 0x3F400000u;
        of<uint32_t>(obj, 0x90) = 0x40066666u;
        of<int16_t>(obj, 0x60) = 1;
        break;
    case 1:
        if (mem.at<uint8_t>(0x51E970) != 0) {
            of<int32_t>(obj, 0x58) = 1;
            of<int32_t>(obj, 0x50) = 0;
            of<int16_t>(obj, 0x60) = 2;
        }
        break;
    case 2: {
        const int32_t t = of<int32_t>(obj, 0x50);
        of<int32_t>(obj, 0x50) = t + 1;
        if (t > 0x167) {
            mem.at<uint8_t>(0x51E971) = 1;
            of<int16_t>(obj, 0x60) = 4;
            of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
            break;
        }
        if (!((of<uint16_t>(obj, 0x58) >> 8) & 1))
            break;
        const uint32_t sp = task_create(F::kind106_lock_spark, 0x58);
        of<int32_t>(sp, 0x54) = 0x37;
        of<uint32_t>(sp, 0x20) = of<uint32_t>(obj, 0x20);
        of<uint32_t>(sp, 0x24) = of<uint32_t>(obj, 0x24);
        of<uint32_t>(sp, 0x28) = of<uint32_t>(obj, 0x28);
        sound_cue(0xA0124A);
        const uint16_t n = uint16_t(of<uint16_t>(obj, 0x70) - 1);
        of<uint16_t>(obj, 0x70) = n;
        if (int32_t(uint32_t(n) << 16) > 0) {
            of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~0x100u;
            break;
        }
        mem.at<uint8_t>(0x51E9D8) = 1;
        if ((of<uint8_t>(obj, 0x58) >> 1) & 1)
            mem.at<uint8_t>(0x51E973) = 1;
        else
            mem.at<uint8_t>(0x51E974) = 1;
        task_exit();
        return;
    }
    case 4:
        if (mem.at<uint8_t>(0x51E972) != 0) {
            task_exit();
            return;
        }
        break;
    default:
        break;
    }
    cam_copro(0x05, {});
    cam_copro(0x12, {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)});
    cam_copro(0x15, {of<uint32_t>(obj, 0x30)});
    const uint32_t s = of<uint32_t>(obj, 0x94);
    cam_copro(0x13, {s, s, s});
    geo_matrix_position(of<uint32_t>(obj, 0x54), obj + 0x38);
    cam_copro(0x06, {});
    depth_list_add(obj);
}

// 0x5C8B0 kind106_lock_spark: the spark's frames (model + 1 each frame, ends at 0x4A), scale 5
template <class P> void Game<P>::kind106_lock_spark(uint32_t obj)
{
    const int32_t m = of<int32_t>(obj, 0x54) + 1;
    of<int32_t>(obj, 0x54) = m;
    if (m == 0x4A) {
        task_exit();
        return;
    }
    cam_copro(0x05, {});
    cam_copro(0x12, {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)});
    cam_copro(0x13, {0x40A00000u, 0x40A00000u, 0x40A00000u});
    geo_draw_model(of<uint32_t>(obj, 0x54));
    cam_copro(0x06, {});
}

// --- kind 189 (Chapter 3 event 0): the locked double door, its two locks choose the route ---

// 0x40B0 kind189_lock_task: a lock (PN_kagii01y 0x318, radius 1.8, shootable); ends at 0x51E9AF. Shot first: opens
// (0x319, sound 0xA01249), route_choice = its +0xCC, 0x51E9AE = 1; the other lock (or the door's timer) set 0x51E9AE:
// lock 0 opens when the route is 0; not shootable any more. Drawn turned by its heading, scale 0.6
template <class P> void Game<P>::kind189_lock_task(uint32_t obj)
{
    if (mem.at<uint8_t>(0x51E9AF) == 1) {
        task_exit();
        return;
    }
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        of<int32_t>(obj, 0x54) = 0x318;
        of<uint32_t>(obj, 0x90) = 0x3FE66666u;
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | 0x401;
        of<int16_t>(obj, 0x60) = int16_t(sub + 1);
    }
    if (sub == 0 || sub == 1) {
        if (mem.at<uint8_t>(0x51E9AE) == 1) {
            if (of<uint16_t>(obj, 0xCC) == 0 && mem.at<uint8_t>(G::route_choice) == 0) {
                of<int32_t>(obj, 0x54) = 0x319;
                sound_cue(0xA01249);
            }
            of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
            of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
        } else if ((of<uint16_t>(obj, 0x58) >> 8) & 1) {
            of<int32_t>(obj, 0x54) = 0x319;
            of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
            sound_cue(0xA01249);
            mem.at<uint8_t>(0x51E9AE) = 1;
            mem.at<uint8_t>(G::route_choice) = of<uint8_t>(obj, 0xCC);
            of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
        }
    }
    cam_copro(0x05, {});
    cam_copro(0x12, {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)});
    cam_copro(0x15, {of<uint32_t>(obj, 0x30)});
    cam_copro(0x13, {0x3F19999Au, 0x3F19999Au, 0x3F19999Au});
    geo_matrix_position(of<uint32_t>(obj, 0x54), obj + 0x38);
    cam_copro(0x06, {});
    if (of<uint8_t>(obj, 0x58) & 1) {
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~0x100u;
        depth_list_add(obj);
    }
}

// 0x4240 kind189_locked_door: ends at 0x51E9AF; set-up: the two locks (kind189_lock_task, +0xCC 0 / 1, places and
// headings from 0xA8680 / 0xA8698), 240 frames; at their end 0x51E9AE = 1 (the route stays as it is)
template <class P> void Game<P>::kind189_locked_door(uint32_t obj)
{
    if (mem.at<uint8_t>(0x51E9AF) == 1) {
        task_exit();
        return;
    }
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        of<int32_t>(obj, 0x50) = 0xF0;
        for (uint32_t i = 0; i < 2; ++i) {
            const uint32_t t = task_create(F::kind189_lock_task, 0x3C8);
            task_clear_work(t);
            of<int16_t>(t, 0xCC) = int16_t(i);
            const uint32_t r = 0xA8680 + i * 0x18;
            of<uint32_t>(t, 0x20) = mem.u32(r);
            of<uint32_t>(t, 0x24) = mem.u32(r + 4);
            of<uint32_t>(t, 0x28) = mem.u32(r + 8);
            of<uint32_t>(t, 0x30) = mem.u32(r + 0x10);
        }
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
    } else if (sub != 1) {
        return;
    }
    const int32_t t = of<int32_t>(obj, 0x50) - 1;
    of<int32_t>(obj, 0x50) = t;
    if (t == 0) {
        mem.at<uint8_t>(0x51E9AE) = 1;
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
    }
}

// --- kind 153 (Chapter 3 events 0-2): the Taran (types 48 / 90), the leaping creatures ---

namespace c3 {
constexpr uint32_t taran_floor = 0x543460;     // float: its floor (set-up: floor_y; actions 7-10: -154.1)
constexpr uint32_t taran_gone = 0x54345C;      // byte: set when action 2's Taran was shot (action 5 does not come)
constexpr uint32_t taran_paths = 0xC5770;      // per action 0x48 bytes: B-spline points (x, z pairs)
constexpr uint32_t eye = 0x51EAB0;             // camera_records[0].eye x / y / z
} // namespace c3

// 0x638D0 kind153_taran: set-up (action 5 not when 0x54345C is set): body (MO 0x1C6), NoBias, counted as an enemy,
// an attack position (not actions 2 / 6), its player +0x2E8 = action; by action: 0 crawls at once (state 1), 2 after
// 30 frames, 3-5 25 frames per step (players alternating), 6 falls from the ceiling (state 3, speed -0.9), 7-10 from
// the upper floor (-154.1); kind153_taran_frame from the next frame
template <class P> void Game<P>::kind153_taran(uint32_t obj)
{
    if (mem.at<uint8_t>(c3::taran_gone) != 0 && of<int16_t>(obj, 0x64) == 5) {
        task_exit();
        return;
    }
    mem.at<int32_t>(G::hit_type) = of<int16_t>(obj, 0xCC);
    body_init(obj, of<int16_t>(obj, 0xCC), 0x1C6);
    of<uint8_t>(obj, 0x3C5) = uint8_t(of<uint8_t>(obj, 0x3C5) | 8);
    creature_type_setup(obj);
    of<int32_t>(obj, 0x2EC) = 0;
    of<int32_t>(obj, 0x30C) = 0;
    of<int32_t>(obj, 0x33C) = 0;
    of<int32_t>(obj, 0x340) = 0;
    of<int32_t>(obj, 0x78) = 0;
    of<int32_t>(obj, 0x7C) = 0;
    of<int16_t>(obj, 0x6C) = 2;
    of<uint8_t>(obj, 0x2E8) = of<uint8_t>(obj, 0x64);
    mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) + 1);
    mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) + 1);
    of<int8_t>(obj, 0x2E7) = -1;
    if (of<int16_t>(obj, 0x64) != 2 && of<int16_t>(obj, 0x64) != 6)
        attack_slot_take_wide(obj);
    of<int16_t>(obj, 0x5C) = 0;
    of<int32_t>(obj, 0x50) = 40;
    const int32_t a = of<int16_t>(obj, 0x64);
    mem.set32(c3::taran_floor, mem.u32(G::floor_y));
    switch (a) {
    case 0:
        of<int32_t>(obj, 0x50) = 0;
        of<int16_t>(obj, 0x5C) = 1;
        mem.at<uint8_t>(c3::taran_gone) = 0;
        break;
    case 2:
        of<int32_t>(obj, 0x50) = 30;
        break;
    case 3: case 4: case 5: {
        of<int32_t>(obj, 0x50) = (a - 2) * 25;
        const int32_t k = a - 3;
        of<int8_t>(obj, 0x2E8) = int8_t(k - ((k < 0 ? a - 2 : k) & ~1));
        break;
    }
    case 6:
        of<int16_t>(obj, 0x5C) = 3;
        of<int32_t>(obj, 0x50) = 100;
        of<int16_t>(obj, 0x6C) = 0;
        of<uint32_t>(obj, 0x7C) = 0xBF666666u;
        break;
    case 7: case 8: case 9: case 10:
        of<int8_t>(obj, 0x2E8) = int8_t(1 - (a - ((a < 0 ? a + 1 : a) & ~1)));
        of<int32_t>(obj, 0x7C) = 0;
        mem.set32(c3::taran_floor, 0xC31A199Au);
        break;
    default:
        break;
    }
    of<uint32_t>(obj, 0) = F::kind153_taran_frame;
}

// inline (kind153_taran_frame's common end, also in kind153_taran_reach_check): off its attack position, x 400 (it
// ends), kind153_taran_fall_off, dying; uncounted (+0x58 bit 20 while others are alive)
template <class P> void Game<P>::kind153_taran_leave(uint32_t obj)
{
    if (of<uint8_t>(obj, 0x2E7) != 0xFF)
        enemy_attack_slot_free(obj);
    of<int32_t>(obj, 0x50) = 0;
    of<float>(obj, 0x20) = 400.0f;
    of<uint32_t>(obj, 0) = F::kind153_taran_fall_off;
    of<uint32_t>(obj, 0x58) = (of<uint32_t>(obj, 0x58) | 0x10000000u) & ~1u;
    if (mem.at<int16_t>(G::enemies_alive) > 1)
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | 0x100000u;
    mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) - 1);
    mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) - 1);
}

// inline: it reached the player (player_hit: the only player in a one-player game, else its +0x2E8), then leaves
template <class P> void Game<P>::kind153_taran_hit_player(uint32_t obj)
{
    const int p = mem.at<uint16_t>(G::players_in_game_b) == 1 ? mem.at<int8_t>(G::solo_player_is_p2) : of<int8_t>(obj, 0x2E8);
    player_hit(p, 1, 1);
    kind153_taran_leave(obj);
}

// 0x63AC0 kind153_taran_frame (with its Ghidra split kind153_taran_reach_check 0x64224): shot: 80 points, spurt,
// sound 0xA01341, MO 0x1C7, falls off (actions 3-6 sideways). By state: 0 waits (+0x50), then 1 / 3 / 4 by action;
// 1 crawls (action 2: along the wall, then up and away; else along its B-spline path 0xC5770 (+0x2EC point, +0x30C
// t), down from z 121, up to x 548, then leaps: 2); 2 the leap at the camera (reaching x 522: hits); 3 falls (action
// 6 from the ceiling, away under -280; else at the player: reaching the floor + 17 + 24 per event: hits); 4 walks to
// z 965, leaps: 5 (reaching z 954: hits). Actions 0-2 face along their way (coprocessor 0x0F); drawn with its shadow
// (action 6 with its thread, model 0x5F8)
template <class P> void Game<P>::kind153_taran_frame(uint32_t obj)
{
    LocalScope ls(*this);
    const uint32_t tw = of<uint32_t>(obj, 0x30C);
    const uint32_t seg = of<uint8_t>(obj, 0x2EC);
    if ((of<uint8_t>(obj, 0x58) & 6) != 0 && !((of<uint32_t>(obj, 0x58) >> 28) & 1)) {
        int p;
        if ((of<uint8_t>(obj, 0x58) & 6) == 6)
            p = int32_t(rand()) % 2;
        else
            p = ((of<uint8_t>(obj, 0x58) >> 1) & 1) ? 0 : 1;
        if (coop4_on())   // 4P: the real shooter behind the bit (the hit table)
            p = coop4_shooter(obj, p);
        add_score(p, 0x50);
        if (of<uint8_t>(obj, 0x2E7) != 0xFF)
            enemy_attack_slot_free(obj);
        prop_spurt_spawn(obj);
        sound_cue(0xA01341);
        of<int32_t>(obj, 0x50) = 0;
        of<uint32_t>(obj, 0x58) = (of<uint32_t>(obj, 0x58) | 0x10000000u) & ~1u;
        set_motion_blend(obj, 0x1C7, 0, 8);
        const int16_t a = of<int16_t>(obj, 0x64);
        of<float>(obj, 0x7C) = 0.5f;
        of<uint32_t>(obj, 0) = F::kind153_taran_fall_off;
        if (a > 2 && a <= 6) {
            of<float>(obj, 0x80) = float(int32_t(of<int8_t>(obj, 0x2E8))) * 0.8f + -0.4f;
            if (mem.at<uint8_t>(G::event_index) == 1)
                of<float>(obj, 0x80) = 0.4f;
        }
        if (of<int16_t>(obj, 0x64) == 2)
            mem.at<uint8_t>(c3::taran_gone) = 1;
        if (mem.at<int16_t>(G::enemies_alive) > 1)
            of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | 0x100000u;
        mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) - 1);
        mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) - 1);
    }
    if (!((of<uint32_t>(obj, 0x58) >> 28) & 1)) {
        const float ex = mem.f32(c3::eye), ey = mem.f32(c3::eye + 4), ez = mem.f32(c3::eye + 8);
        const float floor2 = mem.f32(c3::taran_floor);
        const int16_t a = of<int16_t>(obj, 0x64);
        switch (uint16_t(of<int16_t>(obj, 0x5C))) {
        case 0: {
            const int32_t t = of<int32_t>(obj, 0x50);
            of<int32_t>(obj, 0x50) = t - 1;
            if (t > 0)
                break;
            if (a <= 2) {
                of<int16_t>(obj, 0x5C) = 1;
            } else if (a <= 6) {
                of<int16_t>(obj, 0x5C) = 3;
                if (mem.at<uint8_t>(G::event_index) == 0)
                    of<float>(obj, 0x80) = pdiv((ez - of<float>(obj, 0x28)), 100.0f);
            } else if (of<float>(obj, 0x24) <= floor2) {
                of<int16_t>(obj, 0x5C) = 4;
                of<uint32_t>(obj, 0x24) = mem.u32(c3::taran_floor);
            } else {
                const float vy = of<float>(obj, 0x7C);
                of<float>(obj, 0x24) = of<float>(obj, 0x24) + vy;
                of<float>(obj, 0x7C) = vy - ffrom(0x3CDF0123u);
            }
            break;
        }
        case 1:
            if (a == 2) {
                if (of<float>(obj, 0x28) < 127.0f) {
                    of<float>(obj, 0x20) = of<float>(obj, 0x20) - 0.1f;
                    of<float>(obj, 0x28) = of<float>(obj, 0x28) + 0.25f;
                    break;
                }
                const float y = of<float>(obj, 0x24) + 0.25f;
                of<int32_t>(obj, 0x2C) = 0xC000;
                of<float>(obj, 0x24) = y;
                if (y < 0.0f)
                    break;
                kind153_taran_leave(obj);
            } else {
                of<uint32_t>(obj, 0x33C) = of<uint32_t>(obj, 0x20);
                of<uint32_t>(obj, 0x340) = of<uint32_t>(obj, 0x28);
                const uint32_t b = local_alloc(16);   // fp + 0x40 .. 0x4C
                bspline_basis(seg, ffrom(tw), b, b + 4, b + 8, b + 0xC);
                const uint32_t pt = c3::taran_paths + uint32_t(int32_t(of<int16_t>(obj, 0x64))) * 0x48 + (seg & 0xFF) * 4;
                const float b0 = mem.f32(b), b1 = mem.f32(b + 4), b2 = mem.f32(b + 8), b3 = mem.f32(b + 0xC);
                of<float>(obj, 0x20) = ((b0 * mem.f32(pt) + b1 * mem.f32(pt + 8)) + b2 * mem.f32(pt + 0x10)) + b3 * mem.f32(pt + 0x18);
                const float z = ((b0 * mem.f32(pt + 4) + b1 * mem.f32(pt + 0xC)) + b2 * mem.f32(pt + 0x14)) + b3 * mem.f32(pt + 0x1C);
                of<float>(obj, 0x28) = z;
                if (z > 121.0f && !(of<float>(obj, 0x24) < ffrom(0xC2313333u))) {
                    const float vy = float(double(of<float>(obj, 0x7C)) - 0.15);
                    of<float>(obj, 0x7C) = vy;
                    of<float>(obj, 0x24) = of<float>(obj, 0x24) + vy;
                }
                if (of<float>(obj, 0x20) >= 548.0f) {
                    const float w = of<float>(obj, 0x30C) + 0.02f;
                    of<float>(obj, 0x30C) = w;
                    if (w <= 1.0f)
                        break;
                    of<int32_t>(obj, 0x30C) = 0;
                    of<int32_t>(obj, 0x2EC) = of<int32_t>(obj, 0x2EC) + 2;
                    if (6 >= of<int32_t>(obj, 0x2EC))
                        break;
                    kind153_taran_leave(obj);
                    break;
                }
                of<int16_t>(obj, 0x5C) = 2;
                of<float>(obj, 0x78) = (ex - of<float>(obj, 0x20)) / 70.0f;
                of<float>(obj, 0x7C) = (ey - of<float>(obj, 0x24)) / 90.0f + ffrom(0x3EB851ECu);
                const float tz = float(int32_t(of<int8_t>(obj, 0x2E8)));
                of<float>(obj, 0x80) = (ez - of<float>(obj, 0x28)) / 90.0f - tz * 0.02f;
            }
            break;
        case 2: {
            const float vy = of<float>(obj, 0x7C) - 0.01f;
            const float x = of<float>(obj, 0x20) + of<float>(obj, 0x78);
            const float z = of<float>(obj, 0x28) + of<float>(obj, 0x80);
            of<float>(obj, 0x7C) = vy;
            of<float>(obj, 0x20) = x;
            const float y = of<float>(obj, 0x24) + of<float>(obj, 0x7C);
            of<int32_t>(obj, 0x2C) = of<int32_t>(obj, 0x2C) + 0x100;
            of<float>(obj, 0x28) = z;
            of<float>(obj, 0x24) = y;
            if (!(x < 522.0f))
                break;
            kind153_taran_hit_player(obj);
            break;
        }
        case 3:
            if (a == 6) {
                const float vy = of<float>(obj, 0x7C) + 0.06f;
                const float y = of<float>(obj, 0x24) + vy;
                of<float>(obj, 0x7C) = vy;
                of<float>(obj, 0x24) = y;
                if (y <= -280.0f)
                    break;
                kind153_taran_leave(obj);
            } else {
                const float vy = of<float>(obj, 0x7C) - 0.02f;
                of<float>(obj, 0x7C) = vy;
                of<float>(obj, 0x24) = of<float>(obj, 0x24) + vy;
                if (mem.at<uint8_t>(G::event_index) == 0) {
                    of<int32_t>(obj, 0x2C) = of<int32_t>(obj, 0x2C) + 0x260;
                    if (of<int16_t>(obj, 0x64) == 5) {
                        of<float>(obj, 0x20) = of<float>(obj, 0x20) - 0.13f;
                        of<float>(obj, 0x28) = of<float>(obj, 0x80) * 1.3f + of<float>(obj, 0x28);
                    } else {
                        of<float>(obj, 0x20) = of<float>(obj, 0x20) - 0.11f;
                        of<float>(obj, 0x28) = of<float>(obj, 0x28) + of<float>(obj, 0x80);
                    }
                } else {
                    of<float>(obj, 0x28) = of<float>(obj, 0x28) - 0.25f;
                    of<int32_t>(obj, 0x2C) = of<int32_t>(obj, 0x2C) + 0x160;
                }
                // 0x64224 (kind153_taran_reach_check)
                const float h = (mem.f32(c3::taran_floor) + 17.0f) + float(int32_t(mem.at<int8_t>(G::event_index))) * 24.0f;
                if (of<float>(obj, 0x24) > h)
                    break;
                kind153_taran_hit_player(obj);
            }
            break;
        case 4: {
            const float z = of<float>(obj, 0x28) - 0.4f;
            of<float>(obj, 0x28) = z;
            if (z >= 965.0f)
                break;
            of<int16_t>(obj, 0x5C) = 5;
            const float tx = float(int32_t(of<int8_t>(obj, 0x2E8)));
            of<float>(obj, 0x78) = (ex - of<float>(obj, 0x20)) / 60.0f - tx * 0.025f;
            of<float>(obj, 0x7C) = (ey - of<float>(obj, 0x24)) / 90.0f + 0.2f;
            of<float>(obj, 0x80) = (ez - of<float>(obj, 0x28)) / 90.0f;
            break;
        }
        case 5: {
            const float vz = of<float>(obj, 0x80) - 0.02f;
            of<float>(obj, 0x80) = vz;
            const float z = of<float>(obj, 0x28) + vz;
            const float x = of<float>(obj, 0x20) + of<float>(obj, 0x78);
            const float y = of<float>(obj, 0x24) + of<float>(obj, 0x7C);
            of<float>(obj, 0x28) = z;
            of<int32_t>(obj, 0x2C) = of<int32_t>(obj, 0x2C) + 0x240;
            of<float>(obj, 0x20) = x;
            of<float>(obj, 0x24) = y;
            if (z >= 954.0f)
                break;
            kind153_taran_hit_player(obj);
            break;
        }
        default:
            break;
        }
        if (of<int16_t>(obj, 0x64) <= 2) {
            const Tgp::Out o = cam_copro(0x0F, {fbits(of<float>(obj, 0x20) - of<float>(obj, 0x33C)), 0,
                                                fbits(of<float>(obj, 0x28) - of<float>(obj, 0x340))});
            uint32_t yaw = o.w[0];
            const uint32_t pitch = o.w[1] & 0xFFFF;
            if (((pitch ^ (pitch << 1)) >> 15) & 1)
                yaw = yaw + 0x8000;
            if (of<float>(obj, 0x30C) != ffrom(0x3CA3D70Au))
                of<uint32_t>(obj, 0x30) = yaw;
        }
    }
    if (of<int16_t>(obj, 0x5C) != 0)
        of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) + 1;
    shadow_setup(mem.f32(c3::taran_floor));
    object_draw(obj);
    if (of<int16_t>(obj, 0x64) == 6) {
        cam_copro(0x05, {});
        cam_copro(0x12, {of<uint32_t>(obj, 0x20), fbits(of<float>(obj, 0x24) + 1.0f), of<uint32_t>(obj, 0x28)});
        cam_copro(0x16, {0x8000});
        cam_copro(0x13, {0x3DCCCCCDu, 0x41000000u, 0x3DCCCCCDu});
        geo_draw_model(0x5F8);
        cam_copro(0x06, {});
    }
    depth_list_add(obj);
}

// 0x64650 kind153_taran_fall_off: plays MO 0x1C7 to frame 150; falls (gravity 0.03; actions 3-6 sideways) to its
// floor (then lies flat); blinks from 160 frames; ends at 200 frames or at x 400 (reached the player): parts freed
template <class P> void Game<P>::kind153_taran_fall_off(uint32_t obj)
{
    if (of<int16_t>(obj, 0xF4) <= 0x95)
        of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) + 1;
    if (of<float>(obj, 0x24) <= mem.f32(c3::taran_floor) - 0.01f) {
        of<int32_t>(obj, 0x2C) = 0;
        of<int32_t>(obj, 0x34) = 0;
    } else {
        const float vy = of<float>(obj, 0x7C) - 0.03f;
        const float y = of<float>(obj, 0x24) + vy;
        const int16_t a = of<int16_t>(obj, 0x64);
        of<float>(obj, 0x7C) = vy;
        of<float>(obj, 0x24) = y;
        if (a > 2 && a <= 6) {
            if (mem.at<uint8_t>(G::event_index) == 0) {
                of<float>(obj, 0x20) = of<float>(obj, 0x20) + 0.5f;
                of<float>(obj, 0x28) = of<float>(obj, 0x28) + 0.2f;
            } else {
                of<float>(obj, 0x28) = of<float>(obj, 0x28) + 0.8f;
            }
        }
    }
    const int32_t t = of<int32_t>(obj, 0x50) + 1;
    of<int32_t>(obj, 0x50) = t;
    if (t <= 0x9F || t % 2 != 0) {
        shadow_setup(mem.f32(c3::taran_floor));
        object_draw(obj);
    }
    if (of<int32_t>(obj, 0x50) <= 0xC7 && !(of<float>(obj, 0x20) == 400.0f))
        return;
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xEFFFFFFEu;
    body_free_parts(obj);
    task_exit();
}

// --- kinds 186 / 188 (Chapter 3 events 1, 2, 4-6): route objects ---

// 0x3D80 kind186_moving_object: its record 0xA85B0 + +0x70 x 0x18 (place, rise per frame +0xC, frames +0x10, model
// +0x14, end flag +0x16); kind186_moving_object_frame at once
template <class P> void Game<P>::kind186_moving_object(uint32_t obj)
{
    const uint32_t r = 0xA85B0 + uint32_t(int32_t(of<int16_t>(obj, 0x70))) * 0x18;
    of<int32_t>(obj, 0x50) = int32_t(mem.f32(r + 0x10));
    of<uint32_t>(obj, 0x20) = mem.u32(r);
    of<uint32_t>(obj, 0x24) = mem.u32(r + 4);
    of<uint32_t>(obj, 0) = F::kind186_moving_object_frame;
    of<uint32_t>(obj, 0x28) = mem.u32(r + 8);
    kind186_moving_object_frame(obj);
}

// 0x3DD0 kind186_moving_object_frame: ends when its event flag (0x51E940 + record +0x16) is set; drawn at its place;
// step 0: rises by record +0xC for record +0x10 frames, then step 1 (stays)
template <class P> void Game<P>::kind186_moving_object_frame(uint32_t obj)
{
    const uint32_t r = 0xA85B0 + uint32_t(int32_t(of<int16_t>(obj, 0x70))) * 0x18;
    if (mem.at<uint8_t>(0x51E940 + mem.at<uint16_t>(r + 0x16)) != 0) {
        task_exit();
        return;
    }
    cam_copro(0x05, {});
    cam_copro(0x12, {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)});
    geo_draw_model(mem.at<uint16_t>(r + 0x14));
    cam_copro(0x06, {});
    int16_t s = of<int16_t>(obj, 0x60);
    if (s == 0) {
        of<float>(obj, 0x24) = of<float>(obj, 0x24) + mem.f32(r + 0xC);
        const int32_t t = of<int32_t>(obj, 0x50) - 1;
        of<int32_t>(obj, 0x50) = t;
        if (t == 0)
            s = int16_t(s + 1);
    }
    of<int16_t>(obj, 0x60) = s;
}

// 0x3E90 kind188_route_spot (Chapter 3 event 4): an unseen target (543.1, -93.0, 1434.9, radius 20) once 0x51E9B0
// is set; each shot: sound 0xA0120B and a spark at the shot (kind188_spark); three shots: route_choice 1, sounds
// 0xA01249 / 0xA01102, ends; 0x51E9B1 set first: route 0, ends
template <class P> void Game<P>::kind188_route_spot(uint32_t obj)
{
    const int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        of<uint32_t>(obj, 0x20) = 0x44090893u;
        of<uint32_t>(obj, 0x24) = 0xC2BA0B44u;
        of<uint32_t>(obj, 0x28) = 0x44B35D71u;
        of<uint32_t>(obj, 0x90) = 0x41A00000u;
        of<int32_t>(obj, 0x54) = 0;
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | 0x401;
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
    }
    if (sub == 0 || sub == 1) {
        if (mem.at<uint8_t>(0x51E9B0) == 0)
            return;
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
    }
    if (sub <= 2) {
        if ((of<uint16_t>(obj, 0x58) >> 8) & 1) {
            of<int32_t>(obj, 0x54) = of<int32_t>(obj, 0x54) + 1;
            sound_cue(0xA0120B);
            uint32_t pl = 0x51F280 + mem.u32(0xA8670 + (of<uint32_t>(obj, 0x58) & 6) * 2) * 0xCC;
            if (coop4_on())   // 4P: the spark at the real shooter's aim (the hit table)
                pl = player_rec(coop4_shooter(obj, int(mem.u32(0xA8670 + (of<uint32_t>(obj, 0x58) & 6) * 2)))) - 4;
            const uint32_t t = task_create(F::kind188_spark, 0x58);
            of<int32_t>(t, 0x54) = 0x8B;
            const float z = of<float>(obj, 0x40);
            const float sx = float(int32_t(mem.at<int16_t>(pl + 0x34))) * z / 496.0f;
            const float sy = float(int32_t(mem.at<int16_t>(pl + 0x36))) * z / 496.0f;
            cam_copro(0x74, {1});
            const Tgp::Out p = cam_copro(0x1A, {fbits(sx), fbits(sy), fbits(z)});
            of<uint32_t>(t, 0x20) = p.w[0];
            of<uint32_t>(t, 0x24) = p.w[1];
            of<uint32_t>(t, 0x28) = p.w[2];
            of<uint32_t>(t, 0x28) = of<uint32_t>(obj, 0x28);
            cam_copro(0x06, {});
        }
        if (mem.at<uint8_t>(0x51E9B1) != 0) {
            mem.at<uint8_t>(G::route_choice) = 0;
            task_exit();
            return;
        }
        if (of<int32_t>(obj, 0x54) > 2) {
            mem.at<uint8_t>(G::route_choice) = 1;
            sound_cue(0xA01249);
            sound_cue(0xA01102);
            task_exit();
            return;
        }
    }
    const Tgp::Out v = cam_copro(0x1A, {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)});
    of<uint32_t>(obj, 0x38) = v.w[0];
    of<uint32_t>(obj, 0x3C) = v.w[1];
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~0x100u;
    of<uint32_t>(obj, 0x40) = v.w[2];
    depth_list_add(obj);
}

// 0x4E30 kind188_spark: the spark's frames 0x8B-0x9F (scale 1), then kind188_spark_anim
// Proven on A (vf_c3a: 21 captures, 0 differ)
template <class P> void Game<P>::kind188_spark(uint32_t obj)
{
    const int32_t m = of<int32_t>(obj, 0x54) + 1;
    of<int32_t>(obj, 0x54) = m;
    if (m == 0xA0)
        of<uint32_t>(obj, 0) = F::kind188_spark_anim;
    cam_copro(0x05, {});
    cam_copro(0x12, {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)});
    cam_copro(0x13, {0x3F800000u, 0x3F800000u, 0x3F800000u});
    geo_draw_model(of<uint32_t>(obj, 0x54));
    cam_copro(0x06, {});
}

// 0x4D90 kind188_spark_anim: the mark left (models 0x6B-0x74 by the frame counter 0x51F428 / 3, scale 0.65) until
// 0x51E9F2 / 0x51E9F3
// UNVERIFIED (teaser)
template <class P> void Game<P>::kind188_spark_anim(uint32_t obj)
{
    if (mem.at<uint8_t>(0x51E9F2) != 0 || mem.at<uint8_t>(0x51E9F3) != 0) {
        task_exit();
        return;
    }
    of<int32_t>(obj, 0x54) = (mem.at<int32_t>(0x51F428) / 3) % 10 + 0x6B;
    cam_copro(0x05, {});
    cam_copro(0x12, {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)});
    cam_copro(0x13, {0x3F266666u, 0x3F266666u, 0x3F266666u});
    geo_draw_model(of<uint32_t>(obj, 0x54));
    cam_copro(0x06, {});
}

// --- the player's Chapter 3 cut-scene actions (CCTB 18; the task's +0x20 = the player) ---

// 0x4DA40 player_scene15_jump_down_kb: the body at the table 0xC0FF8 ([players in game][player]), facing 0; player 2
// of two waits 20 frames (0x4DBB0), else jumps down at once (motion 0x17B, 0x4DB30); camera flag on
template <class P> void Game<P>::player_scene15_jump_down_kb(uint32_t task)
{
    const uint32_t p = uint32_t(int32_t(mem.at<int8_t>(task + 0x20)));
    Player& pl = mem.at<Player>(G::players, p);
    const uint32_t body = pl.body.a;
    const uint32_t at = 0xC0FF8 + p * 0xC + uint32_t(int32_t(mem.at<int16_t>(G::players_in_game))) * 0x18;
    Obj& o = mem.at<Obj>(body);
    o.pos[0] = mem.f32(at);
    o.pos[1] = mem.f32(at + 4);
    o.ang[0] = 0;
    o.ang[1] = 0;
    o.ang[2] = 0;
    o.pos[2] = mem.f32(at + 8);
    if (mem.at<uint16_t>(G::players_in_game) == 2 && mem.at<uint8_t>(task + 0x20) == 1) {
        pl.timer = 20;
        pl.scene_routine = 0x4DBB0;
        pl.cam_flags = uint8_t(pl.cam_flags & ~1);
        return;
    }
    pl.cam_flags = uint8_t(pl.cam_flags | 1);
    set_motion(body, 0x17B);
    mem.at<Player>(G::players, p).scene_routine = 0x4DB30;
}

// 0x4DB30: anim_time 33: sound 0xA01223; at 96 held at 95 (0x4DB90)
template <class P> void Game<P>::player_scene15_jump_down_kb_p1(uint32_t task)
{
    Player& pl = mem.at<Player>(G::players, uint32_t(int32_t(mem.at<int8_t>(task + 0x20))));
    Obj& o = mem.at<Obj>(pl.body.a);
    if (o.anim_time == 33) {
        sound_cue(0xA01223);
        return;
    }
    if (o.anim_time == 96) {
        o.anim_time = 0x5F;
        pl.scene_routine = 0x4DB90;
    }
}

// 0x4DB90: held at anim_time 95
template <class P> void Game<P>::player_scene15_jump_down_kb_p1_p(uint32_t task)
{
    mem.at<Obj>(mem.at<Player>(G::players, uint32_t(int32_t(mem.at<int8_t>(task + 0x20)))).body.a).anim_time = 0x5F;
}

// 0x4DBB0: player 2 after its wait: camera flag on, motion 0xC5, 0x4DC00
// Proven on A (vf_c3b: 7 captures, 0 differ)
template <class P> void Game<P>::player_scene15_jump_down_kb_p2(uint32_t task)
{
    Player& pl = mem.at<Player>(G::players, uint32_t(int32_t(mem.at<int8_t>(task + 0x20))));
    const uint32_t body = pl.body.a;
    pl.timer = pl.timer - 1;
    if (pl.timer != 0)
        return;
    pl.cam_flags = uint8_t(pl.cam_flags | 1);
    set_motion(body, 0xC5);
    pl.scene_routine = 0x4DC00;
}

// 0x4DC00: anim_time 33: sound 0xA01225; at 77 held at 76 (0x4DC60)
// Proven on A (vf_c3b: 26 captures, 0 differ)
template <class P> void Game<P>::player_scene15_jump_down_kb_p2_p(uint32_t task)
{
    Player& pl = mem.at<Player>(G::players, uint32_t(int32_t(mem.at<int8_t>(task + 0x20))));
    Obj& o = mem.at<Obj>(pl.body.a);
    if (o.anim_time == 33)
        sound_cue(0xA01225);
    if (o.anim_time == 0x4D) {
        o.anim_time = 0x4C;
        pl.scene_routine = 0x4DC60;
    }
}

// 0x4DC60: held at anim_time 76
// Proven on A (vf_c3b: 5 captures, 0 differ)
template <class P> void Game<P>::player_scene15_jump_down_kb_p2_p_p(uint32_t task)
{
    mem.at<Obj>(mem.at<Player>(G::players, uint32_t(int32_t(mem.at<int8_t>(task + 0x20)))).body.a).anim_time = 0x4C;
}

// 0x4DC80 player_scene16_run_jump: the scene camera (eye / target into camera_records +0x154, camera_look_at), the
// body at the table 0xC1028 (z - 51.49), facing 0, motion 0x173; player 2 of two starts at a random frame (0x4DED0),
// else after 45 frames from frame 3 (0x4DE00)
template <class P> void Game<P>::player_scene16_run_jump(uint32_t task)
{
    const uint32_t rec = G::camera_records + 0x154;
    mem.set32(rec + 0x18, 0x4408070Au);
    mem.set32(rec, 0x440D0917u);
    mem.set32(rec + 4, 0xC397DEB8u);
    mem.set32(rec + 8, 0x452A21C3u);
    mem.set32(rec + 0x1C, 0xC39EDED9u);
    mem.set32(rec + 0x20, 0x45288C52u);
    const uint32_t p = uint32_t(int32_t(mem.at<int8_t>(task + 0x20)));
    const uint32_t body = mem.at<Player>(G::players, p).body.a;
    camera_look_at(rec, rec + 0x18, 0);
    mem.set32(G::camera_view, 1);
    const uint32_t at = 0xC1028 + p * 0xC + uint32_t(int32_t(mem.at<int16_t>(G::players_in_game))) * 0x18;
    Obj& o = mem.at<Obj>(body);
    o.pos[0] = mem.f32(at);
    o.pos[1] = mem.f32(at + 4);
    o.ang[0] = 0;
    o.ang[1] = 0;
    o.ang[2] = 0;
    o.pos[2] = mem.f32(at + 8) - ffrom(0x424DF47Eu);
    set_motion(body, 0x173);
    Player& pl = mem.at<Player>(G::players, p);
    if (mem.at<uint16_t>(G::players_in_game) == 2 && mem.at<uint8_t>(task + 0x20) == 1) {
        const int32_t r = int32_t(rand()) % 14;
        mem.at<Obj>(body).anim_time = r * (int32_t(mem.at<int8_t>(task + 0x20)) + 1);
        pl.scene_routine = F::player_scene16_run_jump_p2;
        return;
    }
    pl.timer = 45;
    mem.at<Obj>(body).anim_time = 3;
    pl.scene_routine = 0x4DE00;
}

// 0x4DE00: after the wait: motion 0x17A, 0x4DE50
template <class P> void Game<P>::player_scene16_run_jump_p(uint32_t task)
{
    Player& pl = mem.at<Player>(G::players, uint32_t(int32_t(mem.at<int8_t>(task + 0x20))));
    const uint32_t body = pl.body.a;
    pl.timer = pl.timer - 1;
    if (pl.timer != 0)
        return;
    set_motion(body, 0x17A);
    pl.scene_routine = 0x4DE50;
}

// 0x4DE50: 0x51EA3C set or the motion ended: held (anim_time back one each frame, 0x4DEA0)
template <class P> void Game<P>::player_scene16_run_jump_p_p(uint32_t task)
{
    Player& pl = mem.at<Player>(G::players, uint32_t(int32_t(mem.at<int8_t>(task + 0x20))));
    Obj& o = mem.at<Obj>(pl.body.a);
    if (mem.at<uint8_t>(0x51EA3C) == 0 && !((mem.at<uint8_t>(pl.body.a + 0x3C5) >> 4) & 1))
        return;
    o.anim_time = o.anim_time - 1;
    pl.scene_routine = 0x4DEA0;
}

// 0x4DEA0: held (anim_time back one each frame)
template <class P> void Game<P>::player_scene16_run_jump_p_p_p(uint32_t task)
{
    Obj& o = mem.at<Obj>(mem.at<Player>(G::players, uint32_t(int32_t(mem.at<int8_t>(task + 0x20)))).body.a);
    o.anim_time = o.anim_time - 1;
}

// 0x4DED0 (player 2 of two): 0x51EA3C set: held (0x4DF20)
// Proven on A (vf_c3b: 35 captures, 0 differ)
template <class P> void Game<P>::player_scene16_run_jump_p2(uint32_t task)
{
    Player& pl = mem.at<Player>(G::players, uint32_t(int32_t(mem.at<int8_t>(task + 0x20))));
    if (mem.at<uint8_t>(0x51EA3C) == 0)
        return;
    Obj& o = mem.at<Obj>(pl.body.a);
    o.anim_time = o.anim_time - 1;
    pl.scene_routine = F::player_scene16_run_jump_p2_p;
}

// 0x4DF20: held (anim_time back one each frame)
// Proven on A (vf_c3b: 101 captures, 0 differ)
template <class P> void Game<P>::player_scene16_run_jump_p2_p(uint32_t task)
{
    Obj& o = mem.at<Obj>(mem.at<Player>(G::players, uint32_t(int32_t(mem.at<int8_t>(task + 0x20)))).body.a);
    o.anim_time = o.anim_time - 1;
}

// --- a shared hit effect reached first in Chapter 3 ---

// 0x43860 part_hit_class17 (part): a body part of damage effect 2 hit: the flash part_class17_anim at the part's
// camera-space point 3 nearer, back in the world (stored matrix 1), models from 0x1B80
template <class P> void Game<P>::part_hit_class17(uint32_t part)
{
    if constexpr (P::pc) {
        (void)part;   // the PC form: not ported (vtable only)
    } else {
        const uint32_t t = task_create(F::part_class17_anim, 0x60);
        cam_copro(0x74, {1});
        const Tgp::Out p = cam_copro(0x1A, {mem.u32(part + 0x5C), mem.u32(part + 0x60), fbits(mem.f32(part + 0x64) - 3.0f)});
        mem.set32(t + 0x20, p.w[0]);
        mem.set32(t + 0x24, p.w[1]);
        cam_copro(0x06, {});
        mem.set32(t + 0x54, 0x1B80);
        mem.set32(t + 0x28, p.w[2]);
    }
}

// 0x450B0 part_class17_anim (task): two models a frame up to 0x1B94; drawn at its point facing the camera (the
// matrix 0xAA690, coprocessor 0x87), scale 4
template <class P> void Game<P>::part_class17_anim(uint32_t t)
{
    const uint32_t m = mem.u32(t + 0x54) + 2;
    mem.set32(t + 0x54, m);
    if (int32_t(m) > 0x1B94) {
        task_exit();
        return;
    }
    cam_copro(0x05, {});
    cam_copro(0x12, {mem.u32(t + 0x20), mem.u32(t + 0x24), mem.u32(t + 0x28)});
    uint32_t w[9];
    for (uint32_t i = 0; i < 9; ++i)
        w[i] = mem.u32(0xAA690 + 4 * i);
    Tgp::Out none;
    copro(0x87, w, 9, none);
    cam_copro(0x13, {0x40800000u, 0x40800000u, 0x40800000u});
    geo_draw_model(mem.u32(t + 0x54));
    cam_copro(0x06, {});
}

// --- kind 70 helpers first reached in Chapter 3 ---

// 0x2FD30 room_ceiling_bounce: the next step in camera space (stored matrix 0); beyond the ceiling plane (0x51FE4C,
// its depth there): +0x324 bit 7, put on the plane (back through stored matrix 1), x / z speed x -0.2; the first such
// hit (+0x334 0, +0x328 < 2): sound 0xA01224, wall_dust_spawn, +0x334 = 1; returns 1 (hit) or -1
template <class P> uint32_t Game<P>::room_ceiling_bounce(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;   // the PC form: not ported (vtable only)
        return 0;
    }
    cam_copro(0x74, {0});
    const Tgp::Out c = cam_copro(0x1A, {fbits(of<float>(obj, 0x20) + of<float>(obj, 0x78)), of<uint32_t>(obj, 0x24),
                                        fbits(of<float>(obj, 0x28) + of<float>(obj, 0x80))});
    const uint32_t lim = mem.u32(0x51FE4C);
    cam_copro(0x06, {});
    if (ffrom(c.w[2]) > ffrom(lim)) {
        of<uint32_t>(obj, 0x324) = of<uint32_t>(obj, 0x324) | 0x80;
        cam_copro(0x74, {1});
        const Tgp::Out w = cam_copro(0x1A, {c.w[0], c.w[1], lim});
        const float vx = float(double(of<float>(obj, 0x78)) * -0.2);
        const float vz = float(double(of<float>(obj, 0x80)) * -0.2);
        of<uint32_t>(obj, 0x20) = w.w[0];
        of<uint32_t>(obj, 0x24) = w.w[1];
        cam_copro(0x06, {});
        of<uint32_t>(obj, 0x28) = w.w[2];
        of<float>(obj, 0x78) = vx;
        of<float>(obj, 0x80) = vz;
    }
    if (of<uint8_t>(obj, 0x324) & 0x80) {
        of<uint32_t>(obj, 0x324) = of<uint32_t>(obj, 0x324) & 0xFFFFFA7Fu;
        if (of<int32_t>(obj, 0x334) == 0 && of<int32_t>(obj, 0x328) <= 1) {
            sound_cue(0xA01224);
            wall_dust_spawn(obj, 0u - of<uint32_t>(obj, 0x30));
            of<int32_t>(obj, 0x334) = 1;
        }
        return 1;
    }
    of<uint32_t>(obj, 0x324) = of<uint32_t>(obj, 0x324) & 0xFFFFFA7Fu;
    return 0xFFFFFFFFu;
}

// 0x30C00 impact_effect (task): models +0x54 up to +0x31C, one a frame, scale 1
template <class P> void Game<P>::impact_effect(uint32_t t)
{
    const int32_t m = of<int32_t>(t, 0x54) + 1;
    of<int32_t>(t, 0x54) = m;
    if (m > of<int32_t>(t, 0x31C)) {
        task_exit();
        return;
    }
    cam_copro(0x05, {});
    cam_copro(0x12, {of<uint32_t>(t, 0x20), of<uint32_t>(t, 0x24), of<uint32_t>(t, 0x28)});
    cam_copro(0x13, {0x3F800000u, 0x3F800000u, 0x3F800000u});
    geo_draw_model(of<uint32_t>(t, 0x54));
    cam_copro(0x06, {});
}

// 0x30C90 impact_effect_spawn (x, y, z, surface): an impact_effect task at the point moved 1 toward the camera
// (stored matrices 0 / 1); surfaces 3 / 10: models 0x8A-0x9E and sound 0xA0120A, 11: models 0x414-0x428, others none
template <class P> void Game<P>::impact_effect_spawn(float x, float y, float z, int surface)
{
    if constexpr (P::pc) {
        (void)x; (void)y; (void)z; (void)surface;   // the PC form: not ported (vtable only)
        return;
    }
    const uint32_t t = task_create(F::impact_effect, 0x3C8);
    cam_copro(0x74, {0});
    const Tgp::Out c = cam_copro(0x1A, {fbits(x), fbits(y), fbits(z)});
    of<uint32_t>(t, 0x38) = c.w[0];
    of<uint32_t>(t, 0x3C) = c.w[1];
    cam_copro(0x06, {});
    of<uint32_t>(t, 0x40) = c.w[2];
    cam_copro(0x74, {1});
    const Tgp::Out w = cam_copro(0x1A, {of<uint32_t>(t, 0x38), of<uint32_t>(t, 0x3C), fbits(of<float>(t, 0x40) - 1.0f)});
    of<uint32_t>(t, 0x20) = w.w[0];
    of<uint32_t>(t, 0x24) = w.w[1];
    cam_copro(0x06, {});
    of<uint32_t>(t, 0x28) = w.w[2];
    if (surface == 3 || surface == 10) {
        of<int32_t>(t, 0x54) = 0x8A;
        of<int32_t>(t, 0x31C) = 0x9E;
        sound_cue(0xA0120A);
    } else if (surface == 11) {
        of<int32_t>(t, 0x54) = 0x414;
        of<int32_t>(t, 0x31C) = 0x428;
    }
}

// --- the iron ball's chain (a task of 0x2B8 bytes: nodes of 36 bytes at +0x64 (place, its speed at +0x18), +0x58
// the link length, +0x5C the last node, +0x2A4 / +0x2A6 the ends held (hand / ball), +0x2A8 slack frames, +0x2AA end,
// +0x2AC the zombie, +0x2B0 the ball, +0x2B4 the floor) ---

// 0x490E0 iron_chain_start (owner, ball): the chain task (iron_chain_task), told to the ball (extra +0x10); from the
// hand (the owner's matrix, point (1.44, 0.45, 1.83), stored matrix 6) to the ball, 11 long in links of 1.375
// Proven on A (ch4p_full: 1 captures, 0 differ)
template <class P> void Game<P>::iron_chain_start(uint32_t owner, uint32_t ball)
{
    if constexpr (P::pc) {
        (void)owner; (void)ball;   // the PC form: not ported (vtable only)
        return;
    }
    const uint32_t x = of<uint32_t>(ball, 0x37C);
    const uint32_t t = task_create(F::iron_chain_task, 0x2B8);
    mem.set32(x + 0x10, t);
    of<uint32_t>(t, 0x2AC) = owner;
    cam_copro(0x74, {6});
    of<uint32_t>(t, 0x2B0) = ball;
    uint32_t m[12];
    for (uint32_t i = 0; i < 12; ++i)
        m[i] = of<uint32_t>(owner, 0x244 + 4 * i);
    Tgp::Out none;
    copro(0x09, m, 12, none);
    const Tgp::Out h = cam_copro(0x1A, {0x3FB851ECu, 0x3EE66666u, 0x3FEA3D71u});
    of<uint32_t>(t, 0x64) = h.w[0];
    of<uint32_t>(t, 0x68) = h.w[1];
    of<uint32_t>(t, 0x6C) = h.w[2];
    cam_copro(0x06, {});
    iron_chain_init(t, of<float>(t, 0x64), of<float>(t, 0x68), of<float>(t, 0x6C), of<float>(ball, 0x20), of<float>(ball, 0x24),
                    of<float>(ball, 0x28), 11.0f, 1.375f);
}

// 0x491F0 iron_chain_init (chain, hand x / y / z, ball x / y / z, length, link): length / link nodes; the links laid
// along the hand-ball direction (coprocessor 0x52: yaw, pitch, distance): straight from the hand, the rest bent in an
// arc (0x2D / 0x0A / 0x1D / 0x1E) to the ball; speeds 0, the floor
// Proven on A (ch4p_full: 1 captures, 0 differ)
template <class P> void Game<P>::iron_chain_init(uint32_t t, float hx, float hy, float hz, float bx, float by, float bz, float len,
                                                 float link)
{
    const int32_t n = int32_t(len / link);
    const float nf = float(n);
    const Tgp::Out a = cam_copro(0x52, {fbits(hx - bx), fbits(hy - by), fbits(hz - bz)});
    const uint32_t yaw = a.w[0], pitch = a.w[1];
    const float dist = ffrom(a.w[2]);
    of<int16_t>(t, 0x2A6) = 0;
    of<int16_t>(t, 0x2A4) = 0;
    of<int16_t>(t, 0x2AA) = 0;
    of<int32_t>(t, 0x5C) = n;
    of<float>(t, 0x58) = len / nf;
    const int32_t r8 = of<int32_t>(t, 0x5C);
    of<uint32_t>(t, 0x2B4) = mem.u32(G::floor_y);
    for (int32_t i = 0; i <= r8; ++i)
        for (uint32_t o = 0x70; o <= 0x84; o += 4)
            of<uint32_t>(t, uint32_t(i) * 36 + o) = 0;
    float b[64][3] = {};
    const int32_t cnt = r8 < 0 ? 0 : (r8 < 63 ? r8 : 63);
    const float lk = of<float>(t, 0x58);
    b[0][0] = 0.0f;
    b[0][1] = 0.0f;
    b[0][2] = dist;
    b[cnt][0] = b[cnt][1] = b[cnt][2] = 0.0f;
    const float two = lk + lk;
    float rem = dist;
    int32_t k = 1;
    if (rem > two) {
        float g4;
        do {
            b[k][0] = 0.0f;
            b[k][1] = 0.0f;
            g4 = rem - lk;
            rem = g4;
            b[k][2] = g4;
            ++k;
        } while (g4 > two && k < 63);
    }
    const float half = rem * 0.5f;
    const float q = half / lk;
    const float s2 = (q * half) / lk;
    const Tgp::Out r = cam_copro(0x2D, {fbits(1.0f - s2)});
    const Tgp::Out ang = cam_copro(0x0A, {fbits(q), r.w[0]});
    uint32_t g1 = ang.w[0] + 0xFFFFD555u;
    for (; k < r8 && k < 63; ++k) {
        b[k][0] = 0.0f;
        g1 += 0x2AAB;
        b[k][1] = ffrom(cam_copro(0x1D, {g1, fbits(of<float>(t, 0x58))}).w[0]);
        b[k][2] = ffrom(cam_copro(0x1E, {g1, fbits(of<float>(t, 0x58))}).w[0]);
    }
    cam_copro(0x21, {});
    cam_copro(0x15, {yaw});
    cam_copro(0x14, {pitch});
    for (int32_t i = 0; i <= r8 && i < 64; ++i) {
        const Tgp::Out p = cam_copro(0x1A, {fbits(b[i][0]), fbits(b[i][1]), fbits(b[i][2])});
        const uint32_t nd = t + uint32_t(i) * 36;
        of<float>(nd, 0x6C) = ffrom(p.w[2]) + bz;
        of<float>(nd, 0x68) = ffrom(p.w[1]) + by;
        of<float>(nd, 0x64) = ffrom(p.w[0]) + bx;
    }
    cam_copro(0x06, {});
}

// 0x48F20 iron_chain_task: the zombie dead (+0x320 bit 0): slack (iron_chain_slack); else the first node at its hand
// (the owner's matrix, stored matrix 1), the last at the ball (or the ball at the last node while +0x2A6 holds it),
// then iron_chain_simulate
// Proven on A (ch4p_full: 139 captures, 0 differ)
template <class P> void Game<P>::iron_chain_task(uint32_t t)
{
    const uint32_t owner = of<uint32_t>(t, 0x2AC);
    const int32_t n = of<int32_t>(t, 0x5C);
    if (of<uint8_t>(owner, 0x320) & 1) {
        of<int16_t>(t, 0x2A8) = 0;
        iron_chain_slack(t);
        return;
    }
    cam_copro(0x74, {1});
    uint32_t m[12];
    for (uint32_t i = 0; i < 12; ++i)
        m[i] = of<uint32_t>(owner, 0x244 + 4 * i);
    Tgp::Out none;
    copro(0x09, m, 12, none);
    const Tgp::Out h = cam_copro(0x1A, {0x3FB851ECu, 0x3EE66666u, 0x3FEA3D71u});
    of<uint32_t>(t, 0x64) = h.w[0];
    of<uint32_t>(t, 0x68) = h.w[1];
    const int16_t held = of<int16_t>(t, 0x2A6);
    cam_copro(0x06, {});
    of<uint32_t>(t, 0x6C) = h.w[2];
    const uint32_t ball = of<uint32_t>(t, 0x2B0);
    const uint32_t nd = t + uint32_t(n) * 36;
    if (held != 0) {
        of<uint32_t>(ball, 0x20) = of<uint32_t>(nd, 0x64);
        of<uint32_t>(ball, 0x24) = of<uint32_t>(nd, 0x68);
        of<uint32_t>(ball, 0x28) = of<uint32_t>(nd, 0x6C);
    } else {
        of<uint32_t>(nd, 0x64) = of<uint32_t>(ball, 0x20);
        of<uint32_t>(nd, 0x68) = of<uint32_t>(ball, 0x24);
        of<uint32_t>(nd, 0x6C) = of<uint32_t>(ball, 0x28);
    }
    iron_chain_simulate(t);
}

// 0x49050 iron_chain_slack: the zombie dead: first frame links of 18 / nodes, the hand end free (+0x2A4 1, +0x2A6
// 0); ends with the ball (+0x2AA); the last node at the ball, iron_chain_simulate
// Proven on A (ch4p_full: 1 captures, 0 differ)
template <class P> void Game<P>::iron_chain_slack(uint32_t t)
{
    int16_t c = of<int16_t>(t, 0x2A8);
    const int32_t n = of<int32_t>(t, 0x5C);
    if (c == 0) {
        const float l = float(18.0 / double(n));
        of<int16_t>(t, 0x2A6) = 0;
        of<int16_t>(t, 0x2A4) = 1;
        c = int16_t(c + 1);
        of<float>(t, 0x58) = l;
    }
    of<int16_t>(t, 0x2A8) = c;
    if (of<int16_t>(t, 0x2AA) != 0) {
        task_exit();
        return;
    }
    const uint32_t ball = of<uint32_t>(t, 0x2B0);
    const uint32_t nd = t + uint32_t(n) * 36;
    of<uint32_t>(nd, 0x64) = of<uint32_t>(ball, 0x20);
    of<uint32_t>(nd, 0x68) = of<uint32_t>(ball, 0x24);
    of<uint32_t>(nd, 0x6C) = of<uint32_t>(ball, 0x28);
    iron_chain_simulate(t);
}

// 0x49980 iron_chain_constrain_fwd (chain, from, to): each node after one of from..to put back at the link length
// from it
// Proven on A (ch4p_full: 59 captures, 0 differ)
template <class P> void Game<P>::iron_chain_constrain_fwd(uint32_t t, int32_t from, int32_t to)
{
    for (int32_t i = from; i <= to; ++i) {
        const uint32_t a = t + uint32_t(i) * 36, b = a + 36;
        const Tgp::Out d = cam_copro(0x1F, {of<uint32_t>(a, 0x64), of<uint32_t>(a, 0x68), of<uint32_t>(a, 0x6C),
                                            of<uint32_t>(b, 0x64), of<uint32_t>(b, 0x68), of<uint32_t>(b, 0x6C)});
        const float k = of<float>(t, 0x58) / ffrom(d.w[0]);
        const float y = (of<float>(b, 0x68) - of<float>(a, 0x68)) * k + of<float>(a, 0x68);
        const float z = (of<float>(b, 0x6C) - of<float>(a, 0x6C)) * k + of<float>(a, 0x6C);
        const float x = (of<float>(b, 0x64) - of<float>(a, 0x64)) * k + of<float>(a, 0x64);
        of<float>(b, 0x68) = y;
        of<float>(b, 0x6C) = z;
        of<float>(b, 0x64) = x;
    }
}

// 0x49A70 iron_chain_constrain_back (chain, from, to): from `to` down to `from`, each node put back at the link
// length from the one after it
// Proven on A (ch4p_full: 124 captures, 0 differ)
template <class P> void Game<P>::iron_chain_constrain_back(uint32_t t, int32_t from, int32_t to)
{
    for (int32_t i = to; i >= from; --i) {
        const uint32_t b = t + uint32_t(i) * 36, a = b + 36;
        const Tgp::Out d = cam_copro(0x1F, {of<uint32_t>(a, 0x64), of<uint32_t>(a, 0x68), of<uint32_t>(a, 0x6C),
                                            of<uint32_t>(b, 0x64), of<uint32_t>(b, 0x68), of<uint32_t>(b, 0x6C)});
        const float k = of<float>(t, 0x58) / ffrom(d.w[0]);
        const float y = (of<float>(b, 0x68) - of<float>(a, 0x68)) * k + of<float>(a, 0x68);
        const float z = (of<float>(b, 0x6C) - of<float>(a, 0x6C)) * k + of<float>(a, 0x6C);
        const float x = (of<float>(b, 0x64) - of<float>(a, 0x64)) * k + of<float>(a, 0x64);
        of<float>(b, 0x68) = y;
        of<float>(b, 0x6C) = z;
        of<float>(b, 0x64) = x;
    }
}

// 0x495C0 iron_chain_simulate: the free nodes (the held ends +0x2A4 / +0x2A6 stay) move by 0.65 of their speed and
// fall (0.025), not under the floor; the links kept (forward / back by the held ends); the new speeds; each link drawn
// (coprocessor 0x84: its angles and length; model 0xAFD40 by length up to 10 and texture area, stretched)
// Proven on A (ch4p_full: 130 captures, 0 differ)
template <class P> void Game<P>::iron_chain_simulate(uint32_t t)
{
    const int32_t n = of<int32_t>(t, 0x5C);
    float prev[64][3] = {};
    for (int32_t i = 0; i <= n && i < 64; ++i) {
        prev[i][0] = of<float>(t, uint32_t(i) * 36 + 0x64);
        prev[i][1] = of<float>(t, uint32_t(i) * 36 + 0x68);
        prev[i][2] = of<float>(t, uint32_t(i) * 36 + 0x6C);
    }
    const bool a4 = of<int16_t>(t, 0x2A4) != 0, a6 = of<int16_t>(t, 0x2A6) != 0;
    const int mode = (a4 ? 1 : 0) | (a6 ? 2 : 0);
    int32_t start, end;
    switch (mode) {
    case 0: start = 1; end = n - 1; break;
    case 1: start = 0; end = n - 1; break;
    case 2: start = 1; end = n; break;
    default: start = 0; end = n; break;
    }
    for (int32_t i = start; i <= end; ++i) {
        const uint32_t nd = t + uint32_t(i) * 36;
        const float vy = of<float>(nd, 0x80) * 0.65f;
        const float y = float(double(of<float>(nd, 0x68)) + (double(vy) - 0.025));
        const float vz = of<float>(nd, 0x84) * 0.65f;
        const float vx = of<float>(nd, 0x7C) * 0.65f;
        of<float>(nd, 0x64) = of<float>(nd, 0x64) + vx;
        of<float>(nd, 0x6C) = of<float>(nd, 0x6C) + vz;
        of<float>(nd, 0x68) = y;
        if (y < of<float>(t, 0x2B4))
            of<uint32_t>(nd, 0x68) = of<uint32_t>(t, 0x2B4);
    }
    if (mode == 0) {
        iron_chain_constrain_fwd(t, 0, n - 2);
        iron_chain_constrain_back(t, 1, n - 1);
    } else if (mode == 1) {
        iron_chain_constrain_back(t, 0, n - 1);
    } else {
        iron_chain_constrain_fwd(t, 0, n - 2);
    }
    for (int32_t i = 0; i <= n && i < 64; ++i) {
        const uint32_t nd = t + uint32_t(i) * 36;
        of<float>(nd, 0x7C) = of<float>(nd, 0x64) - prev[i][0];
        of<float>(nd, 0x80) = of<float>(nd, 0x68) - prev[i][1];
        of<float>(nd, 0x84) = of<float>(nd, 0x6C) - prev[i][2];
    }
    for (int32_t i = 0; i <= n - 1; ++i) {
        const uint32_t nd = t + uint32_t(i) * 36;
        const Tgp::Out a = cam_copro(0x84, {fbits(of<float>(nd, 0x88) - of<float>(nd, 0x64)),
                                            fbits(of<float>(nd, 0x8C) - of<float>(nd, 0x68)),
                                            fbits(of<float>(nd, 0x90) - of<float>(nd, 0x6C))});
        const float l = ffrom(a.w[2]);
        cam_copro(0x05, {});
        cam_copro(0x12, {of<uint32_t>(nd, 0x64), of<uint32_t>(nd, 0x68), of<uint32_t>(nd, 0x6C)});
        cam_copro(0x16, {a.w[0]});
        cam_copro(0x15, {a.w[1]});
        const int32_t idx = l <= 10.0f ? int32_t(l) : 10;
        of<int32_t>(t, 0x54) = mem.at<int16_t>(mem.u32(0xAFD40 + uint32_t(idx) * 8) + mem.u32(G::texture_area) * 2);
        cam_copro(0x13, {fbits(l / mem.f32(0xAFD44 + uint32_t(idx) * 8)), 0x3F800000u, 0x3F800000u});
        geo_draw_model(of<uint32_t>(t, 0x54));
        cam_copro(0x06, {});
    }
}

} // namespace hotd::port
