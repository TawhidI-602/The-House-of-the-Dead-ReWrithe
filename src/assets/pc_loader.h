// PC loader (The House of the Dead, PC 1998). Formats: docs/formats.md §3 (VMC), §9 (world RBH).
// World geometry from ART\STAGEn\WORLD\WORLDn.RBH; world textures are pages 512 and up of ART\STAGE1\CREATURE\BIGGER.VMC
// (the texture word of a world polygon is the VMC page number).
#pragma once

#include "assets/assets.h"

#include <string>

namespace hotd {

struct PcConfig {
    std::string installDir;   // the player's PC install (read only)
    int blood = 0;            // blood colour 0 red (default) .. 3 purple (the launcher setting); -1: the install's TEXTURE.VMC;
                              // kBloodCustom | 0xRRGGBB (display, experimental): B_RED's blood texels tinted to that colour
    static constexpr int kBloodCustom = 0x1000000;
};

// display2: the given BIGGER.VMC pages as the PC game draws them with blood colour `blood` (PcConfig::blood): from
// B_<colour>.VMC (custom: B_RED's blood texels tinted), RGBA8 through tbl / pal when given. false: red / none / missing
bool pcBloodPages(const std::string& installDir, int blood, const std::vector<uint16_t>& pages,
                  std::vector<std::vector<uint8_t>>& out, std::string& err, const std::vector<uint8_t>* tbl = nullptr,
                  const std::vector<uint8_t>* pal = nullptr);

class PcSource : public AssetSource {
public:
    explicit PcSource(PcConfig cfg);
    Source source() const override { return Source::PC; }
    bool available() const override { return available_; }
    std::string status() const override { return status_; }
    int stageCount() const override { return 4; }
    std::unique_ptr<StageData> loadStage(int stage, std::string& err) override;

private:
    PcConfig cfg_;
    bool available_ = false;
    std::string status_;
};

} // namespace hotd
