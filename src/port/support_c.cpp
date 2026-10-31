// Small routines first written in the runtime (proofs-7): task_create_child_alloc 0x15010, spin_axis_from_velocity
// 0x33EC0 (src/game/t1_bounds.cpp) and kind 182, the grey scene 0x9B600 .. 0x9B660 (src/game/palette.cpp), moved here
// for their capture proof. Arcade forms only (the PC's are replayed). Declarations: decl/support_b.h, core_bounds.h,
// hostages.h.
#include "port/game.h"

namespace hotd::port {

// 0x15010 task_create_child_alloc (routine, size, parent): a task right after `parent` in its ring (task_create_child
// with the parent given)
template <class P> uint32_t Game<P>::task_create_child_alloc(uint32_t routine, uint32_t size, uint32_t parent)
{
    if constexpr (P::pc) {
        (void)routine, (void)size, (void)parent;
        return 0;
    } else {
        const uint32_t t = mem_alloc(size);
        mem.set32(t + 0x10, parent);
        mem.set32(t + 0x14, mem.u32(parent + 0x14));
        mem.set32(mem.u32(parent + 0x14) + 0x10, t);
        mem.set32(parent + 0x14, t);
        mem.set32(t, routine);
        mem.set32(t + 0x18, parent);
        mem.set32(t + 4, 0x14E90);
        mem.set32(t + 0x1C, mem.u32(parent + 8));
        const uint32_t serial = mem.u32(0x5122A4);
        mem.set32(0x5122A4, serial + 1);
        mem.set32(t + 0xC, 0x80018691u);
        mem.set32(t + 8, serial);
        return t;
    }
}

// 0x33EC0 spin_axis_from_velocity (vel, out): normalize(-vx vy, vx^2 + vz^2, -vy vz) (coprocessor 0x0C), pointing up
// (y >= 0)
template <class P> void Game<P>::spin_axis_from_velocity(uint32_t vel, uint32_t out)
{
    if constexpr (!P::pc) {
        const float vx = mem.f32(vel), vy = mem.f32(vel + 4), vz = mem.f32(vel + 8);
        const uint32_t in[3] = {fbits(-vx * vy), fbits(vx * vx + vz * vz), fbits(-vy * vz)};
        Tgp::Out o;
        copro(0x0C, in, 3, o);
        uint32_t r[3] = {o.w[0], o.w[1], o.w[2]};
        if (ffrom(o.w[1]) < 0.0f)
            for (uint32_t& w : r)
                w ^= 0x80000000u;
        for (uint32_t k = 0; k < 3; ++k)
            mem.set32(out + 4 * k, r[k]);
    } else {
        (void)vel, (void)out;
    }
}

// --- kind 182: a grey scene (from event flag 0xFC for the record's frames) ---

// 0x9B600 kind182_grey_scene: to kind182_wait, run at once
template <class P> void Game<P>::kind182_grey_scene(uint32_t obj)
{
    if constexpr (!P::pc) {
        mem.set32(obj, 0x9B620);
        kind182_wait(obj);
    } else {
        (void)obj;
    }
}

// 0x9B620 kind182_wait: ends on a skip; at event flag 0xFC the grey look for +0x70 frames
template <class P> void Game<P>::kind182_wait(uint32_t obj)
{
    if constexpr (!P::pc) {
        if (mem.u32(G::evt_skip_end) != 0) {
            task_exit();
            return;
        }
        if (mem.template at<uint8_t>(G::event_flags + 0xFC) != 0) {
            palette_grey_build();
            mem.set32(obj, 0x9B660);
            mem.set32(obj + 0x50, uint32_t(int32_t(mem.template at<int16_t>(obj + 0x70))));
        }
    } else {
        (void)obj;
    }
}

// 0x9B660 kind182_grey_hold: counts down (or a skip), then the colours back and the task ends
template <class P> void Game<P>::kind182_grey_hold(uint32_t obj)
{
    if constexpr (!P::pc) {
        const int32_t t = int32_t(mem.u32(obj + 0x50)) - 1;
        mem.set32(obj + 0x50, uint32_t(t));
        if (t != 0 && mem.u32(G::evt_skip_end) == 0)
            return;
        palette_restore_vblank();
        task_exit();
    } else {
        (void)obj;
    }
}

// --- draw mode 1 (first written in the runtime: src/game/t1_bounds.cpp, T1) ---

// 0x37D80 object_draw_body: body_current = obj; the fade; the motion frame from anim_time (wrap, the frame-match bit 1
// of +0x3C7, the MotFinish step past frame 0), the frame record (0xC7540[motion] + frame x the joint words); push,
// translate to the position, rotate (z, y, x), scale +0x94, the frame root (x, y, -z) and angles; every part
// (body_part_draw_mat); pop; MotFinish = the loop count grew; not fully opaque: geo_send_words
template <class P> void Game<P>::object_draw_body(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;
        return;
    } else {
        const uint32_t root = mem.u32(obj + 0xC8);
        const uint32_t joints = mem.u32(0xC73C0 + uint32_t(int32_t(mem.template at<int16_t>(obj + 0xCC))) * 4);
        mem.set32(0x51FE2C, obj);   // body_current
        draw_fade_begin(int32_t(mem.template at<int16_t>(obj + 0x2E0)));
        const int32_t motion = mem.template at<int16_t>(obj + 0xD0);
        uint32_t anim = mem.u32(obj + 0xF8);
        const uint32_t count = mem.u32(G::motion_frame_counts + uint32_t(motion) * 4);
        uint32_t rem = count ? anim % count : 0;
        if (int32_t(count) > 1) {
            const uint8_t f3 = mem.template at<uint8_t>(obj + 0x3C7);
            const bool eq = int32_t(mem.template at<int16_t>(obj + 0xF4)) == int32_t(rem);
            mem.template at<uint8_t>(obj + 0x3C7) = uint8_t((f3 & ~2u) | (eq ? 2u : 0u));
        }
        mem.template at<int16_t>(obj + 0xF6) = int16_t(count ? anim / count : 0);
        if ((mem.u32(obj + 0x3C4) & 0x2001000u) == 0x1000u) {
            mem.template at<uint8_t>(obj + 0x3C5) &= 0xEF;
            if ((rem & 0xFFFF) == 0) {
                rem += 1;
                anim += 1;
            }
        }
        mem.set32(obj + 0xF8, anim);
        mem.template at<int16_t>(obj + 0xF4) = int16_t(rem);
        const uint32_t words = (joints * 48 + 0x7F) >> 5;
        const uint32_t fd = mem.u32(0xC7540 + uint32_t(motion) * 4) + uint32_t(int32_t(int16_t(rem)) * 4) * words;
        mem.set32(obj + 0xD8, fd);
        mem.set32(0x53E3A0, fd);
        Tgp::Out none;
        copro(0x05, nullptr, 0, none);
        uint32_t t[3] = {mem.u32(obj + 0x20), mem.u32(obj + 0x24), mem.u32(obj + 0x28)};
        copro(0x12, t, 3, none);
        uint32_t r[3] = {mem.u32(obj + 0x34), mem.u32(obj + 0x30), mem.u32(obj + 0x2C)};
        copro(0x86, r, 3, none);
        const uint32_t sc = mem.u32(obj + 0x94);
        uint32_t s[3] = {sc, sc, sc};
        copro(0x13, s, 3, none);
        uint32_t f[3] = {mem.u32(fd), mem.u32(fd + 4), mem.u32(fd + 8) ^ 0x80000000u};
        copro(0x12, f, 3, none);
        uint32_t a[3] = {0u - mem.template at<uint16_t>(fd + 0x10), mem.template at<uint16_t>(fd + 0xE), mem.template at<uint16_t>(fd + 0xC)};
        copro(0x86, a, 3, none);
        // PC Mode / Boss Mode: THOTD.EXE's draw type 1 0x478094 on PM's stack, in step (pushed; translate, turns
        // z / y / x, the 3 x 3 scaled, the frame root (x, y, -z) and its turns: the body file's helpers 0x474CE8)
        const bool pmb = pm_body();
        if (pmb) {
            pc_matrix_push();
            pm::Matrix& c = pm_cur();
            c.translate(mem.f32(obj + 0x20), mem.f32(obj + 0x24), mem.f32(obj + 0x28));
            c.rotate_z(mem.u32(obj + 0x34));
            c.rotate_y(mem.u32(obj + 0x30));
            c.rotate_x(mem.u32(obj + 0x2C));
            const float k = mem.f32(obj + 0x94);
            for (int i = 0; i < 9; ++i)
                c.e[i] = c.e[i] * k;
            c.translate(ffrom(f[0]), ffrom(f[1]), ffrom(f[2]));
            c.rotate_z(a[0]);
            c.rotate_y(a[1]);
            c.rotate_x(a[2]);
        }
        for (int32_t i = 0; i < int32_t(mem.u32(root + 0x34)); ++i)
            body_part_draw_mat(mem.u32(mem.u32(root + 0x24) + 4 * uint32_t(i)));
        copro(0x06, nullptr, 0, none);
        if (pmb)
            pcm_pop();
        const uint32_t loops = (mem.u32(obj + 0xF8) + 1) / mem.u32(G::motion_frame_counts + uint32_t(int32_t(mem.template at<int16_t>(obj + 0xD0))) * 4);
        const bool grew = uint32_t(int32_t(mem.template at<int16_t>(obj + 0xF6))) < loops;
        mem.template at<uint8_t>(obj + 0x3C5) = uint8_t((mem.template at<uint8_t>(obj + 0x3C5) & ~0x10u) | (grew ? 0x10u : 0u));
        if (mem.template at<uint16_t>(obj + 0x2E0) != 0xFF)
            geo_send_words(0, 31, 0x7A0);
    }
}

// 0x37B80 body_part_draw_mat(part): push; the part's offset (x, y, -z) and its joint's angles from the frame record
// (0x53E3A0 + 6 x joint); the body's home part (role class = 0x14A430[type]): its place through stored matrix 1 into
// the body's +0x2D4; the part's model (with +0x6C bit 1 a second pass: model 0x14BA80[type][joint] or the same);
// the children; pop
template <class P> void Game<P>::body_part_draw_mat(uint32_t part)
{
    if constexpr (P::pc) {
        (void)part;
        return;
    } else {
        const uint32_t body = mem.u32(0x51FE2C);
        const int32_t type = mem.template at<int16_t>(body + 0xCC);
        const uint32_t joint = mem.u32(part + 0x38);
        const int8_t cls = mem.template at<int8_t>(0x1184E0 + uint32_t(type * 31) * 8 + joint * 8 + 3);
        Tgp::Out none;
        copro(0x05, nullptr, 0, none);
        uint32_t t[3] = {mem.u32(part + 4), mem.u32(part + 8), mem.u32(part + 0xC) ^ 0x80000000u};
        copro(0x12, t, 3, none);
        const uint32_t rec = mem.u32(0x53E3A0) + joint * 6;
        uint32_t a[3] = {0u - mem.template at<uint16_t>(rec + 0x10), mem.template at<uint16_t>(rec + 0xE), mem.template at<uint16_t>(rec + 0xC)};
        copro(0x86, a, 3, none);
        // PC Mode / Boss Mode: THOTD.EXE's part routine 0x477EC4 on PM's stack (pushed, translate, turns z / y / x)
        const bool pmb = pm_body();
        if (pmb) {
            pc_matrix_push();
            pm::draw1_part(pm_cur(), mem.f32(part + 4), mem.f32(part + 8), mem.f32(part + 0xC), a[0], a[1], a[2]);
        }
        if (int32_t(cls) == int32_t(mem.u32(0x14A430 + uint32_t(type) * 4))) {
            copro(0x05, nullptr, 0, none);
            Tgp::Out v, o;
            copro(0x20, nullptr, 0, v);
            uint32_t one[1] = {1};
            copro(0x73, one, 1, none);
            uint32_t p[3] = {v.w[0], v.w[1], v.w[2]};
            copro(0x1A, p, 3, o);
            if (pmb) {   // the part's place through stored matrix 1 (copied in; the body file's transform 0x474CE8)
                float h[3];
                pm::draw1_home(pm_cur(), pm_camera, h);
                for (int k = 0; k < 3; ++k)
                    o.w[k] = fbits(h[k]);
            }
            mem.set32(body + 0x2D4, o.w[0]);
            mem.set32(body + 0x2DC, o.w[2]);
            mem.set32(body + 0x2D8, o.w[1]);
            copro(0x06, nullptr, 0, none);
        }
        copro(0x05, nullptr, 0, none);
        geo_draw_model(mem.u32(part));
        if ((mem.template at<uint8_t>(body + 0x6C) >> 1) & 1) {
            uint32_t three[1] = {3};
            copro(0x77, three, 1, none);
            const uint32_t tab = mem.u32(0x14BA80 + uint32_t(type) * 4);
            geo_draw_model(tab != 0 ? mem.u32(tab + joint * 4) : mem.u32(part));
        }
        copro(0x06, nullptr, 0, none);
        for (int32_t i = 0; i < int32_t(mem.u32(part + 0x34)); ++i)
            body_part_draw_mat(mem.u32(mem.u32(part + 0x24) + 4 * uint32_t(i)));
        copro(0x06, nullptr, 0, none);
        if (pmb)
            pcm_pop();
    }
}

// --- tumbling boxes and body drips (first written in the runtime: src/game/t1_bounds.cpp, T1) ---

// 0x6A970 tumble_corners(box): the 8 corners (x-/x+, y-/y+, z-/z+, z fastest) through the current matrix
//
template <class P> void Game<P>::tumble_corners(uint32_t box)
{
    if constexpr (P::pc) {
        (void)box;
    } else {
        uint32_t i = 0;
        for (uint32_t xo : {0u, 4u})
            for (uint32_t yo : {8u, 0xCu})
                for (uint32_t zo : {0x10u, 0x14u}) {
                    uint32_t in[3] = {mem.u32(box + xo), mem.u32(box + yo), mem.u32(box + zo)};
                    Tgp::Out o;
                    copro(0x1A, in, 3, o);
                    for (uint32_t k = 0; k < 3; ++k)
                        mem.set32(0x5434C0 + 12 * i + 4 * k, o.w[k]);
                    ++i;
                }
    }
}

// 0x6ADD0 tumble_update(obj, box): state 0: any corner at or under the ground (ground_height of the corner raised 5)
// starts the tumble (state 1 -> tumble_start, 2); state 2: tumble_bounce
template <class P> void Game<P>::tumble_update(uint32_t obj, uint32_t box)
{
    if constexpr (P::pc) {
        (void)obj, (void)box;
    } else {
        int32_t st = int32_t(mem.u32(box + 0x34));
        if (st > 2 || st < 0)
            return;
        if (st == 0) {
            LocalScope scope(*this);
            const uint32_t p = local_alloc(12);
            bool hit = false;
            for (uint32_t i = 0; i < 8 && !hit; ++i) {
                const uint32_t c = 0x5434C0 + 12 * i;
                mem.set32(p, mem.u32(c));
                mem.set32(p + 4, fbits(5.0f + mem.f32(c + 4)));
                mem.set32(p + 8, mem.u32(c + 8));
                if (mem.f32(c + 4) <= ground_height(p))
                    hit = true;
            }
            if (!hit)
                return;
            mem.set32(box + 0x34, 1);
            st = 1;
        }
        if (st == 1) {
            tumble_start(obj, box);
            mem.set32(box + 0x34, mem.u32(box + 0x34) + 1);
        }
        tumble_bounce(obj, box);
    }
}

// PC Mode (docs/pcmode.md): the PC's tumble_update 0x4C5B1C for the kit's thrown objects (held_object_flight), on the
// arcade's corner buffer 0x5434C0 (filled by pm_tumble_corners, the PC's 0x4C5924) and pivot 0x5434B0 (PC 0xDA61E0 /
// 0xDA6250). States as the arcade's (0: the ground test 0x4C5A10, any corner's ground_height under it raised 5 at or
// over it starts; 1: tumble_start; 2: tumble_bounce; 3 and above: nothing), but the start and the bounce are the PC's
// own maths (pm::tumble_start 0x4C5B70, pm::tumble_bounce 0x4C5DC4: table square roots, the PC's matrices, its axis
// turn 0x4C1DD0 and matrix angles) instead of the arcade's coprocessor (and the runtime's stand-in for its unmodelled
// 0x85). The ground height is the PC's ground_height 0x48A254, which is floor_y itself (`fld [floor_y]; ret`: the PC
// has no floor search there; pmrest): a corner at or under floor_y starts it. Also the kind 38 barrel's tumble
// (prop_kinds.cpp)
template <class P> void Game<P>::pm_tumble_update(uint32_t obj, uint32_t box)
{
    if constexpr (P::pc) {
        (void)obj, (void)box;
    } else {
        const uint32_t st = mem.u32(box + 0x34);
        if (st > 2)
            return;
        if (st == 0) {
            // 0x4C5A10: the PC's ground_height 0x48A254 is floor_y itself (the corner raised 5 is not read)
            bool hit = false;
            for (uint32_t i = 0; i < 8 && !hit; ++i)
                hit = !(mem.f32(G::floor_y) < mem.f32(0x5434C0 + 12 * i + 4));   // (fcomp; jb: unordered = a hit)
            if (!hit)
                return;
            mem.set32(box + 0x34, 1);
        }
        pm::TumbleBox b;
        for (uint32_t k = 0; k < 18; ++k)
            b.w[k] = mem.u32(box + 4 * k);
        if (b.w[13] == 1) {
            pm::tumble_start(of<uint32_t>(obj, 0x2C), of<uint32_t>(obj, 0x30), of<uint32_t>(obj, 0x34), b);
            b.w[13] = b.w[13] + 1;
        }
        float pos[3] = {of<float>(obj, 0x20), of<float>(obj, 0x24), of<float>(obj, 0x28)};
        uint32_t ang[3] = {of<uint32_t>(obj, 0x2C), of<uint32_t>(obj, 0x30), of<uint32_t>(obj, 0x34)};
        float corners[24], pivot[3];
        for (uint32_t i = 0; i < 24; ++i)
            corners[i] = mem.f32(0x5434C0 + 4 * i);
        for (uint32_t k = 0; k < 3; ++k)
            pivot[k] = mem.f32(0x5434B0 + 4 * k);
        pm::tumble_bounce(pos, ang, b, corners, pivot, mem.f32(G::floor_y));
        for (uint32_t k = 0; k < 18; ++k)
            mem.set32(box + 4 * k, b.w[k]);
        for (uint32_t i = 0; i < 24; ++i)
            mem.set32(0x5434C0 + 4 * i, fbits(corners[i]));
        for (uint32_t k = 0; k < 3; ++k) {
            mem.set32(0x5434B0 + 4 * k, fbits(pivot[k]));
            of<float>(obj, 0x20 + 4 * k) = pos[k];
            of<uint32_t>(obj, 0x2C + 4 * k) = ang[k];
        }
    }
}

// 0x6AF70 tumble_start(obj, box): of the object's six axis directions (+-x, +-y, +-z turned by its angles) the one
// pointing most up is the local "up" (+0x3C); the rock axis (+0x28) = normalize((0, 1, 0) x up in world); 3
// crossings, angle 32, speed 8, side 1
template <class P> void Game<P>::tumble_start(uint32_t obj, uint32_t box)
{
    if constexpr (P::pc) {
        (void)obj, (void)box;
    } else {
        Tgp::Out none, a, b, c;
        copro(0x21, nullptr, 0, none);
        uint32_t r[3] = {mem.u32(obj + 0x34), mem.u32(obj + 0x30), mem.u32(obj + 0x2C)};
        copro(0x86, r, 3, none);
        uint32_t ex[3] = {fbits(1.0f), 0, 0}, ey[3] = {0, fbits(1.0f), 0}, ez[3] = {0, 0, fbits(1.0f)};
        copro(0x1A, ex, 3, a);
        copro(0x1A, ey, 3, b);
        copro(0x1A, ez, 3, c);
        const float ys[6] = {ffrom(a.w[1]), ffrom(b.w[1]), ffrom(c.w[1]), -ffrom(a.w[1]), -ffrom(b.w[1]), -ffrom(c.w[1])};
        int best = 0;
        float m = ys[0];
        for (int i = 1; i < 6; ++i)
            if (ys[i] > m) {
                m = ys[i];
                best = i;
            }
        const uint32_t row = 0x6AF10 + uint32_t(best) * 0x10;
        for (uint32_t k = 0; k < 3; ++k)
            mem.set32(box + 0x3C + 4 * k, mem.u32(row + 4 * k));
        uint32_t up[3] = {mem.u32(box + 0x3C), mem.u32(box + 0x40), mem.u32(box + 0x44)};
        Tgp::Out w;
        copro(0x1A, up, 3, w);
        const float ux = mem.f32(0xC6820), uy = mem.f32(0xC6824), uz = mem.f32(0xC6828);
        const float wx = ffrom(w.w[0]), wy = ffrom(w.w[1]), wz = ffrom(w.w[2]);
        uint32_t n[3] = {fbits(uy * wz - uz * wy), fbits(uz * wx - ux * wz), fbits(ux * wy - uy * wx)};
        Tgp::Out o;
        copro(0x0C, n, 3, o);
        mem.set32(box + 0x28, o.w[0]);
        mem.set32(box + 0x2C, o.w[1]);
        copro(0x06, nullptr, 0, none);
        mem.set32(box + 0x20, 3);
        mem.set32(box + 0x1C, 32);
        mem.set32(box + 0x18, 8);
        mem.set32(box + 0x38, 1);
        mem.set32(box + 0x30, o.w[2]);
    }
}

// 0x60430 body_drips_draw(part) (a prop body, Ebitan): joints 2 / 3 / 7 drip (0x543430 + joint: -1 idle, 1 in 8
// frames a drip starts; models 0x5EA + n, 30 frames, at the part's camera-space place); joint 2 also the vomit stream
// (0x543438 = 5 or 0x543439 > 0: models 0x20A + n, turned by the camera's yaw + 0xC800 (0x7200 after chapter 12),
// pitch 0x3000, scale 1.2; at 40 frames 0x543438 = 6, at 90 the stream restarts); then the children
template <class P> void Game<P>::body_drips_draw(uint32_t part)
{
    if constexpr (P::pc) {
        (void)part;
    } else {
        const uint32_t j = mem.template at<uint8_t>(part + 0x38);
        Tgp::Out none;
        if (j == 2 || j == 3 || j == 7) {
            int8_t& d = mem.template at<int8_t>(0x543430 + j);
            if (d < 0 && int32_t(rand()) % 8 == 1)
                d = 0;
            if (d > -1) {
                copro(0x10, nullptr, 0, none);
                uint32_t t[3] = {mem.u32(part + 0x5C), mem.u32(part + 0x60), mem.u32(part + 0x64)};
                copro(0x88, t, 3, none);
                uint32_t s[3] = {fbits(1.0f), fbits(1.0f), fbits(1.0f)};
                copro(0x13, s, 3, none);
                const uint8_t n = uint8_t(d);
                d = int8_t(n + 1);
                geo_draw_model(uint32_t(n) + 0x5EA);
                if (d > 29)
                    d = -1;
            }
        }
        if (j == 2 && (mem.template at<uint8_t>(0x543438) == 5 || mem.template at<int8_t>(0x543439) > 0)) {
            copro(0x10, nullptr, 0, none);
            uint32_t t[3] = {mem.u32(part + 0x5C), mem.u32(part + 0x60), mem.u32(part + 0x64)};
            copro(0x88, t, 3, none);
            const uint32_t yaw = mem.u32(0x51EAC0) + (mem.template at<int8_t>(0x51F9D0) > 11 ? 0x7200u : 0xC800u);
            uint32_t r[3] = {0, yaw, 0x3000};
            copro(0x86, r, 3, none);
            uint32_t s[3] = {0x3F99999Au, 0x3F99999Au, 0x3F99999Au};
            copro(0x13, s, 3, none);
            const uint8_t n = mem.template at<uint8_t>(0x543439);
            mem.template at<uint8_t>(0x543439) = uint8_t(n + 1);
            geo_draw_model(uint32_t(n) + 0x20A);
            if (mem.template at<uint8_t>(0x543439) == 40)
                mem.template at<uint8_t>(0x543438) = 6;
            else if (mem.template at<uint8_t>(0x543439) == 0x5A)
                mem.template at<uint8_t>(0x543439) = 0;
        }
        for (uint32_t i = 0; i < mem.u32(part + 0x34); ++i)
            body_drips_draw(mem.u32(mem.u32(part + 0x24) + 4 * i));
    }
}

} // namespace hotd::port
