// The experimental 4-player mode (docs/coop4.md): where players 3 / 4 keep their state in the original address
// space, and the 4-seat trait.
//
// The reserved block: work RAM 0x50C000..0x50FFFF, the upper half of the arcade's main stack. The i960 main stack
// starts at 0x500400 (the boot code at 0x6DC / 0x740: fp = 0x500400, it grows upward) and ends where the interrupt
// stack begins (0x510400: the PRCB at 0xB0, word +0x18). Why nothing of the original uses 0x50C000..0x50FFFF:
// - no 4-aligned word of the program ROM is an address in 0x508560..0x5103FF (tools/i960dis.py --xref; the only hits
//   are odd-aligned pieces of instructions), so no routine addresses it directly;
// - it is zero in all 3,591 MAME work RAM dumps of private\ (fullgame_us / _bufram: a whole game every 0.4 s, the
//   Chapter 3 runs, the hangedman fight, life / ammo scans, boot and attract): the stack is never cleared, so any frame
//   that ever reached it would have left its words there. The deepest stack word seen is 0x50855F (boot);
// - mame_boot_writers.lua's write map (power-on to Chapter 1, private\boot_writers\writers.txt) has no write at all to
//   0x508580..0x5103FF;
// - the port never uses the original stack (its locals are at 0x700000, Game::local_alloc; the runtime runs on the
//   C++ stack), and the runtime's work RAM image is a MAME dump, zero there.
// So the block keeps 16 KB of the stack's never-reached top (0x508580..0x50BFFF stays free above the deepest frame)
// and 0x400 bytes below the interrupt stack. Nothing reads or writes it unless the 4P mode is on (Game::seats4).
#pragma once

#include <cstdint>

namespace hotd::port {

struct A;

// the 4-seat trait: how many seats a layout's logic can run. The arcade logic (A) hosts the 4P mode; the PC logic
// keeps two (decided: 4P runs the arcade logic with either model source). Code for seats 3 / 4 is compiled only where
// Seats<P>::max == 4 and runs only while the runtime's Game::seats4 is set.
template <class P> struct Seats {
    static constexpr int max = 2;
};
template <> struct Seats<A> {
    static constexpr int max = 4;
};

namespace coop4 {

constexpr uint32_t kBase = 0x50C000, kSize = 0x4000;

// the block's fields (offsets from kBase): records of seats 2 / 3 (players 3 / 4) laid out like the originals' pairs,
// so `base + (p - 2) * stride` is the seat's entry
constexpr uint32_t kMagic = kBase + 0x000;          // u32 'C4P4' while the block is in use
constexpr uint32_t kPlayersReal = kBase + 0x004;    // s16: the players in play over all four seats
constexpr uint32_t kSeatsJoined = kBase + 0x006;    // u8: bit p = seat p was started in this game
constexpr uint32_t kHitTable = kBase + 0x008;       // u32: entries used in the hit table this frame
constexpr uint32_t kBodySetB = kBase + 0x00C;       // u8: 1 = coop4_body_set put the title code pair 3 (types 14 / 15)
constexpr uint32_t kEyeSeats = kBase + 0x00F;       // u8: the seats whose shots hit an eye in this enemy_part_hits
constexpr uint32_t kBossPlayers = kBase + 0x00E;    // u8: the players the boss's hit points were scaled for
constexpr uint32_t kBossCur = kBase + 0x010;        // u32: the boss's hit points (s16) being scaled, 0 = no fight
constexpr uint32_t kBossMax = kBase + 0x014;        // u32: its maximum (s16; the life bar's maximum too)
constexpr uint32_t kBossBase = kBase + 0x018;       // s32: the maximum as the arcade has it (two players)
constexpr uint32_t kTargetTurn = kBase + 0x00D;     // u8: the seat the last enemy hit went to (coop4_victim's rotation)
constexpr uint32_t kPlayers = kBase + 0x104;        // 2 x Player (0xCC); kPlayers - 4 = the "record - 4" base
constexpr uint32_t kGunInputs = kBase + 0x300;      // 2 x GunInput (0x1C)
constexpr uint32_t kGunShot = kBase + 0x340;        // 2 x u8 (gun_shot 0x5430A0)
constexpr uint32_t kInvulnerable = kBase + 0x348;   // 2 x s32 (player_invulnerable 0x51F5E0)
constexpr uint32_t kTargeted = kBase + 0x350;       // 2 x u8 (player_targeted 0x5209A0)
constexpr uint32_t kStartButtons = kBase + 0x354;   // 2 x u16 (the start switch words 0x51F99C)
constexpr uint32_t kTorches = kBase + 0x360;        // 2 x 10 bytes (0x51F5C0)
constexpr uint32_t kPopups = kBase + 0x380;         // 2 x 8 bytes (0x51F480)
constexpr uint32_t kPartTally = kBase + 0x390;      // 2 x s16 (part_tally 0x543520: the damage of this hit by seat)
constexpr uint32_t kShotResults = kBase + 0x3A0;    // 2 x ShotResult (0x28)
constexpr uint32_t kShotRays = kBase + 0x400;       // 2 x ShotRay (0x68)
constexpr uint32_t kSparkNext = kBase + 0x01C;     // u8: the next entry of the kSparks ring
constexpr uint32_t kSparks = kBase + 0x4E0;        // 4 x {u32 debris task, u32 seat}: players 3 / 4's shot sparks
constexpr uint32_t kHits = kBase + 0x500;           // the hit table: kHitMax x {u32 target (object or part), u32 seats}
constexpr uint32_t kHitMax = 0x100;
constexpr uint32_t kFlashNext = kBase + 0x01D;      // u8: the next entry of the kFlashes ring
constexpr uint32_t kFlashes = kHits + kHitMax * 8;  // 4 x {u32 flash task, u32 seat | frame << 8}: players 3 / 4's shot flashes
constexpr uint32_t kEnd = kFlashes + 4 * 8;         // 0x50CD20

// the routine word of the seat 2 / 3 tasks (no original routine has it: outside the program ROM)
constexpr uint32_t kSeatRoutine = 0x7FC04000;

static_assert(kEnd <= kBase + kSize);

} // namespace coop4

} // namespace hotd::port
