// lang (translations): a small TrueType / OpenType font reader and glyph rasterizer of our own (no library on disk:
// stb_truetype is not in private\deps). It reads a .ttf / .otf / .ttc's first font: cmap (formats 4 and 12), hmtx,
// TrueType outlines (glyf / loca, composite glyphs) and CFF outlines (Type 2 charstrings, CID-keyed fonts such as
// Noto Sans CJK: FDArray / FDSelect, local and global subroutines, flex). Glyphs are flattened to lines and filled
// with an exact area-coverage accumulator (non-zero winding, anti-aliased), cached per size. No hinting, no kerning,
// no shaping of its own (Hangul, kana and CJK need none; Latin reads fine without kerning at menu sizes; Arabic and
// Hebrew: a line is put in display order and shaped into presentation forms first, bidi.h).
#pragma once

#include "render/bidi.h"

#include <cmath>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace hotd::render {

class TtfFont {
public:
    bool load(const std::string& path, std::string& err);
    bool loadMemory(std::vector<uint8_t> data, std::string& err);
    bool loaded() const { return !data_.empty(); }
    const std::string& file() const { return file_; }

    int glyphIndex(uint32_t cp) const;   // 0: none (.notdef)
    // the font's vertical metrics in font units (hhea; OS/2 typo when hhea is empty)
    int unitsPerEm() const { return upem_; }
    int ascent() const { return ascent_; }
    int descent() const { return descent_; }   // negative
    int capHeight() const { return capH_; }    // OS/2 sCapHeight, else the height of 'H'
    int advance(int gid) const;                // font units

    // a glyph's coverage (one byte a pixel) at scale (pixels a font unit); (x0, y0): its top left relative to the pen
    // on the baseline (y down)
    struct Bitmap { int w = 0, h = 0, x0 = 0, y0 = 0; std::vector<uint8_t> a; };
    const Bitmap& glyph(int gid, float scale) const;   // cached

private:
    struct Pt { float x, y; };
    using Path = std::vector<std::vector<Pt>>;   // closed polylines, font units, y up
    void outline(int gid, Path& out, float tol) const;
    void glyfOutline(int gid, Path& out, float tol, int depth, const float m[6]) const;
    void cffOutline(int gid, Path& out, float tol) const;

    std::vector<uint8_t> data_;
    std::string file_;
    uint32_t cmap_ = 0, glyf_ = 0, loca_ = 0, hmtx_ = 0, cff_ = 0;
    int cmapFormat_ = 0, locFormat_ = 0, numGlyphs_ = 0, numHMetrics_ = 0;
    int upem_ = 1000, ascent_ = 800, descent_ = -200, capH_ = 700;
    // CFF: the charstrings, the global subrs, per font DICT the local subrs; FDSelect (CID fonts)
    struct Index { uint32_t off = 0; int count = 0; int offSize = 0; uint32_t data = 0; };
    Index cs_, gsubr_;
    std::vector<Index> lsubr_;   // one per FD (a non-CID font: one)
    uint32_t fdSelect_ = 0;
    bool cid_ = false;
    int fdFor(int gid) const;
    bool indexItem(const Index& ix, int i, uint32_t& start, uint32_t& end) const;
    Index readIndex(uint32_t at) const;
    mutable std::map<uint64_t, Bitmap> cache_;   // (map: its entries stay put; the menu draws on workers: mu_)
    mutable std::mutex mu_;
};

// text through a TtfFont: UTF-8 decoding, measuring, wrapping (Latin at spaces, CJK between any two characters) and
// drawing as coverage (the caller colours / outlines it)
uint32_t utf8Next(const std::string& s, size_t& i);    // the code point at i (i moves past it); bad bytes: U+FFFD
bool isCjk(uint32_t cp);                                // Hangul, kana, CJK ideographs, full-width forms: break anywhere

class TtfText {
public:
    // the fonts in order: a character missing from the first is taken from the next that has it (lang.font's list)
    explicit TtfText(std::vector<const TtfFont*> fonts) : fonts_(std::move(fonts)) {}
    bool empty() const { return fonts_.empty() || !fonts_[0]->loaded(); }
    const TtfFont& primary() const { return *fonts_[0]; }
    // the font with the character (the first one when none has it) and its glyph
    const TtfFont& pick(uint32_t cp, int& gid) const
    {
        for (const TtfFont* f : fonts_)
            if ((gid = f->glyphIndex(cp)) != 0)
                return *f;
        gid = 0;
        return *fonts_[0];
    }
    // px: the pixel size of the font's em; the width of s in pixels
    float width(const std::string& s, float px) const;
    // the lines of s that fit in maxW pixels (explicit \n breaks too)
    std::vector<std::string> wrap(const std::string& s, float px, float maxW) const;
    // calls put(x, y, coverage 0..1) for each pixel of s drawn with its baseline at (x, baseline), pixels
    template <class Put>
    void draw(const std::string& sIn, float px, float x, float baseline, Put&& put) const
    {
        std::string vis;   // (rtl: a line with right-to-left letters drawn in display order, Arabic shaped: bidi.h)
        const std::string& s = hasRtl(sIn) ? (vis = visual(sIn)) : sIn;
        for (size_t i = 0; i < s.size();) {
            const uint32_t cp = utf8Next(s, i);
            int gid = 0;
            const TtfFont& f = pick(cp, gid);
            const float sc = px / float(f.unitsPerEm());
            const TtfFont::Bitmap& b = f.glyph(gid, sc);
            const int ox = int(std::floor(x)) + b.x0, oy = int(std::lround(baseline)) + b.y0;
            for (int j = 0; j < b.h; ++j)
                for (int k = 0; k < b.w; ++k)
                    if (const uint8_t a = b.a[size_t(j) * size_t(b.w) + size_t(k)])
                        put(ox + k, oy + j, a / 255.0f);
            x += float(f.advance(gid)) * sc;
        }
    }

private:
    std::vector<const TtfFont*> fonts_;
};

} // namespace hotd::render
