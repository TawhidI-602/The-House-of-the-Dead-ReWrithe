// The CRT filter (launcher "CRT Filter"): a post-process pass over the final picture (3D + 2D layer + HUD, the front
// end's menus too). The frame is drawn into an offscreen target at the window's size (Crt::begin), then our CRT shader
// draws it to the window (Crt::present):
//   1. lines: the picture averaged into the GAME's original lines (the arcade's 384, the PC port's 480: the line pitch
//      is the window height / lines, so widescreen keeps the same pitch), in linear light (gamma in)
//   2. horizontal: the beam's horizontal softness (in the original's pixels) and an NTSC-ish colour bleed (YIQ, the
//      chroma blurred wider than the luma)
//   3. glow: a small blurred copy (bloom), two separable passes at 1/8 of the window
//   4. the window: barrel curvature, two neighbouring lines' beams per pixel (Gaussian, the beam wider for brighter
//      colours, integrated over the pixel's height: sharp at any window size, 4K included), the phosphor mask in the
//      display's own pixels (aperture grille / slot mask / shadow mask), brightness compensation for the beam and the
//      mask, the glow, vignette, rounded corners, gamma out.
// Written for ReWrithe (no third-party shader code): it follows the well-known public techniques (Gaussian scanlines
// with the beam width by brightness, RGB phosphor masks, separable bloom, CRT-Geom-like barrel warp).
//
// The pointer: the curvature moves the picture, so a window point P shows the picture's point toSource(P). The guns
// (mouse, light gun, pads) aim through it (main.cpp), so what is under the pointer on the curved screen is what is
// shot; the crisp crosshairs (drawn after the filter, the default) are drawn at P itself, the filtered ones (inside
// the filter) at toSource(P), which the warp shows at P.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hotd {

// the launcher's choices (launcher.ini crt=, crt.<key>=)
struct CrtSettings {
    int preset = 0;            // 0 Off, 1 Arcade Monitor, 2 Consumer TV, 3 Sharp Scanlines, 4 Custom
    // Custom (More Options > CRT Settings), steps 0-10 (mask type 0-3)
    int scan = 8;              // scanline strength
    int mask = 6;              // mask strength
    int maskType = 1;          // 0 none, 1 aperture grille, 2 slot mask, 3 shadow mask
    int curve = 3;             // curvature
    int bloom = 3;             // bloom / glow
    int bright = 5;            // brightness boost (5 = none)
    bool crispSights = true;   // the crosshairs drawn after the filter (crisp, exact) rather than inside it
    bool shotFiltered = true;  // F12 screenshots: the filtered picture as seen (else the plain picture)
};
constexpr int kCrtPresets = 5;
const char* crtPresetKey(int preset);    // launcher.ini / --crt: off, arcade, tv, sharp, custom
const char* crtPresetName(int preset);   // the menu's
int crtPresetParse(const std::string& s);   // -1: none
const char* crtMaskName(int type);

// the shader's values (from a preset, or Custom's steps)
struct CrtParams {
    float scan = 0.8f;                     // 0 flat .. 1 full scanlines
    float beamMin = 0.2f, beamMax = 0.45f; // the beam's Gaussian sigma in lines: dark .. bright
    int maskType = 1;
    float mask = 0.3f;                     // 0 .. 1: how dark the unlit phosphors are
    float curve = 0.03f;                   // barrel curvature
    float bloom = 0.1f;
    float boost = 1.0f;                    // brightness
    float hsoft = 0.3f;                    // horizontal beam softness, in the original's pixels
    float bleed = 0.0f;                    // NTSC-ish chroma bleed, extra in the original's pixels
    float vignette = 0.0f;
    float corner = 0.0f;                   // the corners' radius, a fraction of the window's height
    float gammaIn = 2.2f, gammaOut = 2.2f;
};
CrtParams crtParams(const CrtSettings& s);

class Crt {
public:
    ~Crt();
    // the settings and the game's original line count (384 arcade, 480 the PC port / its menus) and its pixels across
    // a 4:3 screen (496 / 640). Builds the GL objects on first use (a current context); false if they failed (the
    // filter then stays off)
    void configure(const CrtSettings& s, int lines, int pixelsAcross);
    bool active() const { return on_; }
    const CrtSettings& settings() const { return set_; }
    unsigned sceneFramebuffer() const { return sceneFbo_; }
    // binds the offscreen target (ww x wh) when active and leaves the viewport at it; false: nothing changed (draw to
    // the window as usual)
    bool begin(int ww, int wh);
    // the picture drawn since begin through the filter into framebuffer `target` (0 the window); leaves `target`
    // bound with its viewport, no blending
    void present(int ww, int wh, unsigned target = 0);
    // present into a scratch target, read back (RGB, top row first), `after` draws over it first (the crisp sights)
    template <class F> std::vector<uint8_t> presentToImage(int ww, int wh, F&& after)
    {
        const unsigned t = scratch(ww, wh);
        present(ww, wh, t);
        after();
        return readBack(ww, wh);
    }
    // a whole RGB picture (top row first) through the filter (screenshots of CPU-drawn screens); empty if inactive
    std::vector<uint8_t> filterImage(const std::vector<uint8_t>& rgb, int w, int h);
    // the curvature, in window pixels: the picture's point shown at window point (x, y), and its inverse
    void toSource(float& x, float& y, int ww, int wh) const;
    void toScreen(float& x, float& y, int ww, int wh) const;
    // the last present's GPU time (ms) when measured (bench: HOTD_CRT_BENCH)
    double benchPresent(int ww, int wh, int reps);

private:
    CrtSettings set_;
    CrtParams p_;
    int lines_ = 384, across_ = 496;
    bool on_ = false, tried_ = false, ok_ = false;
    unsigned progLines_ = 0, progH_ = 0, progGlow_ = 0, progMain_ = 0, vao_ = 0;
    // scene (RGBA8 + depth), lines A / B (RGBA16F, ww x lines), glow A / B (RGBA16F, 1/8), scratch output
    unsigned sceneFbo_ = 0, sceneTex_ = 0, sceneDepth_ = 0;
    unsigned lineFbo_[2] = {}, lineTex_[2] = {};
    unsigned glowFbo_[2] = {}, glowTex_[2] = {};
    unsigned outFbo_ = 0, outTex_ = 0;
    int w_ = 0, h_ = 0, lw_ = 0, lh_ = 0, gw_ = 0, gh_ = 0, ow_ = 0, oh_ = 0;
    bool build();
    void resize(int ww, int wh);
    unsigned scratch(int ww, int wh);
    std::vector<uint8_t> readBack(int ww, int wh);
};

} // namespace hotd
