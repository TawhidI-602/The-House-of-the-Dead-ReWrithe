// Enemies: kind 32, the zombies with a kit (arcade cluster 0x7EEB0..0x80600; the iron-ball kit and the held objects
// are not ported yet), read from the typed decompilation and the arcade disassembly. The spawn record (+0x35C, kept
// in +0x368) picks the kit (record byte 0, the action): its start motion, start routine and after-hit routine come
// from kind32_kits (0xC9760, 12 bytes): 2 the axe thrower (MO_z_a_kamae2a), 3 the drum carrier (MO_z_a_dolamoti),
// 4 the iron-ball swinger. Every per-frame routine (the object's +0 routine) ends in kind32_hits. Record: +1 a
// variant / event index, +2 the kit table row, +3 bits 1-3 the axe thrower's entrance, bit 4 +0x6E 2; +6 / +8 / +10
// delays; +0xC bits 0-1 the carry style, 6-8 what follows the throw, 10-12 the hiding pose; +0x38 / +0x3C the walk-in
// line and speed. Words: +0x2EC a countdown, +0x320 bits 0-2 the held object gone / broken, bit 4 released,
// +0x324 the hand / pose variant, +0x328 the throw row, +0x32C the kit table row, +0x338 bit 0 a fall death,
// +0x37C the spline buffer (kind32_act2_stance's mem_alloc).
#include "port/game.h"

namespace hotd::port {

// a per-frame routine of kind 32 (the object's +0 routine, set and called at once): ported ones directly
template <class P> void Game<P>::kind32_call(uint32_t routine, uint32_t obj)
{
    switch (routine) {
    case F::kind32_act2_stance: kind32_act2_stance(obj); return;
    case F::kind32_act2_after_hit: kind32_act2_after_hit(obj); return;
    case F::kind32_act3_drum: kind32_act3_drum(obj); return;
    case F::kind32_act3_after_hit: kind32_act3_after_hit(obj); return;
    case F::kind32_act2_rise_start: kind32_act2_rise_start(obj); return;
    case F::kind32_act3_recover: kind32_act3_recover(obj); return;
    case F::kind32_act4_iron_ball: kind32_act4_iron_ball(obj); return;
    case F::kind32_act4_after_hit: kind32_act4_after_hit(obj); return;
    case F::kind32_act4_recover: kind32_act4_recover(obj); return;
    default: call_routine(routine, obj); return;
    }
}

// 0x7EEB0 (kind 32): the record from the owner; the kit's body and start motion, type set-up (chapter 1 event 9
// scene 3: the body's parts not tested one by one), never far from the floor (+0x40 32768), FixMode; counted, a wide
// attack slot, a target player; the kit's start routine
template <class P> void Game<P>::kind32_zombie_kit(uint32_t obj)
{
    const uint32_t r = of<uint32_t>(obj, 0x35C);
    of<uint32_t>(obj, 0x368) = r;
    of<int16_t>(obj, 0x64) = int16_t(mem.at<int8_t>(r));
    const int32_t action = of<int16_t>(obj, 0x64);
    of<int16_t>(obj, 0x60) = 0;
    const int32_t type = mem.at<int16_t>(r + 4);
    const int32_t motion = mem.at<int16_t>(G::kind32_kits + uint32_t(action) * 12);
    mem.set32(G::hit_type, uint32_t(type));
    body_init(obj, type, uint32_t(motion));
    creature_type_setup(obj);
    if (mem.at<uint8_t>(G::chapter) == 0 && mem.at<uint8_t>(G::event_index) == 9 && mem.at<uint8_t>(G::scene_number) == 3) {
        const uint32_t b = mem.u32(G::body_current);
        of<uint16_t>(b, 0x6C) = of<uint16_t>(b, 0x6C) & 0xFFFD;
    }
    of<uint32_t>(obj, 0x40) = 0x47000000u;
    of<uint8_t>(obj, 0x3C6) = of<uint8_t>(obj, 0x3C6) | 1;
    if ((mem.at<uint8_t>(r + 3) >> 4) & 1)
        of<int16_t>(obj, 0x6E) = 2;
    mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) + 1);
    mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) + 1);
    attack_slot_take_wide(obj);
    pick_target_player_rand(obj);
    of<int32_t>(obj, 0x338) = 0;
    of<int32_t>(obj, 0x308) = 0;
    const uint32_t start = mem.u32(G::kind32_kits + uint32_t(of<int16_t>(obj, 0x64)) * 12 + 4);
    of<uint32_t>(obj, 0x0) = start;
    kind32_call(start, obj);
}

// 0x7EFC0 (kit 2 start): +0x2EC the record's +6; the spline buffer; the entrance by record +3 bits 1-3: 1 walks in,
// 2 hides, 4 rises, 0 ready at once, others ready with +0x6E 1
template <class P> void Game<P>::kind32_act2_stance(uint32_t obj)
{
    const uint32_t r = of<uint32_t>(obj, 0x368);
    if constexpr (P::pc)
        mem.set32(G::pc_wide_slot_kind, 6);   // PC only
    of<int32_t>(obj, 0x320) = 0;
    of<int32_t>(obj, 0x2EC) = mem.at<int16_t>(r + 6);
    of<uint32_t>(obj, 0x37C) = mem_alloc(0x48);
    const uint32_t v = (uint32_t(int32_t(mem.at<int8_t>(r + 3))) >> 1) & 7;
    if (v == 1) {
        kind32_act2_walk_in(obj);
        return;
    }
    if (v == 2) {
        kind32_act2_hidden(obj);
        return;
    }
    if (v == 4) {
        kind32_act2_rise(obj);
        return;
    }
    if (v != 0)
        of<int16_t>(obj, 0x6E) = 1;
    kind32_act2_ready(obj);
}

// 0x7F040 (entrance 4): rises in MO_z_a_kamae2a for the rise table's frames (kind32_rise_steps by record +2), not
// shootable meanwhile
template <class P> void Game<P>::kind32_act2_rise(uint32_t obj)
{
    const int32_t row = mem.at<int8_t>(of<uint32_t>(obj, 0x368) + 2);
    set_motion(obj, 0x1F1);
    of<uint8_t>(obj, 0x3C6) = of<uint8_t>(obj, 0x3C6) | 1;
    const float frames = mem.f32(G::kind32_rise_steps + 4 + uint32_t(row) * 0x18);
    of<uint32_t>(obj, 0x0) = F::kind32_act2_rise_frame;
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xFFFFFEF8u;
    of<int32_t>(obj, 0x50) = int32_t(frames);
    kind32_act2_rise_frame(obj);
}

// 0x7F0A0: y + the step each frame; at 0 shootable and the throw wait
template <class P> void Game<P>::kind32_act2_rise_frame(uint32_t obj)
{
    const float y = of<float>(obj, 0x24);
    const float step = mem.f32(G::kind32_rise_steps + uint32_t(int32_t(mem.at<int8_t>(of<uint32_t>(obj, 0x368) + 2))) * 0x18);
    const int32_t t = of<int32_t>(obj, 0x50) - 1;
    of<int32_t>(obj, 0x50) = t;
    of<float>(obj, 0x24) = step + y;
    if (t == 0) {
        of<uint32_t>(obj, 0x0) = F::kind32_throw_wait;
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | 1;
        kind32_throw_wait(obj);
        return;
    }
    kind32_hits(obj);
}

// 0x7F100 (entrance 2): a random hand (+0x324), the hiding pose; anim_time 30 - record +6; waits for combat
template <class P> void Game<P>::kind32_act2_hidden(uint32_t obj)
{
    const uint32_t r = of<uint32_t>(obj, 0x368);
    of<int32_t>(obj, 0x324) = int32_t(rand()) % 2;
    kind32_hide_pose(obj, 0);
    const int32_t d = mem.at<int16_t>(r + 6);
    of<uint32_t>(obj, 0x0) = F::kind32_act2_hidden_frame;
    of<int32_t>(obj, 0xF8) = 0x1E - d;
    of<uint8_t>(obj, 0x3C6) = of<uint8_t>(obj, 0x3C6) | 1;
    kind32_act2_hidden_frame(obj);
}

// 0x7F170: once combat starts the throw, else the pose held
template <class P> void Game<P>::kind32_act2_hidden_frame(uint32_t obj)
{
    if (combat_active() != 0) {
        kind32_axe_throw(obj);
        return;
    }
    of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
    kind32_hits(obj);
}

// 0x7F1A0 (entrance 1): its start point (the record's line +0x38 behind it, through its position and heading:
// coprocessor 0x10 / 0x88 / 0x15 / 0x1A) and step (+0x3C forward); MO_z_a_walk6a
template <class P> void Game<P>::kind32_act2_walk_in(uint32_t obj)
{
    const uint32_t r = of<uint32_t>(obj, 0x368);
    if (pc_path()) {   // the PC, and PC Mode on the arcade logic (docs/pcmode.md row 10)
        // PC: the software matrix stack: the identity with its position as the translation, turned by its heading
        // (the file's copy of pc_matrix_rotate_y); the line behind it, then the step from a fresh identity
        pc_matrix_push();
        pcm_identity();
        pcm_set(9, of<float>(obj, 0x20));
        pcm_set(10, of<float>(obj, 0x24));
        pcm_set(11, of<float>(obj, 0x28));
        pc_matrix_rotate_y7(of<uint32_t>(obj, 0x30));
        float p[3];
        pc_matrix_transform(0.0f, 0.0f, -mem.f32(r + 0x38), p[0], p[1], p[2]);
        of<float>(obj, 0x20) = p[0];
        of<float>(obj, 0x24) = p[1];
        of<float>(obj, 0x28) = p[2];
        pcm_identity();
        pc_matrix_rotate_y7(of<uint32_t>(obj, 0x30));
        pc_matrix_transform(0.0f, 0.0f, mem.f32(r + 0x3C), p[0], p[1], p[2]);
        of<float>(obj, 0x78) = p[0];
        of<float>(obj, 0x7C) = p[1];
        of<float>(obj, 0x80) = p[2];
        of<uint32_t>(obj, 0x33C) = of<uint32_t>(obj, 0x20);
        pcm_pop();
        of<uint32_t>(obj, 0x340) = of<uint32_t>(obj, 0x28);
        set_motion(obj, 0x206);
        of<uint8_t>(obj, 0x3C6) = of<uint8_t>(obj, 0x3C6) & 0xFE;
        of<uint32_t>(obj, 0x0) = F::kind32_act2_walk_in_frame;
        kind32_act2_walk_in_frame(obj);
        return;
    }
    Tgp::Out none, o, o2;
    copro(0x05, nullptr, 0, none);
    copro(0x10, nullptr, 0, none);
    const uint32_t t[3] = {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)};
    copro(0x88, t, 3, none);
    const uint32_t ry[1] = {of<uint32_t>(obj, 0x30)};
    copro(0x15, ry, 1, none);
    const uint32_t v[3] = {0, 0, mem.u32(r + 0x38) ^ 0x80000000u};
    copro(0x1A, v, 3, o);
    of<uint32_t>(obj, 0x20) = o.w[0];
    of<uint32_t>(obj, 0x24) = o.w[1];
    const uint32_t z = o.w[2];
    copro(0x10, nullptr, 0, none);
    const uint32_t ry2[1] = {of<uint32_t>(obj, 0x30)};
    of<uint32_t>(obj, 0x28) = z;
    copro(0x15, ry2, 1, none);
    const uint32_t s[3] = {0, 0, mem.u32(r + 0x3C)};
    copro(0x1A, s, 3, o2);
    of<uint32_t>(obj, 0x78) = o2.w[0];
    of<uint32_t>(obj, 0x7C) = o2.w[1];
    copro(0x06, nullptr, 0, none);
    of<uint32_t>(obj, 0x33C) = of<uint32_t>(obj, 0x20);
    of<uint32_t>(obj, 0x340) = of<uint32_t>(obj, 0x28);
    of<uint32_t>(obj, 0x80) = o2.w[2];
    set_motion(obj, 0x206);
    of<uint32_t>(obj, 0x0) = F::kind32_act2_walk_in_frame;
    of<uint8_t>(obj, 0x3C6) = of<uint8_t>(obj, 0x3C6) & 0xFE;
    kind32_act2_walk_in_frame(obj);
}

// 0x7F310: x / z + the step; past the line (floor distance from the start, coprocessor 0x1F) at a motion end ready
template <class P> void Game<P>::kind32_act2_walk_in_frame(uint32_t obj)
{
    const float z = of<float>(obj, 0x28) + of<float>(obj, 0x80);
    const float x = of<float>(obj, 0x20) + of<float>(obj, 0x78);
    of<float>(obj, 0x20) = x;
    of<float>(obj, 0x28) = z;
    if (pc_path()) {
        // PC: the distance by the file's table square root
        const float dx = x - of<float>(obj, 0x33C), dz = z - of<float>(obj, 0x340);
        if (pc_fast_sqrt9(dx * dx + dz * dz) > mem.f32(of<uint32_t>(obj, 0x368) + 0x38) &&
            ((of<uint8_t>(obj, 0x3C5) >> 4) & 1)) {
            kind32_act2_ready(obj);
            return;
        }
        kind32_hits(obj);
        return;
    }
    const uint32_t in[6] = {of<uint32_t>(obj, 0x33C), 0, of<uint32_t>(obj, 0x340), fbits(x), 0, fbits(z)};
    Tgp::Out o;
    copro(0x1F, in, 6, o);
    if (ffrom(o.w[0]) > mem.f32(of<uint32_t>(obj, 0x368) + 0x38) && ((of<uint8_t>(obj, 0x3C5) >> 4) & 1)) {
        kind32_act2_ready(obj);
        return;
    }
    kind32_hits(obj);
}

// 0x7F3B0: MO_z_a_kamae2a (blended unless no motion yet), FixMode, the throw wait
template <class P> void Game<P>::kind32_act2_ready(uint32_t obj)
{
    if (of<int16_t>(obj, 0xD0) == 0)
        set_motion_blend(obj, 0x1F1, 0, 4);
    else
        set_motion(obj, 0x1F1);
    of<uint32_t>(obj, 0x0) = F::kind32_throw_wait;
    of<uint8_t>(obj, 0x3C6) = of<uint8_t>(obj, 0x3C6) | 1;
    kind32_throw_wait(obj);
}

// what follows a throw (record +0xC bits 6-8): 0 deactivate, 1 the stance, 2 walk off, 3 leave, others the walk
template <class P> void Game<P>::kind32_after_throw(uint32_t obj)
{
    const uint32_t k = (uint32_t(int32_t(mem.at<int16_t>(of<uint32_t>(obj, 0x368) + 0xC))) >> 6) & 7;
    switch (k) {
    case 0: kind32_deactivate(obj); return;
    case 1: kind32_after_throw_stance(obj); return;
    case 2: kind32_walk_off_call(obj); return;
    case 3: kind32_leave(obj); return;
    default: kind32_after_throw_walk(obj); return;
    }
}

// the hands: bit 0 Cyril's bare right hand at node 5 (0x5DE), bit 1 the left at node 8 (0x5DD)
template <class P> uint32_t Game<P>::kind32_bare_hands(uint32_t obj)
{
    uint32_t h = body_node_model(obj, 5) == 0x5DE ? 1 : 0;
    if (body_node_model(obj, 8) == 0x5DD)
        h |= 2;
    return h;
}

// 0x7F400: in combat +0x2EC counts down (not below 0); at 0: both hands empty -> what follows the throw; else the
// throwing hand (+0x324: the free one; both: by record +1 5 the right, else at random), the hiding pose blended and
// the throw
template <class P> void Game<P>::kind32_throw_wait(uint32_t obj)
{
    const uint32_t r = of<uint32_t>(obj, 0x368);
    int go = 0;
    if (combat_active() != 0) {
        int32_t c = of<int32_t>(obj, 0x2EC) - 1;
        if (!(0 <= c))
            c = 0;
        of<int32_t>(obj, 0x2EC) = c;
        const uint32_t h = kind32_bare_hands(obj);
        if (of<int32_t>(obj, 0x2EC) == 0) {
            if (h == 0) {
                go = 2;
            } else {
                if (h == 1)
                    of<int32_t>(obj, 0x324) = 0;
                else if (h == 2)
                    of<int32_t>(obj, 0x324) = 1;
                else if (mem.at<int8_t>(r + 1) == 5)
                    of<int32_t>(obj, 0x324) = 0;
                else
                    of<int32_t>(obj, 0x324) = int32_t(rand()) % 2;
                kind32_hide_pose(obj, 1);
                go = 1;
            }
        }
    }
    if (go == 1) {
        kind32_throw_ready_start(obj);
        return;
    }
    if (go == 2) {
        kind32_after_throw(obj);
        return;
    }
    kind32_hits(obj);
}

// 0x7F560
template <class P> void Game<P>::kind32_throw_ready_start(uint32_t obj)
{
    of<uint32_t>(obj, 0x0) = F::kind32_throw_ready;
    kind32_throw_ready(obj);
}

// 0x7F580: the throw at the motion's throw flag (+0x3C5 bit 6)
template <class P> void Game<P>::kind32_throw_ready(uint32_t obj)
{
    if ((of<uint8_t>(obj, 0x3C5) >> 6) & 1) {
        kind32_axe_throw(obj);
        return;
    }
    kind32_hits(obj);
}

// 0x7F5A0: the throw's row (+0x328), then kind32_axe_throw_frame
template <class P> void Game<P>::kind32_axe_throw(uint32_t obj)
{
    kind32_axe_throw_row(obj, obj + ob(0x328));
    of<uint32_t>(obj, 0x0) = F::kind32_axe_throw_frame;
    kind32_axe_throw_frame(obj);
}

// 0x7F5D0: at the row's release frame kind32_axe_release; at the motion's end: a bare hand left -> the stance again
// (+0x320 + 1, +0x2EC the record's +8), else what follows the throw
template <class P> void Game<P>::kind32_axe_throw_frame(uint32_t obj)
{
    const uint32_t r = of<uint32_t>(obj, 0x368);
    const int32_t rel = int32_t(mem.u32(G::kind32_throw_rows + of<uint32_t>(obj, 0x328) * 0x18));
    if (rel == int32_t(of<int16_t>(obj, 0xF4)))
        kind32_axe_release(obj, of<uint32_t>(obj, 0x328));
    if (((of<uint8_t>(obj, 0x3C5) >> 4) & 1) == 0) {
        kind32_hits(obj);
        return;
    }
    if (kind32_bare_hands(obj) == 0) {
        kind32_after_throw(obj);
        return;
    }
    const int32_t n = of<int32_t>(obj, 0x320);
    const int32_t d = mem.at<int16_t>(r + 8);
    of<int16_t>(obj, 0xD0) = 0;
    of<int32_t>(obj, 0x320) = n + 1;
    of<int32_t>(obj, 0x2EC) = d;
    set_motion_blend(obj, 0x1F1, 0, 4);
    of<uint32_t>(obj, 0x0) = F::kind32_throw_wait;
    of<uint8_t>(obj, 0x3C6) = of<uint8_t>(obj, 0x3C6) | 1;
    kind32_throw_wait(obj);
}

// 0x7F700
template <class P> void Game<P>::kind32_walk_off_call(uint32_t obj)
{
    kind32_walk_off(obj);
}

// 0x7F710: no longer counted (+0x3C4 bit 3 off), its attack slot freed, walks off
template <class P> void Game<P>::kind32_leave(uint32_t obj)
{
    of<uint8_t>(obj, 0x3C4) = of<uint8_t>(obj, 0x3C4) & 0xF7;
    mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) - 1);
    mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) - 1);
    if (of<int8_t>(obj, 0x2E7) != -1)
        enemy_attack_slot_free(obj);
    kind32_walk_off(obj);
}

// 0x7F770: not hit-tested (flags bits 1 / 2 / 8 off), bit 31 on; MO_z_a_walk6a; a spline from its position through
// the record's points (+0x14..0x28) (spline_segment_init on +0x37C), speed and step from kind32_walk_off_spline;
// its first heading
template <class P> void Game<P>::kind32_walk_off(uint32_t obj)
{
    LocalScope ls(*this);
    const uint32_t r = of<uint32_t>(obj, 0x368);
    of<uint32_t>(obj, 0x58) = (of<uint32_t>(obj, 0x58) & 0xFFFFFEF9u) | 0x80000000u;
    set_motion(obj, 0x206);
    const uint32_t pts = local_alloc(0x30);   // fp + 0x40: x, y, z, rec +0x14, +0x18 .. +0x28
    const uint32_t x = of<uint32_t>(obj, 0x20), y = of<uint32_t>(obj, 0x24), z = of<uint32_t>(obj, 0x28);
    of<uint8_t>(obj, 0x3C6) = of<uint8_t>(obj, 0x3C6) & 0xFE;
    mem.set32(pts + 0x00, x);
    mem.set32(pts + 0x04, y);
    mem.set32(pts + 0x08, z);
    mem.set32(pts + 0x0C, mem.u32(r + 0x14));
    mem.set32(pts + 0x10, mem.u32(r + 0x18));
    mem.set32(pts + 0x14, mem.u32(r + 0x1C));
    mem.set32(pts + 0x18, mem.u32(r + 0x20));
    mem.set32(pts + 0x1C, mem.u32(r + 0x24));
    mem.set32(pts + 0x20, mem.u32(r + 0x28));
    spline_segment_init(of<uint32_t>(obj, 0x37C), pts);
    of<uint32_t>(obj, 0x33C) = 0;
    of<uint32_t>(obj, 0x74) = mem.u32(G::kind32_walk_off_spline);
    const uint32_t out = local_alloc(8);   // fp + 0x70 / 0x74
    spline_advance(obj, of<uint32_t>(obj, 0x37C), mem.u32(G::kind32_walk_off_spline + 4),
                   mem.u32(G::kind32_walk_off_spline + 8), out, out + 4);
    relocate(4, out, 4);
    relocate(5, out + 4, 4);
    of<uint32_t>(obj, 0x0) = F::kind32_walk_off_frame;
    of<uint32_t>(obj, 0x2C) = mem.u32(out);
    of<uint32_t>(obj, 0x30) = mem.u32(out + 4);
    kind32_walk_off_frame(obj);
}

// 0x7F840: along the spline (the angles into +0xBC / +0x2C and +0xC0 / +0x30), moved along the heading
// (move_routines[0]); past its end (+0x33C > 1): counted ones uncounted, the extras and body freed, ends
template <class P> void Game<P>::kind32_walk_off_frame(uint32_t obj)
{
    {
        LocalScope ls(*this);
        const uint32_t out = local_alloc(8);   // fp + 0x40 / 0x44
        spline_advance(obj, of<uint32_t>(obj, 0x37C), mem.u32(G::kind32_walk_off_spline + 4),
                       mem.u32(G::kind32_walk_off_spline + 8), out, out + 4);
        relocate(4, out, 4);
        relocate(5, out + 4, 4);
        of<uint32_t>(obj, 0xBC) = mem.u32(out);
        of<uint32_t>(obj, 0x2C) = mem.u32(out);
        of<uint32_t>(obj, 0xC0) = mem.u32(out + 4);
        of<uint32_t>(obj, 0x30) = mem.u32(out + 4);
    }
    call_routine(mem.u32(G::move_routines), obj);
    // (the PC compares the float's bits as a signed integer)
    if (pc_path() ? of<int32_t>(obj, 0x33C) > 0x3F800000 : of<float>(obj, 0x33C) > 1.0f) {
        if ((of<uint8_t>(obj, 0x3C4) >> 3) & 1) {
            mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) - 1);
            mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) - 1);
            if (of<uint8_t>(obj, 0x2E7) != 0xFF)
                enemy_attack_slot_free(obj);
        }
        enemy_free_extras(obj);
        body_free_parts(obj);
        task_exit();
        return;
    }
    kind32_hits(obj);
}

// 0x7F900: MO_z_a_walk6a, a jump up (vy 2, gravity 0.1), FixMode, flags bit 31, a fall death (+0x338 bit 0)
template <class P> void Game<P>::kind32_after_throw_walk(uint32_t obj)
{
    set_motion(obj, 0x206);
    of<uint32_t>(obj, 0x88) = 0xBDCCCCCDu;   // -0.1
    of<float>(obj, 0x7C) = 2.0f;
    of<uint32_t>(obj, 0x0) = F::kind32_after_throw_walk_n;
    of<float>(obj, 0x84) = 0.0f;
    of<float>(obj, 0x8C) = 0.0f;
    of<float>(obj, 0x78) = 0.0f;
    of<float>(obj, 0x80) = 0.0f;
    of<int16_t>(obj, 0x60) = 0;
    of<uint8_t>(obj, 0x3C6) = of<uint8_t>(obj, 0x3C6) | 1;
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | 0x80000000u;
    of<uint32_t>(obj, 0x338) = of<uint32_t>(obj, 0x338) | 1;
    kind32_after_throw_walk_n(obj);
}

// 0x7F970: moved by its velocity (move_routines[1]); below the floor: put back where it was, deactivated
template <class P> void Game<P>::kind32_after_throw_walk_n(uint32_t obj)
{
    uint32_t saved[6];
    for (uint32_t i = 0; i < 6; ++i)
        saved[i] = of<uint32_t>(obj, 0x20 + i * 4);
    call_routine(mem.u32(G::move_routines + 4), obj);
    if (of<float>(obj, 0x24) < mem.f32(G::floor_y)) {
        for (uint32_t i = 0; i < 6; ++i)
            of<uint32_t>(obj, 0x20 + i * 4) = saved[i];
        obj_velocity_zero(obj);
        return;
    }
    kind32_hits(obj);
}

// 0x7F9D0: velocity and acceleration cleared, deactivated
template <class P> void Game<P>::obj_velocity_zero(uint32_t obj)
{
    of<float>(obj, 0x84) = 0.0f;
    of<float>(obj, 0x88) = 0.0f;
    of<float>(obj, 0x8C) = 0.0f;
    of<float>(obj, 0x78) = 0.0f;
    of<float>(obj, 0x7C) = 0.0f;
    of<float>(obj, 0x80) = 0.0f;
    kind32_deactivate(obj);
}

// 0x7F9F0 (kit 2 after a hit)
template <class P> void Game<P>::kind32_act2_after_hit(uint32_t obj)
{
    of<uint32_t>(obj, 0x0) = F::kind32_act2_hit_wait;
    kind32_act2_hit_wait(obj);
}

// 0x7FA10
template <class P> void Game<P>::kind32_act2_hit_wait(uint32_t obj)
{
    if (kind32_hit_reaction_done(obj, F::kind32_act2_rise_start) != 0)
        return;
    kind32_hits(obj);
}

// 0x7FA40: the hiding pose (kind32_hide_motions_r / _l by +0x324, column record +0xC bits 10-12), blended or not
template <class P> void Game<P>::kind32_hide_pose(uint32_t obj, int blend)
{
    const uint32_t k = (uint32_t(int32_t(mem.at<int16_t>(of<uint32_t>(obj, 0x368) + 0xC))) >> 10) & 7;
    const uint32_t t = of<int32_t>(obj, 0x324) == 0 ? G::kind32_hide_motions_r : G::kind32_hide_motions_l;
    if (blend == 0)
        set_motion(obj, uint32_t(int32_t(mem.at<int16_t>(t + k * 2))));
    else
        set_motion_blend(obj, uint32_t(int32_t(mem.at<int16_t>(t + k * 2))), 0, 4);
}

// 0x7FAE0 (object, out): the throw row of the current motion; any other motion: the developers' error text, then
// the endless loop
template <class P> void Game<P>::kind32_axe_throw_row(uint32_t obj, uint32_t out)
{
    switch (of<int16_t>(obj, 0xD0)) {
    case 0x1FB: mem.set32(out, 0); return;
    case 0x263: mem.set32(out, 1); return;
    case 0x262: mem.set32(out, 2); return;
    case 0x1FC: mem.set32(out, 3); return;
    case 0x25F: mem.set32(out, 4); return;
    case 0x25E: mem.set32(out, 5); return;
    default: break;
    }
    if constexpr (P::pc) {
        pc_debug_print();   // PC: the message only (cdecl), then on with the row unchanged
    } else {
        text_cursor(0, 0x28);
        text_printf(G::kind32_unexpected_motion_text);
        debug_halt();
    }
}

// 0x7FBA0 (kit 3 start): the carry motion by the kit row (record +2; record +0xC bits 0-1 set: MO_z_a_dolamoti with
// it as the second motion); record +3 bits 1-3 = 3: FixMode and +0x6E 1; +0x320 / +0x324 0, +0x32C the row; its
// held object (held_object_start: the row, scale, record and action); the carry wait
template <class P> void Game<P>::kind32_act3_drum(uint32_t obj)
{
    const uint32_t r = of<uint32_t>(obj, 0x368);
    const int32_t row = mem.at<int8_t>(r + 2);
    const uint32_t carry = uint32_t(int32_t(mem.at<int16_t>(G::kind32_carry_kits + uint32_t(row) * 0x58)));
    if ((mem.at<uint16_t>(r + 0xC) & 3) == 0) {
        set_motion(obj, carry);
    } else {
        set_motion(obj, 0x1E7);
        set_motion2(obj, carry);
    }
    uint8_t m = of<uint8_t>(obj, 0x3C6);
    if ((mem.at<uint8_t>(r + 3) & 0xE) == 6) {
        of<int16_t>(obj, 0x6E) = 1;
        m = m | 1;
    } else {
        m = m & 0xFE;
    }
    of<uint8_t>(obj, 0x3C6) = m;
    of<int32_t>(obj, 0x320) = 0;
    of<int32_t>(obj, 0x324) = 0;
    of<int32_t>(obj, 0x32C) = int32_t(mem.at<int8_t>(r + 2));
    const uint32_t t = task_create(F::held_object_start, uint32_t(sizeof(Obj)));
    of<int32_t>(t, 0x32C) = int32_t(mem.at<int8_t>(r + 2));
    of<uint32_t>(t, 0x94) = of<uint32_t>(obj, 0x94);
    of<uint32_t>(t, 0x368) = of<uint32_t>(obj, 0x368);
    of<int16_t>(t, 0x64) = of<int16_t>(obj, 0x64);
    kind32_carry_wait_start(obj);
}

// 0x7FC70: +0x2EC the record's +6
template <class P> void Game<P>::kind32_carry_wait_start(uint32_t obj)
{
    const int32_t d = mem.at<int16_t>(of<uint32_t>(obj, 0x368) + 6);
    of<uint32_t>(obj, 0x0) = F::kind32_carry_wait;
    of<int32_t>(obj, 0x2EC) = d;
    kind32_carry_wait(obj);
}

// 0x7FC90: at 0 the throw unless the front attack positions are taken (then again next frame)
template <class P> void Game<P>::kind32_carry_wait(uint32_t obj)
{
    const int32_t t = of<int32_t>(obj, 0x2EC) - 1;
    of<int32_t>(obj, 0x2EC) = t;
    if (t == 0) {
        uint32_t busy;
        if (pc_path()) {
            // PC: attack_front_busy inline: the other front position, or either when it has none of them
            const uint8_t s = of<uint8_t>(obj, 0x2E7);
            if (s == 0)
                busy = mem.at<uint8_t>(G::attack_slots + 20);
            else if (s == 1)
                busy = mem.at<uint8_t>(G::attack_slots);
            else
                busy = (mem.at<uint8_t>(G::attack_slots) != 0 || mem.at<uint8_t>(G::attack_slots + 20) != 0) ? 1 : 0;
        } else {
            busy = attack_front_busy(obj);
        }
        if (busy == 0) {
            kind32_throw_start(obj);
            return;
        }
        of<int32_t>(obj, 0x2EC) = 1;
    }
    kind32_hits(obj);
}

// 0x7FCD0: second motion off; the throw (record +0xC bits 0-1 = 2: MO_z_ben_talunagb, else the kit's +4)
template <class P> void Game<P>::kind32_throw_start(uint32_t obj)
{
    const int32_t kit = of<int32_t>(obj, 0x32C);
    const uint32_t r = of<uint32_t>(obj, 0x368);
    body_second_motion(obj, 0);
    if ((mem.at<uint16_t>(r + 0xC) & 3) == 2)
        set_motion(obj, 0x21A);
    else
        set_motion(obj, uint32_t(int32_t(mem.at<int16_t>(G::kind32_carry_kits + 4 + uint32_t(kit) * 0x58))));
    of<uint32_t>(obj, 0x0) = F::kind32_throw;
    kind32_throw(obj);
}

// 0x7FD40: at the motion end kind32_throw_end; from the kit's release frame (+6) +0x320 bit 4
template <class P> void Game<P>::kind32_throw(uint32_t obj)
{
    if ((of<uint8_t>(obj, 0x3C5) >> 4) & 1) {
        kind32_throw_end(obj);
        return;
    }
    if (mem.at<int16_t>(G::kind32_carry_kits + 6 + uint32_t(of<int32_t>(obj, 0x32C)) * 0x58) <= of<int16_t>(obj, 0xF4))
        of<uint32_t>(obj, 0x320) = of<uint32_t>(obj, 0x320) | 0x10;
    kind32_hits(obj);
}

// the stance motion: MO_z_a_kamae2a or 2b by blink_phase bit 0
template <class P> uint32_t Game<P>::kind32_stance_motion()
{
    return (mem.at<uint8_t>(G::blink_phase) & 1) != 0 ? 0x1F1 : 0x1F2;
}

// 0x7FD90: the stance, +0x2EC the record's +10
template <class P> void Game<P>::kind32_throw_end(uint32_t obj)
{
    const uint32_t m = kind32_stance_motion();
    const uint32_t r = of<uint32_t>(obj, 0x368);
    set_motion_blend(obj, m, 0, 8);
    const int32_t d = mem.at<int16_t>(r + 10);
    of<uint32_t>(obj, 0x0) = F::kind32_throw_end_wait;
    of<int32_t>(obj, 0x2EC) = d;
    kind32_throw_end_wait(obj);
}

// 0x7FDE0: at 0 by record +0xC bits 6-7: deactivate or the stance
template <class P> void Game<P>::kind32_throw_end_wait(uint32_t obj)
{
    const int32_t t = of<int32_t>(obj, 0x2EC) - 1;
    of<int32_t>(obj, 0x2EC) = t;
    if (t != 0) {
        kind32_hits(obj);
        return;
    }
    if (((uint32_t(int32_t(mem.at<int16_t>(of<uint32_t>(obj, 0x368) + 0xC))) >> 6) & 3) == 0) {
        kind32_throw_done(obj);
        return;
    }
    kind32_after_throw_stance(obj);
}

// the stance after a throw / a hit: uncounted (+0x3C4 bit 3 off), the stance motion, its attack slot freed
template <class P> void Game<P>::kind32_stand_down(uint32_t obj)
{
    of<uint8_t>(obj, 0x3C4) = of<uint8_t>(obj, 0x3C4) & 0xF7;
    set_motion_blend(obj, kind32_stance_motion(), 0, 8);
    mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) - 1);
    mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) - 1);
    if (of<int8_t>(obj, 0x2E7) != -1)
        enemy_attack_slot_free(obj);
    of<uint32_t>(obj, 0x0) = F::kind32_after_throw_stance_frame;
    kind32_after_throw_stance_frame(obj);
}

// 0x7FE20
template <class P> void Game<P>::kind32_after_throw_stance(uint32_t obj)
{
    kind32_stand_down(obj);
}

// 0x7FEB0: out of view (coprocessor 0x7B, radius 20, < 0): the extras and body freed, ends; else the hits
template <class P> void Game<P>::kind32_after_throw_stance_frame(uint32_t obj)
{
    bool gone;
    if (pc_path()) {   // the PC, and PC Mode (pm::view_test)
        gone = pc_view_test2(of<float>(obj, 0x20), of<float>(obj, 0x24), of<float>(obj, 0x28), 20.0f) < 0.0f;
    } else {
        const uint32_t in[4] = {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28), 0x41A00000u};
        Tgp::Out o;
        copro(0x7B, in, 4, o);
        gone = !(ffrom(o.w[0]) >= 0.0f);
    }
    if (gone) {
        enemy_free_extras(obj);
        body_free_parts(obj);
        task_exit();
        return;
    }
    kind32_hits(obj);
}

// 0x7FF30
template <class P> void Game<P>::kind32_throw_done(uint32_t obj)
{
    kind32_deactivate(obj);
}

// 0x7FF40 (kit 3 after a hit): the hands not as the type's model set has them (nodes 5 / 8): +0x320 bit 1 (the
// held object lost); the hit wait
template <class P> void Game<P>::kind32_act3_after_hit(uint32_t obj)
{
    const uint32_t hdr = mem.u32(mem.u32(G::creature_type_headers + uint32_t(int32_t(of<int16_t>(obj, 0xCC))) * 4) + 0x1C);
    const uint32_t m5 = body_node_model(obj, 5);
    auto deep = [&](uint32_t a) { return mem.u32(mem.u32(mem.u32(mem.u32(a) + 0x1C) + 0x1C)); };
    if (m5 != deep(hdr + 0x20) || body_node_model(obj, 8) != deep(hdr + 0x24))
        of<uint32_t>(obj, 0x320) = of<uint32_t>(obj, 0x320) | 2;
    of<uint32_t>(obj, 0x0) = F::kind32_act3_hit_wait;
    kind32_act3_hit_wait(obj);
}

// 0x7FFC0
template <class P> void Game<P>::kind32_act3_hit_wait(uint32_t obj)
{
    if (kind32_hit_reaction_done(obj, F::kind32_act3_recover) != 0)
        return;
    kind32_hits(obj);
}

// 0x7FFF0: still holding (+0x320 bits 0-2 clear): the carry motion again (MO_z_a_dolamoti, the kit's carry motion as
// the second layer at frame 8) and the carry wait; else deactivate (record +0xC bits 6-7 0) or stand down
template <class P> void Game<P>::kind32_act3_recover(uint32_t obj)
{
    const int32_t kit = of<int32_t>(obj, 0x32C);
    if ((of<uint32_t>(obj, 0x320) & 7) == 0) {
        set_motion_blend(obj, 0x1E7, 8, 8);
        set_motion2_at_frame(obj, uint32_t(int32_t(mem.at<int16_t>(G::kind32_carry_kits + uint32_t(kit) * 0x58))), 8, 8);
        of<uint32_t>(obj, 0x0) = F::kind32_carry_resume;
        kind32_carry_resume(obj);
        return;
    }
    if (((uint32_t(int32_t(mem.at<int16_t>(of<uint32_t>(obj, 0x368) + 0xC))) >> 6) & 3) == 0) {
        kind32_deactivate(obj);
        return;
    }
    kind32_stand_down(obj);
}

// 0x80100: +0x2EC the record's +8, the carry wait
template <class P> void Game<P>::kind32_carry_resume(uint32_t obj)
{
    const int32_t d = mem.at<int16_t>(of<uint32_t>(obj, 0x368) + 8);
    of<uint32_t>(obj, 0x0) = F::kind32_carry_wait;
    of<int32_t>(obj, 0x2EC) = d;
    kind32_carry_wait(obj);
}

// 0x80120: the drum carrier's reaction when its drum is hit: step 0 second motion off, the kit's hit motion (+8);
// two frames before its end: stand down (record +0xC bits 6-7 set) or deactivate
template <class P> void Game<P>::kind32_hit_react(uint32_t obj)
{
    const int32_t kit = of<int32_t>(obj, 0x32C);
    const uint32_t r = of<uint32_t>(obj, 0x368);
    const int16_t step = of<int16_t>(obj, 0x66);
    if (step == 0) {
        body_second_motion(obj, 0);
        set_motion(obj, uint32_t(int32_t(mem.at<int16_t>(G::kind32_carry_kits + 8 + uint32_t(kit) * 0x58))));
        of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
    } else if (step != 1) {
        kind32_hits(obj);
        return;
    }
    const int32_t end = int32_t(mem.u32(G::motion_frame_counts + uint32_t(int32_t(of<int16_t>(obj, 0xD0))) * 4)) - 2;
    if (end == int32_t(of<int16_t>(obj, 0xF4))) {
        if (((uint32_t(int32_t(mem.at<int16_t>(r + 0xC))) >> 6) & 3) != 0) {
            kind32_stand_down(obj);
            return;
        }
        kind32_deactivate(obj);
        return;
    }
    kind32_hits(obj);
}

// 0x80900 / PC 0x4E53E8 (object, throw row): the axe leaves the hand: the flying axe's task (flying_axe_start). Its
// position and angles from the hand's node matrix (row +4 set: +0x214, else +0x244) times a fixed offset and turn,
// turned half round (stored matrix 1 first); its angle z / y (the matrix angles) give its forward vector (+0x394..,
// x = 1.0), angle x in +0x320. The thrower's hand loses the axe model (node 8 / model 0x464 or node 5 / 0x465), +0x334
// 1 or 0 | row +5 * 2. Kit 0 (Cyril's axe, model 0x5BC): the thrower's scale, record and action, its shot box, the kit's
// first model list word, shootable, type -1. PC: the same on the software matrix stack (the kind 32 file's copies).
template <class P> void Game<P>::kind32_axe_release(uint32_t obj, uint32_t row)
{
    LocalScope ls(*this);
    const uint32_t rw = G::axe_throw_rows + row * 0x18;
    const uint32_t n = task_create(F::flying_axe_start, uint32_t(sizeof(Obj)));
    const bool left = mem.at<uint8_t>(rw + 4) != 0;
    if constexpr (P::pc) {
        pc_matrix_push();
        uint32_t m = mem.u32(G::pc_matrix_ptr);
        for (uint32_t i = 0; i < 12; ++i)
            mem.set32(m + 4 * i, mem.u32(G::pc_stored_matrices + 0x30 + 4 * i));
        if (left) {
            pc_matrix_mul8(obj + ob(0x214));
            pc_matrix_translate8(1.5322f, 1.3037f, -2.452f);   // 0x3FC41F21, 0x3FA6DFA4, 0xC01CED91
            pc_matrix_rotate_z8(0x49E1);
            pc_matrix_rotate_y7(0xE4E6);
            pc_matrix_rotate_x8(0x38B0);
        } else {
            pc_matrix_mul8(obj + ob(0x244));
            pc_matrix_translate8(1.5037f, 1.2829f, 2.636f);    // 0x3FC0793E, 0x3FA43611, 0x4028B439
            pc_matrix_rotate_z8(0xC7CB);
            pc_matrix_rotate_y7(0xE5A2);
            pc_matrix_rotate_x8(0xB713);
        }
        pc_matrix_rotate_y7(0x8000);
        m = mem.u32(G::pc_matrix_ptr);
        of<uint32_t>(n, 0x20) = mem.u32(m + 0x24);
        of<uint32_t>(n, 0x24) = mem.u32(m + 0x28);
        of<uint32_t>(n, 0x28) = mem.u32(m + 0x2C);
        const uint32_t a = local_alloc(12);   // esp: x, z, y
        pc_matrix_angles8(a, a + 8, a + 4);
        relocate(0, a, 4);
        relocate(1, a + 8, 4);
        relocate(2, a + 4, 4);
        of<uint32_t>(n, 0x320) = mem.u32(a);
        pcm_pop();
        pc_matrix_push();
        pc_matrix_identity8();
        pc_matrix_rotate_z8(mem.u32(a + 4));
        pc_matrix_rotate_y7(mem.u32(a + 8));
        pc_matrix_transform8(1.0f, 0.0f, 0.0f, n + ob(0x394), n + ob(0x398), n + ob(0x39C));
        pcm_pop();
    } else if (pcmode != 0) {
        // PC Mode: the PC's form above on local matrices; the hand's node matrix is the arcade body drawing's (in the
        // coprocessor's layout, turned into the PC's)
        pm::Matrix m = pm_m1();
        m.mul(pm_copro_words_at(obj + (left ? 0x214u : 0x244u)).e);
        if (left) {
            m.translate(1.5322f, 1.3037f, -2.452f);
            m.rotate_z(0x49E1);
            m.rotate_y(0xE4E6);
            m.rotate_x(0x38B0);
        } else {
            m.translate(1.5037f, 1.2829f, 2.636f);
            m.rotate_z(0xC7CB);
            m.rotate_y(0xE5A2);
            m.rotate_x(0xB713);
        }
        m.rotate_y(0x8000);
        of<float>(n, 0x20) = m.e[9];
        of<float>(n, 0x24) = m.e[10];
        of<float>(n, 0x28) = m.e[11];
        uint32_t ax = 0, az = 0, ay = 0;   // pc_matrix_angles8(x, z, y): outA, outB, outC
        pm::matrix_angles8(m, ax, az, ay);
        of<uint32_t>(n, 0x320) = ax;
        pm::Matrix d;
        d.rotate_z(ay);
        d.rotate_y(az);
        float v[3];
        d.transform(1.0f, 0.0f, 0.0f, v[0], v[1], v[2]);
        of<float>(n, 0x394) = v[0];
        of<float>(n, 0x398) = v[1];
        of<float>(n, 0x39C) = v[2];
    } else {
        Tgp::Out none, o;
        const uint32_t slot[1] = {1};
        copro(0x74, slot, 1, none);
        uint32_t mt[12];
        const uint32_t node = obj + ob(left ? 0x214 : 0x244);
        for (uint32_t i = 0; i < 12; ++i)
            mt[i] = mem.u32(node + 4 * i);
        copro(0x09, mt, 12, none);
        if (left) {
            const uint32_t t[3] = {0x3FC41F21u, 0x3FA6DFA4u, 0xC01CED91u};
            copro(0x12, t, 3, none);
            const uint32_t r[3] = {0x49E1, 0xE4E6, 0x38B0};
            copro(0x86, r, 3, none);
        } else {
            const uint32_t t[3] = {0x3FC0793Eu, 0x3FA43611u, 0x4028B439u};
            copro(0x12, t, 3, none);
            const uint32_t r[3] = {0xC7CB, 0xE5A2, 0xB713};
            copro(0x86, r, 3, none);
        }
        const uint32_t half[1] = {0x8000};
        copro(0x15, half, 1, none);
        copro(0x20, nullptr, 0, o);
        of<uint32_t>(n, 0x20) = o.w[0];
        of<uint32_t>(n, 0x24) = o.w[1];
        of<uint32_t>(n, 0x28) = o.w[2];
        Tgp::Out an;
        copro(0x71, nullptr, 0, an);
        copro(0x06, nullptr, 0, none);
        of<uint32_t>(n, 0x320) = an.w[2];
        copro(0x21, nullptr, 0, none);
        const uint32_t rz[1] = {an.w[0]};
        copro(0x16, rz, 1, none);
        const uint32_t ry[1] = {an.w[1]};
        copro(0x15, ry, 1, none);
        const uint32_t v[3] = {0x3F800000u, 0, 0};
        Tgp::Out f;
        copro(0x1A, v, 3, f);
        of<uint32_t>(n, 0x394) = f.w[0];
        of<uint32_t>(n, 0x398) = f.w[1];
        of<uint32_t>(n, 0x39C) = f.w[2];
        copro(0x06, nullptr, 0, none);
    }
    if (left) {
        of<int32_t>(n, 0x334) = 1;
        axe_hand_model_hide(obj, 8, 0x464);
    } else {
        of<int32_t>(n, 0x334) = 0;
        axe_hand_model_hide(obj, 5, 0x465);
    }
    of<int32_t>(n, 0x32C) = 0;
    const uint32_t box = G::held_object_shot_boxes + of<uint32_t>(obj, 0x32C) * 0x34;
    const uint32_t kit = G::held_object_kits + of<uint32_t>(n, 0x32C) * 0x58;
    obj_clear_37c(n);
    of<uint32_t>(n, 0x94) = of<uint32_t>(obj, 0x94);
    of<uint32_t>(n, 0x368) = of<uint32_t>(obj, 0x368);
    of<int16_t>(n, 0x64) = of<int16_t>(obj, 0x64);
    of<int32_t>(n, 0x328) = 0;
    of<int32_t>(n, 0x2EC) = 0;
    of<int16_t>(n, 0x6A) = 1;
    of<int16_t>(n, 0x68) = 2;
    of<int32_t>(n, 0x58) = 1;
    of<int16_t>(n, 0x6C) = 0;
    of<int16_t>(n, 0xCC) = -1;
    of<uint32_t>(n, 0x334) = of<uint32_t>(n, 0x334) | (uint32_t(int32_t(mem.at<int8_t>(rw + 5))) << 1);
    of<int32_t>(n, 0xC8) = 0;
    of<uint8_t>(n, 0x3C4) = of<uint8_t>(n, 0x3C4) & 0xFB;
    of<uint32_t>(n, 0x90) = mem.u32(box);
    of<uint8_t>(n, 0x2E7) = 0xFF;
    of<int32_t>(n, 0x54) = 0x5BC;
    of<int16_t>(n, 0x70) = mem.at<int16_t>(mem.u32(kit + 0x18) + 2);
}

// 0x68630 / PC 0x4C4334 (kind 32 not counted): while shootable, part_hit on every child of the root (body_current
// the object), then body_free_cut_parts; no camera-stop test and no killer choice (enemy_part_hits has both)
template <class P> void Game<P>::kind32_part_hits(uint32_t obj)
{
    if ((of<uint8_t>(obj, 0x58) & 1) == 0)
        return;
    const uint32_t root = of<uint32_t>(obj, 0xC8);
    mem.set32(G::body_current, obj);
    uint32_t list = mem.at<BodyPart>(root).children.a;
    for (uint32_t i = 0; i < uint32_t(mem.at<BodyPart>(root).child_count); ++i, list += 4)
        part_hit(mem.u32(list));
    body_free_cut_parts(root);
}

// 0x80240: the drum carrier: its drum hit (+0x324 bit 0, still held) -> kind32_hit_react; +0x320 bit 0 = bit 4.
// Part hits (counted: enemy_part_hits, else kind32_part_hits); a hit reaction (drum carrier holding: MO 0x210; iron
// ball: the type's hit motion; not while attacking or in states 7 / 8) or enemy_hit_reaction; a finished reaction
// (flags 0x10000100) -> the kit's after-hit routine; the death check (+0x338 bit 0: a fall death); at no hit points
// the event flag of kind32_event_flags[action][record +1]; the shadow (rising / iron ball entrance 4: at its y),
// drawn, depth list, anim times, its view position (stored matrix 5)
template <class P> void Game<P>::kind32_hits(uint32_t obj)
{
    const uint32_t r = of<uint32_t>(obj, 0x368);
    if (of<int16_t>(obj, 0x64) == 3) {
        if ((of<uint8_t>(obj, 0x320) & 7) == 0 && (of<uint8_t>(obj, 0x324) & 1) != 0) {
            const uint32_t f = of<uint32_t>(obj, 0x320);
            of<int16_t>(obj, 0x66) = 0;
            of<uint32_t>(obj, 0x0) = F::kind32_hit_react;
            of<uint32_t>(obj, 0x320) = f | 0xC;
            kind32_hit_react(obj);
            return;
        }
        const uint32_t f = of<uint32_t>(obj, 0x320);
        of<uint32_t>(obj, 0x320) = (f & ~1u) | ((uint32_t(int32_t(f) >> 4)) & 1);
    }
    const int32_t type = of<int16_t>(obj, 0xCC);
    mem.at<uint8_t>(G::hit_class) = 0;
    mem.set32(G::hit_type, uint32_t(type));
    mem.at<int16_t>(G::hit_damage) = 0;
    if ((of<uint8_t>(obj, 0x3C4) >> 3) & 1)
        enemy_part_hits(obj);
    else
        kind32_part_hits(obj);
    const int16_t action = of<int16_t>(obj, 0x64);
    bool react = false, generic = false;
    if (action == 3) {
        if ((of<uint8_t>(obj, 0x320) & 7) != 0) {
            generic = true;
        } else if (mem.at<uint8_t>(G::hit_class) != 0 && of<int32_t>(obj, 0x58) >= 0 && of<int16_t>(obj, 0x5C) != 7 &&
                   of<int16_t>(obj, 0x5C) != 8) {
            body_second_motion(obj, 0);
            set_motion(obj, 0x210);
            react = true;
        }
    } else if (action == 4) {
        if (mem.at<uint8_t>(G::hit_class) != 0 && of<int32_t>(obj, 0x58) >= 0 && of<int16_t>(obj, 0x5C) != 7 &&
            of<int16_t>(obj, 0x5C) != 8) {
            body_second_motion(obj, 0);
            const int32_t hc = int32_t(mem.at<int8_t>(G::hit_class)) % 9;
            const uint32_t t = mem.u32(mem.u32(G::enemy_hit_motions + uint32_t(type) * 4) + uint32_t(int32_t(action)) * 4);
            set_motion(obj, mem.u32(t + uint32_t(int32_t(int8_t(hc))) * 4));
            react = true;
        }
    } else {
        generic = true;
    }
    if (react) {
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | (1u << 28);
        enemy_voice(obj, 0);
    } else if (generic) {
        enemy_hit_reaction(obj);
    }
    if ((of<uint32_t>(obj, 0x58) & 0x10000100u) == 0x10000100u)
        of<uint32_t>(obj, 0x0) = mem.u32(G::kind32_kits + 8 + uint32_t(int32_t(of<int16_t>(obj, 0x64))) * 12);
    if ((of<uint32_t>(obj, 0x338) & 1) == 0)
        enemy_check_death(obj);
    else
        enemy_fall_death_check(obj);
    if (!(0 < of<int16_t>(obj, 0x70))) {
        const uint32_t t = mem.u32(G::kind32_event_flags + uint32_t(int32_t(of<int16_t>(obj, 0x64))) * 4);
        const int32_t e = mem.at<int8_t>(t + uint32_t(int32_t(mem.at<int8_t>(r + 1))));
        if (e != -1)
            mem.at<uint8_t>(G::event_flags + uint32_t(e)) = 1;
    }
    const int16_t a2 = of<int16_t>(obj, 0x64);
    if ((a2 == 2 || a2 == 4) && ((uint32_t(int32_t(mem.at<int8_t>(r + 3))) >> 1) & 7) == 4)
        shadow_setup(of<float>(obj, 0x24));
    else
        shadow_update(obj);
    object_draw(obj);
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xFFFFFEF9u;
    depth_list_add(obj);
    of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) + 1;
    of<int32_t>(obj, 0x10C) = of<int32_t>(obj, 0x10C) + 1;
    if (pc_path()) {
        // PC: stored matrix 5 on the software matrix stack, the file's pc_matrix_transform copy
        pc_matrix_push();
        pcm_load_stored5();
        float p[3];
        pc_matrix_transform(of<float>(obj, 0x20), of<float>(obj, 0x24), of<float>(obj, 0x28), p[0], p[1], p[2]);
        of<float>(obj, 0x98) = p[0];
        of<float>(obj, 0x9C) = p[1];
        of<float>(obj, 0xA0) = p[2];
        pcm_pop();
        return;
    }
    Tgp::Out o, none;
    const uint32_t slot[1] = {5};
    copro(0x74, slot, 1, none);
    const uint32_t v[3] = {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)};
    copro(0x1A, v, 3, o);
    of<uint32_t>(obj, 0x98) = o.w[0];
    of<uint32_t>(obj, 0x9C) = o.w[1];
    copro(0x06, nullptr, 0, none);
    of<uint32_t>(obj, 0xA0) = o.w[2];
}

// 0x80510 (object, next): the hit motion over (the second motion's end when it runs one): reaction bit off; a
// counted one stands (the stance motion, kind32_after_throw_stance_frame), others go to `next`; 1. Else 0
template <class P> uint32_t Game<P>::kind32_hit_reaction_done(uint32_t obj, uint32_t next)
{
    bool done;
    if (((of<uint8_t>(obj, 0x3C6) >> 5) & 1) == 0)
        done = ((of<uint8_t>(obj, 0x3C5) >> 4) & 1) != 0;
    else
        done = ((of<uint8_t>(obj, 0x3C5) >> 5) & 1) != 0;
    if (!done)
        return 0;
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xEFFFFFFFu;
    if (((of<uint8_t>(obj, 0x3C4) >> 3) & 1) == 0) {
        set_motion_blend(obj, kind32_stance_motion(), 0, 8);
        of<uint32_t>(obj, 0x0) = F::kind32_after_throw_stance_frame;
        kind32_after_throw_stance_frame(obj);
    } else {
        kind32_call(next, obj);
    }
    return 1;
}

// 0x83270 (kit 2 after a hit, the reaction over): second motion off, +0x2EC 1, MO_z_a_kamae2a from no motion, FixMode,
// the throw wait
template <class P> void Game<P>::kind32_act2_rise_start(uint32_t obj)
{
    body_second_motion(obj, 0);
    of<int32_t>(obj, 0x2EC) = 1;
    of<int16_t>(obj, 0xD0) = 0;
    set_motion_blend(obj, 0x1F1, 0, 4);
    of<uint32_t>(obj, 0x0) = F::kind32_throw_wait;
    of<uint8_t>(obj, 0x3C6) = of<uint8_t>(obj, 0x3C6) | 1;
    kind32_throw_wait(obj);
}

// 0x82F00: becomes a standard enemy: (kit 2 with record +1 = 6: event flag 0x41) second motion off, its attack slot
// freed, +0x330 / +0x304 1, enemy_wait_camera_stop as its routine (step / action / +0x2EC 0, shootable, FixMode off)
template <class P> void Game<P>::kind32_deactivate(uint32_t obj)
{
    const uint32_t r = of<uint32_t>(obj, 0x368);
    if (of<int16_t>(obj, 0x64) == 2 && mem.at<uint8_t>(r + 1) == 6)
        mem.at<uint8_t>(G::event_flags + 0x41) = 1;
    body_second_motion(obj, 0);
    if (of<uint8_t>(obj, 0x2E7) != 0xFF)
        enemy_attack_slot_free(obj);
    of<int32_t>(obj, 0x330) = 1;
    of<int32_t>(obj, 0x304) = 1;
    of<uint32_t>(obj, 0x0) = F::enemy_wait_camera_stop;
    const uint32_t f = of<uint32_t>(obj, 0x58);
    of<int16_t>(obj, 0x66) = 0;
    const uint8_t m = of<uint8_t>(obj, 0x3C6);
    of<int16_t>(obj, 0x64) = 0;
    of<int32_t>(obj, 0x2EC) = 0;
    of<uint32_t>(obj, 0x58) = f | 1;
    of<uint8_t>(obj, 0x3C6) = m & 0xFE;
    enemy_wait_camera_stop(obj);
}

template <class P> float Game<P>::pc_fast_sqrt9(float x) { return pc_fast_sqrt_table(x); }   // PC 0x4E22A8

#include "port/enemy_kit_zombie_iron.inc"
#include "port/enemy_kit_held.inc"

} // namespace hotd::port
