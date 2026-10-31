// The official-files check (docs/teaser_plan.md, the Engine table's last row): the launcher checks that the player's
// files are the official releases, by content only (folder, zip and file names are free).
//
// Arcade: every ROM of MAME's `hotd` set (Revision A, 29 files) is matched by size + CRC-32 among the zips and loose
// files found in the chosen place (a zip, or a folder searched a few levels deep), then its SHA-1 is checked when it
// is read. The program, data, coprocessor, polygon and texture regions are rebuilt from them (as tools/m2_roms.py)
// into the cache folder; the result is remembered (the sources' sizes and dates), so the next start reads nothing.
// PC: the files the rewrite reads (src/game/pc_manifest.inc) are looked for by their path in the chosen folder
// (case-insensitive) and checked by size + SHA-1; a patched install whose patch kept a `.orig` copy passes through
// that copy; a file not found there (renamed or moved) is looked for once by size + SHA-1 among the folder's files
// and the place remembered. Every mismatch is reported with the file and the release expected.
#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace hotd::game {

struct CheckReport {
    bool ok = false;
    std::vector<std::string> lines;   // what was found / wrong, one line each (for the launcher and the log)
};

// the arcade set: `where` = a zip or a folder; on success the regions (maincpu.bin, main_data.bin, copro_data.bin,
// polygons.bin, textures.bin) are in `regionsDir` (created). progress: a short status line now and then (may be empty)
CheckReport checkArcade(const std::string& where, const std::string& regionsDir, bool force,
                        const std::function<void(const std::string&)>& progress = {});

// the PC files: `folder` = the PC game's folder; the resolved place of every file (manifest path -> real path) goes
// to `found` and is remembered in `cacheFile`. Music (Music\TrackNN.ogg, CD audio rips) is reported, not checked.
CheckReport checkPc(const std::string& folder, const std::string& cacheFile, std::map<std::string, std::string>& found,
                    const std::function<void(const std::string&)>& progress = {});

// the map of checked files goes to hotd::setPcFileMap (assets/archive.h), which the loaders consult

} // namespace hotd::game

namespace hotd::game {

// firstrun: after a failed check, one line telling the player what to put where (empty: nothing to add), from what the
// chosen place holds: nothing chosen, a .7z / .rar / CD image, a zip of zips, the PC game where the ROMs belong (or the
// other way round). `arcade`: the place was chosen as the arcade ROMs (else as the PC files)
std::string placementHint(const std::string& where, bool arcade);

} // namespace hotd::game
