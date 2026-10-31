#include "assets/arcade_palette_fx.h"

#include "assets/assets.h"

namespace hotd {

namespace {

void decode(uint16_t c, int p[3]) { p[0] = c & 31; p[1] = c >> 5 & 31; p[2] = c >> 10 & 31; }

// maincpu 0x24F80: components other than 31 lose their lowest bit
uint16_t encode(const int p[3])
{
    int q[3];
    for (int i = 0; i < 3; ++i) q[i] = p[i] == 31 ? 31 : p[i] & 30;
    return uint16_t(0x8000 | q[2] << 10 | q[1] << 5 | q[0]);
}

// color * (k mod 32) / 32 per component (integer division toward zero, as the i960's shrdi)
uint16_t scale(uint16_t c, int k)
{
    int p[3];
    decode(c, p);
    k %= 32;
    for (int& v : p) v = v * k / 32;
    return encode(p);
}

// triangle wave: t mod period, mirrored above half
int triangle(int t, int period, int half)
{
    const int r = t % period;
    return r > half ? period - 1 - r : r;
}

// work RAM address -> polygon palette entry (maincpu 0x24E90), -1 = not copied
int entryOf(uint32_t addr)
{
    if (addr == 0x5206E0) return 959;
    if (addr < 0x520664 || addr > 0x5206AE || addr == 0x520668 || addr == 0x520688 || addr == 0x52068A) return -1;
    return 1021 - int(addr - 0x520664) / 2;
}

} // namespace

bool ArcadePaletteFx::load(const std::string& regionsDir, std::string& err)
{
    return readFile(regionsDir + "/maincpu.bin", maincpu_, err);
}

uint16_t ArcadePaletteFx::rom16(uint32_t a) const
{
    return a + 2 <= maincpu_.size() ? uint16_t(maincpu_[a] | maincpu_[a + 1] << 8) : 0;
}

// maincpu 0x25310 (called from 0x1DE4 once per frame)
void ArcadePaletteFx::compute(int frame, State& st, uint16_t out[1024]) const
{
    for (int i = 0; i < 1024; ++i) out[i] = 0;
    if (!loaded() || st.stage == 5 || (st.stage == 2 && st.f982)) return;
    auto set = [&](uint32_t addr, uint16_t v) {
        if (const int e = entryOf(addr); e >= 0) out[e] = uint16_t(v | 0x8000);
    };
    // 9 entries cycling through the 90-color table at 0xAB120
    for (int i = 0; i < 9; ++i) set(0x52066C + 2 * i, rom16(0xAB120 + 2 * (int(float(frame + 3 * i) / 1.5f) % 90)));
    // 96-frame pulse of the base colors at 0xA35E8..
    int k = int(float(triangle(frame, 96, 47)) / 1.5f);
    for (int i = 0; i < 3; ++i) set(0x52067E + 2 * i, scale(rom16(0xA35E8 + 2 * i), k));
    set(0x520684, st.stage == 6 ? 0 : scale(rom16(0xA35EE), k));
    set(0x52068C, scale(rom16(0xA35EE), k));
    // 14 entries pulsing between 12/32 and 31/32 of their base colors (0xA35FA..), 100 frames, phases 14 frames apart
    for (int i = 0; i < 14; ++i) {
        const uint32_t a = 0x520690 + 2 * i;
        if (a == 0x520690 && st.stage == 3 && st.f9b3) { set(a, 0); continue; }
        if (a == 0x520698) {   // only cleared in stage 6, otherwise left alone
            if (st.stage == 6) set(a, 0);
            continue;
        }
        const int v = int(float(triangle(frame + 0x150 + 14 * i, 100, 49)) / 2.5f + 12.0f);
        if (a == 0x520690 && st.stage == 3 && st.f9b2 && v == 12) { st.f9b3 = 1; continue; }
        set(a, scale(rom16(0xA35FA + 2 * i), v));
    }
    // 64-frame pulse: one base color, a blend 0x801F -> 0x800C (entry 984, the blood color) and entry 959
    k = triangle(frame, 64, 31);
    set(0x52068E, scale(rom16(0xA35F8), k));
    int a[3], b[3], m[3];
    decode(0x801F, a);
    decode(0x800C, b);
    for (int i = 0; i < 3; ++i) m[i] = a[i] + (b[i] - a[i]) * (k % 32) / 32;
    set(0x5206AE, encode(m));
    set(0x5206E0, scale(st.stage == 3 && st.f9a1 ? 0x8012 : 0x800E, k));
}

} // namespace hotd
