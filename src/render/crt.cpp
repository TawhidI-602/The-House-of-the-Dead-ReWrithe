#include "render/crt.h"

#include "render/gl.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>

namespace hotd {

namespace {

const char* const kKeys[kCrtPresets] = {"off", "arcade", "tv", "sharp", "custom"};
const char* const kNames[kCrtPresets] = {"Off", "Arcade Monitor", "Consumer TV", "Sharp Scanlines", "Custom"};

// every pass: one triangle strip over the whole target; the fragment shaders work from gl_FragCoord
const char* kVs = R"(#version 330 core
void main() {
    vec2 c = vec2(gl_VertexID & 1, gl_VertexID >> 1);
    gl_Position = vec4(c * 2.0 - 1.0, 0.0, 1.0);
}
)";

// 1. the picture's rows of each original line averaged (4 bilinear taps across the line: 2.8 rows at 1080, 5.6 at
// 2160), in linear light
const char* kFsLines = R"(#version 330 core
out vec4 o;
uniform sampler2D uSrc;
uniform vec2 uSrcSize;
uniform float uLines, uGamma;
void main() {
    float x = gl_FragCoord.x / uSrcSize.x;
    float n = floor(gl_FragCoord.y);
    float rows = uSrcSize.y / uLines;
    vec3 acc = vec3(0.0);
    for (int i = 0; i < 4; ++i) {
        float y = (n + (float(i) + 0.5) / 4.0) * rows;
        acc += pow(texture(uSrc, vec2(x, y / uSrcSize.y)).rgb, vec3(uGamma));
    }
    o = vec4(acc * 0.25, 1.0);
}
)";

// 2. horizontal: the beam's softness (luma) and the NTSC-ish chroma bleed (I / Q wider), Gaussian, 9 taps
const char* kFsH = R"(#version 330 core
out vec4 o;
uniform sampler2D uSrc;
uniform vec2 uSize;
uniform float uStep, uSoft, uBleed;   // uStep: texels per original pixel; uSoft / uBleed: sigma in original pixels
const mat3 toYiq = mat3(0.299, 0.596, 0.211, 0.587, -0.274, -0.523, 0.114, -0.322, 0.312);
const mat3 toRgb = mat3(1.0, 1.0, 1.0, 0.956, -0.272, -1.106, 0.621, -0.647, 1.703);
void main() {
    vec2 uv = gl_FragCoord.xy / uSize;
    float sy = max(uSoft, 0.05), sc = max(uSoft + uBleed, 0.05);
    float span = 2.0 * max(sy, sc);   // the taps over +-2 sigma of the wider one
    vec3 acc = vec3(0.0);
    vec2 wsum = vec2(0.0);
    for (int k = -4; k <= 4; ++k) {
        float d = float(k) * span / 4.0;   // original pixels
        vec3 yiq = toYiq * texture(uSrc, uv + vec2(d * uStep / uSize.x, 0.0)).rgb;
        float wy = exp(-0.5 * d * d / (sy * sy)), wc = exp(-0.5 * d * d / (sc * sc));
        acc += vec3(yiq.x * wy, yiq.yz * wc);
        wsum += vec2(wy, wc);
    }
    acc /= vec3(wsum.x, wsum.y, wsum.y);
    o = vec4(max(toRgb * acc, 0.0), 1.0);
}
)";

// 3. the glow: separable Gaussian (9 taps); the first pass reads the lines' mip level near 1/8
const char* kFsGlow = R"(#version 330 core
out vec4 o;
uniform sampler2D uSrc;
uniform vec2 uSize, uDir;   // uDir: one texel of this target along the pass
uniform float uLod;
void main() {
    vec2 uv = gl_FragCoord.xy / uSize;
    vec3 acc = vec3(0.0);
    float ws = 0.0;
    for (int k = -4; k <= 4; ++k) {
        float w = exp(-0.5 * float(k * k) / 4.0);
        acc += textureLod(uSrc, uv + uDir * float(k), uLod).rgb * w;
        ws += w;
    }
    o = vec4(acc / ws, 1.0);
}
)";

// 4. the window
const char* kFsMain = R"(#version 330 core
out vec4 o;
uniform sampler2D uLines, uGlow;
uniform vec2 uOut;
uniform float uL, uCurve, uScan, uBeamMin, uBeamMax, uMask, uMaskComp, uMaskPx, uBloom, uBoost, uVig, uCorner, uGammaOut;
uniform int uMaskType;
vec2 warp(vec2 uv) {
    vec2 c = uv * 2.0 - 1.0;
    c *= 1.0 + uCurve * c.yx * c.yx;
    return c * 0.5 + 0.5;
}
// a line of colour c at distance d (in lines) from its centre: a Gaussian beam, wider when brighter, its area kept
// (the line's average brightness stays c whatever the width)
vec3 beam(float d, vec3 c) {
    vec3 s = mix(vec3(uBeamMin), vec3(uBeamMax), sqrt(clamp(c, 0.0, 1.0)));
    return c * exp(-0.5 * d * d / (s * s)) / (s * 2.5066283);
}
vec3 lineAt(float n, float x) {
    return textureLod(uLines, vec2(x, (clamp(n, 0.0, uL - 1.0) + 0.5) / uL), 0.0).rgb;
}
vec3 mask(vec2 px) {
    if (uMaskType == 0) return vec3(1.0);
    float lo = 1.0 - uMask;
    vec2 q = floor(px / uMaskPx);
    float i;
    if (uMaskType == 3) i = mod(q.x + mod(q.y, 2.0) * 1.5, 3.0);   // shadow mask: dot triads, rows offset
    else i = mod(q.x, 3.0);                                         // aperture grille / slot mask: RGB stripes
    vec3 m = vec3(lo);
    m[int(floor(i))] = 1.0;
    if (uMaskType == 2) {   // slot mask: a dark gap every 4 rows, every other triad offset by half
        float col = floor(q.x / 3.0);
        if (mod(q.y + mod(col, 2.0) * 2.0, 4.0) >= 3.0) m = vec3(lo);
    }
    return m;
}
void main() {
    vec2 uv = gl_FragCoord.xy / uOut;
    vec2 p = warp(uv);
    // the screen's edge: rounded corners, 1-pixel smooth
    vec2 half_ = uOut * 0.5;
    float r = uCorner * uOut.y;
    vec2 q = abs(p - 0.5) * uOut - (half_ - r);
    float edge = clamp(0.5 - (length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r), 0.0, 1.0);
    if (edge <= 0.0) { o = vec4(0.0, 0.0, 0.0, 1.0); return; }
    // the two nearest lines' beams, averaged over the pixel's height (two samples)
    float ly = p.y * uL;
    float n = floor(ly);
    float dpx = 0.25 * uL / uOut.y;
    vec3 c0 = lineAt(n, p.x);
    float f = ly - n - 0.5;
    float n2 = n + (f < 0.0 ? -1.0 : 1.0);
    vec3 c1 = lineAt(n2, p.x);
    float d0a = f - dpx, d0b = f + dpx;
    float s2 = f < 0.0 ? 1.0 : -1.0;   // the distance to line n2's centre: d0 + s2
    vec3 lit = 0.5 * (beam(d0a, c0) + beam(d0a + s2, c1) + beam(d0b, c0) + beam(d0b + s2, c1));
    vec3 flat_ = mix(c0, c1, abs(f));
    vec3 col = mix(flat_, lit, uScan);
    col *= mask(gl_FragCoord.xy) * uMaskComp;
    col += uBloom * textureLod(uGlow, p, 0.0).rgb;
    col *= uBoost;
    if (uVig > 0.0) col *= pow(clamp(16.0 * p.x * p.y * (1.0 - p.x) * (1.0 - p.y), 0.0, 1.0), uVig);
    col = pow(clamp(col, 0.0, 1.0), vec3(1.0 / uGammaOut)) * edge;
    o = vec4(col, 1.0);
}
)";

gl::GLuint compile(gl::GLenum type, const char* src)
{
    gl::GLuint s = gl::CreateShader(type);
    gl::ShaderSource(s, 1, &src, nullptr);
    gl::CompileShader(s);
    gl::shaderLog(s, "crt");
    gl::GLint ok = 0;
    gl::GetShaderiv(s, gl::COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        gl::GetShaderInfoLog(s, sizeof log, nullptr, log);
        SDL_Log("crt: shader: %s", log);
        gl::DeleteShader(s);
        return 0;
    }
    return s;
}

gl::GLuint program(const char* fs)
{
    gl::GLuint v = compile(gl::VERTEX_SHADER, kVs), f = v ? compile(gl::FRAGMENT_SHADER, fs) : 0;
    if (!v || !f) return 0;
    gl::GLuint p = gl::CreateProgram();
    gl::AttachShader(p, v);
    gl::AttachShader(p, f);
    gl::LinkProgram(p);
    gl::programLog(p, "crt");
    gl::DeleteShader(v);
    gl::DeleteShader(f);
    gl::GLint ok = 0;
    gl::GetProgramiv(p, gl::LINK_STATUS, &ok);
    if (!ok) { SDL_Log("crt: shader link failed"); return 0; }
    return p;
}

void texture(gl::GLuint& t, int w, int h, gl::GLenum internal, gl::GLenum type, bool mips = false)
{
    if (!t) gl::GenTextures(1, &t);
    gl::BindTexture(gl::TEXTURE_2D, t);
    gl::TexImage2D(gl::TEXTURE_2D, 0, gl::GLint(internal), w, h, 0, gl::RGBA, type, nullptr);
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::GLint(mips ? gl::LINEAR_MIPMAP_LINEAR : gl::LINEAR));
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::LINEAR);
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_S, gl::CLAMP_TO_EDGE);
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_T, gl::CLAMP_TO_EDGE);
    if (mips) gl::GenerateMipmap(gl::TEXTURE_2D);
}

void target(gl::GLuint& fbo, gl::GLuint tex)
{
    if (!fbo) gl::GenFramebuffers(1, &fbo);
    gl::BindFramebuffer(gl::FRAMEBUFFER, fbo);
    gl::FramebufferTexture2D(gl::FRAMEBUFFER, gl::COLOR_ATTACHMENT0, gl::TEXTURE_2D, tex, 0);
}

void warpC(float k, float& cx, float& cy)
{
    const float x = cx, y = cy;
    cx = x * (1.0f + k * y * y);
    cy = y * (1.0f + k * x * x);
}

} // namespace

const char* crtPresetKey(int p) { return kKeys[p >= 0 && p < kCrtPresets ? p : 0]; }
const char* crtPresetName(int p) { return kNames[p >= 0 && p < kCrtPresets ? p : 0]; }
int crtPresetParse(const std::string& s)
{
    for (int p = 0; p < kCrtPresets; ++p)
        if (s == kKeys[p] || s == std::to_string(p)) return p;
    if (s == "arcade-monitor" || s == "monitor") return 1;
    if (s == "consumer" || s == "consumer-tv") return 2;
    if (s == "scanlines") return 3;
    return -1;
}
const char* crtMaskName(int t)
{
    static const char* const k[4] = {"None", "Aperture Grille", "Slot Mask", "Shadow Mask"};
    return k[std::clamp(t, 0, 3)];
}

CrtParams crtParams(const CrtSettings& s)
{
    CrtParams p;
    switch (s.preset) {
    case 1:   // Arcade Monitor: the Model 2 cabinet's medium-res 24 kHz monitor: crisp lines, aperture grille, slight curve
        p.scan = 0.85f, p.beamMin = 0.16f, p.beamMax = 0.40f, p.maskType = 1, p.mask = 0.35f, p.curve = 0.03f;
        p.bloom = 0.10f, p.boost = 1.12f, p.hsoft = 0.35f, p.bleed = 0.0f, p.vignette = 0.10f, p.corner = 0.015f;
        p.gammaIn = 2.4f, p.gammaOut = 2.2f;
        break;
    case 2:   // Consumer TV (the PC port's era, through a TV): softer, wider beams, shadow mask, colour bleed, more curve
        p.scan = 0.55f, p.beamMin = 0.24f, p.beamMax = 0.50f, p.maskType = 3, p.mask = 0.28f, p.curve = 0.06f;
        p.bloom = 0.18f, p.boost = 1.12f, p.hsoft = 0.75f, p.bleed = 1.4f, p.vignette = 0.20f, p.corner = 0.035f;
        p.gammaIn = 2.4f, p.gammaOut = 2.2f;
        break;
    case 3:   // Sharp Scanlines: dark gaps between lines, nothing else
        p.scan = 1.0f, p.beamMin = 0.14f, p.beamMax = 0.30f, p.maskType = 0, p.mask = 0.0f, p.curve = 0.0f;
        p.bloom = 0.04f, p.boost = 1.0f, p.hsoft = 0.0f, p.bleed = 0.0f, p.vignette = 0.0f, p.corner = 0.0f;
        p.gammaIn = 2.2f, p.gammaOut = 2.2f;
        break;
    case 4: {   // Custom: the Arcade Monitor's beam and gamma, the steps the player chose
        CrtSettings a;
        a.preset = 1;
        p = crtParams(a);
        auto st = [](int v) { return float(std::clamp(v, 0, 10)) / 10.0f; };
        p.scan = st(s.scan);
        p.mask = 0.6f * st(s.mask);
        p.maskType = std::clamp(s.maskType, 0, 3);
        p.curve = 0.1f * st(s.curve);
        p.bloom = 0.4f * st(s.bloom);
        p.boost = 0.85f + 0.5f * st(s.bright);   // 5: x1.1 (the beam and mask darken a little)
        p.corner = 0.004f + 0.3f * p.curve;
        p.vignette = 1.5f * p.curve + 0.04f;
        break;
    }
    default:
        break;
    }
    return p;
}

Crt::~Crt() = default;   // GL objects go with the context

void Crt::configure(const CrtSettings& s, int lines, int across)
{
    set_ = s;
    p_ = crtParams(s);
    lines_ = std::max(1, lines);
    across_ = std::max(1, across);
    if (s.preset == 0) { on_ = false; return; }
    if (!tried_) { tried_ = true; ok_ = build(); }
    on_ = ok_;
}

bool Crt::build()
{
    progLines_ = program(kFsLines);
    progH_ = program(kFsH);
    progGlow_ = program(kFsGlow);
    progMain_ = program(kFsMain);
    if (!progLines_ || !progH_ || !progGlow_ || !progMain_) { SDL_Log("crt: the filter is off (the shaders failed)"); return false; }
    gl::GenVertexArrays(1, &vao_);
    auto unit = [](gl::GLuint prog, const char* name, int u) { gl::UseProgram(prog); gl::Uniform1i(gl::GetUniformLocation(prog, name), u); };
    unit(progLines_, "uSrc", 0);
    unit(progH_, "uSrc", 0);
    unit(progGlow_, "uSrc", 0);
    unit(progMain_, "uLines", 0);
    unit(progMain_, "uGlow", 1);
    gl::UseProgram(0);
    return true;
}

void Crt::resize(int ww, int wh)
{
    if (ww != w_ || wh != h_) {
        w_ = ww, h_ = wh;
        texture(sceneTex_, ww, wh, gl::RGBA8, gl::UNSIGNED_BYTE);
        target(sceneFbo_, sceneTex_);
        if (!sceneDepth_) gl::GenRenderbuffers(1, &sceneDepth_);
        gl::BindRenderbuffer(gl::RENDERBUFFER, sceneDepth_);
        gl::RenderbufferStorage(gl::RENDERBUFFER, gl::DEPTH_COMPONENT24, ww, wh);
        gl::FramebufferRenderbuffer(gl::FRAMEBUFFER, gl::DEPTH_ATTACHMENT, gl::RENDERBUFFER, sceneDepth_);
        // compat (2026-10-29): a target this driver cannot draw into (RGBA16F lines / glow, the scene) turns the filter
        // off (logged) instead of drawing into nothing
        ok_ = gl::framebufferOk("CRT filter's scene") && ok_;
        gw_ = std::max(1, ww / 8), gh_ = std::max(1, wh / 8);
        for (int i = 0; i < 2; ++i) {
            texture(glowTex_[i], gw_, gh_, gl::RGBA16F, gl::HALF_FLOAT);
            target(glowFbo_[i], glowTex_[i]);
            ok_ = gl::framebufferOk("CRT filter's glow") && ok_;
        }
    }
    if (ww != lw_ || lines_ != lh_) {
        lw_ = ww, lh_ = lines_;
        for (int i = 0; i < 2; ++i) {
            texture(lineTex_[i], ww, lines_, gl::RGBA16F, gl::HALF_FLOAT, i == 1);
            target(lineFbo_[i], lineTex_[i]);
            ok_ = gl::framebufferOk("CRT filter's lines") && ok_;
        }
    }
}

bool Crt::begin(int ww, int wh)
{
    if (!on_ || ww <= 0 || wh <= 0) return false;
    resize(ww, wh);
    if (!ok_) {   // compat: a framebuffer failed (resize): the filter stays off
        on_ = false;
        gl::BindFramebuffer(gl::FRAMEBUFFER, 0);
        return false;
    }
    gl::BindFramebuffer(gl::FRAMEBUFFER, sceneFbo_);
    gl::Viewport(0, 0, ww, wh);
    return true;
}

void Crt::present(int ww, int wh, unsigned out)
{
    if (!on_) return;
    resize(ww, wh);
    gl::Disable(gl::DEPTH_TEST);
    gl::Disable(gl::BLEND);
    gl::Disable(gl::CULL_FACE);
    gl::Disable(gl::SCISSOR_TEST);
    gl::BindVertexArray(vao_);
    gl::ActiveTexture(gl::TEXTURE0);
    auto u1 = [](gl::GLuint pr, const char* n, float v) { gl::Uniform1f(gl::GetUniformLocation(pr, n), v); };
    auto u2 = [](gl::GLuint pr, const char* n, float x, float y) { gl::Uniform2f(gl::GetUniformLocation(pr, n), x, y); };
    const float L = float(lines_);
    // 1. lines
    gl::BindFramebuffer(gl::FRAMEBUFFER, lineFbo_[0]);
    gl::Viewport(0, 0, ww, lines_);
    gl::UseProgram(progLines_);
    gl::BindTexture(gl::TEXTURE_2D, sceneTex_);
    u2(progLines_, "uSrcSize", float(ww), float(wh));
    u1(progLines_, "uLines", L);
    u1(progLines_, "uGamma", p_.gammaIn);
    gl::DrawArrays(gl::TRIANGLE_STRIP, 0, 4);
    // 2. horizontal (one original pixel: the 4:3 screen's width / its pixels across)
    gl::BindFramebuffer(gl::FRAMEBUFFER, lineFbo_[1]);
    gl::UseProgram(progH_);
    gl::BindTexture(gl::TEXTURE_2D, lineTex_[0]);
    u2(progH_, "uSize", float(ww), L);
    u1(progH_, "uStep", float(wh) * 4.0f / 3.0f / float(across_));
    u1(progH_, "uSoft", p_.hsoft);
    u1(progH_, "uBleed", p_.bleed);
    gl::DrawArrays(gl::TRIANGLE_STRIP, 0, 4);
    // 3. glow
    if (p_.bloom > 0.0f) {
        gl::BindTexture(gl::TEXTURE_2D, lineTex_[1]);
        gl::GenerateMipmap(gl::TEXTURE_2D);
        gl::UseProgram(progGlow_);
        gl::Viewport(0, 0, gw_, gh_);
        u2(progGlow_, "uSize", float(gw_), float(gh_));
        gl::BindFramebuffer(gl::FRAMEBUFFER, glowFbo_[0]);
        u2(progGlow_, "uDir", 1.0f / float(gw_), 0.0f);
        u1(progGlow_, "uLod", std::max(0.0f, std::log2(float(ww) / float(gw_))));
        gl::DrawArrays(gl::TRIANGLE_STRIP, 0, 4);
        gl::BindFramebuffer(gl::FRAMEBUFFER, glowFbo_[1]);
        gl::BindTexture(gl::TEXTURE_2D, glowTex_[0]);
        u2(progGlow_, "uDir", 0.0f, 1.0f / float(gh_));
        u1(progGlow_, "uLod", 0.0f);
        gl::DrawArrays(gl::TRIANGLE_STRIP, 0, 4);
    }
    // 4. the window
    gl::BindFramebuffer(gl::FRAMEBUFFER, out);
    gl::Viewport(0, 0, ww, wh);
    gl::UseProgram(progMain_);
    gl::ActiveTexture(gl::TEXTURE0 + 1);
    gl::BindTexture(gl::TEXTURE_2D, glowTex_[1]);
    gl::ActiveTexture(gl::TEXTURE0);
    gl::BindTexture(gl::TEXTURE_2D, lineTex_[1]);
    const gl::GLuint m = progMain_;
    u2(m, "uOut", float(ww), float(wh));
    u1(m, "uL", L);
    u1(m, "uCurve", p_.curve);
    u1(m, "uScan", p_.scan);
    u1(m, "uBeamMin", p_.beamMin);
    u1(m, "uBeamMax", p_.beamMax);
    gl::Uniform1i(gl::GetUniformLocation(m, "uMaskType"), p_.mask > 0.0f ? p_.maskType : 0);
    u1(m, "uMask", p_.mask);
    // the mask's average (lit 1, the other two lo; the slot mask's gap row all lo) compensated
    const float lo = 1.0f - p_.mask;
    float mean = (1.0f + 2.0f * lo) / 3.0f;
    if (p_.maskType == 2) mean = 0.75f * mean + 0.25f * lo;
    u1(m, "uMaskComp", p_.mask > 0.0f && p_.maskType > 0 ? 1.0f / mean : 1.0f);
    // one triad = 3 display pixels up to 1440 lines, 6 at 2160 (the same mask pitch on the screen)
    u1(m, "uMaskPx", float(std::max(1, int(std::lround(float(wh) / 1440.0f)))));
    u1(m, "uBloom", p_.bloom);
    u1(m, "uBoost", p_.boost);
    u1(m, "uVig", p_.vignette);
    u1(m, "uCorner", p_.corner);
    u1(m, "uGammaOut", p_.gammaOut);
    gl::DrawArrays(gl::TRIANGLE_STRIP, 0, 4);
    gl::UseProgram(0);
    gl::BindVertexArray(0);
}

unsigned Crt::scratch(int ww, int wh)
{
    if (ww != ow_ || wh != oh_) {
        ow_ = ww, oh_ = wh;
        texture(outTex_, ww, wh, gl::RGBA8, gl::UNSIGNED_BYTE);
        target(outFbo_, outTex_);
    }
    return outFbo_;
}

std::vector<uint8_t> Crt::readBack(int ww, int wh)
{
    std::vector<uint8_t> px(size_t(ww) * size_t(wh) * 3), rows(px.size());
    gl::PixelStorei(gl::PACK_ALIGNMENT, 1);
    gl::ReadPixels(0, 0, ww, wh, gl::RGB, gl::UNSIGNED_BYTE, px.data());
    for (int y = 0; y < wh; ++y)
        std::copy_n(px.begin() + ptrdiff_t(size_t(wh - 1 - y) * size_t(ww) * 3), size_t(ww) * 3,
                    rows.begin() + ptrdiff_t(size_t(y) * size_t(ww) * 3));
    return rows;
}

std::vector<uint8_t> Crt::filterImage(const std::vector<uint8_t>& rgb, int w, int h)
{
    if (!on_ || rgb.size() < size_t(w) * size_t(h) * 3) return {};
    resize(w, h);
    std::vector<uint8_t> flip(rgb.size());
    for (int y = 0; y < h; ++y)
        std::copy_n(rgb.begin() + ptrdiff_t(size_t(h - 1 - y) * size_t(w) * 3), size_t(w) * 3, flip.begin() + ptrdiff_t(size_t(y) * size_t(w) * 3));
    gl::BindTexture(gl::TEXTURE_2D, sceneTex_);
    gl::PixelStorei(gl::UNPACK_ALIGNMENT, 1);
    gl::TexImage2D(gl::TEXTURE_2D, 0, gl::GLint(gl::RGBA8), w, h, 0, gl::RGB, gl::UNSIGNED_BYTE, flip.data());
    return presentToImage(w, h, [] {});
}

void Crt::toSource(float& x, float& y, int ww, int wh) const
{
    if (!on_ || p_.curve <= 0.0f || ww <= 0 || wh <= 0) return;
    float cx = x / float(ww) * 2.0f - 1.0f, cy = y / float(wh) * 2.0f - 1.0f;
    warpC(p_.curve, cx, cy);
    x = (cx + 1.0f) * 0.5f * float(ww);
    y = (cy + 1.0f) * 0.5f * float(wh);
}

void Crt::toScreen(float& x, float& y, int ww, int wh) const
{
    if (!on_ || p_.curve <= 0.0f || ww <= 0 || wh <= 0) return;
    const float px = x / float(ww) * 2.0f - 1.0f, py = y / float(wh) * 2.0f - 1.0f;
    float cx = px, cy = py;
    for (int i = 0; i < 12; ++i) {   // fixed point: c = p / (1 + k c.yx^2) (k <= 0.1: converges fast)
        const float nx = px / (1.0f + p_.curve * cy * cy), ny = py / (1.0f + p_.curve * cx * cx);
        cx = nx, cy = ny;
    }
    x = (cx + 1.0f) * 0.5f * float(ww);
    y = (cy + 1.0f) * 0.5f * float(wh);
}

double Crt::benchPresent(int ww, int wh, int reps)
{
    if (!on_ || reps <= 0) return 0.0;
    const unsigned t = scratch(ww, wh);
    uint8_t px[4];
    present(ww, wh, t);
    gl::ReadPixels(0, 0, 1, 1, gl::RGBA, gl::UNSIGNED_BYTE, px);   // warm, and wait
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < reps; ++i) present(ww, wh, t);
    gl::ReadPixels(0, 0, 1, 1, gl::RGBA, gl::UNSIGNED_BYTE, px);
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / double(reps);
}

} // namespace hotd
