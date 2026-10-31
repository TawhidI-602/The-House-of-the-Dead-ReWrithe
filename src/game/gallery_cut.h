// The Gallery's cut creatures: where their textures are (cutrestore, 2026-10-21; docs/cut_content.md "Textures").
// Every cut creature's polygons point into texture sheet 1, the area sheet (the texture headers in the texture ROM:
// sheet select, x, y, size). Which of the 12 area blocks (main_data blocks 1-12, formats.md §17) holds the matching
// pictures was found by laying each part's texture coordinates over every area's sheet and looking: the attract's area 2
// holds MrG's, Lubin's and Disiprin's (faces, chests, sleeves where the polygons land), Chapter 2's area 7 Syndy's, area 5
// Debu's (Bentley's places), area 6 Neozom's chest, Chapter 1's area 3 Baba's. The PC files have none of these (no meshes;
// none of BIGGER.VMC's 421 pages shows them).
#pragma once

namespace hotd::game {

struct GalleryCutLook {
    int type;               // arcade creature type
    int area;               // arcade texture area (1-12) whose sheet holds its textures
    const char* rows;       // the colour rows ("row:entry ...", as GalleryLook)
    const char* facts;      // the info panel's facts line (where the textures come from)
};

// the cut type's look, or null (not a cut type with textures found)
const GalleryCutLook* galleryCutLook(int type);

} // namespace hotd::game
