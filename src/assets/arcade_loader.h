// Arcade loader (Sega Model 2C, House of the Dead Revision A). Formats: docs/formats.md §8, §10, §12, §13.
//
// Geometry, placements, texture coordinates/headers and the luma table come from the ROM regions
// (tools/m2_roms.py extract). Texture RAM, polygon palette and color translation tables come, for now, from MAME dumps of the
// matching scene (tools/run_mame_dump.ps1, replay_hotd.ps1 -TexRam): the game builds them at run time, and rebuilding them
// from the ROM alone is a release requirement (ROADMAP) that isn't done yet.
#pragma once

#include "assets/assets.h"

#include <array>
#include <map>
#include <string>
#include <vector>

namespace hotd {

// rellook: the players' gun hand (class 6 part 0x719 `ter00a`), their empty hand (0x36B `kenter00a`, which
// body_part_draw_hook swaps in) and the generated model of the gun hand's gun alone (ArcadeSource::loadStage)
constexpr uint32_t kPlayerGunHand = 0x719, kPlayerEmptyHand = 0x36B, kGunOnlyKey = 0xF1000719;

// Object ID -> part / model name from main_data (formats.md §22): pointer table at 0xEE4EB0, one CPU address (0x02xxxxxx =
// main_data) of a "PN_..." string per ID; IDs without a name hold 0 and get "". "_DD" at the end marks a shadow model.
std::vector<std::string> arcadeObjectNames(const std::vector<uint8_t>& mainData);

struct ArcadeConfig {
    std::string regionsDir;   // maincpu.bin, main_data.bin, polygons.bin, textures.bin
    std::string dumpsDir;     // MAME dumps: <tag>_texram.bin, <tag>_palette.bin, <tag>_colorxlat.bin
    std::string texsetFile;   // optional: which dump (texture set) each model uses, see tools/texset_assign.py
    bool romColors = true;    // "rom:<n>" sets: polygon palette and color translation from the ROM (else from the dump)
    bool keepAlternates = false;   // keep every alternate-state placement (the caller picks one, tools/arcade_states.py)
    bool defaultSkies = false;     // no texset file: the sky models per area from a built-in table (the viewer; the game has sky_task)
};

class ArcadeSource : public AssetSource {
public:
    explicit ArcadeSource(ArcadeConfig cfg);
    Source source() const override { return Source::Arcade; }
    bool available() const override { return available_; }
    std::string status() const override { return status_; }
    int stageCount() const override { return 4; }
    std::unique_ptr<StageData> loadStage(int stage, std::string& err) override;

    // Generated skins (decompilation.md "World drawing"): polygons the game stretches every frame between node A (the
    // corners follow it) and node B (the skin is drawn with its matrix) of a creature, e.g. the zombies' bellies.
    struct Skin {
        int skin = -1, polygons = 0, nodeA = -1, nodeB = -1;
        uint32_t texPoint = 0, texHeader = 0;
        std::vector<uint8_t> words;                    // the template model
        std::vector<std::array<float, 3>> corners;    // in node A space
        std::vector<int> vertex;                       // per corner: model vertex it moves
        int sharedVertex = -1, sharedCopy = -1;
    };
    // which = 0: the type's skin; 1: Sophie's extra strip (type 34). False: none.
    bool skinInfo(int type, Skin& out, int which = 0);
    // Call before loadStage: the stage then holds a template model under each key (one per creature drawn)
    struct SkinRequest { uint32_t key; int type, which; };
    void requestSkins(int stage, const std::vector<SkinRequest>& req) { skinRequests_[stage] = req; }
    // moved: the corners in node B space (B^-1 x A x corner)
    void rebuildSkin(int stage, const Skin& sk, const std::vector<std::array<float, 3>>& moved, Model& model) const;
    // the colour translation table of texture area `area` (TextureSet::xlat layout) with rows swapped in from the row
    // library (row, entry), as color_row_set does at run time; call after loadStage
    void colorXlat(int area, const std::vector<std::pair<int, int>>& rowSwaps, std::vector<uint8_t>& out) const
    {
        buildColorXlat(area, rowSwaps, out);
    }
    // the texset file gave this set its colour rows (a "rows" line, measured in MAME): its translation is the game's already
    bool hasRowSwaps(int stage, int set) const { return stage >= 0 && stage < 4 && rowSwaps_[stage].count(set) > 0; }

private:
    void buildMeshes(int stage, const std::vector<uint8_t>& poly, size_t addr, uint32_t tpa, uint32_t tha, uint32_t links,
                     Model& model) const;
    std::map<int, std::vector<SkinRequest>> skinRequests_;
    std::vector<std::vector<uint16_t>> palettes_[4];   // per stage, per texture set
    bool loadRegions(std::string& err);
    std::vector<std::string> romStageTags(int stage) const;   // the stage's texture sets from the ROM (no texset file)
    void loadNames();
    std::vector<std::string> names_;          // object ID -> name (arcadeObjectNames)
    bool buildTextureRam(int areaBlock, std::vector<uint8_t>& out, std::string& err);
    // rowSwaps: (odd row, row library entry) pairs applied after the area's own rows (scene events, texset "rows" lines)
    void buildColorXlat(int areaBlock, const std::vector<std::pair<int, int>>& rowSwaps, std::vector<uint8_t>& out) const;
    bool loadTextureSet(int stage, int setIndex, const std::string& tag, TextureSet& out, std::vector<uint16_t>& palette,
                        std::string& err);

    ArcadeConfig cfg_;
    bool available_ = false;
    std::string status_;
    std::vector<uint8_t> maincpu_, mainData_, polygons_, textures_;
    // texture set tags per stage and model -> set index, from cfg_.texsetFile (or built-in defaults)
    std::vector<std::string> stageTags_[4];
    std::map<uint32_t, int> modelSet_[4];
    std::map<uint32_t, std::vector<int>> modelValid_[4];   // sets each model's textures are loaded in (texset "valid" lines)
    struct Light { bool known = false; float dir[3] = {0, 1, 0}, diffuse[32] = {}, ambient[32] = {}, specScale[32] = {}, specCtl[32] = {}; };
    std::map<int, Light> lights_[4];          // per stage, per set index
    std::vector<StageData::Zone> zones_[4];
    std::map<int, std::vector<std::pair<int, int>>> rowSwaps_[4];   // per stage, per set index
    std::map<int, std::vector<StageData::Sky>> skies_[4];           // per stage, per set index (texset "sky" lines)   // recorded camera positions and the texture set loaded there
};

} // namespace hotd
