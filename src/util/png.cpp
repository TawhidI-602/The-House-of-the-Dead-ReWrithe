#include "util/png.h"

#include <algorithm>
#include <fstream>

namespace hotd {

namespace {

uint32_t crc32(const uint8_t* p, size_t n, uint32_t crc = 0)
{
    static uint32_t table[256];
    if (!table[1])
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
    crc = ~crc;
    for (size_t i = 0; i < n; ++i) crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

void be32(std::vector<uint8_t>& v, uint32_t x)
{
    for (int s = 24; s >= 0; s -= 8) v.push_back(uint8_t(x >> s));
}

void chunk(std::ofstream& f, const char* tag, const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> c;
    be32(c, uint32_t(data.size()));
    c.insert(c.end(), tag, tag + 4);
    c.insert(c.end(), data.begin(), data.end());
    be32(c, crc32(c.data() + 4, c.size() - 4));
    f.write(reinterpret_cast<const char*>(c.data()), std::streamsize(c.size()));
}

} // namespace

// channels 3 = RGB8 (colour type 2), 4 = RGBA8 (colour type 6)
static bool writeImpl(const std::string& path, int width, int height, const std::vector<uint8_t>& rgb, int ch)
{
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    const uint8_t sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    f.write(reinterpret_cast<const char*>(sig), 8);
    std::vector<uint8_t> ihdr;
    be32(ihdr, uint32_t(width));
    be32(ihdr, uint32_t(height));
    ihdr.insert(ihdr.end(), {8, uint8_t(ch == 4 ? 6 : 2), 0, 0, 0});   // 8-bit RGB / RGBA
    chunk(f, "IHDR", ihdr);

    std::vector<uint8_t> raw;   // filter byte 0 + row
    raw.reserve(size_t(height) * (width * ch + 1));
    for (int y = 0; y < height; ++y) {
        raw.push_back(0);
        raw.insert(raw.end(), rgb.begin() + ptrdiff_t(size_t(y) * width * ch), rgb.begin() + ptrdiff_t(size_t(y + 1) * width * ch));
    }
    std::vector<uint8_t> z = {0x78, 0x01};   // zlib header, stored blocks
    uint32_t a = 1, b = 0;
    for (uint8_t x : raw) { a = (a + x) % 65521; b = (b + a) % 65521; }
    for (size_t p = 0; p < raw.size() || p == 0;) {
        size_t n = std::min<size_t>(65535, raw.size() - p);
        z.push_back(p + n == raw.size() ? 1 : 0);
        z.push_back(uint8_t(n));
        z.push_back(uint8_t(n >> 8));
        z.push_back(uint8_t(~n));
        z.push_back(uint8_t(~n >> 8));
        z.insert(z.end(), raw.begin() + ptrdiff_t(p), raw.begin() + ptrdiff_t(p + n));
        p += n;
        if (n == 0) break;
    }
    be32(z, (b << 16) | a);
    chunk(f, "IDAT", z);
    chunk(f, "IEND", {});
    return bool(f);
}

bool writePng(const std::string& path, int width, int height, const std::vector<uint8_t>& rgb)
{
    return writeImpl(path, width, height, rgb, 3);
}

bool writePngRgba(const std::string& path, int width, int height, const std::vector<uint8_t>& rgba)
{
    return writeImpl(path, width, height, rgba, 4);
}

} // namespace hotd
