// Window icon from the player's own THOTD.EXE (read only; nothing is stored). Silent no-op when it is missing.
#pragma once
#include <SDL3/SDL.h>

#include <string>

// pcDir = the PC game folder. Returns true when an icon was set (and logs its size).
bool setWindowIconFromPc(SDL_Window* window, const std::string& pcDir);
