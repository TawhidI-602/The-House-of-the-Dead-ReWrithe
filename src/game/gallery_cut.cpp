#include "game/gallery_cut.h"

namespace hotd::game {

const GalleryCutLook* galleryCutLook(int type)
{
    // rows: the colour rows of a creature the game draws in that area (GalleryLook's), so the cut ones are coloured like
    // their neighbours (which rows the game would have given them is not known); area 2 (no creature there): the
    // zombies' usual skin rows
    static const GalleryCutLook kLooks[] = {
        {5, 2, "7:81 9:28", "BO_mr_g. Textures found: attract sheet (area 2). U: untextured"},
        {39, 2, "7:81 9:28", "BO_lubin. Textures found: attract sheet (area 2). U: untextured"},
        {52, 2, "7:81 9:28", "BO_disiprin. Textures found: attract (area 2). U: untextured"},
        {10, 7, "5:39 7:81 9:28 11:28 13:24 17:20 19:71 29:87", "BO_syndy. Textures found: Chapter 2 (area 7). U: untextured"},
        {12, 5, "5:39 7:81 9:28", "BO_debu. Textures: Bentley's places (area 5). U: untextured"},
        {56, 6, "5:39 7:81 9:28 11:56", "BO_neozom. Chest texture found: area 6. U: untextured"},
        {66, 3, "5:119 7:81 9:28 11:37 19:70 21:100", "BO_pbaba. Textures found: Chapter 1 (area 3). U: untextured"},
    };
    for (const GalleryCutLook& l : kLooks)
        if (l.type == type) return &l;
    return nullptr;
}

} // namespace hotd::game
