// The 2D layer's sprite banks (proofs-5): the bank records 0x5123F0 (0x80 x 16 bytes: +0 first character RAM slot,
// -1 = not loaded; +4 slots; +8 pieces (s16); +0xA locked (s16); +0xC the tiles' source) and the slot map 0x512C00
// (0x80 bytes, one per 128-tile slot of character RAM 0x1080000). First written in the runtime (src/game/sprites.cpp,
// hud), moved here for their capture proofs. Arcade forms only (the PC forms are replayed). Declarations:
// decl/support_b.h, decl/chariot.h.
#include "port/game.h"

namespace hotd::port {

namespace sprite_banks_detail {
constexpr uint32_t kBanks = 0x5123F0, kSlotMap = 0x512C00, kSlotsFixedB = 0x512BF0, kSheets = 0xA6590;
constexpr uint32_t kCharRam = 0x1080000, kPalShadow = 0x515CA0;
} // namespace sprite_banks_detail

// 0x17A10 sprite_slots_mark: the first 0x512BF0 slots used, the others free; the unlocked records free
template <class P> void Game<P>::sprite_slots_mark()
{
    using namespace sprite_banks_detail;
    if constexpr (!P::pc) {
        const int32_t fixed = int32_t(mem.u32(kSlotsFixedB));
        for (int32_t i = 0; i < 0x80; ++i)
            mem.template at<uint8_t>(kSlotMap + uint32_t(i)) = i < fixed ? 1 : 0;
        for (uint32_t b = 0; b < 0x80; ++b)
            if (mem.template at<int16_t>(kBanks + b * 16 + 10) == 0)
                mem.set32(kBanks + b * 16, 0xFFFFFFFFu);
    }
}

// 0x17930 sprite_banks_init (boot): every record free and unlocked, slot 0 kept (the font), the four fixed banks
// loaded and locked, the text colour, then the slots in use counted into 0x512BF0
template <class P> void Game<P>::sprite_banks_init()
{
    using namespace sprite_banks_detail;
    if constexpr (!P::pc) {
        for (uint32_t b = 0; b < 0x80; ++b) {
            mem.template at<int16_t>(kBanks + b * 16 + 10) = 0;
            mem.set32(kBanks + b * 16, 0xFFFFFFFFu);
        }
        mem.set32(kSlotsFixedB, 1);
        sprite_slots_mark();
        sprite_bank_load(0x21, 0x3D);
        mem.template at<int16_t>(kBanks + 0x21 * 16 + 10) = 1;
        sprite_bank_load(0xD, 0x5E);
        mem.template at<int16_t>(kBanks + 0xD * 16 + 10) = 1;
        sprite_bank_load(0x2D, 0x22B);
        mem.template at<int16_t>(kBanks + 0x2D * 16 + 10) = 1;
        sprite_bank_load(0xC, 0x8E);
        mem.template at<int16_t>(kBanks + 0xC * 16 + 10) = 1;
        palette_text_white();
        mem.set32(kSlotsFixedB, 0);
        while (mem.template at<uint8_t>(kSlotMap + mem.u32(kSlotsFixedB)) != 0) {
            mem.set32(kSlotsFixedB, mem.u32(kSlotsFixedB) + 1);
            if (int32_t(mem.u32(kSlotsFixedB)) >= 0x80)
                break;
        }
    }
}

// 0x1B270 palette_text_white: palette shadow entry 0x515DC6 = 0xF7BD (near white)
template <class P> void Game<P>::palette_text_white()
{
    mem.template at<uint16_t>(0x515DC6) = 0xF7BD;
}

// 0x17AA0 sprite_bank_size (sheet): the slots a sheet's tiles need (each piece starts on a slot: 128 tiles of 32 bytes)
template <class P> int Game<P>::sprite_bank_size(int sheet)
{
    using namespace sprite_banks_detail;
    uint32_t p = mem.u32(kSheets + uint32_t(sheet) * 12);
    int n = 0;
    do {
        const int32_t c = int32_t(mem.u32(p));
        p += uint32_t(c) * 32 + 4;
        n += int32_t((uint32_t(c) + 0x7F) & ~0x7Fu) / 0x80;
    } while (mem.u32(p) != 0);
    return n;
}

// 0x17AF0 sprite_slots_free (first, count): 1 when that run of slots is free
template <class P> uint32_t Game<P>::sprite_slots_free(int first, int count)
{
    for (int i = 0; i < count; ++i)
        if (mem.template at<uint8_t>(sprite_banks_detail::kSlotMap + uint32_t(first + i)) != 0)
            return 0;
    return 1;
}

// 0x17B20 sprite_bank_alloc (record, sheet): the record's size, then the first free run of slots (the record keeps
// -1 when there is none)
template <class P> void Game<P>::sprite_bank_alloc(uint32_t rec, int sheet)
{
    using namespace sprite_banks_detail;
    const int size = sprite_bank_size(sheet);
    mem.set32(rec + 4, uint32_t(size));
    int i = 0;
    while (mem.template at<uint8_t>(kSlotMap + uint32_t(i)) != 0 || sprite_slots_free(i, size) != 1)
        if (++i > 0x7F)
            return;
    if (size + i > 0x80)
        return;
    for (int k = 0; k < size; ++k)
        mem.template at<uint8_t>(kSlotMap + uint32_t(i + k)) = 1;
    mem.set32(rec, uint32_t(i));
}

// 0x17BD0 sprite_bank_free (bank): unless locked, its slots freed and the record marked free
template <class P> void Game<P>::sprite_bank_free(int bank)
{
    using namespace sprite_banks_detail;
    if constexpr (P::pc) {
        (void)bank;   // the PC form: not ported (vtable only; ProofPc replays it)
    } else {
        const uint32_t rec = kBanks + uint32_t(bank) * 16;
        const int32_t first = int32_t(mem.u32(rec));
        if (mem.template at<int16_t>(rec + 10) == 0 && first != -1) {
            const int32_t n = int32_t(mem.u32(rec + 4));
            for (int32_t i = 0; i < n; ++i)
                mem.template at<uint8_t>(kSlotMap + uint32_t(first + i)) = 0;
            mem.set32(rec, 0xFFFFFFFFu);
        }
    }
}

// 0x17CB0 sprite_bank_tiles_load (record, sheet): each piece to character RAM 0x1080000 + slot x 0x1000, rounded up
// to whole slots; the source and the piece count into the record (character RAM not mapped: only the record)
template <class P> void Game<P>::sprite_bank_tiles_load(uint32_t rec, int sheet)
{
    using namespace sprite_banks_detail;
    if constexpr (P::pc) {
        (void)rec, (void)sheet;
    } else {
        uint32_t p = mem.u32(kSheets + uint32_t(sheet) * 12);
        uint32_t dst = kCharRam + mem.u32(rec) * 0x1000;
        mem.set32(rec + 12, p);
        const bool ram = mem.mapped(kCharRam);
        int16_t pieces = 0;
        do {
            const uint32_t c = mem.u32(p);
            if (ram)
                for (uint32_t i = 0; i < c * 32; i += 4)
                    mem.set32(dst + i, mem.u32(p + 4 + i));
            p += 4 + c * 32;
            dst += ((c + 0x7F) & ~0x7Fu) * 32;
            ++pieces;
        } while (mem.u32(p) != 0);
        mem.template at<int16_t>(rec + 8) = pieces;
    }
}

// 0x17C10 sprite_bank_palette_load (record, sheet): each piece's palette (32 bytes) for each of its slots into the
// shadow 0x515CA0 (+ slot x 0x20), then palette_queue_add (first slot, 16-byte units written)
template <class P> void Game<P>::sprite_bank_palette_load(uint32_t rec, int sheet)
{
    using namespace sprite_banks_detail;
    const int32_t slot = int32_t(mem.u32(rec));
    uint32_t src = mem.u32(kSheets + uint32_t(sheet) * 12 + 4);
    uint32_t t = mem.u32(kSheets + uint32_t(sheet) * 12);
    const uint32_t start = kPalShadow + uint32_t(slot) * 0x20;
    uint32_t dst = start;
    do {
        const int32_t n = int32_t((mem.u32(t) + 0x7F) & ~0x7Fu) / 0x80;
        for (int32_t k = 0; k < n; ++k) {
            for (uint32_t i = 0; i < 0x20; i += 4)
                mem.set32(dst + i, mem.u32(src + i));
            dst += 0x20;
        }
        t += mem.u32(t) * 32 + 4;
        src += 0x20;
    } while (mem.u32(t) != 0);
    palette_queue_add(slot, int32_t(dst - start) / 0x10);
}

// 0x17D50 sprite_bank_load (bank, sheet): loads the bank from that sheet unless it is loaded
template <class P> void Game<P>::sprite_bank_load(int bank, int sheet)
{
    using namespace sprite_banks_detail;
    if constexpr (P::pc) {
        (void)bank, (void)sheet;
    } else {
        const uint32_t rec = kBanks + uint32_t(bank) * 16;
        if (int32_t(mem.u32(rec)) < 0) {
            sprite_bank_alloc(rec, sheet);
            sprite_bank_tiles_load(rec, sheet);
            sprite_bank_palette_load(rec, sheet);
        }
    }
}

} // namespace hotd::port
