# Renderer backends: OpenGL and DirectX (ANGLE)

(angle, 2026-10-29.) The game draws with OpenGL 3.3. PCs from before about 2012 often have graphics whose drivers
never got OpenGL 3.3 (Intel HD 2000 / 3000 on older drivers, older AMD / NVIDIA cards with dropped drivers) but do
have DirectX 10.1 or 11. For those the Windows build can draw through **ANGLE** (Google's OpenGL ES to Direct3D
translator, BSD licence, the one Chrome uses): OpenGL ES 3.0 on top of Direct3D 11.

## Choosing

- **Options > Video > Renderer**: Auto / OpenGL / DirectX (ANGLE). Applies at the next start; the help line says
  which one runs now. launcher.ini `renderer=auto|gl|angle`, command line `--renderer auto|gl|angle`.
- **Auto** (default): OpenGL 3.3 core if the driver gives it (nothing changes on PCs that run the game today), else
  DirectX through ANGLE. The fallback starts SDL's video part again and opens a new window (same title, place, size,
  flags; the icon set again), because SDL prepares a window for EGL only when it is created.
- **DirectX (ANGLE)** chosen but ANGLE can't make its window (Direct3D 10.0 / 9 graphics): the game logs it and
  uses OpenGL.
- Neither works: the start message says the graphics need OpenGL 3.3 or DirectX 10.1, and what to update.
- `HOTD_GL_FAIL=1` (tests): the OpenGL attempt fails as on old graphics (Auto's fallback).
  `HOTD_ANGLE_PLATFORM=d3d11|d3d9|warp`, `HOTD_ANGLE_FL=10_0|10_1|11_0` (tests): ANGLE's Direct3D version, its
  software rasteriser (WARP), or the highest Direct3D 11 feature level (old graphics simulated on a new card).

## How it works (src/render/gl_backend.cpp)

The renderer is written once against `render/gl.h` (function pointers loaded through SDL). On ANGLE the context is
OpenGL ES 3.0 (SDL: `SDL_HINT_OPENGL_ES_DRIVER`, `SDL_GL_EGL_PLATFORM` = `EGL_PLATFORM_ANGLE_ANGLE`, the platform
attributes from `SDL_EGL_SetAttributeCallbacks`), and `gl::loadES()` swaps a few of gl.h's pointers for ES versions.
The desktop path never goes through it: same pointers, same shader text as before.

| Desktop OpenGL 3.3 | On ES 3.0 (ANGLE) |
|---|---|
| `#version 330 core` shaders | rewritten when compiled: `#version 300 es`, `precision highp` for float / int / every sampler type used, `#line 2` (error lines match the source) |
| `samplerBuffer` + `texelFetch(buf, i)` (Arcade Sort's polygon keys) | a 2D RGBA32F texture of 2048-texel rows (`hotd_tb(s, i)` in the shader); the buffer's data kept on the CPU, its changed rows uploaded on `glBufferSubData` |
| `GL_DEPTH_CLAMP` | `GL_EXT_depth_clamp` (ANGLE's Direct3D 11 has it: the rasteriser's depth clip off). Without it (Direct3D 9) the near / far planes clip as without clamp |
| `glDepthRange(double)` | `glDepthRangef` |
| `glReadPixels` RGB, `glTexImage2D` RGB data into RGBA8 | read / uploaded as RGBA, converted on the CPU (screenshots, the CRT's still) |
| GPU timer queries (menu tour's GPU column) | skipped: 0 ms, always ready |
| a fragment output indexed by a loop variable | not allowed in GLSL ES: the grey look's sepia loop writes a `vec3` first (the same result on desktop; 40 renders byte-identical) |

What the renderer did not need: `glPolygonMode`, `glDrawBuffer`, texture borders / clamp-to-border, BGRA or 16-bit
packed formats (none used). `gl_FragDepth` exists in ES 3.0 (the key-band and per-pixel sort programs compile; the default Arcade Sort path is the one rendered above).

**Shader compile time.** ANGLE turns every shader into HLSL and compiles it with d3dcompiler_47: ~1.3-1.9 s for the
game's programs on the RTX 5080 each time a chapter's view is made (OpenGL: ~0.1 s). ANGLE's compiled programs are
therefore kept (EGL_ANDROID_blob_cache): in memory (the next view: 13 ms) and appended to the cache folder's
`angle_shaders.bin` (the next start: 13 ms; ~90 KB; a damaged file or one over 64 MB is started again). ANGLE checks
its own version and the graphics in each entry (a new driver / GPU just compiles again).

## The DLLs

`bin\libEGL.dll`, `bin\libGLESv2.dll` and `bin\z.dll` (ANGLE chromium/7258 and its zlib), about **5.7 MB, 2.1 MB in
the zip**. Built by vcpkg's `angle` port from Google's sources (release, x64, dynamic MSVC runtime: the package's
runtime DLLs serve it). Not in git: CMake copies them beside hotd.exe from `HOTD_ANGLE_DIR` (default
`private\downloads\angle`; the main checkout has them), `tools\make_portable_hotd.ps1` packages them (a warning if a
build lacks them), the release workflow builds them with the runner's vcpkg (optional step). Licences: NOTICE.md
(ANGLE BSD-3-Clause, xxHash BSD-2-Clause inside it, zlib).

To rebuild them: `git clone https://github.com/microsoft/vcpkg`, `bootstrap-vcpkg.bat`, a triplet with
`VCPKG_BUILD_TYPE release` (or plain `x64-windows`), `vcpkg install angle:<triplet>`; copy `bin\libEGL.dll`,
`libGLESv2.dll`, `z.dll` (vcpkg wants a path without spaces: ~3 min on a 16-thread CPU).

- **d3dcompiler_47.dll** is not bundled: Windows 8.1, 10 and 11 have it in System32 (SDL loads it before ANGLE).
- **Windows 7 is not supported** by this ANGLE (it imports Windows 8 API sets, as current Chrome does). Pre-2012 PCs
  running Windows 10 are the target.

## What works where (tested on this PC: RTX 5080, Windows 11)

| Level | How simulated | Result |
|---|---|---|
| Direct3D 11, feature level 11_x | `--renderer angle` | everything: input test 2130 checks 0 failed; Chapter 1 renders (arcade and PC models, 40 frames each) and the CRT presets match OpenGL to rounding (PSNR 43-70 dB: triangle-edge pixels only, looked at); the menu tour |
| feature level 10_1 (Intel HD 2000 / 3000, Radeon HD 3000-4000, GeForce GT 2xx) | `HOTD_ANGLE_FL=10_1` | works: shader model 4.1; its 40 renders byte-identical to 11_x |
| feature level 10_0 (GeForce 8 / 9, Radeon HD 2000, Intel GMA X4500) | `HOTD_ANGLE_FL=10_0` | no: ANGLE offers only OpenGL ES 2.0 there; the game needs ES 3.0 (integer textures, uint vertex data). Auto: the "needs OpenGL 3.3 or DirectX 10.1" message; DirectX chosen: falls back to OpenGL |
| Direct3D 9 (GMA 950 / 3100, older) | `HOTD_ANGLE_PLATFORM=d3d9` | no (ES 2.0 only), same as 10_0 |
| WARP (Microsoft's software Direct3D 11) | `HOTD_ANGLE_PLATFORM=warp` | works, slow: ~32 ms a frame at 992 x 768 on a 16-thread CPU (counted as integrated graphics: depth pre-pass on) |

Not tested: any real old PC (NVIDIA only so far); AMD / Intel Direct3D drivers through ANGLE.
