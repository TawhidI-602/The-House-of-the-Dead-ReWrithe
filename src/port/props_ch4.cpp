// Chapter 4's prop sub-types (kind 128, table 0xAF960; worker "ch2proof" for Chapter 4). Read from the arcade
// disassembly (the decompilation as a guide); arcade forms only. Every routine proven on A (marked below).
#include "port/game.h"

namespace hotd::port {

// 0x3F460 prop_r4_mechanism (sub-types 97-101, the mechanism of Chapter 4's room 4): the "moving" flag 0x53E861
// cleared, then prop_r4_mechanism_run every frame
// Proven on A (ch4p_full: 1 captures, 0 differ)
template <class P> void Game<P>::prop_r4_mechanism(uint32_t obj)
{
    mem.at<uint8_t>(0x53E861) = 0;
    of<uint32_t>(obj, 0) = F::prop_r4_mechanism_run;
}

// 0x3F480 prop_r4_mechanism_run: ended by flag 113 (0x51E9B1); by sub-type: 97 turns (y +0x80) and, once moving,
// sinks 0.4 a frame down to y 54; 98 / 99 slide 0.25 a frame in z (to 2077.6 / down to 1977.6) once moving; 100 turns
// (y +0x50) from flag 112 (0x51E9B0) on and sets "moving" past 0x9FFF; 101 turns the other way until then; drawn
// Proven on A (ch4p_size: 150 captures, 0 differ)
template <class P> void Game<P>::prop_r4_mechanism_run(uint32_t obj)
{
    prop_schedule_update(obj);
    if (mem.at<uint8_t>(G::event_flags + 113) != 0) {
        task_exit();
        return;
    }
    const uint8_t moving = mem.at<uint8_t>(0x53E861);
    const uint8_t started = mem.at<uint8_t>(G::event_flags + 112);
    switch (of<int16_t>(obj, 0xCC)) {
    case 97:
        of<int32_t>(obj, 0x30) += 0x80;
        if (moving && of<float>(obj, 0x24) > ffrom(0x42580000u))
            of<float>(obj, 0x24) = of<float>(obj, 0x24) - ffrom(0x3ECCCCCDu);
        break;
    case 98:
        if (moving && of<float>(obj, 0x28) < ffrom(0x4501D99Au))
            of<float>(obj, 0x28) = of<float>(obj, 0x28) + 0.25f;
        break;
    case 99:
        if (moving && of<float>(obj, 0x28) > ffrom(0x44F73333u))
            of<float>(obj, 0x28) = of<float>(obj, 0x28) - 0.25f;
        break;
    case 100:
        if (!moving && started) {
            const int32_t a = of<int32_t>(obj, 0x30) + 0x50;
            of<int32_t>(obj, 0x30) = a;
            if (a > 0x9FFF)
                mem.at<uint8_t>(0x53E861) = 1;
        }
        break;
    case 101:
        if (!moving && started)
            of<int32_t>(obj, 0x30) -= 0x50;
        break;
    default:
        break;
    }
    prop_draw(obj);
}

// 0x3F600 prop090_drop (kind 154's pieces): gone below y -620; shot: impact, sound 0xA01366 and, when still, it
// starts falling (-0.015) with random spins (x / z, -10..10); while falling the speed grows by 0.015 a frame
// Proven on A (ch4p_full: 150 captures, 0 differ)
template <class P> void Game<P>::prop090_drop(uint32_t obj)
{
    prop_schedule_update(obj);
    if (of<float>(obj, 0x24) < -620.0f) {
        task_exit();
        return;
    }
    if (of<uint8_t>(obj, 0x58) & 6) {
        prop_bullet_impact(obj);
        sound_cue(0xA01366);
        if (of<float>(obj, 0x7C) == 0.0f) {
            of<uint32_t>(obj, 0x7C) = 0xBC75C28Fu;   // -0.015
            of<int32_t>(obj, 0x31C) = int32_t(rand()) % 21 - 10;
            of<int32_t>(obj, 0x320) = int32_t(rand()) % 21 - 10;
        }
    }
    float v = of<float>(obj, 0x7C);
    if (v != 0.0f) {
        v = v - ffrom(0x3C75C28Fu);
        of<int32_t>(obj, 0x2C) += of<int32_t>(obj, 0x31C);
        of<int32_t>(obj, 0x34) += of<int32_t>(obj, 0x320);
        of<float>(obj, 0x24) = of<float>(obj, 0x24) + v;
    }
    of<float>(obj, 0x7C) = v;
    prop_draw(obj);
    prop_shootable(obj);
}

// 0x3F800 prop104_kakaaa (sub-type 104): model frames up to 0x185C, then from 0x1859 again
// Proven on A (ch4p_full: 150 captures, 0 differ)
template <class P> void Game<P>::prop104_kakaaa(uint32_t obj)
{
    prop_schedule_update(obj);
    const int32_t m = of<int32_t>(obj, 0x54);
    of<int32_t>(obj, 0x54) = m > 0x185B ? 0x1859 : m + 1;
    prop_draw(obj);
}

// 0x3F840 prop108_pushukeke (sub-type 108): model frames up to 0x1115, then from 0x10E4 again
// Proven on A (ch4p_full: 54 captures, 0 differ)
template <class P> void Game<P>::prop108_pushukeke(uint32_t obj)
{
    prop_schedule_update(obj);
    const int32_t m = of<int32_t>(obj, 0x54);
    of<int32_t>(obj, 0x54) = m > 0x1114 ? 0x10E4 : m + 1;
    prop_draw(obj);
}

// 0x3F880 prop120_kami_region (sub-type 120): only in region 3 (else the task ends)
// Proven on A (ch4p_full: 1 captures, 0 differ)
template <class P> void Game<P>::prop120_kami_region(uint32_t obj)
{
    prop_schedule_update(obj);
    if (mem.at<uint8_t>(G::region) != 3) {
        task_exit();
        return;
    }
    prop_draw(obj);
}

} // namespace hotd::port
