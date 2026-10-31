// Build helper for -DHOTD_LOCAL_ICON: exeicon <exe> <out.ico>. Reads the exe, writes only the .ico in the build folder.
#include <cstdio>

#include "util/pe_icon.h"

int main(int argc, char** argv) {
    std::vector<uint8_t> ico;
    if (argc < 3 || !peicon::extractIco(argv[1], ico)) { std::fprintf(stderr, "exeicon: no icon in the file\n"); return 1; }
    FILE* f = std::fopen(argv[2], "wb");
    if (!f) return 1;
    std::fwrite(ico.data(), 1, ico.size(), f);
    std::fclose(f);
    return 0;
}
