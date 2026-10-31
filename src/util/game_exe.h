// The package's own game exe (user 2026-10-31): once the files check passes, the setup makes
// "<package>\The House of the Dead ReWrithe.exe" from bin\hotd_launch.exe (src/launch/launch_main.cpp) with the icon of
// the player's THOTD.EXE (its folder or zip; else this program's own icon). Windows only; elsewhere it does nothing.
#pragma once

#include <string>

namespace hotd::util {

constexpr const char* kGameExeName = "The House of the Dead ReWrithe.exe";

// packageRoot: the package's folder (empty: not a package, nothing done); pcDir: the PC game's folder or zip.
// Made again when missing or older than bin\hotd_launch.exe. false: not made (msg says why)
bool makeGameExe(const std::string& packageRoot, const std::string& pcDir, std::string& msg);

} // namespace hotd::util
