// PC Mode's helper copies (port/pm_maths.h) against THOTD.EXE's own instructions (included by proof/main.cpp): every
// copy of the matrix stack helpers, pc_vec_angles, pc_atan2, the table square roots, sin / cos, floor distances,
// pc_angle_turn and the camera matrix, each run by the x86 / x87 interpreter (proof/x86emu.h) on the PC memory image m
// (the EXE's sections, the matrix stack at pc_matrix_ptr) with random inputs. Returns the number of routines with a
// difference (+ 1 for an interpreter error)
int pmmaths_exe(Mem& m, uint64_t& seed)
{
    using namespace hotd::port;
    using hotd::proof::X86;
    using GP = typename Game<Pc>::G;
    using FP = typename Game<Pc>::F;
    const uint32_t cpuStack = 0x31000000, scratch = 0x31010000;
    if (!m.mapped(cpuStack))
        m.map(cpuStack, 0x10000);
    if (!m.mapped(scratch))
        m.map(scratch, 0x1000);
    X86 cpu(m);
    m.bad = false;
    auto rnd = [&]() {
        seed = seed * 6364136223846793005ull + 1442695040888963407ull;
        return uint32_t(seed >> 33);
    };
    auto frand = [&](float lo, float hi) { return lo + (hi - lo) * float(rnd() % 1000001u) / 1000000.0f; };
    auto fb = [](float f) { return fbits(f); };
    const uint32_t cur = m.u32(GP::pc_matrix_ptr);
    auto randMatrix = [&](float out[12]) {
        for (int k = 0; k < 9; ++k)
            out[k] = (rnd() & 15) == 0 ? float(int32_t(rnd() % 3u) - 1) : frand(-1.5f, 1.5f);
        for (int k = 9; k < 12; ++k)
            out[k] = frand(-800.0f, 800.0f);
    };
    auto putMatrix = [&](uint32_t a, const float e[12]) {
        for (int k = 0; k < 12; ++k)
            m.set32(a + 4 * uint32_t(k), fb(e[k]));
    };
    auto sameMatrix = [&](uint32_t a, const float e[12]) {
        for (int k = 0; k < 12; ++k)
            if (m.u32(a + 4 * uint32_t(k)) != fb(e[k]))
                return false;
        return true;
    };
    auto randAngle = [&]() { return (rnd() & 3) == 0 ? rnd() * 7u : rnd() & 0xFFFF; };
    auto randF = [&]() {
        const uint32_t k = rnd() % 8u;
        return k == 0 ? 0.0f : k == 1 ? frand(-0.01f, 0.01f) : k == 2 ? frand(-5000.0f, 5000.0f) : frand(-300.0f, 300.0f);
    };
    int badRoutines = 0, nroutines = 0;
    long total = 0;
    std::string firstErr;
    auto run = [&](uint32_t addr, uint32_t eax, uint32_t edx, std::initializer_list<uint32_t> st) {
        const uint32_t regs[4] = {eax, edx, 0, 0};
        std::vector<uint32_t> s(st);
        cpu.top = 0;
        cpu.cw = 0x107F;
        if (!cpu.call(addr, regs, s.data(), int(s.size()), cpuStack + 0xFF00) && firstErr.empty()) {
            char b[64];
            std::snprintf(b, sizeof b, " (running 0x%X)", addr);
            firstErr = cpu.error + b;
        }
        if (m.u32(GP::pc_matrix_ptr) != cur && firstErr.empty()) {
            char b[64];
            std::snprintf(b, sizeof b, "0x%X moved the matrix pointer", addr);
            firstErr = b;
        }
        m.set32(GP::pc_matrix_ptr, cur);
    };
    auto report = [&](const char* name, uint32_t addr, long cases, long bad) {
        total += cases;
        ++nroutines;
        if (bad) {
            ++badRoutines;
            std::printf("  %s 0x%X: %ld of %ld differ\n", name, addr, bad, cases);
        }
    };
    const int N = 20000;
    // matrix rotations: y, x, z (rotate_b is a z turn)
    struct Rot {
        const char* name;
        uint32_t addr;
        int axis;   // 0 x, 1 y, 2 z
    };
    const Rot rots[] = {
        {"rotate_y", FP::pc_matrix_rotate_y, 1},     {"rotate_y2", FP::pc_matrix_rotate_y2, 1},
        {"rotate_y3", FP::pc_matrix_rotate_y3, 1},   {"rotate_y5", FP::pc_matrix_rotate_y5, 1},
        {"rotate_y6", FP::pc_matrix_rotate_y6, 1},   {"rotate_y7", FP::pc_matrix_rotate_y7, 1},
        {"rotate_y8", FP::pc_matrix_rotate_y8, 1},   {"rotate_y9", FP::pc_matrix_rotate_y9, 1},
        {"rotate_y10", FP::pc_matrix_rotate_y10, 1}, {"rotate_y11", FP::pc_matrix_rotate_y11, 1},
        {"rotate_y12", FP::pc_matrix_rotate_y12, 1}, {"camera rotate_y", 0x45D61C, 1},
        {"rotate_x", FP::pc_matrix_rotate_x, 0},     {"rotate_x2", FP::pc_matrix_rotate_x2, 0},
        {"rotate_x8", FP::pc_matrix_rotate_x8, 0},   {"rotate_x9", FP::pc_matrix_rotate_x9, 0},
        {"rotate_x10", FP::pc_matrix_rotate_x10, 0}, {"rotate_x11", FP::pc_matrix_rotate_x11, 0},
        {"rotate_x12", FP::pc_matrix_rotate_x12, 0}, {"camera rotate_x", 0x45D8B0, 0},
        {"rotate_b", FP::pc_matrix_rotate_b, 2},     {"rotate_b2", FP::pc_matrix_rotate_b2, 2},
        {"rotate_b3", FP::pc_matrix_rotate_b3, 2},   {"rotate_b10", FP::pc_matrix_rotate_b10, 2},
        {"rotate_z8", FP::pc_matrix_rotate_z8, 2},   {"rotate_z9", FP::pc_matrix_rotate_z9, 2},
        {"rotate_z11", FP::pc_matrix_rotate_z11, 2}, {"rotate_z12", FP::pc_matrix_rotate_z12, 2},
        {"camera rotate_z", 0x45D724, 2},
        // pmrest: the kind 38 barrel file's copies (0x432500 / 0x432648: z, y, x)
        {"barrel rotate_z", 0x42FA2C, 2},            {"barrel rotate_y", 0x42F924, 1},
        {"barrel rotate_x", 0x42FBB8, 0},
    };
    for (const Rot& r : rots) {
        long bad = 0;
        for (int n = 0; n < N; ++n) {
            pm::Matrix pm;
            randMatrix(pm.e);
            putMatrix(cur, pm.e);
            const uint32_t a = randAngle();
            run(r.addr, a, 0, {});
            if (r.axis == 0)
                pm.rotate_x(a);
            else if (r.axis == 1)
                pm.rotate_y(a);
            else
                pm.rotate_z(a);
            bad += !sameMatrix(cur, pm.e);
        }
        report(r.name, r.addr, N, bad);
    }
    // translate (x, y, z on the stack)
    const std::pair<const char*, uint32_t> translates[] = {
        {"translate", FP::pc_matrix_translate},     {"translate2", FP::pc_matrix_translate2},
        {"translate4", FP::pc_matrix_translate4},   {"translate5", FP::pc_matrix_translate5},
        {"translate6", FP::pc_matrix_translate6},   {"translate8", FP::pc_matrix_translate8},
        {"translate9", FP::pc_matrix_translate9},   {"translate10", FP::pc_matrix_translate10},
        {"translate11", FP::pc_matrix_translate11}, {"translate12", FP::pc_matrix_translate12},
        {"camera translate", 0x45D3FC},             {"barrel translate", 0x42F704},
    };
    for (const auto& t : translates) {
        long bad = 0;
        for (int n = 0; n < N; ++n) {
            pm::Matrix pm;
            randMatrix(pm.e);
            putMatrix(cur, pm.e);
            const float x = randF(), y = randF(), z = randF();
            run(t.second, 0, 0, {fb(x), fb(y), fb(z)});
            pm.translate(x, y, z);
            bad += !sameMatrix(cur, pm.e);
        }
        report(t.first, t.second, N, bad);
    }
    // transform (x, y, z, &ox, &oy, &oz)
    const std::pair<const char*, uint32_t> transforms[] = {
        {"transform", FP::pc_matrix_transform},     {"transform2", FP::pc_matrix_transform2},
        {"transform3", FP::pc_matrix_transform3},   {"transform4", FP::pc_matrix_transform4},
        {"transform5", FP::pc_matrix_transform5},   {"transform6", FP::pc_matrix_transform6},
        {"transform7", FP::pc_matrix_transform7},   {"transform8", FP::pc_matrix_transform8},
        {"transform9", FP::pc_matrix_transform9},   {"transform10", FP::pc_matrix_transform10},
        {"transform11", FP::pc_matrix_transform11}, {"transform12", FP::pc_matrix_transform12},
    };
    for (const auto& t : transforms) {
        long bad = 0;
        for (int n = 0; n < N; ++n) {
            pm::Matrix pm;
            randMatrix(pm.e);
            putMatrix(cur, pm.e);
            const float x = randF(), y = randF(), z = randF();
            run(t.second, 0, 0, {fb(x), fb(y), fb(z), scratch, scratch + 4, scratch + 8});
            float o[3];
            pm.transform(x, y, z, o[0], o[1], o[2]);
            bad += m.u32(scratch) != fb(o[0]) || m.u32(scratch + 4) != fb(o[1]) || m.u32(scratch + 8) != fb(o[2]) ||
                   !sameMatrix(cur, pm.e);
        }
        report(t.first, t.second, N, bad);
    }
    // identity
    const std::pair<const char*, uint32_t> identities[] = {
        {"identity", FP::pc_matrix_identity},     {"identity2", FP::pc_matrix_identity2},
        {"identity3", FP::pc_matrix_identity3},   {"identity8", FP::pc_matrix_identity8},
        {"identity11", FP::pc_matrix_identity11}, {"identity12", FP::pc_matrix_identity12},
        {"barrel identity", 0x42FF74},
    };
    for (const auto& t : identities) {
        long bad = 0;
        for (int n = 0; n < 100; ++n) {
            pm::Matrix pm;
            randMatrix(pm.e);
            putMatrix(cur, pm.e);
            run(t.second, 0, 0, {});
            pm.identity();
            bad += !sameMatrix(cur, pm.e);
        }
        report(t.first, t.second, 100, bad);
    }
    // matrix products (the other matrix by pointer in eax)
    const std::pair<const char*, uint32_t> muls[] = {{"mul8", FP::pc_matrix_mul8},
                                                     {"mul_stored", FP::pc_matrix_mul_stored},
                                                     {"mul_stored10", FP::pc_matrix_mul_stored10}};
    for (const auto& t : muls) {
        long bad = 0;
        for (int n = 0; n < N; ++n) {
            pm::Matrix pm;
            randMatrix(pm.e);
            putMatrix(cur, pm.e);
            float other[12];
            randMatrix(other);
            putMatrix(scratch + 0x100, other);
            run(t.second, scratch + 0x100, 0, {});
            if (t.second == FP::pc_matrix_mul8)
                pm.mul(other);
            else
                pm.mul_stored(other);
            bad += !sameMatrix(cur, pm.e);
        }
        report(t.first, t.second, N, bad);
    }
    // pc_vec_angles copies (x, y, z, &yaw, &pitch)
    const std::pair<const char*, uint32_t> angles[] = {
        {"vec_angles", FP::pc_vec_angles},     {"vec_angles2", FP::pc_vec_angles2},
        {"vec_angles3", FP::pc_vec_angles3},   {"vec_angles5", FP::pc_vec_angles5},
        {"vec_angles6", FP::pc_vec_angles6},   {"vec_angles7", FP::pc_vec_angles7},
        {"vec_angles8", FP::pc_vec_angles8},   {"vec_angles9", FP::pc_vec_angles9},
        {"vec_angles10", FP::pc_vec_angles10}, {"vec_angles12", FP::pc_vec_angles12},
    };
    for (const auto& t : angles) {
        long bad = 0;
        for (int n = 0; n < N; ++n) {
            const float x = randF(), y = (n & 1) ? 0.0f : randF(), z = randF();
            m.set32(scratch, 0x12345678);
            m.set32(scratch + 4, 0x9ABCDEF0);
            run(t.second, 0, 0, {fb(x), fb(y), fb(z), scratch, scratch + 4});
            uint32_t yaw = 0, pitch = 0;
            pm::vec_angles(x, y, z, yaw, pitch);
            bad += m.u32(scratch) != yaw || m.u32(scratch + 4) != pitch;
        }
        report(t.first, t.second, N, bad);
    }
    // pc_atan2 (a, b) -> eax; the kind 32 file's pc_atan2_8 returns it << 4
    const std::pair<const char*, uint32_t> atans[] = {{"atan2", FP::pc_atan2}, {"atan2_8", FP::pc_atan2_8}};
    for (const auto& t : atans) {
        long bad = 0;
        for (int n = 0; n < N * 5; ++n) {
            const float a = randF(), b = (n % 13 == 0) ? 0.0f : randF();
            run(t.second, 0, 0, {fb(a), fb(b)});
            const uint32_t want = uint32_t(pm::atan2(a, b)) << (t.second == FP::pc_atan2_8 ? 4 : 0);
            const bool d = cpu.r[X86::EAX] != want;
            if (d && bad < 3)
                std::printf("    atan2(%.9g, %.9g): PC 0x%X, pm 0x%X\n", a, b, cpu.r[X86::EAX], want);
            bad += d;
        }
        report(t.first, t.second, N * 5, bad);
    }
    // table square roots (x) -> st0
    const std::pair<const char*, uint32_t> sqrts[] = {
        {"fast_sqrt", FP::pc_fast_sqrt},     {"fast_sqrt2", FP::pc_fast_sqrt2}, {"fast_sqrt3", FP::pc_fast_sqrt3},
        {"fast_sqrt4", FP::pc_fast_sqrt4},   {"fast_sqrt5", FP::pc_fast_sqrt5}, {"fast_sqrt6", FP::pc_fast_sqrt6},
        {"fast_sqrt7", FP::pc_fast_sqrt7},   {"fast_sqrt8", FP::pc_fast_sqrt8}, {"fast_sqrt9", FP::pc_fast_sqrt9},
        {"fast_sqrt10", FP::pc_fast_sqrt10}, {"fast_sqrt12", FP::pc_fast_sqrt12},
    };
    for (const auto& t : sqrts) {
        long bad = 0;
        for (int n = 0; n < N; ++n) {
            const float x = (n & 1) ? frand(0.0f, 4.0f) : frand(0.0f, 1.0e6f);
            run(t.second, 0, 0, {fb(x)});
            bad += fb(cpu.st0f()) != fb(pm::fast_sqrt(x));
        }
        report(t.first, t.second, N, bad);
    }
    // the bats file's sine / cosine (angle in eax) -> st0
    for (int k = 0; k < 2; ++k) {
        const uint32_t addr = k ? FP::pc_cos2 : FP::pc_sin2;
        long bad = 0;
        for (int n = 0; n < N; ++n) {
            const uint32_t a = randAngle();
            run(addr, a, 0, {});
            bad += fb(cpu.st0f()) != fb(pm::trig(a, k));
        }
        report(k ? "cos2" : "sin2", addr, N, bad);
    }
    // floor distances (ax, az, bx, bz) -> st0
    for (const uint32_t addr : {FP::pc_floor_distance2, FP::pc_floor_distance10}) {
        long bad = 0;
        for (int n = 0; n < N; ++n) {
            const float ax = randF(), az = randF(), bx = randF(), bz = randF();
            run(addr, 0, 0, {fb(ax), fb(az), fb(bx), fb(bz)});
            const float dx = bx - ax, dz = bz - az;
            bad += fb(cpu.st0f()) != fb(pm::fast_sqrt(dx * dx + dz * dz));
        }
        report("floor_distance", addr, N, bad);
    }
    // pc_vec_cos (a0, a1, a2, b0, b1, b2) -> st0
    {
        long bad = 0;
        for (int n = 0; n < N; ++n) {
            float v[6];
            for (float& f : v)
                f = randF();
            if (n & 1)
                v[1] = v[4] = 0.0f;
            run(FP::pc_vec_cos, 0, 0, {fb(v[0]), fb(v[1]), fb(v[2]), fb(v[3]), fb(v[4]), fb(v[5])});
            bad += fb(cpu.st0f()) != fb(pm::vec_cos(v[0], v[1], v[2], v[3], v[4], v[5]));
        }
        report("vec_cos", FP::pc_vec_cos, N, bad);
    }
    // pc_angle_turn (angle eax, target edx, k, slow)
    {
        long bad = 0;
        for (int n = 0; n < N; ++n) {
            const int32_t a = int32_t(randAngle());
            const uint32_t t = rnd() & 0xFFFF;
            const float k = 1.0f, slow = frand(0.0f, 240.0f);
            run(FP::pc_angle_turn, uint32_t(a), t, {fb(k), fb(slow)});
            const bool d = int32_t(cpu.r[X86::EAX]) != pm::angle_turn(a, t, k, slow);
            if (d && bad < 3)
                std::printf("    angle_turn(0x%X, 0x%X, %g, %.9g): PC 0x%X, pm 0x%X\n", uint32_t(a), t, k, slow, cpu.r[X86::EAX], uint32_t(pm::angle_turn(a, t, k, slow)));
            bad += d;
        }
        report("angle_turn", FP::pc_angle_turn, N, bad);
    }
    // the camera matrix: camera_matrices_update 0x45F140 run up to its shadow_setup call on random records (eye, pitch,
    // yaw, roll at +0x60.. of each 0x154-byte record from 0xDA01C0; camera_view 0xDA0BD0): stored matrix 1 0xD52EA0
    {
        const uint32_t recs = 0xDA01C0, view = 0xDA0BD0;
        if (!m.mapped(recs))
            m.map(recs & ~0xFFFu, 0x2000);
        if (!m.mapped(view))
            m.map(view & ~0xFFFu, 0x1000);
        if (!m.mapped(0xD9EC00))
            m.map(0xD9E000, 0x1000);
        long bad = 0;
        for (int n = 0; n < 5000; ++n) {
            for (uint32_t k = 0; k < 6; ++k) {
                const uint32_t r = recs + k * 0x154 + 0x60;
                for (uint32_t i = 0; i < 3; ++i)
                    m.set32(r + 4 * i, fb(randF()));
                for (uint32_t i = 3; i < 6; ++i)
                    m.set32(r + 4 * i, randAngle());
            }
            const uint32_t v = rnd() % 6u;
            m.set32(view, v);
            cpu.stopAt = FP::shadow_setup;
            run(FP::camera_matrices_update, 0, 0, {});
            cpu.stopAt = 0xFFFFFFFFu;
            const uint32_t r = recs + v * 0x154 + 0x60;
            const float eye[3] = {m.f32(r), m.f32(r + 4), m.f32(r + 8)};
            float cam[12];
            pm::camera_matrix(eye, m.u32(r + 0xC), m.u32(r + 0x10), m.u32(r + 0x14), cam);
            bad += !sameMatrix(GP::pc_stored_matrices + 0x30, cam);
            pm::camera_inverse(eye, m.u32(r + 0xC), m.u32(r + 0x10), m.u32(r + 0x14), cam);
            bad += !sameMatrix(GP::pc_stored_matrices, cam) || !sameMatrix(cur, cam);   // stored 0 and the current one
            const uint32_t r0 = recs + 0x60;   // stored 5: record 0's inverse
            const float eye0[3] = {m.f32(r0), m.f32(r0 + 4), m.f32(r0 + 8)};
            pm::camera_inverse(eye0, m.u32(r0 + 0xC), m.u32(r0 + 0x10), m.u32(r0 + 0x14), cam);
            bad += !sameMatrix(GP::pc_stored_matrices + 5 * 0x30, cam);
            pm::camera_matrix(eye0, m.u32(r0 + 0xC), m.u32(r0 + 0x10), m.u32(r0 + 0x14), cam);   // stored 6: its view
            bad += !sameMatrix(GP::pc_stored_matrices + 6 * 0x30, cam);
        }
        report("camera matrix and inverse (camera_matrices_update)", FP::camera_matrices_update, 5000, bad);
    }
    // the enemy helpers on a PC object (layout: pos +0x28, angles +0x34, velocity +0x80, attack slot +0x2EF):
    // move_along_heading, angle_to_camera, angle_turn_toward, walk_to, turn_toward_camera against the runtime's PC Mode
    // forms (Game<A>'s PM branches use these same pm_maths.h pieces)
    {
        const uint32_t obj = 0x31020000;
        if (!m.mapped(obj))
            m.map(obj, 0x1000);
        if (!m.mapped(GP::camera_pos))
            m.map(GP::camera_pos & ~0xFFFu, 0x1000);
        if (!m.mapped(GP::enemy_count))
            m.map(GP::enemy_count & ~0xFFFu, 0x1000);
        auto setObj = [&]() {
            for (uint32_t k = 0; k < 0x400; k += 4)
                m.set32(obj + k, 0);
            for (uint32_t i = 0; i < 3; ++i) {
                m.set32(obj + 0x28 + 4 * i, fb(randF()));
                m.set32(obj + 0x34 + 4 * i, randAngle());
            }
            for (uint32_t i = 0; i < 3; ++i)
                m.set32(GP::camera_pos + 4 * i, fb(randF()));
        };
        // the arcade's angle_turn_toward (enemy_helpers.cpp), which PM keeps: the PC's 0x4477A4 must agree
        auto turnToward = [&](uint32_t angle, uint32_t rate) {
            const int32_t cur = m.at<uint16_t>(obj + 0x38), tgt = int32_t(angle & 0xFFFF), r = int32_t(rate);
            const int32_t d = std::abs(cur - tgt), dUp = std::abs(tgt - (cur - 0x10000)),
                          dDown = std::abs(cur - (tgt - 0x10000));
            int32_t out;
            if (d <= r || dUp <= r || dDown <= r)
                out = tgt;
            else if (cur < tgt)
                out = d > dDown ? cur - r : cur + r;
            else
                out = d > dUp ? cur + r : cur - r;
            return uint32_t(out);
        };
        long bad[5] = {};
        for (int n = 0; n < N; ++n) {
            // move_along_heading (obj eax, step)
            setObj();
            const float step = randF();
            float p[3] = {m.f32(obj + 0x28), m.f32(obj + 0x2C), m.f32(obj + 0x30)};
            float s[3];
            pm::heading_step(m.u32(obj + 0x34), m.u32(obj + 0x38), m.u32(obj + 0x3C), step, s);
            run(FP::move_along_heading, obj, 0, {fb(step)});
            bad[0] += m.u32(obj + 0x28) != fb(p[0] + s[0]) || m.u32(obj + 0x2C) != fb(p[1] + s[1]) ||
                      m.u32(obj + 0x30) != fb(p[2] + s[2]);
            // angle_to_camera (obj eax) -> eax
            setObj();
            const float pos[3] = {m.f32(obj + 0x28), m.f32(obj + 0x2C), m.f32(obj + 0x30)};
            const float cam[3] = {m.f32(GP::camera_pos), m.f32(GP::camera_pos + 4), m.f32(GP::camera_pos + 8)};
            const uint32_t want = pm::angle_to_point(pos, m.u32(obj + 0x34), m.u32(obj + 0x38), m.u32(obj + 0x3C), cam);
            run(FP::angle_to_camera, obj, 0, {});
            bad[1] += cpu.r[X86::EAX] != want;
            // angle_turn_toward (obj eax, angle edx, rate ebx)
            setObj();
            if (n & 1)
                m.set32(obj + 0x38, rnd() & 0xFFFF);
            const uint32_t angle = randAngle(), rate = (n & 2) ? rnd() % 0x400u : rnd() % 0x9000u;
            const uint32_t wantT = turnToward(angle, rate);
            {
                const uint32_t regs[4] = {obj, angle, rate, 0};
                cpu.top = 0;
                cpu.cw = 0x107F;
                if (!cpu.call(FP::angle_turn_toward, regs, nullptr, 0, cpuStack + 0xFF00) && firstErr.empty())
                    firstErr = cpu.error + " (angle_turn_toward)";
            }
            bad[2] += m.u32(obj + 0x38) != wantT;
            // walk_to (obj eax, x, z, turn, within) -> eax
            setObj();
            {
                const float x = randF(), z = randF(), within = frand(0.0f, 300.0f);
                const uint32_t turn = rnd() % 0x800u;
                float dir[3];
                uint32_t yaw = 0;
                const uint32_t wantR = pm::walk_to(m.f32(obj + 0x28), m.f32(obj + 0x30), x, z, within, dir, yaw);
                const uint32_t wantYaw = turnToward(yaw, turn);
                run(FP::walk_to, obj, 0, {fb(x), fb(z), turn, fb(within)});
                bad[3] += m.u32(obj + 0x80) != fb(dir[0]) || m.u32(obj + 0x84) != fb(dir[1]) ||
                          m.u32(obj + 0x88) != fb(dir[2]) || m.u32(obj + 0x38) != wantYaw ||
                          (cpu.r[X86::EAX] & 0xFF) != wantR;
            }
            // turn_toward_camera (obj eax, step edx)
            setObj();
            {
                const int16_t enemies = int16_t(rnd() % 4u);
                m.at<int16_t>(GP::enemy_count) = enemies;
                m.at<int8_t>(obj + 0x2EF) = int8_t(int32_t(rnd() % 16u) - 2);
                const uint32_t stepT = rnd() % 0x800u;
                const float camx = m.f32(GP::camera_pos), camz = m.f32(GP::camera_pos + 8);
                const float dx = pm::camera_aim_dx(camx, m.f32(obj + 0x28), enemies >= 2, m.at<int8_t>(obj + 0x2EF));
                const float dz = camz - m.f32(obj + 0x30);
                float dir[3];
                pm::floor_dir(dx, dz, dir);
                uint32_t yaw = 0, pitch = 0;
                pm::vec_angles(dx, 0.0f, dz, yaw, pitch);
                const uint32_t wantYaw = turnToward(yaw, stepT);
                run(FP::turn_toward_camera, obj, stepT, {});
                const bool d = m.u32(obj + 0x80) != fb(dir[0]) || m.u32(obj + 0x84) != fb(dir[1]) ||
                               m.u32(obj + 0x88) != fb(dir[2]) || m.u32(obj + 0x38) != wantYaw;
                if (d && bad[4] < 3)
                    std::printf("    turn_toward_camera: enemies %d slot %d: PC v (%.9g %.9g) yaw 0x%X, pm (%.9g %.9g) 0x%X\n",
                                enemies, m.at<int8_t>(obj + 0x2EF), m.f32(obj + 0x80), m.f32(obj + 0x88), m.u32(obj + 0x38),
                                dir[0], dir[2], wantYaw);
                bad[4] += d;
            }
        }
        // attacker_list_add (obj eax): the key of the new entry; enemy_target_distance (obj eax): the arcade's logic
        // with the side from the current matrix (Game<A>'s PM branches)
        long badKey = 0, badTarget = 0;
        const uint32_t bodies = scratch + 0x400;
        for (int n = 0; n < N; ++n) {
            setObj();
            m.set32(obj + 0x60, 1);           // flags bit 0 (arcade +0x58)
            m.at<int8_t>(obj + 0x2EF) = 3;    // an attack slot past 1
            m.set32(GP::attack_users_count, rnd() % 13u);
            const uint32_t at = GP::attack_users + 8 * m.u32(GP::attack_users_count);
            const float pos[3] = {m.f32(obj + 0x28), m.f32(obj + 0x2C), m.f32(obj + 0x30)};
            const float cam[3] = {m.f32(GP::camera_pos), m.f32(GP::camera_pos + 4), m.f32(GP::camera_pos + 8)};
            run(FP::attacker_list_add, obj, 0, {});
            badKey += int32_t(m.u32(at)) != pm::attacker_key(pos, cam) || m.u32(at + 4) != obj;

            setObj();
            pm::Matrix cm;
            randMatrix(cm.e);
            putMatrix(cur, cm.e);
            const int16_t players = int16_t(rnd() % 4u);
            m.at<int16_t>(GP::players_in_game) = players;
            const uint8_t side = uint8_t(rnd() & 1), solo = uint8_t(rnd() & 1);
            m.at<uint8_t>(GP::attack_side) = side;
            m.at<uint8_t>(GP::solo_player_is_p2) = solo;
            for (uint32_t p = 0; p < 2; ++p) {
                m.set32(GP::players + 0xBC + p * 0xCC, bodies + p * 0x100);
                for (uint32_t i = 0; i < 3; ++i)
                    m.set32(bodies + p * 0x100 + 0x28 + 4 * i, fb(randF()));
            }
            float ox = 0.0f, oy = 0.0f, oz = 0.0f;
            cm.transform(m.f32(obj + 0x28), m.f32(obj + 0x2C), m.f32(obj + 0x30), ox, oy, oz);
            const uint32_t sideOf = 0.0f <= ox ? 1u : 0u;
            uint32_t want[5];   // target x, y, z, slot wanted, players seen
            auto toBody = [&](uint32_t p) {
                const uint32_t body = bodies + p * 0x100;
                want[0] = m.u32(body + 0x28);
                want[1] = 0;
                want[2] = m.u32(body + 0x30);
            };
            if (players == 1) {
                toBody(solo);
                want[3] = side ? sideOf : 0;
            } else if (players == 2) {
                want[3] = sideOf;
                toBody(sideOf);
            } else {
                want[3] = side ? sideOf : 0;
                want[0] = m.u32(GP::camera_pos);
                want[1] = 0;
                want[2] = m.u32(GP::camera_pos + 8);
            }
            want[4] = uint32_t(int32_t(players));
            run(FP::enemy_target_distance, obj, 0, {});
            badTarget += m.u32(obj + 0x3A8) != want[0] || m.u32(obj + 0x3AC) != want[1] || m.u32(obj + 0x3B0) != want[2] ||
                         m.u32(obj + 0x340) != want[3] || m.u32(obj + 0x33C) != want[4];
        }
        // velocity_for_arc (obj eax, from edx, to ebx; gravity, frames), velocity_for_arc_speed (obj, from, to;
        // gravity, speed, &frames), attack_slot_nearest_object (obj) -> eax, camera_side_set (obj): +0x2F1
        long badArc = 0, badArcS = 0, badNear = 0, badSide = 0;
        const uint32_t from = scratch + 0x200, to = scratch + 0x210, holders = 0x31030000;
        if (!m.mapped(holders))
            m.map(holders, 0x4000);
        auto call3 = [&](uint32_t addr, uint32_t a, uint32_t b, uint32_t c, std::initializer_list<uint32_t> st) {
            const uint32_t regs[4] = {a, b, c, 0};
            std::vector<uint32_t> s(st);
            cpu.top = 0;
            cpu.cw = 0x107F;
            if (!cpu.call(addr, regs, s.data(), int(s.size()), cpuStack + 0xFF00) && firstErr.empty())
                firstErr = cpu.error + " (call3)";
        };
        for (int n = 0; n < N; ++n) {
            setObj();
            float f[3], t[3];
            for (int k = 0; k < 3; ++k) {
                f[k] = randF();
                t[k] = randF();
                m.set32(from + 4 * uint32_t(k), fb(f[k]));
                m.set32(to + 4 * uint32_t(k), fb(t[k]));
            }
            const float g = frand(0.0f, 0.2f), frames = float(1 + rnd() % 120u), speed = frand(0.1f, 4.0f);
            float v[3];
            pm::arc_velocity(f, t, g, frames, v);
            call3(FP::velocity_for_arc, obj, from, to, {fb(g), fb(frames)});
            badArc += m.u32(obj + 0x80) != fb(v[0]) || m.u32(obj + 0x84) != fb(v[1]) || m.u32(obj + 0x88) != fb(v[2]) ||
                      m.u32(obj + 0x90) != fb(-g) || m.u32(obj + 0x8C) != 0 || m.u32(obj + 0x94) != 0;
            const float fr = pm::arc_frames(f, t, speed);
            pm::arc_velocity(f, t, g, fr, v);
            m.set32(scratch + 0x220, 0x55555555);
            call3(FP::velocity_for_arc_speed, obj, from, to, {fb(g), fb(speed), scratch + 0x220});
            const bool ds = m.u32(obj + 0x80) != fb(v[0]) || m.u32(obj + 0x84) != fb(v[1]) || m.u32(obj + 0x88) != fb(v[2]) ||
                            int32_t(m.u32(scratch + 0x220)) != int32_t(std::trunc(double(fr)));
            if (ds && badArcS < 3)
                std::printf("    arc speed: PC v (%.9g %.9g %.9g) n %d, pm (%.9g %.9g %.9g) %.9g\n", m.f32(obj + 0x80),
                            m.f32(obj + 0x84), m.f32(obj + 0x88), int32_t(m.u32(scratch + 0x220)), v[0], v[1], v[2], fr);
            badArcS += ds;

            // attack positions 2..12 with holders (bit 18 set, bit 1 clear unless n says so)
            m.set32(GP::attack_users_last, rnd() % 12u);
            m.at<int8_t>(obj + 0x2EF) = int8_t(2 + rnd() % 11u);
            for (uint32_t i = 0; i < 13; ++i) {
                const uint32_t s = GP::attack_slots + i * 0x14, h = holders + i * 0x400;
                m.at<uint8_t>(s) = uint8_t((rnd() % 4u) != 0);
                m.set32(s + 4, (rnd() % 5u) != 0 ? h : 0);
                m.set32(h + 0x28, fb(randF()));
                m.set32(h + 0x30, fb(randF()));
                m.set32(h + 0x32C, ((rnd() & 3) != 0 ? 0x40000u : 0) | ((rnd() & 3) == 0 ? 2u : 0));
            }
            uint32_t best = 0;
            float bestD = 10000.0f;
            int32_t end = int32_t(m.u32(GP::attack_users_last)) + 2;
            if (end > 13)
                end = 13;
            for (int32_t i = 2; i < end; ++i) {
                const uint32_t s = GP::attack_slots + uint32_t(i) * 0x14, h = m.u32(s + 4);
                if (m.at<int8_t>(obj + 0x2EF) == i || h == 0 || m.at<uint8_t>(s) == 0 ||
                    (m.u32(h + 0x32C) & 0x40002u) != 0x40000u)
                    continue;
                const float d = pm::floor_distance(m.f32(h + 0x28), m.f32(h + 0x30), m.f32(obj + 0x28), m.f32(obj + 0x30));
                if (d < bestD) {
                    best = h;
                    bestD = d;
                }
            }
            run(FP::attack_slot_nearest_object, obj, 0, {});
            badNear += cpu.r[X86::EAX] != best;

            // camera_side_set: the arcade's form (kept in PM)
            setObj();
            const float ox = m.f32(obj + 0x28), oz = m.f32(obj + 0x30);
            const float cx = m.f32(GP::camera_pos), cz = m.f32(GP::camera_pos + 8);
            const int32_t dx = std::abs(int32_t(ox - cx)), dz = std::abs(int32_t(oz - cz));
            const int8_t side = dz < dx ? (cx < ox ? 0 : 1) : (cz < oz ? 2 : 3);
            run(FP::camera_side_set, obj, 0, {});
            badSide += m.at<int8_t>(obj + 0x2F1) != side;
        }
        report("velocity_for_arc", FP::velocity_for_arc, N, badArc);
        report("velocity_for_arc_speed", FP::velocity_for_arc_speed, N, badArcS);
        report("attack_slot_nearest_object", FP::attack_slot_nearest_object, N, badNear);
        report("camera_side_set (the arcade's = the PC's)", FP::camera_side_set, N, badSide);
        report("attacker_list_add", FP::attacker_list_add, N, badKey);
        report("enemy_target_distance", FP::enemy_target_distance, N, badTarget);
        report("move_along_heading", FP::move_along_heading, N, bad[0]);
        report("angle_to_camera", FP::angle_to_camera, N, bad[1]);
        report("angle_turn_toward (the arcade's = the PC's)", FP::angle_turn_toward, N, bad[2]);
        report("walk_to", FP::walk_to, N, bad[3]);
        report("turn_toward_camera", FP::turn_toward_camera, N, bad[4]);
    }
    // pmboss: the boss files' own helper copies (each file has the whole helpers block; offsets from its transform
    // copy), the PC's view test and matrix angles (every copy PM uses), the C library rand, tumble_corners, and the boss
    // routines whose PC rules PC Mode follows (pm forms restated from the PC code, docs/pcmode.md row 10)
    {
        struct Blk {
            const char* name;
            uint32_t base;
        };
        const Blk blocks[] = {{"chariot", 0x5134EC}, {"hangedman", 0x497070}, {"magician", 0x4CF570},
                              {"hermit", 0x4EB224},  {"kind17", 0x50C230},
                              // pmbody: the body drawing's file (body_draw 0x477B00, its part draw 0x476B70) and the
                              // common part hook's (0x51D014: the look turns)
                              {"body", 0x474CE8},    {"hook", 0x51AF1C},
                              // the camera file's (camera_look_at 0x461A30's vec_angles 0x45D300)
                              {"camera", 0x45D218},
                              // the barrel file's (also the rain drops 0x431678's transform 0x42F520)
                              {"barrel", 0x42F520},
                              // camdr_mode3_frame 0x4CA680's
                              {"camdr", 0x4C87C0},
                              // the hostages file's (hostage_s03_held 0x4AC89C)
                              {"hostages", 0x4A5AB8},
                              // spline_advance 0x4677FC's
                              {"spline", 0x465D04},
                              // obj_move_heading 0x4C589C's
                              {"move", 0x4C1B60},
                              // pmrest2: the iron ball file's (iron_ball_pos_swing 0x48D46C)
                              {"ironball", 0x48A630},
                              // pmch34: kind 162's file (kind162_door_frame 0x49EF78, obj_kind162_turn 0x49F268)
                              {"door", 0x49D0A0}};
        auto nm = [](const char* a, const char* b) {
            static std::string s;
            s = std::string(a) + " " + b;
            return s.c_str();
        };
        for (const Blk& b : blocks) {
            const int axisOf[3] = {1, 2, 0};
            const uint32_t rotOff[3] = {0x404, 0x50C, 0x698};
            const char* rotName[3] = {"rotate_y", "rotate_z", "rotate_x"};
            for (int r = 0; r < 3; ++r) {
                long bad = 0;
                for (int n = 0; n < N; ++n) {
                    pm::Matrix pm;
                    randMatrix(pm.e);
                    putMatrix(cur, pm.e);
                    const uint32_t a = randAngle();
                    run(b.base + rotOff[r], a, 0, {});
                    if (axisOf[r] == 0)
                        pm.rotate_x(a);
                    else if (axisOf[r] == 1)
                        pm.rotate_y(a);
                    else
                        pm.rotate_z(a);
                    bad += !sameMatrix(cur, pm.e);
                }
                report(nm(b.name, rotName[r]), b.base + rotOff[r], N, bad);
            }
            long badT = 0, badX = 0, badI = 0, badA = 0, badS = 0, badM = 0, badMS = 0;
            for (int n = 0; n < N; ++n) {
                pm::Matrix pm;
                randMatrix(pm.e);
                putMatrix(cur, pm.e);
                float x = randF(), y = randF(), z = randF();
                run(b.base + 0x1E4, 0, 0, {fb(x), fb(y), fb(z)});
                pm.translate(x, y, z);
                badT += !sameMatrix(cur, pm.e);

                randMatrix(pm.e);
                putMatrix(cur, pm.e);
                x = randF(), y = randF(), z = randF();
                run(b.base, 0, 0, {fb(x), fb(y), fb(z), scratch, scratch + 4, scratch + 8});
                float o[3];
                pm.transform(x, y, z, o[0], o[1], o[2]);
                badX += m.u32(scratch) != fb(o[0]) || m.u32(scratch + 4) != fb(o[1]) || m.u32(scratch + 8) != fb(o[2]);

                if (n < 100) {
                    randMatrix(pm.e);
                    putMatrix(cur, pm.e);
                    run(b.base + 0xA54, 0, 0, {});
                    pm.identity();
                    badI += !sameMatrix(cur, pm.e);
                }

                x = randF(), y = (n & 1) ? 0.0f : randF(), z = randF();
                run(b.base + 0xE8, 0, 0, {fb(x), fb(y), fb(z), scratch, scratch + 4});
                uint32_t yaw = 0, pitch = 0;
                pm::vec_angles(x, y, z, yaw, pitch);
                badA += m.u32(scratch) != yaw || m.u32(scratch + 4) != pitch;

                const float q = (n & 1) ? frand(0.0f, 4.0f) : frand(0.0f, 1.0e6f);
                run(b.base + 0x83C, 0, 0, {fb(q)});
                badS += fb(cpu.st0f()) != fb(pm::fast_sqrt(q));

                float other[12];
                randMatrix(pm.e);
                putMatrix(cur, pm.e);
                randMatrix(other);
                putMatrix(scratch + 0x100, other);
                run(b.base + 0xAB0, scratch + 0x100, 0, {});
                pm.mul(other);
                badM += !sameMatrix(cur, pm.e);

                randMatrix(pm.e);
                putMatrix(cur, pm.e);
                run(b.base + 0xC54, scratch + 0x100, 0, {});
                pm.mul_stored(other);
                badMS += !sameMatrix(cur, pm.e);
            }
            report(nm(b.name, "translate"), b.base + 0x1E4, N, badT);
            report(nm(b.name, "transform"), b.base, N, badX);
            report(nm(b.name, "identity"), b.base + 0xA54, 100, badI);
            report(nm(b.name, "vec_angles"), b.base + 0xE8, N, badA);
            report(nm(b.name, "fast_sqrt"), b.base + 0x83C, N, badS);
            report(nm(b.name, "mul (pc_matrix_mul8)"), b.base + 0xAB0, N, badM);
            report(nm(b.name, "mul_stored"), b.base + 0xC54, N, badMS);
            // pmbody: the block's inverse (+0xE80) and its yaw / pitch angles (+0x1250: eax pitch, edx yaw, ebx c)
            long badInv = 0, badYp = 0;
            for (int n = 0; n < N; ++n) {
                pm::Matrix pm;
                if (n % 3 != 0) {   // a body's matrices: turns, a scale, a translation
                    pm.rotate_z(randAngle());
                    pm.rotate_y(randAngle());
                    pm.rotate_x(randAngle());
                    const float sc = (n & 1) ? 1.0f : frand(0.2f, 3.0f);
                    for (int k = 0; k < 9; ++k)
                        pm.e[k] = pm.e[k] * sc;
                    pm.translate(randF(), randF(), randF());
                } else {
                    randMatrix(pm.e);
                }
                putMatrix(cur, pm.e);
                run(b.base + 0xE80, 0, 0, {});
                pm::Matrix inv = pm;
                inv.invert();
                badInv += !sameMatrix(cur, inv.e);
                putMatrix(cur, pm.e);
                const uint32_t regs[4] = {scratch + 0x40, scratch + 0x44, scratch + 0x48, 0};
                cpu.top = 0;
                cpu.cw = 0x107F;
                if (!cpu.call(b.base + 0x1250, regs, nullptr, 0, cpuStack + 0xFF00) && firstErr.empty())
                    firstErr = cpu.error + " (matrix angles yp)";
                if (m.u32(GP::pc_matrix_ptr) != cur && firstErr.empty())
                    firstErr = "matrix angles yp moved the matrix pointer";
                m.set32(GP::pc_matrix_ptr, cur);
                uint32_t yaw = 0, pitch = 0, c = 0;
                pm::matrix_angles_yp(pm, yaw, pitch, c);
                badYp += m.u32(scratch + 0x40) != pitch || m.u32(scratch + 0x44) != yaw || m.u32(scratch + 0x48) != c;
            }
            long badCos = 0;   // the block's vec_cos (+0x914: the camera's look weight 0x461690 uses 0x45DB2C)
            for (int n = 0; n < N; ++n) {
                float v[6];
                for (float& f : v)
                    f = randF();
                run(b.base + 0x914, 0, 0, {fb(v[0]), fb(v[1]), fb(v[2]), fb(v[3]), fb(v[4]), fb(v[5])});
                badCos += fb(cpu.st0f()) != fb(pm::vec_cos(v[0], v[1], v[2], v[3], v[4], v[5]));
            }
            report(nm(b.name, "vec_cos"), b.base + 0x914, N, badCos);
            report(nm(b.name, "invert"), b.base + 0xE80, N, badInv);
            report(nm(b.name, "matrix_angles_yp"), b.base + 0x1250, N, badYp);
        }
        // the view test (stack x, y, z, r -> st0 +1 / -1): stored matrix 0, the projection constants 0xD53058 /
        // 0xD5305C and the viewport rectangle at *0xD5A5C0 + 0x20 (pm::view_test's values)
        const uint32_t vp = scratch + 0x600;
        if (!m.mapped(0xD5A5C0))
            m.map(0xD5A000, 0x1000);
        m.set32(0xD53058, fb(640.0f));
        m.set32(0xD5305C, fb(-620.0f));
        m.set32(0xD5A5C0, vp);
        m.set32(vp + 0x20, uint32_t(-320));
        m.set32(vp + 0x24, uint32_t(-240));
        m.set32(vp + 0x28, 320);
        m.set32(vp + 0x2C, 240);
        std::vector<std::pair<std::string, uint32_t>> views = {{"pc_view_test", 0x4A2808}, {"pc_view_test2", 0x4E3124},
                                                               {"barrel view_test", 0x430BD8}};   // (pmrest)
        for (const Blk& b : blocks)
            views.push_back({std::string(b.name) + " view_test", b.base + 0x16B8});
        long seenIn = 0;
        for (const auto& v : views) {
            long bad = 0;
            for (int n = 0; n < N; ++n) {
                pm::Matrix cam;
                cam.rotate_z(randAngle() & 0x0FFF);
                cam.rotate_x(randAngle() & 0x1FFF);
                cam.rotate_y(randAngle());
                cam.translate(randF(), frand(-50.0f, 50.0f), randF());
                putMatrix(GP::pc_stored_matrices, cam.e);
                float p[3];
                const float r = (n % 5 == 0) ? 0.0f : frand(0.0f, 60.0f);
                if (n & 1) {   // near the line of sight: in, on or just past the edges
                    // the world point that `cam` (rotation rows, then the translation) takes to (vx, vy, vz)
                    const float vz = frand(-20.0f, 400.0f), vx = vz * frand(-0.7f, 0.7f), vy = vz * frand(-0.6f, 0.6f);
                    const float tx = vx - cam.e[9], ty = vy - cam.e[10], tz = vz - cam.e[11];
                    p[0] = tx * cam.e[0] + ty * cam.e[3] + tz * cam.e[6];
                    p[1] = tx * cam.e[1] + ty * cam.e[4] + tz * cam.e[7];
                    p[2] = tx * cam.e[2] + ty * cam.e[5] + tz * cam.e[8];
                } else {
                    p[0] = randF(), p[1] = randF(), p[2] = randF();
                }
                run(v.second, 0, 0, {fb(p[0]), fb(p[1]), fb(p[2]), fb(r)});
                const float want = pm::view_test(cam.e, p[0], p[1], p[2], r);
                seenIn += want > 0.0f;
                bad += fb(cpu.st0f()) != fb(want);
            }
            report(v.first.c_str(), v.second, N, bad);
        }
        if (seenIn == 0)
            std::printf("    view tests: no case was on screen\n");
        // pc_matrix_angles8 and its copies (outA eax, outB edx, outC ebx)
        std::vector<std::pair<std::string, uint32_t>> matAngles = {{"pc_matrix_angles8", 0x4E2E84}};
        for (const Blk& b : blocks)
            matAngles.push_back({std::string(b.name) + " matrix_angles", b.base + 0x1418});
        for (const auto& v : matAngles) {
            long bad = 0;
            for (int n = 0; n < N; ++n) {
                pm::Matrix mm;
                if (n & 1) {
                    mm.rotate_z(randAngle());
                    mm.rotate_y(randAngle());
                    mm.rotate_x(randAngle());
                    mm.translate(randF(), randF(), randF());
                } else {
                    randMatrix(mm.e);
                }
                putMatrix(cur, mm.e);
                const uint32_t regs[4] = {scratch + 0x40, scratch + 0x44, scratch + 0x48, 0};
                cpu.top = 0;
                cpu.cw = 0x107F;
                if (!cpu.call(v.second, regs, nullptr, 0, cpuStack + 0xFF00) && firstErr.empty())
                    firstErr = cpu.error + " (matrix angles)";
                m.set32(GP::pc_matrix_ptr, cur);
                uint32_t a = 0, b2 = 0, c = 0;
                pm::matrix_angles8(mm, a, b2, c);
                bad += m.u32(scratch + 0x40) != a || m.u32(scratch + 0x44) != b2 || m.u32(scratch + 0x48) != c;
            }
            report(v.first.c_str(), v.second, N, bad);
        }
        // pmbody: the camera heading 0x4C9D28 (eax yawRef, edx &c, ebx &h0, ecx &h1)
        {
            long bad = 0;
            for (int n = 0; n < N; ++n) {
                pm::Matrix mm;
                mm.rotate_y(randAngle());
                mm.rotate_x(randAngle());
                mm.rotate_z(randAngle());
                mm.rotate_x(randAngle() & 0x1FFF);
                mm.rotate_y(randAngle());
                if (n % 4 == 0)
                    randMatrix(mm.e);
                putMatrix(cur, mm.e);
                const uint32_t yref = randAngle();
                const uint32_t regs[4] = {yref, scratch + 0x40, scratch + 0x48, scratch + 0x44};   // eax, edx, ebx, ecx
                cpu.top = 0;
                cpu.cw = 0x107F;
                if (!cpu.call(0x4C9D28, regs, nullptr, 0, cpuStack + 0xFF00) && firstErr.empty())
                    firstErr = cpu.error + " (camera heading)";
                m.set32(GP::pc_matrix_ptr, cur);
                uint32_t h0 = 0, h1 = 0, c = 0;
                pm::camera_heading(mm, yref, h0, h1, c);
                bad += m.u32(scratch + 0x40) != c || m.u32(scratch + 0x48) != h0 || m.u32(scratch + 0x44) != h1;
            }
            report("camera heading (camdr_mode3_frame)", 0x4C9D28, N, bad);
        }
        // pmrest2: the game camera's look mode 2 (0x45FF68 from 0x46009B to 0x4600EB: edx the record's eye, ebx its
        // look-at; yaw / roll 0, the pitch from pc_atan2 and the camera file's table square root)
        {
            long bad = 0;
            const uint32_t rec = scratch + 0x100, at = scratch + 0x140;
            for (int n = 0; n < N; ++n) {
                float eye[3], look[3];
                for (int k = 0; k < 3; ++k) {
                    eye[k] = randF();
                    look[k] = (n % 8 == 0) ? eye[k] : randF();
                }
                for (int k = 0; k < 3; ++k) {
                    m.set32(rec + 4 * uint32_t(k), fb(eye[k]));
                    m.set32(at + 4 * uint32_t(k), fb(look[k]));
                }
                m.set32(rec + 0xC, rnd());
                m.set32(rec + 0x10, rnd());
                m.set32(rec + 0x14, rnd());
                const uint32_t regs[4] = {0, rec, at, 0};
                std::vector<uint32_t> s(4, 0);
                cpu.top = 0;
                cpu.cw = 0x107F;
                cpu.stopAt = 0x4600EB;
                if (!cpu.call(0x46009B, regs, s.data(), int(s.size()), cpuStack + 0xFF00) && firstErr.empty())
                    firstErr = cpu.error + " (camera look mode 2)";
                cpu.stopAt = 0xFFFFFFFFu;
                m.set32(GP::pc_matrix_ptr, cur);
                bad += m.u32(rec + 0xC) != pm::look2_pitch(eye, look) || m.u32(rec + 0x10) != 0 || m.u32(rec + 0x14) != 0;
            }
            report("camera look mode 2 (0x45FF68 at 0x46009B)", 0x46009B, N, bad);
        }
        // pmrest2: the heads' eye beam 0x51E258 (the camera_pos 0xD9F038, 15 up) and target beam 0x51E4C8 (the object
        // at +0x398, its +0x28.., 15 up): on the current matrix, stored matrix 1, the look angles +0x14C / +0x150
        {
            const uint32_t body = scratch + 0x400, target = scratch + 0xC00, camPos = 0xD9F038;
            if (!m.mapped(camPos))
                m.map(camPos & ~0xFFFu, 0x1000);
            for (int beam = 0; beam < 2; ++beam) {
                long bad = 0;
                for (int n = 0; n < N; ++n) {
                    pm::Matrix part, cam;
                    part.rotate_y(randAngle());
                    part.rotate_x(randAngle());
                    part.rotate_z(randAngle());
                    part.translate(randF(), randF(), randF());
                    if (n % 4 == 0) {   // a body's scale (a singular random matrix would only compare NaN handling)
                        const float k = frand(0.3f, 3.0f);
                        for (int i = 0; i < 9; ++i)
                            part.e[i] = part.e[i] * k;
                    }
                    cam.rotate_y(randAngle());
                    cam.rotate_x(randAngle() & 0x1FFF);
                    cam.translate(randF(), randF(), randF());
                    putMatrix(cur, part.e);
                    putMatrix(GP::pc_stored_matrices + 0x30, cam.e);
                    const float p[3] = {randF(), randF(), randF()};
                    const uint32_t at = beam == 0 ? camPos : target + 0x28;
                    for (int k = 0; k < 3; ++k)
                        m.set32(at + 4 * uint32_t(k), fb(p[k]));
                    m.set32(body + 0x398, target);
                    uint32_t cy = (n & 1) ? rnd() & 0xFFFF : (n & 2) ? 0x2000u : 0u, cp = (n & 4) ? rnd() & 0xFFFF : 0xE000u;
                    m.set32(body + 0x14C, cy);
                    m.set32(body + 0x150, cp);
                    const uint32_t regs[4] = {body, 0, 0, 0};
                    cpu.top = 0;
                    cpu.cw = 0x107F;
                    if (!cpu.call(beam == 0 ? 0x51E258 : 0x51E4C8, regs, nullptr, 0, cpuStack + 0xFF00) && firstErr.empty())
                        firstErr = cpu.error + " (eye / target beam)";
                    const bool moved = m.u32(GP::pc_matrix_ptr) != cur;
                    m.set32(GP::pc_matrix_ptr, cur);
                    pm::look_turn(part, cam.e, p[0], p[1] + 15.0f, p[2], cy, cp);
                    bad += moved || m.u32(body + 0x14C) != cy || m.u32(body + 0x150) != cp || !sameMatrix(cur, part.e);
                }
                report(beam == 0 ? "eye beam (part hook)" : "target beam (part hook)", beam == 0 ? 0x51E258 : 0x51E4C8, N, bad);
            }
        }
        // pmrest2: draw type 1's part routine 0x477EC4 (a leaf part of the body type's home class: its offset and
        // joint turns, the home place through stored matrix 1 into the body's +0x2DC..; the model draw 0x43D7F8 a ret)
        {
            const uint32_t body = scratch + 0x400, part = scratch + 0x800, frame = scratch + 0xA00;
            int htype = -1, hjoint = -1;
            for (int ty = 0; ty < 0x40 && htype < 0; ++ty)
                for (int j = 0; j < 31; ++j)
                    if (int32_t(int8_t(*m.ptr(0x523A58 + uint32_t(ty * 155 + j * 5 + 3)))) == int32_t(m.u32(0x55375C + 4 * uint32_t(ty)))) {
                        htype = ty, hjoint = j;
                        break;
                    }
            const uint8_t savedOp = *m.ptr(0x43D7F8);
            const uint32_t savedShadow = m.u32(0x5CD36C), savedBody = m.u32(0xDA0BB8), savedFrame = m.u32(0xDA1140);
            *m.ptr(0x43D7F8) = 0xC3;   // ret
            m.set32(0x5CD36C, 0);
            long bad = htype < 0 ? N : 0;
            for (int n = 0; n < N && htype >= 0; ++n) {
                pm::Matrix mm, cam;
                mm.rotate_y(randAngle());
                mm.rotate_x(randAngle());
                mm.translate(randF(), randF(), randF());
                cam.rotate_y(randAngle());
                cam.rotate_x(randAngle() & 0x1FFF);
                cam.translate(randF(), randF(), randF());
                putMatrix(cur, mm.e);
                putMatrix(GP::pc_stored_matrices + 0x30, cam.e);
                m.set32(0xDA0BB8, body);
                *m.ptr(body + 0xD4) = uint8_t(htype);
                *m.ptr(body + 0xD5) = 0;
                const float o[3] = {randF(), randF(), randF()};
                for (int k = 0; k < 3; ++k)
                    m.set32(part + 4 + 4 * uint32_t(k), fb(o[k]));
                m.set32(part, 0x100);
                m.set32(part + 0x34, 0);
                m.set32(part + 0x38, uint32_t(hjoint));
                m.set32(0xDA1140, frame);
                const uint16_t a[3] = {uint16_t(rnd()), uint16_t(rnd()), uint16_t(rnd())};
                for (int k = 0; k < 3; ++k) {
                    *m.ptr(frame + 0xC + uint32_t(hjoint) * 6 + 2 * uint32_t(k)) = uint8_t(a[k]);
                    *m.ptr(frame + 0xD + uint32_t(hjoint) * 6 + 2 * uint32_t(k)) = uint8_t(a[k] >> 8);
                }
                run(0x477EC4, part, 0, {});
                pm::draw1_part(mm, o[0], o[1], o[2], 0u - a[2], a[1], a[0]);
                float h[3];
                pm::draw1_home(mm, cam.e, h);
                bad += m.u32(body + 0x2DC) != fb(h[0]) || m.u32(body + 0x2E0) != fb(h[1]) || m.u32(body + 0x2E4) != fb(h[2]);
            }
            *m.ptr(0x43D7F8) = savedOp;
            m.set32(0x5CD36C, savedShadow);
            m.set32(0xDA0BB8, savedBody);
            m.set32(0xDA1140, savedFrame);
            report("draw type 1 part (home place)", 0x477EC4, N, bad);
        }
        // the C library rand 0x578A6B: the seed through the thread data (a stub for the [0xC67784] getter returning
        // seed - 0xC), a sequence from random seeds
        {
            const uint32_t stub = scratch + 0x800, seedAt = scratch + 0x900;
            if (!m.mapped(0xC67784))
                m.map(0xC67000, 0x1000);
            const uint32_t saved = m.u32(0xC67784);
            m.set32(0xC67784, stub);
            *m.ptr(stub) = 0xB8;   // mov eax, seedAt - 0xC
            m.set32(stub + 1, seedAt - 0xC);
            *m.ptr(stub + 5) = 0xC3;   // ret
            long bad = 0, cases = 0;
            for (int s = 0; s < 50; ++s) {
                uint32_t seedPm = s == 0 ? 1u : rnd() ^ (rnd() << 16);
                m.set32(seedAt, seedPm);
                for (int n = 0; n < 2000; ++n, ++cases) {
                    run(0x578A6B, 0, 0x5A5A5A5A, {});
                    const uint32_t want = pm::crand(seedPm);
                    bad += cpu.r[X86::EAX] != want || m.u32(seedAt) != seedPm || cpu.r[X86::EDX] != 0x5A5A5A5A;
                }
            }
            m.set32(0xC67784, saved);
            report("crand (the C library rand)", 0x578A6B, cases, bad);
        }
        // tumble_corners 0x4C5924 (box eax): the 8 corners through the current matrix into 0xDA61E0..
        {
            if (!m.mapped(0xDA61E0))
                m.map(0xDA6000, 0x1000);
            long bad = 0;
            const uint32_t box = scratch + 0x300;
            for (int n = 0; n < N; ++n) {
                pm::Matrix mm;
                randMatrix(mm.e);
                putMatrix(cur, mm.e);
                for (uint32_t k = 0; k < 6; ++k)
                    m.set32(box + 4 * k, fb(randF()));
                run(0x4C5924, box, 0, {});
                uint32_t i = 0;
                bool d = false;
                for (uint32_t xo : {0u, 4u})
                    for (uint32_t yo : {8u, 0xCu})
                        for (uint32_t zo : {0x10u, 0x14u}) {
                            float o[3];
                            mm.transform(m.f32(box + xo), m.f32(box + yo), m.f32(box + zo), o[0], o[1], o[2]);
                            for (uint32_t k = 0; k < 3; ++k)
                                d |= m.u32(0xDA61E0 + 12 * i + 4 * k) != fb(o[k]);
                            ++i;
                        }
                bad += d;
            }
            report("tumble_corners", 0x4C5924, N, bad);
        }
        // the boss routines (PC layout: position +0x28, angles +0x34, state words +0x64 / +0x68 / +0x6E)
        {
            const uint32_t obj = 0x31020000, blk = scratch + 0xA00, table = scratch + 0xB00;
            if (!m.mapped(0xDA0220))
                m.map(0xDA0000, 0x2000);
            if (!m.mapped(GP::camera_pos))
                m.map(GP::camera_pos & ~0xFFFu, 0x1000);
            auto clearObj = [&]() {
                for (uint32_t k = 0; k < 0x400; k += 4)
                    m.set32(obj + k, 0);
                for (uint32_t i = 0; i < 3; ++i)
                    m.set32(obj + 0x28 + 4 * i, fb(randF()));
            };
            long b14 = 0, b12 = 0, bhr = 0, bhp = 0;
            for (int n = 0; n < N; ++n) {
                // chariot act 14 (0x516780): the yaw to camera record 0's eye
                clearObj();
                for (uint32_t i = 0; i < 3; ++i)
                    m.set32(0xDA0220 + 4 * i, fb(randF()));
                run(0x516780, obj, 0, {});
                uint32_t yaw = 0, pitch = 0;
                pm::vec_angles(m.f32(0xDA0220) - m.f32(obj + 0x28), m.f32(0xDA0224) - m.f32(obj + 0x2C),
                               m.f32(0xDA0228) - m.f32(obj + 0x30), yaw, pitch);
                b14 += m.u32(obj + 0x38) != yaw || m.at<uint16_t>(obj + 0x68) != 0;
                // chariot act 12 (0x5167CC): its back to point [+0x324] of the script's table (block +0x20)
                clearObj();
                m.set32(obj + 0x384, blk);
                m.set32(blk + 0x20, table);
                const uint32_t k = rnd() % 4u;
                m.set32(obj + 0x324, k);
                for (uint32_t i = 0; i < 3; ++i)
                    m.set32(table + 12 * k + 4 * i, fb(randF()));
                run(0x5167CC, obj, 0, {});
                pm::vec_angles(m.f32(table + 12 * k) - m.f32(obj + 0x28), m.f32(table + 12 * k + 4) - m.f32(obj + 0x2C),
                               m.f32(table + 12 * k + 8) - m.f32(obj + 0x30), yaw, pitch);
                b12 += m.u32(obj + 0x38) != yaw + 0x8000;
                // chariot hit reaction (0x5169CC, step 1, motion 0 at frame 0): pushed back in the camera's frame
                clearObj();
                m.set32(obj + 0x384, blk);
                m.at<uint16_t>(obj + 0x6E) = 1;
                const uint32_t cyaw = randAngle();
                m.set32(0xDA0230, cyaw);
                m.set32(0xDA0220, fb(randF()));
                m.set32(0xDA0228, fb(randF()));
                const uint8_t ch = uint8_t(rnd() % 4u);
                m.at<uint8_t>(0xDA0CF4) = ch;
                const float px = m.f32(obj + 0x28), pz = m.f32(obj + 0x30);
                run(0x5169CC, obj, 0, {});
                {
                    const float cx = m.f32(0xDA0220), cz = m.f32(0xDA0228);
                    pm::Matrix mm;
                    mm.rotate_y(cyaw);
                    float l[3];
                    mm.transform(px - cx, 0.0f, pz - cz, l[0], l[1], l[2]);
                    float wx = px, wz = pz;
                    auto back = [&](float x, float z) {
                        pm::Matrix nn;
                        nn.rotate_y(0u - cyaw);
                        float o[3];
                        nn.transform(x, 0.0f, z, o[0], o[1], o[2]);
                        wx = cx + o[0];
                        wz = cz + o[2];
                    };
                    if (!(l[0] >= -40.0f))
                        back(-40.0f, l[2]);
                    if (ch == 0 && l[2] > 80.0f)
                        back(l[0], 80.0f);
                    else if (ch == 3 && !(l[2] >= -100.0f))
                        back(l[0], -100.0f);
                    bhr += m.u32(obj + 0x28) != fb(wx) || m.u32(obj + 0x30) != fb(wz) || m.u32(blk) != 1;
                }
                // Hangedman hover point (0x49CAD0: obj eax, path edx; frame, out): camera_path_eval stubbed to give
                // the eye point; pattern parity, event 15 or not
                {
                    const uint32_t eyeAt = scratch + 0xC00, outAt = scratch + 0xC40;
                    float eye[3] = {frand(-40.0f, 40.0f), frand(-40.0f, 40.0f), frand(-40.0f, 40.0f)};
                    for (uint32_t i = 0; i < 3; ++i)
                        m.set32(eyeAt + 4 * i, fb(eye[i]));
                    uint8_t savedCode[32];
                    std::memcpy(savedCode, m.ptr(0x461980), 32);
                    const uint8_t stubCode[] = {0x51,                                // push ecx
                                                0x8B, 0x4C, 0x24, 0x0C,              // mov ecx, [esp + 0xC] (eye)
                                                0xA1, 0, 0, 0, 0,                    // mov eax, [eyeAt]
                                                0x89, 0x01,                          // mov [ecx], eax
                                                0xA1, 0, 0, 0, 0,                    // mov eax, [eyeAt + 4]
                                                0x89, 0x41, 0x04,                    // mov [ecx + 4], eax
                                                0xA1, 0, 0, 0, 0,                    // mov eax, [eyeAt + 8]
                                                0x89, 0x41, 0x08,                    // mov [ecx + 8], eax
                                                0x59,                                // pop ecx
                                                0xC2, 0x14, 0x00};                   // ret 0x14
                    std::memcpy(m.ptr(0x461980), stubCode, sizeof stubCode);
                    m.set32(0x461980 + 6, eyeAt);
                    m.set32(0x461980 + 13, eyeAt + 4);
                    m.set32(0x461980 + 21, eyeAt + 8);
                    clearObj();
                    const uint32_t pat = rnd() & 0xFF;
                    m.set32(obj + 0x328, pat);
                    const bool ev15 = (rnd() & 1) != 0;
                    m.at<uint8_t>(0xDA0CF3) = ev15 ? 15 : 3;
                    for (uint32_t i = 0; i < 3; ++i)
                        m.set32(GP::camera_pos + 4 * i, fb(randF()));
                    run(0x49CAD0, obj, 0x42, {0, outAt});
                    std::memcpy(m.ptr(0x461980), savedCode, 32);
                    pm::Matrix mm;
                    mm.e[9] = m.f32(GP::camera_pos);
                    mm.e[10] = m.f32(GP::camera_pos + 4);
                    mm.e[11] = m.f32(GP::camera_pos + 8);
                    float x, y, z = eye[2] * 2.0f + 100.0f;
                    if (pat & 1) {
                        mm.rotate_y(ev15 ? 0xFFFFC000u : 0x4000u);
                        x = eye[0] * 2.0f;
                        y = eye[1] * 2.0f + (ev15 ? 30.0f : -5.0f);
                    } else if (ev15) {
                        mm.rotate_y(0xFFFFF800u);
                        y = eye[1] * 2.0f + 20.0f;
                        x = eye[0] * 2.0f + 30.0f;
                    } else {
                        mm.rotate_y(0x8000u);
                        y = eye[1] * 2.0f + 10.0f;
                        x = (eye[0] * 2.0f + 40.0f) + 30.0f;
                    }
                    float o[3];
                    mm.transform(x, y, z, o[0], o[1], o[2]);
                    const bool d = m.u32(outAt) != fb(o[0]) || m.u32(outAt + 4) != fb(o[1]) || m.u32(outAt + 8) != fb(o[2]);
                    if (d && bhp < 3)
                        std::printf("    hover point (pattern %u, event %d): PC (%.9g %.9g %.9g), pm (%.9g %.9g %.9g)\n", pat,
                                    ev15 ? 15 : 3, m.f32(outAt), m.f32(outAt + 4), m.f32(outAt + 8), o[0], o[1], o[2]);
                    bhp += d;
                }
            }
            report("chariot act 14 (face the players)", 0x516780, N, b14);
            report("chariot act 12 (turn to a point)", 0x5167CC, N, b12);
            report("chariot hit reaction (the PC's push-back limits)", 0x5169CC, N, bhr);
            report("hangedman hover point (scale 2, odd patterns, +30)", 0x49CAD0, N, bhp);
        }
        // whole routines with the game's rand 0x471828 and task_create 0x43E400 stubbed (rand: the next word of a
        // table; task_create: a fixed task block): the Magician's fireball aim 0x4D3DEC and the Hermit's spit (action
        // 20, step 2: one ball, 0x4EE4C0 up to hermit_hits_and_draw 0x4EEB34)
        {
            const uint32_t obj = 0x31020000, blk = scratch + 0xA00, task = 0x31030000;
            const uint32_t randCnt = scratch + 0xD00, randTbl = scratch + 0xD10;
            if (!m.mapped(task))
                m.map(task, 0x1000);
            if (!m.mapped(0xDA0B5C))
                m.map(0xDA0000, 0x2000);
            uint8_t savedTask[8];
            std::memcpy(savedTask, m.ptr(0x43E400), sizeof savedTask);
            const uint8_t randStub[] = {0xA1, 0, 0, 0, 0,               // mov eax, [randCnt]
                                        0xFF, 0x05, 0, 0, 0, 0,         // inc dword [randCnt]
                                        0x8B, 0x04, 0x85, 0, 0, 0, 0,   // mov eax, [eax * 4 + randTbl]
                                        0xC3};                          // ret
            static_assert(sizeof randStub <= 24, "rand is 24 bytes");
            uint8_t savedRandAll[24];
            std::memcpy(savedRandAll, m.ptr(0x471828), sizeof savedRandAll);
            std::memcpy(m.ptr(0x471828), randStub, sizeof randStub);
            m.set32(0x471828 + 1, randCnt);
            m.set32(0x471828 + 7, randCnt);
            m.set32(0x471828 + 14, randTbl);
            *m.ptr(0x43E400) = 0xB8;   // mov eax, task
            m.set32(0x43E401, task);
            *m.ptr(0x43E405) = 0xC3;   // ret
            auto setRand = [&](uint32_t a, uint32_t b) {
                m.set32(randCnt, 0);
                m.set32(randTbl, a);
                m.set32(randTbl + 4, b);
            };
            long bAim = 0, bSpit = 0;
            for (int n = 0; n < N; ++n) {
                pm::Matrix st;
                st.rotate_z(randAngle() & 0x0FFF);
                st.rotate_x(randAngle() & 0x1FFF);
                st.rotate_y(randAngle());
                st.translate(randF(), randF(), randF());
                // the Magician's fireball aim (PC layout: kind +0x334, count +0x33C, out +0x3A8; the arcade's +0x32C /
                // +0x334 / +0x3A0): restated from magician_fireball_aim's PC Mode form
                {
                    putMatrix(GP::pc_stored_matrices + 0x30, st.e);
                    for (uint32_t k = 0; k < 0x400; k += 4)
                        m.set32(obj + k, 0);
                    const int32_t kind = int32_t(rnd() % 5u) - 1, cnt = int32_t(rnd() % 7u);
                    const uint16_t players = uint16_t(1 + (rnd() & 1));
                    m.set32(obj + 0x334, uint32_t(kind));
                    m.set32(obj + 0x33C, uint32_t(cnt));
                    m.at<uint16_t>(0xDA0B5C) = players;
                    const uint32_t r1 = rnd(), r2 = rnd();
                    setRand(r1, r2);
                    run(0x4D3DEC, obj, 0, {});
                    float ox = 0.0f, oy = 0.0f;
                    const int32_t k = players == 2 ? 2 : kind;
                    if (k >= 0 && k <= 2) {
                        if (k <= 1)
                            ox = cnt == 1 ? -124.0f : cnt == 2 ? 124.0f : 0.0f;
                        else
                            ox = cnt == 2 ? -82.0f : cnt == 1 ? -164.0f : cnt == 3 ? 82.0f : cnt == 4 ? 164.0f : 0.0f;
                        oy = 32.0f;
                    }
                    const float x = float(int32_t(7 - (r1 & 15))) + ox;
                    const float y = float(int32_t(31 - (r2 & 0x3F))) + oy;
                    const float px = (x * 4.0f) * 0.002016129f, py = (y * 4.0f) * 0.002016129f;
                    float o[3];
                    st.transform(px, py, 4.0f, o[0], o[1], o[2]);
                    const bool d = m.u32(obj + 0x3A8) != fb(o[0]) || m.u32(obj + 0x3AC) != fb(o[1]) ||
                                   m.u32(obj + 0x3B0) != fb(o[2]);
                    if (d && bAim < 3)
                        std::printf("    fireball aim (kind %d, count %d, players %u): PC (%.9g %.9g %.9g), pm (%.9g %.9g "
                                    "%.9g)\n", kind, cnt, players, m.f32(obj + 0x3A8), m.f32(obj + 0x3AC),
                                    m.f32(obj + 0x3B0), o[0], o[1], o[2]);
                    bAim += d;
                }
                // the Hermit's spit (PC layout: step +0x6E, counters +0x324 / +0x32C / +0x330, scale +0x9C; the ball's
                // position +0x28, velocity +0x80, scale +0x9C): restated from hermit_act20_spit's PC Mode form
                {
                    putMatrix(GP::pc_stored_matrices + 6 * 0x30, st.e);
                    for (uint32_t k = 0; k < 0x400; k += 4) {
                        m.set32(obj + k, 0);
                        m.set32(task + k, 0);
                    }
                    m.set32(obj + 0x384, blk);
                    m.at<uint16_t>(obj + 0x6E) = 2;
                    m.set32(obj + 0x32C, 1);
                    m.set32(obj + 0x330, 2 + (rnd() & 3));
                    const float scale = frand(0.5f, 2.0f);
                    m.set32(obj + 0x9C, fb(scale));
                    float mouth[3];
                    for (uint32_t i = 0; i < 3; ++i) {
                        mouth[i] = (n & 1) ? frand(-40.0f, 40.0f) : randF();
                        m.set32(blk + 0x190 + 4 * i, fb(mouth[i]));
                    }
                    const uint32_t r1 = rnd(), r2 = rnd();
                    setRand(r1, r2);
                    cpu.stopAt = 0x4EEB34;
                    run(0x4EE4C0, obj, 0, {});
                    cpu.stopAt = 0xFFFFFFFFu;
                    const float x = (float(int32_t(r1 & 0xFF)) + -127.5f) * 15.56f * 0.002016129f;
                    const float y = (float(int32_t(r2 & 0xFF)) + -127.5f) * 11.67f * 0.002016129f;
                    float w[3];
                    st.transform(x, y, 10.0f, w[0], w[1], w[2]);
                    const float d[3] = {w[0] - mouth[0], w[1] - mouth[1], w[2] - mouth[2]};
                    float u[3];
                    pm::normalize(d, u);
                    bool bad = m.u32(task + 0x9C) != fb(scale * 0.3f);
                    for (uint32_t i = 0; i < 3; ++i)
                        bad |= m.u32(task + 0x28 + 4 * i) != fb(mouth[i]) || m.u32(task + 0x80 + 4 * i) != fb(u[i] * 1.0f);
                    if (bad && bSpit < 3)
                        std::printf("    hermit spit: PC v (%.9g %.9g %.9g) s %.9g, pm v (%.9g %.9g %.9g) s %.9g\n",
                                    m.f32(task + 0x80), m.f32(task + 0x84), m.f32(task + 0x88), m.f32(task + 0x9C), u[0],
                                    u[1], u[2], scale * 0.3f);
                    bSpit += bad;
                }
            }
            std::memcpy(m.ptr(0x471828), savedRandAll, sizeof savedRandAll);
            std::memcpy(m.ptr(0x43E400), savedTask, sizeof savedTask);
            report("magician fireball aim (x 0.0020161290, stored 1)", 0x4D3DEC, N, bAim);
            report("hermit spit (stored 6, x 0.0020161290, speed 1.0)", 0x4EE4C0, N, bSpit);
        }
    }
    // pmmisc: the rest of the kit's thrown objects' tumble for PC Mode (pm_tumble_update): the axis turn 0x4C1DD0, the
    // rock side 0x4C3570, tumble_start 0x4C5B70 and tumble_bounce 0x4C5DC4 (with its lowest corner / floor lift
    // 0x4C5A64 and matrix angles 0x4C2F78) against pm::axis_matrix / tumble_side / tumble_start / tumble_bounce
    {
        const uint32_t obj = 0x31020000, box = scratch + 0x600, vu = scratch + 0x700, ve = scratch + 0x710,
                       vn = scratch + 0x720, corners = 0xDA61E0, pivot = 0xDA6250, floorY = 0xDA0B48;
        if (!m.mapped(obj))
            m.map(obj, 0x1000);
        if (!m.mapped(corners))
            m.map(corners & ~0xFFFu, 0x1000);
        if (!m.mapped(floorY))
            m.map(floorY & ~0xFFFu, 0x1000);
        auto call = [&](uint32_t addr, uint32_t a, uint32_t b, uint32_t c, std::initializer_list<uint32_t> st) {
            const uint32_t regs[4] = {a, b, c, 0};
            std::vector<uint32_t> s(st);
            cpu.top = 0;
            cpu.cw = 0x107F;
            if (!cpu.call(addr, regs, s.data(), int(s.size()), cpuStack + 0xFF00) && firstErr.empty()) {
                char t[64];
                std::snprintf(t, sizeof t, " (running 0x%X)", addr);
                firstErr = cpu.error + t;
            }
            if (m.u32(GP::pc_matrix_ptr) != cur && firstErr.empty()) {
                char t[64];
                std::snprintf(t, sizeof t, "0x%X moved the matrix pointer", addr);
                firstErr = t;
            }
            m.set32(GP::pc_matrix_ptr, cur);
        };
        auto unit = [&](float v[3]) {
            do {
                for (int k = 0; k < 3; ++k)
                    v[k] = (rnd() & 7) == 0 ? float(int32_t(rnd() % 3u) - 1) : frand(-1.0f, 1.0f);
            } while (v[0] == 0.0f && v[1] == 0.0f && v[2] == 0.0f);
        };
        // the axis turn (x, y, z, angle on the stack) on a random current matrix
        long badAxis = 0;
        for (int n = 0; n < N; ++n) {
            pm::Matrix mm;
            randMatrix(mm.e);
            putMatrix(cur, mm.e);
            float a[3];
            unit(a);
            if (n & 1)
                for (float& f : a)
                    f *= frand(0.01f, 50.0f);
            const uint32_t ang = randAngle();
            call(0x4C1DD0, 0, 0, 0, {fb(a[0]), fb(a[1]), fb(a[2]), ang});
            float r[12];
            pm::axis_matrix(a[0], a[1], a[2], ang, r);
            pm::Matrix want = mm;
            want.mul(r);
            badAxis += !sameMatrix(cur, want.e);
        }
        report("tumble axis turn", 0x4C1DD0, N, badAxis);
        // the rock side (u eax, e edx, n ebx) -> eax
        long badSide = 0;
        for (int n = 0; n < N; ++n) {
            float u[3], e[3], nv[3];
            unit(u);
            unit(nv);
            if (n & 1)
                unit(e);
            else
                e[0] = 0.0f, e[1] = 1.0f, e[2] = 0.0f;
            for (uint32_t k = 0; k < 3; ++k) {
                m.set32(vu + 4 * k, fb(u[k]));
                m.set32(ve + 4 * k, fb(e[k]));
                m.set32(vn + 4 * k, fb(nv[k]));
            }
            putMatrix(cur, pm::Matrix().e);
            call(0x4C3570, vu, ve, vn, {});
            badSide += (cpu.r[X86::EAX] & 0xFF) != pm::tumble_side(u, e, nv);
        }
        report("tumble side", 0x4C3570, N, badSide);
        // tumble_start (obj eax: PC angles +0x34 / +0x38 / +0x3C; box edx)
        auto randBox = [&](pm::TumbleBox& b) {
            for (uint32_t k = 0; k < 6; ++k)
                b.w[k] = fb(k & 1 ? frand(0.5f, 20.0f) : frand(-20.0f, -0.5f));
            b.w[6] = uint32_t(int32_t(rnd() % 129u) - 64);
            b.w[7] = randAngle();
            b.w[8] = 1 + rnd() % 3u;
            b.w[9] = rnd();
            float a[3];
            unit(a);
            const float inv = 1.0f / pm::fast_sqrt((a[0] * a[0] + a[1] * a[1]) + a[2] * a[2]);
            for (int k = 0; k < 3; ++k)
                b.w[10 + k] = fb(a[k] * inv);
            b.w[13] = 2;
            b.w[14] = rnd() & 1;
            const int ax = int(rnd() % 3u);
            const float sg = (rnd() & 1) ? 1.0f : -1.0f;
            for (int k = 0; k < 3; ++k)
                b.w[15 + k] = fb(k == ax ? sg : 0.0f);
        };
        auto putBox = [&](const pm::TumbleBox& b) {
            for (uint32_t k = 0; k < 18; ++k)
                m.set32(box + 4 * k, b.w[k]);
        };
        auto sameBox = [&](const pm::TumbleBox& b) {
            for (uint32_t k = 0; k < 18; ++k)
                if (m.u32(box + 4 * k) != b.w[k])
                    return false;
            return true;
        };
        long badStart = 0;
        for (int n = 0; n < N; ++n) {
            pm::TumbleBox b;
            randBox(b);
            putBox(b);
            uint32_t ang[3];
            for (uint32_t k = 0; k < 3; ++k) {
                ang[k] = (rnd() & 7) == 0 ? (rnd() % 4u) * 0x4000u : rnd() & 0xFFFF;
                m.set32(obj + 0x34 + 4 * k, ang[k]);
            }
            putMatrix(cur, pm::Matrix().e);
            call(0x4C5B70, obj, box, 0, {});
            pm::tumble_start(ang[0], ang[1], ang[2], b);
            badStart += !sameBox(b);
        }
        report("tumble_start", 0x4C5B70, N, badStart);
        // tumble_bounce (obj eax: position +0x28, angles +0x34; box edx; corners 0xDA61E0, pivot 0xDA6250, floor
        // 0xDA0B48)
        long badBounce = 0;
        for (int n = 0; n < N; ++n) {
            pm::TumbleBox b;
            randBox(b);
            putBox(b);
            float pos[3];
            uint32_t ang[3];
            for (uint32_t k = 0; k < 3; ++k) {
                pos[k] = randF();
                ang[k] = (rnd() & 7) == 0 ? (rnd() % 4u) * 0x4000u : rnd() & 0xFFFF;
                m.set32(obj + 0x28 + 4 * k, fb(pos[k]));
                m.set32(obj + 0x34 + 4 * k, ang[k]);
            }
            float cs[24], pv[3] = {};
            for (uint32_t i = 0; i < 24; ++i) {
                cs[i] = (i % 3 == 1) ? frand(-10.0f, 10.0f) : randF();
                if (i % 3 == 1 && i > 3 && (rnd() & 7) == 0)
                    cs[i] = cs[1];   // equal lowest corners
                m.set32(corners + 4 * i, fb(cs[i]));
            }
            const float fy = frand(-12.0f, 12.0f);
            m.set32(floorY, fb(fy));
            putMatrix(cur, pm::Matrix().e);
            call(0x4C5DC4, obj, box, 0, {});
            pm::tumble_bounce(pos, ang, b, cs, pv, fy);
            bool d = !sameBox(b);
            for (uint32_t k = 0; k < 3; ++k)
                d |= m.u32(obj + 0x28 + 4 * k) != fb(pos[k]) || m.u32(obj + 0x34 + 4 * k) != ang[k] ||
                     m.u32(pivot + 4 * k) != fb(pv[k]);
            for (uint32_t i = 0; i < 24; ++i)
                d |= m.u32(corners + 4 * i) != fb(cs[i]);
            if (d && badBounce < 3)
                std::printf("    tumble_bounce: PC pos (%.9g %.9g %.9g) ang (0x%X 0x%X 0x%X) speed %d, pm (%.9g %.9g %.9g) "
                            "(0x%X 0x%X 0x%X) %d\n",
                            m.f32(obj + 0x28), m.f32(obj + 0x2C), m.f32(obj + 0x30), m.u32(obj + 0x34), m.u32(obj + 0x38),
                            m.u32(obj + 0x3C), int32_t(m.u32(box + 0x18)), pos[0], pos[1], pos[2], ang[0], ang[1], ang[2],
                            int32_t(b.w[6]));
            badBounce += d;
        }
        report("tumble_bounce (with 0x4C5A64, 0x4C3570, 0x4C1DD0, 0x4C2F78)", 0x4C5DC4, N, badBounce);
        // pmrest: the PC's ground_height 0x48A254 (any point: floor_y, PM's ground_height) and the tumble's ground test
        // 0x4C5A10 (any of the 8 corners at or under it: 1), the rule pm_tumble_update's state 0 uses
        long badGround = 0, badTest = 0, hits = 0;
        for (int n = 0; n < N; ++n) {
            const float fy = (n % 7 == 0) ? 0.0f : frand(-400.0f, 400.0f);
            m.set32(floorY, fb(fy));
            const uint32_t pt = scratch + 0x780;
            m.set32(pt, fb(randF())), m.set32(pt + 4, fb(randF())), m.set32(pt + 8, fb(randF()));
            call(0x48A254, pt, 0, 0, {});
            badGround += fb(cpu.st0f()) != fb(fy);
            bool want = false;
            for (uint32_t i = 0; i < 8; ++i) {
                const float y = (rnd() & 3) == 0 ? fy + float(int32_t(rnd() % 3u) - 1) * ((rnd() & 1) ? 0.0f : 0.001f)
                                                 : fy + frand(-3.0f, 60.0f);
                m.set32(corners + 12 * i, fb(randF()));
                m.set32(corners + 12 * i + 4, fb(y));
                m.set32(corners + 12 * i + 8, fb(randF()));
                want = want || !(fy < y);
            }
            call(0x4C5A10, 0, 0, 0, {});
            hits += want;
            badTest += (cpu.r[X86::EAX] != 0) != want;
        }
        report("ground_height (floor_y)", 0x48A254, N, badGround);
        report("tumble ground test (floor_y)", 0x4C5A10, N, badTest);
        if (hits == 0 || hits == N)
            std::printf("    tumble ground test: one-sided cases (%ld of %d hit)\n", hits, N);
    }
    if (m.bad && firstErr.empty()) {
        char b[80];
        std::snprintf(b, sizeof b, "an unmapped address was touched (0x%X)", m.badAddr);
        firstErr = b;
    }
    std::printf("pm helper copies vs THOTD.EXE's own code (x86 / x87 interpreter): %d routines, %ld cases, %d routines "
                "differ%s%s\n",
                nroutines, total, badRoutines, firstErr.empty() ? "" : "; first run error: ", firstErr.c_str());
    return badRoutines + (firstErr.empty() ? 0 : 1);
}
