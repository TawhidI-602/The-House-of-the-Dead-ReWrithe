// Reads the icon of a Windows executable (PE resources RT_GROUP_ICON / RT_ICON) without any OS API: portable, read only.
#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace peicon {

struct Rgba {
    int w = 0, h = 0;
    std::vector<uint8_t> px;   // w*h*4, top row first
};

// The icon group as a complete .ico file (for the local-only Explorer icon of hotd.exe). False when absent.
bool extractIco(const std::string& exePath, std::vector<uint8_t>& ico);
// the same from the executable's bytes (THOTD.EXE inside a zipped PC install)
bool extractIco(std::vector<uint8_t> exe, std::vector<uint8_t>& ico);

// The largest uncompressed (DIB) image of the first icon group, 32-bit preferred, decoded to RGBA.
bool largestImage(const std::string& exePath, Rgba& out);
// the same from the executable's bytes (THOTD.EXE inside a zipped PC install)
bool largestImage(std::vector<uint8_t> exe, Rgba& out);
// the first bitmap resource (RT_BITMAP) of an executable's bytes, decoded to RGBA (opaque); false when there is none
// (THOTDRUN.EXE: the CD launcher's picture, two clown-like zombies in the sewers under the logo)
bool firstBitmap(std::vector<uint8_t> exe, Rgba& out);

}  // namespace peicon
