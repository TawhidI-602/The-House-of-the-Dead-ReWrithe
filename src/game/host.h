// The game's memory and logic, set up from the player's arcade files (runtime.h); the work RAM is built from the ROM
// (game/boot.h) or, as a developer option, loaded from a MAME work RAM snapshot.
#pragma once

#include "game/runtime.h"

#include <memory>
#include <string>
#include <vector>

namespace hotd::game {

class Host {
public:
    // regionsDir: maincpu.bin, main_data.bin, copro_data.bin (tools/m2_roms.py extract from the arcade dump);
    // snapshot: empty = the work RAM built from the ROM (boot_work_ram), else a 1 MB work RAM dump (MAME,
    // 0x500000..0x5FFFFF) of a game in progress (developer option); namesTsv: optional
    // (tools/ghidra/names.tsv, routine names in the log)
    bool load(const std::string& regionsDir, const std::string& snapshot, const std::string& namesTsv, std::string& err);

    Runtime& runtime() { return *runtime_; }
    bool loaded() const { return runtime_ != nullptr; }   // (remakepages: before the first game: no runtime yet)
    Mem& mem() { return *mem_; }

private:
    std::unique_ptr<Mem> mem_;
    std::vector<uint32_t> copro_;
    std::unique_ptr<Tgp> tgp_;
    std::unique_ptr<Runtime> runtime_;
};

} // namespace hotd::game
