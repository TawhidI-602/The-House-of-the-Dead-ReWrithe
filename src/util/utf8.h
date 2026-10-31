// UTF-8 safe text cutting (misc18): a label, name or path is cut a whole letter (code point) at a time, never in the
// middle of a Korean / Japanese / Arabic / Hebrew letter (half a letter drew as a box or a wrong glyph)
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace hotd::util {

inline bool utf8Cont(char c) { return (uint8_t(c) & 0xC0) == 0x80; }   // a continuation byte (10xxxxxx)

// whole letters only (no lead byte without its continuation bytes, no stray continuation byte): --lang-check's test
inline bool utf8Valid(const std::string& s)
{
    for (size_t i = 0; i < s.size();) {
        const uint8_t c = uint8_t(s[i]);
        const size_t n = c < 0x80 ? 1 : (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3 : (c & 0xF8) == 0xF0 ? 4 : 0;
        if (n == 0 || i + n > s.size()) return false;
        for (size_t k = 1; k < n; ++k)
            if (!utf8Cont(s[i + k])) return false;
        i += n;
    }
    return true;
}

// the last letter removed (its continuation bytes and its lead byte)
inline void utf8PopBack(std::string& s)
{
    while (!s.empty() && utf8Cont(s.back())) s.pop_back();
    if (!s.empty()) s.pop_back();
}

// the first at most n bytes, ending at a letter's end (s.substr(0, n) without half a letter)
inline std::string utf8Head(const std::string& s, size_t n)
{
    if (s.size() <= n) return s;
    while (n > 0 && utf8Cont(s[n])) --n;
    return s.substr(0, n);
}

// the last at most n bytes, starting at a letter's start
inline std::string utf8Tail(const std::string& s, size_t n)
{
    if (s.size() <= n) return s;
    size_t at = s.size() - n;
    while (at < s.size() && utf8Cont(s[at])) ++at;
    return s.substr(at);
}

// s cut a letter at a time (trailing spaces dropped) and `tail` ("..") added until fits(text) says yes or nothing is
// left; s unchanged when it fits as it is. Always ends.
template <class Fits>
std::string utf8Fit(const std::string& s, const char* tail, Fits&& fits)
{
    if (s.empty() || fits(s)) return s;
    std::string base = s;
    do {
        utf8PopBack(base);
        while (!base.empty() && base.back() == ' ') base.pop_back();
    } while (!base.empty() && !fits(base + tail));
    return base + tail;
}

}   // namespace hotd::util
