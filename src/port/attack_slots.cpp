// Attack positions (AttackSlot[16] at G::attack_slots: +0 taken, +4 holder, +8 pos[3]): the waiting / front positions
// the enemies take, and the per-frame re-ordering by depth (attack_slots_update, from game_frame). First written as
// runtime overrides (T1, src/game/t1_bounds.cpp), read from the disassembly. Arcade forms (decl/support.h).
#include "port/game.h"

#include <algorithm>
#include <cstring>

namespace hotd::port {

namespace {
constexpr uint32_t slot_addr(uint32_t base, int32_t i) { return base + uint32_t(i) * 0x14; }
} // namespace

// 0x31E60 attack_slot_take_waiting: the first free waiting position from 2 (to 13); +0x2E7 = it (14 when none, as the
// original leaves it)
template <class P> void Game<P>::attack_slot_take_waiting(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;   // the PC form: not ported (vtable only; ProofPc replays it)
    } else {
        int32_t i = 2;
        for (; i < 14; ++i)
            if (mem.at<uint8_t>(slot_addr(G::attack_slots, i)) == 0) {
                mem.set32(slot_addr(G::attack_slots, i) + 4, obj);
                mem.at<uint8_t>(slot_addr(G::attack_slots, i)) = 1;
                break;
            }
        mem.at<int8_t>(obj + 0x2E7) = int8_t(i);
    }
}

// 0x31D80 attack_slot_take_wide: a position for a big enemy: 2 / 3 / 4 by the front pairs, else 5 / 6, else the first
// free of 7-12
template <class P> void Game<P>::attack_slot_take_wide(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;
    } else {
        auto taken = [&](int32_t i) { return mem.at<uint8_t>(slot_addr(G::attack_slots, i)) != 0; };
        int32_t c = -1;
        if (!taken(4)) {
            if (!taken(2))
                c = 2;
            else
                c = taken(3) ? 4 : 3;
        } else if (!taken(5)) {
            c = taken(2) ? 5 : 2;
        } else if (!taken(6)) {
            c = taken(3) ? 6 : 3;
        } else {
            for (int32_t i = 7; i <= 12; ++i)
                if (!taken(i)) {
                    c = i;
                    break;
                }
        }
        if (c < 0)
            return;
        mem.set32(slot_addr(G::attack_slots, c) + 4, obj);
        mem.at<uint8_t>(slot_addr(G::attack_slots, c)) = 1;
        mem.at<int8_t>(obj + 0x2E7) = int8_t(c);
    }
}

// 0x32010 attack_slot_front_take: from a waiting position 2 / 3 to a free front one (0 / 1) by the player count and
// attack_side; the object's flag bit 30 cleared
template <class P> void Game<P>::attack_slot_front_take(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;
    } else {
        auto taken = [&](int32_t i) { return mem.at<uint8_t>(slot_addr(G::attack_slots, i)) != 0; };
        const int32_t cur = mem.at<int8_t>(obj + 0x2E7);
        const int16_t players = mem.at<int16_t>(G::players_in_game_b);
        const bool side = mem.at<uint8_t>(G::attack_side) != 0;
        int32_t c = -1;
        if (players == 1) {
            if (cur == 2) {
                if (!taken(0))
                    c = 0;
                else if (side && !taken(1))
                    c = 1;
            } else if (cur == 3) {
                if (side && !taken(1))
                    c = 1;
                else if (!taken(0))
                    c = 0;
            }
        } else if (players == 2) {
            if (cur == 2 && !taken(0))
                c = 0;
            else if (cur == 3 && !taken(1))
                c = 1;
        }
        if (c < 0)
            return;
        mem.at<uint8_t>(slot_addr(G::attack_slots, cur)) = 0;
        mem.at<uint8_t>(slot_addr(G::attack_slots, c)) = 1;
        mem.set32(slot_addr(G::attack_slots, cur) + 4, 0);
        mem.set32(slot_addr(G::attack_slots, c) + 4, obj);
        mem.at<int8_t>(obj + 0x2E7) = int8_t(c);
        mem.set32(obj + 0x58, mem.u32(obj + 0x58) & 0xBFFFFFFFu);
    }
}

// 0x2FF90 attack_slot_nearest_object: among positions 2 .. min(attack_users_last + 2, 13) - 1 other than its own, the
// nearest holder (coprocessor 0x1F) whose +0x324 has bit 18 and not bit 1; 0 if none
template <class P> uint32_t Game<P>::attack_slot_nearest_object(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;
        return 0;
    } else {
        int32_t end = int32_t(mem.u32(G::attack_users_last)) + 2;
        if (end > 13)
            end = 13;
        uint32_t best = 0;
        float bestD = 10000.0f;
        for (int32_t i = 2; i < end; ++i) {
            const uint32_t h = mem.u32(slot_addr(G::attack_slots, i) + 4);
            if (mem.at<int8_t>(obj + 0x2E7) == i || h == 0 || mem.at<uint8_t>(slot_addr(G::attack_slots, i)) == 0 ||
                (mem.u32(h + 0x324) & 0x40002u) != 0x40000u)
                continue;
            // the distance on the floor (y = 0), the holder first
            float d;
            if (pc_path()) {   // PC Mode (docs/pcmode.md row 10): the PC's 0x468B1C (its table square root)
                d = pm::floor_distance(mem.f32(h + 0x20), mem.f32(h + 0x28), mem.f32(obj + 0x20), mem.f32(obj + 0x28));
            } else {
                uint32_t in[6] = {mem.u32(h + 0x20), 0, mem.u32(h + 0x28), mem.u32(obj + 0x20), 0, mem.u32(obj + 0x28)};
                Tgp::Out o;
                copro(0x1F, in, 6, o);
                d = ffrom(o.w[0]);
            }
            if (d < bestD) {
                best = h;
                bestD = d;
            }
        }
        return best;
    }
}

// 0x32160 enemy_attack_slot_take: attack positions 0 / 1 (one per player): one player: position 0 when attack_side is
// clear, else the wanted one (+0x338), or with under 3 enemies the other; two players: the wanted one. The old
// position freed; 1 = taken
template <class P> uint32_t Game<P>::enemy_attack_slot_take(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;   // the PC form: not ported (vtable only; ProofPc replays it)
        return 0;
    } else {
        auto taken = [&](int32_t i) { return mem.at<uint8_t>(slot_addr(G::attack_slots, i)) != 0; };
        const int32_t want = int32_t(mem.u32(obj + 0x338));
        const int8_t had = mem.at<int8_t>(obj + 0x2E7);
        int32_t s = -1;
        const int16_t players = mem.at<int16_t>(G::players_in_game);
        if (players == 1) {
            if (mem.at<uint8_t>(G::attack_side) == 0) {
                if (!taken(0))
                    s = 0;
            } else if (!taken(want)) {
                s = int8_t(mem.at<uint8_t>(obj + 0x338));
            } else if (mem.at<int16_t>(G::enemy_count) < 3 && !taken(want == 0 ? 1 : 0)) {
                s = want == 0 ? 1 : 0;
            }
        } else if (players == 2) {
            if (want == 0) {
                if (!taken(0))
                    s = 0;
            } else if (want == 1 && !taken(1)) {
                s = 1;
            }
        }
        if (s < 0)
            return 0;
        mem.at<uint8_t>(slot_addr(G::attack_slots, had)) = 0;
        mem.set32(slot_addr(G::attack_slots, had) + 4, 0);
        mem.at<uint8_t>(slot_addr(G::attack_slots, s)) = 1;
        mem.set32(slot_addr(G::attack_slots, s) + 4, obj);
        mem.at<int8_t>(obj + 0x2E7) = int8_t(s);
        mem.set32(obj + 0x338, uint32_t(s));
        return 1;
    }
}

// 0x32C50 enemy_attack_slot_free: its position no longer taken (the holder stays), +0x2E7 = -1
template <class P> void Game<P>::enemy_attack_slot_free(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;
    } else {
        mem.at<uint8_t>(slot_addr(G::attack_slots, mem.at<int8_t>(obj + 0x2E7))) = 0;
        mem.at<int8_t>(obj + 0x2E7) = -1;
    }
}

// 0x32970 attack_slots_sort: this frame's users (attack_users: key, object; attack_users_count) sorted by key, a
// radix sort over the key's low 16 bits (two byte passes, each counting, then scattering from the last entry into the
// scratch array 0x53E240 and copying it back: stable). The scratch array keeps the last pass's order.
template <class P> void Game<P>::attack_slots_sort()
{
    constexpr uint32_t kScratch = 0x53E240;   // (arcade)
    const int32_t n = int32_t(mem.u32(G::attack_users_count));
    for (uint32_t shift = 0; shift <= 15; shift += 8) {
        int32_t hist[256] = {};
        for (int32_t i = 0; i < n; ++i)
            ++hist[(mem.u32(G::attack_users + 8 * uint32_t(i)) >> shift) & 0xFF];
        for (int32_t i = 1; i < 256; ++i)
            hist[i] += hist[i - 1];
        for (int32_t i = n - 1; i >= 0; --i) {
            const uint32_t key = mem.u32(G::attack_users + 8 * uint32_t(i)), obj = mem.u32(G::attack_users + 8 * uint32_t(i) + 4);
            const int32_t at = --hist[(key >> shift) & 0xFF];
            mem.set32(kScratch + 8 * uint32_t(at), key);
            mem.set32(kScratch + 8 * uint32_t(at) + 4, obj);
        }
        for (int32_t i = 0; i < n; ++i) {
            mem.set32(G::attack_users + 8 * uint32_t(i), mem.u32(kScratch + 8 * uint32_t(i)));
            mem.set32(G::attack_users + 8 * uint32_t(i) + 4, mem.u32(kScratch + 8 * uint32_t(i) + 4));
        }
    }
}

// 0x32440 attack_slots_update (per frame, game_frame): the users sorted by depth take positions 2, 3 ... in order,
// each with its old position's place; positions 0 / 1 keep their holders; a held waiting position whose holder is not
// a user this frame moves to max(its index, the next after the users) (the original does not step past it: two such
// holders can land on the same position); attack_users_last = the count, the count cleared. The original's frame is
// kept as it lays it out (marks at fp+0x40, the old positions at fp+0x50): a user's position index is not range
// checked, so -1 / 14 / 15 land in the marks' padding (zero here, stale stack in the original)
template <class P> void Game<P>::attack_slots_update()
{
    attack_slots_sort();
    static thread_local uint8_t fr[0x1800];   // fr[0xC00 + fp offset] (any int8 position index stays inside)
    std::memset(fr, 0, sizeof fr);
    auto mark = [&](int32_t i) -> uint8_t& { return fr[0xC00 + 0x40 + i]; };
    auto oldAt = [&](int32_t i) { return 0xC00 + 0x50 + i * 0x14; };   // index of old[i] in fr
    auto frU32 = [&](int32_t at) { uint32_t v; std::memcpy(&v, &fr[at], 4); return v; };
    for (int32_t i = 0; i < 14; ++i) {
        for (uint32_t k = 0; k < 0x14; ++k)
            fr[oldAt(i) + int32_t(k)] = mem.at<uint8_t>(slot_addr(G::attack_slots, i) + k);
        mark(i) = 0;
    }
    const int32_t n = int32_t(mem.u32(G::attack_users_count));
    for (int32_t u = 0; u < n; ++u)
        mark(mem.at<int8_t>(mem.u32(G::attack_users + 8 * uint32_t(u) + 4) + 0x2E7)) = 1;
    for (int32_t i = 0; i < 14; ++i)
        if (fr[oldAt(i)] != 0 && mark(i) != 1)
            mark(i) = 2;
    for (int32_t i = 0; i < 14; ++i) {
        mem.at<uint8_t>(slot_addr(G::attack_slots, i)) = 0;
        mem.set32(slot_addr(G::attack_slots, i) + 4, 0);
    }
    int32_t next = 2;
    for (int32_t u = 0; u < int32_t(mem.u32(G::attack_users_count)); ++u, ++next) {
        const uint32_t o = mem.u32(G::attack_users + 8 * uint32_t(u) + 4);
        const int32_t s = mem.at<int8_t>(o + 0x2E7);
        mem.at<int8_t>(o + 0x2E7) = int8_t(next);
        mem.set32(slot_addr(G::attack_slots, next) + 8, frU32(oldAt(s) + 8));
        mem.set32(slot_addr(G::attack_slots, next) + 0xC, frU32(oldAt(s) + 0xC));
        mem.at<uint8_t>(slot_addr(G::attack_slots, next)) = 1;
        mem.set32(slot_addr(G::attack_slots, next) + 4, o);
        mem.set32(slot_addr(G::attack_slots, next) + 0x10, frU32(oldAt(s) + 0x10));
        mem.set32(o + 0x58, mem.u32(o + 0x58) & 0xBFFFFFFFu);
    }
    for (int32_t i = 0; i < 2; ++i)
        if (fr[oldAt(i)] != 0)
            for (uint32_t k = 0; k < 0x14; ++k)
                mem.at<uint8_t>(slot_addr(G::attack_slots, i) + k) = fr[oldAt(i) + int32_t(k)];
    for (int32_t i = 2; i <= 13; ++i) {
        if (mark(i) != 2)
            continue;
        next = std::max(i, next);
        const uint32_t holder = frU32(oldAt(i) + 4);
        mem.at<int8_t>(holder + 0x2E7) = int8_t(next);
        mem.at<uint8_t>(slot_addr(G::attack_slots, next)) = 1;
        mem.set32(slot_addr(G::attack_slots, next) + 4, holder);
        mem.set32(slot_addr(G::attack_slots, next) + 8, frU32(oldAt(i) + 8));
        mem.set32(slot_addr(G::attack_slots, next) + 0xC, frU32(oldAt(i) + 0xC));
        mem.set32(slot_addr(G::attack_slots, next) + 0x10, frU32(oldAt(i) + 0x10));
    }
    const uint32_t count = mem.u32(G::attack_users_count);
    mem.set32(G::attack_users_count, 0);
    mem.set32(G::attack_users_last, count);
}

} // namespace hotd::port
