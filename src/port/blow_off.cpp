// A zombie's head blown off (part_blow_off and the falling pieces it spawns) and the shadow on a sloped floor
// (shadow_update / shadow_projection); proofs-8, from the disassembly. Arcade forms only (the PC forms: vtable only,
// ProofPc replays them). Declarations: decl/core_bounds.h / decl/support_b.h.
#include "port/game.h"

namespace hotd::port {

namespace blow_detail {
constexpr uint32_t kSeveredPartFall = 0x69730;      // severed_part_fall (task)
constexpr uint32_t kFloorBloodSplatTask = 0x45230;  // floor_blood_splat_task
constexpr uint32_t kEyeballPop = 0x43970;           // eyeball_pop (task)
constexpr uint32_t kPlayerCams = 0x51F340;          // per player (0xCC): its camera object (+0x30 its yaw)
constexpr uint32_t kPartIdLow = 0x13599C;           // the replacement model by hit type (900) and role (20), steps 0 / 1
constexpr uint32_t kPartIdHigh = 0x11E008;          // the same by hit type (0x200) and step (4) from step 2 on
constexpr uint32_t kSevered = 0x520AA0;             // part_tree_detach_severed's list of detached subtrees (0-ended)
constexpr uint32_t kPieceScale = 0x118060;          // the falling piece's scale by hit type
constexpr uint32_t kMotionFrames = 0xC7540;         // the frame data by motion
constexpr uint32_t kDrawFrame = 0x53E3A0;           // the frame data the piece's nodes are drawn with

// the i960's real compares (bne / be after cmpr): unordered counts as equal for "differs" (bne not taken)
inline bool differs(float a, float b) { return a < b || a > b; }
} // namespace blow_detail

// 0x69550 part_blow_off(player, part): the part's model by hit type and its step (+0x78), the node flagged in its
// tree's sever marks (+0x20 tree, +0x30 index), the flagged subtrees detached (list 0x520AA0); each subtree whose role
// is of part class 1 (the head) becomes a falling piece (task 0x69730, its owner body_current): its position / angles
// from the node, scale 0x118060, thrown 0.2 back along the shooter's camera yaw, up 0.25..0.29, gravity -0.0272; the
// others are freed
template <class P> void Game<P>::part_blow_off(int player, uint32_t part)
{
    if constexpr (P::pc) {
        (void)player, (void)part;   // the PC form: not ported (vtable only; ProofPc replays it)
    } else {
        using namespace blow_detail;
        const uint32_t cam = mem.u32(kPlayerCams + uint32_t(int32_t(int8_t(player))) * 0xCC);
        const int32_t step = mem.at<int8_t>(part + 0x78);
        const uint32_t ht = mem.u32(G::hit_type);
        if (step <= 1)
            mem.set32(part, mem.u32(kPartIdLow + ht * 900 + mem.u32(part + 0x38) * 20));
        else
            mem.set32(part, mem.u32(kPartIdHigh + (ht << 9) + uint32_t(step) * 4));
        const uint32_t tree = mem.u32(part + 0x20);
        mem.at<uint8_t>(tree + 0x28 + mem.u32(part + 0x30)) |= 1;
        part_tree_detach_severed(tree);
        for (uint32_t l = kSevered;; l += 4) {
            const uint32_t node = mem.u32(l);
            if (node == 0)
                break;
            const uint32_t h = mem.u32(G::hit_type);
            const uint32_t cls = mem.at<uint8_t>(G::part_roles + (h * 31) * 8 + mem.u32(node + 0x38) * 8 + 3);
            if (cls != 1) {
                part_subtree_free(node);
                continue;
            }
            const uint32_t t = task_create(kSeveredPartFall, 0x3C8);
            mem.set32(t + 0x18, mem.u32(G::body_current));
            severed_part_init(t, node);
            mem.set32(t + 0x94, mem.u32(kPieceScale + mem.u32(G::hit_type) * 4));
            cam_copro(0x21, {});
            cam_copro(0x15, {mem.u32(cam + 0x30) + 0x8000u});
            const Tgp::Out d = cam_copro(0x1A, {0, 0, 0xBE4CCCCDu});
            cam_copro(0x06, {});
            mem.set32(t + 0x88, 0xBCDF0123u);
            const int32_t r = int32_t(rand()) % 5;
            mem.set32(t + 0x7C, fbits(pdiv(float(r), 100.0f) + 0.25f));
            mem.set32(t + 0x78, d.w[0] ^ 0x80000000u);
            mem.set32(t + 0x50, 0);
            mem.set32(t + 0x80, d.w[2] ^ 0x80000000u);
        }
    }
}

// 0x39830 severed_part_init(piece, node): the piece's own one-node tree (+0xC8: 0xD8 bytes, +0x24 a list of one, the
// node), the owner's motion (+0xD0); the node's matrix (stored matrix 2 times the node's saved one, +0x7C) gives the
// piece's position (+0x20) and angles (0x71: +0x34, +0x30, +0x2C)
template <class P> void Game<P>::severed_part_init(uint32_t piece, uint32_t node)
{
    const uint32_t tree = mem_alloc(0xD8);
    mem.set32(piece + 0xC8, tree);
    mem.at<int16_t>(piece + 0xD0) = mem.at<int16_t>(mem.u32(G::body_current) + 0xD0);
    mem.set32(tree + 0x20, 0);
    mem.set32(tree + 0x34, 1);
    const uint32_t list = mem_alloc(4);
    mem.set32(tree + 0x24, list);
    mem.set32(list, node);
    mem.at<uint8_t>(tree + 0x28) = 0;
    cam_copro(0x74, {2});
    uint32_t m[12];
    for (uint32_t k = 0; k < 12; ++k)
        m[k] = mem.u32(node + 0x7C + 4 * k);
    Tgp::Out none;
    copro(0x09, m, 12, none);
    const Tgp::Out p = cam_copro(0x20, {});
    mem.set32(piece + 0x20, p.w[0]);
    mem.set32(piece + 0x24, p.w[1]);
    mem.set32(piece + 0x28, p.w[2]);
    const Tgp::Out a = cam_copro(0x71, {});
    mem.set32(piece + 0x34, a.w[0]);
    mem.set32(piece + 0x30, a.w[1]);
    mem.set32(piece + 0x2C, a.w[2]);
    cam_copro(0x06, {});
}

// 0x69730 severed_part_fall (task): freed with its owner (serial 0); in the air gravity (+0x88) and its velocity, on
// the floor (0x51EA40) a bounce (vertical speed halved and turned, x / z x 0.8) with a blood splat for the first five;
// the spin (+0x44, +0x4C) slows by bounce; drawn
template <class P> void Game<P>::severed_part_fall(uint32_t obj)
{
    if (mem.u32(mem.u32(obj + 0x18) + 8) == 0) {
        severed_part_free(obj);
        task_exit();
        return;
    }
    const float g = mem.f32(obj + 0x88);
    if (mem.f32(obj + 0x24) > mem.f32(G::floor_y)) {
        const float vy = mem.f32(obj + 0x7C) + g;
        mem.set32(obj + 0x7C, fbits(vy));
        mem.set32(obj + 0x20, fbits(mem.f32(obj + 0x20) + mem.f32(obj + 0x78)));
        mem.set32(obj + 0x28, fbits(mem.f32(obj + 0x28) + mem.f32(obj + 0x80)));
        mem.set32(obj + 0x24, fbits(mem.f32(obj + 0x24) + vy));
    } else {
        const float vy = g + -(mem.f32(obj + 0x7C) * 0.5f);
        const float x = mem.f32(obj + 0x20), z = mem.f32(obj + 0x28);
        mem.set32(obj + 0x78, fbits(mem.f32(obj + 0x78) * ffrom(0x3F4CCCCDu)));
        mem.set32(obj + 0x80, fbits(mem.f32(obj + 0x80) * ffrom(0x3F4CCCCDu)));
        mem.set32(obj + 0x7C, fbits(vy));
        const float y = mem.f32(G::floor_y) + vy;
        mem.set32(obj + 0x20, fbits(x + mem.f32(obj + 0x78)));
        mem.set32(obj + 0x28, fbits(z + mem.f32(obj + 0x80)));
        mem.set32(obj + 0x24, fbits(y));
        const int32_t n = int32_t(mem.u32(obj + 0x50));
        if (!(n > 4)) {
            mem.set32(obj + 0x50, uint32_t(n + 1));
            floor_blood_splat(mem.f32(obj + 0x20), mem.f32(obj + 0x3C), mem.f32(obj + 0x28));
        }
    }
    const int32_t k = 5 - int32_t(mem.u32(obj + 0x50));
    mem.set32(obj + 0x44, mem.u32(obj + 0x44) + uint32_t((k << 12) / 5));
    mem.set32(obj + 0x4C, mem.u32(obj + 0x4C) + uint32_t((k << 7) / 5));
    severed_part_draw(obj);
}

// 0x38B10 severed_part_draw: at the piece's position, angles (+0x34, +0x30, +0x2C), scale (+0x94), the owner's
// motion's frame data (0xC7540 by +0xD0); the piece becomes body_current; its nodes drawn
template <class P> void Game<P>::severed_part_draw(uint32_t obj)
{
    using namespace blow_detail;
    cam_copro(0x05, {});
    const uint32_t tree = mem.u32(obj + 0xC8);
    cam_copro(0x12, {mem.u32(obj + 0x20), mem.u32(obj + 0x24), mem.u32(obj + 0x28)});
    mem.set32(G::body_current, obj);
    const uint32_t s = mem.u32(obj + 0x94);
    const uint32_t frames = mem.u32(kMotionFrames + uint32_t(int32_t(mem.at<int16_t>(obj + 0xD0))) * 4);
    cam_copro(0x86, {mem.u32(obj + 0x34), mem.u32(obj + 0x30), mem.u32(obj + 0x2C)});
    cam_copro(0x13, {s, s, s});
    mem.set32(kDrawFrame, frames);
    for (int32_t i = 0; i < int32_t(mem.u32(tree + 0x34)); ++i)
        part_node_draw_rec(mem.u32(mem.u32(tree + 0x24) + 4 * uint32_t(i)));
    cam_copro(0x06, {});
}

// 0x38A50 part_node_draw_rec: the node's joint offset (z negated), its angles from the frame data (0x53E3A0, by role)
// plus the piece's spin (+0x44, +0x4C), the model with its position read back (+0x5C), then the children
template <class P> void Game<P>::part_node_draw_rec(uint32_t node)
{
    using namespace blow_detail;
    cam_copro(0x05, {});
    cam_copro(0x12, {mem.u32(node + 4), mem.u32(node + 8), mem.u32(node + 0xC) ^ 0x80000000u});
    const uint32_t body = mem.u32(G::body_current);
    const uint32_t rec = mem.u32(kDrawFrame) + mem.u32(node + 0x38) * 6;
    cam_copro(0x86, {0u - uint32_t(mem.at<uint16_t>(rec + 0x10)), uint32_t(mem.at<uint16_t>(rec + 0xE)) + mem.u32(body + 0x44),
                     uint32_t(mem.at<uint16_t>(rec + 0xC)) + mem.u32(body + 0x4C)});
    geo_matrix_position(mem.u32(node), node + 0x5C);
    for (int32_t i = 0; i < int32_t(mem.u32(node + 0x34)); ++i)
        part_node_draw_rec(mem.u32(mem.u32(node + 0x24) + 4 * uint32_t(i)));
    cam_copro(0x06, {});
}

// 0x38FA0 severed_part_free(piece) / 0x390F0 part_subtree_free(root): each child's children freed
// (part_tree_free_children), then the child lists and the nodes; the piece's tree (+0xC8) / the root itself last
template <class P> void Game<P>::part_subtree_free(uint32_t root)
{
    for (int32_t i = 0; i < int32_t(mem.u32(root + 0x34)); ++i) {
        const uint32_t c = mem.u32(mem.u32(root + 0x24) + 4 * uint32_t(i));
        for (int32_t j = 0; j < int32_t(mem.u32(c + 0x34)); ++j) {
            const uint32_t gc = mem.u32(mem.u32(c + 0x24) + 4 * uint32_t(j));
            part_tree_free_children(gc);
            mem_free(gc);
        }
        if (mem.u32(c + 0x24) != 0)
            mem_free(mem.u32(c + 0x24));
        mem_free(mem.u32(mem.u32(root + 0x24) + 4 * uint32_t(i)));
    }
    if (mem.u32(root + 0x24) != 0)
        mem_free(mem.u32(root + 0x24));
    mem_free(root);
}

template <class P> void Game<P>::severed_part_free(uint32_t piece)
{
    part_subtree_free(mem.u32(piece + 0xC8));
}

// 0x43920 floor_blood_splat(x, y, z): a 120-frame task (0x45230) at (x, floor_y, z), a random yaw (+0x30)
template <class P> void Game<P>::floor_blood_splat(float x, float y, float z)
{
    (void)y;
    const uint32_t t = task_create(blow_detail::kFloorBloodSplatTask, 0x58);
    mem.set32(t + 0x30, rand() & 0xFFFF);
    mem.set32(t + 0x20, fbits(x));
    mem.set32(t + 0x24, mem.u32(G::floor_y));
    mem.set32(t + 0x28, fbits(z));
    mem.set32(t + 0x50, 0x78);
}

// 0x45230 floor_blood_splat_task: counts down (+0x50, ended at 0); model 0x2C7 at its place, its yaw, laid flat
template <class P> void Game<P>::floor_blood_splat_task(uint32_t obj)
{
    const uint32_t n = mem.u32(obj + 0x50) - 1;
    mem.set32(obj + 0x50, n);
    if (n == 0) {
        task_exit();
        return;
    }
    cam_copro(0x05, {});
    cam_copro(0x12, {mem.u32(obj + 0x20), mem.u32(obj + 0x24), mem.u32(obj + 0x28)});
    cam_copro(0x15, {mem.u32(obj + 0x30)});
    cam_copro(0x14, {0xFFFFC000u});
    geo_draw_model(0x2C7);
    cam_copro(0x06, {});
}

// 0x6C9E0 shadow_update(obj): the floor under the object (floor_probe from (x, 32768, z)); the shadow projected onto
// that floor's plane (normal n, n . p + d = 0 through the hit point)
template <class P> void Game<P>::shadow_update(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;   // the PC form: not ported (vtable only; ProofPc replays it)
    } else {
        LocalScope scope(*this);
        const uint32_t f = local_alloc(0x50);   // fp + 0x40: hit point, + 0x10 the probe point, + 0x20 normal, + 0x30 attribute, + 0x40 plane
        mem.set32(f + 0x10, mem.u32(obj + 0x20));
        mem.set32(f + 0x14, fbits(32768.0f));
        mem.set32(f + 0x18, mem.u32(obj + 0x28));
        floor_probe(f + 0x10, f, f + 0x20, f + 0x30);
        const float n0 = mem.f32(f + 0x20), n1 = mem.f32(f + 0x24), n2 = mem.f32(f + 0x28);
        const float d = ((-n0) * mem.f32(f) - n1 * mem.f32(f + 4)) - n2 * mem.f32(f + 8);
        mem.set32(f + 0x40, fbits(n0));
        mem.set32(f + 0x44, fbits(n1));
        mem.set32(f + 0x48, fbits(n2));
        mem.set32(f + 0x4C, fbits(d));
        shadow_projection(f + 0x40);
    }
}

// 0x6C490 shadow_projection(plane): the projection onto the plane p (a, b, c, d) along the shadow light (0x520A80's
// angles: direction v) into stored matrix 3 and its copy 0x51FDF0 (M = I + v p^T x -1 / (p . v)); skipped while the
// light and the plane (0x520A88, 0x520A90) are unchanged
template <class P> void Game<P>::shadow_projection(uint32_t plane)
{
    using namespace blow_detail;
    if (mem.u32(0x520A80) == mem.u32(0x520A88) && mem.u32(0x520A84) == mem.u32(0x520A8C) &&
        !differs(mem.f32(plane), mem.f32(0x520A90)) && !differs(mem.f32(plane + 4), mem.f32(0x520A94)) &&
        !differs(mem.f32(plane + 8), mem.f32(0x520A98)) && mem.f32(plane + 12) == mem.f32(0x520A9C))
        return;
    cam_copro(0x21, {});
    cam_copro(0x15, {mem.u32(0x520A84)});
    cam_copro(0x14, {mem.u32(0x520A80)});
    const Tgp::Out o = cam_copro(0x1A, {0, 0, fbits(1.0f)});
    const float v0 = ffrom(o.w[0]), v1 = ffrom(o.w[1]), v2 = ffrom(o.w[2]);
    const float p0 = mem.f32(plane), p1 = mem.f32(plane + 4), p2 = mem.f32(plane + 8), p3 = mem.f32(plane + 12);
    const float g = -1.0f / ((p0 * v0 + p1 * v1) + p2 * v2);
    cam_copro(0x06, {});
    const float m[12] = {(p0 * v0) * g + 1.0f, (v1 * p0) * g,        (v2 * p0) * g,
                         (v0 * p1) * g,        (v1 * p1) * g + 1.0f, (v2 * p1) * g,
                         (v0 * p2) * g,        (v1 * p2) * g,        (v2 * p2) * g + 1.0f,
                         (v0 * p3) * g,        (v1 * p3) * g,        (v2 * p3) * g};
    uint32_t w[12];
    for (uint32_t k = 0; k < 12; ++k) {
        w[k] = fbits(m[k]);
        mem.set32(0x51FDF0 + 4 * k, w[k]);
    }
    Tgp::Out none;
    cam_copro(0x05, {});
    copro(0x09, w, 12, none);
    cam_copro(0x76, {1});
    cam_copro(0x75, {3});
    mem.set32(0x520A88, mem.u32(0x520A80));
    mem.set32(0x520A8C, mem.u32(0x520A84));
    for (uint32_t k = 0; k < 4; ++k)
        mem.set32(0x520A90 + 4 * k, mem.u32(plane + 4 * k));
    cam_copro(0x06, {});
}

// 0x43B90 eyeball_pop_spawn(player, x, y, z): an eyeball (task 0x43970) at the hit point (camera space, through the
// inverse camera: coprocessor 0x74 1), thrown 0.1 sideways / 0.2 back along the shooter's camera yaw (+0x30), up
// 0.25..0.29, gravity -0.0272; 240 frames; its yaw the camera's - 0x4000. From the disassembly (the decompiler shows
// the coprocessor words as 0xd001a1a)
template <class P> void Game<P>::eyeball_pop_spawn(int player, float x, float y, float z)
{
    if constexpr (P::pc) {
        (void)player, (void)x, (void)y, (void)z;   // the PC form (0x486D60): not ported (vtable only; ProofPc replays it)
    } else {
        using namespace blow_detail;
        const uint32_t cam = mem.u32(kPlayerCams + uint32_t(int32_t(int8_t(player))) * 0xCC);
        const uint32_t t = task_create(kEyeballPop, 0x68);
        cam_copro(0x74, {1});
        const Tgp::Out p = cam_copro(0x1A, {fbits(x), fbits(y), fbits(z)});
        cam_copro(0x10, {});
        cam_copro(0x15, {mem.u32(cam + 0x30)});
        const Tgp::Out d = cam_copro(0x1A, {0xBDCCCCCDu, 0, 0x3E4CCCCDu});
        cam_copro(0x06, {});
        const int32_t r = int32_t(rand()) % 5;
        mem.set32(t + 0x5C, fbits(pdiv(float(r), 100.0f) + 0.25f));
        mem.set32(t + 0x64, 0xBCDF0123u);
        mem.set32(t + 0x20, p.w[0]);
        mem.set32(t + 0x24, p.w[1]);
        mem.set32(t + 0x28, p.w[2]);
        mem.set32(t + 0x58, d.w[0] ^ 0x80000000u);
        mem.set32(t + 0x60, d.w[2] ^ 0x80000000u);
        mem.set32(t + 0x3C, mem.u32(G::floor_y));
        mem.set32(t + 0x54, 0);
        mem.set32(t + 0x50, 0xF0);
        mem.set32(t + 0x30, (mem.u32(cam + 0x30) - 0x4000u) & 0xFFFF);
    }
}

// 0x43970 eyeball_pop (task): counts down (+0x50, ended at 0); above its floor (+0x3C) gravity (+0x64) and its
// velocity (+0x58 / +0x5C / +0x60); on the floor it bounces (half the fall speed back up, 0.8 of the sideways speed)
// and leaves a blood splat (task 0x45230, the first 5 bounces); model 0x77B (PN_z_a_medama) at its place, spinning
// (+0x2C by 0x1000, +0x34 by 0x80 a frame), scale 1.2
template <class P> void Game<P>::eyeball_pop(uint32_t obj)
{
    const int32_t left = mem.template at<int32_t>(obj + 0x50);
    mem.set32(obj + 0x50, uint32_t(left - 1));
    if (left < 1) {
        task_exit();
        return;
    }
    const float y = mem.f32(obj + 0x24), floor = mem.f32(obj + 0x3C), g = mem.f32(obj + 0x64);
    if (y > floor) {
        const float vy = mem.f32(obj + 0x5C) + g;
        mem.set32(obj + 0x5C, fbits(vy));
        mem.set32(obj + 0x20, fbits(mem.f32(obj + 0x20) + mem.f32(obj + 0x58)));
        mem.set32(obj + 0x28, fbits(mem.f32(obj + 0x28) + mem.f32(obj + 0x60)));
        mem.set32(obj + 0x24, fbits(y + vy));
    } else {
        const float vy = -(mem.f32(obj + 0x5C) * 0.5f) + g;
        const float vx = mem.f32(obj + 0x58) * 0.8f, vz = mem.f32(obj + 0x60) * 0.8f;
        const float nx = mem.f32(obj + 0x20) + vx, nz = mem.f32(obj + 0x28) + vz;
        const int32_t n = mem.template at<int32_t>(obj + 0x54);
        mem.set32(obj + 0x54, uint32_t(n + 1));
        mem.set32(obj + 0x58, fbits(vx));
        mem.set32(obj + 0x60, fbits(vz));
        mem.set32(obj + 0x20, fbits(nx));
        mem.set32(obj + 0x28, fbits(nz));
        mem.set32(obj + 0x5C, fbits(vy));
        mem.set32(obj + 0x24, fbits(floor + vy));
        if (n <= 4) {
            const uint32_t t = task_create(blow_detail::kFloorBloodSplatTask, 0x58);
            mem.set32(t + 0x30, rand() & 0xFFFF);
            mem.set32(t + 0x20, mem.u32(obj + 0x20));
            mem.set32(t + 0x28, mem.u32(obj + 0x28));
            mem.set32(t + 0x50, 0x78);
            mem.set32(t + 0x24, mem.u32(G::floor_y));
        }
    }
    cam_copro(0x05, {});
    cam_copro(0x12, {mem.u32(obj + 0x20), mem.u32(obj + 0x24), mem.u32(obj + 0x28)});
    cam_copro(0x15, {mem.u32(obj + 0x30)});
    const uint32_t a0 = mem.u32(obj + 0x2C) + 0x1000;
    mem.set32(obj + 0x2C, a0);
    cam_copro(0x14, {a0});
    const uint32_t a2 = mem.u32(obj + 0x34) + 0x80;
    mem.set32(obj + 0x34, a2);
    cam_copro(0x16, {a2});
    cam_copro(0x13, {0x3F99999Au, 0x3F99999Au, 0x3F99999Au});
    geo_draw_model(0x77B);
    cam_copro(0x06, {});
}

// 0x19D0 geo_polygon_ram_write(src, addr, polygons): geometrizer command 5 writes polygons x 10 + 7 words from src
// into polygon RAM at addr; returns the address after them. Hardware writes only (not in the captures)
template <class P> uint32_t Game<P>::geo_polygon_ram_write(uint32_t src, uint32_t addr, int32_t polygons)
{
    const uint32_t fifo = mem.u32(0x790);
    const uint32_t words = uint32_t(polygons) * 10 + 7;
    hw_write32(0x800050, 0x505);
    hw_write32(fifo, addr & 0xFFFFFF);
    hw_write32(fifo, words);
    for (uint32_t k = 0; k < words; ++k)
        hw_write32(fifo, mem.u32(src + 4 * k));
    return addr + words;
}

// 0x21920 sophie_skin_build(obj): Sophie's two strips (slot buffers 0x51FDCC / 0x51FDD0 + slot x 0x348): their corner
// points (0x21770: 2, 0x21790: 3) from the +0x2A4 part's space into the +0x274 part's (push +0x274, inverse, times
// +0x2A4); each written into its polygon's vertex (map 0x217E8 / 0x217F0 -> 0x217C0 / 0x217D0: polygon x 0x28 + 0xC,
// -1 = the shared corner +0x18)
template <class P> void Game<P>::sophie_skin_build(uint32_t obj)
{
    Tgp::Out none;
    uint32_t m[12];
    for (uint32_t k = 0; k < 12; ++k)
        m[k] = mem.u32(obj + 0x274 + 4 * k);
    copro(0x7F, m, 12, none);
    copro(0x7A, nullptr, 0, none);
    for (uint32_t k = 0; k < 12; ++k)
        m[k] = mem.u32(obj + 0x2A4 + 4 * k);
    copro(0x09, m, 12, none);
    auto strip = [&](uint32_t bufPtr, uint32_t corners, uint32_t table, uint32_t map, uint32_t n) {
        const uint32_t buf = mem.u32(bufPtr);
        for (uint32_t i = 0; i < n; ++i) {
            const uint32_t c = corners + 12 * i;
            const Tgp::Out o = cam_copro(0x1A, {mem.u32(c), mem.u32(c + 4), mem.u32(c + 8)});
            const uint32_t idx = mem.u32(table + 4 * mem.u32(map + 4 * i));
            const uint32_t slot = uint32_t(int32_t(mem.template at<int8_t>(obj + 0x2E6))) * 0x348;
            const uint32_t at = idx == 0xFFFFFFFFu ? buf + slot + 0x18 : buf + slot + idx * 0x28 + 0xC;
            mem.set32(at, o.w[0]);
            mem.set32(at + 4, o.w[1]);
            mem.set32(at + 8, o.w[2]);
        }
    };
    strip(0x51FDCC, 0x21770, 0x217C0, 0x217E8, 2);
    strip(0x51FDD0, 0x21790, 0x217D0, 0x217F0, 3);
    copro(0x06, nullptr, 0, none);
}

// 0x21C30 sophie_skin_draw(obj): creature type 34 only (body_draw). Her strips built, written to polygon RAM at the
// running address (0x51E8CC: 2 then 4 polygons), then drawn with the +0x274 part's matrix (geometrizer command 0x0B)
// by two direct draws (command 1: headers 0x80172 / 0x8017E and 0x80182 / 0x8019A)
template <class P> void Game<P>::sophie_skin_draw(uint32_t obj)
{
    if constexpr (P::pc) {
        (void)obj;   // the PC's body_draw has no such call (vtable only)
    } else {
        sophie_skin_build(obj);
        const uint32_t slot = uint32_t(int32_t(mem.template at<int8_t>(obj + 0x2E6))) * 0x348;
        const uint32_t p0 = mem.u32(0x51E8CC);
        const uint32_t p1 = geo_polygon_ram_write(mem.u32(0x51FDCC) + slot + 0xC, p0, 2);
        mem.set32(0x51E8CC, p1);
        const uint32_t p2 = geo_polygon_ram_write(mem.u32(0x51FDD0) + slot + 0xC, p1, 4);
        cam_copro(0x21, {});
        Tgp::Out none;
        uint32_t m[12];
        for (uint32_t k = 0; k < 12; ++k)
            m[k] = mem.u32(obj + 0x274 + 4 * k);
        copro(0x09, m, 12, none);
        const Tgp::Out mx = cam_copro(0x11, {});
        const uint32_t fifo = mem.u32(0x790);
        hw_write32(0x8000B0, 0xB0B);
        for (int k = 0; k < 12; ++k)
            hw_write32(fifo, mx.w[k]);
        const uint32_t draws[2][4] = {{0x80172, 0x8017E, p0, 2}, {0x80182, 0x8019A, p1, 4}};
        for (const auto& d : draws) {
            hw_write32(0x800010, 0x101);
            for (uint32_t w : d)
                hw_write32(fifo, w);
        }
        cam_copro(0x06, {});
        mem.set32(0x51E8CC, p2);
    }
}

} // namespace hotd::port
