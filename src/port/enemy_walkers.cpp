// Teaser L3 kinds: 96, the zombie walkers (the first zombies of the game: Chapter 1 events 1, 3, 4, 6, 9; they walk a
// path and fall when shot), 98 (enemy_guard_b: enemy_guard with a life timer from the record) and 160 (a water
// splash). Read from the arcade disassembly (R1 had kind 96 in the runtime); arcade forms only. Kind 96's frames
// 0x7EC60 / 0x7EC30 follow kind096_zombie_walker's entry (unanalysed in Ghidra, not in names.tsv: walk_frame /
// fall_frame here).
#include "port/game.h"

namespace hotd::port {

// 0x7EDE0 kind096_zombie_walker (kind 96): one of four walk motions at random (0x284, 0x286-0x288), the type set-up,
// the walk frame; flags2 bit 1 off (no shadow)
template <class P> void Game<P>::kind096_zombie_walker(uint32_t obj)
{
    mem.at<int32_t>(G::hit_type) = of<int16_t>(obj, 0xCC);
    of<int16_t>(obj, 0xCE) = 0;
    const int32_t r = int32_t(rand());
    static constexpr uint32_t motions[4] = {0x284, 0x286, 0x287, 0x288};
    const int32_t m = r % 4;
    if (m >= 0 && m < 4)
        body_init(obj, mem.at<int32_t>(G::hit_type), motions[m]);
    creature_type_setup(obj);
    of<uint32_t>(obj, 0) = 0x7EC60;
    of<uint16_t>(obj, 0x6C) &= uint16_t(~2);
}

// 0x7EC60 (kind 96's walk frame): the part hits; shot: 80 points to the shooter (flags bit 1: player 1, else player 2),
// a fall motion by the angle to the camera (0x237 from 0xA000..0xDFFF, 0x236 from 0x2000..0x5FFF, else 0x233), the
// fall frame; else the life timer (+0x320, counted down while camera_frame is 1.0; at 0 the walker leaves once
// camera_frame reaches its +0x70), action 1 turns 0x20 a frame (by the type's parity), drawn, shootable
template <class P> void Game<P>::kind096_walk_frame(uint32_t obj)
{
    const int32_t type = of<int16_t>(obj, 0xCC);
    mem.at<uint8_t>(G::hit_class) = 0;
    mem.at<int16_t>(G::hit_damage) = 0;
    mem.at<int32_t>(G::hit_type) = type;
    enemy_part_hits(obj);
    if ((of<uint16_t>(obj, 0x58) >> 8) & 1) {
        add_score((of<uint8_t>(obj, 0x58) >> 1) & 1 ? 0 : 1, 0x50);
        const int32_t a = int32_t(angle_to_camera(obj));
        uint32_t m;
        if (a > 0x9FFF && a <= 0xDFFF)
            m = 0x237;
        else if (a > 0x1FFF && a <= 0x5FFF)
            m = 0x236;
        else
            m = 0x233;
        set_motion_blend(obj, m, 0, 6);
        const uint32_t f = of<uint32_t>(obj, 0x58) & 0xFFFFFEFEu;
        of<uint32_t>(obj, 0) = 0x7EC30;
        of<uint32_t>(obj, 0x58) = f;
        return;
    }
    const int32_t t = of<int32_t>(obj, 0x320);
    const float frame = mem.f32(G::camera_frame);
    if (t > 0) {
        if (frame == 1.0f)
            of<int32_t>(obj, 0x320) = t - 1;
    } else if (!(float(of<int16_t>(obj, 0x70)) > frame)) {
        body_free_parts(obj);
        task_exit();
        return;
    }
    if (of<int16_t>(obj, 0x64) == 1) {
        const int32_t ty = of<int16_t>(obj, 0xCC);
        of<int32_t>(obj, 0x30) += ((ty % 2) & 0xFFFF) != 0 ? 0x20 : -0x20;
    }
    object_draw(obj);
    of<uint32_t>(obj, 0xF8) += 1;
    depth_list_add(obj);
}

// 0x7EC30 (kind 96's fall frame): drawn until the fall motion ends (MotFinish), then gone
template <class P> void Game<P>::kind096_fall_frame(uint32_t obj)
{
    object_draw(obj);
    of<uint32_t>(obj, 0xF8) += 1;
    if ((of<uint8_t>(obj, 0x3C5) >> 4) & 1) {
        body_free_parts(obj);
        task_exit();
    }
}

// 0x55B00 enemy_guard_b (kind 98): type, action and the life timer (+0x320) from the record, then enemy_guard
template <class P> void Game<P>::enemy_guard_b(uint32_t obj)
{
    const uint32_t r = of<uint32_t>(obj, 0x35C);
    of<uint32_t>(obj, 0x368) = r;
    of<int16_t>(obj, 0xCC) = int16_t(mem.at<uint8_t>(r));
    of<int16_t>(obj, 0x64) = int16_t(mem.at<uint8_t>(r + 1));
    of<uint32_t>(obj, 0x320) = mem.at<uint16_t>(r + 2);
    enemy_guard(obj);
}

// 0x3970 kind160_splash (kind 160): model 0x415, the animation as its routine and its first frame, the splash sound
template <class P> void Game<P>::kind160_splash(uint32_t obj)
{
    of<int32_t>(obj, 0x54) = 0x415;
    of<uint32_t>(obj, 0) = 0x39A0;
    kind160_splash_anim(obj);
    sound_cue(0xA0110C);
}

// 0x39A0 kind160_splash_anim: the next model each frame until 0x433 (gone); drawn at its position, turned by its y
// angle, scaled 2.5
template <class P> void Game<P>::kind160_splash_anim(uint32_t obj)
{
    const int32_t model = of<int32_t>(obj, 0x54) + 1;
    of<int32_t>(obj, 0x54) = model;
    if (model == 0x433) {
        task_exit();
        return;
    }
    Tgp::Out none;
    copro(0x05, nullptr, 0, none);
    uint32_t t[3] = {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)};
    copro(0x12, t, 3, none);
    uint32_t a[1] = {of<uint32_t>(obj, 0x30)};
    copro(0x15, a, 1, none);
    uint32_t s[3] = {0x40200000, 0x40200000, 0x40200000};
    copro(0x13, s, 3, none);
    geo_draw_model(of<uint32_t>(obj, 0x54));
    copro(0x06, nullptr, 0, none);
}

} // namespace hotd::port
