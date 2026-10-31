// The work RAM at power-on, built from the program ROM (boot.h). UNVERIFIED (teaser): read from the disassembly /
// typed decompilation, checked by diffing against MAME's work RAM at Chapter 1's chapter_load
// (tools\run_boot_writers.ps1; docs/teaser_status/s1.md lists every range that still differs and why), no capture proof.
//
// The original's order: crt_copy_data, boot_start (memory clear, boot_hw_init, boot_globals_reset, first mode,
// srand, heap_reset), then the attract / title modes run until the start button, then game_new -> chapter_load
// (Runtime::chapter_start). What the attract and title modes leave behind (their tasks on the heap, event-script and
// camera leftovers, palette animation state) is overwritten or reset by chapter_load and the chapter's own tasks, so it
// is not rebuilt. The hardware's own state (sound board queue 0x512340.., sprite layer bookkeeping 0x5123F0..,
// palette upload queue 0x517CA0.., IRQ timers 0x51E911.., geometrizer log table 0x500884..) belongs to the engine:
// the rewrite has its own sound, HUD and renderer.
#include "game/boot.h"

#include "game/runtime.h"

#include <cstring>

namespace hotd::game {

namespace {

// 0x5F0 crt_copy_data, the work RAM part: the initialised data 0x2B40.. -> 0x510800..0x510C03 and 0xB0.. ->
// 0x510E00..0x510EAF (the first loop copies 0x580.. to 0xE00000, not work RAM), then 0x510E14 = 0x510800
void crt_copy_data(Mem& m)
{
    for (uint32_t a = 0x510800, s = 0x2B40; a < 0x510C04; a += 4, s += 4)
        m.set32(a, m.u32(s));
    for (uint32_t a = 0x510E00, s = 0xB0; a < 0x510EB0; a += 4, s += 4)
        m.set32(a, m.u32(s));
    m.set32(0x510E14, 0x510800);
}

void copy(Mem& m, uint32_t dst, uint32_t src, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i)
        m.at<uint8_t>(dst + i) = m.at<uint8_t>(src + i);
}

// 0x156C0 input_defaults_init: the default input / gun records (ROM 0xA9A50 / 0xA9A60 / 0xA9A70 / 0xA9A78) into the
// two players' input blocks 0x51F4D0.. / 0x51F514.. (the IO port writes are the engine's)
void input_defaults_init(Mem& m)
{
    copy(m, 0x51F4EC, 0xA9A50, 16);
    copy(m, 0x51F4FC, 0xA9A60, 12);
    copy(m, 0x51F4D0, 0x51F4EC, 16);
    copy(m, 0x51F4E0, 0x51F4FC, 8);
    copy(m, 0x51F4E8, 0x51F504, 4);
    copy(m, 0x51F508, 0xA9A70, 8);
    copy(m, 0x51F510, 0xA9A78, 4);
    copy(m, 0x51F530, 0xA9A50, 16);
    copy(m, 0x51F540, 0xA9A60, 12);
    copy(m, 0x51F514, 0x51F530, 16);
    copy(m, 0x51F524, 0x51F540, 8);
    copy(m, 0x51F52C, 0x51F548, 4);
    copy(m, 0x51F54C, 0xA9A70, 8);
    copy(m, 0x51F554, 0xA9A78, 4);
    m.at<uint16_t>(0x51F4FC) = 0;
    m.at<uint16_t>(0x51F4E0) = 0;
    m.at<uint16_t>(0x51F512) = 0;
}

// 0x22C70 eeprom_checksum
uint32_t eeprom_checksum(Mem& m, uint32_t p, uint32_t n)
{
    uint32_t v = 0xDEBDEB00u;
    auto step = [&]() { v = v < 0x80000000u ? v * 2 : v * 2 + 0x10210000u; };
    for (uint32_t i = 0; i < n; ++i) {
        v += m.at<uint8_t>(p + i);
        for (int k = 0; k < 8; ++k)
            step();
    }
    for (int k = 0; k < 8; ++k)
        step();
    return v >> 16;
}

// 0x227E0 eeprom_load as on a machine whose EEPROM holds the factory settings (eeprom_defaults 0x22A20: "S32A",
// version 0x6655, the 0x28 setting bytes of ROM 0xAA530; eeprom_save 0x22910: save counter, checksum, the second copy
// 0x51F740) with the operator's choices the teaser makes for the player: USA (game setting 0x51F729 = 1, the region),
// free play (COIN/CREDIT SETTING 0x51F720 = 26, coin mode 0x51F721 = 1). Difficulty, lives and blood colour come from
// the launcher (Runtime::apply_settings), not from this image.
void eeprom_load(Mem& m)
{
    for (uint32_t i = 0; i < 0x58; ++i)
        m.at<uint8_t>(0x51F710 + i) = 0;
    m.set32(0x51F710, 0x41323353u);   // "S32A"
    m.at<uint16_t>(0x51F714) = 0x6655;
    copy(m, 0x51F718, 0xAA530, 0x28);
    m.at<uint8_t>(0x51F720) = 26;
    m.at<uint8_t>(0x51F721) = 1;
    m.at<uint8_t>(0x51F729) = 1;
    m.set32(0x51F71C, m.u32(0x51F71C) + 1);
    m.at<uint16_t>(0x51F718) = uint16_t(eeprom_checksum(m, 0x51F71A, 0x26));
    copy(m, 0x51F740, 0x51F718, 0x28);   // settings_copy_backup 0x22B70
}

// 0xDC20 apply_settings, the parts Runtime::apply_settings leaves out: the region (0x51F791), the attract sound
// (0x51F9C1) and the guns' calibration records (gun_inputs 0x51F574 + 0x1C a player: +4 x offset, +8 y offset,
// +0xC x range, +0x10 y range, from ADXMIN / ADYMIN / ADXMAX / ADYMAX 0x51F730 + 8 a player)
void apply_settings_boot(Mem& m)
{
    const uint8_t region = m.at<uint8_t>(0x51F729);
    m.at<uint8_t>(0x51F791) = region > 2 ? 0 : region;
    const uint8_t sound = m.at<uint8_t>(0x51F72B);
    m.at<uint8_t>(0x51F9C1) = sound > 1 ? 1 : sound;
    for (uint32_t p = 0; p < 2; ++p) {
        const float xmin = float(m.at<int16_t>(0x51F730 + p * 8)), ymin = float(m.at<int16_t>(0x51F732 + p * 8));
        const float xmax = float(m.at<int16_t>(0x51F734 + p * 8)), ymax = float(m.at<int16_t>(0x51F736 + p * 8));
        const uint32_t g = 0x51F584 + p * 0x1C;
        auto setf = [&](uint32_t a, float v) { std::memcpy(m.ptr(a), &v, 4); };
        setf(g - 0xC, xmax * -216.0f - xmin * 215.0f);
        setf(g - 8, ymax * 159.0f - ymin * -160.0f);
        setf(g - 4, xmax == xmin ? 1.0f : float(int32_t(xmax) - int32_t(xmin)));
        setf(g, ymax == ymin ? 1.0f : float(int32_t(ymax) - int32_t(ymin)));
    }
}

// 0x1EBA0 coin_settings_init: the coin settings from the EEPROM image (the eeprom_coin_* getters 0x23790..0x23880: a
// byte out of range reads the ROM default at 0xAA538 + its offset); the backup RAM's credits (0x1D0000A / 0x1D0000C)
// are 0 on this machine; max credits 24 (USA) else 9
void coin_settings_init(Mem& m)
{
    auto setting = [&](uint32_t off, uint8_t hi) -> int32_t {
        const uint8_t v = m.at<uint8_t>(0x51F720 + off);
        return v > hi ? m.at<uint8_t>(0xAA538 + off) : v;
    };
    m.at<int16_t>(0x51E810) = int16_t(setting(0, 27));   // COIN/CREDIT SETTING #
    m.set32(0x51FDB8, uint32_t(setting(1, 2)));          // coin mode (1 = free play)
    m.set32(0x51FDBC, uint32_t(setting(8, 1)));          // COIN CHUTE TYPE
    m.set32(0x51F604, uint32_t(setting(6, 5)));          // CREDIT TO START
    m.set32(0x51F608, uint32_t(setting(7, 5)));          // CREDIT TO CONTINUE
    m.at<int16_t>(0x51E812) = int16_t(setting(2, 9));    // COIN TO CREDIT
    m.at<int16_t>(0x51E820) = int16_t(setting(3, 9));    // chute multipliers
    m.at<int16_t>(0x51E822) = int16_t(setting(4, 9));
    m.at<int16_t>(0x51E814) = int16_t(setting(5, 9));    // BONUS ADDER
    const uint32_t maxCredits = m.at<uint8_t>(0x51F791) == 1 ? 0x18 : 9;
    m.set32(0x51F708, maxCredits);
    for (uint32_t a : {0x51E868u, 0x51E888u, 0x51E874u, 0x51E894u, 0x51E860u, 0x51E880u, 0x51E86Cu, 0x51E88Cu,
                       0x51E818u, 0x51E81Cu, 0x51F4B8u, 0x51F4BCu, 0x51F4C0u, 0x51F4C4u, 0x51F9C8u, 0x51F9CCu,
                       0x51FDC0u, 0x51FDC4u})
        m.set32(a, 0);
    for (uint32_t a = 0x51E840; a < 0x51E850; a += 4)
        m.set32(a, 0);
    m.set32(0x51E850, 0);
    m.set32(0x51E854, 1);
    for (uint32_t a : {0x51E824u, 0x51E826u, 0x51E828u, 0x51E82Au, 0x51E82Cu, 0x51E82Eu})
        m.at<int16_t>(a) = 0;
}

// 0x34410 Make_ObjColiTbl (the developers' name): the per-part hit spheres. ROM 0x1359B0: 0x5E lists of 0x384 bytes,
// each a run of 5-word entries (part index, 4 words) ending with index -1; each entry's 4 words go to
// 0x520B10 + index x 16
void Make_ObjColiTbl(Mem& m)
{
    for (uint32_t list = 0; list <= 0x5D; ++list) {
        const uint32_t base = 0x1359B0 + list * 0x384;
        for (uint32_t k = 0; m.u32(base + k * 20) != 0xFFFFFFFFu; ++k) {
            const uint32_t e = base + k * 20;
            copy(m, 0x520B10 + m.u32(e) * 16, e + 4, 16);
        }
    }
}

// 0xDB00 boot_globals_reset
void boot_globals_reset(Mem& m)
{
    Make_ObjColiTbl(m);
    for (uint32_t a : {0x51F798u, 0x51F778u, 0x51F9ACu, 0x51F79Au, 0x51F4C8u, 0x51F4A4u, 0x51F4A0u, 0x51F4A6u, 0x51F4A2u})
        m.at<int16_t>(a) = 0;
    for (uint32_t a : {0x51F9B2u, 0x51F9B1u, 0x51F9B0u, 0x51F280u})
        m.at<uint8_t>(a) = 0;
    m.set32(0x51F9A8, 0x0FFFFFFF);
    m.set32(0x51F9B4, 0xFFFFFFFFu);   // texture_area: none loaded
    m.set32(0x51F9B8, 0);
    m.at<int16_t>(0x51F9A0) = 1;
    m.set32(0x51F4B0, 1);             // ammo_enabled
    m.set32(0x51F284, 9);             // player 1 state: waiting
    m.set32(0x51F350, 9);             // player 2 state
    for (uint32_t a : {0x51F324u, 0x51F3F0u, 0x51F42Cu, 0x51F428u, 0x51F420u, 0x51F794u, 0x51F628u, 0x51F630u, 0x51F9BCu})
        m.set32(a, 0);
    m.at<uint8_t>(0x51F34C) = 1;
    m.set32(0x51F564, 3);             // camera_status
}

} // namespace

void boot_work_ram(Runtime& rt)
{
    Mem& m = rt.mem;
    crt_copy_data(m);
    // boot_start 0xD7C0: memory clear (mem_clear16 0x510EB8.., already zero), gun_test_mode 0, boot_hw_init
    input_defaults_init(m);
    eeprom_load(m);
    rt.apply_settings();
    apply_settings_boot(m);
    coin_settings_init(m);
    boot_globals_reset(m);
    // the first mode by region (USA: 1, the attract mode); srand_leaf(123456) seeds crand (0x543AF0)
    const uint8_t region = m.at<uint8_t>(0x51F791);
    m.at<int16_t>(0x51F494) = int16_t(region == 0 ? (m.u32(0x510EB4) != 1 ? 9 : 0) : region == 1 ? 1 : 9);
    m.set32(0x543AF0, 123456);
    // what the attract mode leaves that the game reads before setting it: the players' cut-scene body routines
    // (player block +0xB0, 0x51F334 / 0x51F400), written by the attract's camdr_mode3 (0x2D7AC / 0x2D7B4) as
    // 0x4B820 (a player_body_at_camera wrapper); the chapter's first frame calls it before CAMDR sets its own
    m.set32(0x51F334, 0x4B820);
    m.set32(0x51F400, 0x4B820);
}

} // namespace hotd::game
