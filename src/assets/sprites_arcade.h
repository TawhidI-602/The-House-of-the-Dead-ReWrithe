// The arcade's 2D layer (docs/formats.md §31): Model 2 carries the System 24 tile chip. The game writes 16-bit cells
// into four 64 x 64 tilemaps (tile RAM 0x1000000: layer 0 at +0, 1 at +0x2000, 2 at +0x4000, 3 at +0x6000), the
// 8 x 8 4-bit tiles into character RAM (0x1080000, 32 bytes a tile, 0x4000 tiles in 128 slots of 128) and the 16-colour
// palettes into palette RAM (the game keeps a shadow at work RAM 0x515CA0, one palette a slot). A cell: bits 0-13 the
// tile, bits 7-14 the palette (so a slot's tiles use the slot's palette; bit 14 picks the upper 128 palettes), bit 15
// the priority (1 = over the 3D picture). Layers 1 and 3 are the "window" layers of 0 and 2: a mask (tile RAM
// +0xC000 for 0 / 1, +0xD000 for 2 / 3; 8 bytes = 64 cell bits a screen line) shows the window layer where its bit is
// set, the main layer where it is clear. Scroll registers at +0xA000 (x, 4 words) and +0xA008 (y; bit 15 = layer off).
// The screen is 496 x 384 (cells 0..61 x 0..47). MAME (model2_v.cpp) draws the layers 3, 2, 1, 0 (0 on top) into an
// RGB bitmap and copies it with black as transparent: a pixel whose colour is black is see-through even over a lower
// layer (the full-screen black sprite 0xA1 is invisible until a fade changes its palette).
#pragma once

#include <cstdint>
#include <vector>

namespace hotd::assets {

struct Arcade2D {
    static constexpr int kWidth = 496, kHeight = 384;
    // tileRam: 0x10000 bytes (0x1000000), charRam: 0x80000 bytes (0x1080000), palette: 0x2000 bytes (256 palettes of 16
    // RGB555 words). Composes one pass of the cells into rgba (kWidth x kHeight x 4): high = the priority cells (over
    // the 3D), !high = the low-priority ones (under the 3D: MAME model2 screen_update draws them, then the polygons,
    // then the priority cells); returns false when nothing was drawn.
    static bool compose(const uint8_t* tileRam, const uint8_t* charRam, const uint8_t* palette, bool high,
                        std::vector<uint8_t>& rgba, const uint8_t* side = nullptr, int want = 0);
    // perf: compose with side for want 0, 1 and 2 at once (rgba[want], any[want]: something drawn there)
    static void composeSides(const uint8_t* tileRam, const uint8_t* charRam, const uint8_t* palette, bool high,
                             const uint8_t* side, std::vector<uint8_t> (&rgba)[3], bool (&any)[3]);
    // side / want (display2): with side (4 layers x 64 x 64 bytes, Runtime::cellSide) only the cells whose side is want
};

} // namespace hotd::assets
