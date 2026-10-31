// Shared creature-posing helpers (hotd_viewer and the game's Gallery, src/game/gallery.h): a skeleton node's Xform
// as a Placement and back, the motions a creature type can play (same joint count, formats.md §23), and a plain kind
// for a motion from its developer name ("MO_z_walk_g" -> Walk).
#pragma once

#include "assets/arcade_anim.h"
#include "assets/assets.h"

#include <string>
#include <utility>
#include <vector>

namespace hotd {

// world = xf: the placement's rotation / position (the rest of the placement is left alone)
void toPlacement(const Xform& xf, Placement& pl);
Xform fromPlacement(const Placement& pl);

// every motion with the type's joint count, in motion-number order (empty: the type has no skeleton)
std::vector<int> compatibleMotions(const CreatureAnimations& anim, int type);
// the next (dir +1) / previous (-1) motion after `motion` with the type's joint count (`motion` if there is none)
int stepMotion(const CreatureAnimations& anim, int type, int motion, int dir);

enum class MotionKind { Idle, Walk, Attack, Hit, Death, Other };
const char* motionKindName(MotionKind k);
// from the developer name's words (walk / aruki / run / dash, atk / att / naguri, hit / dam / dm, dead / sini / taore,
// kamae / pose / base / kihon ...): a guess for the label, nothing depends on it
MotionKind motionKind(const std::string& motionName);

// What the game draws for one body part (body_part_draw_hook 0x9CE60, the model choices only): the part's model, or
// another in its place (Sophie's eyes / hand, Samson's saw, the players' gun and hands), plus extra models drawn with
// the part's matrix (the Hangedman's wings, the Magician's veins, Rogan's arcade coat). tick: frames since the start
// (the game's blink_phase / wing counter); frame: the motion frame; pcModels: the PC model source (no arcade coat).
struct PartDrawState {
    int type = 0, role = 0, cls = -1, motion = 0, frame = 0, tick = 0;
    int roleId = -1;   // the role table's byte 0 (CreatureAnimations::partClass(type, role, 0)): the Hermit's face is 27
    uint32_t model = 0;
    bool pcModels = false;
};
void partDrawModels(const PartDrawState& s, std::vector<uint32_t>& out);
// every model partDrawModels can give for the type besides its skeleton's (to request before the stage loads)
std::vector<uint32_t> partDrawExtraModels(int type);

// Arcade colour rows (color_row_set 0x13F20: the event scripts' GGCOL / GGSET swap rows of the colour translation; the
// creatures are re-coloured that way per spawn group): "row:entry ..." with the translation table's row (2 x row + 1,
// Runtime::colorRows' keys) and the row library entry, as ArcadeSource::colorXlat takes them.
std::vector<std::pair<int, int>> parseColorRows(const char* rows);
// The rows the game has swapped into a chapter's texture area most of the time while creatures are drawn there (counted in
// game runs over every chapter and route, HOTD_TRACE_TYPE_ROWS); "" for an area without any
const char* arcadeAreaColorRows(int chapter, int area);

} // namespace hotd
