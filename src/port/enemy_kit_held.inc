// ---- kind 32: the held objects (arcade cluster 0x805A0..0x82E80, 0x83220 / 0x83230; PC 0x4E50CC..0x4E7498) ----
// Included by enemy_kit_zombie.cpp. Read from the arcade and PC disassembly. Each is a task of its own: the thrown
// axe (kind32_axe_release -> flying_axe_start) and the carried object (held_object_start: drums / barrel). Their kit
// record is held_object_kits[+0x32C] (0x58 bytes: +0 / +4 / +8 sounds, +0x18 the model list (model, hit points per
// stage), +0x1C.. offsets, +0x34.. routines: +0x34 shot down, +0x38 dropped, +0x3C knocked away, +0x40 released,
// +0x44 flying, +0x48 thrown, +0x4C / +0x50 hit / destroyed (with the shot box's +0x2C), +0x54 the damage side); the
// shot box held_object_shot_boxes[+0x32C] (0x34 bytes: +0 radius). PC differences: the matrix work on the software
// matrix stack (the kind 32 file's copies), a vector's angles by pc_atan2 (x16) and the table square root instead of
// coprocessor 0x83, the model drawn only without the option pc_no_extra_model (scaled by the object's scale first).

// a kit routine (held_object_kits +0x34..+0x48, the launch table, move_routines): ported ones directly
template <class P> void Game<P>::held_call(uint32_t routine, uint32_t obj)
{
    switch (routine) {
    case F::flying_axe_shot: flying_axe_shot(obj); return;
    case F::held_object_knocked_away: held_object_knocked_away(obj); return;
    case F::held_object_flight_spin: held_object_flight_spin(obj); return;
    case F::flying_axe_spin: flying_axe_spin(obj); return;
    case F::held_object_launch_arc: held_object_launch_arc(obj); return;
    case F::held_object_launch_random: held_object_launch_random(obj); return;
    case F::held_object_launch_drop: held_object_launch_drop(obj); return;
    case F::held_object_carried: held_object_carried(obj); return;
    case F::held_kit_drop1: held_kit_drop1(obj); return;
    case F::held_kit_drop2: held_kit_drop2(obj); return;
    case F::held_object_release: held_object_release(obj); return;
    case F::held_object_draw: held_object_draw(obj); return;
    case F::held_kit_barrel_drop: held_kit_barrel_drop(obj); return;
    default: call_routine(routine, obj); return;
    }
}

// a kit hit routine (held_object_kits +0x4C / +0x50) with the shot box's +0x2C (a float; on the PC's stack)
template <class P> void Game<P>::held_call_f(uint32_t routine, uint32_t obj, uint32_t v)
{
    switch (routine) {
    case F::hit_spark_spawn: hit_spark_spawn(obj, v); return;
    case F::held_object_no_hit: return;
    default: call_routine_f(routine, obj, ffrom(v)); return;
    }
}

// PC 0x4E1A6C (PC only, kind 32 file): pc_matrix_transform's copy with output pointers (stored x, y, z in turn)
template <class P> void Game<P>::pc_matrix_transform8(float x, float y, float z, uint32_t ox, uint32_t oy, uint32_t oz)
{
    float a, b, c;
    pc_matrix_transform(x, y, z, a, b, c);
    mem.at<float>(ox) = a;
    mem.at<float>(oy) = b;
    mem.at<float>(oz) = c;
}

// PC 0x4E1AD8 (PC only, kind 32 file): pc_atan2 in the arcade's angle units (x16)
template <class P> int32_t Game<P>::pc_atan2_8(float a, float b) { return int32_t(uint32_t(pc_atan2(a, b)) << 4); }

// the angles of the vector +0x394.. (A: coprocessor 0x83; PC: -atan2(y, x), -atan2(-z, |xy|))
template <class P> void Game<P>::held_vec_angles(uint32_t obj, uint32_t& a0, uint32_t& a1)
{
    if (pc_path()) {
        const float x = of<float>(obj, 0x394), y = of<float>(obj, 0x398), z = of<float>(obj, 0x39C);
        a0 = uint32_t(-pc_atan2_8(y, x));
        const float s = pc_fast_sqrt9(x * x + y * y);
        a1 = uint32_t(-pc_atan2_8(-z, s));
    } else {
        Tgp::Out o;
        const uint32_t v[3] = {of<uint32_t>(obj, 0x394), of<uint32_t>(obj, 0x398), of<uint32_t>(obj, 0x39C)};
        copro(0x83, v, 3, o);
        a0 = o.w[0];
        a1 = o.w[1];
    }
}

// one less enemy (enemy_count, enemies_alive)
template <class P> void Game<P>::held_uncount()
{
    mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) - 1);
    mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) - 1);
}

// 0x805A0 / PC 0x4E50CC (kit +0x3C): knocked away: its angles from the vector +0x394 (+0x34 / +0x30); velocity half
// a unit along the camera's heading (y discarded) and up 1.0, gravity -0.1007; 30 frames; not shootable; the
// attack slot freed; the kit's sound +8; then held_object_knocked_away_fall
template <class P> void Game<P>::held_object_knocked_away(uint32_t obj)
{
    LocalScope ls(*this);
    const uint32_t kr = G::held_object_kits + of<uint32_t>(obj, 0x32C) * 0x58;
    uint32_t a0, a1;
    held_vec_angles(obj, a0, a1);
    of<uint32_t>(obj, 0x34) = a0;
    of<uint32_t>(obj, 0x30) = a1;
    if (pc_path()) {
        pc_matrix_push();
        pc_matrix_identity8();
        pc_matrix_rotate_y7(mem.u32(G::camera_records + 0x10));
        const uint32_t y = local_alloc(4);   // esp
        pc_matrix_transform8(0.0f, 0.0f, 0.5f, obj + ob(0x78), y, obj + ob(0x80));
    } else {
        Tgp::Out none, o;
        copro(0x21, nullptr, 0, none);
        const uint32_t ry[1] = {mem.u32(G::camera_records + 0x10)};
        copro(0x15, ry, 1, none);
        const uint32_t v[3] = {0, 0, 0x3F000000u};
        copro(0x1A, v, 3, o);
        of<uint32_t>(obj, 0x78) = o.w[0];
        of<uint32_t>(obj, 0x80) = o.w[2];
        copro(0x06, nullptr, 0, none);
    }
    of<uint32_t>(obj, 0x88) = 0xBDCE4774u;   // -0.1007
    of<float>(obj, 0x7C) = 1.0f;
    of<int32_t>(obj, 0x2EC) = 30;
    of<int32_t>(obj, 0x84) = 0;
    of<int32_t>(obj, 0x8C) = 0;
    of<int16_t>(obj, 0x66) = 0;
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xFFFFFEF8u;
    if (pc_path())
        pcm_pop();
    if (of<uint8_t>(obj, 0x2E7) != 0xFF)
        enemy_attack_slot_free(obj);
    sound_cue(mem.u32(kr + 8));
    of<uint32_t>(obj, 0x0) = F::held_object_knocked_away_fall;
    held_object_knocked_away_fall(obj);
}

// 0x80700 / PC 0x4E5224: 30 frames, then fading (+0x3C4 bit 2) 30 more, then ends (uncounted). Meanwhile turning
// (+0x34 + 0x400) with its forward vector (+0x394) from its angles z / y, moved (move_routines[1]), spun (+0x320
// + 0x800), held_object_flight_spin
template <class P> void Game<P>::held_object_knocked_away_fall(uint32_t obj)
{
    bool spin = false;
    if (of<int16_t>(obj, 0x66) == 0) {
        const int32_t t = of<int32_t>(obj, 0x2EC) - 1;
        of<int32_t>(obj, 0x2EC) = t;
        if (t != 0) {
            spin = true;
        } else {
            of<int32_t>(obj, 0x2EC) = 30;
            of<uint8_t>(obj, 0x3C4) = of<uint8_t>(obj, 0x3C4) | 4;
            of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
        }
    }
    if (!spin) {
        const int32_t t = of<int32_t>(obj, 0x2EC) - 1;
        of<int32_t>(obj, 0x2EC) = t;
        if (t == 0) {
            held_uncount();
            enemy_free_extras(obj);
            task_exit();
            return;
        }
    }
    of<uint32_t>(obj, 0x34) = of<uint32_t>(obj, 0x34) + 0x400;
    if (pc_path()) {
        pc_matrix_push();
        pc_matrix_identity8();
        pc_matrix_rotate_z8(of<uint32_t>(obj, 0x34));
        pc_matrix_rotate_y7(of<uint32_t>(obj, 0x30));
        pc_matrix_transform8(1.0f, 0.0f, 0.0f, obj + ob(0x394), obj + ob(0x398), obj + ob(0x39C));
        pcm_pop();
    } else {
        Tgp::Out none, o;
        copro(0x21, nullptr, 0, none);
        const uint32_t rz[1] = {of<uint32_t>(obj, 0x34)};
        copro(0x16, rz, 1, none);
        const uint32_t ry[1] = {of<uint32_t>(obj, 0x30)};
        copro(0x15, ry, 1, none);
        const uint32_t v[3] = {0x3F800000u, 0, 0};
        copro(0x1A, v, 3, o);
        of<uint32_t>(obj, 0x394) = o.w[0];
        of<uint32_t>(obj, 0x398) = o.w[1];
        of<uint32_t>(obj, 0x39C) = o.w[2];
        copro(0x06, nullptr, 0, none);
    }
    call_routine(mem.u32(G::move_routines + 4), obj);
    of<uint32_t>(obj, 0x320) = of<uint32_t>(obj, 0x320) + 0x800;
    held_object_flight_spin(obj);
}

// 0x80840 / PC 0x4E5314 (kit 0 +0x48, the thrown axe): unless +0x334 bit 1, its spin axis from the velocity
// (spin_axis_from_velocity into +0x394; turned round when the old axis y was below 0); the axe models 0x5BC + 0..29
// in turn, +0x320 + 0x800, not hit-tested (flags bits 1 / 2 / 8 off), depth list, held_object_flight_spin
template <class P> void Game<P>::flying_axe_spin(uint32_t obj)
{
    if (((of<uint8_t>(obj, 0x334) >> 1) & 1) == 0) {
        LocalScope ls(*this);
        const float oldy = of<float>(obj, 0x398);
        const uint32_t vel = local_alloc(16);   // A: fp + 0x40; PC: esp
        mem.set32(vel, of<uint32_t>(obj, 0x78));
        mem.set32(vel + 4, of<uint32_t>(obj, 0x7C));
        mem.set32(vel + 8, of<uint32_t>(obj, 0x80));
        spin_axis_from_velocity(vel, obj + ob(0x394));
        if (oldy < 0.0f) {
            if (pc_path()) {
                of<float>(obj, 0x398) = of<float>(obj, 0x398) * -1.0f;   // the constant -1.0 at PC 0x5A7002
                of<float>(obj, 0x39C) = of<float>(obj, 0x39C) * -1.0f;
                of<float>(obj, 0x394) = of<float>(obj, 0x394) * -1.0f;
            } else {
                of<uint32_t>(obj, 0x394) = of<uint32_t>(obj, 0x394) ^ 0x80000000u;
                of<uint32_t>(obj, 0x398) = of<uint32_t>(obj, 0x398) ^ 0x80000000u;
                of<uint32_t>(obj, 0x39C) = of<uint32_t>(obj, 0x39C) ^ 0x80000000u;
            }
        }
    }
    const int32_t n = (of<int32_t>(obj, 0x2EC) + 1) % 30;
    of<uint32_t>(obj, 0x320) = of<uint32_t>(obj, 0x320) + 0x800;
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xFFFFFEF9u;
    of<int32_t>(obj, 0x2EC) = n;
    of<int32_t>(obj, 0x54) = n + 0x5BC;
    depth_list_add(obj);
    held_object_flight_spin(obj);
}

// 0x80C80 / PC 0x4E56A8 (kit +0x44): drawn at its position, turned by the angles of +0x394 (z, y), +0x320 (x) and
// half round (y 0x8000), scaled by +0x94 (unless fading on an even frame); that point in +0x38..; the same from stored
// matrix 5 into +0x98.. (its camera-space point); +0x2D4.. its position
template <class P> void Game<P>::held_object_flight_spin(uint32_t obj)
{
    uint32_t a0, a1;
    held_vec_angles(obj, a0, a1);
    const bool show = ((of<uint8_t>(obj, 0x3C4) >> 2) & 1) == 0 || (mem.at<uint8_t>(G::blink_phase) & 1) != 0;
    if constexpr (P::pc) {
        pc_matrix_push();
        pc_matrix_translate8(of<float>(obj, 0x20), of<float>(obj, 0x24), of<float>(obj, 0x28));
        pc_matrix_rotate_z8(a0);
        pc_matrix_rotate_y7(a1);
        pc_matrix_rotate_x8(of<uint32_t>(obj, 0x320));
        pc_matrix_rotate_y7(0x8000);
        uint32_t m = mem.u32(G::pc_matrix_ptr);
        of<uint32_t>(obj, 0x38) = mem.u32(m + 0x24);
        of<uint32_t>(obj, 0x3C) = mem.u32(m + 0x28);
        of<uint32_t>(obj, 0x40) = mem.u32(m + 0x2C);
        if (mem.u32(G::pc_no_extra_model) == 0) {
            const float s = of<float>(obj, 0x94);
            for (uint32_t i = 0; i < 9; ++i)
                mem.at<float>(m + 4 * i) = mem.at<float>(m + 4 * i) * s;
            if (show)
                geo_draw_model(of<uint32_t>(obj, 0x54));
        }
        pcm_pop();
        pc_matrix_push();
        m = mem.u32(G::pc_matrix_ptr);
        for (uint32_t i = 0; i < 12; ++i)
            mem.set32(m + 4 * i, mem.u32(G::pc_stored_matrices + 0xF0 + 4 * i));
        pc_matrix_translate8(of<float>(obj, 0x20), of<float>(obj, 0x24), of<float>(obj, 0x28));
        pc_matrix_rotate_z8(a0);
        pc_matrix_rotate_y7(a1);
        pc_matrix_rotate_x8(of<uint32_t>(obj, 0x320));
        pc_matrix_rotate_y7(0x8000);
        m = mem.u32(G::pc_matrix_ptr);
        of<uint32_t>(obj, 0x98) = mem.u32(m + 0x24);
        mem.set32(G::pc_matrix_ptr, m - 0x30);
        of<uint32_t>(obj, 0x9C) = mem.u32(m + 0x28);
        of<uint32_t>(obj, 0xA0) = mem.u32(m + 0x2C);
    } else {
        Tgp::Out none, o;
        copro(0x05, nullptr, 0, none);
        const uint32_t t[3] = {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)};
        copro(0x12, t, 3, none);
        const uint32_t rz[1] = {a0};
        copro(0x16, rz, 1, none);
        const uint32_t ry[1] = {a1};
        copro(0x15, ry, 1, none);
        const uint32_t rx[1] = {of<uint32_t>(obj, 0x320)};
        copro(0x14, rx, 1, none);
        const uint32_t half[1] = {0x8000};
        copro(0x15, half, 1, none);
        copro(0x20, nullptr, 0, o);
        of<uint32_t>(obj, 0x38) = o.w[0];
        of<uint32_t>(obj, 0x3C) = o.w[1];
        of<uint32_t>(obj, 0x40) = o.w[2];
        const uint32_t s = of<uint32_t>(obj, 0x94);
        const uint32_t sc[3] = {s, s, s};
        copro(0x13, sc, 3, none);
        if (show) {
            // the geometrizer: register 0x800160 and the register at the ROM pointer 0x790 (0x804000) around the draw
            const uint32_t reg = mem.u32(0x790);
            hw_write32(0x800160, 0x1616);
            hw_write32(reg, 0);
            geo_draw_model(of<uint32_t>(obj, 0x54));
            hw_write32(0x800160, 0x1616);
            hw_write32(reg, mem.u32(0x51E8C0));
        }
        copro(0x06, nullptr, 0, none);
        const uint32_t slot[1] = {5};
        copro(0x74, slot, 1, none);
        copro(0x12, t, 3, none);
        copro(0x16, rz, 1, none);
        copro(0x15, ry, 1, none);
        const uint32_t rx2[1] = {of<uint32_t>(obj, 0x320)};
        copro(0x14, rx2, 1, none);
        copro(0x15, half, 1, none);
        Tgp::Out v;
        copro(0x20, nullptr, 0, v);
        of<uint32_t>(obj, 0x98) = v.w[0];
        of<uint32_t>(obj, 0x9C) = v.w[1];
        copro(0x06, nullptr, 0, none);
        of<uint32_t>(obj, 0x2D4) = of<uint32_t>(obj, 0x20);
        of<uint32_t>(obj, 0x2D8) = of<uint32_t>(obj, 0x24);
        of<uint32_t>(obj, 0x2DC) = of<uint32_t>(obj, 0x28);
        of<uint32_t>(obj, 0xA0) = v.w[2];
        return;
    }
    of<uint32_t>(obj, 0x2D4) = of<uint32_t>(obj, 0x20);
    of<uint32_t>(obj, 0x2D8) = of<uint32_t>(obj, 0x24);
    of<uint32_t>(obj, 0x2DC) = of<uint32_t>(obj, 0x28);
}

// 0x80F30 / PC 0x4E58AC (the carried object's task): kit +0x32C; no attack slot, shootable, type -1; its shot box
// radius, scale 1.0; the kit's first model and hit points (model list +0 / +2); counted; a 0x48-byte block with the
// shot box's +0x14..+0x2B (+0x34 0); then held_object_frame. PC: pc_wide_slot_kind 7 before the block.
template <class P> void Game<P>::held_object_start(uint32_t obj)
{
    const uint32_t k = of<uint32_t>(obj, 0x32C);
    of<uint8_t>(obj, 0x2E7) = 0xFF;
    of<int16_t>(obj, 0x68) = 2;
    of<int16_t>(obj, 0x6A) = 1;
    of<int32_t>(obj, 0x58) = 1;
    of<int16_t>(obj, 0xCC) = -1;
    of<int16_t>(obj, 0x6C) = 0;
    const uint32_t box = G::held_object_shot_boxes + k * 0x34;
    of<float>(obj, 0x94) = 1.0f;
    of<uint32_t>(obj, 0x90) = mem.u32(box);
    const uint32_t list = mem.u32(G::held_object_kits + k * 0x58 + 0x18);
    of<int32_t>(obj, 0x54) = mem.at<int16_t>(list);
    of<int16_t>(obj, 0x70) = mem.at<int16_t>(list + 2);
    of<int32_t>(obj, 0xC8) = 0;
    of<int32_t>(obj, 0x328) = 0;
    of<uint8_t>(obj, 0x3C4) = of<uint8_t>(obj, 0x3C4) & 0xFB;
    obj_clear_37c(obj);
    if constexpr (P::pc)
        mem.set32(G::pc_wide_slot_kind, 7);   // PC only
    mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) + 1);
    mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) + 1);
    const uint32_t buf = mem_alloc(0x48);
    of<uint32_t>(obj, 0x37C) = buf;
    for (uint32_t i = 0; i < 6; ++i)
        mem.set32(buf + 4 * i, mem.u32(box + 0x14 + 4 * i));
    mem.set32(buf + 0x34, 0);
    of<uint32_t>(obj, 0x0) = F::held_object_frame;
    held_object_frame(obj);
}

// PC 0x4E1B54 (PC only, kind 32 file): a copy of pc_vec_angles (the file's table square root)
template <class P> void Game<P>::pc_vec_angles8(float x, float y, float z, uint32_t& yaw, uint32_t& pitch)
{
    const float h = pc_fast_sqrt9(z * z + x * x);
    pitch = 0u - (uint32_t(pc_atan2(-y, h)) << 4);
    yaw = 0u - (uint32_t(pc_atan2(x, z)) << 4);
    fixAngles(yaw, pitch);
}

// 0x83230 / PC 0x4E509C (the thrown axe's task): counted; flying_axe_launch
template <class P> void Game<P>::flying_axe_start(uint32_t obj)
{
    mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) + 1);
    mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) + 1);
    of<uint32_t>(obj, 0x0) = F::flying_axe_launch;
    flying_axe_launch(obj);
}

// 0x81300 / PC 0x4E5BE0 (also the carried object's throw): +0xA0 65535; the kit's release sound; the launch routine
// of held_object_launch_table[action][record +1] (matrix pushed around it); a random turn -0x1000..0xFFF (kits 1-4
// +0x30, kit 5 +0x2C); then flying_axe_frame
template <class P> void Game<P>::flying_axe_launch(uint32_t obj)
{
    const uint32_t kr = G::held_object_kits + of<uint32_t>(obj, 0x32C) * 0x58;
    const uint32_t r = of<uint32_t>(obj, 0x368);
    of<uint32_t>(obj, 0xA0) = 0x477FFF00u;   // 65535.0
    sound_cue(mem.u32(kr));
    const uint32_t list = mem.u32(G::held_object_launch_table + uint32_t(int32_t(of<int16_t>(obj, 0x64))) * 4);
    const uint32_t launch = mem.u32(list + uint32_t(int32_t(mem.at<int8_t>(r + 1))) * 4);
    if (pc_path()) {
        pc_matrix_push();
        held_call(launch, obj);
        pcm_pop();
    } else {
        Tgp::Out none;
        copro(0x05, nullptr, 0, none);
        held_call(launch, obj);
        copro(0x06, nullptr, 0, none);
    }
    const int32_t turn = int32_t(rand() & 0x1FFF) - 0x1000;
    const int32_t k = of<int32_t>(obj, 0x32C);
    if (k >= 1) {
        if (k <= 4)
            of<int32_t>(obj, 0x30) = of<int32_t>(obj, 0x30) + turn;
        else if (k == 5)
            of<int32_t>(obj, 0x2C) = of<int32_t>(obj, 0x2C) + turn;
    }
    of<uint32_t>(obj, 0x0) = F::flying_axe_frame;
    flying_axe_frame(obj);
}

// 0x813D0 / PC 0x4E5C84 (launch): from stored matrix 6 (loaded, not pushed): a random point ahead (x (r - 127.5) *
// 0.023532, y (r - 127.5) * 0.0182183 (PC one unit lower: 0x3C953E97), z 10); gravity and speed from
// held_launch_arc_params by record +0xC bits 3-5 (all set: the record's +0x30 / +0x34); velocity_for_arc_speed
template <class P> void Game<P>::held_object_launch_arc(uint32_t obj)
{
    LocalScope ls(*this);
    const uint32_t r = of<uint32_t>(obj, 0x368);
    const uint32_t m = (uint32_t(int32_t(mem.at<int16_t>(r + 0xC))) >> 3) & 7;
    const uint32_t pt = local_alloc(16);
    if (pc_path()) {
        pcm_load_stored6();
        const float x = (float(int32_t(rand() & 0xFF)) + -127.5f) * ffrom(0x3CC0C62Eu);
        const float y = (float(int32_t(rand() & 0xFF)) + -127.5f) * ffrom(0x3C953E97u);
        pc_matrix_transform8(x, y, 10.0f, pt, pt + 4, pt + 8);
    } else {
        Tgp::Out none, o;
        const uint32_t slot[1] = {6};
        copro(0x73, slot, 1, none);
        const float x = (float(int32_t(rand() & 0xFF)) - 127.5f) * ffrom(0x3CC0C62Eu);
        const float y = (float(int32_t(rand() & 0xFF)) - 127.5f) * ffrom(0x3C953E98u);
        const uint32_t v[3] = {fbits(x), fbits(y), 0x41200000u};
        copro(0x1A, v, 3, o);
        mem.set32(pt, o.w[0]);
        mem.set32(pt + 4, o.w[1]);
        mem.set32(pt + 8, o.w[2]);
    }
    uint32_t g, s;
    if ((mem.at<uint16_t>(r + 0xC) & 0x38) == 0x38) {
        g = mem.u32(r + 0x30);
        s = mem.u32(r + 0x34);
    } else {
        g = mem.u32(G::held_launch_arc_params + m * 8);
        s = mem.u32(G::held_launch_arc_params + m * 8 + 4);
    }
    const uint32_t frames = local_alloc(4);
    velocity_for_arc_speed(obj, obj + ob(0x20), pt, g, s, frames);
}

// 0x814E0 / PC 0x4E5D74 (launch): from stored matrix 6: the point (x from held_launch_random_xy by record +1 and
// +0x334 bit 0; y from there for record +1 5, else (r - 127.5) / 42.5; z 10); gravity and speed from
// held_launch_random_params; velocity_for_arc_speed
template <class P> void Game<P>::held_object_launch_random(uint32_t obj)
{
    LocalScope ls(*this);
    const uint32_t r = of<uint32_t>(obj, 0x368);
    const int32_t i = mem.at<int8_t>(r + 1);
    const uint32_t row = G::held_launch_random_xy + uint32_t(i) * 16 + (of<uint32_t>(obj, 0x334) & 1) * 8;
    const uint32_t pt = local_alloc(16);
    if (pc_path()) {
        pcm_load_stored6();
    } else {
        Tgp::Out none;
        const uint32_t slot[1] = {6};
        copro(0x73, slot, 1, none);
    }
    const float x = mem.f32(row);
    float y;
    if (i == 5)
        y = mem.f32(row + 4);
    else if (pc_path())
        y = (float(int32_t(rand() & 0xFF)) + -127.5f) * ffrom(0x3CC0C0C1u);
    else
        y = (float(int32_t(rand() & 0xFF)) - 127.5f) * ffrom(0x3CC0C0C1u);
    if (pc_path()) {
        pc_matrix_transform8(x, y, 10.0f, pt, pt + 4, pt + 8);
    } else {
        Tgp::Out o;
        const uint32_t v[3] = {fbits(x), fbits(y), 0x41200000u};
        copro(0x1A, v, 3, o);
        mem.set32(pt, o.w[0]);
        mem.set32(pt + 4, o.w[1]);
        mem.set32(pt + 8, o.w[2]);
    }
    const uint32_t frames = local_alloc(4);
    velocity_for_arc_speed(obj, obj + ob(0x20), pt, mem.u32(G::held_launch_random_params + uint32_t(i) * 8),
                           mem.u32(G::held_launch_random_params + uint32_t(i) * 8 + 4), frames);
}

// 0x815E0 / PC 0x4E5E48 (launch): stored matrix 6 without its translation; the velocity (0, 0, held_launch_drop_z)
// through it; no gravity
template <class P> void Game<P>::held_object_launch_drop(uint32_t obj)
{
    if (pc_path()) {
        pcm_load_stored6();
        pcm_set(9, 0.0f);
        pcm_set(10, 0.0f);
        pcm_set(11, 0.0f);
        pc_matrix_transform8(0.0f, 0.0f, mem.f32(G::held_launch_drop_z), obj + ob(0x78), obj + ob(0x7C), obj + ob(0x80));
        of<int32_t>(obj, 0x84) = 0;
        of<int32_t>(obj, 0x88) = 0;
        of<int32_t>(obj, 0x8C) = 0;
    } else {
        Tgp::Out none, o;
        const uint32_t slot[1] = {6};
        copro(0x73, slot, 1, none);
        const uint32_t zero[3] = {0, 0, 0};
        copro(0x88, zero, 3, none);
        const uint32_t v[3] = {0, 0, mem.u32(G::held_launch_drop_z)};
        copro(0x1A, v, 3, o);
        of<uint32_t>(obj, 0x78) = o.w[0];
        of<uint32_t>(obj, 0x7C) = o.w[1];
        of<int32_t>(obj, 0x84) = 0;
        of<int32_t>(obj, 0x88) = 0;
        of<int32_t>(obj, 0x8C) = 0;
        of<uint32_t>(obj, 0x80) = o.w[2];
    }
}

// a hit on a thrown / carried object (held_object_shot_check: -1 destroyed, 1 hit): the kit's +0x50 / +0x4C
// routine with the shot box's +0x2C and its sound (+8 / +4); the check's result
template <class P> int32_t Game<P>::held_hit(uint32_t obj, uint32_t kr, uint32_t box)
{
    const int32_t h = held_object_shot_check(obj);
    if (h == -1) {
        held_call_f(mem.u32(kr + 0x50), obj, mem.u32(box + 0x2C));
        sound_cue(mem.u32(kr + 8));
    } else if (h == 1) {
        held_call_f(mem.u32(kr + 0x4C), obj, mem.u32(box + 0x2C));
        sound_cue(mem.u32(kr + 4));
    }
    return h;
}

// its camera-space depth below 10 (PC: the bits compared as integers)
template <class P> bool Game<P>::held_near(uint32_t obj, uint32_t a)
{
    if (pc_path())
        return of<int32_t>(obj, a) < 0x41200000;
    else
        return of<float>(obj, a) < 10.0f;
}

// at the camera: projectile_hit_player at the camera-space point (the kit's damage side +0x54); no hit -> ends
// (attack slot freed, uncounted); hit -> the kit's +0x34 routine
template <class P> void Game<P>::held_reach_camera(uint32_t obj, uint32_t kr, uint32_t point)
{
    if (projectile_hit_player(point, 1, mem.at<int16_t>(kr + 0x54)) == 0) {
        if (of<uint8_t>(obj, 0x2E7) != 0xFF)
            enemy_attack_slot_free(obj);
        held_uncount();
        enemy_free_extras(obj);
        task_exit();
        return;
    }
    held_call(mem.u32(kr + 0x34), obj);
}

// 0x81670 / PC 0x4E5EC4 (any thrown object): moved (move_routines[1]); its camera-space point from stored matrix 5
// (+0x98..); hits (held_hit; destroyed -> the kit's +0x3C); within depth 10: clipped to depth 10 from the old point
// (point_clip_to_depth), back to world space by stored matrix 6, held_reach_camera; else the kit's +0x48
template <class P> void Game<P>::flying_axe_frame(uint32_t obj)
{
    LocalScope ls(*this);
    const uint32_t k = of<uint32_t>(obj, 0x32C);
    const uint32_t kr = G::held_object_kits + k * 0x58, box = G::held_object_shot_boxes + k * 0x34;
    const uint32_t old = local_alloc(16);   // A: fp + 0x40; PC: esp
    mem.set32(old, of<uint32_t>(obj, 0x98));
    mem.set32(old + 4, of<uint32_t>(obj, 0x9C));
    mem.set32(old + 8, of<uint32_t>(obj, 0xA0));
    call_routine(mem.u32(G::move_routines + 4), obj);
    if (pc_path()) {
        pc_matrix_push();
        pcm_load_stored5();
        pc_matrix_transform8(of<float>(obj, 0x20), of<float>(obj, 0x24), of<float>(obj, 0x28), obj + ob(0x98),
                             obj + ob(0x9C), obj + ob(0xA0));
        pcm_pop();
    } else {
        Tgp::Out none, o;
        const uint32_t slot[1] = {5};
        copro(0x74, slot, 1, none);
        const uint32_t v[3] = {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)};
        copro(0x1A, v, 3, o);
        of<uint32_t>(obj, 0x98) = o.w[0];
        of<uint32_t>(obj, 0x9C) = o.w[1];
        copro(0x06, nullptr, 0, none);
        of<uint32_t>(obj, 0xA0) = o.w[2];
    }
    if (held_hit(obj, kr, box) == -1) {
        held_call(mem.u32(kr + 0x3C), obj);
        return;
    }
    if (!held_near(obj, 0xA0)) {
        held_call(mem.u32(kr + 0x48), obj);
        return;
    }
    point_clip_to_depth(obj + ob(0x98), old, 10.0f);
    if (pc_path()) {
        pc_matrix_push();
        pcm_load_stored6();
        pc_matrix_transform8(of<float>(obj, 0x98), of<float>(obj, 0x9C), of<float>(obj, 0xA0), obj + ob(0x20),
                             obj + ob(0x24), obj + ob(0x28));
        pcm_pop();
    } else {
        Tgp::Out none, o;
        const uint32_t slot[1] = {6};
        copro(0x74, slot, 1, none);
        const uint32_t v[3] = {of<uint32_t>(obj, 0x98), of<uint32_t>(obj, 0x9C), of<uint32_t>(obj, 0xA0)};
        copro(0x1A, v, 3, o);
        of<uint32_t>(obj, 0x20) = o.w[0];
        of<uint32_t>(obj, 0x24) = o.w[1];
        copro(0x06, nullptr, 0, none);
        of<uint32_t>(obj, 0x28) = o.w[2];
    }
    held_reach_camera(obj, kr, obj + ob(0x98));
}

// 0x81890 / PC 0x4E60BC (kit +0x34, shot down): 60 frames fading; the attack slot freed; the axe (kit 0) turns to
// the camera's heading (record +0xC bits 10-12 below 2: the camera's y angle, negated unless the axis y > 0; else
// its x angle) in +0x320; then flying_axe_shot_fall
template <class P> void Game<P>::flying_axe_shot(uint32_t obj)
{
    const uint32_t r = of<uint32_t>(obj, 0x368);
    of<int32_t>(obj, 0x2EC) = 60;
    of<uint8_t>(obj, 0x3C4) = of<uint8_t>(obj, 0x3C4) | 4;
    if (of<uint8_t>(obj, 0x2E7) != 0xFF)
        enemy_attack_slot_free(obj);
    if (of<int32_t>(obj, 0x32C) == 0) {
        const uint32_t m = (uint32_t(int32_t(mem.at<int16_t>(r + 0xC))) >> 10) & 7;
        uint32_t a;
        if (m > 1) {
            a = mem.u32(G::camera_records + 0xC);
        } else {
            a = mem.u32(G::camera_records + 0x10);
            if (!(of<float>(obj, 0x398) > 0.0f))
                a = 0u - a;
        }
        of<uint32_t>(obj, 0x320) = a;
    }
    of<uint32_t>(obj, 0x0) = F::flying_axe_shot_fall;
    flying_axe_shot_fall(obj);
}

// 0x81920 / PC 0x4E6144: counts +0x2EC down: at the end uncounted, ends; else the kit's +0x44
template <class P> void Game<P>::flying_axe_shot_fall(uint32_t obj)
{
    const int32_t t = of<int32_t>(obj, 0x2EC);
    of<int32_t>(obj, 0x2EC) = t - 1;
    if (t == 1) {
        held_uncount();
        enemy_free_extras(obj);
        task_exit();
        return;
    }
    held_call(mem.u32(G::held_object_kits + of<uint32_t>(obj, 0x32C) * 0x58 + 0x44), obj);
}

// 0x81990 / PC 0x4E61B0 (the carried object thrown at the camera): velocity toward the camera over 120 frames
// (PC: times 1 / 120), up held_throw_vy, gravity -0.1361; +0x320 its x angle; then held_object_throw_flight
template <class P> void Game<P>::held_object_throw_at_camera(uint32_t obj)
{
    of<int32_t>(obj, 0x2EC) = 0;
    of<int32_t>(obj, 0x84) = 0;
    of<int32_t>(obj, 0x8C) = 0;
    of<uint32_t>(obj, 0x88) = 0xBE0B60B6u;   // -0.1361
    of<uint32_t>(obj, 0x320) = of<uint32_t>(obj, 0x2C);
    if (pc_path()) {
        const float c = ffrom(0x3C088889u);   // 1 / 120
        of<float>(obj, 0x78) = (mem.f32(G::camera_pos) - of<float>(obj, 0x20)) * c;
        of<uint32_t>(obj, 0x7C) = mem.u32(G::held_throw_vy);
        of<float>(obj, 0x80) = (mem.f32(G::camera_pos + 8) - of<float>(obj, 0x28)) * c;
    } else {
        of<float>(obj, 0x78) = (mem.f32(G::camera_pos) - of<float>(obj, 0x20)) / 120.0f;
        of<uint32_t>(obj, 0x7C) = mem.u32(G::held_throw_vy);
        of<float>(obj, 0x80) = (mem.f32(G::camera_pos + 8) - of<float>(obj, 0x28)) / 120.0f;
    }
    of<uint32_t>(obj, 0x0) = F::held_object_throw_flight;
    held_object_throw_flight(obj);
}

// 0x81A10 / PC 0x4E6228: hits (destroyed -> the kit's +0x3C); its camera-space depth (+0x40) below 10:
// held_reach_camera at +0x38; else the kit's +0x48
template <class P> void Game<P>::held_object_throw_flight(uint32_t obj)
{
    const uint32_t k = of<uint32_t>(obj, 0x32C);
    const uint32_t kr = G::held_object_kits + k * 0x58, box = G::held_object_shot_boxes + k * 0x34;
    if (held_hit(obj, kr, box) == -1) {
        held_call(mem.u32(kr + 0x3C), obj);
        return;
    }
    if (!held_near(obj, 0x40)) {
        held_call(mem.u32(kr + 0x48), obj);
        return;
    }
    held_reach_camera(obj, kr, obj + ob(0x38));
}

// 0x81DB0 / PC 0x4E6730: -1 destroyed (no hit points: not hit-tested any more), 1 hit, 0 nothing. Each player's hit
// (flags bits 1 / 2, while flags bit 8): hit points (+0x70) - 1; at 0 the next stage of the kit's model list (+0x328;
// its +2 the hit points, below 0 destroyed). return_false (never): stages to the end. The stage's model (+0 of the
// entry) except for the axe.
template <class P> int32_t Game<P>::held_object_shot_check(uint32_t obj)
{
    const uint32_t list = mem.u32(G::held_object_kits + of<uint32_t>(obj, 0x32C) * 0x58 + 0x18);
    const int16_t hp0 = of<int16_t>(obj, 0x70);
    if (hp0 <= 0) {
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xFFFFFEF9u;
        return -1;
    }
    const int32_t stage0 = of<int32_t>(obj, 0x328);
    bool alive = true;
    int32_t stage = stage0;
    if ((of<uint16_t>(obj, 0x58) >> 8) & 1) {
        const uint32_t f = of<uint32_t>(obj, 0x58);
        for (uint32_t bit = 1; bit <= 2; ++bit) {
            if (((f >> bit) & 1) == 0 || !alive)
                continue;
            const int16_t hp = int16_t(of<int16_t>(obj, 0x70) - 1);
            of<int16_t>(obj, 0x70) = hp;
            if (hp > 0)
                continue;
            ++stage;
            const int16_t n = mem.at<int16_t>(list + 2 + uint32_t(stage) * 4);
            of<int16_t>(obj, 0x70) = n;
            if (n < 0)
                alive = false;
        }
        of<uint32_t>(obj, 0x58) = f & 0xFFFFFEF9u;
    }
    of<int32_t>(obj, 0x328) = stage;
    if (return_false() != 0) {
        while (alive) {
            ++stage;
            const int16_t n = mem.at<int16_t>(list + 2 + uint32_t(stage) * 4);
            of<int16_t>(obj, 0x70) = n;
            if (n < 0)
                alive = false;
        }
        of<int32_t>(obj, 0x328) = stage;
    }
    if (of<int32_t>(obj, 0x32C) != 0)
        of<int32_t>(obj, 0x54) = mem.at<int16_t>(list + uint32_t(of<int32_t>(obj, 0x328)) * 4);
    if (!alive)
        return -1;
    if (of<int32_t>(obj, 0x328) == stage0 && of<int16_t>(obj, 0x70) == hp0)
        return 0;
    return 1;
}

// 0x82FE0 / PC 0x4E7498 (kit +0x4C / +0x50 of the drums): a hit_spark task at the object (position and angles),
// model 0x8A, +0x328 0xA1, +0x33C the size (the shot box's +0x2C), the kit
template <class P> void Game<P>::hit_spark_spawn(uint32_t obj, uint32_t size)
{
    const uint32_t t = task_create(F::hit_spark, uint32_t(sizeof(Obj)));
    for (uint32_t a = 0x20; a < 0x38; a += 4)
        of<uint32_t>(t, a) = of<uint32_t>(obj, a);
    of<int32_t>(t, 0x54) = 0x8A;
    of<uint32_t>(t, 0x33C) = size;
    of<int32_t>(t, 0x328) = 0xA1;
    of<uint32_t>(t, 0x32C) = of<uint32_t>(obj, 0x32C);
}

// 0x81030 / PC 0x4E59C4 (the carried object, per frame; +0x18 its carrier): the carrier gone (hit points <= 0) or
// letting go (+0x320 bit 1): the kit's +0x40; dropping (bit 3): the kit's +0x38; throwing (bit 0, while it has hit
// points): flying_axe_launch, or held_object_throw_at_camera by record +0xC bit 2. Else hits (destroyed: the carrier's
// +0x324 bit 0); placed between the carrier's hand nodes (+0x238 and +0x268 through stored matrix 1) plus the kit's
// offset, turned along them (+0x2C / +0x30 the angles plus the kit's, +0x34 the kit's); depth list; the kit's +0x44
template <class P> void Game<P>::held_object_frame(uint32_t obj)
{
    LocalScope ls(*this);
    const uint32_t owner = of<uint32_t>(obj, 0x18);
    const uint32_t k = of<uint32_t>(obj, 0x32C);
    const uint32_t kr = G::held_object_kits + k * 0x58, box = G::held_object_shot_boxes + k * 0x34;
    const uint32_t r = of<uint32_t>(obj, 0x368);
    if (of<int16_t>(owner, 0x70) <= 0 || ((of<uint8_t>(owner, 0x320) >> 1) & 1) != 0) {
        held_call(mem.u32(kr + 0x40), obj);
        return;
    }
    if ((of<uint8_t>(owner, 0x320) >> 3) & 1) {
        held_call(mem.u32(kr + 0x38), obj);
        return;
    }
    if (of<int16_t>(obj, 0x70) >= 0) {
        if (of<uint8_t>(owner, 0x320) & 1) {
            if ((mem.at<uint8_t>(r + 0xC) >> 2) & 1)
                held_object_throw_at_camera(obj);
            else
                flying_axe_launch(obj);
            return;
        }
        if (held_hit(obj, kr, box) == -1)
            of<uint32_t>(owner, 0x324) = of<uint32_t>(owner, 0x324) | 1;
    }
    const uint32_t a = local_alloc(16), b = local_alloc(16);
    uint32_t yaw, pitch;
    if (pc_path()) {
        pc_matrix_push();
        pcm_load_camera();
        pc_matrix_transform8(of<float>(owner, 0x238), of<float>(owner, 0x23C), of<float>(owner, 0x240), a, a + 4, a + 8);
        pc_matrix_transform8(of<float>(owner, 0x268), of<float>(owner, 0x26C), of<float>(owner, 0x270), b, b + 4, b + 8);
        pcm_pop();
        pc_vec_angles8(mem.f32(b) - mem.f32(a), mem.f32(b + 4) - mem.f32(a + 4), mem.f32(b + 8) - mem.f32(a + 8), yaw,
                       pitch);
    } else {
        Tgp::Out none, o, o2;
        const uint32_t slot[1] = {1};
        copro(0x74, slot, 1, none);
        const uint32_t va[3] = {of<uint32_t>(owner, 0x238), of<uint32_t>(owner, 0x23C), of<uint32_t>(owner, 0x240)};
        copro(0x1A, va, 3, o);
        std::memcpy(mem.ptr(a), o.w, 12);
        const uint32_t vb[3] = {of<uint32_t>(owner, 0x268), of<uint32_t>(owner, 0x26C), of<uint32_t>(owner, 0x270)};
        copro(0x1A, vb, 3, o2);
        std::memcpy(mem.ptr(b), o2.w, 12);
        copro(0x06, nullptr, 0, none);
        const float d[3] = {mem.f32(b) - mem.f32(a), mem.f32(b + 4) - mem.f32(a + 4), mem.f32(b + 8) - mem.f32(a + 8)};
        const uint32_t dv[3] = {fbits(d[0]), fbits(d[1]), fbits(d[2])};
        Tgp::Out an;
        copro(0x0F, dv, 3, an);
        yaw = an.w[0];
        pitch = an.w[1];
        fixAngles(yaw, pitch);
    }
    of<float>(obj, 0x20) = (mem.f32(a) + mem.f32(b)) * 0.5f + mem.f32(kr + 0x1C);
    of<float>(obj, 0x24) = (mem.f32(a + 4) + mem.f32(b + 4)) * 0.5f + mem.f32(kr + 0x20);
    of<float>(obj, 0x28) = (mem.f32(a + 8) + mem.f32(b + 8)) * 0.5f + mem.f32(kr + 0x24);
    of<uint32_t>(obj, 0x2C) = pitch + mem.u32(kr + 0x28);
    of<uint32_t>(obj, 0x30) = yaw + mem.u32(kr + 0x2C);
    of<uint32_t>(obj, 0x34) = mem.u32(kr + 0x30);
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xFFFFFEF9u;
    depth_list_add(obj);
    held_call(mem.u32(kr + 0x44), obj);
}

// the angles +0x34 / +0x30 / +0x2C (z, y, x) turned by one more rotation (y or x by `angle`; y last for 0x15) and
// read back (A: coprocessor 0x86 / 0x71; PC, and PC Mode on its own stack: the matrix stack and pc_matrix_angles8)
template <class P> void Game<P>::held_turn(uint32_t obj, bool aboutY, uint32_t angle)
{
    if (pc_path()) {
        pc_matrix_push();
        pc_matrix_identity8();
        pc_matrix_rotate_z8(of<uint32_t>(obj, 0x34));
        pc_matrix_rotate_y7(of<uint32_t>(obj, 0x30));
        pc_matrix_rotate_x8(of<uint32_t>(obj, 0x2C));
        if (aboutY)
            pc_matrix_rotate_y7(angle);
        else
            pc_matrix_rotate_x8(angle);
        pc_matrix_angles8(obj + ob(0x2C), obj + ob(0x30), obj + ob(0x34));
        pcm_pop();
    } else {
        Tgp::Out none, o;
        copro(0x21, nullptr, 0, none);
        const uint32_t r[3] = {of<uint32_t>(obj, 0x34), of<uint32_t>(obj, 0x30), of<uint32_t>(obj, 0x2C)};
        copro(0x86, r, 3, none);
        const uint32_t a[1] = {angle};
        copro(aboutY ? 0x15 : 0x14, a, 1, none);
        copro(0x71, nullptr, 0, o);
        of<uint32_t>(obj, 0x34) = o.w[0];
        of<uint32_t>(obj, 0x30) = o.w[1];
        of<uint32_t>(obj, 0x2C) = o.w[2];
        copro(0x06, nullptr, 0, none);
    }
}

// 0x81F30 / PC 0x4E6880 (kits 1-5 +0x48, the carried object thrown): record +0xC bit 2: moved by x / z velocity;
// tumbling (y 0x400 for the drums and the barrel, else x); with bit 2 also falling (gravity +0x88) and bouncing off
// the floor plus the shot box's +4 (vertical speed times held_bounce_factor, +0x2EC counts the bounces); not
// hit-tested; depth list; held_object_draw
template <class P> void Game<P>::held_object_carried(uint32_t obj)
{
    const uint32_t r = of<uint32_t>(obj, 0x368);
    const int32_t k = of<int32_t>(obj, 0x32C);
    const uint32_t box = G::held_object_shot_boxes + uint32_t(k) * 0x34;
    const bool move = ((mem.at<uint8_t>(r + 0xC) >> 2) & 1) != 0;
    if (move) {
        of<float>(obj, 0x20) = of<float>(obj, 0x20) + of<float>(obj, 0x78);
        of<float>(obj, 0x28) = of<float>(obj, 0x28) + of<float>(obj, 0x80);
    }
    held_turn(obj, k >= 1 && k <= 5, 0x400);
    if (move) {
        const float y0 = of<float>(obj, 0x24);
        float vy = of<float>(obj, 0x7C) + of<float>(obj, 0x88);
        float y = y0 + vy;
        of<float>(obj, 0x7C) = vy;
        of<float>(obj, 0x24) = y;
        if (y < mem.f32(G::floor_y) + mem.f32(box + 4)) {
            vy = vy * mem.f32(G::held_bounce_factor);
            y = y0 + vy;
            of<int32_t>(obj, 0x2EC) = of<int32_t>(obj, 0x2EC) + 1;
            of<float>(obj, 0x7C) = vy;
            of<float>(obj, 0x24) = y;
        }
    }
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xFFFFFEF9u;
    depth_list_add(obj);
    held_object_draw(obj);
}

// 0x820B0 / 0x82400, PC 0x4E69B4 / 0x4E6C30 (the drum's +0x38 / +0x3C, the same code): it breaks in two: no hit
// points, uncounted, extras freed; a second piece (held_kit_piece_bounce task: position, angles, +0x38.., radius,
// kit; model 0x161, this one 0x15F; +0x320 1); both fly off the carrier's heading -0x2000 / +0x2000
// (held_drop_velocity, held_drop_accel); then held_kit_piece_bounce
template <class P> void Game<P>::held_kit_drop(uint32_t obj)
{
    const uint32_t owner = of<uint32_t>(obj, 0x18);
    of<int16_t>(obj, 0x70) = 0;
    held_uncount();
    enemy_free_extras(obj);
    const uint32_t t = task_create(F::held_kit_piece_bounce, uint32_t(sizeof(Obj)));
    for (uint32_t a = 0x20; a < 0x38; a += 4)
        of<uint32_t>(t, a) = of<uint32_t>(obj, a);
    for (uint32_t a = 0x38; a < 0x50; a += 4)
        of<uint32_t>(t, a) = of<uint32_t>(obj, a);
    of<uint32_t>(t, 0x90) = of<uint32_t>(obj, 0x90);
    of<int32_t>(t, 0x31C) = 0;
    of<uint32_t>(t, 0x32C) = of<uint32_t>(obj, 0x32C);
    of<int32_t>(obj, 0x320) = 1;
    of<int32_t>(t, 0x54) = 0x161;
    of<int16_t>(obj, 0x66) = 0;
    of<int32_t>(obj, 0x54) = 0x15F;
    of<int16_t>(t, 0x66) = 0;
    auto launch = [&](uint32_t o) {
        if (pc_path()) {
            pc_matrix_transform8(mem.f32(G::held_drop_accel), mem.f32(G::held_drop_accel + 4),
                                 mem.f32(G::held_drop_accel + 8), o + ob(0x84), o + ob(0x88), o + ob(0x8C));
            pc_matrix_transform8(mem.f32(G::held_drop_velocity), mem.f32(G::held_drop_velocity + 4),
                                 mem.f32(G::held_drop_velocity + 8), o + ob(0x78), o + ob(0x7C), o + ob(0x80));
        } else {
            Tgp::Out a, v;
            const uint32_t ia[3] = {mem.u32(G::held_drop_accel), mem.u32(G::held_drop_accel + 4), mem.u32(G::held_drop_accel + 8)};
            copro(0x1A, ia, 3, a);
            of<uint32_t>(o, 0x84) = a.w[0];
            of<uint32_t>(o, 0x88) = a.w[1];
            of<uint32_t>(o, 0x8C) = a.w[2];
            const uint32_t iv[3] = {mem.u32(G::held_drop_velocity), mem.u32(G::held_drop_velocity + 4),
                                    mem.u32(G::held_drop_velocity + 8)};
            copro(0x1A, iv, 3, v);
            of<uint32_t>(o, 0x78) = v.w[0];
            of<uint32_t>(o, 0x7C) = v.w[1];
            of<uint32_t>(o, 0x80) = v.w[2];
        }
    };
    if (pc_path()) {
        pc_matrix_push();
        pc_matrix_identity8();
        pc_matrix_rotate_y7(of<uint32_t>(owner, 0x30));
        pc_matrix_rotate_y7(0xFFFFE000u);
        launch(obj);
        pc_matrix_rotate_y7(0x4000);
        launch(t);
        pcm_pop();
    } else {
        Tgp::Out none;
        copro(0x21, nullptr, 0, none);
        const uint32_t ry[1] = {of<uint32_t>(owner, 0x30)};
        copro(0x15, ry, 1, none);
        const uint32_t back[1] = {0xFFFFE000u};
        copro(0x15, back, 1, none);
        launch(obj);
        const uint32_t q[1] = {0x4000};
        copro(0x15, q, 1, none);
        launch(t);
        copro(0x06, nullptr, 0, none);
    }
    of<uint32_t>(obj, 0x0) = F::held_kit_piece_bounce;
    held_kit_piece_bounce(obj);
}

template <class P> void Game<P>::held_kit_drop1(uint32_t obj) { held_kit_drop(obj); }   // 0x820B0 / PC 0x4E69B4
template <class P> void Game<P>::held_kit_drop2(uint32_t obj) { held_kit_drop(obj); }   // 0x82400 / PC 0x4E6C30

// 0x82320 / PC 0x4E6B54: first frame: not hit-tested, not fading, +0x2EC 0. Then moved (move_routines[1]), spinning
// (+0x2C +-0x200 by +0x31C); below the floor: bounces (vertical speed x -0.6 from the old height), fading from the
// second bounce, ends after the third; drawn (held_object_draw)
template <class P> void Game<P>::held_kit_piece_bounce(uint32_t obj)
{
    int16_t step = of<int16_t>(obj, 0x66);
    if (step == 0) {
        of<int32_t>(obj, 0x58) = 0;
        of<uint8_t>(obj, 0x3C4) = of<uint8_t>(obj, 0x3C4) & 0xFB;
        of<int32_t>(obj, 0x2EC) = 0;
        of<int16_t>(obj, 0x66) = 1;
        step = 1;
    }
    if (step == 1) {
        const float y0 = of<float>(obj, 0x24);
        call_routine(mem.u32(G::move_routines + 4), obj);
        of<uint32_t>(obj, 0x2C) = of<uint32_t>(obj, 0x2C) + (of<int32_t>(obj, 0x31C) != 0 ? 0x200u : 0xFFFFFE00u);
        bool keep = true;
        if (of<float>(obj, 0x24) < mem.f32(G::floor_y)) {
            const float vy = of<float>(obj, 0x7C) * ffrom(0xBF19999Au);   // -0.6
            of<float>(obj, 0x7C) = vy;
            of<float>(obj, 0x24) = y0 + vy;
            const int32_t n = of<int32_t>(obj, 0x2EC);
            if (n == 1)
                of<uint8_t>(obj, 0x3C4) = of<uint8_t>(obj, 0x3C4) | 4;
            of<int32_t>(obj, 0x2EC) = n + 1;
            if (n + 1 > 2)
                keep = false;
        }
        if (!keep) {
            task_exit();
            return;
        }
    }
    held_object_draw(obj);
}

// 0x82670 / PC 0x4E6DCC (the drum's +0x40, let go): shootable, uncounted; it falls forward from the carrier's
// heading (velocity (0, 0, -1), acceleration (0, -0.0817, 0)); +0x320 / +0x324 0, the tumble state (block +0x34)
// 0; then held_object_flight
template <class P> void Game<P>::held_object_release(uint32_t obj)
{
    const uint32_t owner = of<uint32_t>(obj, 0x18);
    const uint32_t buf = of<uint32_t>(obj, 0x37C);
    of<int16_t>(obj, 0x66) = 0;
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | 1;
    if (pc_path()) {
        held_uncount();
        pc_matrix_push();
        pc_matrix_identity8();
        pc_matrix_rotate_y7(of<uint32_t>(owner, 0x30));
        pc_matrix_transform8(0.0f, ffrom(0xBDA740DAu), 0.0f, obj + ob(0x84), obj + ob(0x88), obj + ob(0x8C));
        pc_matrix_transform8(0.0f, 0.0f, -1.0f, obj + ob(0x78), obj + ob(0x7C), obj + ob(0x80));
        of<int32_t>(obj, 0x320) = 0;
        of<int32_t>(obj, 0x324) = 0;
        mem.set32(buf + 0x34, 0);
        pcm_pop();
    } else {
        Tgp::Out none, a, v;
        copro(0x21, nullptr, 0, none);
        const uint32_t ry[1] = {of<uint32_t>(owner, 0x30)};
        copro(0x15, ry, 1, none);
        const uint32_t ia[3] = {0, 0xBDA740DAu, 0};   // -0.0817
        copro(0x1A, ia, 3, a);
        of<uint32_t>(obj, 0x84) = a.w[0];
        of<uint32_t>(obj, 0x88) = a.w[1];
        of<uint32_t>(obj, 0x8C) = a.w[2];
        const uint32_t iv[3] = {0, 0, 0xBF800000u};
        copro(0x1A, iv, 3, v);
        of<uint32_t>(obj, 0x78) = v.w[0];
        of<uint32_t>(obj, 0x7C) = v.w[1];
        copro(0x06, nullptr, 0, none);
        held_uncount();
        of<int32_t>(obj, 0x320) = 0;
        of<int32_t>(obj, 0x324) = 0;
        of<uint32_t>(obj, 0x80) = v.w[2];
        mem.set32(buf + 0x34, 0);
    }
    of<uint32_t>(obj, 0x0) = F::held_object_flight;
    held_object_flight(obj);
}

// 0x82790 / PC 0x4E6EAC (the released drum): destroyed -> counted again, the kit's +0x50 / sound / +0x3C; hit ->
// knocked off the camera's heading (held_hit_velocity / _accel), spin +0x320 0x200 and a random turn +0x324, the
// tumble restarted, the kit's +0x4C / sound. States (+0x66): 0-1 flying: the box corners (tumble_corners) and, falling,
// tumble_update; when it touches (block +0x34) it stops and 2: tumbling on the floor (tumble_update) until state 3:
// resting, ended once off screen (A: coprocessor 0x7B; PC: pc_view_test2). Each frame moved (move_routines[1]),
// turned (+0x2C + +0x324, then by +0x320 about y), +0x320 slowing by 24, not hit-tested, the kit's +0x44
template <class P> void Game<P>::held_object_flight(uint32_t obj)
{
    const uint32_t k = of<uint32_t>(obj, 0x32C);
    const uint32_t kr = G::held_object_kits + k * 0x58, box = G::held_object_shot_boxes + k * 0x34;
    const uint32_t buf = of<uint32_t>(obj, 0x37C);
    const int32_t h = held_object_shot_check(obj);
    if (h == -1) {
        mem.at<uint16_t>(G::enemy_count) = uint16_t(mem.at<uint16_t>(G::enemy_count) + 1);
        mem.at<uint16_t>(G::enemies_alive) = uint16_t(mem.at<uint16_t>(G::enemies_alive) + 1);
        held_call_f(mem.u32(kr + 0x50), obj, mem.u32(box + 0x2C));
        sound_cue(mem.u32(kr + 8));
        held_call(mem.u32(kr + 0x3C), obj);
        return;
    }
    if (h == 1) {
        of<int32_t>(obj, 0x320) = 0x200;
        of<int32_t>(obj, 0x324) = int32_t(rand() & 0x1FFF) - 0x1000;
        if (pc_path()) {
            pc_matrix_push();
            pc_matrix_identity8();
            pc_matrix_rotate_y7(mem.u32(G::camera_records + 0x10));
            pc_matrix_transform8(mem.f32(G::held_hit_accel), mem.f32(G::held_hit_accel + 4), mem.f32(G::held_hit_accel + 8),
                                 obj + ob(0x84), obj + ob(0x88), obj + ob(0x8C));
            pc_matrix_transform8(mem.f32(G::held_hit_velocity), mem.f32(G::held_hit_velocity + 4),
                                 mem.f32(G::held_hit_velocity + 8), obj + ob(0x78), obj + ob(0x7C), obj + ob(0x80));
            of<int16_t>(obj, 0x66) = 0;
            mem.set32(buf + 0x34, 0);
            pcm_pop();
        } else {
            Tgp::Out none, a, v;
            copro(0x21, nullptr, 0, none);
            const uint32_t ry[1] = {mem.u32(G::camera_records + 0x10)};
            copro(0x15, ry, 1, none);
            const uint32_t ia[3] = {mem.u32(G::held_hit_accel), mem.u32(G::held_hit_accel + 4), mem.u32(G::held_hit_accel + 8)};
            copro(0x1A, ia, 3, a);
            of<uint32_t>(obj, 0x84) = a.w[0];
            of<uint32_t>(obj, 0x88) = a.w[1];
            of<uint32_t>(obj, 0x8C) = a.w[2];
            const uint32_t iv[3] = {mem.u32(G::held_hit_velocity), mem.u32(G::held_hit_velocity + 4),
                                    mem.u32(G::held_hit_velocity + 8)};
            copro(0x1A, iv, 3, v);
            of<uint32_t>(obj, 0x78) = v.w[0];
            of<uint32_t>(obj, 0x7C) = v.w[1];
            copro(0x06, nullptr, 0, none);
            of<int16_t>(obj, 0x66) = 0;
            of<uint32_t>(obj, 0x80) = v.w[2];
            mem.set32(buf + 0x34, 0);
        }
        held_call_f(mem.u32(kr + 0x4C), obj, mem.u32(box + 0x2C));
        sound_cue(mem.u32(kr + 4));
    }
    // the box corners at its position and angles (tumble_corners)
    auto corners = [&]() {
        if (pc_path()) {
            pc_matrix_push();
            pc_matrix_identity8();
            pc_matrix_translate8(of<float>(obj, 0x20), of<float>(obj, 0x24), of<float>(obj, 0x28));
            pc_matrix_rotate_z8(of<uint32_t>(obj, 0x34));
            pc_matrix_rotate_y7(of<uint32_t>(obj, 0x30));
            pc_matrix_rotate_x8(of<uint32_t>(obj, 0x2C));
            if constexpr (P::pc)
                tumble_corners(buf);
            else
                pm_tumble_corners(buf);   // PC Mode: the PC's 0x4C5924 on PM's stack
            pcm_pop();
        } else {
            Tgp::Out none;
            copro(0x21, nullptr, 0, none);
            const uint32_t t[3] = {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)};
            copro(0x12, t, 3, none);
            const uint32_t r[3] = {of<uint32_t>(obj, 0x34), of<uint32_t>(obj, 0x30), of<uint32_t>(obj, 0x2C)};
            copro(0x86, r, 3, none);
            tumble_corners(buf);
            copro(0x06, nullptr, 0, none);
        }
    };
    int16_t step = of<int16_t>(obj, 0x66);
    if (pc_path())
        step = (uint16_t(step) > 3) ? 4 : step;   // the PC's jump table: anything above 3 (or negative) moves on
    bool tail = true;
    if (step == 0) {
        of<int16_t>(obj, 0x66) = 1;
        step = 1;
    }
    if (step == 1) {
        corners();
        if (of<float>(obj, 0x7C) < 0.0f)
            pm_on() ? pm_tumble_update(obj, buf) : tumble_update(obj, buf);   // (PC Mode: the PC's 0x4C5B1C)
        if (mem.u32(buf + 0x34) != 0) {
            of<int32_t>(obj, 0x80) = 0;
            of<int32_t>(obj, 0x7C) = 0;
            of<int32_t>(obj, 0x78) = 0;
            of<int32_t>(obj, 0x8C) = 0;
            of<int32_t>(obj, 0x88) = 0;
            of<int32_t>(obj, 0x84) = 0;
            of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
        }
    } else if (step == 2 || step == 3) {
        bool rest = step == 3;
        if (step == 2) {
            corners();
            pm_on() ? pm_tumble_update(obj, buf) : tumble_update(obj, buf);
            if (mem.u32(buf + 0x34) == 3) {
                of<int16_t>(obj, 0x66) = int16_t(of<int16_t>(obj, 0x66) + 1);
                rest = true;
            }
        }
        if (rest) {
            const float rad = of<float>(obj, 0x90) * of<float>(obj, 0x94);
            float seen;
            if (pc_path()) {
                seen = pc_view_test2(of<float>(obj, 0x20), of<float>(obj, 0x24), of<float>(obj, 0x28), rad);
            } else {
                Tgp::Out o;
                const uint32_t v[4] = {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28), fbits(rad)};
                copro(0x7B, v, 4, o);
                seen = ffrom(o.w[0]);
            }
            if (!(seen > 0.0f)) {
                enemy_free_extras(obj);
                task_exit();
                return;
            }
        }
    }
    (void)tail;
    call_routine(mem.u32(G::move_routines + 4), obj);
    of<uint32_t>(obj, 0x2C) = of<uint32_t>(obj, 0x2C) + of<uint32_t>(obj, 0x324);
    if (pc_path()) {
        pc_matrix_push();
        pc_matrix_identity8();
        pc_matrix_rotate_z8(of<uint32_t>(obj, 0x34));
        pc_matrix_rotate_y7(of<uint32_t>(obj, 0x30));
        pc_matrix_rotate_x8(of<uint32_t>(obj, 0x2C));
        pc_matrix_rotate_y7(of<uint32_t>(obj, 0x320));
        pc_matrix_angles8(obj + ob(0x2C), obj + ob(0x30), obj + ob(0x34));
        pcm_pop();
    } else {
        Tgp::Out none, o;
        copro(0x21, nullptr, 0, none);
        const uint32_t r[3] = {of<uint32_t>(obj, 0x34), of<uint32_t>(obj, 0x30), of<uint32_t>(obj, 0x2C)};
        copro(0x86, r, 3, none);
        const uint32_t ry[1] = {of<uint32_t>(obj, 0x320)};
        copro(0x15, ry, 1, none);
        copro(0x71, nullptr, 0, o);
        of<uint32_t>(obj, 0x34) = o.w[0];
        of<uint32_t>(obj, 0x30) = o.w[1];
        of<uint32_t>(obj, 0x2C) = o.w[2];
        copro(0x06, nullptr, 0, none);
    }
    const int32_t spin = of<int32_t>(obj, 0x320);
    of<int32_t>(obj, 0x320) = spin > 24 ? spin - 24 : 0;
    of<int32_t>(obj, 0x324) = 0;
    of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) & 0xFFFFFEF9u;
    depth_list_add(obj);
    held_call(mem.u32(kr + 0x44), obj);
}

// 0x82C30 / PC 0x4E722C (the drums / barrel, +0x44): at its position and angles (+0x38.. that point); drawn offset by
// minus the shot box's +8..+0x10 (A: faded by the kit's +0x56 unless 0xFF; PC: no fade, only without the option
// pc_no_extra_model) unless fading on an even frame; its camera-space point from stored matrix 5; +0x2D4.. its position
template <class P> void Game<P>::held_object_draw(uint32_t obj)
{
    const uint32_t k = of<uint32_t>(obj, 0x32C);
    const uint32_t box = G::held_object_shot_boxes + k * 0x34, kr = G::held_object_kits + k * 0x58;
    const bool show = ((of<uint8_t>(obj, 0x3C4) >> 2) & 1) == 0 || (mem.at<uint8_t>(G::blink_phase) & 1) != 0;
    if constexpr (P::pc) {
        auto place = [&]() {
            pc_matrix_translate8(of<float>(obj, 0x20), of<float>(obj, 0x24), of<float>(obj, 0x28));
            pc_matrix_rotate_z8(of<uint32_t>(obj, 0x34));
            pc_matrix_rotate_y7(of<uint32_t>(obj, 0x30));
            pc_matrix_rotate_x8(of<uint32_t>(obj, 0x2C));
        };
        pc_matrix_push();
        place();
        uint32_t m = mem.u32(G::pc_matrix_ptr);
        of<uint32_t>(obj, 0x38) = mem.u32(m + 0x24);
        of<uint32_t>(obj, 0x3C) = mem.u32(m + 0x28);
        of<uint32_t>(obj, 0x40) = mem.u32(m + 0x2C);
        if (mem.u32(G::pc_no_extra_model) == 0 && show) {
            pc_matrix_translate8(-mem.f32(box + 8), -mem.f32(box + 0xC), -mem.f32(box + 0x10));
            geo_draw_model(of<uint32_t>(obj, 0x54));
        }
        pcm_pop();
        pc_matrix_push();
        m = mem.u32(G::pc_matrix_ptr);
        for (uint32_t i = 0; i < 12; ++i)
            mem.set32(m + 4 * i, mem.u32(G::pc_stored_matrices + 0xF0 + 4 * i));
        place();
        m = mem.u32(G::pc_matrix_ptr);
        of<uint32_t>(obj, 0x98) = mem.u32(m + 0x24);
        mem.set32(G::pc_matrix_ptr, m - 0x30);
        of<uint32_t>(obj, 0x9C) = mem.u32(m + 0x28);
        of<uint32_t>(obj, 0xA0) = mem.u32(m + 0x2C);
    } else {
        Tgp::Out none, o, v;
        const uint32_t t[3] = {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)};
        const uint32_t r[3] = {of<uint32_t>(obj, 0x34), of<uint32_t>(obj, 0x30), of<uint32_t>(obj, 0x2C)};
        copro(0x05, nullptr, 0, none);
        copro(0x12, t, 3, none);
        copro(0x86, r, 3, none);
        copro(0x20, nullptr, 0, o);
        of<uint32_t>(obj, 0x38) = o.w[0];
        of<uint32_t>(obj, 0x3C) = o.w[1];
        of<uint32_t>(obj, 0x40) = o.w[2];
        if (show) {
            const uint32_t off[3] = {mem.u32(box + 8) ^ 0x80000000u, mem.u32(box + 0xC) ^ 0x80000000u,
                                     mem.u32(box + 0x10) ^ 0x80000000u};
            copro(0x12, off, 3, none);
            if (mem.at<uint16_t>(kr + 0x56) != 0xFF) {
                draw_fade_begin(mem.at<int16_t>(kr + 0x56));
                geo_draw_model(of<uint32_t>(obj, 0x54));
                geo_send_words(0, 31, 0x7A0);
            } else {
                geo_draw_model(of<uint32_t>(obj, 0x54));
            }
        }
        copro(0x06, nullptr, 0, none);
        const uint32_t slot[1] = {5};
        copro(0x74, slot, 1, none);
        copro(0x12, t, 3, none);
        copro(0x86, r, 3, none);
        copro(0x20, nullptr, 0, v);
        of<uint32_t>(obj, 0x98) = v.w[0];
        of<uint32_t>(obj, 0x9C) = v.w[1];
        copro(0x06, nullptr, 0, none);
        of<uint32_t>(obj, 0x2D4) = of<uint32_t>(obj, 0x20);
        of<uint32_t>(obj, 0x2D8) = of<uint32_t>(obj, 0x24);
        of<uint32_t>(obj, 0x2DC) = of<uint32_t>(obj, 0x28);
        of<uint32_t>(obj, 0xA0) = v.w[2];
        return;
    }
    of<uint32_t>(obj, 0x2D4) = of<uint32_t>(obj, 0x20);
    of<uint32_t>(obj, 0x2D8) = of<uint32_t>(obj, 0x24);
    of<uint32_t>(obj, 0x2DC) = of<uint32_t>(obj, 0x28);
}

// 0x82E30 / PC 0x4E7390 (the barrel's +0x38 / +0x3C): no hit points, uncounted, extras freed; held_kit_barrel_breakup
template <class P> void Game<P>::held_kit_barrel_drop(uint32_t obj)
{
    of<int16_t>(obj, 0x70) = 0;
    held_uncount();
    enemy_free_extras(obj);
    of<uint32_t>(obj, 0x0) = F::held_kit_barrel_breakup;
    of<int16_t>(obj, 0x66) = 0;
    held_kit_barrel_breakup(obj);
}

// 0x82E80 / PC 0x4E73D4: not hit-tested; the break-up models 0x689.. one per frame (fading after 0x6A7), ends after
// 0x6CF; drawn (held_object_draw)
template <class P> void Game<P>::held_kit_barrel_breakup(uint32_t obj)
{
    const int16_t step = of<int16_t>(obj, 0x66);
    if (step == 0 || step == 1) {
        if (step == 0) {
            of<int32_t>(obj, 0x58) = 0;
            of<int32_t>(obj, 0x2EC) = 0;
            of<int32_t>(obj, 0x54) = 0x689;
            of<uint8_t>(obj, 0x3C4) = of<uint8_t>(obj, 0x3C4) & 0xFB;
            of<int16_t>(obj, 0x66) = 1;
        }
        const int32_t m = of<int32_t>(obj, 0x54) + 1;
        if (m > 0x6A7)
            of<uint8_t>(obj, 0x3C4) = of<uint8_t>(obj, 0x3C4) | 4;
        of<int32_t>(obj, 0x54) = m;
        if (m > 0x6CF) {
            task_exit();
            return;
        }
    }
    held_object_draw(obj);
}
