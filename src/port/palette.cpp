// The palette animation (proofs-4): palette_animation 0x25310, first written in the runtime (src/game/palette_anim.cpp,
// palanim, from the typed decompilation; formats.md §24), moved here for its capture proof. Arcade form only (the PC
// has its own palette code). Declaration: decl/support_b.h.
#include "port/game.h"

namespace hotd::port {

namespace palette_detail {
// 0x24F80 rgb555_pack: components other than 31 lose their lowest bit
inline uint16_t pack(int r, int g, int b)
{
    auto q = [](int v) { return v == 31 ? 31 : v & 30; };
    return uint16_t(0x8000 | q(b) << 10 | q(g) << 5 | q(r));
}
// rgb555_unpack (0x24F40), every component x (k mod 32) / 32, rgb555_pack
inline uint16_t scale(uint16_t c, int k)
{
    k %= 32;
    return pack(int((c & 31) * k) / 32, int((c >> 5 & 31) * k) / 32, int((c >> 10 & 31) * k) / 32);
}
inline int triangle(int t, int period, int half)
{
    const int r = t % period;
    return r > half ? period - 1 - r : r;
}
} // namespace palette_detail

// 0x25310 palette_animation (geo_frame_begin, every frame): the pulsing polygon colours in work RAM 0x52066C..0x5206E0
// from the frame counter, the chapter and the event flags 0x51E982 / 0x51E9A1 / 0x51E9B2 / 0x51E9B3 (copied into the
// palette at the vblank by 0x24E90); in chapter 5 / chapter 2 with 0x51E982 set only the title screen's step (game mode
// 9, scene 3: palette_animation_step 0x24020 on palette 0xD)
template <class P> void Game<P>::palette_animation()
{
    using namespace palette_detail;
    const int frame = int32_t(mem.u32(G::frame_counter));
    const uint8_t chapter = mem.template at<uint8_t>(G::chapter);
    if (chapter == 5 || (chapter == 2 && mem.template at<uint8_t>(0x51E982) != 0)) {
        if (mem.template at<int16_t>(G::game_mode) == 9 && mem.template at<uint8_t>(G::scene_number) == 3 &&
            mem.template at<int16_t>(0x51F702) == 0 && mem.template at<int16_t>(0x51F704) == 0) {
            int v = (frame % 0x40) * 4;
            if (0x80 < v)
                v = (frame % 0x40) * -4 + 0x100;
            palette_animation_step(0xD, v << 1);
        }
        return;
    }
    auto w = [&](uint32_t a, uint16_t v) { mem.template at<uint16_t>(a) = v; };
    auto rom = [&](uint32_t a) { return mem.template at<uint16_t>(a); };
    // 9 entries cycling through the 90-colour table 0xAB120
    for (int i = 0; i < 9; ++i)
        w(0x52066C + 2 * uint32_t(i), rom(0xAB120 + 2 * uint32_t(int(float(frame + 3 * i) / 1.5f) % 90)));
    // 96-frame pulse of the record's base colours 0xA35E8
    int k = int(float(triangle(frame, 96, 47)) / 1.5f);
    for (uint32_t i = 0; i < 3; ++i)
        w(0x52067E + 2 * i, scale(rom(0xA35E8 + 2 * i), k));
    w(0x520684, chapter == 6 ? 0 : scale(0xD6B5, k));
    w(0x52068C, scale(0xD6B5, k));
    // 14 light strips (0xA35FA), 100 frames, 12/32 .. 31/32, phases 14 frames apart (0x520698 only cleared in chapter 6)
    for (int i = 0; i < 14; ++i) {
        const uint32_t a = 0x520690 + 2 * uint32_t(i);
        if (a == 0x520690 && chapter == 3 && mem.template at<uint8_t>(0x51E9B3) != 0) {
            w(a, 0);
            continue;
        }
        if (a == 0x520698) {
            if (chapter == 6)
                w(a, 0);
            continue;
        }
        const int v = int(float(triangle(frame + 0x150 + 14 * i, 100, 49)) / 2.5f + 12.0f);
        if (a == 0x520690 && chapter == 3 && mem.template at<uint8_t>(0x51E9B2) != 0 && v == 12) {
            mem.template at<uint8_t>(0x51E9B3) = 1;
            continue;
        }
        w(a, scale(rom(0xA35FA + 2 * uint32_t(i)), v));
    }
    // 64-frame pulse
    k = triangle(frame, 64, 31);
    w(0x52068E, scale(0xC0DF, k));
    // 0x801F blended toward 0x800C by k / 32 (the blood pulse)
    w(0x5206AE, pack(31 + int((k % 32) * uint32_t(12 - 31)) / 32, 0, 0));
    w(0x5206E0, scale(chapter == 3 && mem.template at<uint8_t>(0x51E9A1) != 0 ? 0x8012 : 0x800E, k));
}

// 0x1A400 palette_grey_build (kind 172's chapter opening, kind 182): the sepia look. The three colour curve tables
// 0x512CA0 / 0x513CA0 / 0x514CA0 (0x800 s16 each) averaged and looked up in the ROM's sepia ramp 0xAA050 (64 x {r, g, b}
// s16) into the palette queue 0x517CB0 / 0x518CB0 / 0x519CB0; palette bank 2 (0x1802000, the 2D layer's) greyed into
// 0x51D4B0 (saved in 0x51CCB0), uploaded at the vblank by 0x1A300; 0x51E8C4 the upload request
template <class P> void Game<P>::palette_grey_build()
{
    if constexpr (P::pc) {
        // the PC form: not ported (vtable only; ProofPc replays it)
    } else {
        mem.template at<uint8_t>(0x51E8C4) = 0;
        for (uint32_t i = 0; i < 0x800; ++i) {
            const int32_t sum = int32_t(mem.template at<int16_t>(0x512CA0 + i * 2)) +
                                mem.template at<int16_t>(0x513CA0 + i * 2) + mem.template at<int16_t>(0x514CA0 + i * 2);
            const int32_t idx = int32_t(int16_t(int16_t(sum / 3) / 4)) * 6;
            for (uint32_t c = 0; c < 3; ++c)
                mem.template at<uint16_t>(0x517CB0 + c * 0x1000 + i * 2) =
                    mem.template at<uint16_t>(0xAA050 + uint32_t(idx) + c * 2);
        }
        // the grey of an RGB555 word: (r + g + b) / 3, even, in all three channels, the top bit kept (the runtime maps
        // palette bank 2 only when something does)
        if (mem.mapped(0x1802000)) {
            auto grey = [&](uint32_t n, uint32_t off) {
                for (uint32_t i = 0; i < n; ++i) {
                    const uint16_t c = mem.template at<uint16_t>(0x1802000 + off + i * 2);
                    const uint32_t s = ((c >> 5) & 0x1F) + (c & 0x1F) + ((c >> 10) & 0x1F);
                    mem.template at<uint16_t>(0x51D4B0 + off + i * 2) = uint16_t((c & 0x8000) | ((s / 3) & 0x1E) * 0x421);
                    mem.template at<uint16_t>(0x51CCB0 + off + i * 2) = c;
                }
            };
            grey(8, 0);
            grey(0x3E8, 0x30);
        }
        vblank_callback_add(F::palette_bank2_upload);
        mem.template at<uint16_t>(0x51F704) = 1;
        mem.template at<uint16_t>(G::palette_grey_on) = 1;
        mem.template at<uint8_t>(0x51E8C4) = 1;
    }
}

// 0x2780 vblank_callback_add: the routine into the first free slot of the vblank's once-list 0x511FE0 (0x20 slots),
// the count 0x512064
template <class P> void Game<P>::vblank_callback_add(uint32_t routine)
{
    if constexpr (P::pc) {
        (void)routine;   // the PC form: not ported (vtable only; ProofPc replays it)
    } else {
        uint32_t i = 0;
        while (i < 0x20 && mem.u32(0x511FE0 + i * 4) != 0)
            ++i;
        if (i >= 0x20)
            return;
        mem.set32(0x511FE0 + i * 4, routine);
        mem.set32(0x512064, i + 1);
    }
}

// palette bank 2 (0x1802000: entries 0-7 and 0x18..0x3FF) from a copy in work RAM; not mapped = no palette RAM (the
// runtime's engine, which has none)
template <class P> void Game<P>::palette_bank2_from(uint32_t src)
{
    if (!mem.mapped(0x1802000))
        return;
    for (uint32_t i = 0; i < 8; ++i)
        mem.template at<uint16_t>(0x1802000 + i * 2) = mem.template at<uint16_t>(src + i * 2);
    for (uint32_t i = 0; i < 0x3E8; ++i)
        mem.template at<uint16_t>(0x1802030 + i * 2) = mem.template at<uint16_t>(src + 0x30 + i * 2);
}

// 0x1A300 palette_bank2_upload (the vblank's once-list, from palette_grey_build): the grey shadow 0x51D4B0 into
// palette bank 2
template <class P> void Game<P>::palette_bank2_upload()
{
    if constexpr (!P::pc)
        palette_bank2_from(0x51D4B0);
}

// 0x1A710 palette_restore_vblank: palette bank 2 from the saved copy 0x51CCB0, the grey look off (0x51F704,
// palette_grey_on), the palette queue upload requested (0x51E8C4)
template <class P> void Game<P>::palette_restore_vblank()
{
    if constexpr (!P::pc) {
        palette_bank2_from(0x51CCB0);
        mem.template at<uint16_t>(0x51F704) = 0;
        mem.template at<uint16_t>(G::palette_grey_on) = 0;
        mem.template at<uint8_t>(0x51E8C4) = 1;
    }
}

// 0x24E90 palette_anim_copy (the vblank): palette_animation's colours (work RAM 0x520664 + 4n / + 4n + 2) into the
// polygon palette entries 1021 - 2n / 1020 - 2n (0x18027FA down), n = 0..18, but for six of them; entry 689 = 0x9854
// in texture area 9, entry 211 = 0xEBDF, entry 959 = the colour at 0x5206E0. Writes palette RAM only (the runtime's renderer reads
// the work RAM colours: src/assets/arcade_palette_fx.cpp)
template <class P> void Game<P>::palette_anim_copy()
{
    if constexpr (!P::pc) {
        if (!mem.mapped(0x1802000))
            return;
        uint32_t dst = 0x18027FA, src = 0x520664;
        for (int i = 4; i < 0x4F; i += 4) {
            if (i != 8 && i != 0x28 && i != 0x2A)
                mem.template at<uint16_t>(dst) = mem.template at<uint16_t>(src);
            if (i != 6 && i != 0x26 && i != 0x28)
                mem.template at<uint16_t>(dst - 2) = mem.template at<uint16_t>(src + 2);
            dst -= 4;
            src += 4;
        }
        if (mem.u32(G::texture_area) == 9)
            mem.template at<uint16_t>(0x1802562) = 0x9854;
        mem.template at<uint16_t>(0x18021A6) = 0xEBDF;
        mem.template at<uint16_t>(0x180277E) = mem.template at<uint16_t>(0x5206E0);
    }
}

// 0x23CC0 colour_blend32 (a, b, t): each component of a moved towards b's by (t % 32) / 32, packed again (the item
// sparkle / life item glow, palette entry 985). Arcade form only (the PC draws the glow its own way)
template <class P> uint32_t Game<P>::colour_blend32(uint32_t a, uint32_t b, uint32_t t)
{
    using namespace palette_detail;
    if constexpr (P::pc) {
        (void)a, (void)b, (void)t;
        return 0;
    } else {
        const int k = int32_t(t) % 32;
        auto mix = [k](uint32_t x, uint32_t y, int sh) {
            const int u = int(x >> sh & 31), v = int(y >> sh & 31);
            return int16_t((k * (v - u)) / 32) + u;
        };
        return pack(mix(a, b, 0), mix(a, b, 5), mix(a, b, 10));
    }
}

} // namespace hotd::port
