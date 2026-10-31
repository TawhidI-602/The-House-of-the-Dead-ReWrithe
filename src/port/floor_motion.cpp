// Floors (the scene's floor quads under a point), jump arcs (kind 70, the frogs, the Molb) and moving along the
// heading. First written as runtime overrides (T1, src/game/t1_bounds.cpp), read from the disassembly. Arcade forms
// (decl/support.h).
#include "port/game.h"

#include <cstring>

namespace hotd::port {

namespace {
constexpr uint32_t kFloorUp = 0x45FD0;   // the default floor normal (0, 1, 0) (arcade)
} // namespace

// 0x45FE0 floor_probe(point, outPoint, outNormal, outAttr): the scene's floor quads (scene_quads +0x61 lists of
// shot_list_b) ray-tested downward (coprocessor 0x62, direction (0, -1, 0)) from the point; the first hit: its point,
// plane normal and attribute, 1. None: (x, floor_y, z), normal (0, 1, 0), -1
template <class P> uint32_t Game<P>::floor_probe(uint32_t point, uint32_t outA, uint32_t outB, uint32_t outC)
{
    if constexpr (P::pc) {
        (void)point, (void)outA, (void)outB, (void)outC;   // the PC form: not ported (vtable only; ProofPc replays it)
        return 0;
    } else {
        const uint32_t scene = mem.u32(G::scene_quads);
        // PC Mode / Boss Mode: THOTD.EXE's floor_probe 0x48A15C tests its list with 0x489330, an empty stub (it only
        // clears the outputs): in the PC captures no floor quad is ever hit (Chapter 1's kind 11 death at a water quad
        // does not sink, pmbody), so the no-floor answer, as ground_height's
        const uint32_t lists = pc_path() ? 0u : uint32_t(mem.at<uint8_t>(scene + 0x61));
        for (uint32_t i = 0; i < lists; ++i) {
            uint32_t in[7] = {mem.u32(G::shot_list_b + 4 * i), mem.u32(point), mem.u32(point + 4), mem.u32(point + 8), 0,
                              fbits(-1.0f), 0};
            Tgp::Out hit;
            copro(0x62, in, 7, hit);
            if (ffrom(hit.w[0]) > 0.0f) {
                for (uint32_t k = 0; k < 3; ++k) {
                    mem.set32(outA + 4 * k, hit.w[1 + k]);
                    mem.set32(outB + 4 * k, hit.w[5 + k]);
                }
                mem.set32(outC, hit.w[8]);
                return 1;
            }
        }
        mem.set32(outA, mem.u32(point));
        mem.set32(outA + 4, mem.u32(G::floor_y));
        mem.set32(outA + 8, mem.u32(point + 8));
        mem.set32(outB, mem.u32(kFloorUp));
        mem.set32(outB + 4, mem.u32(kFloorUp + 4));
        mem.set32(outB + 8, mem.u32(kFloorUp + 8));
        return 0xFFFFFFFFu;
    }
}

// 0x461D0 ground_height(point): the first floor quad's hit height under the point (as floor_probe), else floor_y.
// The PC's ground_height 0x48A254 is only `fld [floor_y]; ret` (no floor quads): its 18 callers (the Hangedman's fall,
// the hostages, the Molb, the tumble, the bats, kind 70, the knives / projectiles' shadows) all get floor_y, and so
// does PC Mode here (pccap, 2026-10-27: PC captures, docs/pcmode.md "Ground"). The one arcade caller the PC does not
// call it from, hangedman_hit, has its own PC rule there (Chapter 4: -180 == floor_y)
template <class P> float Game<P>::ground_height(uint32_t point)
{
    if (pc_path()) {
        (void)point;
        return mem.f32(G::floor_y);
    }
    if constexpr (!P::pc) {
        const uint32_t scene = mem.u32(G::scene_quads);
        for (uint32_t i = 0; i < mem.at<uint8_t>(scene + 0x61); ++i) {
            uint32_t in[7] = {mem.u32(G::shot_list_b + 4 * i), mem.u32(point), mem.u32(point + 4), mem.u32(point + 8), 0,
                              fbits(-1.0f), 0};
            Tgp::Out hit;
            copro(0x62, in, 7, hit);
            if (ffrom(hit.w[0]) > 0.0f)
                return ffrom(hit.w[2]);
        }
    }
    return mem.f32(G::floor_y);
}

// 0x2F2F0 jump_arc_set(obj, from xyz, to xyz, frames): +0x394 from, +0x3A0 to, +0x2EC step 0, +0x2F0 frames; motion
// flags bit 0 (+0x3C6) and bits 2 / 3 (+0x3C5)
template <class P>
void Game<P>::jump_arc_set(uint32_t obj, uint32_t fx, uint32_t fy, uint32_t fz, uint32_t tx, uint32_t ty, uint32_t tz, int frames)
{
    if constexpr (P::pc) {
        (void)obj, (void)fx, (void)fy, (void)fz, (void)tx, (void)ty, (void)tz, (void)frames;
    } else {
        mem.set32(obj + 0x394, fx);
        mem.set32(obj + 0x398, fy);
        mem.set32(obj + 0x39C, fz);
        mem.set32(obj + 0x3A0, tx);
        mem.set32(obj + 0x3A4, ty);
        mem.set32(obj + 0x2EC, 0);
        mem.set32(obj + 0x3A8, tz);
        mem.set32(obj + 0x2F0, uint32_t(frames));
        mem.at<uint8_t>(obj + 0x3C6) |= 1;
        mem.at<uint8_t>(obj + 0x3C5) |= 0xC;
    }
}

// 0x2F330 jump_arc_step_b: step n + 1 of the arc (+0x394 -> +0x3A0 over +0x2F0 frames, gravity 0.0272 in single
// precision); 1 when the step count has reached the frames, else -1
template <class P> uint32_t Game<P>::jump_arc_step_b(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;
        return 0;
    } else {
        const int32_t n = int32_t(mem.u32(obj + 0x2EC)), f = int32_t(mem.u32(obj + 0x2F0)), n1 = n + 1;
        uint32_t r = 1;
        if (n < f && pc_path()) {
            // PC Mode / Boss Mode: THOTD.EXE 0x46A984 (x87, 24 bits): x / z by (to - from) x (1 / f) x n + from (pmbody)
            const float rc = 1.0f / float(f);
            const float q = ((mem.f32(obj + 0x3A4) - mem.f32(obj + 0x398)) * 2.0f + float(f * f) * ffrom(0x3CDF0123u)) /
                            float(f << 1);
            const float fn = float(n1);
            mem.set32(obj + 0x20, fbits(((mem.f32(obj + 0x3A0) - mem.f32(obj + 0x394)) * rc) * fn + mem.f32(obj + 0x394)));
            mem.set32(obj + 0x28, fbits(fn * (rc * (mem.f32(obj + 0x3A8) - mem.f32(obj + 0x39C))) + mem.f32(obj + 0x39C)));
            mem.set32(obj + 0x24, fbits((q * fn - float(n1 * n1) * ffrom(0x3C5F0123u)) + mem.f32(obj + 0x398)));
            r = 0xFFFFFFFFu;
        } else if (n < f) {
            const float ff = float(f), fn = float(n1);
            const float dy = mem.f32(obj + 0x3A4) - mem.f32(obj + 0x398);
            const float q = (float(f * f) * ffrom(0x3CDF0123u) + (dy + dy)) / float(f << 1);
            const float x = (mem.f32(obj + 0x3A0) - mem.f32(obj + 0x394)) / ff * fn + mem.f32(obj + 0x394);
            const float z = (mem.f32(obj + 0x3A8) - mem.f32(obj + 0x39C)) / ff * fn + mem.f32(obj + 0x39C);
            const float y = (-(float(n1 * n1) * ffrom(0x3C5F0123u)) + q * fn) + mem.f32(obj + 0x398);
            mem.set32(obj + 0x20, fbits(x));
            mem.set32(obj + 0x28, fbits(z));
            mem.set32(obj + 0x24, fbits(y));
            r = 0xFFFFFFFFu;
        }
        mem.set32(obj + 0x2EC, uint32_t(n1));
        return r;
    }
}

// 0x2F400 jump_arc_step: the same with the height in double precision (0.0544.. / 0.0272..)
template <class P> uint32_t Game<P>::jump_arc_step(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;
        return 0;
    } else {
        const int32_t n = int32_t(mem.u32(obj + 0x2EC)), f = int32_t(mem.u32(obj + 0x2F0)), n1 = n + 1;
        uint32_t r = 1;
        if (n < f && pc_path()) {
            // PC Mode / Boss Mode: THOTD.EXE 0x46AA70: as jump_arc_step_b's PC form, the two height constants doubles
            // multiplied at the x87's 24 bits (pmbody)
            const float rc = 1.0f / float(f);
            const float a = float(double(float(f * f)) * ffrom64(0x3FABE02468ACF135ull));
            const float q = ((mem.f32(obj + 0x3A4) - mem.f32(obj + 0x398)) * 2.0f + a) / float(f << 1);
            const float fn = float(n1);
            const float b = float(double(float(n1 * n1)) * ffrom64(0x3F9BE02468ACF135ull));
            mem.set32(obj + 0x20, fbits(((mem.f32(obj + 0x3A0) - mem.f32(obj + 0x394)) * rc) * fn + mem.f32(obj + 0x394)));
            mem.set32(obj + 0x28, fbits(fn * (rc * (mem.f32(obj + 0x3A8) - mem.f32(obj + 0x39C))) + mem.f32(obj + 0x39C)));
            mem.set32(obj + 0x24, fbits((q * fn - b) + mem.f32(obj + 0x398)));
            r = 0xFFFFFFFFu;
        } else if (n < f) {
            const float fy = mem.f32(obj + 0x398);
            const float dy = mem.f32(obj + 0x3A4) - fy;
            const double k1 = ffrom64(0x3FABE02468ACF135ull), k2 = ffrom64(0x3F9BE02468ACF135ull);
            const double q = (double(f * f) * k1 + double(dy + dy)) / double(f << 1);
            const double yd = (-(double(n1 * n1) * k2) + q * double(n1)) + double(fy);
            const float ff = float(f), fn = float(n1);
            const float x = (mem.f32(obj + 0x3A0) - mem.f32(obj + 0x394)) / ff * fn + mem.f32(obj + 0x394);
            const float z = (mem.f32(obj + 0x3A8) - mem.f32(obj + 0x39C)) / ff * fn + mem.f32(obj + 0x39C);
            mem.set32(obj + 0x20, fbits(x));
            mem.set32(obj + 0x28, fbits(z));
            mem.set32(obj + 0x24, fbits(float(yd)));
            r = 0xFFFFFFFFu;
        }
        mem.set32(obj + 0x2EC, uint32_t(n1));
        return r;
    }
}

// 0x2F560 / 0x2F610 jump_arc_velocity(_b): the velocity of the arc's next frame; 1 when over (the first copy also
// zeroes the velocity then)
template <class P> uint32_t Game<P>::jump_arc_velocity(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;
        return 0;
    } else {
        const int32_t n = int32_t(mem.u32(obj + 0x2EC)), f = int32_t(mem.u32(obj + 0x2F0)), n1 = n + 1;
        uint32_t r = 1;
        if (n < f) {
            const float dy = mem.f32(obj + 0x3A4) - mem.f32(obj + 0x398);
            if (pc_path()) {   // THOTD.EXE 0x46AB60 / 0x46AC40: times 1 / f (x87, 24 bits; pmrest2)
                const float rc = 1.0f / float(f);
                mem.set32(obj + 0x78, fbits((mem.f32(obj + 0x3A0) - mem.f32(obj + 0x394)) * rc));
                mem.set32(obj + 0x80, fbits(rc * (mem.f32(obj + 0x3A8) - mem.f32(obj + 0x39C))));
            } else {
                mem.set32(obj + 0x78, fbits((mem.f32(obj + 0x3A0) - mem.f32(obj + 0x394)) / float(f)));
                mem.set32(obj + 0x80, fbits((mem.f32(obj + 0x3A8) - mem.f32(obj + 0x39C)) / float(f)));
            }
            mem.set32(obj + 0x7C, fbits((dy + dy + float(f * f) * ffrom(0x3CDF0123u)) / float(f << 1) +
                                        float(n1) * -ffrom(0x3CDF0123u)));
            r = 0xFFFFFFFFu;
        } else {
            mem.set32(obj + 0x78, 0);
            mem.set32(obj + 0x7C, 0);
            mem.set32(obj + 0x80, 0);
        }
        mem.set32(obj + 0x2EC, uint32_t(n1));
        return r;
    }
}

// 0x2F610 jump_arc_velocity_b: as jump_arc_velocity, the velocity kept when over
template <class P> uint32_t Game<P>::jump_arc_velocity_b(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;
        return 0;
    } else {
        const int32_t n = int32_t(mem.u32(obj + 0x2EC)), f = int32_t(mem.u32(obj + 0x2F0)), n1 = n + 1;
        uint32_t r = 1;
        if (n < f) {
            const float dy = mem.f32(obj + 0x3A4) - mem.f32(obj + 0x398);
            if (pc_path()) {   // THOTD.EXE 0x46AB60 / 0x46AC40: times 1 / f (x87, 24 bits; pmrest2)
                const float rc = 1.0f / float(f);
                mem.set32(obj + 0x78, fbits((mem.f32(obj + 0x3A0) - mem.f32(obj + 0x394)) * rc));
                mem.set32(obj + 0x80, fbits(rc * (mem.f32(obj + 0x3A8) - mem.f32(obj + 0x39C))));
            } else {
                mem.set32(obj + 0x78, fbits((mem.f32(obj + 0x3A0) - mem.f32(obj + 0x394)) / float(f)));
                mem.set32(obj + 0x80, fbits((mem.f32(obj + 0x3A8) - mem.f32(obj + 0x39C)) / float(f)));
            }
            mem.set32(obj + 0x7C, fbits((dy + dy + float(f * f) * ffrom(0x3CDF0123u)) / float(f << 1) +
                                        float(n1) * -ffrom(0x3CDF0123u)));
            r = 0xFFFFFFFFu;
        }
        mem.set32(obj + 0x2EC, uint32_t(n1));
        return r;
    }
}

// 0x6A8A0 obj_move_heading: velocity = (0, 0, speed +0x74) turned by +0xC0 (y) and +0xBC (x); position += it
//
template <class P> void Game<P>::obj_move_heading(uint32_t obj)
{
    if (pc_path()) {
        // PC Mode / Boss Mode: THOTD.EXE 0x4C589C on its helpers block 0x4C1B60 (identity, turned y then x, (0, 0,
        // speed) through it; pmbody)
        pm::Matrix m;
        m.rotate_y(mem.u32(obj + 0xC0));
        m.rotate_x(mem.u32(obj + 0xBC));
        float v[3];
        m.transform(0.0f, 0.0f, mem.f32(obj + 0x74), v[0], v[1], v[2]);
        for (uint32_t k = 0; k < 3; ++k) {
            mem.set32(obj + 0x78 + 4 * k, fbits(v[k]));
            mem.set32(obj + 0x20 + 4 * k, fbits(mem.f32(obj + 0x20 + 4 * k) + v[k]));
        }
        return;
    }
    Tgp::Out none, o;
    copro(0x21, nullptr, 0, none);
    uint32_t ay[1] = {mem.u32(obj + 0xC0)};
    copro(0x15, ay, 1, none);
    uint32_t ax[1] = {mem.u32(obj + 0xBC)};
    copro(0x14, ax, 1, none);
    uint32_t v[3] = {0, 0, mem.u32(obj + 0x74)};
    copro(0x1A, v, 3, o);
    mem.set32(obj + 0x78, o.w[0]);
    mem.set32(obj + 0x7C, o.w[1]);
    mem.set32(obj + 0x80, o.w[2]);
    const float x = mem.f32(obj + 0x20) + ffrom(o.w[0]), y = mem.f32(obj + 0x24) + ffrom(o.w[1]);
    const float z = mem.f32(obj + 0x28) + ffrom(o.w[2]);
    copro(0x06, nullptr, 0, none);
    mem.set32(obj + 0x20, fbits(x));
    mem.set32(obj + 0x24, fbits(y));
    mem.set32(obj + 0x28, fbits(z));
}

} // namespace hotd::port
