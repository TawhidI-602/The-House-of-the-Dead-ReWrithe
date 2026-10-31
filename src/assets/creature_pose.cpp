#include "assets/creature_pose.h"

#include <algorithm>
#include <cstdlib>

namespace hotd {

void toPlacement(const Xform& xf, Placement& pl)
{
    std::copy(std::begin(xf.rot), std::end(xf.rot), pl.rot);
    std::copy(std::begin(xf.pos), std::end(xf.pos), pl.pos);
}

Xform fromPlacement(const Placement& pl)
{
    Xform w;
    std::copy(std::begin(pl.rot), std::end(pl.rot), w.rot);
    std::copy(std::begin(pl.pos), std::end(pl.pos), w.pos);
    return w;
}

std::vector<int> compatibleMotions(const CreatureAnimations& anim, int type)
{
    std::vector<int> out;
    if (type < 0 || type >= anim.typeCount()) return out;
    const Skeleton& sk = anim.skeleton(type);
    if (sk.nodes.empty()) return out;
    for (int m = 0; m < anim.motionCount(); ++m)
        if (anim.motionJoints(m) == sk.joints && anim.motionFrames(m) > 0) out.push_back(m);
    return out;
}

int stepMotion(const CreatureAnimations& anim, int type, int motion, int dir)
{
    if (type < 0 || type >= anim.typeCount() || anim.motionCount() == 0) return motion;
    const int joints = anim.skeleton(type).joints;
    for (int k = 0, m = motion; k < anim.motionCount(); ++k) {
        m = (m + dir + anim.motionCount()) % anim.motionCount();
        if (anim.motionJoints(m) == joints) return m;
    }
    return motion;
}

const char* motionKindName(MotionKind k)
{
    switch (k) {
    case MotionKind::Idle: return "Idle";
    case MotionKind::Walk: return "Walk";
    case MotionKind::Attack: return "Attack";
    case MotionKind::Hit: return "Hit";
    case MotionKind::Death: return "Death";
    default: return "Motion";
    }
}

MotionKind motionKind(const std::string& name)
{
    auto has = [&](std::initializer_list<const char*> words) {
        for (const char* w : words)
            if (name.find(w) != std::string::npos) return true;
        return false;
    };
    if (has({"dead", "sini", "taore", "finish", "sinderu", "saigo", "taor"})) return MotionKind::Death;
    if (has({"hit", "dam", "dm", "yara", "utare", "futtob"})) return MotionKind::Hit;
    if (has({"atk", "att", "naguri", "nagu", "kou", "chop", "tackle", "kamu", "kiri", "nage", "kick", "keri", "tossin",
             "uti", "hikkaki", "bat_", "knif"}))
        return MotionKind::Attack;
    if (has({"walk", "aruk", "aluk", "run", "dash", "hasiri", "hashiru", "fly", "glider", "alu"})) return MotionKind::Walk;
    if (has({"kamae", "pose", "base", "kihon", "taiki", "machi", "mati", "kyoro", "tomari", "otake"})) return MotionKind::Idle;
    return MotionKind::Other;
}

} // namespace hotd

namespace hotd {

// body_part_draw_hook 0x9CE60 (the game: Runtime::body_part_draw_hook, src/game/creature.cpp, and body_hook_special,
// src/game/t1_bounds.cpp), the model part only: no look angles, no flashes, the part undamaged (part +0x78 = 0)
void partDrawModels(const PartDrawState& s, std::vector<uint32_t>& out)
{
    out.clear();
    const int type = s.type, cls = s.cls, m = s.motion, f = s.frame, blink = s.tick;
    const uint32_t own = s.model;
    auto in = [](int v, int lo, int hi) { return v >= lo && v <= hi; };          // inclusive (body_hook_special)
    auto between = [](int v, int lo, int hi) { return lo < v && v < hi; };     // exclusive (the players' case)
    if (type == 50 || type == 69) {   // the Hangedman's wings at role 1: model (+0x50 % 100) + 0x1401 / 0x195F
        if (s.role == 1) out.push_back(uint32_t(s.tick % 100) + (type == 50 ? 0x1401u : 0x195Fu));
        out.push_back(own);
        return;
    }
    if (type == 30) {   // Samson: role 5 (the saw blade, undamaged) cycles 0x557 / 0x558
        out.push_back(s.role == 5 ? uint32_t(blink % 2 + 0x557) : own);
        return;
    }
    if (type == 34) {   // Sophie: the hand holding something, the eyes by motion
        if (cls == 6) {
            const bool hand = in(m, 0x170, 0x171) || m == 0xD3 || in(m, 0xC1, 0xC5) || in(m, 0xBC, 0xBF) || m == 199 ||
                              in(m, 0x15A, 0x15B) || in(m, 0x153, 0x156) || in(m, 0x15E, 0x162) || in(m, 0x17E, 0x180) ||
                              in(m, 0x173, 0x174) || in(m, 0x177, 0x17B) || in(m, 0x1CB, 0x1D2) || m == 400 || in(m, 0x29A, 0x2A0);
            out.push_back(hand ? 0x631u : own);
            return;
        }
        if (cls == 1) {
            uint32_t head = blink % 0x5A > 2 ? 0x60F : 0x60E;   // open (blinking)
            if (in(m, 0x173, 0x174) || in(m, 0x153, 0x156) || in(m, 0xC1, 0xC5) || in(m, 0xBC, 0xBF) || m == 199 || m == 0xD3 ||
                in(m, 0x15E, 0x162) || in(m, 0x15A, 0x15B) || in(m, 0x170, 0x171) || in(m, 0x17E, 0x180) || in(m, 0x177, 0x17B) ||
                m == 0x190 || (m >= 0x192 && (in(m, 0x1CB, 0x1D2) || in(m, 0x29A, 0x2A0))))
                head = 0x61F;   // looking
            else if (m == 0x18A)
                head = uint32_t((s.tick / 2) % 15 + 0x610);
            else if (m == 0x182 || in(m, 0x187, 0x188) || m == 0x18C || m == 0x191)
                head = 0x60E;   // closed
            else if (m == 399)
                head = f > 0x35 && f < 0x45 ? uint32_t(((f - 0x38) % 15 + 15) % 15 + 0x610)
                       : (f < 10 || f > 12) && (f < 0x46 || f > 0x5F) ? 0x60F : 0x60E;
            out.push_back(head);
            return;
        }
        out.push_back(own);
        return;
    }
    if (type == 64) {   // the Hermit (hermit_part_draw_hook 0x87560): the face (role byte 27) draws its animation too
        out.push_back(own);
        if (s.roleId == 27) out.push_back(uint32_t(blink % 50 + 0x1AEE));
        return;
    }
    if (type == 76) {   // the Magician: classes 3 / 4 / 11 / 12 first draw a vein model of a 25-member cycle
        const uint32_t base = cls == 3 ? 0x1C2A : cls == 4 ? 0x1C75 : cls == 11 ? 0x1C47 : cls == 12 ? 0x1C07 : 0;
        if (base) out.push_back(base + uint32_t(blink / 2 % 25));
        out.push_back(own);
        return;
    }
    if (type == 2 || (type >= 13 && type <= 15)) {   // the players' characters: the gun, hand poses, Rogan's coat
        if (cls == 7 && between(m, 0xC1, 0xC4) && between(f, 0x1D, 100)) { out.push_back(0x716); return; }
        if (cls == 6) {
            if (m == 0x168) { out.push_back(0x36B); return; }
            if (between(m, 0xC1, 0xC4) || (between(m, 0x16F, 0x172) && between(f, 0x4C, 0x57))) { out.push_back(0x2B3); return; }
            if ((m == 0xBC && between(f, 0x13, 0x29)) || (m == 0x153 && between(f, 0, 0x13))) { out.push_back(0x71B); return; }
        }
        out.push_back(own);
        if (cls == 8 && type == 2 && m != 0x168 && !s.pcModels)   // the arcade's coat, animated by frame
            out.push_back(uint32_t(f / 2 % 100 + 0xB5));
        return;
    }
    out.push_back(own);
}

std::vector<uint32_t> partDrawExtraModels(int type)
{
    std::vector<uint32_t> ids;
    auto range = [&](uint32_t a, uint32_t n) { for (uint32_t i = 0; i < n; ++i) ids.push_back(a + i); };
    if (type == 50) range(0x1401, 100);
    if (type == 69) range(0x195F, 100);
    if (type == 30) range(0x557, 2);
    if (type == 34) { range(0x60E, 2); range(0x610, 15); ids.push_back(0x61F); ids.push_back(0x631); }
    if (type == 64) range(0x1AEE, 50);
    if (type == 76) { range(0x1C07, 25); range(0x1C2A, 25); range(0x1C47, 25); range(0x1C75, 25); }
    if (type == 2 || (type >= 13 && type <= 15)) { ids.insert(ids.end(), {0x716, 0x36B, 0x2B3, 0x71B}); range(0xB5, 100); }
    return ids;
}

} // namespace hotd

namespace hotd {

std::vector<std::pair<int, int>> parseColorRows(const char* rows)
{
    std::vector<std::pair<int, int>> out;
    for (const char* p = rows ? rows : ""; *p;) {
        char* end = nullptr;
        const long row = std::strtol(p, &end, 10);
        if (end == p || *end != ':') break;
        const char* q = end + 1;
        const long entry = std::strtol(q, &end, 10);
        if (end == q) break;
        out.push_back({int(row), int(entry)});
        p = end;
        while (*p == ' ') ++p;
    }
    return out;
}

const char* arcadeAreaColorRows(int chapter, int area)
{
    // per chapter (0-3) and texture area: the rows with the most creature-frames there (HOTD_TRACE_TYPE_ROWS over Chapters
    // 1-4: the default route, 2 players, both sides of every branch; 2026-10-20). Areas 4 and 9 come back later in the
    // game with other rows.
    struct Row { int chapter, area; const char* rows; };
    static const Row kRows[] = {
        {0, 1, "7:81 9:28"},
        {0, 3, "5:119 7:81 9:28 11:37 19:70 21:100"},
        {0, 4, "7:81 9:1 11:45 15:48 19:117"},
        {1, 5, "5:39 7:81 9:28 11:28 13:55 15:80 17:67 19:59"},
        {1, 6, "5:39 7:81 9:28 11:28"},
        {1, 7, "5:66 7:81 9:103 11:28 13:92 17:32 19:118 29:87"},
        {2, 8, "7:81 9:28"},
        {2, 9, "7:81 9:34 11:38"},
        {2, 10, "7:81 9:50 11:61"},
        {2, 11, "5:53 9:81 11:11 17:98 19:83 21:23 23:17 29:23"},
        {3, 4, "7:81 9:1 11:45 13:26 15:5 17:12 19:104 21:111"},
        {3, 9, "5:39 7:81 9:73 11:0 21:107"},
        {3, 10, "15:68 17:120 19:104"},
        {3, 12, "1:34 5:15 13:37 21:16 23:17"},
    };
    for (const Row& r : kRows)
        if (r.chapter == chapter && r.area == area) return r.rows;
    return "";
}

} // namespace hotd
