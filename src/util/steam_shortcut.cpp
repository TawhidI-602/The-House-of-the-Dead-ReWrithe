#include "util/steam_shortcut.h"

#include "assets/archive.h"
#include "assets/pc_files.h"
#include "assets/sprites_pc.h"
#include "util/pe_icon.h"
#include "util/png.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#else
#include <cstdlib>
#ifdef __APPLE__
#include <libproc.h>
#endif
#endif

namespace hotd::steam {

namespace {

namespace fs = std::filesystem;

const char* const kName = "The House of the Dead: ReWrithe";
const char* const kOldNames[] = {"HOTD ReWrithe"};   // earlier names: updated in place, never duplicated

bool ourName(const std::string& n)
{
    if (n == kName)
        return true;
    for (const char* o : kOldNames)
        if (n == o)
            return true;
    return false;
}

bool sameText(const std::string& a, const std::string& b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    return true;
}

// ---- binary KeyValues: 0 map, 1 string, 2 int32, 7 uint64, 8 end of map ----
struct KV {
    uint8_t type = 0;
    std::string key, s;
    uint32_t i = 0;
    uint64_t q = 0;
    std::vector<KV> kids;
};

bool readMap(const std::vector<uint8_t>& b, size_t& p, std::vector<KV>& out)
{
    auto str = [&](std::string& s) {
        size_t e = p;
        while (e < b.size() && b[e]) ++e;
        if (e >= b.size()) return false;
        s.assign(reinterpret_cast<const char*>(b.data() + p), e - p);
        p = e + 1;
        return true;
    };
    while (p < b.size()) {
        KV kv;
        kv.type = b[p++];
        if (kv.type == 8) return true;
        if (!str(kv.key)) return false;
        if (kv.type == 0) {
            if (!readMap(b, p, kv.kids)) return false;
        } else if (kv.type == 1) {
            if (!str(kv.s)) return false;
        } else if (kv.type == 2) {
            if (p + 4 > b.size()) return false;
            std::memcpy(&kv.i, &b[p], 4), p += 4;
        } else if (kv.type == 7) {
            if (p + 8 > b.size()) return false;
            std::memcpy(&kv.q, &b[p], 8), p += 8;
        } else {
            return false;   // a type Steam's shortcuts never use: leave the file alone
        }
        out.push_back(std::move(kv));
    }
    return false;
}

void writeMap(std::vector<uint8_t>& o, const std::vector<KV>& m)
{
    for (const KV& kv : m) {
        o.push_back(kv.type);
        o.insert(o.end(), kv.key.begin(), kv.key.end());
        o.push_back(0);
        if (kv.type == 0) {
            writeMap(o, kv.kids);
        } else if (kv.type == 1) {
            o.insert(o.end(), kv.s.begin(), kv.s.end());
            o.push_back(0);
        } else if (kv.type == 2) {
            const uint8_t* x = reinterpret_cast<const uint8_t*>(&kv.i);
            o.insert(o.end(), x, x + 4);
        } else if (kv.type == 7) {
            const uint8_t* x = reinterpret_cast<const uint8_t*>(&kv.q);
            o.insert(o.end(), x, x + 8);
        }
    }
    o.push_back(8);
}

// the file: 00 "shortcuts" 00 <entries "0", "1", ...> 08 08
bool loadVdf(const fs::path& path, std::vector<KV>& entries, std::string& err)
{
    std::error_code ec;
    if (!fs::exists(path, ec))
        return true;
    std::ifstream f(path, std::ios::binary);
    const std::vector<uint8_t> b((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (b.empty())
        return true;
    size_t p = 1;
    while (p < b.size() && b[p]) ++p;
    ++p;
    if (b[0] != 0 || p > b.size() || !readMap(b, p, entries)) {
        err = "cannot read " + path.string() + " (left as it is)";
        return false;
    }
    return true;
}

bool saveVdf(const fs::path& path, const std::vector<KV>& entries)
{
    std::vector<uint8_t> o = {0};
    const std::string root = "shortcuts";
    o.insert(o.end(), root.begin(), root.end());
    o.push_back(0);
    writeMap(o, entries);
    o.push_back(8);
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f.write(reinterpret_cast<const char*>(o.data()), std::streamsize(o.size()));
    return bool(f);
}

KV& field(KV& e, const std::string& key, uint8_t type)
{
    for (KV& k : e.kids)
        if (sameText(k.key, key))
            return k;
    KV k;
    k.type = type;
    k.key = key;
    e.kids.push_back(k);
    return e.kids.back();
}

std::string text(const KV& e, const std::string& key)
{
    for (const KV& k : e.kids)
        if (sameText(k.key, key) && k.type == 1)
            return k.s;
    return {};
}

// Steam's id for a non-Steam shortcut: CRC-32 of the quoted exe + the name, top bit set (also the grid art's prefix)
uint32_t shortcutId(const std::string& quotedExe, const std::string& name)
{
    const std::string s = quotedExe + name;
    return crc32(reinterpret_cast<const uint8_t*>(s.data()), s.size()) | 0x80000000u;
}

void renumber(std::vector<KV>& entries)
{
    for (size_t n = 0; n < entries.size(); ++n)
        entries[n].key = std::to_string(n);
}

std::string stamp()
{
    const std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char b[32];
    std::strftime(b, sizeof b, "%Y%m%d-%H%M%S", &tm);
    return b;
}

// ---- the art: small RGBA image helpers ----
struct Img {
    int w = 0, h = 0;
    std::vector<uint8_t> px;   // RGBA
    uint8_t* at(int x, int y) { return &px[(size_t(y) * w + size_t(x)) * 4]; }
    const uint8_t* at(int x, int y) const { return &px[(size_t(y) * w + size_t(x)) * 4]; }
};

Img blank(int w, int h, uint8_t r, uint8_t g, uint8_t b)
{
    Img o{w, h, std::vector<uint8_t>(size_t(w) * h * 4)};
    for (size_t i = 0; i < o.px.size(); i += 4)
        o.px[i] = r, o.px[i + 1] = g, o.px[i + 2] = b, o.px[i + 3] = 255;
    return o;
}

Img onBlack(Img im)
{
    for (size_t i = 0; i < im.px.size(); i += 4) {
        const int a = im.px[i + 3];
        for (int c = 0; c < 3; ++c)
            im.px[i + c] = uint8_t(im.px[i + c] * a / 255);
        im.px[i + 3] = 255;
    }
    return im;
}

// separable resampling with a triangle filter as wide as the scale (smooth when shrinking, linear when growing)
Img resize(const Img& s, int w, int h)
{
    auto pass = [](const Img& in, int ow, int oh, bool horizontal) {
        Img out{ow, oh, std::vector<uint8_t>(size_t(ow) * oh * 4)};
        const int inLen = horizontal ? in.w : in.h, outLen = horizontal ? ow : oh;
        const double scale = double(inLen) / outLen, support = std::max(1.0, scale);
        for (int o = 0; o < outLen; ++o) {
            const double c = (o + 0.5) * scale - 0.5;
            const int a = std::max(0, int(std::floor(c - support))), b = std::min(inLen - 1, int(std::ceil(c + support)));
            double sumW = 0;
            std::vector<std::pair<int, double>> taps;
            for (int k = a; k <= b; ++k) {
                const double wgt = std::max(0.0, 1.0 - std::fabs(k - c) / support);
                if (wgt > 0) taps.push_back({k, wgt}), sumW += wgt;
            }
            if (taps.empty()) taps.push_back({std::clamp(int(c + 0.5), 0, inLen - 1), 1.0}), sumW = 1;
            const int lines = horizontal ? oh : ow;
            for (int l = 0; l < lines; ++l) {
                double acc[4] = {};
                for (const auto& [k, wgt] : taps) {
                    const uint8_t* p = horizontal ? in.at(k, l) : in.at(l, k);
                    for (int ch = 0; ch < 4; ++ch) acc[ch] += p[ch] * wgt;
                }
                uint8_t* d = horizontal ? out.at(o, l) : out.at(l, o);
                for (int ch = 0; ch < 4; ++ch) d[ch] = uint8_t(std::clamp(acc[ch] / sumW + 0.5, 0.0, 255.0));
            }
        }
        return out;
    };
    return pass(pass(s, w, s.h, true), w, h, false);
}

Img crop(const Img& s, int x0, int y0, int w, int h)
{
    Img o{w, h, std::vector<uint8_t>(size_t(w) * h * 4)};
    for (int y = 0; y < h; ++y)
        std::memcpy(o.at(0, y), s.at(x0, y0 + y), size_t(w) * 4);
    return o;
}

Img cover(const Img& s, int w, int h)   // fills w x h, the overflow cut evenly
{
    const double k = std::max(double(w) / s.w, double(h) / s.h);
    const int rw = std::max(w, int(std::lround(s.w * k))), rh = std::max(h, int(std::lround(s.h * k)));
    return crop(resize(s, rw, rh), (rw - w) / 2, (rh - h) / 2, w, h);
}

void paste(Img& d, const Img& s, int x0, int y0)
{
    for (int y = 0; y < s.h; ++y)
        for (int x = 0; x < s.w; ++x)
            if (x0 + x >= 0 && x0 + x < d.w && y0 + y >= 0 && y0 + y < d.h)
                std::memcpy(d.at(x0 + x, y0 + y), s.at(x, y), 4);
}

Img fit(const Img& s, int w, int h)   // the whole picture on a dark w x h
{
    const double k = std::min(double(w) / s.w, double(h) / s.h);
    const Img r = resize(s, std::max(1, int(std::lround(s.w * k))), std::max(1, int(std::lround(s.h * k))));
    Img o = blank(w, h, 8, 8, 10);
    paste(o, r, (w - r.w) / 2, (h - r.h) / 2);
    return o;
}

Img blurDark(Img im, int radius, double gain)   // three box blurs (about a Gaussian), then darker
{
    for (int pass = 0; pass < 3; ++pass)
        for (int horizontal = 1; horizontal >= 0; --horizontal) {
            Img out = im;
            const int len = horizontal ? im.w : im.h, lines = horizontal ? im.h : im.w;
            for (int l = 0; l < lines; ++l)
                for (int o = 0; o < len; ++o) {
                    int acc[3] = {}, n = 0;
                    for (int k = std::max(0, o - radius); k <= std::min(len - 1, o + radius); ++k, ++n) {
                        const uint8_t* p = horizontal ? im.at(k, l) : im.at(l, k);
                        acc[0] += p[0], acc[1] += p[1], acc[2] += p[2];
                    }
                    uint8_t* d = horizontal ? out.at(o, l) : out.at(l, o);
                    for (int c = 0; c < 3; ++c) d[c] = uint8_t(acc[c] / n);
                }
            im = std::move(out);
        }
    for (size_t i = 0; i < im.px.size(); i += 4)
        for (int c = 0; c < 3; ++c) im.px[i + c] = uint8_t(im.px[i + c] * gain);
    return im;
}

// the logo: stray pixels of the sprite's colour key (connected pieces under 150 pixels) dropped, then cut to its
// pixels with a 4-pixel margin; the lettering, the crow and Rogan are pieces of thousands
Img cleanLogo(Img im)
{
    std::vector<uint8_t> seen(size_t(im.w) * im.h, 0);
    std::vector<std::pair<int, int>> stack, piece;
    for (int y0 = 0; y0 < im.h; ++y0)
        for (int x0 = 0; x0 < im.w; ++x0) {
            if (seen[size_t(y0) * im.w + x0] || !im.at(x0, y0)[3]) continue;
            seen[size_t(y0) * im.w + x0] = 1;
            stack = {{x0, y0}};
            piece.clear();
            while (!stack.empty()) {
                const auto [x, y] = stack.back();
                stack.pop_back();
                piece.push_back({x, y});
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) {
                        const int nx = x + dx, ny = y + dy;
                        if (nx < 0 || ny < 0 || nx >= im.w || ny >= im.h) continue;
                        uint8_t& s = seen[size_t(ny) * im.w + nx];
                        if (!s && im.at(nx, ny)[3]) s = 1, stack.push_back({nx, ny});
                    }
            }
            if (piece.size() < 150)
                for (const auto& [x, y] : piece) im.at(x, y)[3] = 0;
        }
    int x0 = im.w, y0 = im.h, x1 = -1, y1 = -1;
    for (int y = 0; y < im.h; ++y)
        for (int x = 0; x < im.w; ++x)
            if (im.at(x, y)[3]) x0 = std::min(x0, x), y0 = std::min(y0, y), x1 = std::max(x1, x), y1 = std::max(y1, y);
    if (x1 < 0) return im;
    x0 = std::max(0, x0 - 4), y0 = std::max(0, y0 - 4), x1 = std::min(im.w - 1, x1 + 4), y1 = std::min(im.h - 1, y1 + 4);
    return crop(im, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
}

struct Art {
    bool ok = false;
    bool iconAlpha = false;   // the icon has transparency (THOTD.EXE's): written as RGBA
    std::vector<uint8_t> ico; // THOTD.EXE's own icon group as a .ico (the official icon, every size it has)
    Img cover, capsule, hero, logo, icon;
};

Art composeArt(const std::string& pcDir, std::vector<std::string>& lines)
{
    Art a;
    if (pcDir.empty()) {
        lines.push_back("no artwork: the PC files are not set");
        return a;
    }
    assets::PcSprites sp;
    std::string err;
    if (!sp.load(pcDir, err, 1, 0, "MENU")) {
        lines.push_back("no artwork: " + err);
        return a;
    }
    auto sprite = [](const assets::PcSprites& s, int id) { return Img{s.width(id), s.height(id), s.image(id)}; };
    // the title art as THOTD's splash colours it (TITLE.TBL / TITLE.PAL), the menu's pictures through MENU
    assets::PcSprites title_sp;
    std::string terr;
    const bool haveTitle = title_sp.load(pcDir, terr, 1, 0, "TITLE");
    const Img title = onBlack(sprite(haveTitle ? title_sp : sp, 0xAA)), hall = onBlack(sprite(sp, 0xD1)), logo = sprite(sp, 0xD4);
    if (title.px.empty() || hall.px.empty() || logo.px.empty()) {
        lines.push_back("no artwork: the PC files have no title / mansion / logo pictures");
        return a;
    }
    a.cover = blurDark(cover(title, 600, 900), 10, 0.45);   // the title art over a blurred, darker copy of itself
    paste(a.cover, resize(title, 600, int(std::lround(title.h * 600.0 / title.w))), 0,
          (900 - int(std::lround(title.h * 600.0 / title.w))) / 2);
    a.capsule = fit(title, 920, 430);
    // the banner (Steam's wide cover; user 2026-10-18: "the default, since it comes with everyone's PC copy"): the CD
    // launcher's picture in THOTDRUN.EXE (two clown-like zombies in the sewers under the logo, 225 x 160), fitted to
    // the height over a blurred, darker copy of itself filling the width; the title art if it is not there
    {
        std::vector<uint8_t> exe;
        peicon::Rgba bm;
        if (pcfs::readFile(pcDir + "/THOTDRUN.EXE", exe) && peicon::firstBitmap(exe, bm) && bm.w > 0 && bm.h > 0) {
            const Img run{bm.w, bm.h, bm.px};
            // at 78 % of the height, centred: Steam's "Recent Games" tile zooms and crops the wide cover (the logo's
            // top line was cut, user 2026-10-18); the margin of blurred copy around it is what gets cut instead
            a.capsule = blurDark(cover(run, 920, 430), 12, 0.40);
            const int h = int(std::lround(430 * 0.78)), w = int(std::lround(run.w * double(h) / run.h));
            paste(a.capsule, resize(run, w, h), (920 - w) / 2, (430 - h) / 2);
        }
    }
    // the hero (the library page's background): the mansion with no lettering (0xD1). Tried 2026-10-18: the title art
    // (zombie, "(c) Sega"), but Steam draws its logo over the hero, so the lettering showed twice; the user: "if it does
    // not look right, go back to the mansion"
    a.hero = cover(hall, 1920, 620);
    a.logo = cleanLogo(logo);
    // the icon (user 2026-10-18): THOTD.EXE's own icon, the red bite, enlarged with sharp pixels; else the title art
    a.icon = fit(title, 256, 256);
    {
        std::vector<uint8_t> exe;
        peicon::Rgba ic;
        const std::string p = pcDir + "/THOTD.EXE";
        if (pcfs::readFile(p, exe))
            peicon::extractIco(exe, a.ico);
        if (!exe.empty() && peicon::largestImage(exe, ic) && ic.w > 0 && ic.h > 0) {
            Img big{256, 256, std::vector<uint8_t>(size_t(256) * 256 * 4, 0)};
            for (int y = 0; y < 256; ++y)
                for (int x = 0; x < 256; ++x)
                    std::memcpy(big.at(x, y), &ic.px[(size_t(y * ic.h / 256) * ic.w + size_t(x * ic.w / 256)) * 4], 4);
            a.icon = big;
            a.iconAlpha = true;
        }
    }
    a.ok = true;
    return a;
}

bool writeRgb(const fs::path& p, const Img& im)
{
    std::vector<uint8_t> rgb(size_t(im.w) * im.h * 3);
    for (size_t i = 0, j = 0; i < im.px.size(); i += 4, j += 3)
        rgb[j] = im.px[i], rgb[j + 1] = im.px[i + 1], rgb[j + 2] = im.px[i + 2];
    return writePng(p.string(), im.w, im.h, rgb);
}

// Steam's grid names: <id>p cover (600 x 900), <id> banner (920 x 430), <id>_hero (1920 x 620), <id>_logo, <id>_icon
const std::pair<const char*, const char*> kArtKinds[] = {
    {"cover", "p"}, {"banner", ""}, {"hero", "_hero"}, {"logo", "_logo"}, {"icon", "_icon"}};

void removeArt(const fs::path& grid, uint32_t id, std::vector<std::string>& lines)
{
    std::error_code ec;
    for (const auto& [kind, suffix] : kArtKinds)
        for (const char* x : {".png", ".jpg", ".jpeg", ".ico"}) {
            const fs::path f = grid / (std::to_string(id) + suffix + x);
            if (fs::remove(f, ec)) lines.push_back("  removed " + f.string());
        }
}

std::string lowerExt(const fs::path& p)
{
    std::string x = p.extension().string();
    for (char& c : x) c = char(std::tolower(static_cast<unsigned char>(c)));
    return x;
}

// the player's own pictures (user 2026-10-18: "choose their own art or upload their own"): in `dir`, files named
// cover / banner / hero / logo / icon (any case) as .png / .jpg / .jpeg (the icon also .ico); each one found replaces
// the picture made from the PC files. Nothing of ours is shipped: they are the player's files
std::map<std::string, fs::path> ownPictures(const std::string& dir, std::vector<std::string>& lines)
{
    std::map<std::string, fs::path> out;
    std::error_code ec;
    if (dir.empty() || !fs::is_directory(dir, ec))
        return out;
    for (const auto& e : fs::directory_iterator(dir, ec)) {
        if (!e.is_regular_file(ec)) continue;
        std::string stem = e.path().stem().string();
        for (char& c : stem) c = char(std::tolower(static_cast<unsigned char>(c)));
        const std::string x = lowerExt(e.path());
        const bool image = x == ".png" || x == ".jpg" || x == ".jpeg" || (stem == "icon" && x == ".ico");
        for (const auto& [kind, suffix] : kArtKinds)
            if (image && stem == kind && !out.count(kind)) out[kind] = e.path();
    }
    if (!out.empty())
        lines.push_back("your own pictures from " + dir + ": " + std::to_string(out.size()));
    return out;
}

#ifdef _WIN32
std::string narrow(const std::wstring& w)
{
    if (w.empty()) return {};
    std::string s(size_t(WideCharToMultiByte(CP_UTF8, 0, w.data(), int(w.size()), nullptr, 0, nullptr, nullptr)), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), int(w.size()), s.data(), int(s.size()), nullptr, nullptr);
    return s;
}
#endif

std::string steamFromRegistry()
{
#ifdef _WIN32
    wchar_t buf[MAX_PATH * 2];
    DWORD size = sizeof buf;
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath", RRF_RT_REG_SZ, nullptr, buf, &size) != ERROR_SUCCESS)
        return {};
    std::string s = narrow(buf);
    std::replace(s.begin(), s.end(), '/', '\\');
    return s;
#else
    // Linux (also the Steam Deck / SteamOS): ~/.steam/steam (the link Steam keeps to its folder), the native folder,
    // the Flatpak and the Snap; macOS: ~/Library/Application Support/Steam. The first with a userdata folder.
    const char* h = std::getenv("HOME");
    if (!h || !*h)
        return {};
    const fs::path home = h;
    const fs::path candidates[] = {
#ifdef __APPLE__
        home / "Library" / "Application Support" / "Steam",
#else
        home / ".steam" / "steam", home / ".local" / "share" / "Steam",
        home / ".var" / "app" / "com.valvesoftware.Steam" / ".local" / "share" / "Steam",
        home / "snap" / "steam" / "common" / ".local" / "share" / "Steam",
#endif
    };
    std::error_code ec;
    for (const fs::path& c : candidates)
        if (fs::is_directory(c / "userdata", ec)) {
            const fs::path real = fs::canonical(c, ec);
            return ec ? c.string() : real.string();
        }
    return {};
#endif
}

struct Target {
    fs::path vdf, grid;
};

// the targets: every signed-in account's config folder (or the test file); false + a message when Steam can't be used
bool targets(const Options& o, std::vector<Target>& out, Result& r)
{
    if (!o.testVdf.empty()) {
        out.push_back({o.testVdf, o.gridOut.empty() ? fs::path(o.testVdf).parent_path() / "grid" : fs::path(o.gridOut)});
        return true;
    }
    const std::string steamDir = o.steamDir.empty() ? steamFromRegistry() : o.steamDir;
    std::error_code ec;
    if (steamDir.empty() || !fs::is_directory(steamDir, ec)) {
#ifdef _WIN32
        r.lines.push_back("Steam was not found (no SteamPath in the registry). Install Steam and sign in once.");
#else
        r.lines.push_back("Steam was not found (no ~/.steam/steam or ~/.local/share/Steam with a userdata folder). "
                          "Install Steam and sign in once.");
#endif
        r.brief = "Steam not found";
        return false;
    }
    if (steamRunning()) {
#ifdef _WIN32
        r.lines.push_back("Steam is running: it rewrites its shortcut list when it exits, so close Steam completely "
                          "(tray icon -> Exit), then press again.");
#else
        // the Steam Deck: Game Mode is Steam itself, so this works from Desktop Mode with Steam exited
        r.lines.push_back("Steam is running: it rewrites its shortcut list when it exits, so close Steam completely "
                          "(Steam menu -> Exit; on a Steam Deck: Desktop Mode, then exit Steam), then press again.");
#endif
        r.brief = "Close Steam first";
        return false;
    }
    for (const auto& e : fs::directory_iterator(fs::path(steamDir) / "userdata", ec)) {
        const std::string id = e.path().filename().string();
        if (!e.is_directory(ec) || id.empty() || id == "0" ||
            !std::all_of(id.begin(), id.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }))
            continue;
        if (fs::is_directory(e.path() / "config", ec))
            out.push_back({e.path() / "config" / "shortcuts.vdf", e.path() / "config" / "grid"});
    }
    if (out.empty()) {
        r.lines.push_back("No Steam account found in " + steamDir + "\\userdata: start Steam once and sign in.");
        r.brief = "No Steam account";
        return false;
    }
    return true;
}

bool backup(const fs::path& vdf, std::vector<std::string>& lines)
{
    std::error_code ec;
    if (!fs::exists(vdf, ec))
        return true;
    const fs::path bk = vdf.string() + ".hotd-backup-" + stamp();
    if (!fs::copy_file(vdf, bk, fs::copy_options::overwrite_existing, ec)) {
        lines.push_back("cannot back up " + vdf.string() + ": left as it is");
        return false;
    }
    lines.push_back("  backup: " + bk.string());
    return true;
}

} // namespace

std::string artDir(const std::string& exePath)
{
    std::error_code ec;
    const fs::path exe = fs::absolute(exePath, ec);
    const fs::path pkg = exe.parent_path().parent_path();
    return (fs::exists(pkg / "hotd_package.txt", ec) ? pkg / "data" / "steam" : exe.parent_path() / "steam art").string();
}

namespace {
// a .png / .jpg picture's size from its header (PNG IHDR; JPEG's first SOF0-3 / SOF5-7 / SOF9-11 / SOF13-15 marker)
bool imageSize(const fs::path& p, int& w, int& h)
{
    std::ifstream f(p, std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (b.size() >= 24 && b[0] == 0x89 && b[1] == 'P') {
        w = (b[16] << 24) | (b[17] << 16) | (b[18] << 8) | b[19];
        h = (b[20] << 24) | (b[21] << 16) | (b[22] << 8) | b[23];
        return true;
    }
    if (b.size() >= 4 && b[0] == 0xFF && b[1] == 0xD8) {
        for (size_t i = 2; i + 9 < b.size();) {
            if (b[i] != 0xFF) { ++i; continue; }
            const uint8_t m = b[i + 1];
            if (m == 0xFF) { ++i; continue; }
            const size_t len = size_t(b[i + 2] << 8 | b[i + 3]);
            if (m >= 0xC0 && m <= 0xCF && m != 0xC4 && m != 0xC8 && m != 0xCC) {
                h = b[i + 5] << 8 | b[i + 6];
                w = b[i + 7] << 8 | b[i + 8];
                return true;
            }
            i += 2 + len;
        }
    }
    return false;
}
} // namespace

std::vector<std::string> artChoices(const std::string& dir, const std::string& kind)
{
    std::vector<std::string> out;
    std::error_code ec;
    auto scan = [&](const char* sub, int shape) {   // shape: 0 any, 1 tall (covers), 2 wide (banners)
        for (const auto& e : fs::directory_iterator(fs::path(dir) / sub, ec)) {
            if (!e.is_regular_file(ec)) continue;
            const std::string x = lowerExt(e.path());
            if (!(x == ".png" || x == ".jpg" || x == ".jpeg" || (kind == "icon" && x == ".ico"))) continue;
            if (shape) {
                int w = 0, h = 0;
                if (!imageSize(e.path(), w, h) || (shape == 1) != (h > w)) continue;
            }
            std::string low = e.path().filename().string();
            for (char& ch : low) ch = char(std::tolower(static_cast<unsigned char>(ch)));
            if (low.rfind("default", 0) == 0) continue;   // the game's own picture: the "Default" entry
            out.push_back(std::string(sub) + "/" + e.path().filename().string());
        }
    };
    if (kind == "cover") { scan("covers", 0); scan("grids", 1); }
    else if (kind == "banner") { scan("wide covers", 0); scan("banners", 0); scan("grids", 2); }   // Steam's wide cover
    else if (kind == "hero") { scan("heros", 0); scan("heroes", 0); }
    else if (kind == "logo") scan("logos", 0);
    else if (kind == "icon") scan("icons", 0);
    std::sort(out.begin(), out.end());
    return out;
}

bool steamRunning()
{
#ifdef _WIN32
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return false;
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof pe;
    bool found = false;
    for (BOOL ok = Process32FirstW(snap, &pe); ok && !found; ok = Process32NextW(snap, &pe))
        found = _wcsicmp(pe.szExeFile, L"steam.exe") == 0;
    CloseHandle(snap);
    return found;
#elif defined(__APPLE__)
    // macOS: the processes' names (libproc); Steam's is "steam_osx"
    const int n = proc_listallpids(nullptr, 0);
    if (n <= 0)
        return false;
    std::vector<pid_t> pids(size_t(n) + 16);
    const int got = proc_listallpids(pids.data(), int(pids.size() * sizeof(pid_t)));
    for (int i = 0; i < got; ++i) {
        char name[256] = {};
        if (proc_name(pids[size_t(i)], name, sizeof name) > 0 && std::strcmp(name, "steam_osx") == 0)
            return true;
    }
    return false;
#else
    // Linux: a process named "steam" (/proc/<pid>/comm; the Steam client's main process on desktops and the Deck)
    std::error_code ec;
    for (const auto& e : fs::directory_iterator("/proc", ec)) {
        const std::string pid = e.path().filename().string();
        if (pid.empty() || !std::all_of(pid.begin(), pid.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }))
            continue;
        std::ifstream f(e.path() / "comm");
        std::string comm;
        if (std::getline(f, comm) && comm == "steam")
            return true;
    }
    return false;
#endif
}

bool writeDefaultPictures(const Options& o, std::vector<std::string>& lines)
{
    // the pictures made from the player's PC files, as Default.png in each kind's folder (user 2026-10-18: "add the
    // default too"), so they can be seen next to the others; they are the "Default" choice
    const Art art = composeArt(o.pcDir, lines);
    if (!art.ok)
        return false;
    std::error_code ec;
    const fs::path dir = o.customDir.empty() ? fs::path(artDir(o.exe)) : fs::path(o.customDir);
    const std::pair<const char*, const Img*> kinds[] = {
        {"covers", &art.cover}, {"wide covers", &art.capsule}, {"heros", &art.hero}, {"logos", &art.logo}, {"icons", &art.icon}};
    bool ok = true;
    for (const auto& [sub, im] : kinds) {
        fs::create_directories(dir / sub, ec);
        const fs::path p = dir / sub / "Default.png";
        const bool alpha = im == &art.logo || (im == &art.icon && art.iconAlpha);
        ok = (alpha ? writePngRgba(p.string(), im->w, im->h, im->px) : writeRgb(p, *im)) && ok;
    }
    if (!art.ico.empty()) {   // the official icon too (user 2026-10-18): THOTD.EXE's icon group as it is
        std::ofstream f(dir / "icons" / "Default.ico", std::ios::binary | std::ios::trunc);
        f.write(reinterpret_cast<const char*>(art.ico.data()), std::streamsize(art.ico.size()));
        ok = bool(f) && ok;
    }
    lines.push_back("the default pictures (from your PC files): " + dir.string() + "\\*\\Default.png");
    return ok;
}

Result addToSteam(const Options& o)
{
    Result r;
    std::vector<Target> ts;
    if (!targets(o, ts, r))
        return r;
    std::error_code ec;
    const fs::path exe = fs::absolute(o.exe, ec);
    const std::string quoted = "\"" + exe.string() + "\"", startDir = "\"" + exe.parent_path().string() + "\"";
    const uint32_t id = shortcutId(quoted, kName);
    const Art art = composeArt(o.pcDir, r.lines);
    // the player's own pictures: the ones chosen per kind (Options::picks, from the art folder's subfolders), else
    // files named cover / banner / hero / logo / icon directly in the folder
    const std::string customDir = o.customDir.empty() ? artDir(exe.string()) : o.customDir;
    std::map<std::string, fs::path> own = ownPictures(customDir, r.lines);
    for (const auto& [kind, rel] : o.picks) {
        if (rel.empty()) continue;
        const fs::path p = fs::path(customDir) / rel;
        if (fs::is_regular_file(p, ec)) {
            own[kind] = p;
            r.lines.push_back("  " + kind + ": chosen " + rel);
        } else {
            r.lines.push_back("  " + kind + ": " + rel + " is not there any more: the game's picture");
        }
    }
    int done = 0;
    for (const Target& t : ts) {
        std::vector<KV> entries;
        std::string err;
        if (!loadVdf(t.vdf, entries, err)) {
            r.lines.push_back(err);
            continue;
        }
        KV* e = nullptr;
        for (KV& k : entries)
            if (k.type == 0 && ourName(text(k, "AppName"))) { e = &k; break; }
        const bool update = e != nullptr;
        std::vector<uint32_t> oldIds;   // the art of an earlier name / exe goes
        if (update && (text(*e, "AppName") != kName || text(*e, "Exe") != quoted))
            oldIds.push_back(shortcutId(text(*e, "Exe"), text(*e, "AppName")));
        if (!backup(t.vdf, r.lines))
            continue;
        if (!e) {
            entries.push_back(KV{});
            e = &entries.back();
            e->type = 0;
            e->key = std::to_string(entries.size() - 1);
        }
        const auto ownIcon = own.find("icon");
        const fs::path icon = ownIcon != own.end() ? t.grid / (std::to_string(id) + "_icon" + lowerExt(ownIcon->second))
                                                   : t.grid / (std::to_string(id) + (art.ico.empty() ? "_icon.png" : "_icon.ico"));
        field(*e, "appid", 2).i = id;
        field(*e, "AppName", 1).s = kName;
        field(*e, "Exe", 1).s = quoted;
        field(*e, "StartDir", 1).s = startDir;
        field(*e, "icon", 1).s = art.ok || ownIcon != own.end() ? icon.string() : std::string();
        // the rest only when missing (an update keeps the player's play time, tags and launch options)
        for (const char* k : {"ShortcutPath", "LaunchOptions", "DevkitGameID", "FlatpakAppID"}) field(*e, k, 1);
        for (const char* k : {"IsHidden", "OpenVR", "Devkit", "DevkitOverrideAppID", "LastPlayTime"}) field(*e, k, 2);
        for (const char* k : {"AllowDesktopConfig", "AllowOverlay"}) {
            KV& f = field(*e, k, 2);
            if (!update) f.i = 1;
        }
        field(*e, "tags", 0);
        renumber(entries);
        fs::create_directories(t.vdf.parent_path(), ec);
        if (!saveVdf(t.vdf, entries)) {
            r.lines.push_back("cannot write " + t.vdf.string());
            continue;
        }
        r.lines.push_back(std::string(update ? "updated" : "added") + " \"" + kName + "\" in " + t.vdf.string());
        if (art.ok || !own.empty()) {
            // each picture: the player's own (customDir) if there is one, else the one made from the PC files; the
            // same picture in another format from an earlier press goes first (Steam would pick either)
            fs::create_directories(t.grid, ec);
            const std::string pre = (t.grid / std::to_string(id)).string();
            bool ok = true;
            for (const auto& [kind, suffix] : kArtKinds) {
                for (const char* x : {".png", ".jpg", ".jpeg", ".ico"}) fs::remove(pre + suffix + x, ec);
                const auto mine = own.find(kind);
                if (mine != own.end()) {
                    ok = fs::copy_file(mine->second, pre + suffix + lowerExt(mine->second), fs::copy_options::overwrite_existing, ec) && ok;
                    r.lines.push_back("  " + std::string(kind) + ": your picture " + mine->second.string());
                    continue;
                }
                if (!art.ok) continue;
                if (kind == std::string("icon") && !art.ico.empty()) {   // the Default icon: THOTD.EXE's own .ico
                    std::ofstream f(pre + suffix + ".ico", std::ios::binary | std::ios::trunc);
                    f.write(reinterpret_cast<const char*>(art.ico.data()), std::streamsize(art.ico.size()));
                    ok = bool(f) && ok;
                    continue;
                }
                const Img& im = kind == std::string("cover") ? art.cover : kind == std::string("banner") ? art.capsule
                              : kind == std::string("hero") ? art.hero : kind == std::string("logo") ? art.logo : art.icon;
                const bool alpha = kind == std::string("logo") || (kind == std::string("icon") && art.iconAlpha);
                ok = (alpha ? writePngRgba(pre + suffix + ".png", im.w, im.h, im.px) : writeRgb(pre + suffix + ".png", im)) && ok;
            }
            r.lines.push_back(ok ? "  artwork: " + pre + "*" : "  artwork: could not write all of " + pre + "*");
        }
        for (uint32_t old : oldIds)
            if (old != id) removeArt(t.grid, old, r.lines);
        ++done;
    }
    r.ok = done > 0;
    r.brief = r.ok ? "Added: start Steam" : "Failed";
    if (r.ok)
        r.lines.push_back("Done: start Steam; \"" + std::string(kName) + "\" is in your Library.");
    return r;
}

Result removeFromSteam(const Options& o)
{
    Result r;
    std::vector<Target> ts;
    if (!targets(o, ts, r))
        return r;
    int removed = 0;
    for (const Target& t : ts) {
        std::vector<KV> entries;
        std::string err;
        if (!loadVdf(t.vdf, entries, err)) {
            r.lines.push_back(err);
            continue;
        }
        std::vector<uint32_t> ids;
        for (const KV& k : entries)
            if (k.type == 0 && ourName(text(k, "AppName")))
                ids.push_back(shortcutId(text(k, "Exe"), text(k, "AppName")));
        if (ids.empty())
            continue;
        if (!backup(t.vdf, r.lines))
            continue;
        entries.erase(std::remove_if(entries.begin(), entries.end(),
                                     [](const KV& k) { return k.type == 0 && ourName(text(k, "AppName")); }),
                      entries.end());
        renumber(entries);
        if (!saveVdf(t.vdf, entries)) {
            r.lines.push_back("cannot write " + t.vdf.string());
            continue;
        }
        r.lines.push_back("removed \"" + std::string(kName) + "\" from " + t.vdf.string());
        for (uint32_t id : ids) removeArt(t.grid, id, r.lines);
        removed += int(ids.size());
    }
    r.ok = removed > 0;
    r.brief = r.ok ? "Removed" : "Not in Steam";
    if (!r.ok && r.lines.empty())
        r.lines.push_back("\"" + std::string(kName) + "\" is not in Steam: nothing to remove.");
    return r;
}

} // namespace hotd::steam
