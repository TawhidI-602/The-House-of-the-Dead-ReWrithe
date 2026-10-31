// The per-type part damage routines (table 0xC66A0 by hit_type, called by part_hit through part_damage_callback, and
// some of them as object_draw's part hooks): the damage models that cost a creature its head or arms (parts_lost,
// severed children, blood drips). First written as runtime overrides (T1, src/game/t1_bounds.cpp), read from the
// disassembly. Arcade forms (decl/support_b.h).
#include "port/game.h"

namespace hotd::port {

namespace {
bool damageModelIn(uint32_t model, std::initializer_list<uint32_t> models)
{
    for (uint32_t m : models)
        if (m == model)
            return true;
    return false;
}
} // namespace

// the i-th child of a part (+0x24 list, +0x34 count) severed (+0x28 + i bit 0)
template <class P> void Game<P>::part_child_sever(uint32_t part, uint32_t i, bool drip)
{
    mem.at<uint8_t>(part + 0x28 + i) |= 1;
    if (drip) {
        const uint32_t c = mem.u32(mem.u32(part + 0x24) + 4 * i);
        blood_drip_at(mem.f32(c + 0x5C), mem.f32(c + 0x60), mem.f32(c + 0x64));
    }
}

// the first child at the joint severed with a blood drip (the head / an arm)
template <class P> void Game<P>::part_child_sever_first(uint32_t part, uint32_t joint)
{
    const int32_t n = int32_t(mem.u32(part + 0x34));
    for (int32_t i = 0; i < n; ++i)
        if (mem.u32(mem.u32(mem.u32(part + 0x24) + 4 * uint32_t(i)) + 0x38) == joint) {
            part_child_sever(part, uint32_t(i), true);
            return;
        }
}

// types 11 / 12 / 16 / 24 / 31 / 32 / 57 (zom, debu, zonbiman, mummy, haris, gilmoa, mummy2): at the type's last damage
// models the head comes off: parts_lost |= 7, the children of joint 2 severed with a blood drip, of joints 3 / 6
// without, +0x3C7 bit 3 cleared
template <class P> void Game<P>::part_damage_head_off(uint32_t part)
{
    const uint32_t body = mem.u32(G::body_current);
    mem.at<uint8_t>(body + 0x2E4) |= 7;
    for (int32_t i = 0; i < int32_t(mem.u32(part + 0x34)); ++i) {
        if (mem.u32(mem.u32(mem.u32(part + 0x24) + 4 * uint32_t(i)) + 0x38) == 2)
            part_child_sever(part, uint32_t(i), true);
        const uint32_t j = mem.u32(mem.u32(mem.u32(part + 0x24) + 4 * uint32_t(i)) + 0x38);
        if (j == 3 || j == 6)
            part_child_sever(part, uint32_t(i), false);
    }
    mem.at<uint8_t>(mem.u32(G::body_current) + 0x3C7) &= 0xF7;
}

// 0x689E0 part_damage_type00_dkiller(part) (types 0, 8, 40, 79-81): the last head damage models (0x84F / 0x86C /
// 0x881 / 0x88C / 0xB8A / 0xB9F) lose the head (parts_lost bit 0, the first child severed with a blood drip); the
// models before them (0x845 / 0x863 / 0x87B / 0x886 / 0xB7F / 0xB99) only sever the first child; the placeholder model
// 1 gives its parent the type's damage model (+0x79 = 1) and +0x78 = 1
template <class P> void Game<P>::part_damage_type00_dkiller(uint32_t part)
{
    if (damageModelIn(mem.u32(part), {0x881, 0x84F, 0x86C, 0xB8A, 0x88C, 0xB9F})) {
        mem.at<uint8_t>(mem.u32(G::body_current) + 0x2E4) |= 1;
        part_child_sever(part, 0, true);
    }
    const uint32_t m = mem.u32(part);
    if (damageModelIn(m, {0x87B, 0x845, 0x863, 0xB7F, 0x886, 0xB99}))
        mem.at<uint8_t>(part + 0x28) |= 1;
    if (m != 1)
        return;
    mem.at<uint8_t>(mem.u32(part + 0x20) + 0x79) = 1;
    uint32_t model = 0;
    switch (mem.u32(G::hit_type)) {
    case 0x50: model = 0x88C; break;
    case 0x4F: model = 0x881; break;
    case 0x28: model = 0xB8A; break;
    case 0: model = 0x84F; break;
    case 8: model = 0x86C; break;
    case 0x51: model = 0xB9F; break;
    default: break;
    }
    if (model)
        mem.set32(mem.u32(part + 0x20), model);
    mem.at<uint8_t>(mem.u32(part + 0x20) + 0x78) = 1;
}

// 0x68B50 part_damage_type01_mon(part) (kind 70): by the part's damage model: 0xC8E / 0xCC1 / 0xCAA lose the head
// (bit 0, the first child of joint 2), 0xCA9 the joint-3 child (bit 1), 0xCA8 the joint-6 child (bit 2), 0xC8D /
// 0xCC0 both arms (bits 1 / 2, every child of joints 3 / 6); each severed child with a blood drip
template <class P> void Game<P>::part_damage_type01_mon(uint32_t part)
{
    const uint32_t model = mem.u32(part);
    if (model == 0xC8E || model == 0xCC1) {
        mem.at<uint8_t>(mem.u32(G::body_current) + 0x2E4) |= 1;
        part_child_sever_first(part, 2);
    } else if (model == 0xC8D || model == 0xCC0) {
        mem.at<uint8_t>(mem.u32(G::body_current) + 0x2E4) |= 6;
        for (int32_t i = 0; i < int32_t(mem.u32(part + 0x34)); ++i) {
            const uint32_t j = mem.u32(mem.u32(mem.u32(part + 0x24) + 4 * uint32_t(i)) + 0x38);
            if (j == 3 || j == 6)
                part_child_sever(part, uint32_t(i), true);
        }
    } else if (model == 0xCA8) {
        mem.at<uint8_t>(mem.u32(G::body_current) + 0x2E4) |= 4;
        part_child_sever_first(part, 6);
    } else if (model == 0xCA9) {
        mem.at<uint8_t>(mem.u32(G::body_current) + 0x2E4) |= 2;
        part_child_sever_first(part, 3);
    } else if (model == 0xCAA) {
        mem.at<uint8_t>(mem.u32(G::body_current) + 0x2E4) |= 1;
        part_child_sever_first(part, 2);
    }
}

// 0x68E20 part_damage_type18_semu(part): models 0x17F / 0x580 / 0xAAA / 0x1482: parts_lost |= 7, the children of joints
// 2, 3 and 6 severed, each with a blood drip
template <class P> void Game<P>::part_damage_type18_semu(uint32_t part)
{
    if (!damageModelIn(mem.u32(part), {0x580, 0x17F, 0xAAA, 0x1482}))
        return;
    mem.at<uint8_t>(mem.u32(G::body_current) + 0x2E4) |= 7;
    for (int32_t i = 0; i < int32_t(mem.u32(part + 0x34)); ++i) {
        const uint32_t j = mem.u32(mem.u32(mem.u32(part + 0x24) + 4 * uint32_t(i)) + 0x38);
        if (j == 2 || j == 3 || j == 6)
            part_child_sever(part, uint32_t(i), true);
    }
}

// 0x68EC0 part_damage_type53_tetuman(part): model 0x732: parts_lost |= 2, the children of joint 3 severed
template <class P> void Game<P>::part_damage_type53_tetuman(uint32_t part)
{
    if (mem.u32(part) != 0x732)
        return;
    mem.at<uint8_t>(mem.u32(G::body_current) + 0x2E4) |= 2;
    for (int32_t i = 0; i < int32_t(mem.u32(part + 0x34)); ++i)
        if (mem.u32(mem.u32(mem.u32(part + 0x24) + 4 * uint32_t(i)) + 0x38) == 3)
            part_child_sever(part, uint32_t(i), false);
}

// 0x68F20 part_damage_type11_zom
template <class P> void Game<P>::part_damage_type11_zom(uint32_t part)
{
    if (damageModelIn(mem.u32(part), {0x78F, 0x460, 0x5BA, 0x16EB, 0x139D, 0x1940}))
        part_damage_head_off(part);
}

// 0x69010 part_damage_type16_zonbiman
template <class P> void Game<P>::part_damage_type16_zonbiman(uint32_t part)
{
    if (mem.u32(part) == 0x7B0)
        part_damage_head_off(part);
}

// 0x690C0 part_damage_type12_debu
template <class P> void Game<P>::part_damage_type12_debu(uint32_t part)
{
    if (damageModelIn(mem.u32(part), {0x13C6, 0x1135, 0x553, 0x125C, 0x12B7, 0x175A, 0x151E, 0x176D, 0x1955}))
        part_damage_head_off(part);
}

// 0x691E0 part_damage_type32_gilmoa
template <class P> void Game<P>::part_damage_type32_gilmoa(uint32_t part)
{
    if (damageModelIn(mem.u32(part), {0x1338, 0x1145, 0x152A, 0x17DA}))
        part_damage_head_off(part);
}

// 0x692C0 part_damage_type31_haris
template <class P> void Game<P>::part_damage_type31_haris(uint32_t part)
{
    if (damageModelIn(mem.u32(part), {0x117B, 0xDF8, 0x1590, 0x183D}))
        part_damage_head_off(part);
}

// 0x693A0 part_damage_type24_mummy (Kageo)
template <class P> void Game<P>::part_damage_type24_mummy(uint32_t part)
{
    if (damageModelIn(mem.u32(part), {0x1607, 0x965, 0x1A45}))
        part_damage_head_off(part);
}

// 0x69470 part_damage_type57_mummy2 (Bourbon)
template <class P> void Game<P>::part_damage_type57_mummy2(uint32_t part)
{
    if (damageModelIn(mem.u32(part), {0x15B0, 0x945, 0x19E7}))
        part_damage_head_off(part);
}

} // namespace hotd::port
