// lang: a TrueType / OpenType (glyf and CFF) reader and rasterizer (ttf.h)
#include "render/ttf.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>

namespace hotd::render {

namespace {

uint16_t u16(const std::vector<uint8_t>& d, size_t at) { return at + 2 <= d.size() ? uint16_t(d[at] << 8 | d[at + 1]) : 0; }
int16_t i16(const std::vector<uint8_t>& d, size_t at) { return int16_t(u16(d, at)); }
uint32_t u32(const std::vector<uint8_t>& d, size_t at)
{
    return at + 4 <= d.size() ? uint32_t(d[at]) << 24 | uint32_t(d[at + 1]) << 16 | uint32_t(d[at + 2]) << 8 | d[at + 3] : 0;
}
uint8_t u8(const std::vector<uint8_t>& d, size_t at) { return at < d.size() ? d[at] : 0; }
uint32_t uN(const std::vector<uint8_t>& d, size_t at, int n)
{
    uint32_t v = 0;
    for (int i = 0; i < n; ++i)
        v = v << 8 | u8(d, at + size_t(i));
    return v;
}

// a curve flattened into the polyline (font units): n segments from the deviation and the tolerance

int segmentsFor(float dev, float tol) { return std::clamp(int(std::ceil(std::sqrt(std::max(0.0f, dev) / tol))), 1, 48); }

} // namespace

bool TtfFont::load(const std::string& path, std::string& err)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        err = "cannot open " + path;
        return false;
    }
    std::vector<uint8_t> d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    file_ = path;
    return loadMemory(std::move(d), err);
}

bool TtfFont::loadMemory(std::vector<uint8_t> d, std::string& err)
{
    data_.clear();
    cache_.clear();
    if (d.size() < 12) {
        err = "not a font file";
        return false;
    }
    size_t base = 0;
    if (u32(d, 0) == 0x74746366)   // 'ttcf': the collection's first font
        base = u32(d, 12);
    const uint32_t ver = u32(d, base);
    if (ver != 0x00010000 && ver != 0x4F54544F && ver != 0x74727565) {   // 1.0, 'OTTO', 'true'
        err = "not a TrueType / OpenType font";
        return false;
    }
    const int n = u16(d, base + 4);
    uint32_t head = 0, maxp = 0, hhea = 0, os2 = 0;
    cmap_ = glyf_ = loca_ = hmtx_ = cff_ = 0;
    for (int i = 0; i < n; ++i) {
        const size_t r = base + 12 + size_t(i) * 16;
        const uint32_t tag = u32(d, r), off = u32(d, r + 8);
        if (off >= d.size()) continue;
        switch (tag) {
        case 0x636D6170: cmap_ = off; break;   // cmap
        case 0x676C7966: glyf_ = off; break;   // glyf
        case 0x6C6F6361: loca_ = off; break;   // loca
        case 0x686D7478: hmtx_ = off; break;   // hmtx
        case 0x43464620: cff_ = off; break;    // 'CFF '
        case 0x68656164: head = off; break;    // head
        case 0x6D617870: maxp = off; break;    // maxp
        case 0x68686561: hhea = off; break;    // hhea
        case 0x4F532F32: os2 = off; break;     // OS/2
        default: break;
        }
    }
    if (!cmap_ || !head || !maxp || !hhea || !hmtx_ || (!cff_ && (!glyf_ || !loca_))) {
        err = "the font misses a table this reader needs (cmap / head / maxp / hhea / hmtx / glyf or CFF)";
        return false;
    }
    data_ = std::move(d);
    upem_ = std::max<int>(16, u16(data_, head + 18));
    locFormat_ = i16(data_, head + 50);
    numGlyphs_ = u16(data_, maxp + 4);
    ascent_ = i16(data_, hhea + 4);
    descent_ = i16(data_, hhea + 6);
    numHMetrics_ = std::max<int>(1, u16(data_, hhea + 34));
    if (os2 && ascent_ == 0 && descent_ == 0)
        ascent_ = i16(data_, os2 + 68), descent_ = i16(data_, os2 + 70);
    capH_ = 0;
    if (os2 && u16(data_, os2) >= 2)
        capH_ = i16(data_, os2 + 88);
    // the cmap subtable: Unicode full (3,10 / 0,4 / 0,6: format 12), else BMP (3,1 / 0,3: format 4)
    {
        const int nt = u16(data_, cmap_ + 2);
        uint32_t best = 0;
        int bestScore = 0;
        for (int i = 0; i < nt; ++i) {
            const size_t r = cmap_ + 4 + size_t(i) * 8;
            const int pid = u16(data_, r), eid = u16(data_, r + 2);
            const uint32_t off = cmap_ + u32(data_, r + 4);
            const int fmt = u16(data_, off);
            int score = 0;
            if (fmt == 12 && ((pid == 3 && eid == 10) || pid == 0)) score = 3;
            else if (fmt == 4 && ((pid == 3 && eid == 1) || pid == 0)) score = 2;
            if (score > bestScore) bestScore = score, best = off;
        }
        if (!best) {
            data_.clear();
            err = "the font has no Unicode cmap (format 4 / 12)";
            return false;
        }
        cmap_ = best;
        cmapFormat_ = u16(data_, best);
    }
    if (cff_) {
        // CFF: header, Name INDEX, Top DICT INDEX, String INDEX, Global Subr INDEX
        auto indexEnd = [&](const Index& ix) { return ix.count ? ix.data + uN(data_, ix.off + 3 + size_t(ix.count) * ix.offSize, ix.offSize) : ix.off + 2; };
        const Index names = readIndex(cff_ + u8(data_, cff_ + 2));
        const Index top = readIndex(indexEnd(names));
        const Index strings = readIndex(indexEnd(top));
        gsubr_ = readIndex(indexEnd(strings));
        uint32_t s0, s1;
        if (!indexItem(top, 0, s0, s1)) {
            data_.clear();
            err = "bad CFF table";
            return false;
        }
        // a DICT's operators: (op, operands) through a callback
        auto dict = [&](uint32_t a, uint32_t b, auto&& fn) {
            std::vector<double> ops;
            while (a < b) {
                const int b0 = u8(data_, a);
                if (b0 <= 21) {
                    int op = b0;
                    ++a;
                    if (b0 == 12) op = 1200 + u8(data_, a++);
                    fn(op, ops);
                    ops.clear();
                } else if (b0 == 28) { ops.push_back(i16(data_, a + 1)); a += 3; }
                else if (b0 == 29) { ops.push_back(int32_t(u32(data_, a + 1))); a += 5; }
                else if (b0 == 30) {   // a real: nibbles to 0xF (parsed roughly: only offsets / sizes matter here)
                    std::string t;
                    ++a;
                    for (bool end = false; !end && a < b; ++a)
                        for (int k = 0; k < 2 && !end; ++k) {
                            const int nb = k ? u8(data_, a) & 15 : u8(data_, a) >> 4;
                            if (nb <= 9) t += char('0' + nb);
                            else if (nb == 0xA) t += '.';
                            else if (nb == 0xB) t += 'E';
                            else if (nb == 0xC) t += "E-";
                            else if (nb == 0xE) t += '-';
                            else if (nb == 0xF) end = true;
                        }
                    ops.push_back(std::atof(t.c_str()));
                } else if (b0 >= 32 && b0 <= 246) { ops.push_back(b0 - 139); a += 1; }
                else if (b0 >= 247 && b0 <= 250) { ops.push_back((b0 - 247) * 256 + u8(data_, a + 1) + 108); a += 2; }
                else if (b0 >= 251 && b0 <= 254) { ops.push_back(-(b0 - 251) * 256 - u8(data_, a + 1) - 108); a += 2; }
                else ++a;
            }
        };
        uint32_t charStrings = 0, fdArray = 0, privSize = 0, privOff = 0;
        cid_ = false;
        fdSelect_ = 0;
        dict(s0, s1, [&](int op, const std::vector<double>& o) {
            if (op == 17 && !o.empty()) charStrings = uint32_t(o[0]);
            else if (op == 18 && o.size() >= 2) privSize = uint32_t(o[0]), privOff = uint32_t(o[1]);
            else if (op == 1230) cid_ = true;
            else if (op == 1236 && !o.empty()) fdArray = uint32_t(o[0]);
            else if (op == 1237 && !o.empty()) fdSelect_ = cff_ + uint32_t(o[0]);
        });
        cs_ = readIndex(cff_ + charStrings);
        auto privSubrs = [&](uint32_t size, uint32_t off) {
            Index ix;
            uint32_t subrs = 0;
            dict(cff_ + off, cff_ + off + size, [&](int op, const std::vector<double>& o) {
                if (op == 19 && !o.empty()) subrs = uint32_t(o[0]);
            });
            if (subrs) ix = readIndex(cff_ + off + subrs);
            return ix;
        };
        lsubr_.clear();
        if (cid_ && fdArray) {
            const Index fds = readIndex(cff_ + fdArray);
            for (int i = 0; i < fds.count; ++i) {
                uint32_t a, b, ps = 0, po = 0;
                if (indexItem(fds, i, a, b))
                    dict(a, b, [&](int op, const std::vector<double>& o) {
                        if (op == 18 && o.size() >= 2) ps = uint32_t(o[0]), po = uint32_t(o[1]);
                    });
                lsubr_.push_back(ps ? privSubrs(ps, po) : Index{});
            }
        } else
            lsubr_.push_back(privSize ? privSubrs(privSize, privOff) : Index{});
        if (cs_.count <= 0) {
            data_.clear();
            err = "bad CFF table (no charstrings)";
            return false;
        }
    }
    if (capH_ <= 0) {   // the height of 'H'
        Path p;
        outline(glyphIndex('H'), p, 1.0f);
        float top = 0;
        for (const auto& c : p)
            for (const Pt& q : c) top = std::max(top, q.y);
        capH_ = top > 0 ? int(top) : upem_ * 7 / 10;
    }
    return true;
}

TtfFont::Index TtfFont::readIndex(uint32_t at) const
{
    Index ix;
    ix.off = at;
    ix.count = u16(data_, at);
    if (ix.count) {
        ix.offSize = u8(data_, at + 2);
        ix.data = at + 3 + uint32_t(ix.count + 1) * uint32_t(ix.offSize) - 1;
    }
    return ix;
}

bool TtfFont::indexItem(const Index& ix, int i, uint32_t& start, uint32_t& end) const
{
    if (i < 0 || i >= ix.count || ix.offSize < 1 || ix.offSize > 4)
        return false;
    start = ix.data + uN(data_, ix.off + 3 + size_t(i) * ix.offSize, ix.offSize);
    end = ix.data + uN(data_, ix.off + 3 + size_t(i + 1) * ix.offSize, ix.offSize);
    return end >= start && end <= data_.size();
}

int TtfFont::fdFor(int gid) const
{
    if (!cid_ || !fdSelect_)
        return 0;
    const int fmt = u8(data_, fdSelect_);
    if (fmt == 0)
        return u8(data_, fdSelect_ + 1 + uint32_t(gid));
    if (fmt == 3) {
        const int n = u16(data_, fdSelect_ + 1);
        for (int i = 0; i < n; ++i) {
            const uint32_t r = fdSelect_ + 3 + uint32_t(i) * 3;
            const int first = u16(data_, r), next = u16(data_, r + 3);
            if (gid >= first && gid < next)
                return u8(data_, r + 2);
        }
    }
    return 0;
}

int TtfFont::glyphIndex(uint32_t cp) const
{
    if (data_.empty())
        return 0;
    if (cmapFormat_ == 12) {
        const uint32_t n = u32(data_, cmap_ + 12);
        uint32_t lo = 0, hi = n;
        while (lo < hi) {
            const uint32_t mid = (lo + hi) / 2;
            const size_t r = cmap_ + 16 + size_t(mid) * 12;
            const uint32_t s = u32(data_, r), e = u32(data_, r + 4);
            if (cp < s) hi = mid;
            else if (cp > e) lo = mid + 1;
            else return int(u32(data_, r + 8) + (cp - s));
        }
        return 0;
    }
    if (cp > 0xFFFF)
        return 0;
    const int segX2 = u16(data_, cmap_ + 6), segs = segX2 / 2;
    const size_t ends = cmap_ + 14, starts = ends + size_t(segX2) + 2, deltas = starts + size_t(segX2), ranges = deltas + size_t(segX2);
    int lo = 0, hi = segs;
    while (lo < hi) {   // the first segment whose end >= cp
        const int mid = (lo + hi) / 2;
        if (u16(data_, ends + size_t(mid) * 2) < cp) lo = mid + 1;
        else hi = mid;
    }
    if (lo >= segs)
        return 0;
    const uint16_t start = u16(data_, starts + size_t(lo) * 2);
    if (cp < start)
        return 0;
    const uint16_t delta = u16(data_, deltas + size_t(lo) * 2), ro = u16(data_, ranges + size_t(lo) * 2);
    if (!ro)
        return (cp + delta) & 0xFFFF;
    const uint16_t g = u16(data_, ranges + size_t(lo) * 2 + ro + (cp - start) * 2);
    return g ? (g + delta) & 0xFFFF : 0;
}

int TtfFont::advance(int gid) const
{
    if (data_.empty())
        return 0;
    return u16(data_, hmtx_ + size_t(std::min(gid, numHMetrics_ - 1)) * 4);
}

void TtfFont::outline(int gid, Path& out, float tol) const
{
    if (gid < 0 || gid >= numGlyphs_)
        return;
    if (cff_)
        cffOutline(gid, out, tol);
    else {
        const float m[6] = {1, 0, 0, 1, 0, 0};
        glyfOutline(gid, out, tol, 0, m);
    }
}

// TrueType: quadratic contours (on / off curve points, the implied on points between two off ones)
void TtfFont::glyfOutline(int gid, Path& out, float tol, int depth, const float m[6]) const
{
    if (depth > 8 || gid < 0 || gid >= numGlyphs_)
        return;
    const uint32_t o0 = locFormat_ ? u32(data_, loca_ + size_t(gid) * 4) : uint32_t(u16(data_, loca_ + size_t(gid) * 2)) * 2;
    const uint32_t o1 = locFormat_ ? u32(data_, loca_ + size_t(gid + 1) * 4) : uint32_t(u16(data_, loca_ + size_t(gid + 1) * 2)) * 2;
    if (o1 <= o0)
        return;
    const size_t g = glyf_ + o0;
    const int nc = i16(data_, g);
    auto tf = [&](float x, float y) { return Pt{m[0] * x + m[2] * y + m[4], m[1] * x + m[3] * y + m[5]}; };
    if (nc >= 0) {
        std::vector<int> ends(static_cast<size_t>(nc));
        for (int i = 0; i < nc; ++i)
            ends[size_t(i)] = u16(data_, g + 10 + size_t(i) * 2);
        const int np = nc ? ends.back() + 1 : 0;
        size_t p = g + 10 + size_t(nc) * 2;
        p += 2 + u16(data_, p);
        std::vector<uint8_t> fl(static_cast<size_t>(np));
        for (int i = 0; i < np;) {
            const uint8_t f = u8(data_, p++);
            fl[size_t(i++)] = f;
            if (f & 8)
                for (int r = u8(data_, p++); r > 0 && i < np; --r)
                    fl[size_t(i++)] = f;
        }
        std::vector<float> xs(static_cast<size_t>(np)), ys(static_cast<size_t>(np));
        int v = 0;
        for (int i = 0; i < np; ++i) {
            const uint8_t f = fl[size_t(i)];
            if (f & 2) { const int dv = u8(data_, p++); v += (f & 0x10) ? dv : -dv; }
            else if (!(f & 0x10)) { v += i16(data_, p); p += 2; }
            xs[size_t(i)] = float(v);
        }
        v = 0;
        for (int i = 0; i < np; ++i) {
            const uint8_t f = fl[size_t(i)];
            if (f & 4) { const int dv = u8(data_, p++); v += (f & 0x20) ? dv : -dv; }
            else if (!(f & 0x20)) { v += i16(data_, p); p += 2; }
            ys[size_t(i)] = float(v);
        }
        int s = 0;
        for (int c = 0; c < nc; ++c) {
            const int e = ends[size_t(c)];
            if (e < s || e >= np) break;
            const int cnt = e - s + 1;
            auto on = [&](int k) { return (fl[size_t(s + (k % cnt))] & 1) != 0; };
            auto pt = [&](int k) { return Pt{xs[size_t(s + (k % cnt))], ys[size_t(s + (k % cnt))]}; };
            // the start: an on point (or the middle of the first two off points)
            int k0 = 0;
            while (k0 < cnt && !on(k0)) ++k0;
            Pt start;
            if (k0 == cnt) {
                const Pt a = pt(0), b = pt(1);
                start = {(a.x + b.x) / 2, (a.y + b.y) / 2};
                k0 = 0;   // (start sits between 0 and 1: walk from 1)
            } else
                start = pt(k0);
            std::vector<Pt> poly{tf(start.x, start.y)};
            Pt cur = start;
            bool haveCtl = false;
            Pt ctl{};
            auto quad = [&](Pt c1, Pt to) {
                const float dx = cur.x - 2 * c1.x + to.x, dy = cur.y - 2 * c1.y + to.y;
                const int n = segmentsFor(std::sqrt(dx * dx + dy * dy) * 0.25f, tol);
                for (int i = 1; i <= n; ++i) {
                    const float t = float(i) / float(n), u = 1 - t;
                    poly.push_back(tf(u * u * cur.x + 2 * u * t * c1.x + t * t * to.x, u * u * cur.y + 2 * u * t * c1.y + t * t * to.y));
                }
                cur = to;
            };
            for (int i = 1; i <= cnt; ++i) {   // (all off: from between 0 and 1 round to 0, then back to start)
                const int k = k0 + i;
                const Pt q = pt(k);
                const bool qon = on(k);
                if (qon) {
                    if (haveCtl) quad(ctl, q);
                    else { poly.push_back(tf(q.x, q.y)); cur = q; }
                    haveCtl = false;
                } else {
                    if (haveCtl) {
                        const Pt mid{(ctl.x + q.x) / 2, (ctl.y + q.y) / 2};
                        quad(ctl, mid);
                    }
                    ctl = q;
                    haveCtl = true;
                }
            }
            if (haveCtl)
                quad(ctl, start);
            out.push_back(std::move(poly));
            s = e + 1;
        }
        return;
    }
    // a composite: its components through their transforms
    size_t p = g + 10;
    for (int guard = 0; guard < 64; ++guard) {
        const uint16_t f = u16(data_, p), sub = u16(data_, p + 2);
        p += 4;
        float dx, dy;
        if (f & 1) { dx = float(i16(data_, p)); dy = float(i16(data_, p + 2)); p += 4; }
        else { dx = float(int8_t(u8(data_, p))); dy = float(int8_t(u8(data_, p + 1))); p += 2; }
        if (!(f & 2)) dx = dy = 0;   // (point matching: not supported)
        float a = 1, b = 0, c = 0, d = 1;
        auto f2 = [&](size_t at) { return float(i16(data_, at)) / 16384.0f; };
        if (f & 8) { a = d = f2(p); p += 2; }
        else if (f & 0x40) { a = f2(p); d = f2(p + 2); p += 4; }
        else if (f & 0x80) { a = f2(p); b = f2(p + 2); c = f2(p + 4); d = f2(p + 6); p += 8; }
        const float cm[6] = {m[0] * a + m[2] * b, m[1] * a + m[3] * b, m[0] * c + m[2] * d, m[1] * c + m[3] * d,
                             m[0] * dx + m[2] * dy + m[4], m[1] * dx + m[3] * dy + m[5]};
        glyfOutline(sub, out, tol, depth + 1, cm);
        if (!(f & 0x20))
            break;
    }
}

// CFF: a Type 2 charstring interpreter (the path only: hints counted and skipped, the width ignored: hmtx has it)
void TtfFont::cffOutline(int gid, Path& out, float tol) const
{
    uint32_t a, b;
    if (!indexItem(cs_, gid, a, b))
        return;
    const int fd = fdFor(gid);
    const Index& local = lsubr_.empty() ? Index{} : lsubr_[size_t(std::min<int>(fd, int(lsubr_.size()) - 1))];
    auto bias = [](int n) { return n < 1240 ? 107 : n < 33900 ? 1131 : 32768; };
    float st[64];
    int sp = 0, stems = 0;
    float x = 0, y = 0;
    bool open = false, done = false;
    std::vector<Pt> poly;
    auto close = [&]() {
        if (open && poly.size() > 1) out.push_back(poly);
        poly.clear();
        open = false;
    };
    auto moveTo = [&](float dx, float dy) {
        close();
        x += dx, y += dy;
        poly.push_back({x, y});
        open = true;
    };
    auto lineTo = [&](float dx, float dy) {
        if (!open) moveTo(0, 0);
        x += dx, y += dy;
        poly.push_back({x, y});
    };
    auto curveTo = [&](float dx1, float dy1, float dx2, float dy2, float dx3, float dy3) {
        if (!open) moveTo(0, 0);
        const Pt p0{x, y}, p1{x + dx1, y + dy1}, p2{p1.x + dx2, p1.y + dy2}, p3{p2.x + dx3, p2.y + dy3};
        const float ax = p0.x - 2 * p1.x + p2.x, ay = p0.y - 2 * p1.y + p2.y, bx = p1.x - 2 * p2.x + p3.x, by = p1.y - 2 * p2.y + p3.y;
        const float dev = 0.75f * std::sqrt(std::max(ax * ax + ay * ay, bx * bx + by * by));
        const int n = segmentsFor(dev, tol);
        for (int i = 1; i <= n; ++i) {
            const float t = float(i) / float(n), u = 1 - t;
            const float w0 = u * u * u, w1 = 3 * u * u * t, w2 = 3 * u * t * t, w3 = t * t * t;
            poly.push_back({w0 * p0.x + w1 * p1.x + w2 * p2.x + w3 * p3.x, w0 * p0.y + w1 * p1.y + w2 * p2.y + w3 * p3.y});
        }
        x = p3.x, y = p3.y;
    };
    std::function<void(uint32_t, uint32_t, int)> run = [&](uint32_t p, uint32_t e, int depth) {
        if (depth > 12)
            return;
        while (p < e && !done) {
            const int b0 = u8(data_, p);
            if (b0 >= 32 || b0 == 28) {
                float v;
                if (b0 == 28) { v = float(i16(data_, p + 1)); p += 3; }
                else if (b0 <= 246) { v = float(b0 - 139); p += 1; }
                else if (b0 <= 250) { v = float((b0 - 247) * 256 + u8(data_, p + 1) + 108); p += 2; }
                else if (b0 <= 254) { v = float(-(b0 - 251) * 256 - u8(data_, p + 1) - 108); p += 2; }
                else { v = float(int32_t(u32(data_, p + 1))) / 65536.0f; p += 5; }
                if (sp < 48) st[sp++] = v;
                continue;
            }
            ++p;
            int i = 0;
            switch (b0) {
            case 1: case 3: case 18: case 23:   // stems
                stems += sp / 2;
                sp = 0;
                break;
            case 19: case 20:   // hintmask / cntrmask (the vstems before it counted)
                stems += sp / 2;
                sp = 0;
                p += uint32_t((stems + 7) / 8);
                break;
            case 21:   // rmoveto
                if (sp >= 2) moveTo(st[sp - 2], st[sp - 1]);
                sp = 0;
                break;
            case 22:   // hmoveto
                if (sp >= 1) moveTo(st[sp - 1], 0);
                sp = 0;
                break;
            case 4:    // vmoveto
                if (sp >= 1) moveTo(0, st[sp - 1]);
                sp = 0;
                break;
            case 5:    // rlineto
                for (; i + 1 < sp; i += 2) lineTo(st[i], st[i + 1]);
                sp = 0;
                break;
            case 6: case 7: {   // hlineto / vlineto (alternating)
                bool h = b0 == 6;
                for (; i < sp; ++i, h = !h) lineTo(h ? st[i] : 0, h ? 0 : st[i]);
                sp = 0;
                break;
            }
            case 8:    // rrcurveto
                for (; i + 5 < sp; i += 6) curveTo(st[i], st[i + 1], st[i + 2], st[i + 3], st[i + 4], st[i + 5]);
                sp = 0;
                break;
            case 24:   // rcurveline
                for (; i + 5 < sp - 2; i += 6) curveTo(st[i], st[i + 1], st[i + 2], st[i + 3], st[i + 4], st[i + 5]);
                if (i + 1 < sp) lineTo(st[i], st[i + 1]);
                sp = 0;
                break;
            case 25:   // rlinecurve
                for (; i + 1 < sp - 6; i += 2) lineTo(st[i], st[i + 1]);
                if (i + 5 < sp) curveTo(st[i], st[i + 1], st[i + 2], st[i + 3], st[i + 4], st[i + 5]);
                sp = 0;
                break;
            case 26: {  // vvcurveto
                float dx1 = 0;
                if (sp & 1) dx1 = st[i++];
                for (; i + 3 < sp; i += 4, dx1 = 0) curveTo(dx1, st[i], st[i + 1], st[i + 2], 0, st[i + 3]);
                sp = 0;
                break;
            }
            case 27: {  // hhcurveto
                float dy1 = 0;
                if (sp & 1) dy1 = st[i++];
                for (; i + 3 < sp; i += 4, dy1 = 0) curveTo(st[i], dy1, st[i + 1], st[i + 2], st[i + 3], 0);
                sp = 0;
                break;
            }
            case 30: case 31: {   // vhcurveto / hvcurveto (alternating; the last may have a 5th argument)
                bool h = b0 == 31;
                for (; i + 3 < sp; i += 4, h = !h) {
                    const float last = (sp - i == 5) ? st[i + 4] : 0;
                    if (h) curveTo(st[i], 0, st[i + 1], st[i + 2], last, st[i + 3]);
                    else curveTo(0, st[i], st[i + 1], st[i + 2], st[i + 3], last);
                }
                sp = 0;
                break;
            }
            case 10: case 29: {   // callsubr / callgsubr
                if (sp < 1) break;
                const Index& ix = b0 == 10 ? local : gsubr_;
                const int k = int(st[--sp]) + bias(ix.count);
                uint32_t s0, s1;
                if (indexItem(ix, k, s0, s1))
                    run(s0, s1, depth + 1);
                break;
            }
            case 11:   // return
                return;
            case 14:   // endchar
                close();
                done = true;
                return;
            case 12: {
                const int b1 = u8(data_, p++);
                if (b1 == 35 && sp >= 13) {   // flex
                    curveTo(st[0], st[1], st[2], st[3], st[4], st[5]);
                    curveTo(st[6], st[7], st[8], st[9], st[10], st[11]);
                } else if (b1 == 34 && sp >= 7) {   // hflex
                    curveTo(st[0], 0, st[1], st[2], st[3], 0);
                    curveTo(st[4], 0, st[5], -st[2], st[6], 0);
                } else if (b1 == 36 && sp >= 9) {   // hflex1
                    curveTo(st[0], st[1], st[2], st[3], st[4], 0);
                    curveTo(st[5], 0, st[6], st[7], st[8], -(st[1] + st[3] + st[7]));
                } else if (b1 == 37 && sp >= 11) {   // flex1
                    const float dx = st[0] + st[2] + st[4] + st[6] + st[8], dy = st[1] + st[3] + st[5] + st[7] + st[9];
                    curveTo(st[0], st[1], st[2], st[3], st[4], st[5]);
                    if (std::fabs(dx) > std::fabs(dy)) curveTo(st[6], st[7], st[8], st[9], st[10], -dy);
                    else curveTo(st[6], st[7], st[8], st[9], -dx, st[10]);
                }
                sp = 0;
                break;
            }
            default:
                sp = 0;
                break;
            }
        }
    };
    run(a, b, 0);
    close();
}

// the coverage accumulator: each line adds its signed area to the cells it crosses (and the rest of the row through
// the running sum); the filled glyph is |sum| clamped (non-zero winding for the overlaps fonts have)
const TtfFont::Bitmap& TtfFont::glyph(int gid, float scale) const
{
    const uint64_t key = uint64_t(uint32_t(gid)) | uint64_t(uint32_t(std::lround(scale * 65536.0f))) << 32;
    std::lock_guard<std::mutex> lk(mu_);
    auto it = cache_.find(key);
    if (it != cache_.end())
        return it->second;
    Bitmap bm;
    Path path;
    outline(gid, path, 0.2f / std::max(scale, 1e-4f));
    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
    for (const auto& c : path)
        for (const Pt& p : c) {
            x0 = std::min(x0, p.x * scale), x1 = std::max(x1, p.x * scale);
            y0 = std::min(y0, -p.y * scale), y1 = std::max(y1, -p.y * scale);
        }
    if (!path.empty() && x1 > x0 && y1 > y0) {
        bm.x0 = int(std::floor(x0)) - 1;
        bm.y0 = int(std::floor(y0)) - 1;
        bm.w = int(std::ceil(x1)) - bm.x0 + 2;
        bm.h = int(std::ceil(y1)) - bm.y0 + 2;
        const int W = bm.w, H = bm.h;
        std::vector<float> acc(size_t(W) * size_t(H) + size_t(W) + 4, 0.0f);
        auto line = [&](float ax, float ay, float bx, float by) {
            if (ay == by) return;
            float dir = 1;
            if (ay > by) { std::swap(ax, bx); std::swap(ay, by); dir = -1; }
            const float dxdy = (bx - ax) / (by - ay);
            float x = ax;
            int ys = int(ay);
            if (ay < 0) { x -= ay * dxdy; ys = 0; }
            const int ye = std::min(H, int(std::ceil(by)));
            for (int yy = ys; yy < ye; ++yy) {
                const size_t row = size_t(yy) * size_t(W);
                const float dy = std::min(float(yy + 1), by) - std::max(float(yy), ay);
                const float xn = x + dxdy * dy, d = dy * dir;
                float xa = x, xb = xn;
                if (xa > xb) std::swap(xa, xb);
                const float xaf = std::floor(xa);
                const int xai = std::max(0, int(xaf));
                const float xbc = std::ceil(xb);
                const int xbi = int(xbc);
                if (xbi <= xai + 1) {
                    const float xm = 0.5f * (x + xn) - xaf;
                    acc[row + size_t(xai)] += d - d * xm;
                    acc[row + size_t(xai) + 1] += d * xm;
                } else {
                    const float s = 1.0f / (xb - xa);
                    const float xa0 = xa - xaf;
                    const float a0 = 0.5f * s * (1 - xa0) * (1 - xa0);
                    const float xb1 = xb - xbc + 1;
                    const float am = 0.5f * s * xb1 * xb1;
                    acc[row + size_t(xai)] += d * a0;
                    if (xbi == xai + 2)
                        acc[row + size_t(xai) + 1] += d * (1 - a0 - am);
                    else {
                        const float a1 = s * (1.5f - xa0);
                        acc[row + size_t(xai) + 1] += d * (a1 - a0);
                        for (int xi = xai + 2; xi < xbi - 1; ++xi)
                            acc[row + size_t(xi)] += d * s;
                        const float a2 = a1 + float(xbi - xai - 3) * s;
                        acc[row + size_t(xbi) - 1] += d * (1 - a2 - am);
                    }
                    acc[row + size_t(xbi)] += d * am;
                }
                x = xn;
            }
        };
        for (const auto& c : path)
            for (size_t i = 0; i < c.size(); ++i) {
                const Pt& p = c[i];
                const Pt& q = c[(i + 1) % c.size()];
                line(p.x * scale - float(bm.x0), -p.y * scale - float(bm.y0), q.x * scale - float(bm.x0), -q.y * scale - float(bm.y0));
            }
        bm.a.resize(size_t(W) * size_t(H));
        float sum = 0;
        for (size_t i = 0; i < bm.a.size(); ++i) {
            sum += acc[i];
            bm.a[i] = uint8_t(std::min(1.0f, std::fabs(sum)) * 255.0f + 0.5f);
        }
    }
    return cache_.emplace(key, std::move(bm)).first->second;
}

// ---- text ----
uint32_t utf8Next(const std::string& s, size_t& i)
{
    const uint8_t c = uint8_t(s[i]);
    int n = c < 0x80 ? 0 : (c >> 5) == 6 ? 1 : (c >> 4) == 14 ? 2 : (c >> 3) == 30 ? 3 : -1;
    if (n < 0 || i + size_t(n) >= s.size() + (n ? 0 : 1)) {
        ++i;
        return n == 0 ? c : 0xFFFD;
    }
    uint32_t cp = n == 0 ? c : n == 1 ? (c & 0x1F) : n == 2 ? (c & 0x0F) : (c & 0x07);
    for (int k = 1; k <= n; ++k) {
        const uint8_t b = uint8_t(s[i + size_t(k)]);
        if ((b >> 6) != 2) {
            ++i;
            return 0xFFFD;
        }
        cp = cp << 6 | (b & 0x3F);
    }
    i += size_t(n) + 1;
    return cp;
}

bool isCjk(uint32_t cp)
{
    return (cp >= 0x1100 && cp <= 0x11FF) || (cp >= 0x2E80 && cp <= 0x9FFF) || (cp >= 0xA960 && cp <= 0xA97F) ||
           (cp >= 0xAC00 && cp <= 0xD7FF) || (cp >= 0xF900 && cp <= 0xFAFF) || (cp >= 0xFF00 && cp <= 0xFFEF) ||
           (cp >= 0x20000 && cp <= 0x3FFFF);
}

namespace {
// no line starts with these (closing punctuation, small kana, the long vowel mark)
bool noBreakBefore(uint32_t cp)
{
    static const uint32_t k[] = {0x3001, 0x3002, 0xFF0C, 0xFF0E, 0x30FB, 0x30FC, 0x300D, 0x300F, 0xFF09, 0xFF01, 0xFF1F,
                                 0x2026, 0x3009, 0x300B, 0x3011, 0xFF3D, 0x3063, 0x30C3, 0x3083, 0x3085, 0x3087, 0x30E3,
                                 0x30E5, 0x30E7, '.', ',', '!', '?', ')', ':', ';', '%'};
    return std::find(std::begin(k), std::end(k), cp) != std::end(k);
}
} // namespace

float TtfText::width(const std::string& sIn, float px) const
{
    std::string vis;   // (rtl: shaped as drawn)
    const std::string& s = hasRtl(sIn) ? (vis = visual(sIn)) : sIn;
    float w = 0;
    for (size_t i = 0; i < s.size();) {
        int gid = 0;
        const TtfFont& f = pick(utf8Next(s, i), gid);
        w += float(f.advance(gid)) * px / float(f.unitsPerEm());
    }
    return w;
}

std::vector<std::string> TtfText::wrap(const std::string& s, float px, float maxW) const
{
    // units: a word with its trailing spaces, or one CJK character (with the punctuation after it); "\n": a break
    std::vector<std::string> units;
    std::string cur;
    bool curCjk = false;
    auto flush = [&]() {
        if (!cur.empty()) units.push_back(cur);
        cur.clear();
        curCjk = false;
    };
    for (size_t i = 0; i < s.size();) {
        const size_t at = i;
        const uint32_t cp = utf8Next(s, i);
        const std::string ch = s.substr(at, i - at);
        if (cp == '\n') { flush(); units.push_back("\n"); continue; }
        if (noBreakBefore(cp) && (!cur.empty() || !units.empty())) {
            if (cur.empty() && units.back() != "\n") units.back() += ch;
            else cur += ch;
            continue;
        }
        if (isCjk(cp)) { flush(); cur = ch; curCjk = true; continue; }
        if (curCjk) flush();
        cur += ch;
        if (cp == ' ') flush();
    }
    flush();
    std::vector<std::string> lines;
    std::string line;
    auto trimEnd = [](std::string t) { while (!t.empty() && t.back() == ' ') t.pop_back(); return t; };
    for (const std::string& u : units) {
        if (u == "\n") { lines.push_back(trimEnd(line)); line.clear(); continue; }
        if (!line.empty() && width(trimEnd(line + u), px) > maxW) {
            lines.push_back(trimEnd(line));
            line.clear();
        }
        if (line.empty() && width(trimEnd(u), px) > maxW) {   // a unit too wide alone: by character
            for (size_t i = 0; i < u.size();) {
                const size_t at = i;
                utf8Next(u, i);
                const std::string ch = u.substr(at, i - at);
                if (!line.empty() && width(line + ch, px) > maxW) { lines.push_back(line); line.clear(); }
                line += ch;
            }
            continue;
        }
        line += u;
    }
    if (!line.empty() || lines.empty())
        lines.push_back(trimEnd(line));
    return lines;
}

} // namespace hotd::render
