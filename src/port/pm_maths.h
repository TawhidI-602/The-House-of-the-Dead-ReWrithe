// PC Mode on the arcade logic (docs/pcmode.md row 10): the 1998 PC port's maths on plain values, for the runtime's PM
// branches (enemy turn, shot ray). The PC's routines (Game<Pc>: pc_atan2, pc_vec_angles, pc_matrix_rotate_y,
// pc_vec_cos, pc_angle_turn, shot_ray_setup's PC branch) read THOTD.EXE's tables and its software matrix stack from
// the PC's memory; these copies work on the runtime's own values and rebuild the two tables from formulas (nothing
// copied from THOTD.EXE):
//   - the sin / cos table 0x5697B0 (4096 (sin, cos) float pairs, index = angle / 16 & 0xFFF): round(sin(i 2 pi /
//     4096) x 65536) / 65536, except the 8 entries whose value x 65536 is +-43562.4987 (i = 474, 550, 1498, 1574,
//     2522, 2598, 3546, 3622): THOTD.EXE has +-43563 there (its builder rounded that one value up), so +-43562 -> +-43563;
//   - the arctangent table 0xC653B8 (1025 ints in THOTD.EXE's initialised data): round(atan(i / 1024) x 2048 / pi).
// `hotd_proof pmmaths <THOTD.EXE>` checks both tables entry by entry and these routines against Game<Pc>'s.
#pragma once

#include "port/tgp.h"
#include "port/types.h"
#include "port/util.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace hotd::port::pm {

inline const std::array<float, 8192>& trig_table()
{
    static const auto t = [] {
        std::array<float, 8192> a{};
        const double pi = 3.14159265358979323846;
        for (int i = 0; i < 4096; ++i)
            for (int k = 0; k < 2; ++k) {
                const double ang = double(i) * 2.0 * pi / 4096.0;
                double r = std::floor((k ? std::cos(ang) : std::sin(ang)) * 65536.0 + 0.5);
                if (r == 43562.0 || r == -43562.0)
                    r += r > 0 ? 1.0 : -1.0;
                a[size_t(i) * 2 + size_t(k)] = float(r / 65536.0);
            }
        return a;
    }();
    return t;
}

inline const std::array<int32_t, 1025>& atan_table()
{
    static const auto t = [] {
        std::array<int32_t, 1025> a{};
        const double pi = 3.14159265358979323846;
        for (int i = 0; i <= 1024; ++i)
            a[size_t(i)] = int32_t(std::floor(std::atan(double(i) / 1024.0) * 2048.0 / pi + 0.5));
        return a;
    }();
    return t;
}

// the table index of a 16-bit angle: / 16 truncated toward zero, & 0xFFF; k 0 = sin, 1 = cos
inline float trig(uint32_t angle, int k)
{
    const int32_t i = (int32_t(int16_t(angle & 0xFFFF)) / 16) & 0xFFF;
    return trig_table()[size_t(i) * 2 + size_t(k)];
}

// the PC's table square root (pc_fast_sqrt; the 256-word table 0x5CCEFC from its formula, as Game<A>::pm_fast_sqrt)
inline float fast_sqrt(float x)
{
    static const auto table = [] {
        std::array<uint32_t, 256> t{};
        for (uint32_t i = 0; i < 256; ++i)
            t[i] = fbits(float(std::sqrt((1.0 + double(i & 127) / 128.0) * ((i & 128) ? 2.0 : 1.0)))) - 0x3F800000u;
        return t;
    }();
    const uint32_t b = fbits(x);
    const int32_t e = int32_t(b - 0x3F800000u);
    const uint32_t index = ((b & 0x7FFFFFu) | (uint32_t(e) & 0x800000u)) >> 16;
    const int32_t half = (int32_t(uint32_t(e) & 0xFF000000u) >> 1) + 0x3F800000;
    return ffrom(table[index] + uint32_t(half));
}

inline int32_t fistp(double v)
{
    if (!(v > -2147483649.0 && v < 2147483648.0))
        return INT32_MIN;
    return int32_t(v);
}

// the x87's a + b at 24-bit precision for operands that are not both floats (e.g. an int loaded by fild): the exact
// sum rounded once to a float. The double sum s is exact up to err (two-sum); s itself rounds correctly unless it
// sits exactly halfway between two floats, where err's sign decides
inline float add_round(double a, double b)
{
    const double s = a + b;
    const double bb = s - a;
    const double err = (a - (s - bb)) + (b - bb);
    const float f = float(s);
    if (err == 0.0 || double(f) == s)
        return f;
    const float g = std::nextafter(f, double(f) < s ? HUGE_VALF : -HUGE_VALF);
    if ((double(f) + double(g)) * 0.5 != s)
        return f;   // not a tie: s and the exact sum round alike
    return (err > 0.0) == (double(g) > double(f)) ? g : f;
}

// PC 0x568F10 pc_atan2 (a, b): an angle in 1/4096 turns (Game<Pc>::pc_atan2 on the rebuilt table)
inline int32_t atan2(float a, float b)
{
    int32_t X = fistp(std::trunc(double(a) * 65536.0)), Y = fistp(std::trunc(double(b) * 65536.0));
    int q = 0;
    if (X < 0) {
        X = int32_t(0u - uint32_t(X));
        q = 2;
    }
    if (Y < 0) {
        Y = int32_t(0u - uint32_t(Y));
        ++q;
    }
    while (0x100000 < Y || 0x100000 < X) {
        X >>= 1;
        Y >>= 1;
    }
    int32_t r;
    if (Y == 0)
        r = 0x400;
    else if (X <= Y)
        r = atan_table()[size_t((int64_t(X) * 1024 + Y / 2) / Y)];
    else
        r = 0x400 - atan_table()[size_t((int64_t(Y) * 1024 + X / 2) / X)];
    switch (q) {
    case 0: return r;
    case 1: return 0x800 - r;
    case 2: return -r;
    default: return r - 0x800;
    }
}

// PC 0x412D28 pc_vec_angles (and its copies): yaw / pitch of (x, y, z) with the pitch fold
inline void vec_angles(float x, float y, float z, uint32_t& yaw, uint32_t& pitch)
{
    const float h = fast_sqrt(z * z + x * x);
    pitch = 0u - (uint32_t(atan2(-y, h)) << 4);
    yaw = 0u - (uint32_t(atan2(x, z)) << 4);
    fixAngles(yaw, pitch);
}

// PC 0x4D7A0C pc_vec_cos: the cosine between two vectors, lengths by the table square root
inline float vec_cos(float a0, float a1, float a2, float b0, float b1, float b2)
{
    const float dot = (a0 * b0 + a1 * b1) + a2 * b2;
    const float la = fast_sqrt((a0 * a0 + a1 * a1) + a2 * a2);
    const float lb = fast_sqrt((b0 * b0 + b1 * b1) + b2 * b2);
    return dot / (lb * la);
}

// PC 0x4D8D7C pc_angle_turn (angle, target, k, slow)
inline int32_t angle_turn(int32_t angle, uint32_t target, float k, float slow)
{
    const uint32_t delta = (target - uint32_t(angle)) & 0xFFFF;
    const float den = k + slow;
    float q;
    if (int32_t(delta) > 0x7FFF)
        q = -(float(int32_t(0x10000 - delta)) * k);
    else
        q = float(int32_t(delta)) * k;
    q = q / den;
    const float r = add_round(double(q), double(angle));   // fild angle; fadd: the exact sum rounded once
    return fistp(std::trunc(double(r)));
}

// the PC's software matrix (row-major 3 x 3, then the translation): the current matrix of pc_matrix_* as 12 floats
struct Matrix {
    float e[12] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};   // pc_identity_matrix
    // PC 0x4D74FC pc_matrix_rotate_y: each row (a, b, d) becomes (c a + s d, b, c d - s a)
    void rotate_y(uint32_t angle)
    {
        const float s = trig(angle, 0), c = trig(angle, 1);
        for (int row = 0; row < 3; ++row) {
            const float a = e[row * 3], d = e[row * 3 + 2];
            e[row * 3] = s * d + c * a;
            e[row * 3 + 2] = c * d + (-s) * a;
        }
    }
    // PC 0x412C40 pc_matrix_transform (and its copies pc_matrix_transform2..12)
    void transform(float x, float y, float z, float& ox, float& oy, float& oz) const
    {
        ox = ((e[1] * y + e[0] * x) + e[2] * z) + e[9];
        oy = ((e[3] * x + e[4] * y) + e[5] * z) + e[10];
        oz = ((e[7] * y + e[6] * x) + e[8] * z) + e[11];
    }
    // PC 0x4D7B4C pc_matrix_identity (and its copies): the identity 0x5CE4F4
    void identity() { *this = Matrix(); }
    // PC 0x4D72DC pc_matrix_translate (and its copies, the camera's 0x45D3FC): the translation becomes the point
    // (x, y, z) through the current matrix, in pc_matrix_transform's x87 order
    void translate(float x, float y, float z)
    {
        float a = 0.0f, b = 0.0f, c = 0.0f;
        transform(x, y, z, a, b, c);
        e[9] = a;
        e[10] = b;
        e[11] = c;
    }
    // PC 0x4F54B8 pc_matrix_rotate_x (and its copies, the camera's 0x45D8B0): each row (a, b, d) becomes
    // (a, c b - s d, s b + c d)
    void rotate_x(uint32_t angle)
    {
        const float s = trig(angle, 0), c = trig(angle, 1);
        for (int row = 0; row < 3; ++row) {
            const float b = e[row * 3 + 1], d = e[row * 3 + 2];
            e[row * 3 + 1] = c * b + (-s) * d;
            e[row * 3 + 2] = c * d + s * b;
        }
    }
    // PC 0x4D7604 pc_matrix_rotate_b and the rotate_z copies (pc_matrix_rotate_z8 / 9 / 11 / 12, rotate_b2 / b3 /
    // b10, the camera's 0x45D724): each row (a, b, d) becomes (c a - s b, s a + c b, d)
    void rotate_z(uint32_t angle)
    {
        const float s = trig(angle, 0), c = trig(angle, 1);
        for (int row = 0; row < 3; ++row) {
            const float a = e[row * 3], b = e[row * 3 + 1];
            e[row * 3] = c * a + (-s) * b;
            e[row * 3 + 1] = s * a + c * b;
        }
    }
    // PC 0x4E251C pc_matrix_mul8: the current matrix times m (m applied first): rows of the current one by m's
    // columns, the translation = m's through the current matrix
    void mul(const float m[12])
    {
        float r[12];
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 3; ++col)
                r[row * 3 + col] =
                    (e[row * 3] * m[col] + e[row * 3 + 1] * m[3 + col]) + e[row * 3 + 2] * m[6 + col];
        for (int row = 0; row < 3; ++row)
            r[9 + row] = ((e[row * 3] * m[9] + e[row * 3 + 1] * m[10]) + e[row * 3 + 2] * m[11]) + e[9 + row];
        for (int i = 0; i < 12; ++i)
            e[i] = r[i];
    }
    // PC 0x4E8374 pc_matrix_mul_stored (and 0x4FCB84 pc_matrix_mul_stored10): m times the current matrix (the
    // current one applied first)
    void mul_stored(const float m[12])
    {
        float r[12];
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 3; ++col)
                r[row * 3 + col] =
                    (m[row * 3] * e[col] + m[row * 3 + 1] * e[3 + col]) + m[row * 3 + 2] * e[6 + col];
        for (int row = 0; row < 3; ++row)
            r[9 + row] = ((m[row * 3] * e[9] + m[row * 3 + 1] * e[10]) + m[row * 3 + 2] * e[11]) + m[9 + row];
        for (int i = 0; i < 12; ++i)
            e[i] = r[i];
    }
    // PC 0x475B68 (the body file's helpers block +0xE80; every helpers block has a copy): the current matrix inverted
    // by cofactors over the determinant. Written instruction by instruction from THOTD.EXE's x87 code (one float
    // operation per x87 operation, the PC's 24-bit precision; the L slots are its stack locals)
    void invert()
    {
        const float* m = e;
        float L[26] = {};
        const float t0 = m[3];
        const float t1 = m[8];
        const float t2 = t0 * t1;
        const float t3 = m[5];
        const float t4 = m[6];
        const float t5 = t3 * t4;
        const float t6 = m[1];
        const float t7 = t2 - t5;
        L[12] = t7;
        const float t8 = L[12];
        const float t9 = t6 * t8;
        const float t10 = m[4];
        const float t11 = m[8];
        const float t12 = t10 * t11;
        const float t13 = m[5];
        const float t14 = m[7];
        const float t15 = t13 * t14;
        const float t16 = t12 - t15;
        L[19] = t16;
        const float t17 = m[0];
        const float t18 = L[19];
        const float t19 = t17 * t18;
        const float t20 = t19 - t9;
        const float t21 = m[3];
        const float t22 = m[7];
        const float t23 = t21 * t22;
        const float t24 = m[4];
        const float t25 = m[6];
        const float t26 = t24 * t25;
        const float t27 = t23 - t26;
        L[25] = t27;
        const float t28 = m[2];
        const float t29 = L[25];
        const float t30 = t28 * t29;
        const float t31 = t20 + t30;
        L[14] = t31;
        const float t32 = L[14];
        const float t33 = 1.0f / t32;
        const float t34 = m[1];
        const float t35 = m[8];
        const float t36 = t34 * t35;
        const float t37 = m[1];
        const float t38 = m[5];
        const float t39 = t37 * t38;
        const float t40 = m[5];
        const float t41 = m[11];
        const float t42 = t40 * t41;
        const float t43 = m[10];
        const float t44 = m[8];
        const float t45 = t43 * t44;
        const float t46 = m[2];
        const float t47 = m[7];
        const float t48 = t46 * t47;
        const float t49 = m[2];
        const float t50 = m[4];
        const float t51 = t49 * t50;
        const float t52 = t36 - t48;
        L[20] = t33;
        const float t53 = L[19];
        const float t54 = L[20];
        const float t55 = t53 * t54;
        const float t56 = -t52;
        const float t57 = L[20];
        const float t58 = t56 * t57;
        const float t59 = m[2];
        const float t60 = m[6];
        const float t61 = t59 * t60;
        const float t62 = t39 - t51;
        const float t63 = m[2];
        const float t64 = m[3];
        const float t65 = t63 * t64;
        L[21] = t42;
        const float t66 = m[1];
        const float t67 = L[21];
        const float t68 = t67 - t45;
        L[21] = t68;
        const float t69 = L[21];
        const float t70 = t66 * t69;
        L[0] = t55;
        L[1] = t58;
        const float t71 = m[4];
        const float t72 = m[11];
        const float t73 = t71 * t72;
        const float t74 = L[20];
        const float t75 = t62 * t74;
        const float t76 = m[3];
        const float t77 = m[11];
        const float t78 = t76 * t77;
        L[22] = t73;
        L[2] = t75;
        const float t79 = m[10];
        const float t80 = m[7];
        const float t81 = t79 * t80;
        const float t82 = m[10];
        L[17] = t81;
        const float t83 = L[22];
        const float t84 = L[17];
        const float t85 = t83 - t84;
        const float t86 = m[6];
        const float t87 = t82 * t86;
        L[24] = t85;
        const float t88 = m[2];
        const float t89 = L[24];
        const float t90 = t88 * t89;
        const float t91 = m[0];
        const float t92 = t70 - t90;
        const float t93 = m[8];
        const float t94 = t91 * t93;
        const float t95 = m[9];
        const float t96 = L[19];
        const float t97 = t95 * t96;
        L[15] = t78;
        L[23] = t87;
        const float t98 = L[15];
        const float t99 = L[23];
        const float t100 = t98 - t99;
        const float t101 = m[2];
        L[18] = t100;
        const float t102 = t92 + t97;
        const float t103 = L[18];
        const float t104 = t101 * t103;
        const float t105 = -t102;
        const float t106 = L[20];
        const float t107 = t105 * t106;
        const float t108 = m[0];
        const float t109 = m[5];
        const float t110 = t108 * t109;
        const float t111 = m[0];
        const float t112 = L[21];
        const float t113 = t111 * t112;
        const float t114 = t113 - t104;
        const float t115 = t94 - t61;
        const float t116 = m[9];
        const float t117 = L[12];
        const float t118 = t116 * t117;
        const float t119 = t110 - t65;
        const float t120 = L[20];
        const float t121 = t115 * t120;
        const float t122 = -t119;
        const float t123 = L[12];
        const float t124 = -t123;
        const float t125 = L[20];
        const float t126 = t124 * t125;
        const float t127 = t114 + t118;
        const float t128 = L[20];
        const float t129 = t122 * t128;
        const float t130 = L[20];
        const float t131 = t127 * t130;
        L[9] = t107;
        const float t132 = L[25];
        const float t133 = L[20];
        const float t134 = t132 * t133;
        L[3] = t126;
        L[4] = t121;
        L[5] = t129;
        L[10] = t131;
        L[6] = t134;
        const float t135 = m[1];
        const float t136 = L[18];
        const float t137 = t135 * t136;
        const float t138 = m[1];
        const float t139 = m[6];
        const float t140 = t138 * t139;
        const float t141 = m[1];
        const float t142 = m[3];
        const float t143 = t141 * t142;
        const float t144 = m[0];
        const float t145 = L[24];
        const float t146 = t144 * t145;
        const float t147 = m[0];
        const float t148 = m[4];
        const float t149 = t147 * t148;
        const float t150 = m[0];
        const float t151 = t149 - t143;
        const float t152 = m[7];
        const float t153 = t150 * t152;
        const float t154 = L[20];
        const float t155 = t151 * t154;
        const float t156 = t146 - t137;
        const float t157 = t153 - t140;
        const float t158 = m[9];
        const float t159 = L[25];
        const float t160 = t158 * t159;
        const float t161 = -t157;
        const float t162 = t156 + t160;
        const float t163 = t161 * t154;
        const float t164 = -t162;
        const float t165 = t154 * t164;
        L[8] = t155;
        L[7] = t163;
        L[11] = t165;
        for (int i = 0; i < 12; ++i)
            e[i] = L[i];
    }
};

// the PC's camera matrix (stored matrix 1, 0xD52EA0: camera_matrices_update 0x45F140 for the camera_view record): the
// identity, translated by the eye (record + 0x60), turned about y by the yaw (+0x70), x by the pitch (+0x6C), z by
// the roll (+0x74), each through the camera file's own copies of the helpers (0x45D3FC, 0x45D61C, 0x45D8B0, 0x45D724)
inline void camera_matrix(const float eye[3], uint32_t pitch, uint32_t yaw, uint32_t roll, float out[12])
{
    Matrix m;
    m.translate(eye[0], eye[1], eye[2]);
    m.rotate_y(yaw);
    m.rotate_x(pitch);
    m.rotate_z(roll);
    for (int i = 0; i < 12; ++i)
        out[i] = m.e[i];
}

// the PC's camera inverse (stored matrix 0: the identity, turned about z by -roll, x by -pitch, y by -yaw, translated
// by -eye), the current matrix the PC's enemy code transforms with (enemy_target_distance's screen side)
inline void camera_inverse(const float eye[3], uint32_t pitch, uint32_t yaw, uint32_t roll, float out[12])
{
    Matrix m;
    m.rotate_z(0u - roll);
    m.rotate_x(0u - pitch);
    m.rotate_y(0u - yaw);
    m.translate(-eye[0], -eye[1], -eye[2]);
    for (int i = 0; i < 12; ++i)
        out[i] = m.e[i];
}

// the game camera's look mode 2 (THOTD.EXE 0x45FF68 at 0x46009B; record +0xF0 == 2, the PC has no mode 1): yaw and
// roll 0, the pitch -(pc_atan2(dy, table square root of (dz dz + dx dx)) x 16), d = the eye minus the look-at
inline uint32_t look2_pitch(const float eye[3], const float at[3])
{
    const float dz = eye[2] - at[2], dx = eye[0] - at[0], dy = eye[1] - at[1];
    const float h = fast_sqrt(dz * dz + dx * dx);
    return 0u - (uint32_t(atan2(dy, h)) << 4);
}

// draw type 1's part (THOTD.EXE 0x477EC4, the body file's helpers 0x474CE8): the part's offset (x, y, -z), then its
// joint's turns z / y / x on the pushed matrix
inline void draw1_part(Matrix& m, float x, float y, float z, uint32_t az, uint32_t ay, uint32_t ax)
{
    m.translate(x, y, -z);
    m.rotate_z(az);
    m.rotate_y(ay);
    m.rotate_x(ax);
}

// its home part's place (0x477F78): the part matrix's translation through stored matrix 1 (copied in)
inline void draw1_home(const Matrix& m, const float cam[12], float out[3])
{
    Matrix c;
    for (int k = 0; k < 12; ++k)
        c.e[k] = cam[k];
    c.transform(m.e[9], m.e[10], m.e[11], out[0], out[1], out[2]);
}

// the heads' eye / target beam (THOTD.EXE 0x51E258 / 0x51E4C8, the part hook file's helpers 0x51AF1C): on a copy of
// the part's matrix turned about y by -0x4000, times stored matrix 1 (the camera), inverted, the point (x, y, z) into
// it and its vec_angles (with the fold), both kept to 16 bits
inline void look_angles(const Matrix& part, const float cam[12], float x, float y, float z, uint32_t& yaw, uint32_t& pitch)
{
    Matrix m = part;
    m.rotate_y(0xFFFFC000u);
    m.mul_stored(cam);
    m.invert();
    float p[3];
    m.transform(x, y, z, p[0], p[1], p[2]);
    vec_angles(p[0], p[1], p[2], yaw, pitch);
    yaw &= 0xFFFF;
    pitch &= 0xFFFF;
}

// the whole beam (0x51E258 / 0x51E4C8 after the target test): look_angles, each clamped to +-0x2000 (0x2000..0x7FFF ->
// 0x2000, 0x8000..0xDFFF -> 0xE000), the head's look angles cy (+0x14C) / cp (+0x150) eased toward them by 0xC0 (the
// nearer way round, at once when within 0xC0), kept to 16 bits; then the part's matrix turned about y by cy, z by -cp
inline void look_turn(Matrix& part, const float cam[12], float x, float y, float z, uint32_t& cy, uint32_t& cp)
{
    uint32_t t[2];
    look_angles(part, cam, x, y, z, t[0], t[1]);
    uint32_t* c[2] = {&cy, &cp};
    for (int k = 0; k < 2; ++k) {
        int32_t g = int32_t(t[k]);
        if (g < 0x8000 && g > 0x2000)
            g = 0x2000;
        else if (g >= 0x8000 && g < 0xE000)
            g = 0xE000;
        const int32_t cur = int32_t(*c[k]);
        const int32_t d = std::abs(cur - g), dUp = std::abs(g + 0x10000 - cur), dDown = std::abs(cur + 0x10000 - g);
        int32_t v;
        if (d <= 0xC0 || dUp <= 0xC0 || dDown <= 0xC0)
            v = g;
        else if (cur >= g)
            v = cur + (d > dUp ? 0xC0 : -0xC0);
        else
            v = cur + (d > dDown ? -0xC0 : 0xC0);
        *c[k] = uint32_t(v) & 0xFFFF;
    }
    part.rotate_y(cy);
    part.rotate_z(0u - cp);
}

// PC 0x4C521C move_along_heading: (0, 0, step) through the object's angles (the identity turned about z by +0x34, y
// by +0x30, x by +0x2C: the helpers file's copies 12) -> the step to add to the position
inline void heading_step(uint32_t ax, uint32_t ay, uint32_t az, float step, float out[3])
{
    Matrix m;
    m.rotate_z(az);
    m.rotate_y(ay);
    m.rotate_x(ax);
    m.transform(0.0f, 0.0f, step, out[0], out[1], out[2]);
}

// PC 0x4C5048 angle_to_camera: the camera point in the object's own frame (the identity turned by -x, -y, -z,
// translated by -pos), its yaw (pc_vec_angles12), low 16 bits
inline uint32_t angle_to_point(const float pos[3], uint32_t ax, uint32_t ay, uint32_t az, const float cam[3])
{
    Matrix m;
    m.rotate_x(0u - ax);
    m.rotate_y(0u - ay);
    m.rotate_z(0u - az);
    m.translate(-pos[0], -pos[1], -pos[2]);
    float v[3];
    m.transform(cam[0], cam[1], cam[2], v[0], v[1], v[2]);
    uint32_t yaw = 0, pitch = 0;
    vec_angles(v[0], v[1], v[2], yaw, pitch);
    return yaw & 0xFFFF;
}

// the PC's unit floor direction to (dx, dz) (walk_to 0x4479CC, turn_toward_camera 0x44782C, dog_st2_move ...): 1 /
// pc_fast_sqrt4(dz dz + dx dx), each part times it (y: 0 x it)
inline void floor_dir(float dx, float dz, float out[3])
{
    const float inv = 1.0f / fast_sqrt(dz * dz + dx * dx);
    out[0] = dx * inv;
    out[1] = 0.0f * inv;
    out[2] = dz * inv;
}

// PC 0x4479CC walk_to's maths: the unit floor direction from (px, pz) to (x, z), its yaw (pc_vec_angles3, the
// caller turns the heading by angle_turn_toward) and 1 when pc_fast_sqrt4(dz dz + dx dx) < within
inline uint32_t walk_to(float px, float pz, float x, float z, float within, float dir[3], uint32_t& yaw)
{
    const float dx = x - px, dz = z - pz;
    floor_dir(dx, dz, dir);
    uint32_t pitch = 0;
    yaw = 0;
    vec_angles(dx, 0.0f, dz, yaw, pitch);
    return fast_sqrt(dz * dz + dx * dx) < within ? 1u : 0u;
}

// PC 0x44782C turn_toward_camera's x offset to the camera: with two or more enemies 3 units left or right by the
// attack slot's parity, in the x87's float order ((camx + parity x 6) + -3) - x
inline float camera_aim_dx(float camx, float x, bool twoOrMore, int32_t slot)
{
    if (!twoOrMore)
        return camx - x;
    const float a = camx + float(slot % 2) * 6.0f;
    return (a + -3.0f) - x;
}

// PC 0x46EEE0 attacker_list_add's key: (int)(the camera distance x 10), the distance pc_fast_sqrt((dy dy + dx dx) +
// dz dz) of d = camera - position, x 10.0f, chopped (__CHP)
inline int32_t attacker_key(const float pos[3], const float cam[3])
{
    const float dy = cam[1] - pos[1], dx = cam[0] - pos[0], dz = cam[2] - pos[2];
    const float d = fast_sqrt((dy * dy + dx * dx) + dz * dz) * 10.0f;
    return int32_t(int64_t(std::trunc(double(d))));
}

// PC 0x4C56F0 velocity_for_arc's velocity (to land on `to` after n frames under gravity g): x / z (to - from) x (1 /
// n), y ((g n) n + dy x 2) / (2 n), each operation at the x87's 24 bits
inline void arc_velocity(const float from[3], const float to[3], float g, float n, float v[3])
{
    const float inv = 1.0f / n;
    v[0] = (to[0] - from[0]) * inv;
    v[1] = ((g * n) * n + (to[1] - from[1]) * 2.0f) / (2.0f * n);
    v[2] = (to[2] - from[2]) * inv;
}

// PC 0x4C576C velocity_for_arc_speed's flight time: the floor distance (pc_fast_sqrt12) / speed
inline float arc_frames(const float from[3], const float to[3], float speed)
{
    const float dx = to[0] - from[0], dz = to[2] - from[2];
    return fast_sqrt(dx * dx + dz * dz) / speed;
}

// the PC's floor distance between two points through its table square root (attack_slot_nearest_object 0x46B4A8's
// 0x468B1C, the holder first)
inline float floor_distance(float ax, float az, float bx, float bz)
{
    const float dx = bx - ax, dz = bz - az;
    return fast_sqrt(dx * dx + dz * dz);
}

// the arcade coprocessor's stored matrix n (column layout: x' = x m0 + y m3 + z m6 + m9) in the PC's row layout
inline void copro_matrix(const Tgp& t, int n, float out[12])
{
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col)
            out[row * 3 + col] = t.stored(n, col * 3 + row);
    for (int k = 9; k < 12; ++k)
        out[k] = t.stored(n, k);
}

// 12 words of the arcade coprocessor's matrix layout (a body node, a kept part matrix, an 0x11 read of the current
// matrix) in the PC's row layout: PC Mode's PC maths on a matrix only the arcade's body drawing makes
inline Matrix from_copro_words(const uint32_t w[12])
{
    Matrix m;
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col)
            m.e[row * 3 + col] = ffrom(w[col * 3 + row]);
    for (int k = 9; k < 12; ++k)
        m.e[k] = ffrom(w[k]);
    return m;
}

inline Matrix from_floats(const float f[12])
{
    Matrix m;
    for (int k = 0; k < 12; ++k)
        m.e[k] = f[k];
    return m;
}

// PC 0x4A2808 pc_view_test (and its copies, at +0x16B8 of each helpers block: 0x4E3124 pc_view_test2, the Chariot's
// 0x514BA4, ...): is the sphere (x, y, z, r) on screen. The point through stored matrix 0 (cam_inv); behind the eye
// (z + r < 0): -1; else projected with the PC's constants 0xD53058 / 0xD5305C (640, -620: its 640 x 480 window) and
// tested against the viewport rectangle (*0xD5A5C0 +0x20..: -320, -240, 320, 240): +1 on screen, -1 off. The
// constants and the rectangle are the values the PC harness captures read (k97aim_pm, frog_pm, molb_pm); the x87
// order and its unordered compares as THOTD.EXE's
inline float view_test(const float cam_inv[12], float x, float y, float z, float r)
{
    const Matrix m = from_floats(cam_inv);
    float ox = 0.0f, oy = 0.0f, oz = 0.0f;
    m.transform(x, y, z, ox, oy, oz);
    if (0.0f > oz + r)
        return -1.0f;
    const float kx = 640.0f, ky = -620.0f;
    const float w = 1.0f / oz;
    const float sx = (ox * kx) * w, sy = (oy * ky) * w;
    const float rx = (kx * r) * w, ry = (ky * r) * w;
    if (!(sx + rx >= -320.0f))
        return -1.0f;
    if (sx - rx >= 320.0f)
        return -1.0f;
    if (!(sy - ry >= -240.0f))
        return -1.0f;
    return !(sy + ry >= 240.0f) ? 1.0f : -1.0f;
}

// PC 0x4E2E84 pc_matrix_angles8 (and its copies at +0x1418 of each helpers block, e.g. the Chariot's 0x514904): the
// angles of the current matrix m, written as the PC does: (1, 0, 0) through m without its translation gives c (yaw of
// (y, z, x)) and b (its pitch); a = pc_atan2(-z, y) << 4 of (0, 1, 0) through m, turned back by -b about y and -c
// about z
inline void matrix_angles8(const Matrix& cur, uint32_t& a, uint32_t& b, uint32_t& c)
{
    Matrix t = cur;
    t.e[9] = t.e[10] = t.e[11] = 0.0f;
    float x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
    t.transform(1.0f, 0.0f, 0.0f, x1, y1, z1);
    vec_angles(y1, z1, x1, c, b);
    float x2 = 0.0f, y2 = 0.0f, z2 = 0.0f;
    t.transform(0.0f, 1.0f, 0.0f, x2, y2, z2);
    Matrix u;
    u.rotate_y(0u - b);
    u.rotate_z(0u - c);
    float x3 = 0.0f, y3 = 0.0f, z3 = 0.0f;
    u.transform(x2, y2, z2, x3, y3, z3);
    a = uint32_t(atan2(-z3, y3)) << 4;
}

// PC 0x475F38 (the body file's helpers block +0x1250; every block has a copy): the angles of the current matrix m
// without its translation: (0, 0, 1) through m gives the yaw and pitch (pc_vec_angles: the body's direction is the
// yaw); c = pc_atan2(x, -y) << 4 of (1, 0, 0) through m, turned back by -pitch about x and -yaw about y
inline void matrix_angles_yp(const Matrix& cur, uint32_t& yaw, uint32_t& pitch, uint32_t& c)
{
    Matrix t = cur;
    t.e[9] = t.e[10] = t.e[11] = 0.0f;
    float x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
    t.transform(0.0f, 0.0f, 1.0f, x1, y1, z1);
    vec_angles(x1, y1, z1, yaw, pitch);
    float x2 = 0.0f, y2 = 0.0f, z2 = 0.0f;
    t.transform(1.0f, 0.0f, 0.0f, x2, y2, z2);
    Matrix u;
    u.rotate_x(0u - pitch);
    u.rotate_y(0u - yaw);
    float x3 = 0.0f, y3 = 0.0f, z3 = 0.0f;
    u.transform(x2, y2, z2, x3, y3, z3);
    c = uint32_t(atan2(x3, -y3)) << 4;
}

// PC 0x461858 (curve, frame): one keyframed camera curve at an integer frame F (the PC's camera_path_eval 0x461980
// truncates the frame). A curve (the PC's CAMALL.RBH channel, or the coprocessor data ROM's: the same layout): a
// header word (low half the key count, the search starting at its half; high half the search steps + 1), then 4-word
// keys (time, value, two tangents: the PC reads the lower key's word 3 and the upper key's word 2). The search as the
// PC's (F > key time: up), then its Hermite form in its x87 order (each operation a float operation: written
// instruction by instruction from THOTD.EXE). word(i) reads word i of the data
template <class Word> inline float path_curve(const Word& word, uint32_t c, int32_t F)
{
    auto key = [&](int32_t i, int w) {
        const uint32_t b = word(c + 1 + 4 * uint32_t(i) + uint32_t(w));
        float f;
        std::memcpy(&f, &b, 4);
        return f;
    };
    const uint32_t h = word(c);
    const int32_t lo = int16_t(h & 0xFFFF), steps = int16_t(h >> 16) - 1;
    int32_t i = lo / 2, d = lo / 4;
    const float fF = float(F);
    for (int32_t n = 0; n < steps; ++n) {
        i += fF > key(i, 0) ? d : -d;
        d /= 2;
    }
    if (fF > key(i, 0))
        ++i;
    const float k0 = key(i, 0), km4 = key(i - 1, 0), km3 = key(i - 1, 1), k1 = key(i, 1), km1 = key(i - 1, 3),
                k2 = key(i, 2);
    const float t2 = k0 - km4;
    const float t3 = t2 * t2;
    const float t4 = 1.0f / t3;
    const float t7 = fF - km4;
    const float t8 = t7 * t7;
    const float t9 = t3 * t2;
    const float t10 = t8 * t7;
    const float t11 = t10 / t9;
    const float t13 = t8 * t4;
    const float t15 = t11 * 2.0f;
    const float t17 = t10 * t4;
    const float t19 = t13 * 3.0f;
    const float t20 = t8 / t2;
    const float t22 = t20 * 2.0f;
    const float t24 = t15 - t19;
    const float t26 = t11 * -2.0f;
    const float t27 = t24 + 1.0f;
    const float t29 = t27 * km3;
    const float t31 = t26 + t19;
    const float t33 = t31 * k1;
    const float t34 = t17 - t22;
    const float t35 = t29 + t33;
    const float t36 = t34 + t7;
    const float t38 = t36 * km1;
    const float t39 = t35 + t38;
    const float t40 = t17 - t20;
    const float t42 = t40 * k2;
    return t39 + t42;
}

// PC 0x4C9D28 (the helpers block 0x4C87C0's +0x1568: camdr_mode3_frame 0x4CA680's heading): the current matrix
// without its translation: (1, 0, 0) through it, pc_vec_angles of (y, z, x) (the block's 0x4C88A8 by way of 0x4C9CC0:
// folded again when (yawRef - pitch + 0x4000) has bit 15) -> h0 = the pitch, h1 = the yaw; c = pc_atan2(y, -z) << 4 of
// (0, 1, 0) through it turned back by -h0 about y and -h1 about z
inline void camera_heading(const Matrix& cur, uint32_t yawRef, uint32_t& h0, uint32_t& h1, uint32_t& c)
{
    Matrix t = cur;
    t.e[9] = t.e[10] = t.e[11] = 0.0f;
    float x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
    t.transform(1.0f, 0.0f, 0.0f, x1, y1, z1);
    uint32_t yw = 0, pt = 0;
    vec_angles(y1, z1, x1, yw, pt);
    if (int16_t(uint16_t((yawRef - pt) + 0x4000)) < 0) {
        yw += 0x8000;
        pt = 0x8000 - pt;
    }
    h0 = pt;
    h1 = yw;
    float x2 = 0.0f, y2 = 0.0f, z2 = 0.0f;
    t.transform(0.0f, 1.0f, 0.0f, x2, y2, z2);
    Matrix u;
    u.rotate_y(0u - pt);
    u.rotate_z(0u - yw);
    float x3 = 0.0f, y3 = 0.0f, z3 = 0.0f;
    u.transform(x2, y2, z2, x3, y3, z3);
    c = uint32_t(atan2(y3, -z3)) << 4;
}

// the PC's normalise (inline in the boss files: 1 / pc_fast_sqrt((x x + y y) + z z), each part times it)
inline void normalize(const float d[3], float out[3])
{
    const float inv = 1.0f / fast_sqrt((d[0] * d[0] + d[1] * d[1]) + d[2] * d[2]);
    out[0] = d[0] * inv;
    out[1] = d[1] * inv;
    out[2] = d[2] * inv;
}

// the PC's C library rand (THOTD.EXE 0x578A6B, Watcom: the thread's seed x 0x41C64E6D + 0x3039, bits 16-30; srand
// 0x578A8F is called only by the debug menu's random autofire and an unreferenced start-up routine, so the seed
// starts at the library's 1)
inline uint32_t crand(uint32_t& seed)
{
    seed = seed * 0x41C64E6Du + 0x3039u;
    return (seed >> 16) & 0x7FFFu;
}

// the PC's enemy turn toward the target (enemy_turn_pc: PC 0x4DA030 enemy_ai, 0x52307C kind11_leap): the new yaw
inline int32_t enemy_turn(int32_t yaw0, const float pos[3], const float target[3], bool backwards, bool atLeast4)
{
    Matrix m;
    m.rotate_y(uint32_t(yaw0));
    float hx = 0.0f, hy = 0.0f, hz = 0.0f;
    m.transform(0.0f, 0.0f, 10.0f, hx, hy, hz);
    const float dz = target[2] - pos[2];
    const float dx = target[0] - pos[0];
    uint32_t yaw = 0, pitch = 0;
    vec_angles(dx, 0.0f, dz, yaw, pitch);
    const float c = vec_cos(hx, hy, hz, dx, 0.0f, dz);
    yaw &= 0xFFFF;
    float slow = !(0.0f >= c) ? (1.0f - c) * 60.0f : 60.0f;
    if (atLeast4 && !(slow >= 4.0f))
        slow = 4.0f;
    if (backwards) {
        const int32_t base = int32_t((uint32_t(yaw0) + 0x8000) & 0xFFFF);
        return int32_t((uint32_t(angle_turn(base, yaw, 1.0f, slow * 4.0f)) + 0x8000) & 0xFFFF);
    }
    return angle_turn(yaw0, yaw, 1.0f, slow);
}

// the PC's shot ray (shot_ray_setup 0x414810's PC branch): cam = stored matrix 1 (the camera) in the PC's layout
template <class Ray> void shot_ray(float x, float y, float focal, const float cam[12], Ray& r)
{
    const float inv = 1.0f / fast_sqrt((x * x + y * y) + focal * focal);
    r.cam_dir[0] = x * inv;
    r.cam_dir[1] = y * inv;
    r.cam_dir[2] = focal * inv;
    r.cam_origin[0] = 0.0f;
    r.cam_origin[1] = 0.0f;
    r.cam_origin[2] = 0.0f;
    uint32_t a0 = 0, a1 = 0;
    vec_angles(r.cam_dir[0], r.cam_dir[1], r.cam_dir[2], a0, a1);
    Matrix m;
    for (int i = 0; i < 12; ++i)
        m.e[i] = cam[i];
    m.transform(r.cam_origin[0], r.cam_origin[1], r.cam_origin[2], r.origin[0], r.origin[1], r.origin[2]);
    m.e[9] = m.e[10] = m.e[11] = 0.0f;
    m.transform(r.cam_dir[0], r.cam_dir[1], r.cam_dir[2], r.dir[0], r.dir[1], r.dir[2]);
    uint32_t b0 = 0, b1 = 0;
    vec_angles(r.dir[0], r.dir[1], r.dir[2], b0, b1);
    r.aim[0] = x;
    r.aim[1] = y;
    r.cam_ang_neg[0] = int32_t(0 - a1);
    r.cam_ang_neg[1] = int32_t(0 - a0);
    r.cam_trig[0] = trig(0 - a1, 0);
    r.cam_trig[1] = trig(0 - a1, 1);
    r.cam_trig[2] = trig(0 - a0, 0);
    r.cam_trig[3] = trig(0 - a0, 1);
    r.world_ang_neg[0] = int32_t(0 - b1);
    r.world_ang_neg[1] = int32_t(0 - b0);
    r.world_trig[0] = trig(0 - b1, 0);
    r.world_trig[1] = trig(0 - b1, 1);
    r.world_trig[2] = trig(0 - b0, 0);
    r.world_trig[3] = trig(0 - b0, 1);
}

// the PC's ray test (ray_sign_test 0x414AAC): 1 unless the point's distance from the ray (cam_trig) is over radius
template <class Ray> int ray_sign(const Ray& r, float x, float y, float z, float radius)
{
    const float c0 = r.cam_trig[0], c1 = r.cam_trig[1], s0 = r.cam_trig[2], s1 = r.cam_trig[3];
    const float a = (x * c0) * s0, b = x * s1, c = y * c1, d = z * s0, e = z * c0;
    const float u = b - d;
    const float v = (a + c) + e * s1;
    return fast_sqrt(v * v + u * u) > radius ? -1 : 1;
}

// PC 0x4C1DD0 (the helpers file): the turn by `angle` about the axis (x, y, z), normalised by 1 / the table square
// root: n n^T (1 - c) + c I + s [n]x as the x87 builds it, translation 0; 0x4C2610 then multiplies it in
// (Matrix::mul: the current matrix times it)
inline void axis_matrix(float x, float y, float z, uint32_t angle, float m[12])
{
    const float s = trig(angle, 0), c = trig(angle, 1);
    const float inv = 1.0f / fast_sqrt((x * x + y * y) + z * z);
    const float nx = x * inv, ny = y * inv, nz = inv * z;
    const float t = 1.0f - c;
    const float nxyt = (ny * nx) * t, nzxt = (nz * nx) * t, nzyt = (nz * ny) * t;
    const float nzs = nz * s, nys = ny * s, nxs = nx * s;
    m[0] = (nx * nx) * t + c;
    m[1] = nxyt + nzs;
    m[2] = nzxt - nys;
    m[3] = nxyt - nzs;
    m[4] = (ny * ny) * t + c;
    m[5] = nzyt + nxs;
    m[6] = nzxt + nys;
    m[7] = nzyt - nxs;
    m[8] = (nz * nz) * t + c;
    m[9] = m[10] = m[11] = 0.0f;
}

// PC 0x4C3570 (the tumble's side of the rock): u (the object's "up" in the world) seen from the rock axis n's frame
// (the identity turned by minus n's pitch / yaw, pc_vec_angles12), its roll pc_atan2(x, -y) x 16; e (the world's up)
// through the identity turned by minus roll, pitch, yaw: 1 when its y is below 0
inline uint32_t tumble_side(const float u[3], const float e[3], const float n[3])
{
    uint32_t yaw = 0, pitch = 0;
    vec_angles(n[0], n[1], n[2], yaw, pitch);
    Matrix m;
    m.rotate_x(0u - pitch);
    m.rotate_y(0u - yaw);
    float v[3];
    m.transform(u[0], u[1], u[2], v[0], v[1], v[2]);
    const uint32_t roll = uint32_t(atan2(v[0], -v[1])) << 4;
    m.identity();
    m.rotate_z(0u - roll);
    m.rotate_x(0u - pitch);
    m.rotate_y(0u - yaw);
    float w[3];
    m.transform(e[0], e[1], e[2], w[0], w[1], w[2]);
    return 0.0f > w[1] ? 1u : 0u;
}

// the tumbling box block (the object's +0x37C; the same layout on both versions): words 0-5 the half sizes (x-, x+,
// y-, y+, z-, z+), 6 the rock speed, 7 the rock angle, 8 the crossings left, 10-12 the rock axis, 13 the state, 14
// the last side, 15-17 the local "up" axis
struct TumbleBox {
    uint32_t w[18];
    float f(int i) const { return ffrom(w[i]); }
};

// PC 0x4C5B70 tumble_start (object angles x / y / z): of the six axis directions (+-x, +-y, +-z through the identity
// turned by z, y, x) the one pointing most up (the first of equal ones) is the local "up"; the rock axis = (0, 1, 0)
// x (up in the world), normalised by 1 / the table square root; 3 crossings, angle 32, speed 8, side 1
inline void tumble_start(uint32_t ax, uint32_t ay, uint32_t az, TumbleBox& b)
{
    Matrix m;
    m.rotate_z(az);
    m.rotate_y(ay);
    m.rotate_x(ax);
    float e[3][3];
    m.transform(1.0f, 0.0f, 0.0f, e[0][0], e[0][1], e[0][2]);
    m.transform(0.0f, 1.0f, 0.0f, e[1][0], e[1][1], e[1][2]);
    m.transform(0.0f, 0.0f, 1.0f, e[2][0], e[2][1], e[2][2]);
    const float ys[6] = {e[0][1], e[1][1], e[2][1], -e[0][1], -e[1][1], -e[2][1]};
    int best = 0;
    float most = ys[0];
    for (int i = 1; i < 6; ++i)
        if (ys[i] > most) {
            most = ys[i];
            best = i;
        }
    static const float kUp[6][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {-1, 0, 0}, {0, -1, 0}, {0, 0, -1}};
    for (int k = 0; k < 3; ++k)
        b.w[15 + k] = fbits(kUp[best][k]);
    float w[3];
    m.transform(b.f(15), b.f(16), b.f(17), w[0], w[1], w[2]);
    const float ex = 0.0f, ey = 1.0f, ez = 0.0f;   // 0x5EEF50
    const float nx = ey * w[2] - ez * w[1], ny = ez * w[0] - ex * w[2], nz = ex * w[1] - ey * w[0];
    const float inv = 1.0f / fast_sqrt((nx * nx + ny * ny) + nz * nz);
    b.w[10] = fbits(nx * inv);
    b.w[11] = fbits(ny * inv);
    b.w[12] = fbits(nz * inv);
    b.w[8] = 3;
    b.w[7] = 0x20;
    b.w[6] = 8;
    b.w[14] = 1;
}

// PC 0x4C5DC4 tumble_bounce (object position / angles x, y, z; its box; the 8 corners; the floor height): the side of
// the rock (tumble_side of the object's up in the world) flips -> a crossing (speed x -2, chopped; the last one: state
// + 1); angle += speed; PC 0x4C5A64: the lowest corner (the first of equal ones) is the pivot, lifted onto the floor
// with the object and corners 1-7 when under it; then the object turns by the angle about the rock axis through the
// pivot: its new position and angles (pc_matrix_angles12) from the identity translated to the pivot, turned
// (0x4C1DD0), translated back, translated to the position and turned by z, y, x
inline void tumble_bounce(float pos[3], uint32_t ang[3], TumbleBox& b, float corners[24], float pivot[3], float floorY)
{
    const float n[3] = {b.f(10), b.f(11), b.f(12)};
    Matrix m;
    m.rotate_z(ang[2]);
    m.rotate_y(ang[1]);
    m.rotate_x(ang[0]);
    float u[3];
    m.transform(b.f(15), b.f(16), b.f(17), u[0], u[1], u[2]);
    const float up[3] = {0.0f, 1.0f, 0.0f};
    const uint32_t side = tumble_side(u, up, n);
    if (side != b.w[14]) {
        b.w[8] = b.w[8] - 1;
        if (b.w[8] == 0)
            b.w[13] = b.w[13] + 1;
        b.w[6] = uint32_t(fistp(std::trunc(double(float(int32_t(b.w[6])) * -2.0f))));
    }
    b.w[14] = side;
    b.w[7] = b.w[7] + b.w[6];
    for (int k = 0; k < 3; ++k)
        pivot[k] = corners[k];
    for (int i = 1; i < 8; ++i)
        if (pivot[1] > corners[i * 3 + 1])
            for (int k = 0; k < 3; ++k)
                pivot[k] = corners[i * 3 + k];
    if (floorY > pivot[1]) {
        const float d = floorY - pivot[1];
        pivot[1] = pivot[1] + d;
        pos[1] = pos[1] + d;
        for (int i = 1; i < 8; ++i)
            corners[i * 3 + 1] = corners[i * 3 + 1] + d;
    }
    const float p[3] = {pivot[0], pivot[1], pivot[2]};
    Matrix k;
    k.translate(p[0], p[1], p[2]);
    float r[12];
    axis_matrix(n[0], n[1], n[2], b.w[7], r);
    k.mul(r);
    k.translate(-p[0], -p[1], -p[2]);
    k.translate(pos[0], pos[1], pos[2]);
    k.rotate_z(ang[2]);
    k.rotate_y(ang[1]);
    k.rotate_x(ang[0]);
    pos[0] = k.e[9];
    pos[1] = k.e[10];
    pos[2] = k.e[11];
    matrix_angles8(k, ang[0], ang[1], ang[2]);   // (0x4C2F78, the helpers file's copy)
}

} // namespace hotd::port::pm
