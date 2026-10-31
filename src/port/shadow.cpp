// The shadow light and projection (first written in the runtime, src/game/chapter.cpp, from the disassembly), moved
// here for their capture proofs (proofs-4). Arcade forms only (the PC forms are replayed). Declarations:
// decl/core_bounds.h / decl/event.h.
#include "port/game.h"

namespace hotd::port {

namespace shadow_detail {
// the light direction (0, 0, 1) turned by yaw b then pitch a (0x21 push + identity, 0x15, 0x14, 0x1A), popped
template <class GameT> void light_dir(GameT& g, uint32_t a, uint32_t b, float v[3])
{
    Tgp::Out none, o;
    g.copro(0x21, nullptr, 0, none);
    uint32_t yaw[1] = {b}, pitch[1] = {a};
    g.copro(0x15, yaw, 1, none);
    g.copro(0x14, pitch, 1, none);
    uint32_t z[3] = {0, 0, fbits(1.0f)};
    g.copro(0x1A, z, 3, o);
    for (int k = 0; k < 3; ++k)
        v[k] = ffrom(o.w[size_t(k)]);
    g.copro(0x06, nullptr, 0, none);
}

// the shadow projection m (12 words, coprocessor layout) times stored matrix 1 into stored matrix 3 (0x09, 0x76 1,
// 0x75 3 inside a push / pop): the matrix the shadow models (_DD) are drawn with
template <class GameT> void store(GameT& g, const float m[12])
{
    Tgp::Out none;
    g.copro(0x05, nullptr, 0, none);
    uint32_t w[12];
    for (int k = 0; k < 12; ++k)
        w[k] = fbits(m[k]);
    g.copro(0x09, w, 12, none);
    uint32_t one[1] = {1}, three[1] = {3};
    g.copro(0x76, one, 1, none);
    g.copro(0x75, three, 1, none);
    g.copro(0x06, nullptr, 0, none);
}

template <class M> void set4(M& m, uint32_t a, float x, float y, float z, float w)
{
    m.set32(a, fbits(x));
    m.set32(a + 4, fbits(y));
    m.set32(a + 8, fbits(z));
    m.set32(a + 12, fbits(w));
}
} // namespace shadow_detail

// 0x6C2A0 shadow_light_reset(pitch, yaw) (SHDWLT, SCNINIT): the shadow light's angles (0x520A80 = a, b, a, b), the
// direction it gives (x, y, z), and a shadow projection onto y = 0 along it (x, z sheared by -x / y, -z / y) into
// stored matrix 3 and its copies 0x51FDF0 / 0x51FE00 / 0x51FE10; 0x520A90 = (0, 1, 0, 0) (the floor plane)
template <class P> void Game<P>::shadow_light_reset(uint32_t a, uint32_t b)
{
    if constexpr (P::pc) {
        (void)a, (void)b;   // the PC form: not ported (vtable only; ProofPc replays it)
    } else {
        using namespace shadow_detail;
        mem.set32(0x520A80, a);
        mem.set32(0x520A84, b);
        mem.set32(0x520A88, a);
        mem.set32(0x520A8C, b);
        float v[3];
        light_dir(*this, mem.u32(0x520A80), mem.u32(0x520A84), v);
        const float g5 = -1.0f / v[1];
        const float g4 = g5 * v[1] + 1.0f;
        const float m[12] = {1.0f, 0.0f, 0.0f, v[0] * g5, g4, v[2] * g5, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
        store(*this, m);
        set4(mem, 0x520A90, 0.0f, 1.0f, 0.0f, 0.0f);
        set4(mem, 0x51FE10, 1.0f, 0.0f, 0.0f, 0.0f);
        set4(mem, 0x51FDF0, 1.0f, 0.0f, 0.0f, v[0] * g5);
        set4(mem, 0x51FE00, g4, v[2] * g5, 0.0f, 0.0f);
    }
}

// 0x6C760 shadow_setup(floor y): the shadow projection onto the plane y = floor along the shadow light (0x520A80),
// into stored matrix 3 (as shadow_light_reset, with the plane's offset); skipped when the light and the plane
// (0x520A88.., 0x520A90 = (0, 1, 0, -y)) did not change
template <class P> void Game<P>::shadow_setup(float y)
{
    if constexpr (P::pc) {
        (void)y;   // the PC form: not ported (vtable only; ProofPc replays it)
    } else {
        using namespace shadow_detail;
        const float ny = -y;
        if (mem.u32(0x520A80) == mem.u32(0x520A88) && mem.u32(0x520A84) == mem.u32(0x520A8C) &&
            mem.f32(0x520A90) == 0.0f && mem.f32(0x520A94) == 1.0f && mem.f32(0x520A98) == 0.0f &&
            ny == mem.f32(0x520A9C))
            return;
        const uint32_t a = mem.u32(0x520A80), b = mem.u32(0x520A84);
        float v[3];
        light_dir(*this, a, b, v);
        const float g5 = -1.0f / v[1];
        const float m3 = v[0] * g5, m5 = v[2] * g5;
        const float m9 = (ny * v[0]) * g5, m11 = (ny * v[2]) * g5;
        const float m[12] = {1.0f, 0.0f, 0.0f, m3, 0.0f, m5, 0.0f, 0.0f, 1.0f, m9, y, m11};
        store(*this, m);
        mem.set32(0x520A88, a);
        mem.set32(0x520A8C, b);
        set4(mem, 0x520A90, 0.0f, 1.0f, 0.0f, ny);
        set4(mem, 0x51FDF0, 1.0f, 0.0f, 0.0f, m3);
        set4(mem, 0x51FE00, 0.0f, m5, 0.0f, 0.0f);
        set4(mem, 0x51FE10, 1.0f, m9, y, m11);
    }
}

} // namespace hotd::port
