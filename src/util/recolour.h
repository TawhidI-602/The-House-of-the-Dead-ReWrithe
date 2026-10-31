// One recolour helper for RGBA8 texels (texture packs' mod.ini rules, docs/mods.md; the 4P colours of the HUD and
// torches, docs/coop4.md). Pure functions, no state: visual only, never game state (netplay: each PC its own look).
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace hotd {

struct Recolour {
    enum Op : uint8_t {
        None,
        Tint,       // each texel's brightness (its strongest channel) times rgb: a red blood texel -> the same shade of rgb
        Hue,        // hue turned by hueDeg (saturation and value kept)
        Multiply,   // each channel times rgb / 255
        Ramp,       // a colour ramp: each texel's level (half luminance, half its strongest channel: dark shadows,
                    // saturated mids, a white-hot core) through five stops (ramp[0] at 0 .. ramp[4] at 1), so the
                    // art's shading survives in the new colour; `select` limits it to some texels
    };
    enum Select : uint8_t { All, BlueDominant, RedDominant };
    Op op = None;
    float hueDeg = 0;
    uint8_t rgb[3] = {255, 255, 255};
    uint8_t ramp[5][3] = {};
    Select select = All;
};

// alpha untouched; texels with alpha 0 are left as they are
void recolourRgba(uint8_t* rgba, size_t pixels, const Recolour& r);

// "tint #RRGGBB" / "tint RRGGBB" / "#RRGGBB" (= tint), "hue <degrees>", "multiply #RRGGBB" (or "mul"); false: unknown
bool parseRecolour(const std::string& text, Recolour& out);

} // namespace hotd
