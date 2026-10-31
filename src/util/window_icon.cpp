#include "util/window_icon.h"

#include "assets/pc_files.h"
#include "util/pe_icon.h"

bool setWindowIconFromPc(SDL_Window* window, const std::string& pcDir) {
    if (!window || pcDir.empty()) return false;
    // the PC folder or the PC zip (pczip: THOTD.EXE read through the zip reader, any case, maybe one folder down)
    std::vector<uint8_t> exe;
    peicon::Rgba img;
    if (!hotd::pcfs::readFile(pcDir + "/THOTD.EXE", exe) || !peicon::largestImage(std::move(exe), img)) return false;
    SDL_Surface* s = SDL_CreateSurfaceFrom(img.w, img.h, SDL_PIXELFORMAT_RGBA32, img.px.data(), img.w * 4);
    if (!s) return false;
    const bool ok = SDL_SetWindowIcon(window, s);   // copies the pixels
    SDL_DestroySurface(s);
    if (ok) SDL_Log("window icon: THOTD.EXE, %dx%d", img.w, img.h);
    return ok;
}
