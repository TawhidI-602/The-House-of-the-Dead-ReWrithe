// The Ragdoll mod (game/ragdoll.h, docs/modding.md). Draw only: nothing here writes the address space.
//
// How it works, per frame (after the frame's logic, Runtime::frame):
//  1. The draw list is grouped by body (DrawModel::body) and part (DrawModel::part: the part the draw hook drew).
//  2. A creature body (ragall: any type but the players / Sophie / bosses; or a task drawn without a body: bats,
//     Murrers, rats ..., its models in draw order its parts, up to 3 models one rigid part) whose routine (or state, for
//     kind 70 / frogs / bats / kind 65) is a death gets a ragdoll: per
//     drawn part 4 particles (its pivot and three points along its axes), started at the drawn pose, moving with the
//     pose's last step (the death motion's speed).
//  3. Constraints: each part's 4 particles keep their distances (a rigid tetrahedron); a child's pivot keeps its
//     distances to its parent's 4 particles (a ball joint, the pivot fixed in the parent); a part's far end keeps at least
//     the distance from its parent's pivot that a 50 degree bend leaves (a simple angle limit). Verlet steps with the
//     arcade's gravity, 12 solver passes, the floor at the creature's own height with a friction per frame of contact.
//  4. Shots: each new shot flash (shot_effect_flash's first frame) is a ray from the eye; the nearest dead body's bone it
//     crosses is pushed along the shot (the whole body a little, the bone hit more): it slides over the floor.
//  5. Each part's frame is rebuilt from its particles and every model the arcade drew for that part is drawn with it
//     (the model's place relative to its part kept). While the arcade no longer draws the body (the corpse's blinking,
//     or once removed) the last models drawn are drawn again, for Bodies Stay, fading out over its last half second.
#include "game/ragdoll.h"
#include "game/runtime.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <vector>

namespace hotd::game {

RagdollSettings& ragdollSettings()
{
    static RagdollSettings s;
    return s;
}

void ragdollConfigure(bool on, const std::map<std::string, std::string>& values)
{
    RagdollSettings s;
    s.on = on;
    auto get = [&](const char* k, const char* def) {
        auto it = values.find(k);
        return it != values.end() ? it->second : std::string(def);
    };
    // (leftovers, user 2026-10-26: the shot's push is fixed (no setting); Friction Rough / Normal / Smooth)
    const std::string fr = get("friction", "Normal"), stay = get("stay", "8 s"), grav = get("gravity", "Normal");
    s.push = 1.0f;
    s.friction = fr == "Rough" ? 0.3f : fr == "Smooth" ? 0.05f : 0.12f;
    s.dismember = get("gore", "On") != "Off";
    s.stayFrames = stay == "Arcade" ? 0 : std::clamp(std::atoi(stay.c_str()), 1, 120) * 60;
    s.gravity = grav == "Low" ? 0.5f : grav == "Heavy" ? 1.7f : 1.0f;
    ragdollSettings() = s;
}

bool ragdollPropTask(const Runtime& rt, uint32_t task)
{
    if (task == 0)
        return true;
    using F = A::F;
    static const uint32_t kNot[] = {
        F::blood_burst, F::blood_drip, F::hit_splash, F::floor_splat,                  // on a creature / its parts
        F::chariot_hit_glow, F::chariot_armour_piece_fade, F::magician_afterimage,    // a boss's
        F::kind70_spawn9_fade_in,                                                     // a creature fading in
        F::held_object_frame, F::flying_axe_frame, F::held_kit_piece_bounce, F::held_kit_barrel_breakup,   // held / thrown
        F::shot_effect_flash, F::shot_effect_debris,                                  // the shots'
        F::prop_splash, F::frog_splash, F::kind160_splash_anim,                       // splashes
        F::item_sparkle, F::item_life_frame};                                         // pickups
    const uint32_t r = rt.mem.u32(task);
    return std::find(std::begin(kNot), std::end(kNot), r) == std::end(kNot);
}

namespace {

using V3 = std::array<float, 3>;
V3 operator+(V3 a, V3 b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
V3 operator-(V3 a, V3 b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
V3 operator*(V3 a, float s) { return {a[0] * s, a[1] * s, a[2] * s}; }
float dot(V3 a, V3 b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
float len(V3 a) { return std::sqrt(dot(a, a)); }
V3 cross(V3 a, V3 b) { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }
V3 norm(V3 a)
{
    const float l = len(a);
    return l > 1e-9f ? a * (1.0f / l) : V3{0, 0, 0};
}

// an affine 3 x 4: r row-major (column j = the image of axis j), t the translation
struct Mat {
    float r[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    float t[3] = {};
    V3 col(int j) const { return {r[j], r[3 + j], r[6 + j]}; }
    V3 pos() const { return {t[0], t[1], t[2]}; }
};
// the coprocessor's layout (DrawModel::m: the images of x, y, z, then the translation)
Mat fromCopro(const float* m)
{
    Mat o;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            o.r[i * 3 + j] = m[3 * j + i];
        o.t[i] = m[9 + i];
    }
    return o;
}
void toCopro(const Mat& a, float* m)
{
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            m[3 * j + i] = a.r[i * 3 + j];
        m[9 + i] = a.t[i];
    }
}
Mat mul(const Mat& a, const Mat& b)
{
    Mat o;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            o.r[i * 3 + j] = a.r[i * 3] * b.r[j] + a.r[i * 3 + 1] * b.r[3 + j] + a.r[i * 3 + 2] * b.r[6 + j];
        o.t[i] = a.r[i * 3] * b.t[0] + a.r[i * 3 + 1] * b.t[1] + a.r[i * 3 + 2] * b.t[2] + a.t[i];
    }
    return o;
}
float det(const Mat& a)
{
    const float* r = a.r;
    return r[0] * (r[4] * r[8] - r[5] * r[7]) - r[1] * (r[3] * r[8] - r[5] * r[6]) + r[2] * (r[3] * r[7] - r[4] * r[6]);
}
Mat inverse(const Mat& a)
{
    const float* r = a.r;
    const float d = det(a);
    Mat o;
    if (std::abs(d) < 1e-12f)
        return o;
    const float k = 1.0f / d;
    o.r[0] = (r[4] * r[8] - r[5] * r[7]) * k;
    o.r[1] = (r[2] * r[7] - r[1] * r[8]) * k;
    o.r[2] = (r[1] * r[5] - r[2] * r[4]) * k;
    o.r[3] = (r[5] * r[6] - r[3] * r[8]) * k;
    o.r[4] = (r[0] * r[8] - r[2] * r[6]) * k;
    o.r[5] = (r[2] * r[3] - r[0] * r[5]) * k;
    o.r[6] = (r[3] * r[7] - r[4] * r[6]) * k;
    o.r[7] = (r[1] * r[6] - r[0] * r[7]) * k;
    o.r[8] = (r[0] * r[4] - r[1] * r[3]) * k;
    for (int i = 0; i < 3; ++i)
        o.t[i] = -(o.r[i * 3] * a.t[0] + o.r[i * 3 + 1] * a.t[1] + o.r[i * 3 + 2] * a.t[2]);
    return o;
}
bool finite(const Mat& a)
{
    for (float v : a.r)
        if (!std::isfinite(v)) return false;
    for (float v : a.t)
        if (!std::isfinite(v)) return false;
    return true;
}

constexpr int kSolverPasses = 12;
constexpr size_t kMaxRagdolls = 32;   // (ragall: 24 -> 32: the severed heads and the small creatures too)
constexpr int kFadeFrames = 30;

} // namespace

class Ragdolls {
public:
    void frame(Runtime& rt);
    void clear() { rags_.clear(); prevAnim_.clear(); flashes_.clear(); recentShots_.clear(); seen_.clear(); }
    // testing (the clip, HOTD_RAGDOLL_AIM): the k-th dead body on the screen, its middle part, in the arcade's aim units
    bool aim(const Runtime& rt, int k, float& ax, float& ay) const
    {
        const Mat cam = fromCopro(rt.camera);
        std::vector<std::pair<float, float>> on;
        for (const Rag& r : rags_) {
            if (r.parts.empty()) continue;
            const V3 w = r.parts[r.parts.size() / 3].frame.pos();
            V3 c;
            for (int i = 0; i < 3; ++i)
                c[i] = cam.r[i * 3] * w[0] + cam.r[i * 3 + 1] * w[1] + cam.r[i * 3 + 2] * w[2] + cam.t[i];
            if (c[2] < 1.0f) continue;
            const float x = c[0] * 496.0f / c[2], y = c[1] * 496.0f / c[2];
            if (std::abs(x) < 240.0f && std::abs(y) < 185.0f) on.push_back({x, y});
        }
        if (on.empty()) return false;
        ax = on[size_t(k) % on.size()].first;
        ay = on[size_t(k) % on.size()].second;
        return true;
    }

private:
    struct Part {
        uint32_t addr = 0;
        int parent = -1;            // index in parts, -1 the root
        int p = 0;                  // its 4 particles: p pivot, p + 1 .. p + 3 along its x / y / z
        float scale[3] = {1, 1, 1}; // its axes' lengths (the body's scale)
        bool mirrored = false;
        Mat frame;                  // this frame's (rebuilt from the particles)
    };
    struct Con {
        int a, b;
        float rest;
        bool atLeast;               // an angle limit: only kept from getting shorter
    };
    struct Cached {
        DrawModel d;
        int part;
        Mat local;                  // the model's place in its part's frame
    };
    struct CachedSkin {
        Runtime::SkinDraw s;
        int pa, pb;
        Mat la, lb;
    };
    struct Rag {
        uint32_t body = 0;
        int32_t serial = 0;
        int type = 0, id = 0;
        bool loose = false;         // ragall: drawn by its task alone (no body: bats, Murrers, rats ...), body = the task
        bool track = false;         // ragall: its floor follows the arcade's lowest drawn part (not the human skeleton)
        bool arcade = true;         // the arcade still has the body (its task, in a death routine)
        int age = 0, gone = 0;      // frames since the death / since the arcade removed it
        int still = 0;
        int wallHits = 0;           // leftovers: particles kept off a wall (the trace)
        int bodyHits = 0;           // ipolish: frames it was pushed off another body (the trace)
        bool sleeping = false;
        float floor = 0, radius = 1, size = 17;
        std::vector<Part> parts;
        std::vector<V3> x, xp;
        std::vector<Con> cons;
        std::vector<Cached> cache;
        std::vector<CachedSkin> skins;
        int partOf(uint32_t addr) const
        {
            for (size_t i = 0; i < parts.size(); ++i)
                if (parts[i].addr == addr) return int(i);
            return -1;
        }
        int nearest(V3 at) const
        {
            int best = 0;
            float bd = 1e30f;
            for (size_t i = 0; i < parts.size(); ++i)
                if (const float d = len(parts[i].frame.pos() - at); d < bd) bd = d, best = int(i);
            return best;
        }
    };

    std::vector<Rag> rags_;
    std::map<uint32_t, std::map<uint32_t, Mat>> prevAnim_;   // the creatures' part frames of the last frame
    std::set<std::pair<uint32_t, int32_t>> flashes_;          // shot flashes seen (task, serial)
    struct Shot {
        uint64_t frame;
        V3 origin, dir;   // world space
    };
    std::vector<Shot> recentShots_;                            // the last 4 frames' shots (a death a frame or two after its shot)
    uint64_t lastFrame_ = 0;
    bool started_ = false;
    int chapter_ = -1, nextId_ = 1;
    std::map<int, bool> humanType_;

    bool human(Runtime& rt, int type);
    bool humanSkeleton(Runtime& rt, int type) const;
    static bool looseCreature(Runtime& rt, uint32_t task);
    bool dying(Runtime& rt, uint32_t body) const;
    // ragdoll2: Dismemberment Off: each creature's parts as last drawn (their world frame, parent, place among the
    // parent's children, models in the part's frame), to draw the parts the arcade severs on, held by their parent
    struct Seen {
        Mat world;
        uint32_t parent = 0, index = 0;
        uint64_t frame = ~0ull;   // the frame it was last drawn
        std::vector<std::pair<DrawModel, Mat>> models;
    };
    struct SeenBody {
        int32_t serial = 0;
        uint64_t frame = 0;
        std::map<uint32_t, Seen> parts;
        std::vector<uint32_t> order;   // first seen first (parents before children)
    };
    std::map<uint32_t, SeenBody> seen_;
    void keepParts(Runtime& rt, const Mat& cam, const Mat& inv);
    // ragall: a body's drawn parts this frame (a part tree's BodyPart addresses, or a loose creature's model order
    // 0x80000000 + k), in draw order (parents first)
    struct Pose {
        std::map<uint32_t, Mat> at;
        std::vector<uint32_t> order;
        bool loose = false;
    };
    void create(Runtime& rt, uint32_t body, const Pose& pose);
    void step(Rag& r);
    // leftovers: the scene's walls (the view's, this frame; null: none) and the triangles near the body being stepped
    const RagdollWalls* walls_ = nullptr;
    std::vector<const float*> near_, nearFloors_;
    void gatherWalls(const Rag& r);
    bool groundUnder(float x, float z, float top, float& y) const;
    void keepOffWalls(Rag& r, const std::vector<V3>& from);
    void rebuild(Rag& r);
    void push(Rag& r, int part, V3 dir, float k);
    void collideBodies();   // ipolish: the dead bodies pushed off each other (their bones as capsules)
    static bool trace() { static const bool t = std::getenv("HOTD_RAGDOLL_TRACE") != nullptr; return t; }
};

bool Ragdolls::human(Runtime& rt, int type)
{
    auto it = humanType_.find(type);
    if (it != humanType_.end())
        return it->second;
    // ragall: every creature type (its own part tree: zombies, dogs, Ebitan, frogs, the agile ones ...); not the
    // players (2, 13-15), Sophie (34), the bosses (35 / 36 the Chariot, 50 / 55 / 69 the Hangedman, 64 the Hermit, 76
    // the Magician): they keep their own deaths. (Only a death routine makes a ragdoll: the researchers never have one.)
    static const int kNot[] = {2, 13, 14, 15, 34, 35, 36, 50, 55, 64, 69, 76};
    const bool ok = type >= 0 && type < 100 && std::find(std::begin(kNot), std::end(kNot), type) == std::end(kNot);
    humanType_[type] = ok;
    if (trace() && rt.log)
        rt.log("ragdoll: type " + std::to_string(type) + (ok ? " can be a ragdoll" : " is not a ragdoll"));
    return ok;
}

// ragall: the human skeleton (the plain zombie's joint count, type 11): its body stands on its own height (+0x24)
bool Ragdolls::humanSkeleton(Runtime& rt, int type) const
{
    const uint32_t j = rt.mem.u32(0xC73C0 + 4 * uint32_t(type)), z = rt.mem.u32(0xC73C0 + 4 * 11u);
    return j == z && j != 0;
}

// ragall: a creature drawn without a body (its models drawn by its own task: bats, Murrers, rats, slugs, insects, the
// falling frogs, the severed heads), by its task's routine: alive or dying (its draws are grouped by task)
bool Ragdolls::looseCreature(Runtime& rt, uint32_t task)
{
    using F = A::F;
    static const uint32_t kLoose[] = {
        F::bat_frame, F::bat_swarm_frame,                                                   // the bats (by state)
        F::kind35_molb_fall, F::kind35_molb_dead_wait, F::kind35_molb_remove, F::kind39_molb_dying,   // the Murrers
        F::kind175_rat_shot, F::kind142_slug_fall, F::prop083_kyochuu_fall, F::prop063_frog_fall,
        F::prop119_creature_fall, F::severed_part_fall};
    const uint32_t r = rt.mem.u32(task);
    return r != 0 && std::find(std::begin(kLoose), std::end(kLoose), r) != std::end(kLoose);
}

bool Ragdolls::dying(Runtime& rt, uint32_t body) const
{
    using F = A::F;
    using G = A::G;
    const uint32_t r = rt.mem.u32(body);
    static const uint32_t kDead[] = {
        F::enemy_dying, F::enemy_corpse, F::enemy_corpse_sink, F::enemy_fall_death_land, F::enemy_scripted_dead,
        F::enemy_scripted_dead_fall, F::Zombie_Dead, F::Zombie_Dead_frame,                      // the zombies, the dogs
        F::kind132_ebitb_dead, F::kind135_ebitb_dead_fall, F::kind135_ebitb_dead_glide, F::kind150_ebitan_slide_dying,   // Ebitan
        F::kind153_taran_fall_off,                                                               // Taran
        F::kind35_molb_fall, F::kind35_molb_dead_wait, F::kind35_molb_remove, F::kind39_molb_dying,   // the Murrers
        F::kind175_rat_shot, F::kind142_slug_fall, F::prop083_kyochuu_fall, F::prop063_frog_fall,
        F::prop119_creature_fall, F::severed_part_fall};
    if (r != 0 && std::find(std::begin(kDead), std::end(kDead), r) != std::end(kDead))
        return true;
    // the kinds with one frame routine and a state table (+0x5C): their death states
    uint32_t table = 0;
    if (r == F::kind70_frame) table = G::kind70_states;
    else if (r == F::kind69_frog_frame) table = G::kind69_states;
    else if (r == F::bat_frame || r == F::bat_swarm_frame) table = G::bat_states;
    else if (r == F::kind65_frame) table = G::kind65_states;
    if (table == 0)
        return false;
    const int st = rt.mem.at<int16_t>(body + 0x5C);
    if (st < 0 || st >= 48)
        return false;
    const uint32_t s = rt.mem.u32(table + 4 * uint32_t(st));
    return s == F::kind70_st01_death || s == F::kind69_frog_st2_die || s == F::bat_st5_shot || s == F::bat_st6_fall ||
           s == F::kind65_st3_die;
}

void Ragdolls::create(Runtime& rt, uint32_t body, const Pose& pose)
{
    const std::map<uint32_t, Mat>& anim = pose.at;
    if (anim.empty())
        return;
    Rag r;
    r.body = body;
    r.loose = pose.loose;
    r.serial = int32_t(rt.mem.u32(body + 8));
    r.type = pose.loose ? -1 : rt.mem.at<int16_t>(body + 0xCC);
    r.id = nextId_++;
    // the body's size (its drawn parts' extent) sets the particle arms, the floor radius and the pushes
    V3 lo{1e30f, 1e30f, 1e30f}, hi{-1e30f, -1e30f, -1e30f};
    for (const auto& [a, m] : anim)
        for (int k = 0; k < 3; ++k)
            lo[k] = std::min(lo[k], m.t[k]), hi[k] = std::max(hi[k], m.t[k]);
    r.size = std::clamp(len(hi - lo), 6.0f, 80.0f);
    r.radius = std::clamp(r.size * 0.06f, 0.5f, 3.0f);
    const float arm = std::clamp(r.size * 0.12f, 0.5f, 6.0f);
    // the floor: the creature's own height (+0x24: where its logic keeps it), never above its lowest part. ragall: a
    // creature not on the human skeleton (flying, on a wall or a ceiling, in the air: bats, Moody, Taran, Ebitan ...)
    // and the loose ones: its lowest part now, then (Ragdolls::frame) the lowest the arcade's own death draws it, so
    // it lands where the arcade's body lands
    float floor = rt.mem.f32(body + 0x24);
    r.track = pose.loose || !humanSkeleton(rt, r.type);
    if (r.track)
        floor = lo[1] - r.radius;
    if (!std::isfinite(floor) || floor > lo[1])
        floor = lo[1] - r.radius;
    r.floor = std::min(floor, lo[1] - r.radius);   // (a pivot already lower: the floor under it)
    // the parts (in the draw order: parents first) and their particles. ragall: a loose creature of up to 3 models is
    // one rigid body (its first model's frame; the others ride on it)
    std::vector<uint32_t> order = pose.order;
    if (pose.loose && order.size() <= 3)
        order.resize(1);
    const auto prev = prevAnim_.find(body);
    for (uint32_t a : order) {
        Part p;
        p.addr = a;
        // the start pose: the last living pose drawn (the death motion's first frame often jumps from it), else this one
        const Mat* was = nullptr;
        if (prev != prevAnim_.end())
            if (auto pw = prev->second.find(a); pw != prev->second.end()) was = &pw->second;
        const Mat& now = anim.at(a);
        p.frame = was ? *was : now;
        p.mirrored = det(p.frame) < 0;
        p.p = int(r.x.size());
        V3 axis[3];
        for (int j = 0; j < 3; ++j) {
            const V3 c = p.frame.col(j);
            p.scale[j] = std::max(len(c), 1e-6f);
            axis[j] = c * (1.0f / p.scale[j]);
        }
        const V3 o = p.frame.pos();
        const V3 pts[4] = {o, o + axis[0] * arm, o + axis[1] * arm, o + axis[2] * arm};
        // each point's step to this frame's pose (averaged below: the start velocity)
        const Mat rel = mul(now, inverse(p.frame));
        for (int k = 0; k < 4; ++k) {
            V3 q;
            for (int i = 0; i < 3; ++i)
                q[i] = rel.r[i * 3] * pts[k][0] + rel.r[i * 3 + 1] * pts[k][1] + rel.r[i * 3 + 2] * pts[k][2] + rel.t[i];
            r.x.push_back(pts[k]);
            r.xp.push_back(pts[k] - (q - pts[k]));
        }
        r.parts.push_back(p);
    }
    // the start velocity: the body's mean step only (the death motion's first frame often jumps from the last pose:
    // per point it would spin the body), at most 0.5 units a frame (a camera cut or a teleport is not a speed)
    {
        V3 mean{0, 0, 0};
        for (size_t i = 0; i < r.x.size(); ++i) mean = mean + (r.x[i] - r.xp[i]);
        mean = mean * (1.0f / float(std::max<size_t>(r.x.size(), 1)));
        mean[1] = std::min(mean[1], 0.0f);   // (never up: a dying body does not leap)
        // ragall: a loose creature not seen alive last frame (a severed head, a shot rat): its task's own speed
        // (+0x78..+0x80, where the arcade throws it), lift kept
        if (pose.loose && prev == prevAnim_.end()) {
            const V3 v{rt.mem.f32(body + 0x78), rt.mem.f32(body + 0x7C), rt.mem.f32(body + 0x80)};
            if (std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2])) mean = v;
        }
        if (len(mean) > 0.5f) mean = norm(mean) * 0.5f;
        for (size_t i = 0; i < r.x.size(); ++i) r.xp[i] = r.x[i] - mean;
    }
    // the tree: each part's parent (BodyPart +0x20) among the drawn ones; the root's children all hang off the first.
    // ragall: a loose creature's models (no tree): each hangs off the nearest model drawn before it
    for (size_t i = 0; i < r.parts.size(); ++i) {
        Part& p = r.parts[i];
        if (!pose.loose) {
            p.parent = r.partOf(rt.mem.u32(p.addr + 0x20));
            continue;
        }
        float bd = 1e30f;
        for (size_t k = 0; k < i; ++k)
            if (const float d = len(r.parts[k].frame.pos() - p.frame.pos()); d < bd) bd = d, p.parent = int(k);
    }
    int root = -1;
    for (size_t i = 0; i < r.parts.size(); ++i)
        if (r.parts[i].parent < 0) {
            if (root < 0) root = int(i);
            else r.parts[i].parent = root;
        }
    auto rest = [&](int a, int b, bool atLeast = false) { r.cons.push_back({a, b, len(r.x[size_t(a)] - r.x[size_t(b)]), atLeast}); };
    for (size_t i = 0; i < r.parts.size(); ++i) {
        const Part& p = r.parts[i];
        for (int a = 0; a < 4; ++a)
            for (int b = a + 1; b < 4; ++b)
                rest(p.p + a, p.p + b);
        if (p.parent >= 0)
            for (int k = 0; k < 4; ++k)
                rest(p.p, r.parts[size_t(p.parent)].p + k);
    }
    // the angle limits: a part's first child's pivot (its far end) keeps from folding back onto the parent's pivot
    constexpr float kMinBend = 50.0f * 3.14159265f / 180.0f;
    for (size_t i = 0; i < r.parts.size(); ++i) {
        const Part& c = r.parts[i];
        if (c.parent < 0) continue;
        int far = -1;
        for (size_t k = 0; k < r.parts.size() && far < 0; ++k)
            if (r.parts[k].parent == int(i)) far = int(k);
        if (far < 0) continue;
        const V3 P = r.x[size_t(r.parts[size_t(c.parent)].p)], J = r.x[size_t(c.p)], K = r.x[size_t(r.parts[size_t(far)].p)];
        const float a = len(P - J), b = len(K - J);
        if (a < 0.3f || b < 0.3f) continue;
        const float dmin = std::min(std::sqrt(std::max(a * a + b * b - 2 * a * b * std::cos(kMinBend), 0.0f)), len(P - K) * 0.98f);
        r.cons.push_back({r.parts[size_t(c.parent)].p, r.parts[size_t(far)].p, dmin, true});
    }
    if (trace() && rt.log) {
        char buf[200];
        std::snprintf(buf, sizeof buf, "ragdoll %d: body %08X type %d, %zu parts, size %.1f, floor %.2f (pos y %.2f, lowest %.2f), frame %llu",
                      r.id, body, r.type, r.parts.size(), r.size, r.floor, rt.mem.f32(body + 0x24), lo[1],
                      (unsigned long long)rt.frameCount);
        rt.log(std::string(buf) + ", " + rt.routineName(rt.mem.u32(body)));
    }
    if (rags_.size() >= kMaxRagdolls) {   // the oldest one the arcade no longer has makes room (else the oldest)
        auto old = std::find_if(rags_.begin(), rags_.end(), [](const Rag& q) { return !q.arcade; });
        rags_.erase(old != rags_.end() ? old : rags_.begin());
    }
    rags_.push_back(std::move(r));
}

// leftovers: the walls' triangles near the body (the active world objects whose box meets the body's, both this frame's
// start and its end, with room for a frame's fastest move)
void Ragdolls::gatherWalls(const Rag& r)
{
    near_.clear();
    nearFloors_.clear();
    if (!walls_ || r.x.empty())
        return;
    V3 lo{1e30f, 1e30f, 1e30f}, hi{-1e30f, -1e30f, -1e30f};
    for (size_t i = 0; i < r.x.size(); ++i)
        for (int k = 0; k < 3; ++k) {
            lo[k] = std::min({lo[k], r.x[i][k], r.xp[i][k]});
            hi[k] = std::max({hi[k], r.x[i][k], r.xp[i][k]});
        }
    const float m = r.radius * 2.0f + 6.0f;
    for (int k = 0; k < 3; ++k)
        lo[k] -= m, hi[k] += m;
    for (const int oi : walls_->active) {
        if (oi < 0 || size_t(oi) >= walls_->objs.size())
            continue;
        const RagdollWalls::Obj& o = walls_->objs[size_t(oi)];
        if (o.hi[0] < lo[0] || o.lo[0] > hi[0] || o.hi[1] < lo[1] || o.lo[1] > hi[1] || o.hi[2] < lo[2] || o.lo[2] > hi[2])
            continue;
        for (size_t t = 0; t + 11 < o.tris.size(); t += 12) {
            const float* p = &o.tris[t];
            bool out = false;
            for (int k = 0; k < 3 && !out; ++k)
                out = std::max({p[k], p[3 + k], p[6 + k]}) < lo[k] || std::min({p[k], p[3 + k], p[6 + k]}) > hi[k];
            if (!out)
                near_.push_back(p);
        }
        // the ground: the lying triangles under the body's box (down to 40 units below it)
        for (size_t t = 0; t + 11 < o.floors.size(); t += 12) {
            const float* p = &o.floors[t];
            if (std::max({p[0], p[3], p[6]}) < lo[0] || std::min({p[0], p[3], p[6]}) > hi[0] ||
                std::max({p[2], p[5], p[8]}) < lo[2] || std::min({p[2], p[5], p[8]}) > hi[2] ||
                std::min({p[1], p[4], p[7]}) > hi[1] || std::max({p[1], p[4], p[7]}) < lo[1] - 40.0f)
                continue;
            nearFloors_.push_back(p);
        }
    }
}

// leftovers: the ground's height under (x, z): the highest lying triangle there that is not above `top`; false: none
bool Ragdolls::groundUnder(float x, float z, float top, float& y) const
{
    bool found = false;
    for (const float* t : nearFloors_) {
        // (x, z) inside the triangle's ground plan (barycentric in x / z)
        const float ax = t[0], az = t[2], bx = t[3] - ax, bz = t[5] - az, cx = t[6] - ax, cz = t[8] - az;
        const float den = bx * cz - bz * cx;
        if (std::abs(den) < 1e-6f)
            continue;
        const float px = x - ax, pz = z - az;
        const float v = (px * cz - pz * cx) / den, w = (bx * pz - bz * px) / den;
        if (v < -0.01f || w < -0.01f || v + w > 1.01f)
            continue;
        const float h = t[1] + v * (t[4] - t[1]) + w * (t[7] - t[1]);
        if (h <= top && (!found || h > y))
            y = h, found = true;
    }
    return found;
}

// leftovers: each particle stays on the side of the near walls it started the frame on, a radius off them (where its
// place, or where its path crossed, lies over the triangle); its speed into the wall stops, along it is kept
void Ragdolls::keepOffWalls(Rag& r, const std::vector<V3>& from)
{
    const float rad = r.radius * 1.5f;
    for (size_t i = 0; i < r.x.size(); ++i)
        for (const float* t : near_) {
            const V3 a{t[0], t[1], t[2]}, n{t[9], t[10], t[11]};
            const float d0 = dot(from[i] - a, n), d1 = dot(r.x[i] - a, n);
            if (d0 * d1 > 0.0f && std::min(std::abs(d0), std::abs(d1)) >= rad)
                continue;   // (both well on one side: the usual case, cheap)
            const V3 b{t[3], t[4], t[5]}, c{t[6], t[7], t[8]};
            const float side = d0 >= 0.0f ? 1.0f : -1.0f;
            // how far off it stays: the radius, or less when it started nearer (never pushed out further than it was:
            // no jump at the death pose)
            const float keep = std::min(rad, side * d0);
            if (side * d1 >= keep || std::abs(d0) > 40.0f)
                continue;   // (far enough on its side)
            // the point on the wall's plane: where the path crossed it, else under the particle
            V3 q = r.x[i] - n * d1;
            if (side * d1 < 0.0f && std::abs(d0 - d1) > 1e-6f)
                q = from[i] + (r.x[i] - from[i]) * (d0 / (d0 - d1));
            // over the triangle (barycentric, a little slack at its edges)
            const V3 v0 = b - a, v1 = c - a, v2 = q - a;
            const float d00 = dot(v0, v0), d01 = dot(v0, v1), d11 = dot(v1, v1), d20 = dot(v2, v0), d21 = dot(v2, v1);
            const float den = d00 * d11 - d01 * d01;
            if (std::abs(den) < 1e-9f)
                continue;
            const float v = (d11 * d20 - d01 * d21) / den, w = (d00 * d21 - d01 * d20) / den;
            const float slack = -0.02f;
            if (v < slack || w < slack || v + w > 1.0f - slack)
                continue;
            r.x[i] = r.x[i] + n * (side * keep - d1);
            const V3 vel = r.x[i] - r.xp[i];
            r.xp[i] = r.xp[i] + n * dot(vel, n);   // (no speed into the wall)
            ++r.wallHits;
        }
}

void Ragdolls::step(Rag& r)
{
    const RagdollSettings& s = ragdollSettings();
    if (r.sleeping)
        return;
    const float g = 0.0272f * s.gravity;
    float most = 0;
    for (size_t i = 0; i < r.x.size(); ++i) {
        V3 v = (r.x[i] - r.xp[i]) * 0.995f;
        if (const float l = len(v); l > 6.0f) v = v * (6.0f / l);
        r.xp[i] = r.x[i];
        r.x[i] = r.x[i] + v;
        r.x[i][1] -= g;
    }
    gatherWalls(r);
    const std::vector<V3> from = r.xp;   // (leftovers: the frame's start: the side of a wall each particle is on)
    // the ground under each point: the creature's floor height, or (leftovers: with the view's world) the floor, step or
    // slope under it (never more than 8 units below the creature's floor: no fall through a gap in the floor; a point
    // that started below the surface stays where it was: no jump)
    const float base = r.floor + r.radius;
    std::vector<float> gr(r.x.size(), base);
    if (!nearFloors_.empty())
        for (size_t i = 0; i < r.x.size(); ++i) {
            float y = 0;
            const float top = std::max(from[i][1], r.x[i][1]) + r.radius * 0.5f;
            if (!groundUnder(r.x[i][0], r.x[i][2], top, y))
                continue;
            gr[i] = std::max(y + r.radius, base - 8.0f);
            if (gr[i] > from[i][1])
                gr[i] = std::max(from[i][1], std::min(base, gr[i]));
        }
    for (int pass = 0; pass < kSolverPasses; ++pass) {
        for (const Con& c : r.cons) {
            V3& a = r.x[size_t(c.a)];
            V3& b = r.x[size_t(c.b)];
            const V3 d = b - a;
            const float l = len(d);
            if (l < 1e-6f || (c.atLeast && l >= c.rest)) continue;
            const V3 corr = d * (0.5f * (l - c.rest) / l);
            a = a + corr;
            b = b - corr;
        }
        for (size_t i = 0; i < r.x.size(); ++i)
            if (r.x[i][1] < gr[i]) r.x[i][1] = gr[i];
        if (!near_.empty())
            keepOffWalls(r, from);
    }
    // the floor's friction: a particle touching it keeps (1 - friction) of its sliding speed and no fall speed;
    // (leftovers) a slow one stops (static friction: no creeping, no endless slide), and the body's points off the
    // floor lose a little too while most of it lies (the whole body comes to rest together)
    int grounded = 0;
    for (size_t i = 0; i < r.x.size(); ++i)
        grounded += r.x[i][1] <= gr[i] + 0.05f;
    const bool lying = grounded * 3 >= int(r.x.size());
    for (size_t i = 0; i < r.x.size(); ++i) {
        const bool on = r.x[i][1] <= gr[i] + 0.05f;
        if (on || lying) {
            const float k = 1.0f - (on ? s.friction : s.friction * 0.35f);
            r.xp[i][0] = r.x[i][0] - (r.x[i][0] - r.xp[i][0]) * k;
            r.xp[i][2] = r.x[i][2] - (r.x[i][2] - r.xp[i][2]) * k;
            if (on && r.xp[i][1] > r.x[i][1]) r.xp[i][1] = r.x[i][1];   // (falling: stopped; a push's lift kept)
            const float hx = r.x[i][0] - r.xp[i][0], hz = r.x[i][2] - r.xp[i][2];
            if (on && hx * hx + hz * hz < 0.012f * 0.012f)
                r.xp[i][0] = r.x[i][0], r.xp[i][2] = r.x[i][2];
        }
        most = std::max(most, len(r.x[i] - r.xp[i]));
    }
    // settled: asleep until a shot wakes it (no creeping, no jitter)
    r.still = most < 0.02f ? r.still + 1 : 0;
    if (r.still > 30) {
        r.sleeping = true;
        r.xp = r.x;
    }
}

void Ragdolls::rebuild(Rag& r)
{
    for (Part& p : r.parts) {
        const V3 o = r.x[size_t(p.p)];
        const V3 e0 = norm(r.x[size_t(p.p + 1)] - o);
        V3 e1 = r.x[size_t(p.p + 2)] - o;
        e1 = norm(e1 - e0 * dot(e0, e1));
        V3 e2 = cross(e0, e1);
        if (p.mirrored) e2 = e2 * -1.0f;
        if (len(e0) < 0.5f || len(e1) < 0.5f) continue;   // (degenerate: keep the last frame)
        const V3 c[3] = {e0 * p.scale[0], e1 * p.scale[1], e2 * p.scale[2]};
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j)
                p.frame.r[i * 3 + j] = c[j][i];
            p.frame.t[i] = o[i];
        }
    }
}

// ipolish: bodies against bodies. Each bone (a part's pivot to its parent's; the root a sphere) is a capsule as thick
// as the body's size gives; two bodies' boxes that meet have their crossing capsules moved apart, half each (a sleeping
// body moves only if pushed more than a little, and wakes), the move taken from the speed too (no bounce)
void Ragdolls::collideBodies()
{
    if (rags_.size() < 2)
        return;
    struct Box { V3 lo, hi; };
    std::vector<Box> box(rags_.size());
    for (size_t i = 0; i < rags_.size(); ++i) {
        Box& b = box[i];
        b.lo = {1e30f, 1e30f, 1e30f}, b.hi = {-1e30f, -1e30f, -1e30f};
        for (const V3& p : rags_[i].x)
            for (int k = 0; k < 3; ++k) b.lo[k] = std::min(b.lo[k], p[k]), b.hi[k] = std::max(b.hi[k], p[k]);
        const float m = std::clamp(rags_[i].size * 0.09f, 0.6f, 4.0f);
        for (int k = 0; k < 3; ++k) b.lo[k] -= m, b.hi[k] += m;
    }
    // the closest points of segments p0-p1 and q0-q1: their parameters s, t (0..1)
    auto closest = [](V3 p0, V3 p1, V3 q0, V3 q1, float& s, float& t) {
        const V3 d1 = p1 - p0, d2 = q1 - q0, r = p0 - q0;
        const float a = dot(d1, d1), e = dot(d2, d2), f = dot(d2, r);
        if (a < 1e-9f && e < 1e-9f) { s = t = 0; return; }
        if (a < 1e-9f) { s = 0; t = std::clamp(f / e, 0.0f, 1.0f); return; }
        const float c = dot(d1, r);
        if (e < 1e-9f) { t = 0; s = std::clamp(-c / a, 0.0f, 1.0f); return; }
        const float b = dot(d1, d2), den = a * e - b * b;
        s = den > 1e-9f ? std::clamp((b * f - c * e) / den, 0.0f, 1.0f) : 0.0f;
        t = (b * s + f) / e;
        if (t < 0) t = 0, s = std::clamp(-c / a, 0.0f, 1.0f);
        else if (t > 1) t = 1, s = std::clamp((b - c) / a, 0.0f, 1.0f);
    };
    // a part's 4 particles moved by v (the speed kept: the previous places too)
    auto move = [](Rag& r, int part, V3 v) {
        const int p0 = r.parts[size_t(part)].p;
        for (int k = 0; k < 4; ++k) {
            r.x[size_t(p0 + k)] = r.x[size_t(p0 + k)] + v;
            r.xp[size_t(p0 + k)] = r.xp[size_t(p0 + k)] + v;
        }
    };
    for (size_t i = 0; i < rags_.size(); ++i)
        for (size_t j = i + 1; j < rags_.size(); ++j) {
            const Box &A = box[i], &B = box[j];
            if (A.hi[0] < B.lo[0] || B.hi[0] < A.lo[0] || A.hi[1] < B.lo[1] || B.hi[1] < A.lo[1] || A.hi[2] < B.lo[2] || B.hi[2] < A.lo[2])
                continue;
            Rag &ra = rags_[i], &rb = rags_[j];
            if (ra.sleeping && rb.sleeping)
                continue;
            const float tA = std::clamp(ra.size * 0.09f, 0.6f, 4.0f), tB = std::clamp(rb.size * 0.09f, 0.6f, 4.0f);
            // (two bodies that start in each other, dying in one spot: pushed apart over a few frames, not at once)
            constexpr float kMostStep = 0.4f;
            float movedA = 0, movedB = 0;
            for (size_t pa = 0; pa < ra.parts.size(); ++pa) {
                const Part& a = ra.parts[pa];
                const V3 a0 = ra.x[size_t(a.p)], a1 = a.parent >= 0 ? ra.x[size_t(ra.parts[size_t(a.parent)].p)] : a0;
                for (size_t pb = 0; pb < rb.parts.size(); ++pb) {
                    const Part& b = rb.parts[pb];
                    const V3 b0 = rb.x[size_t(b.p)], b1 = b.parent >= 0 ? rb.x[size_t(rb.parts[size_t(b.parent)].p)] : b0;
                    float s = 0, t = 0;
                    closest(a0, a1, b0, b1, s, t);
                    const V3 ca = a0 + (a1 - a0) * s, cb = b0 + (b1 - b0) * t;
                    V3 d = ca - cb;
                    const float l = len(d), want = tA + tB;
                    if (l >= want)
                        continue;
                    d = l > 1e-5f ? d * (1.0f / l) : V3{0, 1, 0};
                    d[1] = std::max(d[1], 0.0f);   // (sideways or up: never into the floor)
                    d = norm(d);
                    const float depth = std::min((want - l) * 0.5f, kMostStep);
                    // half each, shared between the bone's two ends by where they touch (asleep: kept still)
                    const float wa = ra.sleeping ? 0.0f : rb.sleeping ? 2.0f : 1.0f, wb = rb.sleeping ? 0.0f : ra.sleeping ? 2.0f : 1.0f;
                    move(ra, int(pa), d * (depth * wa * (1.0f - s)));
                    if (a.parent >= 0) move(ra, a.parent, d * (depth * wa * s));
                    move(rb, int(pb), d * (-depth * wb * (1.0f - t)));
                    if (b.parent >= 0) move(rb, b.parent, d * (-depth * wb * t));
                    movedA += depth * wa, movedB += depth * wb;
                }
            }
            ra.bodyHits += movedA > 0, rb.bodyHits += movedB > 0;   // (the trace)
        }
}

void Ragdolls::push(Rag& r, int part, V3 dir, float k)
{
    // along the shot, mostly over the floor (a little lift so it slides instead of digging in)
    V3 j{dir[0], 0.0f, dir[2]};
    const float h = len(j);
    j = h > 1e-4f ? j * (1.0f / h) : V3{0, 0, 0};
    j[1] = 0.15f + std::max(dir[1], 0.0f) * 0.3f;
    const float speed = k * std::clamp(r.size / 17.0f, 0.5f, 3.0f);
    for (V3& p : r.xp)
        p = p - j * (0.3f * speed);
    const Part& hit = r.parts[size_t(part)];
    for (int i = 0; i < 4; ++i)
        r.xp[size_t(hit.p + i)] = r.xp[size_t(hit.p + i)] - j * (0.35f * speed);
    if (hit.parent >= 0)
        r.xp[size_t(r.parts[size_t(hit.parent)].p)] = r.xp[size_t(r.parts[size_t(hit.parent)].p)] - j * (0.15f * speed);
    r.sleeping = false;
    r.still = 0;
}

// ragdoll2: Dismemberment Off. Every creature body's parts as drawn (their frames and models, by part); a part no
// longer drawn whose own sever mark (its parent's +0x28 byte at its place +0x30: part_blow_off) or an ancestor's is
// set is drawn again where its parent is now, as it last hung from it (its models, its shadow). The pieces the
// arcade throws (severed_part_fall) are not drawn (Ragdolls::frame). Draws only
void Ragdolls::keepParts(Runtime& rt, const Mat& cam, const Mat& inv)
{
    // this frame's part frames (the first model of each part), the creature bodies only
    std::map<uint32_t, std::map<uint32_t, Mat>> now;
    for (const DrawModel& d : rt.drawList)
        if (d.body != 0 && d.part != 0 && !d.shadow) {
            auto& m = now[d.body];
            if (!m.count(d.part)) {
                const Mat w = mul(inv, fromCopro(d.m));
                if (finite(w)) m.emplace(d.part, w);
            }
        }
    // the severed parts of the bodies seen before, drawn at their parents
    std::set<std::pair<uint32_t, uint32_t>> added;
    std::vector<DrawModel> extra;
    for (auto& [body, sb] : seen_) {
        auto nb = now.find(body);
        if (nb == now.end() || int32_t(rt.mem.u32(body + 8)) != sb.serial)
            continue;
        auto severed = [&](uint32_t part) {
            for (int depth = 0; depth < 16; ++depth) {
                auto e = sb.parts.find(part);
                if (e == sb.parts.end() || e->second.parent == 0 || e->second.index >= 32)
                    return false;
                if (rt.mem.at<uint8_t>(e->second.parent + 0x28 + e->second.index) & 1)
                    return true;
                if (nb->second.count(e->second.parent) && !added.count({body, e->second.parent}))
                    return false;   // (its parent drawn by the arcade and its own mark clear)
                part = e->second.parent;
            }
            return false;
        };
        for (const uint32_t part : sb.order) {
            if (nb->second.count(part)) continue;
            const Seen& e = sb.parts[part];
            auto par = nb->second.find(e.parent);
            auto pseen = sb.parts.find(e.parent);
            if (par == nb->second.end() || pseen == sb.parts.end() || !severed(part)) continue;
            const Mat w = mul(par->second, mul(inverse(pseen->second.world), e.world));
            if (!finite(w)) continue;
            nb->second.emplace(part, w);
            added.insert({body, part});
            for (const auto& [dm, local] : e.models) {
                DrawModel d = dm;
                toCopro(mul(cam, mul(w, local)), d.m);
                extra.push_back(d);
            }
        }
    }
    // what is seen (not the parts drawn again above: they keep their last real frame)
    for (const DrawModel& d : rt.drawList) {
        if (d.body == 0 || d.part == 0) continue;
        auto nb = now.find(d.body);
        if (nb == now.end()) continue;
        auto pw = nb->second.find(d.part);
        if (pw == nb->second.end()) continue;
        SeenBody& sb = seen_[d.body];
        const int32_t serial = int32_t(rt.mem.u32(d.body + 8));
        if (sb.serial != serial)
            sb = SeenBody{}, sb.serial = serial;
        sb.frame = rt.frameCount;
        auto [it, fresh] = sb.parts.try_emplace(d.part);
        if (fresh) sb.order.push_back(d.part);
        Seen& e = it->second;
        if (e.frame != rt.frameCount) {   // (the part's first model this frame: its models drawn now replace the last)
            e.frame = rt.frameCount;
            e.models.clear();
            e.world = pw->second;
            e.parent = rt.mem.u32(d.part + 0x20);
            e.index = rt.mem.u32(d.part + 0x30);
        }
        e.models.push_back({d, mul(inverse(pw->second), mul(inv, fromCopro(d.m)))});
    }
    for (auto it = seen_.begin(); it != seen_.end();)
        it = it->second.frame + 600 < rt.frameCount ? seen_.erase(it) : std::next(it);
    if (trace() && rt.log && !added.empty() && rt.frameCount % 30 == 0)
        rt.log("ragdoll: frame " + std::to_string(rt.frameCount) + ": " + std::to_string(added.size()) + " severed parts kept on (" +
               std::to_string(extra.size()) + " models)");
    rt.drawList.insert(rt.drawList.end(), extra.begin(), extra.end());
}

void Ragdolls::frame(Runtime& rt)
{
    const RagdollSettings& s = ragdollSettings();
    const int chapter = rt.mem.at<int8_t>(A::G::chapter);
    // a new game / chapter, or a seek back (the cut-scene viewer): start over. A netplay rollback's re-run of frames
    // already stepped only draws (frameCount not past the last stepped one)
    bool advance = rt.frameCount > lastFrame_;
    if (!started_ || chapter != chapter_ || rt.frameCount + 600 < lastFrame_) {
        clear();
        chapter_ = chapter;
        started_ = advance = true;
    }
    if (advance) lastFrame_ = rt.frameCount;
    const Mat cam = fromCopro(rt.camera), inv = inverse(cam);
    // (testing, HOTD_RAGDOLL_KINDS=1: every drawn body's routine changes: type, routine, parts drawn)
    static const bool kinds = std::getenv("HOTD_RAGDOLL_KINDS") != nullptr;
    if (kinds && advance && rt.log) {
        static std::map<uint32_t, uint64_t> lastRoutine;
        std::map<uint32_t, std::pair<std::set<uint32_t>, int>> seen;
        for (const DrawModel& d : rt.drawList)
            if (!d.shadow && (d.body != 0 || d.task != 0)) {
                auto& e = seen[d.body ? d.body : d.task];
                if (d.part) e.first.insert(d.part);
                else ++e.second;
            }
        for (const auto& [b, e] : seen) {
            const uint32_t r = rt.mem.u32(b);
            const uint64_t key = (uint64_t(r) << 16) | rt.mem.at<uint16_t>(b + 0x5C);
            if (lastRoutine[b] == key) continue;
            lastRoutine[b] = key;
            char buf[260];
            std::snprintf(buf, sizeof buf, "kinds: f%llu body %08X type %d routine %s state %d parts %zu loose %d life %d flags %X model %X",
                          (unsigned long long)rt.frameCount, b, rt.mem.at<int16_t>(b + 0xCC), rt.routineName(r).c_str(),
                          rt.mem.at<int16_t>(b + 0x5C), e.first.size(), e.second, rt.mem.at<int16_t>(b + 0x70), rt.mem.u32(b + 0x58),
                          rt.mem.u32(b + 0x54));
            rt.log(buf);
        }
    }
    // ragdoll2: Dismemberment Off: the severed pieces not drawn, the severed parts drawn on their bodies
    if (!s.dismember) {
        const size_t n0 = rt.drawList.size();
        rt.drawList.erase(std::remove_if(rt.drawList.begin(), rt.drawList.end(),
                                         [&](const DrawModel& d) { return d.task != 0 && d.body == 0 && rt.mem.u32(d.task) == A::F::severed_part_fall; }),
                          rt.drawList.end());
        if (trace() && rt.log && advance && rt.drawList.size() != n0)
            rt.log("ragdoll: frame " + std::to_string(rt.frameCount) + ": " + std::to_string(n0 - rt.drawList.size()) +
                   " severed piece models not drawn");
        keepParts(rt, cam, inv);
    } else if (!seen_.empty()) {
        seen_.clear();
    }
    // 1. this frame's creature bodies: their parts' world frames (the first model each part drew). ragall: every
    // creature type's part tree (by body / part), and the loose creatures' models (by task / model order)
    const size_t nDraw0 = rt.drawList.size();
    std::vector<std::pair<uint32_t, uint32_t>> who(nDraw0, {0u, 0u});   // per draw: its creature, its part (0: none)
    std::map<uint32_t, Pose> poses;
    std::map<uint32_t, uint32_t> looseN;
    std::map<uint32_t, bool> looseOk;
    for (size_t i = 0; i < nDraw0; ++i) {
        const DrawModel& d = rt.drawList[i];
        uint32_t key = 0, part = 0;
        if (d.body != 0) {
            if (!human(rt, rt.mem.at<int16_t>(d.body + 0xCC))) continue;
            key = d.body, part = d.part;
        } else if (d.task != 0) {
            auto lo = looseOk.find(d.task);
            if (lo == looseOk.end()) lo = looseOk.emplace(d.task, looseCreature(rt, d.task)).first;
            if (!lo->second) continue;
            key = d.task;
            part = d.shadow ? 0 : 0x80000000u | looseN[d.task]++;
        } else {
            continue;
        }
        who[i] = {key, part};
        if (part == 0 || d.shadow) continue;
        Pose& p = poses[key];
        p.loose = d.body == 0;
        if (!p.at.count(part)) {
            const Mat w = mul(inv, fromCopro(d.m));
            if (finite(w)) p.at.emplace(part, w), p.order.push_back(part);
        }
    }
    std::map<uint32_t, std::map<uint32_t, Mat>> anim;
    for (auto& [k, p] : poses) anim[k] = p.at;
    // 2. the arcade's hold on each ragdoll; new deaths
    for (Rag& r : rags_)
        r.arcade = r.arcade && rt.mem.u32(r.body + 8) == uint32_t(r.serial) && dying(rt, r.body);
    const size_t known0 = rags_.size();
    if (advance)
        for (const auto& [body, pose] : poses) {
            if (!dying(rt, body)) continue;
            const int32_t serial = int32_t(rt.mem.u32(body + 8));
            const bool known = std::any_of(rags_.begin(), rags_.end(), [&](const Rag& r) { return r.body == body && r.serial == serial; });
            if (!known) create(rt, body, pose);
        }
    // 3. shots: each shot flash's first frame (its counter +0x50 is 1 after its first draw) is a ray from the eye
    if (advance) {
        std::set<std::pair<uint32_t, int32_t>> now;
        std::vector<Shot> fresh;
        for (const DrawModel& d : rt.drawList) {
            if (d.task == 0 || rt.mem.u32(d.task) != A::F::shot_effect_flash) continue;
            const std::pair<uint32_t, int32_t> key{d.task, int32_t(rt.mem.u32(d.task + 8))};
            now.insert(key);
            if (flashes_.count(key) || rt.mem.u32(d.task + 0x50) > 2) continue;
            const V3 c{rt.mem.f32(d.task + 0x38), rt.mem.f32(d.task + 0x3C), rt.mem.f32(d.task + 0x40)};
            if (!(len(c) > 1e-6f)) continue;
            Shot sh{rt.frameCount, inv.pos(), {}};
            for (int i = 0; i < 3; ++i)
                sh.dir[i] = inv.r[i * 3] * c[0] + inv.r[i * 3 + 1] * c[1] + inv.r[i * 3 + 2] * c[2];
            sh.dir = norm(sh.dir);
            fresh.push_back(sh);
        }
        flashes_ = std::move(now);
        // the nearest bone (a part's pivot to its parent's, as a capsule) a ray crosses: the part, -1 none
        auto hit = [](const Rag& r, const Shot& sh, float& bestT) {
            int best = -1;
            const float rad = r.radius * 2.2f;
            for (size_t i = 0; i < r.parts.size(); ++i) {
                const V3 a = r.parts[i].frame.pos();
                const V3 b = r.parts[i].parent >= 0 ? r.parts[size_t(r.parts[i].parent)].frame.pos() : a;
                // closest points of the ray (origin + t dir) and the segment a + u (b - a)
                const V3 e = b - a, w0 = sh.origin - a;
                const float bb = dot(sh.dir, e), cc = dot(e, e), dd = dot(sh.dir, w0), ee = dot(e, w0);
                const float den = cc - bb * bb;
                const float u = cc > 1e-9f && den > 1e-9f ? std::clamp((ee - bb * dd) / den, 0.0f, 1.0f) : 0.0f;
                const float t = dot(a + e * u - sh.origin, sh.dir);
                if (t < 0) continue;
                if (len(sh.origin + sh.dir * t - (a + e * u)) < rad && t < bestT) bestT = t, best = int(i);
            }
            return best;
        };
        auto apply = [&](Rag& r, int part, const Shot& sh) {
            push(r, part, sh.dir, s.push);
            if (trace() && rt.log)
                rt.log("ragdoll " + std::to_string(r.id) + ": shot at part " + std::to_string(part) + ", frame " +
                       std::to_string(rt.frameCount) + (sh.frame != rt.frameCount ? " (the killing shot)" : ""));
        };
        if (s.push > 0) {
            // a body that died this frame: the shots of the last frames that hit it (the killing shot's flash is
            // drawn on the frame of the hit, the death routine starts a frame or two later)
            for (size_t k = known0; k < rags_.size(); ++k)
                for (const Shot& sh : recentShots_) {
                    float t = 1e30f;
                    if (const int p = hit(rags_[k], sh, t); p >= 0) apply(rags_[k], p, sh);
                }
            for (const Shot& sh : fresh) {
                Rag* best = nullptr;
                int bestPart = -1;
                float bestT = 1e30f;
                for (Rag& r : rags_)
                    if (const int p = hit(r, sh, bestT); p >= 0) best = &r, bestPart = p;
                if (best) apply(*best, bestPart, sh);
                else if (trace() && rt.log && !rags_.empty())
                    rt.log("ragdoll: shot at frame " + std::to_string(rt.frameCount) + " missed the bodies");
            }
        }
        recentShots_.erase(std::remove_if(recentShots_.begin(), recentShots_.end(),
                                          [&](const Shot& sh) { return sh.frame + 4 < rt.frameCount; }),
                           recentShots_.end());
        recentShots_.insert(recentShots_.end(), fresh.begin(), fresh.end());
    }
    // 4. the steps and the parts' frames (leftovers: against the scene's walls, when the view has them)
    static const bool noWalls = std::getenv("HOTD_RAGDOLL_NO_WALLS") != nullptr;   // (testing: as before, through them)
    walls_ = rt.ragdollWorld && !noWalls ? rt.ragdollWorld() : nullptr;
    if (advance)
        collideBodies();   // (ipolish: before the steps: their constraints and floors settle the pushes)
    for (Rag& r : rags_) {
        if (advance) {
            // ragall: a tracked floor goes down with the arcade's own fall (its lowest drawn part)
            if (r.track && r.arcade)
                if (auto a = anim.find(r.body); a != anim.end()) {
                    float low = 1e30f;
                    for (const auto& [p, m] : a->second) low = std::min(low, m.t[1]);
                    if (std::isfinite(low) && low - r.radius < r.floor - 0.01f) {
                        r.floor = low - r.radius;
                        r.sleeping = false, r.still = 0;
                    }
                }
            // (the arcade gone while its body was still falling, as a ceiling Ebitan: on down to the room's floor)
            if (r.track && !r.arcade && r.gone == 0) {
                const float fy = rt.mem.f32(A::G::floor_y);
                if (std::isfinite(fy) && fy < r.floor - 0.01f && r.floor - fy < 150.0f) {
                    r.floor = fy;
                    r.sleeping = false, r.still = 0;
                }
            }
            step(r);
            ++r.age;
            if (!r.arcade) ++r.gone;
            if (trace() && rt.log && (r.age % 20 == 0 || r.age < 16) && !r.parts.empty()) {
                const V3 p = r.parts[0].frame.pos();
                float fast = 0, top = -1e30f;
                for (size_t i = 0; i < r.x.size(); ++i) fast = std::max(fast, len(r.x[i] - r.xp[i])), top = std::max(top, r.x[i][1]);
                char buf[240];
                std::snprintf(buf, sizeof buf, "ragdoll %d: age %d at %.1f %.1f %.1f (floor %.1f, top %.1f, fastest %.2f)%s%s, %zu models kept, walls %d, bodies %d",
                              r.id, r.age, p[0], p[1], p[2], r.floor, top, fast, r.arcade ? " arcade" : "", r.sleeping ? " asleep" : "",
                              r.cache.size(), r.wallHits, r.bodyHits);
                rt.log(buf);
                if (std::getenv("HOTD_RAGDOLL_PARTS") && (r.age == 1 || r.age % 40 == 0))
                    for (size_t i = 0; i < r.parts.size(); ++i) {
                        const V3 q = r.x[size_t(r.parts[i].p)];
                        std::snprintf(buf, sizeof buf, "  part %zu (parent %d, model %X): %.1f %.1f %.1f", i, r.parts[i].parent,
                                      rt.mem.u32(r.parts[i].addr), q[0], q[1], q[2]);
                        rt.log(buf);
                    }
            }
        }
        rebuild(r);
    }
    // the ones whose time is over
    rags_.erase(std::remove_if(rags_.begin(), rags_.end(),
                               [&](const Rag& r) { return !r.arcade && r.gone >= s.stayFrames; }),
                rags_.end());
    // 5. the draws: the arcade's models of a ragdoll's body posed by the ragdoll (and kept), else the kept ones
    const size_t nDraw = rt.drawList.size(), nSkin = rt.skinDraws.size();
    for (Rag& r : rags_) {
        const bool drawn = r.arcade && anim.count(r.body);
        if (drawn) {
            const auto& now = anim.at(r.body);
            r.cache.clear();
            for (size_t i = 0; i < nDraw; ++i) {
                DrawModel& d = rt.drawList[i];
                if (who[i].first != r.body) continue;   // (ragall: its body, or its task for a loose creature)
                const Mat w = mul(inv, fromCopro(d.m));
                int pi = who[i].second != 0 ? r.partOf(who[i].second) : -1;
                if (pi < 0) pi = r.nearest(w.pos());
                auto pa = now.find(r.parts[size_t(pi)].addr);
                if (pa == now.end()) continue;   // (its part not drawn this frame: as the arcade drew it)
                Cached c{d, pi, mul(inverse(pa->second), w)};
                toCopro(mul(cam, mul(r.parts[size_t(pi)].frame, c.local)), d.m);
                r.cache.push_back(c);
            }
            r.skins.clear();
            for (size_t i = 0; i < nSkin; ++i) {
                Runtime::SkinDraw& sd = rt.skinDraws[i];
                if (sd.body != r.body) continue;
                const Mat wa = mul(inv, fromCopro(sd.a)), wb = mul(inv, fromCopro(sd.b));
                CachedSkin c{sd, r.nearest(wa.pos()), r.nearest(wb.pos()), {}, {}};
                auto fa = now.find(r.parts[size_t(c.pa)].addr), fb = now.find(r.parts[size_t(c.pb)].addr);
                if (fa == now.end() || fb == now.end()) continue;
                c.la = mul(inverse(fa->second), wa);
                c.lb = mul(inverse(fb->second), wb);
                toCopro(mul(cam, mul(r.parts[size_t(c.pa)].frame, c.la)), sd.a);
                toCopro(mul(cam, mul(r.parts[size_t(c.pb)].frame, c.lb)), sd.b);
                r.skins.push_back(c);
            }
            continue;
        }
        // not drawn by the arcade this frame: the kept models (no blinking), fading out at the end
        const bool reused = rt.mem.u32(r.body + 8) != uint32_t(r.serial);   // its task now another object's
        int fade = 255;
        if (!r.arcade && s.stayFrames > 0)
            fade = std::clamp((s.stayFrames - r.gone) * 255 / kFadeFrames, 0, 255);
        for (const Cached& c : r.cache) {
            DrawModel d = c.d;
            toCopro(mul(cam, mul(r.parts[size_t(c.part)].frame, c.local)), d.m);
            d.fade = uint8_t(std::min<int>(d.fade, fade));
            if (reused) d.task = 0xF0000000u | uint32_t(r.id), d.body = d.body ? 0xF0000000u | uint32_t(r.id) : 0u;
            if (d.shadow) {   // squashed straight down onto the ragdoll's floor (the view's plain shadow)
                d.shadowFloor = r.floor;
                std::memset(d.shadowM, 0, sizeof d.shadowM);
            }
            rt.drawList.push_back(d);
        }
        for (const CachedSkin& c : r.skins) {
            Runtime::SkinDraw sd = c.s;
            toCopro(mul(cam, mul(r.parts[size_t(c.pa)].frame, c.la)), sd.a);
            toCopro(mul(cam, mul(r.parts[size_t(c.pb)].frame, c.lb)), sd.b);
            if (reused) sd.body = 0xF0000000u | uint32_t(r.id);
            rt.skinDraws.push_back(sd);
        }
    }
    // 6. the poses kept for the next frame's start velocities
    if (advance)
        prevAnim_ = std::move(anim);
}

bool Runtime::ragdoll_aim(int k, float& ax, float& ay) const
{
    return ragdolls && ragdollSettings().on && ragdolls->aim(*this, k, ax, ay);
}

void Runtime::ragdoll_frame()
{
    if (!ragdollSettings().on) {
        if (ragdolls) ragdolls->clear();
        return;
    }
    if (!ragdolls)
        ragdolls = std::make_shared<Ragdolls>();
    ragdolls->frame(*this);
}

} // namespace hotd::game
