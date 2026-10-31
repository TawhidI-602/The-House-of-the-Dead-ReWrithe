// Minimal PNG reader (our own, on assets/archive.h's inflater; no third-party library): non-interlaced images of any
// colour type and bit depth, to RGBA8. For the player's own crosshair pictures (game/sights.h).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hotd {

// false (err set) for a file that is not a readable PNG (interlaced, broken, larger than 1024 x 1024)
bool readPngRgba(const std::string& path, int& width, int& height, std::vector<uint8_t>& rgba, std::string& err);

} // namespace hotd
