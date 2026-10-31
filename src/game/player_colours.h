// The players' colours (docs/coop4.md): P1 red and P2 blue (the arcade's guns, drawn in each piece of art's own
// shades), P3 green and P4 yellow for the experimental 4P mode. Players 3 / 4 never get edited art: their crosshairs and
// fire lamps are player 1's / 2's own art recoloured in code when it is built (util/recolour.h, the step texture packs
// use too).
#pragma once

#include "util/recolour.h"

#include <cstdint>

namespace hotd::game {

// the colour a player's art is tinted to (P1 / P2: the reference shades of their art; P3 green, P4 yellow)
inline constexpr uint8_t kPlayerRgb[4][3] = {{255, 60, 60}, {90, 170, 255}, {70, 235, 70}, {255, 225, 40}};

// the art players 3 / 4 are made from: player 2's for both (its blue flames and sight separate cleanly from the brass /
// dark parts by hue; player 1's red flames do not from the brass), players 1 / 2 their own
inline int playerArtSource(int p) { return p >= 2 ? 1 : p; }

// the colour ramps of players 3 / 4 (user 2026-10-20: "proper shades", not a flat recolour): shadow, deep, mid, hot,
// white-hot core, hand-tuned to read as green / amber-yellow next to player 1's red and player 2's blue
inline constexpr uint8_t kPlayerRamp[2][5][3] = {
    {{4, 24, 8}, {14, 88, 30}, {40, 176, 56}, {140, 242, 112}, {236, 255, 226}},     // P3 green
    {{44, 26, 0}, {146, 92, 0}, {236, 180, 16}, {255, 230, 84}, {255, 252, 216}},    // P4 yellow (mid less orange: 2026-10-20)
};

// player 2's hit notices (the blood splash / claw marks) in the 3-4 player mode: the red art through a blue ramp
// (user 2026-10-26), shadow .. core, in the shades of player 2's flames
inline constexpr uint8_t kPlayer2BloodRamp[5][3] = {{2, 8, 36}, {12, 36, 120}, {36, 100, 225}, {120, 186, 255}, {228, 240, 255}};

// the recolour of that art for player p: player 2's blue texels through the seat's ramp (none for players 1 / 2)
inline Recolour playerRecolour(int p)
{
    Recolour r;
    if (p >= 2) {
        r.op = Recolour::Ramp;
        r.select = Recolour::BlueDominant;
        for (int k = 0; k < 5; ++k)
            for (int c = 0; c < 3; ++c)
                r.ramp[k][c] = kPlayerRamp[p - 2][k][c];
    }
    return r;
}

} // namespace hotd::game
