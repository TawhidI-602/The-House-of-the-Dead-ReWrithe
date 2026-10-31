// "Add to Steam" (user, 2026-10-17; ROADMAP "Add to Steam"): adds The House of the Dead: ReWrithe to Steam as a
// non-Steam game, on the player's press only. It edits Steam's user config only (never an executable): each
// account's userdata\<id>\config\shortcuts.vdf (binary KeyValues; backed up first) and its grid\ art, composed at that
// moment from the player's own PC files (sprites 0xAA title art, 0xD1 mansion, 0xD4 logo with Rogan; never shipped).
// Steam rewrites shortcuts.vdf when it exits, so it refuses while Steam runs. An entry under the name (or an earlier
// name) is updated in place, never duplicated; "remove" takes it and its art out again.
#pragma once

#include <map>
#include <string>
#include <vector>

namespace hotd::steam {

struct Options {
    std::string exe;       // the program Steam starts (this hotd.exe; a package's bin\hotd.exe finds data\ itself)
    std::string pcDir;     // the player's PC files (folder or zip): the art; empty = no art
    std::string customDir; // the player's own pictures (cover / banner / hero / logo / icon .png / .jpg; icon .ico too)
    // the player's choice per picture ("cover", "banner", "hero", "logo", "icon"): a path relative to artDir(exe), as
    // artChoices lists it; missing or empty = the picture made from the PC files
    std::map<std::string, std::string> picks;
    std::string steamDir;  // empty: from the registry (HKCU\Software\Valve\Steam SteamPath)
    std::string testVdf;   // testing: this shortcuts.vdf instead of Steam's (no Steam check)
    std::string gridOut;   // testing: the art here (with testVdf)
};

struct Result {
    bool ok = false;
    std::vector<std::string> lines;   // what was done / why not, for the log and the launcher
    std::string brief;                // a few words for a menu value ("Added", "Close Steam first", ...)
};

bool steamRunning();
// the folder of the player's own Steam pictures for this exe: a package's data\steam, else "steam art" next to the exe.
// Inside: covers\ (600 x 900), "wide covers"\ (920 x 430; Steam's wide cover, old name banners\ too), heros\ (1920 x 620), logos\, icons\ (SteamGridDB's grids\ too:
// its tall pictures count as covers, its wide ones as wide covers); any .png / .jpg (icons also .ico)
std::string artDir(const std::string& exe);
// the pictures there for one kind, as paths relative to the folder, sorted
std::vector<std::string> artChoices(const std::string& dir, const std::string& kind);
Result addToSteam(const Options& o);
// the pictures made from the PC files saved as Default.png in each folder of artDir (cover, banner, hero, logo, icon)
bool writeDefaultPictures(const Options& o, std::vector<std::string>& lines);
Result removeFromSteam(const Options& o);

} // namespace hotd::steam
