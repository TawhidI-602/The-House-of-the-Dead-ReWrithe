// Minimal PNG writer (RGB8, uncompressed deflate blocks). For screenshots; no dependencies.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hotd {

bool writePng(const std::string& path, int width, int height, const std::vector<uint8_t>& rgbRows);
// the same with an alpha channel (RGBA8 rows)
bool writePngRgba(const std::string& path, int width, int height, const std::vector<uint8_t>& rgbaRows);

} // namespace hotd
