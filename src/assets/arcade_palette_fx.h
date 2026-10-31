// Arcade palette animation (docs/formats.md §24): maincpu 0x25310 runs once per game frame and computes pulsing colors from
// the frame counter (work RAM 0x51F428) into work RAM 0x52066C-0x5206E0; 0x24E90 copies them into polygon palette entries
// 984-1021 and 959. The picture shows the previous frame's values. Verified against 2630 MAME dumps (palette and work RAM).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hotd {

class ArcadePaletteFx {
public:
    bool load(const std::string& regionsDir, std::string& err);
    bool loaded() const { return !maincpu_.empty(); }

    // Game state the routine reads besides the frame counter: the stage byte (work RAM 0x51F9D1; 0-3 = Chapters 1-4) and
    // four event flags (0x51E982, 0x51E9A1, 0x51E9B2, 0x51E9B3), 0 outside their scenes.
    struct State { int stage = 0; uint8_t f982 = 0, f9a1 = 0, f9b2 = 0, f9b3 = 0; };

    // out[1024]: polygon palette entries for game frame `frame`: RGB555 | 0x8000 for the entries the routine sets, 0 for the
    // rest (those keep their loaded value). May set state.f9b3 like the game does.
    void compute(int frame, State& state, uint16_t out[1024]) const;

private:
    uint16_t rom16(uint32_t a) const;
    std::vector<uint8_t> maincpu_;
};

} // namespace hotd
