// pmfix2: the PC Mode extras' portraits (pm_portraits.h)
#include "game/pm_portraits.h"

#include "assets/sprites_pc.h"
#include "game/frontend.h"
#include "game/gallery.h"
#include "port/game.h"
#include "util/png.h"
#include "util/png_read.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace hotd::game {

namespace {
using Chars = port::Game<port::A>;

// the Gallery entry a type is drawn as (its chapter stage): its own, else the nearest kind's (Researcher (5) / (6) and
// Dr. Curien (Dead) have no Gallery entry of their own)
std::vector<int> entriesOf(int type)
{
    const auto& es = galleryEntries();
    auto find = [&](const std::string& key) {
        for (size_t i = 0; i < es.size(); ++i)
            if (key == es[i].key) return int(i);
        return -1;
    };
    std::vector<int> v;
    for (const std::string& k : {"t" + std::to_string(type), std::string("t21"), std::string("t25"), std::string("t60"), std::string("t67")})
        if (const int i = find(k); i >= 0 && std::find(v.begin(), v.end(), i) == v.end())
            v.push_back(i);
    return v;
}

// the picture has the head: something drawn in its top half's middle
bool hasHead(const std::vector<uint8_t>& m, int w, int h)
{
    int n = 0;
    for (int y = 0; y < h / 2; ++y)
        for (int x = w / 4; x < 3 * w / 4; ++x)
            n += m[(size_t(y) * w + size_t(x)) * 4 + 3] > 128;
    return n > w * h / 60;
}

// portraits2 (user 2026-10-28: "recreate the additional researchers to match the 1998 PC: same pose, camera, etc,
// just different coloured backgrounds"): the camera aimed at the neck (Gallery::PortraitHead), fitted to the PC's
// Researcher (SPR.RBH 0xF2, drawn from type 0x1A, our "Researcher (woman, 3)") by overlay: her eyes on y 48 of the
// 160 x 160, chin ~86, the face turned 25 degrees to the picture's left, the shoulders dropped, square on; the
// small picture the same render's head (the PC's small ones are its big ones' heads, 4 to 1).
// HOTD_PM_PORTRAIT_HEAD="fov,dist,panx,pany,yaw,pitch,headyaw,headpitch,headroll,frame,lightdeg,lightup,armdrop,armdropx" (testing)
struct Cam {
    float fov, dist, panX, panY, yaw, pitch, headYaw, headPitch, headRoll;
    int frame;
    float lightDeg, lightUp, armDrop, armDropX;
    int cropX, cropY;   // (the small picture: the big render's 96 x 96 square round the head from here, 4 to 1 as the PC's)
};
Cam camFor(int type)
{
    Cam c{30.0f, 2.73f, -0.065f, -0.114f, 0.0f, 0.0f, 25.0f, 0.0f, 0.0f, 1, 300.0f, 0.8f, -18.0f, 0.0f, 30, 2};
    if (const char* e = std::getenv("HOTD_PM_PORTRAIT_HEAD"))
        std::sscanf(e, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%d,%f,%f,%f,%f", &c.fov, &c.dist, &c.panX, &c.panY, &c.yaw, &c.pitch, &c.headYaw,
                    &c.headPitch, &c.headRoll, &c.frame, &c.lightDeg, &c.lightUp, &c.armDrop, &c.armDropX);
    // per body: the men's heads are bigger than the women's (a little further away, the eyes kept on the PC's line);
    // Dr. Curien and the zombie Sophie (user: "her face is cut off, so move the camera so her whole head and a little
    // bit of her shoulders show"): further away, their own motions' head turn kept.
    // HOTD_PM_PORTRAIT_ADJ="type,distmul,dpanx,dpany,headyaw,cropx,cropy;..." (testing; 0 = the men's row)
    struct Adj { int type; float dist, panX, panY, headYaw; int cropX, cropY; };
    static std::vector<Adj> adj = [] {
        std::vector<Adj> a = {{0, 1.2f, 0.0f, -0.05f, 25.0f, 30, 2}, {60, 1.15f, 0.0f, 0.0f, 0.0f, 32, 12}, {78, 1.15f, 0.0f, 0.0f, 0.0f, 32, 12},
                              {67, 1.2f, 0.0f, 0.03f, 0.0f, 40, 14}};   // (portraits3: closer, the PC's framing)
        if (const char* e = std::getenv("HOTD_PM_PORTRAIT_ADJ"))
            for (std::string s = e; !s.empty();) {
                const size_t q = s.find(';');
                Adj x{};
                if (std::sscanf(s.substr(0, q).c_str(), "%d,%f,%f,%f,%f,%d,%d", &x.type, &x.dist, &x.panX, &x.panY, &x.headYaw, &x.cropX, &x.cropY) == 7) {
                    for (Adj& y : a)
                        if (y.type == x.type) y = x, x.type = -1;
                    if (x.type >= 0) a.push_back(x);
                }
                s = q == std::string::npos ? std::string() : s.substr(q + 1);
            }
        return a;
    }();
    const bool man = type == 17 || type == 19 || type == 20 || type == 21 || type == 22 || type == 49;
    for (const Adj& a : adj)
        if (a.type == (man ? 0 : type)) c.dist *= a.dist, c.panX += a.panX, c.panY += a.panY, c.headYaw = a.headYaw,
            c.cropX = a.cropX, c.cropY = a.cropY;
    return c;
}

// the grounds, per extra (7..19): the big one's dark tint (the PC's own are near-black, each its own) and the small
// one's brighter flat colour (as the PC's small pictures: blue, amber, brown...). Dr. Curien grey, his dead self
// maroon (user 2026-10-28)
struct Ground { uint8_t big[3], small[3]; };
constexpr Ground kGrounds[13] = {
    {{10, 18, 44}, {24, 56, 150}},    // Researcher (1): navy
    {{44, 26, 6}, {160, 97, 0}},      // (2): brown / amber
    {{8, 34, 22}, {20, 110, 60}},     // (3): green
    {{28, 14, 48}, {96, 48, 150}},    // (4): violet
    {{6, 32, 38}, {0, 110, 120}},     // (5): teal
    {{40, 34, 6}, {130, 115, 10}},    // (6): olive
    {{52, 14, 34}, {150, 40, 90}},    // (woman): rose
    {{16, 16, 48}, {60, 60, 170}},    // (woman, 2): indigo
    {{12, 30, 44}, {40, 96, 150}},    // (woman, 3): steel blue
    {{44, 20, 8}, {150, 60, 20}},     // (woman, 4): rust
    {{52, 52, 56}, {112, 112, 118}},  // Dr. Curien: grey
    {{64, 6, 12}, {124, 16, 26}},     // Dr. Curien (Dead): maroon
    {{20, 36, 16}, {64, 100, 40}},    // Sophie (zombie): sick green
};

uint8_t clamp8(float v) { return uint8_t(std::clamp(v, 0.0f, 255.0f)); }

// the PC Researcher's grading (measured: her coat a flat 231 grey-white, her face 223 / 190 / 166 at its lightest):
// less saturation, the whites held under 232, neutral
void grade(uint8_t* p)
{
    float c[3] = {float(p[0]), float(p[1]), float(p[2])};
    const float l = 0.3f * c[0] + 0.59f * c[1] + 0.11f * c[2];
    for (int k = 0; k < 3; ++k) {
        float v = l + (c[k] - l) * 0.75f;
        v = v * 0.9f + 2.0f;
        c[k] = v;
    }
    c[2] *= 1.02f;
    for (int k = 0; k < 3; ++k) p[k] = clamp8(c[k]);
}

// the PC Researcher's soft edge (measured): the body fades out over the bottom 16 rows and the sides' 14 columns
float vignette(int x, int y, int w, int h)
{
    auto s = [](float d, float wd) { return std::clamp(d / wd, 0.0f, 1.0f); };
    return s(float(h - y) - 0.5f, 16.0f) * s(float(x) + 0.5f, 14.0f) * s(float(w - x) - 0.5f, 14.0f) * s(float(y) + 0.5f, 6.0f);
}

// the model over the big portrait's flat ground, graded, the edges faded into the ground
std::vector<uint8_t> composeBig(const std::vector<uint8_t>& m, const uint8_t ground[3])
{
    const int w = kPmPortraitW, h = kPmPortraitH;
    std::vector<uint8_t> out(size_t(w) * h * 4);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            const uint8_t* s = &m[(size_t(y) * w + size_t(x)) * 4];
            uint8_t px[4] = {s[0], s[1], s[2], 255};
            grade(px);
            const float a = s[3] / 255.0f * vignette(x, y, w, h);
            uint8_t* o = &out[(size_t(y) * w + size_t(x)) * 4];
            for (int k = 0; k < 3; ++k) o[k] = clamp8(px[k] * a + ground[k] * (1 - a));
            o[3] = 255;
        }
    return out;
}

// the small picture: the big render's head square, averaged down, on the flat ground
std::vector<uint8_t> composeSmall(const std::vector<uint8_t>& m, const Ground& g, int cropX, int cropY)
{
    const int w = kPmSmallW, h = kPmSmallH, n = 4;
    std::vector<uint8_t> out(size_t(w) * h * 4);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            float sum[4] = {};
            for (int j = 0; j < n; ++j)
                for (int i = 0; i < n; ++i) {
                    const int sx = cropX + x * n + i, sy = cropY + y * n + j;
                    if (sx < 0 || sy < 0 || sx >= kPmPortraitW || sy >= kPmPortraitH) continue;
                    const uint8_t* s = &m[(size_t(sy) * kPmPortraitW + size_t(sx)) * 4];
                    const float a = s[3] / 255.0f;
                    for (int c = 0; c < 3; ++c) sum[c] += s[c] * a;
                    sum[3] += a;
                }
            uint8_t px[4] = {0, 0, 0, 255};
            const float a = sum[3] / float(n * n);
            if (sum[3] > 0)
                for (int c = 0; c < 3; ++c) px[c] = clamp8(sum[c] / sum[3]);
            grade(px);
            uint8_t* o = &out[(size_t(y) * w + size_t(x)) * 4];
            for (int c = 0; c < 3; ++c) o[c] = clamp8(px[c] * a + g.small[c] * (1 - a));
            o[3] = 255;
        }
    return out;
}

std::string bigPath(const std::string& dir, int ch) { return dir + "/ch" + std::to_string(ch) + "_big.png"; }
std::string smallPath(const std::string& dir, int ch) { return dir + "/ch" + std::to_string(ch) + "_small.png"; }
} // namespace

bool pmPortraitMake(Gallery& g, const assets::PcSprites& spr, int ch, bool pcSet, std::vector<uint8_t>& big, std::vector<uint8_t>& small)
{
    const auto* x = Chars::pm_extra(ch);
    if (!x || !g.isOpen()) return false;
    // the PC's look where the PC files have the model (the PC's portraits are its models), else the arcade's; drawn
    // as its own Gallery entry, else as one in another chapter's stage (the stage that has its head: Researcher (5) /
    // (6), Dr. Curien (Dead) have no entry). The motion: a standing one (the researchers' own begin crouched: the
    // players', the same skeleton; Curien and the zombie Sophie their own first one)
    static const char* envMo = std::getenv("HOTD_PM_PORTRAIT_MOTIONS");
    const bool researcher = x->type != 60 && x->type != 78 && x->type != 67;
    const char* mo = researcher ? (envMo ? envMo : "MO_p_l_tomari") : x->type == 67 ? "MO_s_zonbi" : "MO_kyu";
    const Cam c = camFor(x->type);
    const Ground& gr = kGrounds[ch - Chars::kPmPcChars];
    // pmpick (user 2026-10-30: "in PC Mode all characters have the same 1998 PC black square as their background"):
    // the big picture on the PC's own portraits' ground (the PC Researcher's corner, 0xF2), the coloured grounds only
    // for the small High Scores pictures
    uint8_t ground[3] = {0, 0, 0};
    if (const int pid = Chars::pm_table(6).portrait, pw = spr.width(pid), ph = spr.height(pid); pw > 4 && ph > 4) {
        const std::vector<uint8_t> im = spr.image(pid);
        if (im.size() >= size_t(pw) * size_t(ph) * 4 && im[(size_t(2) * size_t(pw) + 2) * 4 + 3])
            for (int k = 0; k < 3; ++k) ground[k] = im[(size_t(2) * size_t(pw) + 2) * 4 + size_t(k)];
    }
    g.portraitHead.on = true;
    g.portraitHead.fovDeg = c.fov, g.portraitHead.headYaw = c.headYaw, g.portraitHead.headPitch = c.headPitch;
    g.portraitHead.headRoll = c.headRoll, g.portraitHead.frame = c.frame;
    g.portraitHead.lightDeg = c.lightDeg, g.portraitHead.lightUp = c.lightUp;
    g.portraitHead.armDrop = c.armDrop, g.portraitHead.armDropX = c.armDropX;
    std::vector<uint8_t> m;
    std::vector<Source> sources;
    // pmpick (user 2026-10-30: "custom characters should have their arcade models used for arcade models and
    // textures"): Video > Models Arcade: the arcade's model first; PC: the PC's (where the PC files have it)
    if (pcSet && x->pcModel && g.hasSource(Source::PC)) sources.push_back(Source::PC);
    if (g.hasSource(Source::Arcade)) sources.push_back(Source::Arcade);
    if (!pcSet && x->pcModel && g.hasSource(Source::PC)) sources.push_back(Source::PC);
    bool ok = false;
    for (Source s : sources) {
        for (int entry : entriesOf(x->type)) {
            g.setSource(s);
            if (!g.makePortrait(x->type, entry, mo, kPmPortraitW, kPmPortraitH, c.yaw, c.pitch, c.dist, c.panX, c.panY, m) ||
                !hasHead(m, kPmPortraitW, kPmPortraitH))
                continue;
            big = composeBig(m, ground);
            small = composeSmall(m, gr, c.cropX, c.cropY);
            ok = true;
            break;
        }
        if (ok) break;
    }
    g.portraitHead = {};
    return ok;
}

bool pmPortraitSave(const std::string& dir, int ch, const std::vector<uint8_t>& big, const std::vector<uint8_t>& small)
{
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    const bool ok = writePngRgba(bigPath(dir, ch), kPmPortraitW, kPmPortraitH, big) &&
                    writePngRgba(smallPath(dir, ch), kPmSmallW, kPmSmallH, small);
    if (ok && ch == Chars::kPmCharCount - 1)
        std::ofstream(dir + "/version.txt") << kPmPortraitVersion << "\n";
    return ok;
}

bool pmPortraitsLoad(Frontend& fe, const std::string& dir)
{
    std::ifstream v(dir + "/version.txt");
    std::string line;
    if (!std::getline(v, line) || line != kPmPortraitVersion) return false;
    for (int ch = Chars::kPmPcChars; ch < Chars::kPmCharCount; ++ch) {
        int w = 0, h = 0, sw = 0, sh = 0;
        std::vector<uint8_t> big, small;
        std::string err;
        if (!readPngRgba(bigPath(dir, ch), w, h, big, err) || w != kPmPortraitW || h != kPmPortraitH ||
            !readPngRgba(smallPath(dir, ch), sw, sh, small, err) || sw != kPmSmallW || sh != kPmSmallH)
            return false;
        fe.setPcPortrait(ch, false, w, h, std::move(big));
        fe.setPcPortrait(ch, true, sw, sh, std::move(small));
    }
    return true;
}

bool pmPortraitSheet(const Frontend& fe, const std::string& path)
{
    const assets::PcSprites& spr = fe.sprites();
    const int cell = kPmPortraitW + 8, cols = 7, rows = 3, W = cols * cell, H = rows * (cell + 80);
    std::vector<uint8_t> c(size_t(W) * H * 3, 24);
    auto put = [&](int id, int x0, int y0, int scale) {
        const int w = spr.width(id), h = spr.height(id);
        const std::vector<uint8_t> im = spr.image(id);
        if (w <= 0 || h <= 0 || im.size() < size_t(w) * h * 4) return;
        for (int y = 0; y < h * scale; ++y)
            for (int x = 0; x < w * scale; ++x) {
                const int px = x0 + x, py = y0 + y;
                if (px < 0 || px >= W || py < 0 || py >= H) continue;
                const uint8_t* s = &im[(size_t(y / scale) * w + size_t(x / scale)) * 4];
                uint8_t* d = &c[(size_t(py) * W + size_t(px)) * 3];
                if (s[3]) d[0] = s[0], d[1] = s[1], d[2] = s[2];
            }
    };
    // row 1: the PC's six (Rogan, G, Rogan 2, G 2, Sophie, Researcher), each with its small picture (x2) under it
    for (int k = 0; k < 6; ++k) {
        const auto& t = Chars::pm_table(k + 1);
        put(t.portrait, k * cell + 4, 4, 1);
        put(t.sprite2, k * cell + 4, kPmPortraitH + 8, 3);
    }
    // rows 2 / 3: the extras' (7..19), 7 to a row
    for (int ch = Chars::kPmPcChars; ch < Chars::kPmCharCount; ++ch) {
        const int i = ch - Chars::kPmPcChars, x0 = (i % cols) * cell + 4, y0 = (1 + i / cols) * (cell + 80) + 4;
        put(Frontend::kPmPortraitBig + ch, x0, y0, 1);
        put(Frontend::kPmPortraitSmall + ch, x0, y0 + kPmPortraitH + 4, 3);
    }
    return writePng(path, W, H, c);
}

} // namespace hotd::game
