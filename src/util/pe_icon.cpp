#include "util/pe_icon.h"

#include <algorithm>
#include <cstdio>

namespace peicon {
namespace {

bool readFile(const std::string& path, std::vector<uint8_t>& d) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    const long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (n <= 0 || n > (1l << 30)) { std::fclose(f); return false; }
    d.resize(size_t(n));
    const bool ok = std::fread(d.data(), 1, d.size(), f) == d.size();
    std::fclose(f);
    return ok;
}

uint16_t u16(const std::vector<uint8_t>& d, size_t o) { return o + 2 <= d.size() ? uint16_t(d[o] | d[o + 1] << 8) : uint16_t(0); }
uint32_t u32(const std::vector<uint8_t>& d, size_t o) {
    return o + 4 <= d.size() ? uint32_t(d[o]) | uint32_t(d[o + 1]) << 8 | uint32_t(d[o + 2]) << 16 | uint32_t(d[o + 3]) << 24 : 0u;
}

struct Pe {
    std::vector<uint8_t> d;
    struct Sec { uint32_t va, vsize, raw, rawSize; };
    std::vector<Sec> secs;
    uint32_t resRva = 0;
    size_t resOff = 0;

    bool rvaToOff(uint32_t rva, size_t& off) const {
        for (const Sec& s : secs)
            if (rva >= s.va && rva < s.va + std::max(s.vsize, s.rawSize)) { off = size_t(s.raw) + (rva - s.va); return off < d.size(); }
        return false;
    }
    bool open(const std::string& path) {
        std::vector<uint8_t> data;
        return readFile(path, data) && load(std::move(data));
    }
    bool load(std::vector<uint8_t> data) {
        d = std::move(data);
        if (d.size() < 0x100 || d[0] != 'M' || d[1] != 'Z') return false;
        const size_t pe = u32(d, 0x3c);
        if (u32(d, pe) != 0x00004550) return false;
        const unsigned nsec = u16(d, pe + 6);
        const size_t optSize = u16(d, pe + 20), opt = pe + 24;
        const uint16_t magic = u16(d, opt);
        if (magic != 0x10b && magic != 0x20b) return false;
        const size_t dirs = opt + (magic == 0x20b ? 112 : 96);
        resRva = u32(d, dirs + 16);
        for (unsigned i = 0; i < nsec && i < 96; ++i) {
            const size_t s = opt + optSize + i * 40;
            secs.push_back({u32(d, s + 12), u32(d, s + 8), u32(d, s + 20), u32(d, s + 16)});
        }
        return resRva && rvaToOff(resRva, resOff);
    }
    struct Ent { uint32_t id; uint32_t target; bool isDir; };
    // Children of the directory at `dir` (offset inside the resource section); a named entry keeps its name-offset
    // flag in `id` (THOTD.EXE's icon group is named), so it still compares equal to itself.
    std::vector<Ent> entries(uint32_t dir) const {
        std::vector<Ent> r;
        const size_t o = resOff + dir;
        const unsigned n = unsigned(u16(d, o + 12)) + u16(d, o + 14);
        for (unsigned i = 0; i < n && i < 512; ++i) {
            const uint32_t id = u32(d, o + 16 + i * 8), t = u32(d, o + 20 + i * 8);
            r.push_back({id, t & 0x7fffffffu, (t & 0x80000000u) != 0});
        }
        return r;
    }
    // Data of resource (type, id), first language.
    bool find(uint32_t type, uint32_t id, std::vector<uint8_t>& out) const {
        for (const Ent& t : entries(0)) {
            if (t.id != type || !t.isDir) continue;
            for (const Ent& n : entries(t.target)) {
                if (n.id != id || !n.isDir) continue;
                for (const Ent& l : entries(n.target)) {
                    if (l.isDir) continue;
                    const size_t e = resOff + l.target;
                    size_t off;
                    const uint32_t size = u32(d, e + 4);
                    if (!rvaToOff(u32(d, e), off) || off + size > d.size()) return false;
                    out.assign(d.begin() + long(off), d.begin() + long(off + size));
                    return true;
                }
            }
        }
        return false;
    }
    bool firstGroup(uint32_t& id) const {
        for (const Ent& t : entries(0))
            if (t.id == 14 && t.isDir) {
                const std::vector<Ent> g = entries(t.target);
                if (g.empty()) return false;
                id = g.front().id;
                return true;
            }
        return false;
    }
};

struct Entry { int w, h, bits; uint16_t id; };

bool readGroup(const Pe& pe, std::vector<Entry>& es) {
    uint32_t gid;
    std::vector<uint8_t> g;
    if (!pe.firstGroup(gid) || !pe.find(14, gid, g) || g.size() < 6) return false;
    const unsigned n = u16(g, 4);
    for (unsigned i = 0; i < n && 6 + (i + 1) * 14 <= g.size(); ++i) {
        const size_t o = 6 + i * 14;
        es.push_back({g[o] ? g[o] : 256, g[o + 1] ? g[o + 1] : 256, int(u16(g, o + 6)), u16(g, o + 12)});
    }
    return !es.empty();
}

bool isPng(const std::vector<uint8_t>& b) { return b.size() > 8 && b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G'; }

bool decodeDib(const std::vector<uint8_t>& b, Rgba& out) {
    if (b.size() < 40 || u32(b, 0) < 40) return false;
    const int w = int(u32(b, 4));
    const int h2 = int(u32(b, 8));
    const int bits = u16(b, 14);
    const uint32_t comp = u32(b, 16);
    uint32_t colors = u32(b, 32);
    if (comp != 0 || w <= 0 || w > 1024 || h2 <= 0 || h2 > 2048) return false;
    const int h = h2 / 2;
    if (bits != 1 && bits != 4 && bits != 8 && bits != 24 && bits != 32) return false;
    if (bits <= 8 && colors == 0) colors = 1u << bits;
    const size_t palOff = u32(b, 0), pixOff = palOff + (bits <= 8 ? size_t(colors) * 4 : 0);
    const size_t stride = ((size_t(w) * size_t(bits) + 31) / 32) * 4;
    const size_t maskOff = pixOff + stride * size_t(h), maskStride = ((size_t(w) + 31) / 32) * 4;
    if (maskOff > b.size()) return false;
    const bool haveMask = maskOff + maskStride * size_t(h) <= b.size();
    out.w = w; out.h = h; out.px.assign(size_t(w) * size_t(h) * 4, 0);
    bool anyAlpha = false;
    for (int y = 0; y < h; ++y) {
        const size_t row = pixOff + stride * size_t(h - 1 - y);
        uint8_t* dst = &out.px[size_t(y) * size_t(w) * 4];
        for (int x = 0; x < w; ++x) {
            uint8_t r, g, bl, a = 255;
            if (bits == 32 || bits == 24) {
                const size_t p = row + size_t(x) * size_t(bits / 8);
                bl = b[p]; g = b[p + 1]; r = b[p + 2];
                if (bits == 32) { a = b[p + 3]; anyAlpha |= a != 0; }
            } else {
                unsigned idx;
                if (bits == 8) idx = b[row + size_t(x)];
                else if (bits == 4) idx = (b[row + size_t(x) / 2] >> ((x & 1) ? 0 : 4)) & 15;
                else idx = (b[row + size_t(x) / 8] >> (7 - (x & 7))) & 1;
                const size_t p = palOff + size_t(idx) * 4;
                if (p + 3 >= b.size()) return false;
                bl = b[p]; g = b[p + 1]; r = b[p + 2];
            }
            dst[x * 4] = r; dst[x * 4 + 1] = g; dst[x * 4 + 2] = bl; dst[x * 4 + 3] = a;
        }
    }
    if ((bits != 32 || !anyAlpha) && haveMask) {   // the AND mask gives the transparency
        for (int y = 0; y < h; ++y) {
            const size_t row = maskOff + maskStride * size_t(h - 1 - y);
            for (int x = 0; x < w; ++x)
                out.px[(size_t(y) * size_t(w) + size_t(x)) * 4 + 3] = ((b[row + size_t(x) / 8] >> (7 - (x & 7))) & 1) ? 0 : 255;
        }
    } else if (bits == 32 && !anyAlpha) {
        for (size_t i = 3; i < out.px.size(); i += 4) out.px[i] = 255;
    }
    return true;
}

// a bitmap resource (RT_BITMAP: a BMP without its file header): uncompressed 1 / 4 / 8 / 24 / 32-bit, bottom-up or
// top-down (negative height); opaque
bool decodeBitmapRes(const std::vector<uint8_t>& b, Rgba& out) {
    if (b.size() < 40 || u32(b, 0) < 40) return false;
    const int w = int(u32(b, 4)), hs = int(int32_t(u32(b, 8)));
    const int bits = u16(b, 14);
    const uint32_t comp = u32(b, 16);
    uint32_t colors = u32(b, 32);
    const bool topDown = hs < 0;
    const int h = topDown ? -hs : hs;
    if ((comp != 0 && comp != 3) || w <= 0 || w > 4096 || h <= 0 || h > 4096) return false;
    if (bits != 1 && bits != 4 && bits != 8 && bits != 24 && bits != 32) return false;
    if (bits <= 8 && colors == 0) colors = 1u << bits;
    const size_t palOff = u32(b, 0) + (comp == 3 ? 12 : 0), pixOff = palOff + (bits <= 8 ? size_t(colors) * 4 : 0);
    const size_t stride = ((size_t(w) * size_t(bits) + 31) / 32) * 4;
    if (pixOff + stride * size_t(h) > b.size()) return false;
    out.w = w; out.h = h; out.px.assign(size_t(w) * size_t(h) * 4, 255);
    for (int y = 0; y < h; ++y) {
        const size_t row = pixOff + stride * size_t(topDown ? y : h - 1 - y);
        uint8_t* dst = &out.px[size_t(y) * size_t(w) * 4];
        for (int x = 0; x < w; ++x) {
            uint8_t r, g, bl;
            if (bits == 32 || bits == 24) {
                const size_t p = row + size_t(x) * size_t(bits / 8);
                bl = b[p]; g = b[p + 1]; r = b[p + 2];
            } else {
                unsigned idx;
                if (bits == 8) idx = b[row + size_t(x)];
                else if (bits == 4) idx = (b[row + size_t(x) / 2] >> ((x & 1) ? 0 : 4)) & 15;
                else idx = (b[row + size_t(x) / 8] >> (7 - (x & 7))) & 1;
                const size_t p = palOff + size_t(idx) * 4;
                if (p + 3 >= b.size()) return false;
                bl = b[p]; g = b[p + 1]; r = b[p + 2];
            }
            dst[x * 4] = r; dst[x * 4 + 1] = g; dst[x * 4 + 2] = bl;
        }
    }
    return true;
}

}  // namespace

bool firstBitmap(std::vector<uint8_t> exe, Rgba& out) {
    Pe pe;
    if (!pe.load(std::move(exe))) return false;
    for (const Pe::Ent& t : pe.entries(0)) {
        if (t.id != 2 || !t.isDir) continue;   // RT_BITMAP
        for (const Pe::Ent& n : pe.entries(t.target)) {
            if (!n.isDir) continue;
            std::vector<uint8_t> data;
            if (pe.find(2, n.id, data) && decodeBitmapRes(data, out)) return true;
        }
    }
    return false;
}

bool extractIco(const std::string& exePath, std::vector<uint8_t>& ico) {
    std::vector<uint8_t> d;
    return readFile(exePath, d) && extractIco(std::move(d), ico);
}

bool extractIco(std::vector<uint8_t> exe, std::vector<uint8_t>& ico) {
    Pe pe;
    std::vector<Entry> es;
    if (!pe.load(std::move(exe)) || !readGroup(pe, es)) return false;
    std::vector<std::vector<uint8_t>> data;
    std::vector<Entry> kept;
    for (const Entry& e : es) {
        std::vector<uint8_t> b;
        if (pe.find(3, e.id, b)) { data.push_back(std::move(b)); kept.push_back(e); }
    }
    if (kept.empty()) return false;
    auto put16 = [&](unsigned v) { ico.push_back(uint8_t(v)); ico.push_back(uint8_t(v >> 8)); };
    auto put32 = [&](uint32_t v) { put16(v & 0xffff); put16(v >> 16); };
    ico.clear();
    put16(0); put16(1); put16(unsigned(kept.size()));
    uint32_t off = 6 + 16 * uint32_t(kept.size());
    for (size_t i = 0; i < kept.size(); ++i) {
        ico.push_back(uint8_t(kept[i].w >= 256 ? 0 : kept[i].w));
        ico.push_back(uint8_t(kept[i].h >= 256 ? 0 : kept[i].h));
        ico.push_back(0); ico.push_back(0);
        put16(1); put16(unsigned(kept[i].bits));
        put32(uint32_t(data[i].size())); put32(off);
        off += uint32_t(data[i].size());
    }
    for (const auto& b : data) ico.insert(ico.end(), b.begin(), b.end());
    return true;
}

bool largestImage(const std::string& exePath, Rgba& out) {
    std::vector<uint8_t> d;
    return readFile(exePath, d) && largestImage(std::move(d), out);
}

bool largestImage(std::vector<uint8_t> exe, Rgba& out) {
    Pe pe;
    std::vector<Entry> es;
    if (!pe.load(std::move(exe)) || !readGroup(pe, es)) return false;
    std::stable_sort(es.begin(), es.end(), [](const Entry& a, const Entry& b) {
        return a.w != b.w ? a.w > b.w : a.bits > b.bits;
    });
    for (const Entry& e : es) {
        std::vector<uint8_t> b;
        if (pe.find(3, e.id, b) && !isPng(b) && decodeDib(b, out)) return true;   // PNG entries (Vista+) are skipped
    }
    return false;
}

}  // namespace peicon
