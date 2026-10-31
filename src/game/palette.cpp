// fix-logic (overnight 2026-10-16): the grey / sepia palette look and the vblank callback list. Read from the typed
// decompilation (0x1A300 .. 0x1A710, 0x2780, 0x9B600 .. 0x9B660); 0x1A400 / 0x2780 moved to the port (proofs-4), 0x1A300 / 0x1A710 (proofs-5), the rest UNVERIFIED.
//
// palette_grey_build (0x1A400) is what gives Chapter 1's opening (kind 172, the 60 frames after the chapter card) and
// kind 182's scenes their sepia look: the three colour curve tables (0x512CA0 / 0x513CA0 / 0x514CA0, 0x800 entries
// each, the hardware's per-channel gamma) are averaged and looked up in the ROM's sepia ramp 0xAA050 (64 x {r, g, b}
// s16) into the palette queue 0x517CB0 / 0x518CB0 / 0x519CB0, which the vblank uploads; palette bank 2
// (0x1802000, the polygon palette: the textures' 1024 colours) is greyed through the shadow 0x51D4B0 (saved in
// 0x51CCB0 for palette_restore_vblank). The runtime keeps
// the game state (the queue in work RAM, palette_grey_on) and exposes the look as Runtime::greyPalette for the
// renderer (the render worker's part): the engine has no gamma RAM.
#include "game/runtime.h"

#include <cstring>

namespace hotd::game {

using G = A::G;
using F = A::F;

namespace {
constexpr uint32_t kSepia = 0xAA050;        // ROM: 64 x {r, g, b} s16
constexpr uint32_t kVblankList = 0x511FE0;
constexpr uint32_t kPalBank2 = 0x1802000;   // palette RAM bank 2: the polygon palette
} // namespace

// 0x1A400 palette_grey_build / 0x2780 vblank_callback_add: the port's (src/port/palette.cpp, proofs-4, capture-proven);
// the runtime adds the renderer's view of the look
void Runtime::palette_grey_build()
{
    port::Game<A>::palette_grey_build();
    grey_palette_sync();
}

// the vblank interrupt's list: each routine once, then the list is empty
void Runtime::vblank_run()
{
    for (uint32_t i = 0; i < 0x20; ++i) {
        const uint32_t r = mem.u32(kVblankList + i * 4);
        if (r == 0)
            break;
        mem.set32(kVblankList + i * 4, 0);
        if (r == F::palette_restore_vblank)
            palette_restore_vblank();
        else if (r == F::palette_bank2_upload)
            palette_bank2_upload();
        else if (r == 0x1F70) {
            // texture_palette_upload_vblank: the area's texture palette (count, then the words) into palette RAM
            // 0x18023E8 (the renderer has the area's palette), then vblank_words_upload 0xA35C8; the list pointer
            // 0x511F04 ends past the words (ch4proof)
            mem.set32(0x511F04, palette_list_upload(0x18023E8, mem.u32(0x511F04)));
            vblank_words_upload(0xA35C8);
        }
        else
            run(r, 0);
    }
    // (0x512064, the count vblank_callback_add 0x2780 writes, is left as the original leaves it: nothing resets it)
    palette_flush_ram();
    grey_palette_sync();   // also picks up kind 176's direct clear (clear.cpp)
}

// ch4proof (the whole-game differential, docs/proof.md): the palette and colour-row queues in work RAM, as the
// original keeps them. The renderer draws from the shadows (0x515CA0, the colour rows by Runtime::colorRows), so these
// only keep the RAM as the arcade's; before, the queues were never filled (palette_queue_add was empty) and the rows
// were not copied into the shadow 0x512D20 that palette_grey_build averages. Read from the disassembly.

// 0x1A980 palette_queue_add(first, count): entries first + 1 .. first + count appended to the list 0x51DCC0 (0 ended),
// 0x51DCB4 = 1 (an upload pending)
void Runtime::palette_queue_add(int32_t first, int32_t count)
{
    constexpr uint32_t kList = 0x51DCC0;
    uint32_t n = 0;
    while (mem.u32(kList + n * 4) != 0)
        ++n;
    for (int32_t k = 0; k < count; ++k)
        mem.set32(kList + (n++) * 4, uint32_t(first + 1 + k));
    mem.set32(kList + n * 4, 0);
    mem.set32(0x51DCB4, 1);
}

// 0x1A930 colour_row_queue(row): (row x 2 + 1, 0) appended to the list 0x51E0D0, 0x51DCB0 = 1
void Runtime::colour_row_queue(int32_t row)
{
    constexpr uint32_t kList = 0x51E0D0;
    uint32_t n = 0;
    while (mem.u32(kList + n * 4) != 0)
        ++n;
    mem.set32(kList + n * 4, uint32_t(row * 2 + 1));
    mem.set32(kList + n * 4 + 4, 0);
    mem.set32(0x51DCB0, 1);
}

// 0x1AC70 palette_flush (the vblank interrupt): its work RAM part; the uploads to palette RAM 0x1800000 / the colour
// tables 0x1810000 (0x1AF60 / 0x1B010 / 0x1B0C0 / 0x1B1C0) write no work RAM
void Runtime::palette_flush_ram()
{
    if (mem.at<uint16_t>(0x51F9EE) != 0)
        return;
    mem.at<uint16_t>(0x51F9EC) = 1;
    if (mem.at<uint16_t>(0x51F702) != 0)
        mem.set32(0x51DCB4, 0);
    if (mem.at<uint16_t>(0x51F704) != 0)
        mem.set32(0x51DCB0, 0);
    if (mem.at<uint16_t>(0x51F706) != 0) {
        const int32_t c = int32_t(mem.u32(0x517CA8));
        mem.set32(0x51DCC0, 0);
        if (c % 2 != 0)   // (the bank upload; an even count uploads all)
            mem.at<uint16_t>(0x51F706) = 0;
        mem.set32(0x517CA8, uint32_t(c + 1));
    } else if (mem.u32(0x51DCB4) != 0) {
        for (uint32_t a = 0x51DCC0; mem.u32(a) != 0; a += 4)
            mem.set32(a, 0);
        mem.set32(0x51DCB4, 0);
    }
    if (mem.at<uint16_t>(0x51E8C4) != 0) {
        const int32_t c = int32_t(mem.u32(0x517CAC));
        mem.set32(0x51E0D0, 0);
        if (c % 2 != 0)
            mem.at<uint16_t>(0x51E8C4) = 0;
        mem.set32(0x517CAC, uint32_t(c + 1));
    } else if (mem.u32(0x51DCB0) != 0) {
        for (uint32_t a = 0x51E0D0; mem.u32(a) != 0; a += 4)
            mem.set32(a, 0);
        mem.set32(0x51DCB0, 0);
    }
    mem.at<uint16_t>(0x51F9EC) = 0;
}

// 0x1A300 palette_bank2_upload / 0x1A710 palette_restore_vblank: the port's (src/port/palette.cpp, proofs-5); the
// runtime adds the renderer's view of the look
void Runtime::palette_bank2_upload()
{
    port::Game<A>::palette_bank2_upload();
}

void Runtime::palette_restore_vblank()
{
    port::Game<A>::palette_restore_vblank();
    grey_palette_sync();
}

// the renderer's view of the look (Runtime::greyPalette)
void Runtime::grey_palette_sync()
{
    const bool on = mem.at<uint16_t>(G::palette_grey_on) != 0;
    if (on && !greyPalette.on)
        for (uint32_t i = 0; i < 64; ++i)
            for (uint32_t c = 0; c < 3; ++c) {
                const int v = mem.at<int16_t>(kSepia + i * 6 + c * 2);
                greyPalette.sepia[i][c] = uint8_t(v < 0 ? 0 : v > 255 ? 255 : v);
            }
    if (on != greyPalette.on) {
        greyPalette.on = on;
        ++greyPalette.serial;
        if (log)
            log(on ? "grey palette on (palette_grey_build; sepia[0/32/63] = " + std::to_string(greyPalette.sepia[0][0]) + "," +
                         std::to_string(greyPalette.sepia[0][1]) + "," + std::to_string(greyPalette.sepia[0][2]) + " / " +
                         std::to_string(greyPalette.sepia[32][0]) + "," + std::to_string(greyPalette.sepia[32][1]) + "," +
                         std::to_string(greyPalette.sepia[32][2]) + " / " + std::to_string(greyPalette.sepia[63][0]) + "," +
                         std::to_string(greyPalette.sepia[63][1]) + "," + std::to_string(greyPalette.sepia[63][2]) + ")"
                   : std::string("grey palette off"));
    }
}

// arcfix2: palette RAM bank 2 (0x1802000, the polygon palette, 1024 RGB555 words). The renderer draws from the asset
// palettes (src/assets/arcade_loader.cpp) and Runtime::polyPaletteAnim, so this bank is not drawn from; but
// palette_grey_build saves it into work RAM (0x51CCB0, greyed into 0x51D4B0) and palette_restore_vblank puts it back,
// so the runtime keeps it as the arcade does for that work RAM to be the arcade's. Its writers (MAME write tap on
// 0x1802000..0x18027FF from boot to Chapter 1, read from the disassembly): palette_init 0x16AB0 (boot), the area
// load's texture_palette_upload_vblank 0x1F70 + vblank_words_upload 0x24D70, palette_anim_copy 0x24E90, the grey
// upload / restore 0x1A300 / 0x1A710 and the Magician's colour word 0x180248E (hw_write32).
void Runtime::palette_ram_map()
{
    if (!mem.mapped(kPalBank2))
        mem.map(kPalBank2, 0x800);
}

// a count-prefixed list of RGB555 words into palette RAM from dst up (palette_init's and 0x1F70's loop): the address
// past the list
uint32_t Runtime::palette_list_upload(uint32_t dst, uint32_t list)
{
    const uint32_t n = mem.at<uint16_t>(list);
    for (uint32_t i = 0; i < n; ++i)
        if (mem.mapped(dst))
            mem.at<uint16_t>(dst + i * 2) = mem.at<uint16_t>(list + 2 + i * 2);
    return list + 2 + 2 * n;
}

// 0x16AB0 palette_init, its bank-2 part (the boot): area 0's texture palette at entry 0, area 1's at entry 500
// (the shadow part is sprites_boot2d's; banks 0 / 1 (the text colours) are not kept)
void Runtime::palette_init_bank2()
{
    palette_ram_map();
    palette_list_upload(kPalBank2, mem.u32(0xA9960));
    palette_list_upload(0x18023E8, mem.u32(0xA9964));
}

// 0x24D70 vblank_words_upload(src): the count-prefixed record at src into palette RAM downward from entry 1023; the
// palette animation's start values (work RAM 0x520662 = ROM 0xA35CC, 0x520664..0x52069F = 0xA35CE..: the same
// words as the record's from entry 1021 down); entries 1023 and 984 from table 0xAB110 by the byte 0x51F9C0
void Runtime::vblank_words_upload(uint32_t src, bool workRam)
{
    const uint32_t n = mem.at<uint16_t>(src);
    if (mem.mapped(kPalBank2))
        for (uint32_t i = 0; i < n; ++i)
            mem.at<uint16_t>(0x18027FE - i * 2) = mem.at<uint16_t>(src + 2 + i * 2);
    if (workRam) {
        mem.at<uint16_t>(0x520662) = mem.at<uint16_t>(0xA35CC);
        for (uint32_t i = 0; i < 30; ++i)
            mem.at<uint16_t>(0x520664 + i * 2) = mem.at<uint16_t>(0xA35CE + i * 2);
    }
    const uint16_t c = mem.at<uint16_t>(0xAB110 + 2 * uint32_t(mem.at<uint8_t>(0x51F9C0)));
    if (mem.mapped(kPalBank2)) {
        mem.at<uint16_t>(0x18027FE) = c;
        mem.at<uint16_t>(0x18027B0) = c;
    }
}

// the differential's start (--diff-replay, from the MAME run's snapshot): the runtime's own 2D set-up (sprites_ensure:
// tile / character RAM, the banks) done now, its work RAM writes undone (the snapshot already has the arcade's: the
// sprite banks' palette uploads queued again had left stale words in the queue 0x51DCC0); palette bank 2 as the MAME
// run had it (palette2.bin); an older recording without it: rebuilt from what the snapshot's work RAM says
// (palette_init's part, the current area's texture palette and record 0xA35C8's colours; entries an earlier, longer
// area list wrote past this one's stay 0)
void Runtime::diff_replay_start(const std::vector<uint8_t>& bank)
{
    const std::vector<uint8_t> ram(mem.ptr(port::Mem::kRam), mem.ptr(port::Mem::kRam) + port::Mem::kRamSize);
    sprites_ensure();
    std::memcpy(mem.ptr(port::Mem::kRam), ram.data(), ram.size());
    palette_ram_map();
    if (bank.size() == 0x800) {
        std::memcpy(mem.ptr(kPalBank2), bank.data(), 0x800);
        return;
    }
    palette_init_bank2();
    const uint32_t area = mem.u32(G::texture_area);
    if (area < 13)
        palette_list_upload(0x18023E8, mem.u32(0xA9960 + area * 4));
    const uint32_t n = mem.at<uint16_t>(0xA35C8);
    for (uint32_t i = 0; i < n; ++i)
        mem.at<uint16_t>(0x18027FE - i * 2) = mem.at<uint16_t>(0xA35CA + i * 2);
    const uint16_t c = mem.at<uint16_t>(0xAB110 + 2 * uint32_t(mem.at<uint8_t>(0x51F9C0)));
    mem.at<uint16_t>(0x18027FE) = c;
    mem.at<uint16_t>(0x18027B0) = c;
}

// the differential: a once-list routine the arcade's vblank interrupt ran in the middle of the frame's logic (the
// interrupt hits long routines such as chapter_load: its texture area's palette upload). The replay's writes bring its
// work RAM part and empty its slot, so the runtime's vblank would never run it: its palette RAM part here
void Runtime::diff_vblank_ran(uint32_t routine, uint32_t listPtr)
{
    if (routine == 0x1F70) {
        palette_list_upload(0x18023E8, listPtr);
        vblank_words_upload(0xA35C8, false);
    } else if (routine == F::palette_bank2_upload) {
        port::Game<A>::palette_bank2_from(0x51D4B0);
    } else if (routine == F::palette_restore_vblank) {
        port::Game<A>::palette_bank2_from(0x51CCB0);
    }
}

// kind 182 (0x9B600 .. 0x9B660): the port's (src/port/support_c.cpp, proofs-7)

} // namespace hotd::game
