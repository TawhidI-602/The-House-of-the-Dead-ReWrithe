# Meta Quest 3 (standalone)

Worker "quest", 2026-11-03. Goal: the game in VR on a Meta Quest 3 with no PC: an Android APK (arm64-v8a) sideloaded
over USB. **Status: the APK builds and its contents are checked; nothing has run on a headset yet** (no headset was
connected while it was made). The Windows build is unchanged (the regression runs give the same numbers).
2026-11-09 (worker "vrship"): rebuilt from that day's main (v1.3.1 + the Rainbow Blood tint, VR "all around", cinema
mode, palOver / pcTint: their shaders compile and draw through the same OpenGL ES 3.0 layer on ANGLE, 0 GL errors,
including the VR preview with `around` and `cinema` on); the first-timer guide below; the game files can be copied with
Windows Explorer or SideQuest (no adb needed); the INTERNET permission (online co-op). **Still never run on a headset.**
A built APK for testing: `<main checkout>\private\portable\quest\` (never committed).

2026-11-10 (worker "quest"): rebuilt from that day's main (vr2 - vr4, vrnull, vrflick, vrfade, vrattract, vrsight: they
compile for Android / OpenGL ES as they are); on the headset the CPU aim ray from the first frame (no depth pass, no
failed depth read), and the session's end (Quit in the Meta menu) now ends the app instead of falling back to a flat
game no one can see. **An experimental test build: still never run on a headset.**

## Install on your Quest 3 (short)

1. **Developer mode:** make a free developer account at developers.meta.com with your headset's Meta account; then the
   phone's **Meta Horizon** app > Devices > your Quest 3 > Headset Settings > **Developer Mode** on; restart the headset.
2. **Install:** cable the headset to the PC, answer **Allow USB debugging** (Always allow) in the headset, then drag
   the APK onto **SideQuest** (sidequestvr.com), or `adb install -r HOTD-ReWrithe-quest-<version>-debug.apk`.
3. **Start it once:** App Library > the filter (All) > **Unknown Sources** > **HOTD ReWrithe**. It shows "needs your
   own game files" and makes its folder.
4. **Copy your game files** to `Quest 3\Internal shared storage\Android\data\com.tawhid.hotdrewrithe\files` (Windows
   Explorer after "Allow access to data", or SideQuest's file manager; adb: `/sdcard/Android/data/com.tawhid.hotdrewrithe/files`):
   `hotd.zip` (the arcade ROM zip) right there, your 1998 PC game's folder (with THOTD.EXE) into `pc`. Within 3 seconds
   the menu comes. (No storage permission is asked: that folder is the app's own.)
5. **If it fails, send back:** the text on the big screen (or "black" / "it quits"), `settings\hotd.log` from that
   folder, and the live log from the PC with the cable in, started before the game: `adb logcat -s hotd SDL SDL/APP
   OpenXR` (Ctrl+C after the problem; the `openxr:` and `quest:` lines matter most).

The long version, for a first-timer: "First time with a Quest" below.

What it is: the same game and VR mode as `hotd --vr openxr` on a PC (docs/vr.md), built for Android:

- **Graphics:** OpenGL ES 3 (the Quest's Adreno, ES 3.2) through the renderer's existing ES layer (the one ANGLE uses:
  the shaders rewritten as `#version 300 es`, texture buffers as 2D textures, RGB uploads / reads as RGBA, no GPU timer
  queries; render/gl_backend.cpp). On Android it always runs, on the device's own EGL driver.
- **VR:** the OpenXR backend (src/vr/vr_openxr.cpp) with the Khronos OpenXR loader for Android: `xrInitializeLoaderKHR`
  (XR_KHR_loader_init_android: the Java VM and the activity), XR_KHR_android_create_instance, XR_KHR_opengl_es_enable
  (`XrGraphicsBindingOpenGLESAndroidKHR`: the game's EGL display, config and context). The loader finds Meta's runtime
  through the OpenXR runtime broker (the manifest's queries).
- **The menus in VR:** every flat screen (the files message, the setup page, the menu, the end screen) is shown on a
  big screen in front of you; the right controller is the mouse (its ray, the trigger clicks). In play the game is the
  VR game of docs/vr.md (the gun hand, the crosshair, the HUD on the arcade screen's plane).
- **Sound:** SDL3's Android audio (AAudio). **Settings, scores, the log:** the app's files folder (below).
- **No game data in the APK.** The player copies their own files to the headset (below). The APK holds only the game's
  own `data\` folder (fonts, menu pictures, translations, vr.ini), as every package does.

## Building the APK

Needs (all in `<main checkout>\private\deps`, never committed): `android-sdk` (platform-tools, platforms;android-34,
build-tools;35.0.1, ndk;27.3.13750724, cmake;3.31.6), `openxr_android` (the Khronos loader
`openxr_loader_for_android-1.1.65.aar` from Maven Central, Apache 2.0; the script unpacks it), the usual `dr_libs`,
`musashi`, `mame_scsp`, `stb`; a JDK 17+ on PATH (21 used); Python 3; SDL 3.2.10's source (the main checkout's
`build\_deps\sdl3-src`); a Windows build's `build\m68kgen` (the arcade sound's generated 68000 core: a cross build
cannot run m68kmake; without it the APK has no arcade sound board, the PC sound still works).

```
powershell -ExecutionPolicy Bypass -File tools\make_quest_apk.ps1            # build (CMake + NDK) and package
powershell -ExecutionPolicy Bypass -File tools\make_quest_apk.ps1 -NoBuild   # package the last build again
```

Result: `build-quest\HOTD-ReWrithe-quest-<version>-debug.apk` (about 15 MB), signed with a debug key that the script
makes once as `private\deps\android-debug.keystore` (keep it: an update must be signed with the same key to install over
the last one).

How (no Gradle, nothing downloaded at build time): CMake with the NDK's toolchain builds the game as `libmain.so` (SDL's
Android entry: `SDL_main`) and SDL as `libSDL3.so` (arm64-v8a, minSdk 32, the static libc++); `javac` + `d8` compile SDL's
own Java activity (the SDL source's `android-project`, the same version as the library) and ours
(`android/quest/java/.../HotdActivity.java`: SDL's activity, loading `libopenxr_loader.so` first); `aapt2` links the
manifest (`android/quest/AndroidManifest.xml`) with the game's `data\` as assets; `aapt` adds `classes.dex` and the three
libraries; `zipalign`, `apksigner`. The CMake changes for Android are all under `if(ANDROID)` (the game a shared library,
the developer tools left out, `--driver-mode=g++` for an NDK in a folder with spaces, a pre-generated `m68kops.c`).

Checked on the APK (aapt2 dump badging / xmltree, aapt list, apksigner verify): package `com.tawhid.hotdrewrithe`, label
"HOTD ReWrithe", no icon (Android's default), minSdk 32 / target 34, `lib/arm64-v8a/` libmain.so + libSDL3.so +
libopenxr_loader.so (native-code arm64-v8a only), the launcher intent with `com.oculus.intent.category.VR` and
`org.khronos.openxr.intent.category.IMMERSIVE_HMD`, `android.hardware.vr.headtracking` required, OpenGL ES 3.2, the
OpenXR permissions and broker queries, landscape, `assets/data_index.txt` + 751 data files.

## First time with a Quest: step by step (sideloading)

"Sideloading" = installing an app that is not from Meta's Store, from your PC over a USB cable. It is free and allowed;
the headset only needs **Developer Mode** once. **Do steps 1 to 3 yourself with your own Meta account** (nobody else
should sign in for you). Time: about 20 minutes the first time.

What you need: the Quest 3 (charged, set up, the **Meta Horizon** app on your phone paired with it, as when you set it
up), a **USB-C cable that carries data** (the headset's own charging cable does; some cheap cables only charge), the
PC, and the APK file `HOTD-ReWrithe-quest-<version>-debug.apk`.

### 1. A free Meta developer account (once)

1. On the PC, open **developers.meta.com** (Meta's developer site; it was developer.oculus.com) and log in with **the
   same Meta account your headset uses**.
2. It asks you to create an **organization** (any name, e.g. your own name: it is only a label) and to accept the
   developer agreement. Meta may ask you to **verify the account** (a phone number by SMS, or a payment card that is
   not charged). That is all: no fee.

### 2. Developer Mode on the headset (once)

1. On the phone, open the **Meta Horizon** app > **Devices** (or Menu > Devices) > your **Quest 3** > **Headset
   Settings** > **Developer Mode** > switch it **on**. (No such switch: step 1 is not finished, or the app needs a
   minute; close and open it again.)
2. Restart the headset (hold its power button > Restart).

### 3. Install the game (pick one of three ways)

Connect the headset to the PC with the USB-C cable and put it on. A box asks **"Allow USB debugging?"**: tick
**Always allow from this computer**, then **Allow**. (A second box may ask **"Allow access to data?"**: Allow; that one
is for copying files in step 4.) If neither box comes: unplug and plug the cable in again with the headset on.

- **SideQuest** (the easiest; free, from **sidequestvr.com**: the "Advanced Installer" for Windows; it brings its own
  adb). Start it: the dot at the top left turns **green** with the headset's name (red / orange: answer the box in the
  headset). Then drag the APK file onto SideQuest's window, or click the **"Install APK file from folder"** icon (a box
  with an arrow, top right) and choose the APK. It says when the install is done.
- **Meta Quest Developer Hub** (Meta's own PC tool, free, from developers.meta.com: "Meta Quest Developer Hub"): sign
  in, the headset shows under Devices; drag the APK onto the **Apps** box (or "Add Build").
- **adb** (a command line; "Android SDK Platform-Tools" from developer.android.com, or the one SideQuest / the
  Developer Hub brought). In a terminal in its folder:

```
adb devices                                  # your headset's serial, then "device" ("unauthorized": answer the box)
adb install -r "C:\path\to\HOTD-ReWrithe-quest-1.3.1-debug.apk"
```

(`-r` installs over an older copy and keeps its files. To remove the game: `adb uninstall com.tawhid.hotdrewrithe`,
SideQuest's "Currently installed apps", or in the headset: the App Library > the app's ... menu > Uninstall. **Removing
it also deletes the game files you copied to it.** An update must be the same "debug" signature: an APK built on
another PC does not install over this one; uninstall first.)

### 4. Start it once, then copy your own game files

1. In the headset: open the **App Library** (the grid icon in the bar) > the filter at the top right (**All**) >
   **Unknown Sources** > **HOTD ReWrithe**. It starts in VR; a big screen in front of you says **"needs your own game
   files"** with what is missing. This first start makes the game's folder on the headset.
2. Copy your files (the same ones the PC version uses: the arcade ROM zip and the 1998 PC game) into the game's folder
   on the headset. That folder is:

```
Quest 3 > Internal shared storage > Android > data > com.tawhid.hotdrewrithe > files      (in Windows Explorer)
/sdcard/Android/data/com.tawhid.hotdrewrithe/files                                           (adb, SideQuest)

    hotd.zip                        the arcade ROM zip (MAME's "hotd" set); another name works when it is the only zip here
    pc\                             the 1998 PC game: everything in your installed PC game's folder (THOTD.EXE in pc\
                                    or one folder down, so copying the whole "HOTD" folder into pc\ works), or a zip of it
    PUT YOUR GAME FILES HERE.txt    made by the game: the same as these lines
    settings\                       made by the game: launcher.ini, scores, hotd.log (the log)
    vr\vr.ini                       made by the game: the VR settings
```

   - **Windows Explorer** (no extra program): with the cable in and "Allow access to data" answered, the headset is
     under **This PC** as **Quest 3**. Open the path above, drag `hotd.zip` into `files`, and your PC game's folder
     (e.g. `C:\Games\HOTD`, or its contents) into `pc`. If `com.tawhid.hotdrewrithe` is not listed: start the game once
     (step 1), then unplug and plug the cable in again (Windows only sees new folders after that); if `Android\data`
     stays empty, use SideQuest instead.
   - **SideQuest**: the folder icon at the top ("Manage files on the headset") > Android > data >
     com.tawhid.hotdrewrithe > files; drag the zip and the folder onto that window.
   - **adb**:

```
adb push "C:\path\to\hotd.zip" /sdcard/Android/data/com.tawhid.hotdrewrithe/files/hotd.zip
adb push "C:\Games\HOTD" /sdcard/Android/data/com.tawhid.hotdrewrithe/files/pc
```

3. Back in the headset (the game still open): it looks again every 3 seconds. Once both show **found**, it checks
   them (the first check reads the ROM zip once: a short wait) and the main menu comes on the big screen. If you closed
   the game meanwhile: start it again from Unknown Sources.

The copy takes a minute or two (the PC game with its music is a few hundred MB). The game files never leave the
headset; nothing is uploaded anywhere.

### 5. The log (when something goes wrong)

- **The game's log**: `settings\hotd.log` in the game's folder (above): copy it to the PC with Explorer or SideQuest,
  or `adb pull /sdcard/Android/data/com.tawhid.hotdrewrithe/files/settings/hotd.log`. `hotd.prev.log` is the run
  before. Send it with a bug report (it has no game data in it).
- **The live log** (also what happens before the game's own log starts, e.g. a crash at the start), with the cable in:
  `adb logcat -s SDL SDL/APP` (Ctrl+C ends; `adb logcat -d > quest_log.txt` saves everything so far to a file).
  The lines to look at start with `openxr:` (the runtime, the headset, the session's states 1 2 3 4 5) and `quest:`.
- **VR settings**: `vr\vr.ini` in the game's folder: copy it to the PC, edit it in Notepad, copy it back (the game
  reads it at each start).

## Starting it

In the headset: the App Library > the filter at the top right (All) > **Unknown Sources** > **HOTD ReWrithe**. It
starts in VR (no 2D window). Quit: the Meta button (right controller) > Quit, or the game's own Quit. Play standing or
seated, with the Guardian (the play area) set up as for any game; hold the Meta button to recenter the view.

## Controls (Touch controllers)

| where | right controller | left controller |
|---|---|---|
| menus, setup, end screen (the big screen) | point = the mouse, trigger = click, A = Enter, B = back (Esc) | X = Enter, Y = back, menu = back |
| in play | the gun (the game's own gun hand): trigger shoots, grip / A / B reload, point steeply down reloads | X = Start (join / continue), Y / grip reload, menu = pause |
| the pause | point at Continue / Exit (on the HUD's screen), trigger chooses | menu = pause / back to the game |

No keyboard or mouse is needed.

## vr.ini on the Quest (performance and comfort)

All of docs/vr.md's keys, and (data/vr/vr.ini lists them):

| key | default | what |
|---|---|---|
| `menu_panel` | auto (on on the Quest) | the flat screens on the big screen, the controller as the mouse |
| `menu_distance`, `menu_width` | 2.5, 3.2 | the big screen: metres ahead, metres wide |
| `resolution_scale` | 1.0 | the eyes' resolution x this (0.5 .. 1.5 of the headset's recommended size): **0.8 if it stutters** |
| `hud_lines` | 960 | the HUD panel's height in pixels (mipmapped every frame): 720 / 540 are cheaper |
| `arcade_sort` | auto (off on the Quest) | the flat screens' 3D (the menu's live background) with the arcade polygon sort; off: the depth buffer. On OpenGL ES the sort's keys are a float texture rewritten every frame (heavy); VR's eyes never sort |
| `srgb_swapchain` | on | off: an RGBA8 headset picture (if the colours look washed out or too dark) |
| `refresh_rate` | auto | vr4: the display's rate asked of the runtime: auto = 90 Hz on the Quest (or the highest offered under it), or 72 / 90 / 120 (the nearest offered) |
| `sustained_performance` | on | vr4: the CPU and GPU levels at sustained high while VR runs (XR_EXT_performance_settings) |
| `auto_recentre` | 3 | vr4: the headset back after more than this many seconds off: recentred by itself (`off`: never) |

### Refresh rate and performance (vr4)

The Quest starts an app at 72 Hz. With **XR_FB_display_refresh_rate** (Meta's runtime has it; it is enabled only when
the runtime lists it) the game asks for `refresh_rate` right after the session begins: auto = 90 Hz on a standalone
headset (the Quest 2 / 3 offer 72 / 80 / 90 / 120), on a PC nothing (Quest Link / SteamVR choose it in their own
settings). The log says `openxr: display N Hz (offered 72 / 80 / 90 / 120; refresh_rate auto) -> asked 90 Hz` and
`openxr: display refresh rate 72 -> 90 Hz` when it changed. **XR_EXT_performance_settings** (when there): the CPU and
GPU at **sustained high** (`openxr: performance level sustained high: CPU set, GPU set`; a warning from the headset
when it gets warm is logged as `openxr: performance notification ...`). If 90 Hz stutters: `refresh_rate = 72`, or
`resolution_scale = 0.8`. The headset paces the game at whatever rate it runs (Smooth Motion blends the camera).

**Performance (72 / 90 Hz on the Quest 3), what looks heavy:** the world is drawn twice (once per eye) at the headset's
recommended size (about 1680 x 1760 per eye): `resolution_scale` is the main switch. The arcade sort (above) is off by
default. The HUD panel is redrawn and mipmapped every frame (`hud_lines`). Texture packs (data mods) make 2048 x 2048
atlases (up to 31 layers, RGBA): keep big packs off on the headset (Options > Mods). The headset's `xrWaitFrame` paces
the game (the frame limiter and the window's swap are off on Android while VR runs). The game logic is the arcade's 57.5 /
60 Hz either way; Smooth Motion blends the camera to the display's rate. The eye's depth cannot be read back on OpenGL
ES: vr4 measures how far the gun's ray goes with a CPU ray test against the stage's triangles and the creatures' parts
(docs/vr.md "Aim depth"; ~3 microseconds a ray). Before, a fixed `aim_distance` (8 m) put the crosshair at the wrong
depth and the aim off by the hand's parallax (mean 66 arcade units in the preview test; now 1.4).

Recentre (vr4): holding a thumbstick works without a valid head position too (no boundary: the heading only); taking
the headset off for more than 3 s and putting it back recentres by itself (`auto_recentre`).

## First test on the headset (checklist)

1. The install succeeds (SideQuest / the Developer Hub / `adb install`); the app is under Unknown Sources.
2. Start it: a VR view (not a flat window) with a dark room and the big screen: the message "needs your own game files"
   with the folder as the PC shows it and the adb lines. If nothing shows or it quits: `adb logcat -s SDL SDL/APP` and
   look for `openxr:` lines (the runtime, its extensions, the swapchain formats, the session's states 1 2 3 4 5) and
   `quest:` lines.
3. Copy the files (step 4 above: Explorer, SideQuest or adb); within a few seconds the check runs and the menu shows on
   the big screen. (Explorer: is `Android\data\com.tawhid.hotdrewrithe\files` with `PUT YOUR GAME FILES HERE.txt` and
   `pc` there? Say if not.)
4. The right controller's red ray and dot follow your hand on the screen; the trigger clicks menu items; B goes back.
5. Start Arcade: the world around you, the gun hand on the right controller, the crosshair where it points, the HUD
   ahead; the trigger shoots (a short rumble), grip / A / B reload, X = Start, the left menu button pauses (point at
   Continue / Exit, trigger).
6. Colours as on the PC? Washed out: `srgb_swapchain = off` in vr.ini (and say which looked right). The log says
   `openxr: OpenGL ES sRGB write control there / missing` and which swapchain format it took.
7. Smooth at 90 Hz (the log: `openxr: display ... -> asked 90 Hz`, then `display refresh rate 72 -> 90 Hz`)? If
   not: `refresh_rate = 72`, `resolution_scale = 0.8`, then `hud_lines = 720`; send hotd.log.
7b. (vr4) The crosshair sits on what it points at (a zombie near, a wall far), not floating at one depth; shots land
   under it at close range. Take the headset off for 5 s and put it back: the view faces ahead again.
8. Sound plays (menu music, shots).
9. Take the headset off and on, open the Meta menu during play and close it: the game comes back; Quit from the Meta
   menu ends it.
10. If the gun sits wrong in the hand: `gun_offset` / `gun_rotate` (docs/vr.md).

## Untested / open (needs the headset)

Everything on the device: the app starting at all (SDL's activity in an immersive VR app; the window surface SDL makes
is never shown), the loader finding Meta's runtime, the session and its states, the ES binding (the EGL config looked up
from the context's EGL_CONFIG_ID), the swapchain format and sRGB, the frame rate, AAudio, the files check on the
headset's storage (files pushed with adb into the app's folder: normally readable by the app; if not, start the app
once before pushing), suspend / resume (SDL blocks its loop while the activity is paused), the controls' bindings
(the Touch profile of docs/vr.md), the haptics. The first start copies the game's data folder (751 files, 13 MB) from
the APK into the app's internal storage (a few seconds once per build).
