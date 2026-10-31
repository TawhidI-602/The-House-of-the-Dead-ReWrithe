// Enemies: kind 69, the frog Bouere (creature type 7 `BO_frog`; arcade cluster 0x88200..0x8A3F0), read from the arcade
// disassembly. The spawner (kind69_frog_bouere) runs the set-up of its record's action (kind69_setups 0xCA180) and
// ends; the set-ups make the frogs, which then run kind69_frog_frame with a state routine (kind69_states 0xCA150 by
// +0x5C; states 6-10 are the set-ups again) and sub-steps in +0x60. Hops go along a jump arc (jump_arc_set /
// jump_arc_step_b: from the position to +0x3A0..+0x3A8 over 35 frames). The arcade computes the hop targets in double
// precision (i960 long-real ops through the fp registers): kept here as double maths rounded to float at the same
// points. Words: +0x2D4.. its hit point (drawn / shot), +0x2E0 its shadow size, +0x330 / +0x334 per-scene flags,
// +0x3AC.. the position before a leap, +0x3B8.. the leap point, +0x50 a timer.
#include "port/game.h"

namespace hotd::port {

// 0x88210 (kind 69): sub-step 0, action = +0x70, its set-up (kind69_setups), then the spawner ends
template <class P> void Game<P>::kind69_frog_bouere(uint32_t obj)
{
    const int32_t a = of<int16_t>(obj, 0x70);
    of<int16_t>(obj, 0x60) = 0;
    of<int16_t>(obj, 0x64) = int16_t(a);
    kind69_call(mem.u32(G::kind69_setups + uint32_t(a) * 4), obj);
    task_exit();
}

// a call through kind69_setups / kind69_states: the ported routines directly, the rest as boundaries
template <class P> void Game<P>::kind69_call(uint32_t routine, uint32_t obj)
{
    switch (routine) {
    case F::kind69_frog_st0_none: return;
    case F::kind69_frog_st1_hit: kind69_frog_st1_hit(obj); return;
    case F::kind69_frog_st2_die: kind69_frog_st2_die(obj); return;
    case F::kind69_frog_st3_hop: kind69_frog_st3_hop(obj); return;
    case F::kind69_frog_st4_hop_in_place: kind69_frog_st4_hop_in_place(obj); return;
    case F::kind69_frog_st5_leap_attack: kind69_frog_st5_leap_attack(obj); return;
    case F::kind69_frog_setup0_swimmers: kind69_frog_setup0_swimmers(obj); return;
    case F::kind69_frog_setup1_pose: kind69_frog_setup1_pose(obj); return;
    case F::kind69_frog_setup2_leap_in: kind69_frog_setup2_leap_in(obj); return;
    case F::kind69_frog_setup3_swim_out: kind69_frog_setup3_swim_out(obj); return;
    case F::kind69_frog_setup4_swim_out_far: kind69_frog_setup4_swim_out_far(obj); return;
    default: call_routine(routine, obj); return;
    }
}

// 0x88230: body (its type, MO 0xBB), type set-up, counted
template <class P> void Game<P>::kind69_frog_body_init(uint32_t obj)
{
    const int32_t type = of<int16_t>(obj, 0xCC);
    mem.set32(G::hit_type, uint32_t(type));
    body_init(obj, type, 0xBB);
    creature_type_setup(obj);
    enemy_counts_add(1);
}

// 0x88280: a hit (flags bit 8, or return_false) saves state / sub-step in +0x5E / +0x62 and goes to state 1; the
// state routine; the shadow above the floor (+0x6C 2) or, on it, none with its size (+0x2E0) halved while the body
// is drawn; its hit point through the current matrix (+0x38); anim_time; depth and attacker lists
template <class P> void Game<P>::kind69_frog_frame(uint32_t obj)
{
    if (((of<uint16_t>(obj, 0x58) >> 8) & 1) || return_false() != 0) {
        const int16_t sub = of<int16_t>(obj, 0x60);
        of<int16_t>(obj, 0x60) = 0;
        const int16_t st = of<int16_t>(obj, 0x5C);
        of<int16_t>(obj, 0x5C) = 1;
        of<int16_t>(obj, 0x5E) = st;
        of<int16_t>(obj, 0x62) = sub;
    }
    fpAdjust += 0x40;   // the state routine's frame (callx from a routine without locals)
    kind69_call(mem.u32(G::kind69_states + uint32_t(int32_t(of<int16_t>(obj, 0x5C))) * 4), obj);
    fpAdjust -= 0x40;
    const uint16_t shadow = of<uint16_t>(obj, 0x2E0);
    if (of<float>(obj, 0x24) > mem.f32(G::floor_y)) {
        shadow_setup(mem.f32(G::floor_y));
        of<int16_t>(obj, 0x6C) = 2;
    } else {
        const int32_t s = of<int16_t>(obj, 0x2E0);
        of<int16_t>(obj, 0x6C) = 0;
        of<int16_t>(obj, 0x2E0) = int16_t(s / 2);
    }
    body_draw(obj);
    of<uint16_t>(obj, 0x2E0) = shadow;
    of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) + 1;
    if (pc_path()) {
        // PC: its own copy of pc_matrix_transform (0x4F4E20)
        float ox, oy, oz;
        pc_matrix_transform(of<float>(obj, 0x2D4), of<float>(obj, 0x2D8), of<float>(obj, 0x2DC), ox, oy, oz);
        of<float>(obj, 0x38) = ox;
        of<float>(obj, 0x3C) = oy;
        of<float>(obj, 0x40) = oz;
    } else {
        Tgp::Out o;
        const uint32_t p[3] = {of<uint32_t>(obj, 0x2D4), of<uint32_t>(obj, 0x2D8), of<uint32_t>(obj, 0x2DC)};
        copro(0x1A, p, 3, o);
        of<uint32_t>(obj, 0x38) = o.w[0];
        of<uint32_t>(obj, 0x3C) = o.w[1];
        of<uint32_t>(obj, 0x40) = o.w[2];
    }
    depth_list_add(obj);
    attacker_list_add(obj);
}

// 0x88370: hit: bit 8 cleared; a frog hit in its leap (saved state 5) with a target frees one attacker
// (bats_attacking) and the target; then dying (state 2)
template <class P> void Game<P>::kind69_frog_st1_hit(uint32_t obj)
{
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~(1u << 8);
    if (of<int16_t>(obj, 0x5E) == 5 && of<uint8_t>(obj, 0x2E8) != 0xFF) {
        mem.set32(G::bats_attacking, mem.u32(G::bats_attacking) - 1);
        of<int8_t>(obj, 0x2E8) = -1;
    }
    of<int16_t>(obj, 0x60) = 0;
    of<int16_t>(obj, 0x5C) = 2;
}

// 0x883D0: dying: step 0 sound 0xA01345, a hit effect at its hit point, kill score 80; 1 not shootable, its attack
// position and target freed, thrown off (up 1.633, sideways 0.2 away from the camera by its side, turned a quarter),
// MO 0xB3; 2 falls (gravity -0.0817; the motion held after frame 9) until below the floor; 3 counters down, body
// freed, the task ends
template <class P> void Game<P>::kind69_frog_st2_die(uint32_t obj)
{
    int32_t sub = of<int16_t>(obj, 0x60);
    if (sub < 0 || sub > 3)
        return;
    if (sub == 0) {
        sound_cue(P::pc ? 0x83 : 0xA01345);
        hit_effect_at(of<float>(obj, 0x2D4), of<float>(obj, 0x2D8), of<float>(obj, 0x2DC));
        kill_score_shooter(obj, 0x50);
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
        sub = 1;
    }
    if (sub == 1) {
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
        mem.at<uint8_t>(G::attack_slots + uint32_t(int32_t(of<int8_t>(obj, 0x2E7))) * 20) = 0;
        const int32_t side = of<int8_t>(obj, 0x2E9);
        of<int8_t>(obj, 0x2E8) = -1;
        uint32_t vx = 0, vz = 0;
        of<uint32_t>(obj, 0x7C) = 0x3FD11111u;
        switch (side) {
        case 0: of<uint32_t>(obj, 0x34) = 0x4000; vx = 0x3E4CCCCDu; break;
        case 1: of<uint32_t>(obj, 0x34) = 0xFFFFC000u; vx = 0xBE4CCCCDu; break;
        case 2: of<uint32_t>(obj, 0x2C) = 0x4000; vz = 0x3E4CCCCDu; break;
        case 3: of<uint32_t>(obj, 0x2C) = 0xFFFFC000u; vz = 0xBE4CCCCDu; break;
        default: break;
        }
        of<uint32_t>(obj, 0x80) = vz;
        of<uint32_t>(obj, 0x78) = vx;
        set_motion_blend(obj, 0xB3, 0, 8);
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
        sub = 2;
    }
    if (sub == 2) {
        if (of<int16_t>(obj, 0xF4) > 9)
            of<int32_t>(obj, 0xF8) = of<int32_t>(obj, 0xF8) - 1;
        const float vy = of<float>(obj, 0x7C) + ffrom(0xBDA740DAu);
        const float x = of<float>(obj, 0x20) + of<float>(obj, 0x78);
        const float z = of<float>(obj, 0x28) + of<float>(obj, 0x80);
        const float y = of<float>(obj, 0x24) + vy;
        of<float>(obj, 0x20) = x;
        of<float>(obj, 0x28) = z;
        of<float>(obj, 0x7C) = vy;
        of<float>(obj, 0x24) = y;
        if (y > mem.f32(G::floor_y))
            return;
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
    }
    mem.set32(G::frog_count, mem.u32(G::frog_count) - 1);
    mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) - 1);
    mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) - 1);
    body_free_parts(obj);
    task_exit();
}

// the hop target: (x, y, z) + 14 x normalize(d) in double precision, each rounded to float (as the arcade's
// mulrl / addrl through the fp registers); A only
template <class P> void Game<P>::frog_hop_target(uint32_t obj, float dx, float dy, float dz)
{
    Tgp::Out o;
    const uint32_t v[3] = {fbits(dx), fbits(dy), fbits(dz)};
    copro(0x0C, v, 3, o);
    const double k = 14.0;
    const float tx = float(double(of<float>(obj, 0x20)) + double(ffrom(o.w[0])) * k);
    const float ty = float(double(of<float>(obj, 0x24)) + double(ffrom(o.w[1])) * k);
    const float tz = float(double(of<float>(obj, 0x28)) + double(ffrom(o.w[2])) * k);
    of<float>(obj, 0x3A0) = tx;
    of<float>(obj, 0x3A4) = ty;
    of<float>(obj, 0x3A8) = tz;
}

// 0x885E0: hops toward the camera: step 0 its side, MO 0xB2; 1 faces the camera, the hop target 14 ahead put on the
// floor (floor_probe); 2 after frame 8 the jump arc (35 frames); 3 landing: dust, sound 0xA0125F; 4 after frame 59
// tilts by the pitch toward a word left on its stack (+0x58 of its frame: an uninitialised local), within 45 of the
// camera the leap (state 5), else 3 in 4 hops again, 1 in 4 sound 0xA01269 and MO 0xB9 (a pause); 5 at the motion end
// step 0
template <class P> void Game<P>::kind69_frog_st3_hop(uint32_t obj)
{
    uint32_t sub = uint32_t(int32_t(of<int16_t>(obj, 0x60)));
    if (sub > 5)
        return;
    auto next = [&]() { of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1); };
    switch (sub) {
    case 0:
        camera_side_set(obj);
        set_motion_blend(obj, 0xB2, 0, 8);
        next();
        [[fallthrough]];
    case 1: {
        LocalScope scope(*this);
        const uint32_t L = local_alloc(0x40);   // fp + 0x70 .. + 0xAF
        const float dx = mem.f32(G::camera_pos) - of<float>(obj, 0x20);
        const float dz = mem.f32(G::camera_pos + 8) - of<float>(obj, 0x28);
        const uint32_t yaw = yaw_toward_a(dx, 0.0f, dz);
        frog_hop_target(obj, dx, 0.0f, dz);
        of<uint32_t>(obj, 0x30) = yaw;
        mem.set32(L + 0x10, of<uint32_t>(obj, 0x3A0));
        mem.set32(L + 0x14, 0x47000000u);
        mem.set32(L + 0x18, of<uint32_t>(obj, 0x3A8));
        // its frame's +0x58 (read again in step 4, of a later call): dx, or the floor probe's second point's x
        const uint32_t slot58 = orig_fp() + fpAdjust + 0x58;
        mem.at<float>(slot58) = dx;
        if (floor_probe(L + 0x10, L, L + 0x20, L + 0x30) == 1) {
            relocate(1, L, 0x10);
            relocate(2, L + 0x20, 0x10);
            of<uint32_t>(obj, 0x3A4) = mem.u32(L + 4);
            mem.set32(slot58, mem.u32(L + 0x20));
        }
        next();
    }
        [[fallthrough]];
    case 2:
        if (!(of<int16_t>(obj, 0xF4) > 8))
            return;
        jump_arc_set(obj, of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28),
                     of<uint32_t>(obj, 0x3A0), of<uint32_t>(obj, 0x3A4), of<uint32_t>(obj, 0x3A8), 35);
        next();
        [[fallthrough]];
    case 3:
        if (jump_arc_step_b(obj) != 1)
            return;
        hop_dust_spawn(of<float>(obj, 0x20), of<float>(obj, 0x24), of<float>(obj, 0x28));
        sound_cue(P::pc ? 0x57 : 0xA0125F);
        next();
        [[fallthrough]];
    case 4: {
        if (!(of<int16_t>(obj, 0xF4) > 59))
            return;
        // the original reads +0x58 of its own frame, which nothing wrote in this call
        const uint32_t stale = mem.u32(orig_fp() + fpAdjust + 0x58);
        Tgp::Out o;
        const uint32_t v[3] = {stale, 0, 0};
        copro(0x0F, v, 3, o);
        uint32_t pitch = o.w[1];
        const uint32_t s = uint32_t(int32_t(int16_t(pitch & 0xFFFF)));
        if (((s << 1) ^ s) & 0x8000)
            pitch = 0x8000 - pitch;
        of<uint32_t>(obj, 0x2C) = pitch - 0x2000;
        const float d = enemy_distance(of<float>(obj, 0x20), of<float>(obj, 0x28), mem.f32(G::camera_pos),
                                       mem.f32(G::camera_pos + 8));
        if (d < 45.0f) {
            of<int16_t>(obj, 0x60) = 0;
            of<int16_t>(obj, 0x5C) = 5;
            return;
        }
        if (((rand() & 3) & 0xFF) != 0) {
            of<int16_t>(obj, 0x60) = 1;
            return;
        }
        sound_cue(P::pc ? 0x5A : 0xA01269);
        set_motion_blend(obj, 0xB9, 0, 8);
        next();
    }
        [[fallthrough]];
    case 5:
        if ((of<uint8_t>(obj, 0x3C5) >> 4) & 1)
            of<int16_t>(obj, 0x60) = 0;
        return;
    }
}

// 0x88A50: hops along its side's axis toward the camera without a floor probe (the target keeps its height); the
// same landing, distance test and pause as state 3 (no tilt)
template <class P> void Game<P>::kind69_frog_st4_hop_in_place(uint32_t obj)
{
    uint32_t sub = uint32_t(int32_t(of<int16_t>(obj, 0x60)));
    if (sub > 5)
        return;
    auto next = [&]() { of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1); };
    switch (sub) {
    case 0:
        camera_side_set(obj);
        set_motion_blend(obj, 0xB2, 0, 8);
        next();
        [[fallthrough]];
    case 1: {
        const float dy = mem.f32(G::camera_pos + 4) - of<float>(obj, 0x24);
        const uint8_t side = of<uint8_t>(obj, 0x2E9);
        float dx = 0.0f, dz = 0.0f;
        if (side == 0 || side == 1)
            dx = mem.f32(G::camera_pos) - of<float>(obj, 0x20);
        if (side == 2 || side == 3)
            dz = mem.f32(G::camera_pos + 8) - of<float>(obj, 0x28);
        const uint32_t yaw = yaw_toward_a(dx, dy, dz);
        if (pc_path()) {
            // PC: the direction normalised in floats (its table square root), then 14.0 along it
            const float inv = 1.0f / pc_fast_sqrt((dx * dx + dy * dy) + dz * dz);
            const float nx = dx * inv, ny = dy * inv, nz = dz * inv;
            of<float>(obj, 0x3A0) = of<float>(obj, 0x20) + nx * 14.0f;
            of<float>(obj, 0x3A4) = of<float>(obj, 0x24) + ny * 14.0f;
            of<float>(obj, 0x3A8) = of<float>(obj, 0x28) + nz * 14.0f;
        } else
            frog_hop_target(obj, dx, dy, dz);
        next();
        of<uint32_t>(obj, 0x30) = yaw;
    }
        [[fallthrough]];
    case 2:
        if (!(of<int16_t>(obj, 0xF4) > 8))
            return;
        jump_arc_set(obj, of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28),
                     of<uint32_t>(obj, 0x3A0), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x3A8), 35);
        next();
        [[fallthrough]];
    case 3:
        if (jump_arc_step_b(obj) != 1)
            return;
        hop_dust_spawn(of<float>(obj, 0x20), mem.f32(G::floor_y), of<float>(obj, 0x28));
        sound_cue(P::pc ? 0x57 : 0xA0125F);
        next();
        [[fallthrough]];
    case 4: {
        if (!(of<int16_t>(obj, 0xF4) > 59))
            return;
        const float d = enemy_distance(of<float>(obj, 0x20), of<float>(obj, 0x28), mem.f32(G::camera_pos),
                                       mem.f32(G::camera_pos + 8));
        if (d < 45.0f) {
            of<int16_t>(obj, 0x60) = 0;
            of<int16_t>(obj, 0x5C) = 5;
            return;
        }
        if (((rand() & 3) & 0xFF) != 0) {
            of<int16_t>(obj, 0x60) = 1;
            return;
        }
        sound_cue(P::pc ? 0x5A : 0xA01269);
        set_motion_blend(obj, 0xB9, 0, 8);
        next();
    }
        [[fallthrough]];
    case 5:
        if ((of<uint8_t>(obj, 0x3C5) >> 4) & 1)
            of<int16_t>(obj, 0x60) = 0;
        return;
    }
}

// 0x88E30: the leap at a player: picks a target (a free front position unless +0x330), one more attacker, its
// position saved; the screen point (+-124 by the target's side, 0 with one player; -80 high) 10 in front of the camera
// (stored matrix 1), faces it, MO 0xB5; after frame 8 the arc there (splash when it leaves the water), hits the player
// (player_hit(target, 1, 1)), MO 0xBB for 20 frames, MO 0xB5 and the arc back to the saved position; in chapter 1
// events 9 (state 9) / 6, 12, 14 (state 10) with +0x330 back into its scene (+0x31C + 6); else landing dust and sound,
// one attacker less, after frame 59 step 0
template <class P> void Game<P>::kind69_frog_st5_leap_attack(uint32_t obj)
{
    uint32_t sub = uint32_t(int32_t(of<int16_t>(obj, 0x60)));
    if (sub > 10)
        return;
    auto next = [&]() { of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1); };
    switch (sub) {
    case 0:
        set_motion_blend(obj, 0xB9, 0, 8);
        next();
        [[fallthrough]];
    case 1:
        pick_target_player(obj);
        if (of<uint8_t>(obj, 0x2E8) == 0xFF) {
            of<int8_t>(obj, 0x2E8) = -1;
            return;
        }
        if (of<uint32_t>(obj, 0x330) != 1) {
            const int32_t slot = of<int8_t>(obj, 0x2E7);
            of<uint32_t>(obj, 0x334) = 1;
            if (slot > 2)
                return;
        }
        mem.set32(G::bats_attacking, mem.u32(G::bats_attacking) + 1);
        of<uint32_t>(obj, 0x3AC) = of<uint32_t>(obj, 0x20);
        of<uint32_t>(obj, 0x3B0) = of<uint32_t>(obj, 0x24);
        next();
        of<uint32_t>(obj, 0x3B4) = of<uint32_t>(obj, 0x28);
        [[fallthrough]];
    case 2: {
        float sx = of<uint8_t>(obj, 0x2E8) == 0 ? -124.0f : 124.0f;
        float sy = -80.0f;
        if (mem.at<uint16_t>(G::players_in_game) == 1) {
            sx = 0.0f;
            sy = -80.0f;
        }
        float dx, dy, dz;
        if (pc_path()) {
            // PC: x 10 x (1/496) (its constant 0x5A70C8), stored matrix 1 copied onto the software stack, the point
            // through it (0x4F4E20, a copy of pc_matrix_transform), popped
            const float px = (sx * 10.0f) * ffrom(0x3B042108u);
            const float py = (sy * 10.0f) * ffrom(0x3B042108u);
            pc_matrix_push();
            pcm_load_camera();
            float ox, oy, oz;
            pc_matrix_transform(px, py, 10.0f, ox, oy, oz);
            of<float>(obj, 0x3B8) = ox;
            of<float>(obj, 0x3BC) = oy;
            of<float>(obj, 0x3C0) = oz;
            dx = of<float>(obj, 0x3B8) - of<float>(obj, 0x20);
            dy = of<float>(obj, 0x3BC) - of<float>(obj, 0x24);
            dz = of<float>(obj, 0x3C0) - of<float>(obj, 0x28);
            pcm_pop();
        } else {
            const float px = (sx * 10.0f) / 496.0f;
            const float py = (sy * 10.0f) / 496.0f;
            Tgp::Out o, none;
            const uint32_t one[1] = {1};
            copro(0x74, one, 1, none);
            const uint32_t v[3] = {fbits(px), fbits(py), fbits(10.0f)};
            copro(0x1A, v, 3, o);
            of<uint32_t>(obj, 0x3B8) = o.w[0];
            of<uint32_t>(obj, 0x3BC) = o.w[1];
            of<uint32_t>(obj, 0x3C0) = o.w[2];
            dx = of<float>(obj, 0x3B8) - of<float>(obj, 0x20);
            dy = of<float>(obj, 0x3BC) - of<float>(obj, 0x24);
            dz = of<float>(obj, 0x3C0) - of<float>(obj, 0x28);
            copro(0x06, nullptr, 0, none);
        }
        of<uint32_t>(obj, 0x30) = yaw_toward_a(dx, dy, dz);
        set_motion_blend(obj, 0xB5, 0, 8);
        next();
    }
        [[fallthrough]];
    case 3:
        if (!(of<int16_t>(obj, 0xF4) > 8))
            return;
        jump_arc_set(obj, of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28),
                     of<uint32_t>(obj, 0x3B8), of<uint32_t>(obj, 0x3BC), of<uint32_t>(obj, 0x3C0), 35);
        if (of<uint32_t>(obj, 0x330) == 1)
            of<uint32_t>(obj, 0x334) = 0;
        next();
        [[fallthrough]];
    case 4:
        if (!(of<float>(obj, 0x24) < mem.f32(G::floor_y)) && of<uint32_t>(obj, 0x334) == 0) {
            frog_splash_spawn(obj);
            of<uint32_t>(obj, 0x334) = 1;
        }
        if (jump_arc_step_b(obj) != 1)
            return;
        next();
        [[fallthrough]];
    case 5:
        player_hit(of<int8_t>(obj, 0x2E8), 1, 1);
        of<uint32_t>(obj, 0x50) = 20;
        set_motion_blend(obj, 0xBB, 0, 8);
        next();
        [[fallthrough]];
    case 6: {
        const int32_t n = of<int32_t>(obj, 0x50) - 1;
        of<int32_t>(obj, 0x50) = n;
        if (0 <= n)
            return;
        set_motion_blend(obj, 0xB5, 0, 8);
        next();
    }
        [[fallthrough]];
    case 7:
        if (!(of<int16_t>(obj, 0xF4) > 8))
            return;
        jump_arc_set(obj, of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28),
                     of<uint32_t>(obj, 0x3AC), of<uint32_t>(obj, 0x3B0), of<uint32_t>(obj, 0x3B4), 35);
        next();
        [[fallthrough]];
    case 8: {
        int16_t back = 0;
        if (mem.at<uint8_t>(G::chapter) == 0 && of<uint32_t>(obj, 0x330) == 1) {
            const uint8_t ev = mem.at<uint8_t>(G::event_index);
            if (ev == 9)
                back = 9;
            else if (ev == 6 || ev == 12 || ev == 14)
                back = 10;
        }
        if (back) {
            const int32_t t = of<int32_t>(obj, 0x31C);
            of<int16_t>(obj, 0x5C) = back;
            of<uint32_t>(obj, 0x334) = 0;
            of<int16_t>(obj, 0x60) = 3;
            of<int32_t>(obj, 0x31C) = t + 6;
            return;
        }
        if (jump_arc_step_b(obj) != 1)
            return;
        hop_dust_spawn(of<float>(obj, 0x20), mem.f32(G::floor_y), of<float>(obj, 0x28));
        sound_cue(P::pc ? 0x57 : 0xA0125F);
        next();
    }
        [[fallthrough]];
    case 9:
        mem.set32(G::bats_attacking, mem.u32(G::bats_attacking) - 1);
        of<int8_t>(obj, 0x2E8) = -1;
        next();
        [[fallthrough]];
    case 10:
        if (of<int16_t>(obj, 0xF4) > 59)
            of<int16_t>(obj, 0x60) = 0;
        return;
    }
}

// ---------------------------------------------------------------- the set-ups (states 6-10)

// a swimming frog drawn as a plain model: faded (draw_fade_begin 0x7D), its position and angles, scaled (0.5, 0.1,
// 0.5), then geo_send_words(0, 31, 0x7A0)
template <class P> void Game<P>::frog_swim_draw(uint32_t obj)
{
    if constexpr (P::pc) {
        // PC: the two draw-state calls are an empty routine (0x42CE70); nothing drawn with pc_no_extra_model; the
        // software matrix stack (the frog file's copies), the scale multiplied into the matrix's columns
        if (mem.u32(G::pc_no_extra_model) != 0)
            return;
        pc_matrix_push();
        pc_matrix_translate5(of<float>(obj, 0x20), of<float>(obj, 0x24), of<float>(obj, 0x28));
        pc_matrix_rotate_y(of<uint32_t>(obj, 0x30));
        pc_matrix_rotate_x(of<uint32_t>(obj, 0x2C));
        pc_matrix_rotate_b2(of<uint32_t>(obj, 0x34));
        const uint32_t m = mem.u32(G::pc_matrix_ptr);
        for (uint32_t i = 0; i < 9; ++i)
            mem.at<float>(m + 4 * i) = mem.f32(m + 4 * i) * (i % 3 == 1 ? ffrom(0x3DCCCCCDu) : 0.5f);
        geo_draw_model(of<uint32_t>(obj, 0x54));
        pcm_pop();
        return;
    }
    draw_fade_begin(0x7D);
    Tgp::Out none;
    copro(0x05, nullptr, 0, none);
    const uint32_t t[3] = {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)};
    copro(0x12, t, 3, none);
    const uint32_t ry[1] = {of<uint32_t>(obj, 0x30)};
    copro(0x15, ry, 1, none);
    const uint32_t rx[1] = {of<uint32_t>(obj, 0x2C)};
    copro(0x14, rx, 1, none);
    const uint32_t rz[1] = {of<uint32_t>(obj, 0x34)};
    copro(0x16, rz, 1, none);
    const uint32_t sc[3] = {0x3F000000u, 0x3DCCCCCDu, 0x3F000000u};
    copro(0x13, sc, 3, none);
    geo_draw_model(of<uint32_t>(obj, 0x54));
    copro(0x06, nullptr, 0, none);
    geo_send_words(0, 31, 0x7A0);
}

// the swim step: every third frame the next swim model (0x1DF..0x1E9; at the wrap the speed back to 0.4); speed -
// 0.01 along +0x78 (x) or +0x80 (z)
template <class P> void Game<P>::frog_swim_step(uint32_t obj, uint32_t speedOff, uint32_t posOff)
{
    uint32_t n = of<uint32_t>(obj, 0x338) + 1;
    float v = of<float>(obj, speedOff);
    if (int32_t(n) > 2) {
        n = 0;
        const uint32_t m = of<uint32_t>(obj, 0x54) + 1;
        of<uint32_t>(obj, 0x54) = m;
        if (int32_t(m) > 0x1E9) {
            of<uint32_t>(obj, 0x54) = 0x1DF;
            v = ffrom(0x3ECCCCCDu);
        }
    }
    v = v - ffrom(0x3C23D70Au);
    const float p = v + of<float>(obj, posOff);
    of<uint32_t>(obj, 0x338) = n;
    of<float>(obj, speedOff) = v;
    of<float>(obj, posOff) = p;
}

// a frog task of a set-up: shootable, at the table's point, its index in +0x31C
template <class P> uint32_t Game<P>::frog_task(uint32_t routine, uint32_t pt, uint32_t index)
{
    const uint32_t t = task_create(routine, uint32_t(sizeof(Obj)));
    task_clear_work(t);
    of<uint32_t>(t, 0x58) = 1;
    of<uint32_t>(t, 0x20) = mem.u32(pt);
    of<uint32_t>(t, 0x24) = mem.u32(pt + 4);
    of<uint32_t>(t, 0x28) = mem.u32(pt + 8);
    of<uint32_t>(t, 0x31C) = index;
    return t;
}

// a set-up frog's body: type 7, MO 0xBB, set-up, counted, its shadow size (+0x94 0.75, +0x90 3.5)
template <class P> void Game<P>::frog_body(uint32_t t)
{
    mem.set32(G::hit_type, uint32_t(int32_t(of<int16_t>(t, 0xCC))));
    body_init(t, int32_t(of<int16_t>(t, 0xCC)), 0xBB);
    creature_type_setup(t);
    of<uint32_t>(t, 0x94) = 0x3F400000u;
    of<uint32_t>(t, 0x90) = 0x40600000u;
    enemy_counts_add(1);
}

// 0x892B0 (set-up 0): 8 swimming frogs (points 0xCA1A0, delays 0xCA280; random swim model and phase) that glide
// across the water after their delay and leave within 2 of their end point (0xCA210)
template <class P> void Game<P>::kind69_frog_setup0_swimmers(uint32_t obj)
{
    const int32_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        for (uint32_t i = 0; i <= 7; ++i) {
            const uint32_t t = task_create(F::kind69_frog_setup0_swimmers, uint32_t(sizeof(Obj)));
            task_clear_work(t);
            of<uint32_t>(t, 0x58) = 1;
            of<uint32_t>(t, 0x20) = mem.u32(G::kind69_swim_points + i * 12);
            of<uint32_t>(t, 0x31C) = i;
            of<uint32_t>(t, 0x24) = mem.u32(G::kind69_swim_points + 4 + i * 12);
            of<uint32_t>(t, 0x30) = 0xFFFFC000u;
            of<uint32_t>(t, 0x28) = mem.u32(G::kind69_swim_points + 8 + i * 12);
            of<uint32_t>(t, 0x50) = mem.u32(G::kind69_swim_delays + i * 4);
            of<uint32_t>(t, 0x54) = (rand() & 7) + 0x1DF;
            const uint32_t ph = rand() & 3;
            of<int16_t>(t, 0x5C) = 6;
            of<int16_t>(t, 0x60) = 1;
            of<uint32_t>(t, 0x338) = ph;
        }
        return;
    }
    if (sub == 1) {
        const int32_t n = of<int32_t>(obj, 0x50) - 1;
        of<int32_t>(obj, 0x50) = n;
        if (0 <= n)
            return;
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
    } else if (sub != 2) {
        return;
    }
    frog_swim_step(obj, 0x78, 0x20);
    const uint32_t end = G::kind69_swim_ends + of<uint32_t>(obj, 0x31C) * 12;
    const float d = enemy_distance(of<float>(obj, 0x20), of<float>(obj, 0x28), mem.f32(end), mem.f32(end + 8));
    if (!(d >= 2.0f)) {
        task_exit();
        return;
    }
    frog_swim_draw(obj);
}

// 0x89600 (set-up 1): two frogs (points 0xCA2B0): the first hops (state 3, facing +z), the second sits (MO 0xB9, state
// 7) for 90 frames, then hops in steps of 14 along x toward 0xCA2D0 (arc 29 frames, jump_arc_step) and then hops
template <class P> void Game<P>::kind69_frog_setup1_pose(uint32_t obj)
{
    const int32_t sub = of<int16_t>(obj, 0x60);
    if (sub < 0 || sub > 3)
        return;
    if (sub == 0) {
        for (uint32_t i = 0; i <= 1; ++i) {
            const uint32_t t = frog_task(F::kind69_frog_frame, G::kind69_pose_points + i * 12, i);
            of<int16_t>(t, 0xCC) = 7;
            frog_body(t);
            if (i == 0) {
                of<int16_t>(t, 0x5C) = 3;
                of<int16_t>(t, 0x60) = 0;
                of<uint32_t>(t, 0x30) = 0x8000;
            } else {
                of<int16_t>(t, 0x5C) = 7;
                of<int16_t>(t, 0x60) = 1;
                of<uint32_t>(t, 0x50) = 90;
                of<uint32_t>(t, 0x30) = 0xFFFFC000u;
                set_motion(t, 0xB9);
            }
        }
        mem.set32(G::bats_attacking, 0);
        return;
    }
    if (sub == 1) {
        const int32_t n = of<int32_t>(obj, 0x50) - 1;
        of<int32_t>(obj, 0x50) = n;
        if (0 <= n)
            return;
        of<uint32_t>(obj, 0x3AC) = mem.u32(G::kind69_pose_end);
        of<uint32_t>(obj, 0x3B0) = mem.u32(G::kind69_pose_end + 4);
        of<uint32_t>(obj, 0x3B4) = mem.u32(G::kind69_pose_end + 8);
        set_motion_blend(obj, 0xBB, 0, 8);
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
    }
    if (sub <= 2) {
        const float dx = of<float>(obj, 0x3AC) - of<float>(obj, 0x20);
        const float dy = of<float>(obj, 0x3B0) - of<float>(obj, 0x24);
        const float dz = of<float>(obj, 0x3B4) - of<float>(obj, 0x28);
        const uint32_t yaw = yaw_toward_a(dx, dy, dz);
        Tgp::Out o;
        const uint32_t v[3] = {fbits(dx), fbits(dy), fbits(dz)};
        copro(0x0C, v, 3, o);
        const float tx = float(double(of<float>(obj, 0x20)) + double(ffrom(o.w[0])) * 14.0);
        of<uint32_t>(obj, 0x30) = yaw;
        of<float>(obj, 0x3A0) = tx;
        jump_arc_set(obj, of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28), fbits(tx),
                     of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28), 29);
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
    }
    if (jump_arc_step(obj) != 1)
        return;
    if (of<float>(obj, 0x20) > of<float>(obj, 0x3AC)) {
        of<int16_t>(obj, 0x60) = 2;
        return;
    }
    of<int16_t>(obj, 0x60) = 0;
    of<int16_t>(obj, 0x5C) = 3;
}

// 0x89930 (set-up 2): 3 x players frogs (points 0xCA2E0, delays 0xCA380) leap in one by one (at most 3 at a time,
// frog_count 0x52079C): after the delay an arc (60 frames) to their point (0xCA330), MO 0xB5, splash sound 0xA0110C;
// landing dust and sound, then state 4
template <class P> void Game<P>::kind69_frog_setup2_leap_in(uint32_t obj)
{
    const int32_t sub = of<int16_t>(obj, 0x60);
    if (sub < 0 || sub > 3)
        return;
    if (sub == 0) {
        const int32_t players = mem.at<int16_t>(G::players_in_game);
        mem.set32(G::frog_count, 0);
        const uint32_t n = uint32_t(players * 3);
        for (uint32_t i = 0; i < n; ++i) {
            const uint32_t t = frog_task(F::kind69_frog_frame, G::kind69_leap_in_points + i * 12, i);
            of<uint32_t>(t, 0x30) = 0;
            of<int16_t>(t, 0xCC) = 7;
            of<uint32_t>(t, 0x50) = mem.u32(G::kind69_leap_in_delays + i * 4);
            frog_body(t);
            of<int16_t>(t, 0x5C) = 8;
            mem.at<Obj>(t).routine = F::kind69_frog_setup2_leap_in;
            if (int32_t(of<uint32_t>(t, 0x31C)) > 2) {
                of<int16_t>(t, 0x60) = 1;
            } else {
                of<int16_t>(t, 0x60) = 2;
                mem.set32(G::frog_count, mem.u32(G::frog_count) + 1);
            }
        }
        return;
    }
    if (sub == 1) {
        if (int32_t(mem.u32(G::frog_count)) > 2)
            return;
        mem.set32(G::frog_count, mem.u32(G::frog_count) + 1);
        mem.at<Obj>(obj).routine = F::kind69_frog_frame;
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
    }
    if (sub <= 2) {
        const int32_t n = of<int32_t>(obj, 0x50) - 1;
        of<int32_t>(obj, 0x50) = n;
        if (0 <= n)
            return;
        mem.at<Obj>(obj).routine = F::kind69_frog_frame;
        enemy_slot_take(obj);
        const uint32_t pt = G::kind69_leap_in_ends + of<uint32_t>(obj, 0x31C) * 12;
        of<uint32_t>(obj, 0x3AC) = mem.u32(pt);
        of<uint32_t>(obj, 0x3B0) = mem.u32(pt + 4);
        of<uint32_t>(obj, 0x3B4) = mem.u32(pt + 8);
        jump_arc_set(obj, of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28),
                     of<uint32_t>(obj, 0x3AC), mem.u32(pt + 4), mem.u32(pt + 8), 60);   // 0x3C on A and PC (was 29 on A: MAME shows 60)
        set_motion_blend(obj, 0xB5, 0, 8);
        sound_cue(P::pc ? 0x1B : 0xA0110C);
        of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
    }
    if (jump_arc_step(obj) != 1)
        return;
    hop_dust_spawn(of<float>(obj, 0x20), mem.f32(G::floor_y), of<float>(obj, 0x28));
    sound_cue(P::pc ? 0x57 : 0xA0125F);
    of<int16_t>(obj, 0x60) = 0;
    of<int16_t>(obj, 0x5C) = 4;
}

// set-ups 3 / 4: the frogs swim (x: set-up 3, z: set-up 4); the first three become frogs after their delay
// (enemy slot, state 5); swimmers past the edge (x > -125 in chapter 1 event 9 / z > 250) end; after a leap they come
// back here (state 9 / 10, step 3): the arc down (splash on reaching the water), body freed, one attacker less
template <class P> void Game<P>::frog_swim_out(uint32_t obj, bool far)
{
    const uint32_t self = far ? F::kind69_frog_setup4_swim_out_far : F::kind69_frog_setup3_swim_out;
    const int32_t sub = of<int16_t>(obj, 0x60);
    if (sub < 0 || sub > 3)
        return;
    if (sub == 0) {
        uint32_t n;
        uint32_t pts, delays;
        if (far) {
            const int32_t players = mem.at<int16_t>(G::players_in_game);
            mem.set32(G::frog_count, 0);
            mem.set32(G::bats_attacking, 0);
            n = uint32_t(players + 1);
            pts = G::kind69_swim_out_far_points;
            delays = G::kind69_swim_out_far_delays;
        } else {
            mem.set32(G::frog_count, 0);
            mem.set32(G::bats_attacking, 0);
            n = 6;
            pts = G::kind69_swim_out_points;
            delays = G::kind69_swim_out_delays;
        }
        for (uint32_t i = 0; i < n; ++i) {
            const uint32_t t = frog_task(F::kind69_frog_setup3_swim_out, pts + i * 12, i);
            of<uint32_t>(t, 0x30) = far ? 0x8000u : 0xFFFFC000u;
            of<int16_t>(t, 0xCC) = 7;
            of<uint32_t>(t, 0x50) = mem.u32(delays + i * 4);
            of<uint32_t>(t, 0x54) = (rand() & 7) + 0x1DF;
            const uint32_t ph = rand() & 3;
            of<uint32_t>(t, 0x330) = 1;
            of<uint32_t>(t, 0x338) = ph;
            if (!(int32_t(of<uint32_t>(t, 0x31C)) > 2)) {
                mem.set32(G::hit_type, uint32_t(int32_t(of<int16_t>(t, 0xCC))));
                body_init(t, int32_t(of<int16_t>(t, 0xCC)), 0xBB);
                creature_type_setup(t);
                of<uint32_t>(t, 0x94) = 0x3F400000u;
                of<uint32_t>(t, 0x90) = 0x40600000u;
                enemy_counts_add(1);
            }
            of<int16_t>(t, 0x5C) = far ? 10 : 9;
            of<int16_t>(t, 0x60) = 1;
            mem.set32(G::frog_count, mem.u32(G::frog_count) + 1);
        }
        return;
    }
    if (sub == 1) {
        const int32_t n = of<int32_t>(obj, 0x50) - 1;
        of<int32_t>(obj, 0x50) = n;
        if (!(0 <= n) && !(int32_t(of<uint32_t>(obj, 0x31C)) > 2)) {
            of<int16_t>(obj, 0x60) = 2;
            return;
        }
        frog_swim_step(obj, far ? 0x80 : 0x78, far ? 0x28 : 0x20);
        frog_swim_draw(obj);
        if (far) {
            if (!(of<float>(obj, 0x28) > 250.0f))
                return;
        } else {
            if (!(of<float>(obj, 0x20) > (P::pc ? -100.0f : -125.0f)))   // PC: -100
                return;
            if (mem.at<uint8_t>(G::chapter) != 0 || mem.at<uint8_t>(G::event_index) != 9)
                return;
        }
        if (P::pc && of<uint32_t>(obj, 0xC8) != 0)   // PC: the body freed here (not after the leap back)
            body_free_parts(obj);
        if (int32_t(of<uint32_t>(obj, 0x31C)) > 5) {
            mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) - 1);
            mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) - 1);
            of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & ~1u;
            mem.at<uint8_t>(G::attack_slots + uint32_t(int32_t(of<int8_t>(obj, 0x2E7))) * 20) = 0;
        }
        task_exit();
        return;
    }
    if (sub == 2) {
        enemy_slot_take(obj);
        mem.at<Obj>(obj).routine = F::kind69_frog_frame;
        of<int16_t>(obj, 0x60) = 0;
        of<int16_t>(obj, 0x5C) = 5;
        return;
    }
    if (!(of<float>(obj, 0x24) > mem.f32(G::floor_y)) && of<uint32_t>(obj, 0x334) == 0) {
        frog_splash_spawn(obj);
        of<uint32_t>(obj, 0x334) = 1;
    }
    if (jump_arc_step_b(obj) != 1)
        return;
    if constexpr (!P::pc)
        body_free_parts(obj);
    mem.set32(G::bats_attacking, mem.u32(G::bats_attacking) - 1);
    of<int8_t>(obj, 0x2E8) = -1;
    mem.at<Obj>(obj).routine = self;
    of<int16_t>(obj, 0x60) = 1;
}

// 0x89BC0 (set-up 3): 6 frogs swimming along x (points 0xCA3B0, delays 0xCA400)
template <class P> void Game<P>::kind69_frog_setup3_swim_out(uint32_t obj) { frog_swim_out(obj, false); }

// 0x89FE0 (set-up 4): players + 1 frogs swimming along z (points 0xCA430, delays 0xCA460); their tasks run set-up 3
// (as the original: they swim along x until a leap brings them back here)
template <class P> void Game<P>::kind69_frog_setup4_swim_out_far(uint32_t obj) { frog_swim_out(obj, true); }

} // namespace hotd::port
