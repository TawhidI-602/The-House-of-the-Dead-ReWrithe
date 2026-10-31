#include "assets/arcade_anim.h"

#include "assets/assets.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>

namespace hotd {

namespace {

// maincpu tables (formats.md §23)
constexpr uint32_t kSkeletons = 0xC7240;     // root node per creature type
constexpr uint32_t kJointCounts = 0xC73C0;   // int32 per type
constexpr uint32_t kMotionPtrs = 0xC7540;    // first frame record per motion
constexpr uint32_t kMotionLens = 0xC7FD0;    // frame count per motion
constexpr uint32_t kBodyNames = 0xC8A60;     // name string pointer per creature type ("BO_...")
constexpr uint32_t kMotionNames = 0xC8BE0;   // name string pointer per motion ("MO_...")
constexpr uint32_t kFirstSide = 0x118000;    // byte per type: bit 0 = the side of role-1 first-level parts (part_init)
constexpr uint32_t kMirrored = 0x7C6F0;      // byte per motion: a mirrored motion (part_draw)
constexpr uint32_t kPartRoles = 0x1184E0;    // 0xF8 per type, 8 per role: byte 3 = the part class
constexpr int kTypes = 94, kMotions = 674;

int recordSize(int joints) { return (12 + 6 * joints + 3) & ~3; }

uint32_t rd32(const std::vector<uint8_t>& b, size_t a)
{
    uint32_t v = 0;
    if (a + 4 <= b.size()) std::memcpy(&v, b.data() + a, 4);
    return v;
}

void mul3(const float a[9], const float b[9], float out[9])
{
    float r[9];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) r[i * 3 + j] = a[i * 3] * b[j] + a[i * 3 + 1] * b[3 + j] + a[i * 3 + 2] * b[6 + j];
    std::memcpy(out, r, sizeof r);
}

} // namespace

Xform operator*(const Xform& a, const Xform& b)
{
    Xform r;
    mul3(a.rot, b.rot, r.rot);
    for (int i = 0; i < 3; ++i)
        r.pos[i] = a.rot[i * 3] * b.pos[0] + a.rot[i * 3 + 1] * b.pos[1] + a.rot[i * 3 + 2] * b.pos[2] + a.pos[i];
    return r;
}

Xform inverse(const Xform& a)
{
    const float* m = a.rot;
    const float c00 = m[4] * m[8] - m[5] * m[7], c01 = m[5] * m[6] - m[3] * m[8], c02 = m[3] * m[7] - m[4] * m[6];
    const float det = m[0] * c00 + m[1] * c01 + m[2] * c02;
    Xform r;
    if (std::fabs(det) < 1e-12f) return r;
    const float k = 1 / det;
    r.rot[0] = c00 * k;
    r.rot[1] = (m[2] * m[7] - m[1] * m[8]) * k;
    r.rot[2] = (m[1] * m[5] - m[2] * m[4]) * k;
    r.rot[3] = c01 * k;
    r.rot[4] = (m[0] * m[8] - m[2] * m[6]) * k;
    r.rot[5] = (m[2] * m[3] - m[0] * m[5]) * k;
    r.rot[6] = c02 * k;
    r.rot[7] = (m[1] * m[6] - m[0] * m[7]) * k;
    r.rot[8] = (m[0] * m[4] - m[1] * m[3]) * k;
    for (int i = 0; i < 3; ++i)
        r.pos[i] = -(r.rot[i * 3] * a.pos[0] + r.rot[i * 3 + 1] * a.pos[1] + r.rot[i * 3 + 2] * a.pos[2]);
    return r;
}

const std::string& CreatureAnimations::bodyName(int type) const
{
    static const std::string none;
    return type >= 0 && type < int(bodyNames_.size()) ? bodyNames_[type] : none;
}

const std::string& CreatureAnimations::motionName(int motion) const
{
    static const std::string none;
    return motion >= 0 && motion < int(motionNames_.size()) ? motionNames_[motion] : none;
}

bool CreatureAnimations::load(const std::string& regionsDir, std::string& err)
{
    if (!readFile(regionsDir + "/maincpu.bin", maincpu_, err) || !readFile(regionsDir + "/main_data.bin", mainData_, err))
        return false;
    // name tables: pointers (maincpu addresses) to zero-terminated strings, one per creature type / motion
    auto names = [&](uint32_t table, int n, std::vector<std::string>& out) {
        out.assign(n, {});
        for (int i = 0; i < n; ++i) {
            uint32_t p = rd32(maincpu_, table + 4 * size_t(i));
            while (p < maincpu_.size() && maincpu_[p] && out[i].size() < 64) out[i] += char(maincpu_[p++]);
        }
    };
    names(kBodyNames, kTypes, bodyNames_);
    names(kMotionNames, kMotions, motionNames_);
    // skeletons: node +0 model, +4 offset (3 floats), +0x10 flag, +0x14 joint, +0x18 child count, +0x1C child pointers
    skeletons_.assign(kTypes, {});
    for (int t = 0; t < kTypes; ++t) {
        Skeleton& s = skeletons_[t];
        s.joints = int(rd32(maincpu_, kJointCounts + 4 * t));
        std::function<void(uint32_t, int, int)> node = [&](uint32_t a, int parent, int depth) {
            if (a + 0x1C > maincpu_.size() || depth > 32 || s.nodes.size() > 64) return;
            SkeletonNode n;
            n.model = rd32(maincpu_, a);
            for (int i = 0; i < 3; ++i) std::memcpy(&n.offset[i], maincpu_.data() + a + 4 + 4 * i, 4);
            n.flag = rd32(maincpu_, a + 0x10);
            n.joint = int(rd32(maincpu_, a + 0x14));
            n.parent = parent;
            n.mirror = (n.flag & 1) != 0;
            // part_init: side flags bit 5 = ((role == 1 ? b : ~b) & 1) for the root's children, inherited below; the
            // parts with bit 5 clear take the second motion's angles
            if (depth == 1) {
                const uint8_t b = t < int(maincpu_.size()) - int(kFirstSide) ? maincpu_[kFirstSide + size_t(t)] : 0;
                n.second = (((n.joint == 1 ? b : uint8_t(~b)) & 1) == 0);
            } else {
                n.second = parent >= 0 && s.nodes[size_t(parent)].second;
            }
            const uint32_t kids = rd32(maincpu_, a + 0x18);
            if (n.joint < 0 || n.joint >= s.joints || kids > 16) return;
            const int me = int(s.nodes.size());
            s.nodes.push_back(n);
            for (uint32_t i = 0; i < kids; ++i) node(rd32(maincpu_, a + 0x1C + 4 * i), me, depth + 1);
        };
        if (const uint32_t root = rd32(maincpu_, kSkeletons + 4 * t); root && s.joints > 0 && s.joints < 64) node(root, -1, 0);
    }
    // motions: pointer 0x06xxxxxx = main_data upper half, 0x02xxxxxx = main_data, else maincpu
    motions_.assign(kMotions, {});
    for (int m = 0; m < kMotions; ++m) {
        Motion& mo = motions_[m];
        const uint32_t p = rd32(maincpu_, kMotionPtrs + 4 * m);
        mo.frames = int(rd32(maincpu_, kMotionLens + 4 * m));
        const std::vector<uint8_t>* buf = &maincpu_;
        size_t off = p;
        if (p >> 24 == 6) { buf = &mainData_; off = p - 0x06000000u + 0x1000000u; }
        else if (p >> 24 == 2) { buf = &mainData_; off = p - 0x02000000u; }
        // joint count from the size: frames * record size, padded to 16 bytes, fits exactly one count
        if (m + 1 < kMotions) {
            const uint32_t q = rd32(maincpu_, kMotionPtrs + 4 * (m + 1));
            if (q >> 24 == p >> 24 && q > p)
                for (int j = 1; j < 64; ++j)
                    if ((uint64_t(mo.frames) * recordSize(j) + 15) / 16 * 16 == q - p) { mo.joints = j; break; }
        }
        if (mo.frames <= 0 || off + uint64_t(mo.frames) * recordSize(mo.joints ? mo.joints : 1) > buf->size()) {
            mo = Motion{};
            continue;
        }
        mo.data = buf->data() + off;
        mo.mirrored = kMirrored + size_t(m) < maincpu_.size() && maincpu_[kMirrored + size_t(m)] != 0;
    }
    return true;
}

int CreatureAnimations::nodeOfRole(int type, int role) const
{
    if (type < 0 || type >= int(skeletons_.size())) return -1;
    const std::vector<SkeletonNode>& nodes = skeletons_[size_t(type)].nodes;
    for (size_t n = 0; n < nodes.size(); ++n)
        if (nodes[n].joint == role) return int(n);
    return -1;
}

int CreatureAnimations::partClass(int type, int role, int byte) const
{
    const size_t a = kPartRoles + size_t(type) * 0xF8 + size_t(role) * 8 + size_t(byte & 7);
    if (type < 0 || type >= kTypes || role < 0 || role >= 31 || a >= maincpu_.size()) return -1;
    return int8_t(maincpu_[a]);
}

const int16_t* CreatureAnimations::angles(int motion, int frame) const
{
    const Motion& m = motions_[motion];
    return reinterpret_cast<const int16_t*>(m.data + size_t(frame - 1) * recordSize(m.joints) + 12);
}

const float* CreatureAnimations::root(int motion, int frame) const
{
    const Motion& m = motions_[motion];
    return reinterpret_cast<const float*>(m.data + size_t(frame - 1) * recordSize(m.joints));
}

void CreatureAnimations::jointRotation(int motion, int frame, int joint, float out[9]) const
{
    const int16_t* a = angles(motion, frame) + 3 * joint;
    constexpr float k = 3.14159265358979f / 32768;
    const float x = -a[0] * k, y = -a[1] * k, z = a[2] * k;
    const float cx = std::cos(x), sx = std::sin(x), cy = std::cos(y), sy = std::sin(y), cz = std::cos(z), sz = std::sin(z);
    // Rz(z) * Ry(y) * Rx(x)
    out[0] = cz * cy; out[1] = cz * sy * sx - sz * cx; out[2] = cz * sy * cx + sz * sx;
    out[3] = sz * cy; out[4] = sz * sy * sx + cz * cx; out[5] = sz * sy * cx - cz * sx;
    out[6] = -sy;     out[7] = cy * sx;                out[8] = cy * cx;
}

void CreatureAnimations::pose(int type, int motion, int frame, std::vector<Xform>& out, int motion2, int frame2) const
{
    const Skeleton& s = skeletons_[type];
    out.assign(s.nodes.size(), Xform{});
    if (s.nodes.empty() || !motions_[motion].data || motions_[motion].joints != s.joints) return;
    const float* r = root(motion, frame);
    const float* r1 = root(motion, 1);
    for (size_t i = 0; i < s.nodes.size(); ++i) {
        const SkeletonNode& n = s.nodes[i];
        Xform local;
        const bool use2 = n.second && motion2 >= 0 && motions_[motion2].data && motions_[motion2].joints == s.joints;
        jointRotation(use2 ? motion2 : motion, use2 ? frame2 : frame, n.joint, local.rot);
        if (n.parent < 0) {   // root: (x - x1, y, -(z - z1)), then joint 0
            local.pos[0] = r[0] - r1[0];
            local.pos[1] = r[1];
            local.pos[2] = -(r[2] - r1[2]);
            out[i] = local;
        } else {
            local.pos[0] = n.offset[0];
            local.pos[1] = n.offset[1];
            local.pos[2] = -n.offset[2];
            out[i] = out[n.parent] * local;
            // part_draw: a mirrored part in a mirrored motion keeps its position but takes the body's matrix (the root
            // translation, before the root joint's turn) instead of its parent's rotation
            if (n.mirror && motions_[motion].mirrored) std::copy(local.rot, local.rot + 9, out[i].rot);
        }
    }
}

} // namespace hotd
