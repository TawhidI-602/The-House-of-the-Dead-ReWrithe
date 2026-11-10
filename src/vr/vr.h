// VR mode (2026-11-03, user: "using the player hand and gun along with the crosshair shown"; docs/vr.md).
//
// The game stays the arcade game: its camera is authoritative (on rails), the player's head turns and leans on top of
// it, the right controller is the light gun (its ray picks the arcade screen point the game shoots at), the 2D HUD
// sits on the arcade's own screen plane in front of the player. This header is the backend-independent part:
//  - the per-frame data a headset runtime hands over (head pose, two eyes with their own frusta and render targets,
//    controller poses and buttons), in the runtime's tracking space: OpenXR's convention (right-handed, metres,
//    +x right, +y up, -z ahead; quaternions x, y, z, w);
//  - Backend: "none" (VR off: nothing of this runs), "preview" (a desktop simulation: side-by-side stereo in the window,
//    the head on keys, the gun on the mouse; vr_preview.cpp) and "openxr" (vr_openxr.cpp: a headset; built when CMake
//    finds the OpenXR SDK: HOTD_OPENXR);
//  - Settings (comfort, scale, aim, the gun / hand models: data/vr/vr.ini).
// The game world is left-handed (+z ahead of the camera, game units): vr_math.h converts.
#pragma once

#include <cstdint>
#include <memory>
#include <string>

union SDL_Event;

namespace hotd::vr {

struct Pose {
    float pos[3] = {0, 0, 0};       // metres, tracking space
    float rot[4] = {0, 0, 0, 1};    // unit quaternion x, y, z, w (tracking space)
    bool valid = false;
};

// an eye's frustum: the tangents of its edge angles (OpenXR XrFovf: angleLeft / angleDown negative)
struct Fov {
    float tanLeft = -1.0f, tanRight = 1.0f, tanUp = 1.0f, tanDown = -1.0f;
};

struct Eye {
    Pose pose;                  // the eye in tracking space (the head pose + half the IPD)
    Fov fov;
    int width = 0, height = 0;  // its render target's size (pixels)
};

struct Controller {
    bool active = false;
    Pose grip;                  // OpenXR /input/grip/pose: the hand's palm (the gun is held there)
    Pose aim;                   // OpenXR /input/aim/pose: the pointing ray (-z); the preview sets it = the gun's barrel
    float trigger = 0.0f;       // 0..1
    float squeeze = 0.0f;       // 0..1 (the grip button)
    bool primary = false, secondary = false, menu = false, stickClick = false;   // A / B (X / Y), menu, thumbstick press
                                                                                 // (openxr: held 1 s recentres)
    float stick[2] = {0, 0};
};

struct FrameState {
    bool shouldRender = true;   // OpenXR xrWaitFrame's shouldRender (false: the headset is not showing us; submit empty)
    int64_t displayTime = 0;    // the predicted display time (OpenXR XrTime; the preview: SDL nanoseconds)
    Pose head;
    Eye eye[2];                 // 0 left, 1 right
    Controller hand[2];         // 0 left, 1 right (the gun)
};

// comfort and play settings (data/vr/vr.ini [vr]; docs/vr.md "Comfort")
struct Settings {
    // how the player's space follows the game camera: 0 follow (the game camera's full orientation, the head on top;
    // the arcade's framing), 1 level horizon (the game camera's position and heading only: its pitch is not forced on
    // the player's head; comfortable on the arcade's steep looks up / down)
    int follow = 0;
    bool vignette = false;          // (off by default, user 2026-11-10) darken the view's edges while the game camera turns / moves fast
    float vignetteStrength = 0.7f;  // 0..1 at the fastest
    float vignetteTurn = 60.0f;     // degrees / second where the vignette starts
    float unitsPerMetre = 10.0f;    // game units per metre (the world's scale; docs/vr.md "Scale")
    float panelDistance = 3.0f;     // metres: the HUD's screen plane (the arcade screen) in front of the player
    bool panelDepthTest = false;    // the HUD hidden behind nearer walls (off: always on top, the usual VR HUD)
    float aimDistance = 8.0f;       // metres: the aim's distance when the ray hits nothing the eye's depth knows
    bool laser = true;              // a faint laser from the muzzle to the crosshair
    bool crosshair = true;          // vrsight (user 2026-11-10): the 3D crosshair (off: none drawn, the aim still works)
    float crosshairDegrees = 2.5f;  // the 3D crosshair's angular size (the same size at any distance)
    bool reloadDown = true;         // pointing the gun steeply down reloads (as a shot off the screen)
    float reloadPitch = 55.0f;      // degrees below the horizon for reloadDown / the off-screen state
    // vr4 (2026-11-09, docs/vr.md "Off the screen"): what a pull off the arcade's 4:3 screen does. 0 below (the
    // default): only aimed below the screen (or the gun pointed down) it reloads; at the world beside / behind / above
    // the screen it does nothing (the crosshair greyed: the world all around is drawn, and shooting a creature beside
    // you reloaded). 1 any: every pull off the screen reloads (the light gun's rule, as before)
    int offscreenReload = 0;
    // vr4 (docs/vr.md "The HUD and close creatures"): 1 adaptive (the default): the HUD's screen plane eases from
    // panel_distance to just in front of the nearest creature inside it (never nearer than hudMinDistance metres; its
    // angular size kept); 0 fixed at panel_distance (as before)
    int hudDistance = 1;
    float hudMinDistance = 0.7f;
    // vr4 (docs/vr.md "Aim depth"): how far the gun's ray goes: 0 auto (the eye's depth where it can be read, else the
    // CPU ray test: OpenGL ES / ANGLE), 1 depth (the eye's depth only; the fixed aim_distance where unreadable), 2 cpu
    // (the CPU ray against the stage's triangles and the creatures' parts; a desktop test of the Quest's path), 3 fixed
    // (aim_distance always: OpenGL ES before vr4, the tests' baseline)
    int aimDepth = 0;
    // vr4 (docs/vr.md "Shot flash"): the game's shot flash / spark (drawn 1 unit = 10 cm in front of the game camera:
    // the size the flat picture wants; in VR before your eyes) moved out to where the shot's ray meets the world, the
    // same size to the game camera
    bool shotFlashAtHit = true;
    // vr4 (docs/quest.md "Refresh rate"): the display's refresh rate asked of the runtime (XR_FB_display_refresh_rate,
    // when it has it): 0 auto (on the Quest 90, or the highest under it; on a PC the runtime's own), or 72 / 90 / 120
    // (the nearest offered). sustainedPerformance: XR_EXT_performance_settings' sustained high for the CPU and the GPU
    int refreshRate = 0;
    bool sustainedPerformance = true;
    // vr4 (docs/vr.md "Recentre"): the headset back after more than this many seconds away (taken off, the app hidden;
    // not the runtime's dashboard over the game): the seat recentred by itself; 0 never
    float autoRecentre = 3.0f;
    // shake-to-reload (user 2026-11-09; docs/vr.md "Shake to reload"): a back-and-forth shake of a gun controller
    // reloads. shakeSensitivity: the muzzle's speed (m/s) both swings must pass (lower: easier). On by default since
    // user 2026-11-10
    bool shakeReload = true;
    float shakeSensitivity = 3.5f;
    // wrist-flick reload (user 2026-11-10; docs/vr.md "Wrist flick to reload", vr_aim.h FlickDetector): one sharp
    // upward snap of the gun's muzzle reloads. flickSensitivity: the pitch rate (deg/s) the snap must reach (lower:
    // easier). The shake's and the flick's 0.7 s cooldown is shared (one gesture = one reload)
    bool flickReload = true;
    float flickSensitivity = 600.0f;
    // vr3 (user 2026-11-09, docs/vr.md "All around you"): the world in every direction: the neighbouring draw zones'
    // pieces outside the game camera's view (GameView::vrAround). Off: only what the arcade's zone list draws
    bool around = true;
    // vr3 (user 2026-11-09, docs/vr.md "Cut-scenes"): 0 world (you stand where the game's camera is and look around
    // freely, the default), 1 cinema (the flat picture, bars and captions as the arcade shows them, on a big screen
    // in front of you while a cut-scene plays: for comfort when the camera cuts fast); cinemaDistance / cinemaWidth:
    // its place (metres ahead of the seat, its width)
    int cutscenes = 0;
    float cinemaDistance = 3.0f;
    float cinemaWidth = 4.0f;
    // vrnull (user 2026-11-09, docs/vr.md "The desktop window"): the game's window during VR: 0 eye (one eye, the
    // left, filling the window cropped to its shape), 1 both (the two eyes side by side), 2 spectator (the flat game's
    // picture from the game camera with the HUD and player 1's sight: for watching / streaming). The preview's own
    // picture stays --vr-eye's (spectator works there too)
    int desktopView = 0;
    bool leftHand = true;           // draw the left hand (when tracked)
    // dual pistols (player request 2026-11-04; docs/vr.md "Dual pistols"): a gun in each hand, each controller aims and
    // fires for itself (its own laser, crosshair, trigger, rumble), one magazine for the pair; the left gun is the
    // game's gun hand 0x719 mirrored. Off (the default): one gun, the left hand empty. Survival's Double Pistols
    // weapon is always two guns in VR
    bool dualPistols = false;
    // the game's hand models on the controllers (vr_scene.cpp's anchors) moved / turned from their default place:
    // metres along the pose's axes (x right, y up, -z ahead), degrees pitch / yaw / roll (vr.ini gun_offset,
    // gun_rotate, left_hand_offset, left_hand_rotate: "x y z")
    float gunOffset[3] = {0, 0, 0}, gunRotate[3] = {0, 0, 0};
    float leftOffset[3] = {0, 0, 0}, leftRotate[3] = {0, 0, 0};
    float ipd = 0.064f;             // the preview's eye distance (metres; a headset reports its own)
    // quest (2026-11-03, docs/quest.md): the flat screens (setup, menus, the end screen) on a big screen in VR, the
    // right controller's ray as the mouse (trigger = click; B / Y / menu = Esc; A = Enter). Default on Android only
    // (a PC player has the desktop window and the mouse)
#if defined(__ANDROID__)
    bool menuPanel = true;
#else
    bool menuPanel = false;
#endif
    float menuDistance = 2.5f;      // metres ahead of the start pose
    float menuWidth = 3.2f;         // metres (the height from the window's shape)
    // performance (the Quest 3: 72 Hz): the eyes' resolution x this (0.5..1.5 of the runtime's recommended size);
    // the HUD panel's lines (960: sharp; 720 / 540 cheaper: it is mipmapped every frame); the flat screens' 3D with the
    // arcade's polygon sort (the Draw Depth setting; off: the depth buffer, cheaper on OpenGL ES where the sort's keys
    // are a float texture updated per frame; VR's eyes never sort); srgb_swapchain off: an RGBA8 swapchain (a check
    // on the headset if the colours look wrong)
    float resolutionScale = 1.0f;
    int hudLines = 960;
#if defined(__ANDROID__)
    bool arcadeSort = false;
#else
    bool arcadeSort = true;
#endif
    bool srgbSwapchain = true;
    bool load(const std::string& path);   // missing keys keep their defaults; false: no file
    void set(const std::string& key, const std::string& value);   // one vr.ini key (vr3: also --vr-set key=value)
};

// the per-frame interface every backend implements (GL context current on the calling thread)
class Backend {
public:
    virtual ~Backend() = default;
    virtual const char* name() const = 0;
    // after the GL context exists
    virtual bool init(const Settings& s, std::string& err) = 0;
    virtual void shutdown() = 0;
    // window events (the preview's keys / mouse); true: used (not passed on to the game)
    virtual bool event(const SDL_Event&) { return false; }
    // the poses for the frame about to be drawn (OpenXR: xrWaitFrame + xrBeginFrame + xrLocateViews + xrSyncActions)
    virtual bool beginFrame(FrameState& fs) = 0;
    // the GL framebuffer (colour + depth) to draw eye `i` into, sized fs.eye[i].width x height (OpenXR: the acquired
    // swapchain image wrapped in an FBO)
    virtual unsigned eyeFramebuffer(int i) = 0;
    virtual unsigned eyeTexture(int i) = 0;   // its colour texture (the desktop mirror)
    // the eyes are drawn: hand them over (OpenXR: release the images, xrEndFrame with a projection layer)
    virtual void endFrame(const FrameState& fs) = 0;
    // the desktop window's picture (the preview: the stereo pair; OpenXR: a mirror of one eye) into framebuffer `fbo`
    virtual void present(unsigned fbo, int width, int height) = 0;
    // openxr: a controller's rumble (hand 0 left, 1 right; amplitude 0..1): the gun's kick on a shot
    virtual void haptic(int, float, float) {}
    // openxr: the runtime ended the session for good (EXITING / LOSS_PENDING: quit from its dashboard, the headset
    // unplugged): the game drops VR and goes on flat (main.cpp vrFrame)
    virtual bool lost() const { return false; }
    // openxr: the headset paces the loop (xrWaitFrame; the backend's own ~90 Hz wait while the headset shows nothing):
    // main.cpp applyPresentation drops its frame limiter and the window's vsync then (vr2 2026-11-09: they capped a
    // 90 Hz headset at the monitor's 60)
    virtual bool pacesFrames() const { return false; }

    // ---- the desktop preview's extras (a headset backend leaves them alone) ----
    // the window's size before beginFrame (the preview sizes its eyes from it)
    virtual void setOutputSize(int, int) {}
    // the mouse as a ray from the eye it is over (tracking space, unit dir); the scene measures how far it reaches in
    // that eye's depth and hands it back (cursorDistance), so the preview's gun aims at what the cursor shows
    virtual bool desktopCursorRay(float[3], float[3]) { return false; }
    virtual void cursorDistance(float) {}
    // tests: the cursor put at (u, v) of eye `eye`'s picture (0..1 from the top-left); the head held at yaw / pitch /
    // roll (degrees, right-handed: +yaw turns left) and leaned by (x, y, z) metres
    virtual void setCursor(int, float, float) {}
    virtual void scriptHead(const float[3], const float[3]) {}
    // dual pistols: the left hand holds a gun too (the preview points it at the cursor's point as the right one)
    virtual void setDual(bool) {}
    // tests: hand `hand`'s trigger held at `value` (0..1) until changed (the preview; a headset reads its own)
    virtual void scriptTrigger(int, float) {}
    int display = 2;   // the desktop picture: 0 the left eye, 1 the right eye, 2 both side by side
};

std::unique_ptr<Backend> makePreview();
std::unique_ptr<Backend> makeOpenXr();   // null when built without HOTD_OPENXR
std::unique_ptr<Backend> make(const std::string& name);   // "preview" / "openxr"; null: unknown / not built

} // namespace hotd::vr
