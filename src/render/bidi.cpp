// lang (rtl): the bidirectional algorithm and Arabic shaping for one line of text (bidi.h)
#include "render/bidi.h"

#include "render/ttf.h"

#include <algorithm>
#include <atomic>
#include <utility>

namespace hotd::render {

namespace {

std::atomic<bool> gBaseRtl{false};

// ---- the bidirectional character types (UAX #9 table 4), the ranges UI texts meet ----
enum T : uint8_t { L, R, AL, EN, ES, ET, AN, CS, NSM, BN, B, S, WS, ON };

bool in(uint32_t c, uint32_t a, uint32_t b) { return c >= a && c <= b; }

bool isNsm(uint32_t c)
{
    return in(c, 0x0300, 0x036F) || in(c, 0x0483, 0x0489) || in(c, 0x0591, 0x05BD) || c == 0x05BF ||
           in(c, 0x05C1, 0x05C2) || in(c, 0x05C4, 0x05C5) || c == 0x05C7 || in(c, 0x0610, 0x061A) ||
           in(c, 0x064B, 0x065F) || c == 0x0670 || in(c, 0x06D6, 0x06DC) || in(c, 0x06DF, 0x06E4) ||
           in(c, 0x06E7, 0x06E8) || in(c, 0x06EA, 0x06ED) || c == 0x0711 || in(c, 0x0730, 0x074A) ||
           in(c, 0x07A6, 0x07B0) || in(c, 0x07EB, 0x07F3) || in(c, 0x0816, 0x082D) || in(c, 0x0859, 0x085B) ||
           in(c, 0x08D3, 0x08FF) || in(c, 0x0900, 0x0902) || c == 0x093A || c == 0x093C || in(c, 0x0941, 0x0948) ||
           c == 0x094D || in(c, 0x0951, 0x0957) || in(c, 0x0962, 0x0963) || c == 0x0E31 || in(c, 0x0E34, 0x0E3A) ||
           in(c, 0x0E47, 0x0E4E) || in(c, 0x20D0, 0x20FF) || in(c, 0xFE00, 0xFE0F) || in(c, 0xFE20, 0xFE2F) ||
           c == 0xFB1E;
}

T bidiType(uint32_t c)
{
    if (c < 0x80) {
        if (c >= '0' && c <= '9') return EN;
        if (c >= 'A' && c <= 'Z') return L;
        if (c >= 'a' && c <= 'z') return L;
        switch (c) {
        case ' ': case 0x0C: return WS;
        case '\t': case 0x0B: case 0x1F: return S;
        case '\n': case '\r': case 0x1C: case 0x1D: case 0x1E: return B;
        case '+': case '-': return ES;
        case '#': case '$': case '%': return ET;
        case ',': case '.': case '/': case ':': return CS;
        default: break;
        }
        if (c < 0x20 || c == 0x7F) return BN;
        return ON;
    }
    if (c < 0x100) {
        if (c == 0x85) return B;
        if (c < 0xA0) return BN;
        if (c == 0xA0) return CS;
        if (c == 0xAD) return BN;
        if (in(c, 0xA2, 0xA5) || c == 0xB0 || c == 0xB1) return ET;
        if (c == 0xB2 || c == 0xB3 || c == 0xB9) return EN;
        if (c == 0xAA || c == 0xB5 || c == 0xBA || c >= 0xC0) return c == 0xD7 || c == 0xF7 ? ON : L;
        return ON;
    }
    if (isNsm(c)) return NSM;
    if (in(c, 0x0590, 0x05FF) || in(c, 0x07C0, 0x085F) || in(c, 0xFB1D, 0xFB4F) || in(c, 0x10800, 0x10FFF) ||
        in(c, 0x1E800, 0x1EDFF) || in(c, 0x1EF00, 0x1EFFF))
        return R;
    if (in(c, 0x0600, 0x07BF) || in(c, 0x0860, 0x08FF) || in(c, 0xFB50, 0xFDFF) || in(c, 0xFE70, 0xFEFE) ||
        in(c, 0x1EE00, 0x1EEFF)) {
        if (in(c, 0x0660, 0x0669) || c == 0x066B || c == 0x066C || in(c, 0x0600, 0x0605) || c == 0x06DD) return AN;
        if (in(c, 0x06F0, 0x06F9)) return EN;
        if (c == 0x066A) return ET;
        if (c == 0x060C) return CS;
        if (c == 0x060E || c == 0x060F || c == 0x06DE || c == 0x06E9 || in(c, 0xFD3E, 0xFD3F)) return ON;
        return AL;
    }
    if (in(c, 0x2000, 0x200A) || c == 0x2028 || c == 0x205F || c == 0x3000) return WS;
    if (c == 0x2029) return B;
    if (in(c, 0x200B, 0x200D) || in(c, 0x2060, 0x206F) || c == 0xFEFF) return BN;
    if (c == 0x200E) return L;
    if (c == 0x200F) return R;
    if (c == 0x202F || c == 0x2044) return CS;
    if (in(c, 0x2030, 0x2034) || in(c, 0x20A0, 0x20CF) || c == 0x2213) return ET;
    if (c == 0x2212) return ES;
    if (in(c, 0x2070, 0x2079) && c != 0x2071) return EN;
    if (in(c, 0x2080, 0x2089)) return EN;
    if (in(c, 0xFF10, 0xFF19)) return EN;
    if (c == 0xFF0B || c == 0xFF0D) return ES;
    if (in(c, 0xFF03, 0xFF05) || c == 0xFFE0 || c == 0xFFE1 || c == 0xFFE5 || c == 0xFFE6) return ET;
    if (c == 0xFF0C || c == 0xFF0E || c == 0xFF0F || c == 0xFF1A) return CS;
    if (in(c, 0x2010, 0x2027) || in(c, 0x2035, 0x205E) || in(c, 0x2100, 0x2101) || in(c, 0x2190, 0x2BFF) ||
        in(c, 0x2E00, 0x2E7F) || in(c, 0x3001, 0x3004) || in(c, 0x3008, 0x3020) || c == 0x30FB ||
        in(c, 0xFE50, 0xFE6F) || in(c, 0xFF01, 0xFF20) || in(c, 0xFF3B, 0xFF40) || in(c, 0xFF5B, 0xFF65) ||
        in(c, 0xFFF9, 0xFFFD))
        return ON;
    return L;
}

// the paired brackets (BidiBrackets.txt, the ones UI texts use): opening -> closing
uint32_t closingOf(uint32_t c)
{
    switch (c) {
    case '(': return ')';
    case '[': return ']';
    case '{': return '}';
    case 0x2045: return 0x2046;
    case 0x207D: return 0x207E;
    case 0x208D: return 0x208E;
    case 0x2329: return 0x232A;
    case 0x3008: return 0x3009;
    case 0x300A: return 0x300B;
    case 0x300C: return 0x300D;
    case 0x300E: return 0x300F;
    case 0x3010: return 0x3011;
    case 0x3014: return 0x3015;
    case 0x3016: return 0x3017;
    case 0x3018: return 0x3019;
    case 0x301A: return 0x301B;
    case 0xFF08: return 0xFF09;
    case 0xFF3B: return 0xFF3D;
    case 0xFF5B: return 0xFF5D;
    case 0xFF5F: return 0xFF60;
    case 0xFF62: return 0xFF63;
    default: return 0;
    }
}
bool isClosing(uint32_t c)
{
    switch (c) {
    case ')': case ']': case '}': case 0x2046: case 0x207E: case 0x208E: case 0x232A: case 0x3009: case 0x300B:
    case 0x300D: case 0x300F: case 0x3011: case 0x3015: case 0x3017: case 0x3019: case 0x301B: case 0xFF09:
    case 0xFF3D: case 0xFF5D: case 0xFF60: case 0xFF63:
        return true;
    default: return false;
    }
}

// Bidi_Mirroring_Glyph (the pairs UI texts use)
uint32_t mirrorOf(uint32_t c)
{
    static const uint32_t pairs[][2] = {
        {'(', ')'}, {'<', '>'}, {'[', ']'}, {'{', '}'}, {0xAB, 0xBB}, {0x2039, 0x203A}, {0x2045, 0x2046},
        {0x207D, 0x207E}, {0x208D, 0x208E}, {0x2264, 0x2265}, {0x2329, 0x232A}, {0x3008, 0x3009}, {0x300A, 0x300B},
        {0x300C, 0x300D}, {0x300E, 0x300F}, {0x3010, 0x3011}, {0x3014, 0x3015}, {0x3016, 0x3017}, {0x3018, 0x3019},
        {0x301A, 0x301B}, {0xFF08, 0xFF09}, {0xFF1C, 0xFF1E}, {0xFF3B, 0xFF3D}, {0xFF5B, 0xFF5D}, {0xFF5F, 0xFF60},
        {0xFF62, 0xFF63}};
    for (const auto& p : pairs) {
        if (c == p[0]) return p[1];
        if (c == p[1]) return p[0];
    }
    return c;
}

// ---- Arabic shaping ----
// the presentation forms of a letter: isolated, final, initial, medial (0: none); n: how many it has (4: the letter
// joins on both sides, 2: on the right only)
struct Forms { uint32_t base, first; int n; };
const Forms kForms[] = {
    // Arabic Presentation Forms-B (FE80..FEF4), in the order of the block
    {0x0621, 0xFE80, 1}, {0x0622, 0xFE81, 2}, {0x0623, 0xFE83, 2}, {0x0624, 0xFE85, 2}, {0x0625, 0xFE87, 2},
    {0x0626, 0xFE89, 4}, {0x0627, 0xFE8D, 2}, {0x0628, 0xFE8F, 4}, {0x0629, 0xFE93, 2}, {0x062A, 0xFE95, 4},
    {0x062B, 0xFE99, 4}, {0x062C, 0xFE9D, 4}, {0x062D, 0xFEA1, 4}, {0x062E, 0xFEA5, 4}, {0x062F, 0xFEA9, 2},
    {0x0630, 0xFEAB, 2}, {0x0631, 0xFEAD, 2}, {0x0632, 0xFEAF, 2}, {0x0633, 0xFEB1, 4}, {0x0634, 0xFEB5, 4},
    {0x0635, 0xFEB9, 4}, {0x0636, 0xFEBD, 4}, {0x0637, 0xFEC1, 4}, {0x0638, 0xFEC5, 4}, {0x0639, 0xFEC9, 4},
    {0x063A, 0xFECD, 4}, {0x0641, 0xFED1, 4}, {0x0642, 0xFED5, 4}, {0x0643, 0xFED9, 4}, {0x0644, 0xFEDD, 4},
    {0x0645, 0xFEE1, 4}, {0x0646, 0xFEE5, 4}, {0x0647, 0xFEE9, 4}, {0x0648, 0xFEED, 2}, {0x0649, 0xFEEF, 2},
    {0x064A, 0xFEF1, 4},
    // Arabic Presentation Forms-A (Persian, Urdu ... letters)
    {0x0671, 0xFB50, 2}, {0x067B, 0xFB52, 4}, {0x067E, 0xFB56, 4}, {0x0680, 0xFB5A, 4}, {0x067A, 0xFB5E, 4},
    {0x067F, 0xFB62, 4}, {0x0679, 0xFB66, 4}, {0x06A4, 0xFB6A, 4}, {0x06A6, 0xFB6E, 4}, {0x0684, 0xFB72, 4},
    {0x0683, 0xFB76, 4}, {0x0686, 0xFB7A, 4}, {0x0687, 0xFB7E, 4}, {0x068D, 0xFB82, 2}, {0x068C, 0xFB84, 2},
    {0x068E, 0xFB86, 2}, {0x0688, 0xFB88, 2}, {0x0698, 0xFB8A, 2}, {0x0691, 0xFB8C, 2}, {0x06A9, 0xFB8E, 4},
    {0x06AF, 0xFB92, 4}, {0x06B3, 0xFB96, 4}, {0x06B1, 0xFB9A, 4}, {0x06BA, 0xFB9E, 2}, {0x06BB, 0xFBA0, 4},
    {0x06C0, 0xFBA4, 2}, {0x06C1, 0xFBA6, 4}, {0x06BE, 0xFBAA, 4}, {0x06D2, 0xFBAE, 2}, {0x06D3, 0xFBB0, 2},
    {0x06AD, 0xFBD3, 4}, {0x06C7, 0xFBD7, 2}, {0x06C6, 0xFBD9, 2}, {0x06C8, 0xFBDB, 2}, {0x06CB, 0xFBDE, 2},
    {0x06C5, 0xFBE0, 2}, {0x06C9, 0xFBE2, 2}, {0x06D0, 0xFBE4, 4}, {0x06CC, 0xFBFC, 4},
};

const Forms* formsOf(uint32_t c)
{
    if (c < 0x0621 || c > 0x06D3) return nullptr;
    for (const Forms& f : kForms)
        if (f.base == c) return &f;
    return nullptr;
}

// joining types: 'D' both sides, 'R' right only, 'C' join causing (tatweel, ZWJ), 'T' transparent (marks), 'U' none
char joiningType(uint32_t c)
{
    if (c == 0x0640 || c == 0x200D) return 'C';
    if (isNsm(c) && (in(c, 0x0610, 0x061A) || in(c, 0x064B, 0x065F) || c == 0x0670 || in(c, 0x06D6, 0x06ED)))
        return 'T';
    if (const Forms* f = formsOf(c)) return f->n == 4 ? 'D' : f->n == 2 ? 'R' : 'U';
    return 'U';
}

} // namespace

bool isRtlChar(uint32_t cp)
{
    const T t = bidiType(cp);
    return t == R || t == AL;
}

bool hasRtl(const std::string& s)
{
    for (size_t i = 0; i < s.size();) {
        if (uint8_t(s[i]) < 0x80) { ++i; continue; }   // (ASCII: never)
        if (isRtlChar(utf8Next(s, i))) return true;
    }
    return false;
}

void setBaseRtl(bool rtl) { gBaseRtl = rtl; }
bool baseRtl() { return gBaseRtl; }

std::vector<uint32_t> shapeArabic(const std::vector<uint32_t>& in)
{
    const size_t n = in.size();
    std::vector<char> jt(n);
    bool any = false;
    for (size_t i = 0; i < n; ++i) {
        jt[i] = joiningType(in[i]);
        any = any || jt[i] == 'D' || jt[i] == 'R';
    }
    if (!any) return in;
    // the joining neighbours (marks skipped)
    auto prevJ = [&](size_t i) -> char {
        while (i-- > 0)
            if (jt[i] != 'T') return jt[i];
        return 'U';
    };
    auto nextIdx = [&](size_t i) -> size_t {
        for (++i; i < n; ++i)
            if (jt[i] != 'T') return i;
        return n;
    };
    std::vector<uint32_t> out;
    out.reserve(n);
    std::vector<bool> skip(n, false);
    for (size_t i = 0; i < n; ++i) {
        if (skip[i]) continue;
        const uint32_t c = in[i];
        const char t = jt[i];
        if (t != 'D' && t != 'R') { out.push_back(c); continue; }
        const char pj = prevJ(i);
        const bool joinPrev = pj == 'D' || pj == 'C';
        const size_t ni = nextIdx(i);
        // lam + alef: one ligature (isolated / final), the marks between them after it
        if (c == 0x0644 && ni < n) {
            const uint32_t a = in[ni];
            const uint32_t lig = a == 0x0622 ? 0xFEF5 : a == 0x0623 ? 0xFEF7 : a == 0x0625 ? 0xFEF9 : a == 0x0627 ? 0xFEFB : 0;
            if (lig) {
                out.push_back(lig + (joinPrev ? 1 : 0));
                skip[ni] = true;
                continue;
            }
        }
        const char nj = ni < n ? jt[ni] : 'U';
        const bool joinNext = t == 'D' && (nj == 'D' || nj == 'R' || nj == 'C');
        const Forms* f = formsOf(c);
        int form = joinPrev && joinNext ? 3 : joinNext ? 2 : joinPrev ? 1 : 0;   // medi, init, fina, isol
        if (form >= f->n) form = joinPrev ? 1 : 0;
        if (form >= f->n) form = 0;
        out.push_back(f->first + uint32_t(form));
    }
    return out;
}

std::vector<uint32_t> visualLine(const std::vector<uint32_t>& cps, int base)
{
    const size_t n = cps.size();
    if (n == 0) return cps;
    std::vector<T> orig(n), t(n);
    for (size_t i = 0; i < n; ++i) orig[i] = t[i] = bidiType(cps[i]);
    // P2 / P3: the paragraph level
    int p = base > 0 ? 1 : 0;
    if (base < 0)
        for (size_t i = 0; i < n; ++i)
            if (t[i] == L || t[i] == R || t[i] == AL) { p = t[i] == L ? 0 : 1; break; }
    const T e = p ? R : L;   // the embedding direction (also sos / eos)
    // W1: a mark takes the type before it
    for (size_t i = 0; i < n; ++i)
        if (t[i] == NSM) t[i] = i ? (t[i - 1] == BN ? e : t[i - 1]) : e;
    // W2: European digits after Arabic letters are Arabic numbers
    {
        T last = e;
        for (size_t i = 0; i < n; ++i) {
            if (t[i] == L || t[i] == R || t[i] == AL) last = t[i];
            else if (t[i] == EN && last == AL) t[i] = AN;
        }
    }
    // W3
    for (T& x : t)
        if (x == AL) x = R;
    // W4: one separator between two numbers of a kind
    for (size_t i = 1; i + 1 < n; ++i) {
        if (t[i] == ES && t[i - 1] == EN && t[i + 1] == EN) t[i] = EN;
        else if (t[i] == CS && t[i - 1] == EN && t[i + 1] == EN) t[i] = EN;
        else if (t[i] == CS && t[i - 1] == AN && t[i + 1] == AN) t[i] = AN;
    }
    // W5: terminators next to European digits
    for (size_t i = 0; i < n;) {
        if (t[i] != ET) { ++i; continue; }
        size_t j = i;
        while (j < n && (t[j] == ET || t[j] == BN)) ++j;
        if ((i > 0 && t[i - 1] == EN) || (j < n && t[j] == EN))
            for (size_t k = i; k < j; ++k) t[k] = EN;
        i = j;
    }
    // W6
    for (T& x : t)
        if (x == ES || x == ET || x == CS) x = ON;
    // W7: European digits after Latin letters are Latin
    {
        T last = e;
        for (size_t i = 0; i < n; ++i) {
            if (t[i] == L || t[i] == R) last = t[i];
            else if (t[i] == EN && last == L) t[i] = L;
        }
    }
    // N0: bracket pairs
    {
        std::vector<std::pair<size_t, size_t>> pairs;
        std::vector<std::pair<uint32_t, size_t>> stack;   // (the closing it waits for, where it opened)
        for (size_t i = 0; i < n; ++i) {
            if (t[i] != ON) continue;
            if (const uint32_t cl = closingOf(cps[i])) {
                if (stack.size() >= 63) break;
                stack.push_back({cl, i});
            } else if (isClosing(cps[i])) {
                for (size_t k = stack.size(); k-- > 0;)
                    if (stack[k].first == cps[i]) {
                        pairs.push_back({stack[k].second, i});
                        stack.resize(k);
                        break;
                    }
            }
        }
        std::sort(pairs.begin(), pairs.end());
        auto strongOf = [&](T x) -> int { return x == L ? 0 : (x == R || x == EN || x == AN) ? 1 : -1; };
        const int ed = p ? 1 : 0;
        for (const auto& pr : pairs) {
            bool foundE = false, foundO = false;
            for (size_t k = pr.first + 1; k < pr.second; ++k) {
                const int s = strongOf(t[k]);
                if (s == ed) foundE = true;
                else if (s == 1 - ed) foundO = true;
            }
            int dir = -1;
            if (foundE) dir = ed;
            else if (foundO) {
                int before = ed;   // sos
                for (size_t k = pr.first; k-- > 0;)
                    if (strongOf(t[k]) >= 0) { before = strongOf(t[k]); break; }
                dir = before == 1 - ed ? 1 - ed : ed;
            }
            if (dir < 0) continue;
            const T d = dir ? R : L;
            for (const size_t at : {pr.first, pr.second}) {
                t[at] = d;
                for (size_t k = at + 1; k < n && orig[k] == NSM; ++k) t[k] = d;   // (marks on the bracket)
            }
        }
    }
    // N1 / N2: neutrals between two strong sides of one direction take it, else the embedding direction
    auto isNeutral = [](T x) { return x == B || x == S || x == WS || x == ON || x == BN; };
    auto asStrong = [](T x) { return x == L ? L : R; };   // (EN / AN count as R)
    for (size_t i = 0; i < n;) {
        if (!isNeutral(t[i])) { ++i; continue; }
        size_t j = i;
        while (j < n && isNeutral(t[j])) ++j;
        const T before = i ? asStrong(t[i - 1]) : e, after = j < n ? asStrong(t[j]) : e;
        const T d = before == after ? before : e;
        for (size_t k = i; k < j; ++k) t[k] = d;
        i = j;
    }
    // I1 / I2: the levels
    std::vector<int> lv(n);
    for (size_t i = 0; i < n; ++i) {
        if (p == 0) lv[i] = t[i] == R ? 1 : (t[i] == AN || t[i] == EN) ? 2 : 0;
        else lv[i] = (t[i] == L || t[i] == EN || t[i] == AN) ? 2 : 1;
    }
    // L1: separators, and spaces before them or at the end, at the paragraph level
    {
        bool tail = true;
        for (size_t i = n; i-- > 0;) {
            const T o = orig[i];
            if (o == S || o == B) { lv[i] = p; tail = true; }
            else if (o == WS || o == BN) { if (tail) lv[i] = p; }
            else tail = false;
        }
    }
    // L2: reverse, from the highest level down to the lowest odd one
    std::vector<uint32_t> out(cps);
    int hi = 0, loOdd = 99;
    for (const int l : lv) {
        hi = std::max(hi, l);
        if (l & 1) loOdd = std::min(loOdd, l);
    }
    // L4 first (on the logical positions): mirrored characters at odd levels
    for (size_t i = 0; i < n; ++i)
        if (lv[i] & 1) out[i] = mirrorOf(out[i]);
    for (int k = hi; k >= loOdd && k > 0; --k)
        for (size_t i = 0; i < n;) {
            if (lv[i] < k) { ++i; continue; }
            size_t j = i;
            while (j < n && lv[j] >= k) ++j;
            std::reverse(out.begin() + std::ptrdiff_t(i), out.begin() + std::ptrdiff_t(j));
            std::reverse(lv.begin() + std::ptrdiff_t(i), lv.begin() + std::ptrdiff_t(j));
            i = j;
        }
    return out;
}

namespace {
void putUtf8(std::string& o, uint32_t c)
{
    if (c < 0x80) o += char(c);
    else if (c < 0x800) o += char(0xC0 | (c >> 6)), o += char(0x80 | (c & 0x3F));
    else if (c < 0x10000) o += char(0xE0 | (c >> 12)), o += char(0x80 | ((c >> 6) & 0x3F)), o += char(0x80 | (c & 0x3F));
    else
        o += char(0xF0 | (c >> 18)), o += char(0x80 | ((c >> 12) & 0x3F)), o += char(0x80 | ((c >> 6) & 0x3F)),
            o += char(0x80 | (c & 0x3F));
}
} // namespace

std::string visual(const std::string& s)
{
    if (!hasRtl(s)) return s;
    std::string out;
    out.reserve(s.size());
    std::vector<uint32_t> line;
    auto flush = [&]() {
        bool rtl = false;
        for (const uint32_t c : line) rtl = rtl || isRtlChar(c);
        if (rtl) line = visualLine(shapeArabic(line), baseRtl() ? 1 : -1);
        for (const uint32_t c : line)
            if (c != 0x200E && c != 0x200F)   // (relmenu: the direction marks steer the order only; no font draws them)
                putUtf8(out, c);
        line.clear();
    };
    for (size_t i = 0; i < s.size();) {
        const uint32_t c = utf8Next(s, i);
        if (c == '\n') { flush(); out += '\n'; continue; }
        line.push_back(c);
    }
    flush();
    return out;
}

} // namespace hotd::render
