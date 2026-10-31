// Windows shell integration on the player's own machine: the desktop / Start menu shortcut, whose icon points at the
// player's THOTD.EXE (nothing of Sega's is copied or shipped; no executable is ever modified). Linux: a .desktop file; macOS: "not supported".
#pragma once
#include <string>

namespace winshell {

enum class ShortcutResult { Created, Exists, Failed };

// A "The House of the Dead - ReWrithe.lnk" (the old "HOTD ReWrithe.lnk" is replaced) pointing at this hotd.exe (working folder = its folder), IconLocation = <pcDir>\THOTD.EXE,0.
// folder empty: the user's Desktop (and the Start menu programs folder when startMenu); else only `folder` (testing).
// An existing shortcut is left alone (Exists, nothing written) unless overwrite.
ShortcutResult createShortcut(const std::string& folder, bool startMenu, const std::string& pcDir, bool overwrite,
                              std::string& msg);

// this program's own path (Windows: GetModuleFileName; Linux: /proc/self/exe; macOS: _NSGetExecutablePath)
std::string selfExePath();

// setup (2026-10-31): Windows: the window's frame as a fixed dialog (on: no minimize / maximize box, no sizing border;
// off: the minimize box back, the rest is SDL's again). hwnd = SDL's SDL_PROP_WINDOW_WIN32_HWND_POINTER; elsewhere nothing
void dialogFrame(void* hwnd, bool on);

}  // namespace winshell
