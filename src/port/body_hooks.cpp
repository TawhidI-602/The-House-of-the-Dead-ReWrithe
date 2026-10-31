// The heads that look at something (the eye / target beams, body hooks of object_draw's part class 1), the player's
// hit notice and the hit screen shake. First written as runtime overrides (T1, src/game/t1_bounds.cpp), read from the
// disassembly. Arcade forms (decl/support_b.h).
#include "port/game.h"

#include <cstdlib>

namespace hotd::port {

// body_hook_eye_beam / _target_beam's common body (inline in both): the point into the head's frame (push, y -0x4000,
// stored matrix 1 (0x77), inverse (0x7A), 0x1A, angles 0x0F folded, pop), each angle clamped to +-0x2000; the look
// angles +0x144 (yaw) / +0x148 (pitch) turn toward them by 0xC0 a frame (as angle_turn_toward, kept to 16 bits), then
// the head turns by them (y +0x144, z -0x148)
template <class P> void Game<P>::body_look_at(uint32_t body, float x, float y, float z)
{
    Tgp::Out none, p, a;
    copro(0x05, nullptr, 0, none);
    uint32_t q[1] = {0xFFFFC000u};
    copro(0x15, q, 1, none);
    uint32_t one[1] = {1};
    copro(0x77, one, 1, none);
    copro(0x7A, nullptr, 0, none);
    uint32_t in[3] = {fbits(x), fbits(y), fbits(z)};
    copro(0x1A, in, 3, p);
    uint32_t v[3] = {p.w[0], p.w[1], p.w[2]};
    copro(0x0F, v, 3, a);
    uint32_t yaw = a.w[0], pitch = a.w[1];
    if (((((pitch & 0xFFFF) << 1) ^ (pitch & 0xFFFF)) & 0x8000) != 0) {
        pitch = 0x8000 - pitch;
        yaw = yaw + 0x8000;
    }
    copro(0x06, nullptr, 0, none);
    if (pm_body()) {
        // PC Mode / Boss Mode: THOTD.EXE's eye / target beam 0x51E258 / 0x51E4C8 on PM's part matrix (the hook file's
        // helpers; pm::look_turn, proven in hotd_proof pmmaths), the coprocessor turned by the same angles (pmrest2)
        uint32_t cy = mem.u32(body + 0x144), cp = mem.u32(body + 0x148);
        pm::look_turn(pm_cur(), pm_camera, x, y, z, cy, cp);
        mem.set32(body + 0x148, cp);
        mem.set32(body + 0x144, cy);
        uint32_t ry[1] = {cy};
        copro(0x15, ry, 1, none);
        uint32_t rz[1] = {0u - cp};
        copro(0x16, rz, 1, none);
        return;
    }
    auto clamp = [](uint32_t w) {
        w &= 0xFFFF;
        if (w <= 0x7FFF)
            return w > 0x2000 ? 0x2000u : w;
        return w <= 0xDFFF ? 0xE000u : w;
    };
    auto ease = [](int32_t cur, int32_t tgt) {
        const int32_t r = 0xC0;
        const int32_t d = std::abs(tgt - cur), dUp = std::abs(tgt - (cur - 0x10000)), dDown = std::abs(cur - (tgt - 0x10000));
        if (d <= r || dUp <= r || dDown <= r)
            return tgt;
        if (cur < tgt)
            return d > dDown ? cur - r : cur + r;
        return d > dUp ? cur + r : cur - r;
    };
    const uint32_t ty = clamp(yaw), tp = clamp(pitch);
    const uint32_t ny = uint32_t(ease(int32_t(mem.u32(body + 0x144)), int32_t(ty))) & 0xFFFF;
    const uint32_t np = uint32_t(ease(int32_t(mem.u32(body + 0x148)), int32_t(tp))) & 0xFFFF;
    mem.set32(body + 0x148, np);
    mem.set32(body + 0x144, ny);
    uint32_t ry[1] = {ny};
    copro(0x15, ry, 1, none);
    uint32_t rz[1] = {0u - np};
    copro(0x16, rz, 1, none);
}

// 0x9E820 body_hook_eye_beam(body): the head looks at the camera (15 up)
template <class P> void Game<P>::body_hook_eye_beam(uint32_t body)
{
    body_look_at(body, mem.f32(G::camera_pos), mem.f32(G::camera_pos + 4) + 15.0f, mem.f32(G::camera_pos + 8));
}

// 0x9EB30 body_hook_target_beam(body): the head looks at the object +0x390 (15 up)
template <class P> void Game<P>::body_hook_target_beam(uint32_t body)
{
    const uint32_t t = mem.u32(body + 0x390);
    if (t == 0 && pm_body())   // THOTD.EXE 0x51E4C8: no target, no look and no turn
        return;
    body_look_at(body, mem.f32(t + 0x20), mem.f32(t + 0x24) + 15.0f, mem.f32(t + 0x28));
}

// 0x6290 player_hit_notice (player task, its camera-drive routine): hit this frame (+0xB4) ->
// hud_player_notice_show(task, the hit's side +0xB8)
template <class P> void Game<P>::player_hit_notice(uint32_t task)
{
    const uint32_t pl = G::players + uint32_t(int32_t(mem.at<int8_t>(task + 0x20))) * 0xCC;
    if (mem.u32(pl + 0xB4) != 0)
        hud_player_notice_show(task, int32_t(mem.u32(pl + 0xB8)));
}

// 0x6350 hit_screen_shake (shake_task 0x5DC0's second call): a player hit (+0xB4 = 1, cleared) starts a 48-frame
// shake (shake_timer counts down); the view offset 0x51F614 = (int) coprocessor 0x1E (timer x 0x1800, timer x 48 /
// 48), 0x51F610 = 0; both 0 when it ends
template <class P> void Game<P>::hit_screen_shake()
{
    int32_t t = int32_t(mem.u32(G::shake_timer));
    for (int p = 0; p < seat_count(); ++p) {   // 4P: seats 2 / 3's hits shake it too
        if (mem.u32(player_rec(p) + 0xB4) == 1) {
            t = 0x30;
            mem.set32(player_rec(p) + 0xB4, 0);
        }
    }
    t -= 1;
    if (t < 1)
        t = 0;
    mem.set32(G::shake_timer, uint32_t(t));
    if (t == 0) {
        mem.set32(0x51F610, 0);
        mem.set32(0x51F614, 0);
        return;
    }
    uint32_t in[2] = {uint32_t(t * 0x1800), fbits(float(t) * 48.0f * ffrom(0x3CAAAAABu))};
    Tgp::Out o;
    copro(0x1E, in, 2, o);
    mem.set32(0x51F610, 0);
    mem.set32(0x51F614, uint32_t(cvtzril(ffrom(o.w[0]))));
}

} // namespace hotd::port
