// pmfix2 (docs/pcmode.md "Extra Characters", user 2026-10-28): the PC Mode extras' portraits in the style of the
// PC's own (THOTD.EXE's SPR.RBH 0xED..0xF2, 160 x 160: head and shoulders turned a little, a dark ground, the body
// fading out at the bottom; 0xF3..0xF8, 24 x 24: the face on a flat colour), made from each extra's own model with the
// Gallery's renderer at the first launch, from the player's files, and kept in the settings folder (pmportraits\);
// never shipped. The pick screen and the High Scores tables use them (Frontend::setPcPortrait / pcPortraitId).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hotd::assets { class PcSprites; }

namespace hotd::game {

class Gallery;
class Frontend;

constexpr int kPmPortraitW = 160, kPmPortraitH = 160, kPmSmallW = 24, kPmSmallH = 24;
constexpr const char* kPmPortraitVersion = "pmportraits 5";   // (pmportraits_<set>\version.txt: made again when it differs)

// character ch (7..19)'s two pictures from its model (gallery open; pmpick: its source the model set's, pcSet: the PC
// files when they have the model, else the arcade's; arcade: the arcade's); the big one on the PC portraits' black
// ground (spr: the front end's PC pictures), the small one on its own colour. Kept per model set (pmportraits_arcade /
// pmportraits_pc in the settings folder)
bool pmPortraitMake(Gallery& g, const assets::PcSprites& spr, int ch, bool pcSet, std::vector<uint8_t>& big, std::vector<uint8_t>& small);
// the cached pictures into the front end (false: some missing or another version: make them again)
bool pmPortraitsLoad(Frontend& fe, const std::string& dir);
bool pmPortraitSave(const std::string& dir, int ch, const std::vector<uint8_t>& big, const std::vector<uint8_t>& small);
// a sheet: the PC's six portraits and small pictures, then the extras' (testing / the changelog)
bool pmPortraitSheet(const Frontend& fe, const std::string& path);

} // namespace hotd::game
