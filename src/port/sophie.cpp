// Teaser L7: Chapter 1 event 19's scene objects: kind 16 Sophie (creature type 34) in her scripted scene, kind 78 (a
// timed sound, every 60 frames), kind 170 (the breaking glass). Read from the arcade disassembly (0x95EE0..0x963F0,
// 0x310B0, 0x3520); arcade forms only.
//
// Sophie's scene is a chain of per-frame routines, each waiting for an event flag (0x51E940 + n: 0x99 the scene,
// 0x9C struck (set by the Chariot's entrance), 0x9D / 0x9E lying, 0xFD the end) or a camera frame; every one draws her
// (with her shadow) and ends at once when the scene is skipped (evt_skip_end).
#include "port/game.h"

namespace hotd::port {

// the frame common to Sophie's routines: shadow, drawn, gone when the scene is skipped; `count`: her frame counter
template <class P> void Game<P>::sophie_draw(uint32_t obj, bool count)
{
    shadow_setup(mem.f32(G::floor_y));
    object_draw(obj);
    if (mem.u32(G::evt_skip_end) != 0) {
        task_exit();
        return;
    }
    if (count)
        of<uint32_t>(obj, 0xF8) += 1;
}

// 0x95EE0 sophie (kind 16): type 34, motion 0x184, at (243.4, -0.5, 562.2) facing 0xDDBF; waits for the scene
template <class P> void Game<P>::sophie(uint32_t obj)
{
    if (boss_on()) {   // PC 0x50E318: no Sophie in Boss Mode (frontend_state 4)
        task_exit();
        return;
    }
    of<int16_t>(obj, 0xCC) = 34;
    mem.set32(G::hit_type, 34);
    body_init(obj, 34, 0x184);
    creature_type_setup(obj);
    of<uint32_t>(obj, 0x20) = 0x437E676D;
    of<uint32_t>(obj, 0x24) = 0xBF000000;
    of<uint32_t>(obj, 0x28) = 0x440C8CCD;
    of<uint32_t>(obj, 0x30) = 0xDDBF;
    of<uint32_t>(obj, 0) = F::sophie_wait_scene;
    sophie_wait_scene(obj);
}

// 0x95F50 sophie_wait_scene: flag 0x99 -> +0x3C4 bit 7, sophie_stand_up
template <class P> void Game<P>::sophie_wait_scene(uint32_t obj)
{
    if (mem.at<uint8_t>(G::event_flags + 0x99) != 0) {
        of<uint8_t>(obj, 0x3C4) |= 0x80;
        of<uint32_t>(obj, 0) = F::sophie_stand_up;
    }
    sophie_draw(obj, true);
}

// 0x95FB0 sophie_stand_up: at camera frame 90 MO 0x18F (standing up, blend 10)
template <class P> void Game<P>::sophie_stand_up(uint32_t obj)
{
    if (mem.f32(G::camera_frame) == 90.0f) {
        set_motion_blend(obj, 0x18F, 0, 10);
        of<uint32_t>(obj, 0) = F::sophie_writhe;
    }
    sophie_draw(obj, true);
}

// 0x96020 sophie_writhe: at camera frame 220 MO 0x191 (writhing, blend 20), +0x3C4 bit 7 off
template <class P> void Game<P>::sophie_writhe(uint32_t obj)
{
    if (mem.f32(G::camera_frame) == 220.0f) {
        set_motion_blend(obj, 0x191, 0, 20);
        of<uint32_t>(obj, 0) = F::sophie_wait_struck;
        of<uint8_t>(obj, 0x3C4) &= 0x7F;
    }
    sophie_draw(obj, true);
}

// 0x960A0 sophie_wait_struck: flag 0x9C -> MO 0x182 (struck, blend 5), 60 frames
template <class P> void Game<P>::sophie_wait_struck(uint32_t obj)
{
    if (mem.at<uint8_t>(G::event_flags + 0x9C) != 0) {
        set_motion_blend(obj, 0x182, 0, 5);
        of<int32_t>(obj, 0x50) = 60;
        of<uint32_t>(obj, 0) = F::sophie_fall;
    }
    sophie_draw(obj, true);
}

// 0x96110 sophie_fall: after 60 frames moved to where she lies (208.65, -0, 579.97), MO 0x187
template <class P> void Game<P>::sophie_fall(uint32_t obj)
{
    const int32_t t = of<int32_t>(obj, 0x50) - 1;
    of<int32_t>(obj, 0x50) = t;
    if (t == 0) {
        of<uint32_t>(obj, 0x20) = 0x4350A694;
        of<uint32_t>(obj, 0x24) = 0x80000000;
        of<uint32_t>(obj, 0x28) = 0x4410FE04;
        set_motion(obj, 0x187);
        of<uint32_t>(obj, 0) = F::sophie_wait_lying;
    }
    sophie_draw(obj, true);
}

// 0x96190 sophie_wait_lying: flag 0x9D -> her counter to 98, sophie_lying_b
template <class P> void Game<P>::sophie_wait_lying(uint32_t obj)
{
    if (mem.at<uint8_t>(G::event_flags + 0x9D) != 0) {
        of<uint32_t>(obj, 0xF8) = 0x62;
        of<uint32_t>(obj, 0) = F::sophie_lying_b;
    }
    sophie_draw(obj, true);
}

// 0x961F0 sophie_lying_b: MO 0x188
template <class P> void Game<P>::sophie_lying_b(uint32_t obj)
{
    set_motion(obj, 0x188);
    of<uint32_t>(obj, 0) = F::sophie_wait_lying_b;
    sophie_draw(obj, false);
}

// 0x96230 sophie_wait_lying_b: flag 0x9E -> sophie_lying_c
template <class P> void Game<P>::sophie_wait_lying_b(uint32_t obj)
{
    if (mem.at<uint8_t>(G::event_flags + 0x9E) != 0)
        of<uint32_t>(obj, 0) = F::sophie_lying_c;
    sophie_draw(obj, false);
}

// 0x96280 sophie_lying_c: at the motion's end (+0x3C5 bit 4) MO 0x18A, a counter (+0x54) from 0
template <class P> void Game<P>::sophie_lying_c(uint32_t obj)
{
    if ((of<uint8_t>(obj, 0x3C5) >> 4) & 1) {
        set_motion(obj, 0x18A);
        of<uint32_t>(obj, 0x54) = 0;
        of<uint32_t>(obj, 0) = F::sophie_lying_d;
    }
    sophie_draw(obj, true);
}

// 0x962E0 sophie_lying_d: after 390 frames MO 0x18C
template <class P> void Game<P>::sophie_lying_d(uint32_t obj)
{
    const int32_t n = of<int32_t>(obj, 0x54) + 1;
    of<int32_t>(obj, 0x54) = n;
    if (n == 0x186) {
        set_motion(obj, 0x18C);
        of<uint32_t>(obj, 0) = F::sophie_wait_frame37;
    }
    sophie_draw(obj, true);
}

// 0x96350 sophie_wait_frame37: her counter at 37 -> sophie_wait_end
template <class P> void Game<P>::sophie_wait_frame37(uint32_t obj)
{
    if (of<uint32_t>(obj, 0xF8) == 37)
        of<uint32_t>(obj, 0) = F::sophie_wait_end;
    sophie_draw(obj, true);
}

// 0x963A0 sophie_wait_end: flag 0xFD -> her body freed, gone
template <class P> void Game<P>::sophie_wait_end(uint32_t obj)
{
    if (mem.at<uint8_t>(G::event_flags + 0xFD) != 0) {
        body_free_parts(obj);
        enemy_free_extras(obj);
        task_exit();
        return;
    }
    sophie_draw(obj, false);
}

// 0x310B0 kind78_delay (kind 78): waits record (+0x70) x 61 frames, sound 0xA01352 every 60 of them, then gone
template <class P> void Game<P>::kind78_delay(uint32_t obj)
{
    const int32_t s = of<int16_t>(obj, 0x60);
    if (s == 0) {
        of<int32_t>(obj, 0x50) = int32_t(of<int16_t>(obj, 0x70)) * 61;
        of<uint16_t>(obj, 0x60) = uint16_t(of<uint16_t>(obj, 0x60) + 1);
    } else if (s != 1) {
        return;
    }
    const int32_t t = of<int32_t>(obj, 0x50) - 1;
    of<int32_t>(obj, 0x50) = t;
    if (t % 60 == 0)
        sound_cue(0xA01352);
    if (of<int32_t>(obj, 0x50) < 0)
        task_exit();
}

// 0x3520 kind170_glass_break (kind 170): at (248.5, 1.064, 565.08) the glass models 0xD7B..0xDDD (sound 0xA01101),
// then gone
template <class P> void Game<P>::kind170_glass_break(uint32_t obj)
{
    const int32_t s = of<int16_t>(obj, 0x60);
    if (s == 0 || s == 1) {
        if (s == 0) {
            of<uint32_t>(obj, 0x20) = 0x43788000;
            of<uint32_t>(obj, 0x24) = 0x3F8837B5;
            of<uint32_t>(obj, 0x28) = 0x440D4550;
            of<int32_t>(obj, 0x54) = 0xD7B;
            of<uint16_t>(obj, 0x60) = uint16_t(of<uint16_t>(obj, 0x60) + 1);
            sound_cue(0xA01101);
        }
        const int32_t m = of<int32_t>(obj, 0x54) + 1;
        of<int32_t>(obj, 0x54) = m;
        if (m == 0xDDE) {
            task_exit();
            return;
        }
    }
    cam_copro(0x05, {});
    cam_copro(0x12, {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)});
    geo_draw_model(of<uint32_t>(obj, 0x54));
    cam_copro(0x06, {});
}

} // namespace hotd::port
