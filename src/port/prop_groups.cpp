// Kind 138 (a group of default props from a table) and prop 068 (the old key that wobbles and breaks). First written
// as runtime overrides (T1, src/game/t1_bounds.cpp), read from the disassembly. Arcade forms (decl/support_b.h).
#include "port/game.h"

namespace hotd::port {

// 0x3A780 kind138_prop_group: default props (prop_default 0x3B160) from table 0xAA388 + type x 0x16C (0x1C each:
// sub-type, position, angles; -1 ends): prop_init, the group's +0x70 / +0x64, raised by the sub-type's height (0xAC30C
// + 0x4C x sub-type), flags: 0x1000 off the 30 line, 0x4000000 by 0xAC328 bit 0, sub-type 0x2B at x 82.9 / 97.3 ->
// event flags 0x23 / 0x24, others but 2 / 3 / 0x22 / 0x23 bit 30, sub-type 2 bit 6; then gone
template <class P> void Game<P>::kind138_prop_group(uint32_t obj)
{
    for (uint32_t i = 0;; ++i) {
        const uint32_t table = 0xAA388 + uint32_t(int32_t(mem.at<int16_t>(obj + 0xCC))) * 0x16C;
        const uint32_t e = table + (i & 0xFF) * 0x1C;
        const uint32_t p = task_create(F::prop_default, 0x3C8);
        task_clear_work(p);
        mem.at<int16_t>(p + 0xCC) = mem.at<int16_t>(e);
        prop_init(p);
        mem.at<int16_t>(p + 0xCE) = 0;
        mem.at<int16_t>(p + 0x70) = mem.at<int16_t>(obj + 0x70);
        mem.at<int16_t>(p + 0x64) = mem.at<int16_t>(obj + 0x64);
        for (uint32_t k = 0; k < 3; ++k) {
            mem.set32(p + 0x20 + 4 * k, mem.u32(e + 4 + 4 * k));
            mem.set32(p + 0x2C + 4 * k, mem.u32(e + 0x10 + 4 * k));
        }
        if (mem.f32(p + 0x24) != 30.0f)
            mem.set32(p + 0x58, mem.u32(p + 0x58) | 0x1000);
        const uint32_t t = uint32_t(int32_t(mem.at<int16_t>(p + 0xCC))) * 0x4C;
        mem.set32(p + 0x24, fbits(mem.f32(p + 0x24) + mem.f32(0xAC30C + t)));
        if (mem.at<uint8_t>(0xAC328 + t) & 1)
            mem.set32(p + 0x58, mem.u32(p + 0x58) | 0x4000000);
        const uint16_t st = mem.at<uint16_t>(p + 0xCC);
        if (st == 0x2B) {
            if (mem.f32(p + 0x20) == ffrom(0x42A5CCCDu))        // 82.9
                mem.at<int16_t>(p + 0xCE) = 0x23;
            else if (mem.f32(p + 0x20) == ffrom(0x42C2999Au))   // 97.3
                mem.at<int16_t>(p + 0xCE) = 0x24;
        } else if (st != 2 && st != 0x22 && st != 0x23 && st != 3) {
            mem.set32(p + 0x58, mem.u32(p + 0x58) | 0x40000000);
        }
        if (mem.at<uint16_t>(p + 0xCC) == 2)
            mem.set32(p + 0x58, mem.u32(p + 0x58) | 0x40);
        const uint32_t next = 0xAA388 + uint32_t(int32_t(mem.at<int16_t>(obj + 0xCC))) * 0x16C + ((i + 1) & 0xFF) * 0x1C;
        if (int32_t(mem.u32(next)) == -1)
            break;
    }
    task_exit();
}

// 0x3E630 prop068_furukagi (the old key): past camera frame 521, after 226 frames it breaks (pieces, sound 0xA01300)
// and ends; in camera drive mode 1 it starts after 31 frames (or when shot): sound 0xA0120C, a bullet mark, then a
// wobble on z (amplitude +0x31C from 0x1200, -0x100 a swing, phase +0x320 += 0x1000)
template <class P> void Game<P>::prop068_furukagi(uint32_t obj)
{
    if (!(mem.f32(0x51EB9C) <= 521.0f)) {
        const int32_t t = int32_t(mem.u32(obj + 0x50)) + 1;
        mem.set32(obj + 0x50, uint32_t(t));
        if (t > 0xE2) {
            prop_break_pieces(obj);
            sound_cue(0xA01300);
            task_exit();
        }
    }
    if (mem.u32(G::camera_drive_mode) == 1 && mem.u32(obj + 0x324) == 0) {
        mem.set32(obj + 0x2EC, uint32_t(-0xE));
        mem.set32(obj + 0x324, 1);
    }
    bool start = false;
    if (mem.u32(G::camera_drive_mode) == 1) {
        const int32_t w = int32_t(mem.u32(obj + 0x2EC));
        mem.set32(obj + 0x2EC, uint32_t(w + 1));
        start = 30 < w;
    }
    if (!start && (mem.at<uint8_t>(obj + 0x58) & 6) != 0 && mem.u32(obj + 0x324) == 0)
        start = true;
    if (start) {
        if (mem.u32(obj + 0x324) == 0) {
            sound_cue(0xA0120C);
            prop_bullet_impact(obj);
        }
        mem.set32(obj + 0x2EC, 0);
        mem.set32(obj + 0x320, 0);
        mem.set32(obj + 0x34, 0);
        mem.set32(obj + 0x31C, 0x1200);
        mem.set32(obj + 0x58, mem.u32(obj + 0x58) | 0x10000000);
    }
    if ((mem.u32(obj + 0x58) >> 28) & 1) {
        uint32_t a[1] = {mem.u32(obj + 0x320)};
        Tgp::Out s;
        copro(0x1B, a, 1, s);
        const int32_t v = int32_t(float(int32_t(mem.u32(obj + 0x31C))) * ffrom(s.w[0]));
        const int32_t ph = int32_t(mem.u32(obj + 0x320)) + 0x1000;
        mem.set32(obj + 0x320, uint32_t(ph));
        mem.set32(obj + 0x34, uint32_t(v));
        if (ph > 0xFFFE) {
            const int32_t na = int32_t(mem.u32(obj + 0x31C)) - 0x100;
            mem.set32(obj + 0x31C, uint32_t(na));
            if (na < 1)
                mem.set32(obj + 0x58, mem.u32(obj + 0x58) & 0xEFFFFFFFu);
        }
    }
    prop_draw(obj);
    prop_shootable(obj);
}

} // namespace hotd::port
