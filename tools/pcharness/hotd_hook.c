/*
 * hotd_hook.c - PC harness for the 1998 PC port of The House of the Dead (THOTD.EXE), built as a proxy dinput.dll.
 *
 * Put next to a COPY of THOTD.EXE (never the original install). Windows loads it as the game's dinput.dll before
 * the game's code runs; it forwards DirectInputCreateA to the system's dinput.dll and installs:
 *   - a deterministic clock: timeGetTime / GetTickCount return base + ticks * 1000 / 60 (+1 per call since the last
 *     tick, so a loop that waits for time to pass still ends); the main loop is switched to its own
 *     "one tick per drawn frame" mode (0x5CE688 = 0), which never reads the clock;
 *   - no focus or mouse grabbing: ShowWindow / SetForegroundWindow / SetActiveWindow / BringWindowToTop /
 *     SetFocus never activate, SetCursorPos and ClipCursor do nothing, the window is created off screen, the game's
 *     window procedure always sees itself active;
 *   - a wrapper around frame_tick (0x43B6E0, the 60 Hz game tick) that logs each tick and can stop the game after
 *     a number of ticks.
 * Settings (environment): HOTD_PC_LOG (log file), HOTD_PC_TICKS (stop after this many ticks, 0 = never),
 * HOTD_PC_MUTE (1: every DirectSound buffer is held at minimum volume).
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define VA(x) ((void *)(x))

/* ---- game addresses (THOTD.EXE, PC port; see docs/decompilation.md) ---- */
#define PC_FRAME_TICK       0x43B6E0u   /* the 60 Hz game tick */
#define PC_TICK_MODE        0x5CE688u   /* 1: ticks from timeGetTime (default), 0: one tick per drawn frame */
#define PC_FRAME_COUNTER    0xDA0B08u   /* frame counter (arcade 0x51F428) */
#define PC_GAME_MODE        0xDA0CDCu   /* game mode (u16; frame_tick reads the dword at 0xDA0CDA, sar 16) */
#define PC_RAND_STATE       0xDA10F0u   /* rand LCG state (arcade 0x53E380) */
#define PC_FRONTEND_STATE   0x5CED98u   /* PC front end state (mode switch) */
#define PC_SCREEN_ID        0x5CE8A8u   /* front-end screen id (8 high scores, 9 PC Mode high scores, docs/decompilation.md) */
#define PC_MENU_SCREEN      0x5CE8A8u   /* menu screen number (set by each screen builder) */

static FILE *g_log;
static void cap_block(DWORD at, DWORD size);
static unsigned g_ticks, g_calls_since_tick, g_stop_after;
static FILE *g_cap;
static void cap_unwound(DWORD esp);
static void cap_close(void);
static DWORD g_base_ms = 100000;
static HWND g_hwnd;
static WNDPROC g_game_wndproc;

/* ---- small helpers ---- */
static void logf(const char *fmt, ...)
{
    va_list ap;
    if (!g_log) return;
    va_start(ap, fmt);
    vfprintf(g_log, fmt, ap);
    va_end(ap);
    fflush(g_log);
}

static void write_code(void *dst, const void *src, size_t n)
{
    DWORD old;
    VirtualProtect(dst, n, PAGE_EXECUTE_READWRITE, &old);
    memcpy(dst, src, n);
    VirtualProtect(dst, n, old, &old);
    FlushInstructionCache(GetCurrentProcess(), dst, n);
}

/* replace every import slot of module m that points at `real` */
static int patch_iat(HMODULE m, void *real, void *hook)
{
    BYTE *base = (BYTE *)m;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    IMAGE_IMPORT_DESCRIPTOR *d;
    int n = 0;
    if (!dir.VirtualAddress || !real) return 0;
    for (d = (IMAGE_IMPORT_DESCRIPTOR *)(base + dir.VirtualAddress); d->Name; d++) {
        void **slot = (void **)(base + d->FirstThunk);
        for (; *slot; slot++) {
            if (*slot == real) {
                DWORD old;
                VirtualProtect(slot, sizeof(void *), PAGE_READWRITE, &old);
                *slot = hook;
                VirtualProtect(slot, sizeof(void *), old, &old);
                n++;
            }
        }
    }
    return n;
}

static void *proc(const char *dll, const char *name)
{
    HMODULE m = GetModuleHandleA(dll);
    return m ? (void *)GetProcAddress(m, name) : NULL;
}

/* 5-byte jump at a hot-patchable Win32 export (8B FF 55 8B EC); returns a trampoline to the original, or NULL */
static void *hook_export(const char *dll, const char *name, void *hook)
{
    BYTE *f = (BYTE *)proc(dll, name), *tr, jmp[5];
    static const BYTE prologue[5] = {0x8B, 0xFF, 0x55, 0x8B, 0xEC};
    if (!f || memcmp(f, prologue, 5) != 0) {
        logf("hook %s!%s: not hot-patchable, skipped\n", dll, name);
        return NULL;
    }
    tr = (BYTE *)VirtualAlloc(NULL, 16, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    memcpy(tr, prologue, 5);
    tr[5] = 0xE9;
    *(DWORD *)(tr + 6) = (DWORD)(f + 5) - (DWORD)(tr + 10);
    jmp[0] = 0xE9;
    *(DWORD *)(jmp + 1) = (DWORD)hook - (DWORD)(f + 5);
    write_code(f, jmp, 5);
    return tr;
}

/* ---- the deterministic clock ---- */
static DWORD vclock(void)
{
    return g_base_ms + (DWORD)(((unsigned long long)g_ticks * 1000) / 60) + g_calls_since_tick++;
}
static void cap_clock(DWORD routine, DWORD v);
static DWORD WINAPI h_timeGetTime(void) { DWORD v = vclock(); cap_clock(0xFFFFFF01u, v); return v; }
static DWORD WINAPI h_GetTickCount(void) { DWORD v = vclock(); cap_clock(0xFFFFFF02u, v); return v; }

/* ---- the same heap addresses every run: the game's VirtualAlloc calls without an address (the C runtime's heap)
   get consecutive 64 KB-aligned addresses from a fixed base, so pointers into the heap repeat between runs ---- */
static DWORD g_next_va = 0x20000000;
static LPVOID WINAPI h_VirtualAlloc(LPVOID addr, SIZE_T size, DWORD type, DWORD prot)
{
    if (!addr && size) {
        DWORD step = (DWORD)((size + 0xFFFF) & ~0xFFFFu);
        int tries;
        for (tries = 0; tries < 64; tries++) {
            LPVOID r = VirtualAlloc((LPVOID)g_next_va, size, type | MEM_RESERVE, prot);
            DWORD at = g_next_va;
            g_next_va += step;
            if (r) {
                cap_block(at, step);
                logf("VirtualAlloc %lu bytes at %08lx\n", (unsigned long)size, at);
                return r;
            }
        }
        logf("VirtualAlloc %lu bytes: no fixed address free\n", (unsigned long)size);
    }
    return VirtualAlloc(addr, size, type, prot);
}

/* GlobalAlloc without GMEM_MOVEABLE (the game's master memory pool, 0x563050, and others): served the same way;
   the handle is the pointer, as for fixed global memory */
#define MAX_FIXED 256
static DWORD g_fixed_ptr[MAX_FIXED], g_fixed_size[MAX_FIXED];
static int find_fixed(const void *p)
{
    int i;
    for (i = 0; i < MAX_FIXED; i++)
        if (g_fixed_ptr[i] && g_fixed_ptr[i] == (DWORD)p) return i;
    return -1;
}
static HGLOBAL WINAPI h_GlobalAlloc(UINT flags, SIZE_T size)
{
    if (!(flags & GMEM_MOVEABLE) && size) {
        int i = find_fixed(NULL);
        for (i = 0; i < MAX_FIXED && g_fixed_ptr[i]; i++) {}
        if (i < MAX_FIXED) {
            LPVOID p = h_VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);  /* zero filled */
            if (p) {
                g_fixed_ptr[i] = (DWORD)p;
                g_fixed_size[i] = (DWORD)size;
                return (HGLOBAL)p;
            }
        }
    }
    return GlobalAlloc(flags, size);
}
static LPVOID WINAPI h_GlobalLock(HGLOBAL h) { return find_fixed(h) >= 0 ? (LPVOID)h : GlobalLock(h); }
static BOOL WINAPI h_GlobalUnlock(HGLOBAL h) { return find_fixed(h) >= 0 ? FALSE : GlobalUnlock(h); }
static SIZE_T WINAPI h_GlobalSize(HGLOBAL h) { int i = find_fixed(h); return i >= 0 ? g_fixed_size[i] : GlobalSize(h); }
static HGLOBAL WINAPI h_GlobalFree(HGLOBAL h)
{
    int i = find_fixed(h);
    if (i < 0) return GlobalFree(h);
    VirtualFree(h, 0, MEM_RELEASE);
    g_fixed_ptr[i] = 0;
    return NULL;
}

/* ---- no focus, no mouse grabbing ---- */
typedef BOOL (WINAPI *ShowWindow_t)(HWND, int);
static ShowWindow_t o_ShowWindow;
static BOOL WINAPI h_ShowWindow(HWND h, int cmd)
{
    if (cmd == SW_SHOW || cmd == SW_SHOWNORMAL || cmd == SW_SHOWDEFAULT || cmd == SW_RESTORE ||
        cmd == SW_SHOWMAXIMIZED)
        cmd = SW_SHOWNOACTIVATE;
    else if (cmd == SW_SHOWMINIMIZED)
        cmd = SW_SHOWMINNOACTIVE;
    return o_ShowWindow ? o_ShowWindow(h, cmd) : TRUE;
}
static BOOL WINAPI h_SetForegroundWindow(HWND h) { (void)h; return TRUE; }
static HWND WINAPI h_SetActiveWindow(HWND h) { return h; }
static BOOL WINAPI h_BringWindowToTop(HWND h) { (void)h; return TRUE; }
static HWND WINAPI h_SetFocus(HWND h) { return h; }
static HWND WINAPI h_GetActiveWindow(void) { return g_hwnd; }
static HWND WINAPI h_GetForegroundWindow(void) { return g_hwnd; }
static BOOL WINAPI h_ClipCursor(const RECT *r) { (void)r; return TRUE; }

/* ---- injected input: a virtual cursor, virtual key states for GetAsyncKeyState and for the DirectInput
   keyboard; changed only by the input script (HOTD_PC_INPUT), between ticks ---- */
static LONG g_cursor_x = 320, g_cursor_y = 240;
static BYTE g_vk[256];      /* GetAsyncKeyState: 1 = down (VK_LBUTTON = trigger, VK_RBUTTON = reload, ...) */
static BYTE g_dik[256];     /* DirectInput keyboard: 0x80 = down (DIK_ codes) */
static LONG g_mouse_dx, g_mouse_dy;   /* DirectInput mouse: motion still to report */
static BYTE g_mbtn[8];      /* DirectInput mouse buttons: 0x80 = down (0 = left: trigger, 1 = right: reload) */
static MMRESULT WINAPI h_joyGetPosEx(UINT id, void *ji) { (void)id; (void)ji; return 167; /* JOYERR_UNPLUGGED */ }
static MMRESULT WINAPI h_joyGetPos(UINT id, void *ji) { (void)id; (void)ji; return 167; }
static BOOL WINAPI h_SetCursorPos(int x, int y) { g_cursor_x = x; g_cursor_y = y; return TRUE; }
static BOOL WINAPI h_GetCursorPos(POINT *p)
{
    if (p) { p->x = g_cursor_x; p->y = g_cursor_y; }
    return TRUE;
}
static SHORT WINAPI h_GetAsyncKeyState(int vk) { return (vk >= 0 && vk < 256 && g_vk[vk]) ? (SHORT)0x8000 : 0; }

/* the script: one command per line, "<poll> <command> <arguments>", applied when the game reads the DirectInput
   keyboard for the <poll>-th time (once per drawn frame, in the menus as in play; with the harness's one tick per
   frame, poll ~ tick in play; pc.log lists both):
     <tick> cursor <x> <y>          virtual cursor position (screen coordinates; the game recentres at 320, 240)
     <tick> vk <code> <0|1>         GetAsyncKeyState key up / down (1 = left button, 2 = right button)
     <tick> dik <code> <0|1>        DirectInput keyboard key up / down (0x1C Enter, 0x01 Esc, 0xC8 up, 0xD0 down)
   lines starting with # are comments */
typedef struct { unsigned tick; int cmd, a, b; } InputEvent;
/* auto-aim ("<poll> autoaim 1" / "0"): player 1's mouse steered at the nearest shootable object drawn in the last tick
   (camera-space view_pos +0x40 projected with focal 496, as shot_ray_setup takes the aim), a click when on target,
   a right click (reload) when the rounds are out. Only game state decides the input, so runs still repeat. */
static int g_autoaim, g_aim_click, g_aim_reload, g_inflives, g_inflives2, g_seen_table;   /* g_seen_table: the high-score table was shown */
static int g_nroute, g_route_ch[32], g_route_ev[32], g_route_val[32];   /* HOTD_PC_ROUTE */
static int g_naim_skip; static DWORD g_aim_skip[16];   /* HOTD_PC_AIM_SKIP: routines never aimed at (hex list) */
#define PC_ROUTE_CHOICE 0xDA0CF2u   /* route_choice (byte; arcade 0x51F270) */
#define PC_EVENT_INDEX  0xDA0CF3u   /* event_index: current event of the chapter (byte) */
#define PC_CHAPTER      0xDA0CF4u   /* chapter, 0-based (byte) */
static unsigned g_polls;
static void harness_stop(void);
#define MAX_TASKS 4096
static DWORD g_last_tasks[MAX_TASKS];
static unsigned g_nlast_tasks;
#define PC_GUN0_AIM     0xD9F5E4u   /* gun record 0 +4: aim x, y (short) */
#define PC_PLAYER0      0xD9F05Cu   /* players[0]: rounds +0x3D */
static void autoaim_step(LONG *dx, LONG *dy)
{
    unsigned i;
    float best = 1e30f, tx = 0, ty = 0;
    int found = 0;
    short ax = *(short *)VA(PC_GUN0_AIM), ay = *(short *)VA(PC_GUN0_AIM + 2);
    for (i = 0; i < g_nlast_tasks; i++) {
        DWORD t = g_last_tasks[i];
        float vx, vy, vz;
        if (IsBadReadPtr(VA(t), 0x70)) continue;
        if (!(*(BYTE *)VA(t + 0x60) & 1)) continue;          /* shootable */
        {
            int k, skip = 0;
            for (k = 0; k < g_naim_skip; k++) if (*(DWORD *)VA(t) == g_aim_skip[k]) skip = 1;
            if (skip) continue;
        }
        vx = *(float *)VA(t + 0x40); vy = *(float *)VA(t + 0x44); vz = *(float *)VA(t + 0x48);
        if (!(vz > 5.0f) || vz > 2000.0f) continue;
        vx = vx * 496.0f / vz; vy = vy * 496.0f / vz;
        if (vx < -300 || vx > 300 || vy < -220 || vy > 220) continue;
        if (vz < best) { best = vz; tx = vx; ty = vy; found = 1; }
    }
    g_mbtn[0] = 0;
    g_mbtn[1] = 0;
    if (*(WORD *)VA(PC_GAME_MODE) == 7 && (*(LONG *)VA(PC_SCREEN_ID) == 8 || *(LONG *)VA(PC_SCREEN_ID) == 9)) {
        /* the high-score table after the name entry. The front end reads only the DirectInput keyboard here, on press
           edges (current 0x12FDBE0 against previous 0x12FDBE4): frontend_input 0x41DA18 sends Escape to widget 1 (else
           3); 0x420A60 moves the pointer (0xD5A97C / 0xD5A980) with the arrow keys and activates the widget under it on
           Enter / Space / a mouse-button edge. (0x42A26C / 0x42AD5C, GetCursorPos, have no callers.) So: Escape edges,
           and between them the arrows steer the pointer to Exit (320, 448) and Enter is pressed there. */
        unsigned ph = g_polls % 32;
        LONG px = *(LONG *)VA(0xD5A97C), py = *(LONG *)VA(0xD5A980);
        int at_exit = px > 300 && px < 340 && py > 436 && py < 460;
        g_seen_table = 1;
        g_vk[VK_LBUTTON] = 0;
        g_dik[0x01] = ph < 4 ? 0x80 : 0;                                /* DIK_ESCAPE */
        g_dik[0xCB] = !at_exit && ph >= 8 && px > 330 ? 0x80 : 0;          /* left */
        g_dik[0xCD] = !at_exit && ph >= 8 && px < 310 ? 0x80 : 0;          /* right */
        g_dik[0xC8] = !at_exit && ph >= 8 && py > 456 ? 0x80 : 0;          /* up */
        g_dik[0xD0] = !at_exit && ph >= 8 && py < 440 ? 0x80 : 0;          /* down */
        g_dik[0x1C] = at_exit && ph >= 16 && ph < 20 ? 0x80 : 0;           /* DIK_RETURN */
        if ((g_polls % 120) == 0)
            logf("autoaim poll %u: high-score table, Escape / pointer %ld %ld -> Exit (front %ld)\n", g_polls, px, py,
                 *(LONG *)VA(PC_FRONTEND_STATE));
        return;
    }
    g_dik[0x01] = g_dik[0xCB] = g_dik[0xCD] = g_dik[0xC8] = g_dik[0xD0] = 0;
    if (*(WORD *)VA(PC_GAME_MODE) == 7 && g_seen_table && *(LONG *)VA(PC_SCREEN_ID) == 1 &&
        *(LONG *)VA(PC_FRONTEND_STATE) == 0) {
        /* back on the main menu (screen 1) after the table: the game is over, so the run ends here (the frame
           ticks stop in the menus, a run left there would only reach its timeout) */
        logf("autoaim poll %u (tick %u): main menu after the game, run ended\n", g_polls, g_ticks);
        harness_stop();
        return;
    }
    if (*(WORD *)VA(PC_GAME_MODE) == 7) {                   /* name entry (screen 15): Enter, pressed every 8 polls */
        g_vk[VK_LBUTTON] = 0;
        g_dik[0x1C] = (g_polls % 16) < 8 ? 0x80 : 0;         /* DIK_RETURN */
        if ((g_polls % 120) == 0)
            logf("autoaim poll %u: name entry, pressing Enter (front %ld, screen %ld)\n", g_polls,
                 *(LONG *)VA(PC_FRONTEND_STATE), *(LONG *)VA(PC_SCREEN_ID));
        return;
    }
    g_dik[0x1C] = 0;
    g_vk[VK_LBUTTON] = 0;
    if (*(BYTE *)VA(PC_PLAYER0 + 0x3D) == 0) {             /* out of rounds: reload (one right click) */
        g_aim_reload ^= 1;
        g_mbtn[1] = g_aim_reload ? 0x80 : 0;
        return;
    }
    if ((g_polls % 30) == 0)
        logf("autoaim poll %u: aim %d %d target %s %.1f %.1f rounds %u score %ld\n", g_polls, ax, ay, found ? "yes" : "no",
             tx, ty, *(BYTE *)VA(PC_PLAYER0 + 0x3D), *(LONG *)VA(PC_PLAYER0 + 8));
    if (!found) return;
    *dx = (LONG)((tx - ax) / 2);
    *dy = (LONG)((ay - ty) / 2);   /* the mouse moves the aim y the other way */
    if (tx - ax < 6 && ax - tx < 6 && ty - ay < 6 && ay - ty < 6) {
        g_aim_click ^= 1;                                  /* press on one poll, release on the next */
        g_mbtn[0] = g_aim_click ? 0x80 : 0;
    }
}
static InputEvent *g_ev;
static unsigned g_nev, g_next_ev;

static void input_load(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[256], cmd[32];
    unsigned cap = 0;
    if (!f) { logf("input script %s not found\n", path); return; }
    while (fgets(line, sizeof line, f)) {
        InputEvent e;
        if (line[0] == '#' || sscanf(line, "%u %31s %i %i", &e.tick, cmd, &e.a, &e.b) < 3) continue;
        e.cmd = !strcmp(cmd, "cursor") ? 1 : !strcmp(cmd, "vk") ? 2 : !strcmp(cmd, "dik") ? 3 :
                !strcmp(cmd, "mouse") ? 4 : !strcmp(cmd, "mbtn") ? 5 : !strcmp(cmd, "autoaim") ? 6 : 0;
        if (!e.cmd) continue;
        if (g_nev == cap) { cap = cap ? cap * 2 : 64; g_ev = (InputEvent *)realloc(g_ev, cap * sizeof *g_ev); }
        g_ev[g_nev++] = e;
    }
    fclose(f);
    logf("input script %s: %u events\n", path, g_nev);
}

static void input_apply(unsigned tick)
{
    while (g_next_ev < g_nev && g_ev[g_next_ev].tick <= tick) {
        InputEvent *e = &g_ev[g_next_ev++];
        if (e->cmd == 1) { g_cursor_x = e->a; g_cursor_y = e->b; }
        else if (e->cmd == 2 && e->a >= 0 && e->a < 256) g_vk[e->a] = (BYTE)(e->b != 0);
        else if (e->cmd == 3 && e->a >= 0 && e->a < 256) g_dik[e->a] = e->b ? 0x80 : 0;
        else if (e->cmd == 4) { g_mouse_dx += e->a; g_mouse_dy += e->b; }
        else if (e->cmd == 5 && e->a >= 0 && e->a < 8) g_mbtn[e->a] = e->b ? 0x80 : 0;
        else if (e->cmd == 6) g_autoaim = e->a;
        logf("input at poll %u (tick %u): %s %d %d\n", tick, g_ticks, e->cmd == 1 ? "cursor" : e->cmd == 2 ? "vk" : e->cmd == 3 ? "dik" : e->cmd == 4 ? "mouse" : e->cmd == 5 ? "mbtn" : "autoaim",
             e->a, e->b);
    }
}

/* DirectInput: IDirectInputA::CreateDevice (vtable 3) is wrapped; every device's Acquire (7) succeeds and its
   GetDeviceState (9) returns the virtual state (keyboard: 256 bytes; other sizes: zeros), GetDeviceData (10) no
   events. The real devices are never read, so the user's typing never reaches the game. */
typedef HRESULT (WINAPI *DICreateDevice_t)(void *, const GUID *, void **, void *);
static DICreateDevice_t o_DICreateDevice;
static void *o_DIAcquire, *o_DIGetState, *o_DIGetData;
static HRESULT WINAPI h_DIAcquire(void *d) { (void)d; return 0; }
static unsigned g_trace_menu;
static LONG g_last_front = -1, g_last_screen = -1;
static HRESULT WINAPI h_DIGetState(void *d, DWORD size, void *out)
{
    (void)d;
    if (size == 256) {
        /* polls are counted from the first game tick (the attract's start): the boot screens before it take a
           varying number of frames, the game state does not depend on them */
        LONG fs = *(LONG *)VA(PC_FRONTEND_STATE);
        if (g_ticks) {
            g_polls++;
            input_apply(g_polls);
        }
        if (fs != g_last_front || *(LONG *)VA(PC_MENU_SCREEN) != g_last_screen) {
            g_last_screen = *(LONG *)VA(PC_MENU_SCREEN);
            logf("poll %u (tick %u): front %ld screen %ld\n", g_polls, g_ticks, fs, g_last_screen);
            g_last_front = fs;
        }
        if (g_trace_menu && g_polls && fs != 6)
            logf("poll %u menu cursor %ld %ld\n", g_polls, *(LONG *)VA(0xD5A97C), *(LONG *)VA(0xD5A980));
    }
    if (!out) return 0;
    if (size == 256) memcpy(out, g_dik, 256);
    else if (size == 16 || size == 20) {
        /* DIMOUSESTATE(2): relative motion since the last read, then the buttons (0x80 = down); the game
           accumulates the motion into its own cursor / aim (0x577F.., 0x12FDB90) */
        LONG *m = (LONG *)out;
        memset(out, 0, size);
        if (g_autoaim) autoaim_step(&g_mouse_dx, &g_mouse_dy);
        m[0] = g_mouse_dx;
        m[1] = g_mouse_dy;
        g_mouse_dx = g_mouse_dy = 0;
        memcpy((BYTE *)out + 12, g_mbtn, size - 12);
    } else
        memset(out, 0, size);
    return 0;
}
static HRESULT WINAPI h_DIGetData(void *d, DWORD objsize, void *data, DWORD *n, DWORD flags)
{
    (void)d; (void)objsize; (void)data; (void)flags;
    if (n) *n = 0;
    return 0;
}

static void patch_vtbl_slot(void **vt, int i, void *hook, void **orig)
{
    DWORD old;
    if (vt[i] == hook) return;
    VirtualProtect(&vt[i], sizeof(void *), PAGE_READWRITE, &old);
    if (orig && !*orig) *orig = vt[i];
    vt[i] = hook;
    VirtualProtect(&vt[i], sizeof(void *), old, &old);
}

static HRESULT WINAPI h_DICreateDevice(void *di, const GUID *g, void **dev, void *outer)
{
    HRESULT r = o_DICreateDevice(di, g, dev, outer);
    if (r >= 0 && dev && *dev) {
        void **vt = *(void ***)*dev;
        patch_vtbl_slot(vt, 7, (void *)h_DIAcquire, &o_DIAcquire);
        patch_vtbl_slot(vt, 9, (void *)h_DIGetState, &o_DIGetState);
        patch_vtbl_slot(vt, 10, (void *)h_DIGetData, &o_DIGetData);
        logf("DirectInput device wrapped\n");
    }
    return r;
}

static LRESULT CALLBACK h_wndproc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_ACTIVATEAPP: wp = TRUE; break;
    case WM_ACTIVATE: wp = WA_ACTIVE; break;
    case WM_KILLFOCUS: return 0;
    case WM_NCACTIVATE: wp = TRUE; break;
    case WM_SYSCOMMAND:   /* never minimized: a minimized game stops for good (its display surface is lost) */
        if ((wp & 0xFFF0) == SC_MINIMIZE) return 0;
        break;
    }
    return CallWindowProcA(g_game_wndproc, h, msg, wp, lp);
}

typedef ATOM (WINAPI *RegisterClassA_t)(const WNDCLASSA *);
static ATOM WINAPI h_RegisterClassA(const WNDCLASSA *wc)
{
    WNDCLASSA c = *wc;
    g_game_wndproc = c.lpfnWndProc;
    c.lpfnWndProc = h_wndproc;
    logf("RegisterClassA %s\n", c.lpszClassName ? c.lpszClassName : "?");
    return RegisterClassA(&c);
}

static HWND WINAPI h_CreateWindowExA(DWORD ex, LPCSTR cls, LPCSTR name, DWORD style, int x, int y, int w, int h,
                                     HWND parent, HMENU menu, HINSTANCE inst, LPVOID param)
{
    HWND r;
    /* no taskbar / Alt-Tab button (WS_EX_TOOLWINDOW), so the user can't bring it up or minimize it by accident */
    ex = (ex & ~(WS_EX_TOPMOST | WS_EX_APPWINDOW)) | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;
    r = CreateWindowExA(ex, cls, name, style, -20000, -20000, w, h, parent, menu, inst, param);
    if (!parent) g_hwnd = r;
    logf("CreateWindowExA %s -> %p (style %08lx ex %08lx, %dx%d)\n", name ? name : "?", (void *)r, style, ex, w, h);
    return r;
}

/* ---- more than one game at a time: the game exits at start (0x418655) when the global atom "TheHouseOfTheDead"
   exists (it adds it there and deletes it at exit), and WinMain (0x563475) gives up when FindWindowA finds a window
   of its class. Both checks are kept to this process: the atom is never looked up or added in the global table (a
   fake atom stands in for it), FindWindowA finds nothing. The settings files the game writes (thotd.cfg,
   scores.cfg, cstats.cfg: read at start, written at exit) are redirected into HOTD_PC_CFG_DIR (the run's folder,
   run_pc.ps1 copies them there first), so runs never share them. ---- */
#define FAKE_ATOM 0xBEEF
static int g_fake_atom_live;
static int is_instance_atom(LPCSTR s) { return s && HIWORD(s) && lstrcmpiA(s, "TheHouseOfTheDead") == 0; }
static ATOM WINAPI h_GlobalFindAtomA(LPCSTR s)
{
    if (is_instance_atom(s)) return g_fake_atom_live ? FAKE_ATOM : 0;
    return GlobalFindAtomA(s);
}
static ATOM WINAPI h_GlobalAddAtomA(LPCSTR s)
{
    if (is_instance_atom(s)) { g_fake_atom_live = 1; logf("instance atom: kept to this process\n"); return FAKE_ATOM; }
    return GlobalAddAtomA(s);
}
static ATOM WINAPI h_GlobalDeleteAtom(ATOM a)
{
    if (a == FAKE_ATOM && g_fake_atom_live) { g_fake_atom_live = 0; return 0; }
    return GlobalDeleteAtom(a);
}
static HWND WINAPI h_FindWindowA(LPCSTR cls, LPCSTR name)
{
    logf("FindWindowA %s: nothing (instance check kept to this process)\n", cls && HIWORD(cls) ? cls : "?");
    (void)name;
    return NULL;
}

static char g_cfg_dir[MAX_PATH];
/* the run's copy of a settings file the game writes, or `path` itself */
static LPCSTR cfg_path(LPCSTR path, char *buf)
{
    static const char *const names[] = {"thotd.cfg", "scores.cfg", "cstats.cfg"};
    const char *b, *p;
    int i;
    if (!g_cfg_dir[0] || !path) return path;
    for (b = p = path; *p; p++)
        if (*p == '\\' || *p == '/' || *p == ':') b = p + 1;
    for (i = 0; i < 3; i++)
        if (lstrcmpiA(b, names[i]) == 0) {
            _snprintf(buf, MAX_PATH, "%s\\%s", g_cfg_dir, names[i]);
            buf[MAX_PATH - 1] = 0;
            return buf;
        }
    return path;
}
static HANDLE WINAPI h_CreateFileA(LPCSTR path, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa, DWORD disp,
                                   DWORD flags, HANDLE tmpl)
{
    char buf[MAX_PATH];
    LPCSTR p = cfg_path(path, buf);
    if (p != path) logf("CreateFileA %s -> %s (%s)\n", path, p, (access & GENERIC_WRITE) ? "write" : "read");
    return CreateFileA(p, access, share, sa, disp, flags, tmpl);
}
static BOOL WINAPI h_DeleteFileA(LPCSTR path)
{
    char buf[MAX_PATH];
    return DeleteFileA(cfg_path(path, buf));
}
static BOOL WINAPI h_MoveFileA(LPCSTR from, LPCSTR to)
{
    char b1[MAX_PATH], b2[MAX_PATH];
    return MoveFileA(cfg_path(from, b1), cfg_path(to, b2));
}
static DWORD WINAPI h_GetFileAttributesA(LPCSTR path)
{
    char buf[MAX_PATH];
    return GetFileAttributesA(cfg_path(path, buf));
}

/* mute: the game keeps its real DirectSound device (it refuses to start without one, and sound status may matter to
   it), but every buffer is held at minimum volume */
typedef HRESULT (WINAPI *DirectSoundCreate_t)(void *, void **, void *);
typedef HRESULT (WINAPI *CreateSoundBuffer_t)(void *, const void *, void **, void *);
typedef HRESULT (WINAPI *SetVolume_t)(void *, LONG);
typedef HRESULT (WINAPI *Play_t)(void *, DWORD, DWORD, DWORD);
static DirectSoundCreate_t o_DirectSoundCreate;
static CreateSoundBuffer_t o_CreateSoundBuffer;
static SetVolume_t o_SetVolume;
static Play_t o_Play;
static unsigned g_mute_fail;

static void patch_vtbl(void **vt, int i, void *hook, void **orig)
{
    DWORD old;
    if (vt[i] == hook) return;
    VirtualProtect(&vt[i], sizeof(void *), PAGE_READWRITE, &old);
    if (!*orig) *orig = vt[i];
    vt[i] = hook;
    VirtualProtect(&vt[i], sizeof(void *), old, &old);
}
static HRESULT WINAPI h_SetVolume(void *b, LONG v) { (void)v; return o_SetVolume(b, -10000); }
static HRESULT WINAPI h_Play(void *b, DWORD r1, DWORD prio, DWORD flags)
{
    if (o_SetVolume(b, -10000) < 0 && g_mute_fail++ < 5) logf("mute: a sound buffer has no volume control\n");
    return o_Play(b, r1, prio, flags);
}
static HRESULT WINAPI h_CreateSoundBuffer(void *ds, const void *desc, void **buf, void *outer)
{
    HRESULT r = o_CreateSoundBuffer(ds, desc, buf, outer);
    if (r >= 0 && buf && *buf) {
        void **vt = *(void ***)*buf;
        patch_vtbl(vt, 15, (void *)h_SetVolume, (void **)&o_SetVolume);
        patch_vtbl(vt, 12, (void *)h_Play, (void **)&o_Play);
        o_SetVolume(*buf, -10000);
    }
    return r;
}
static HRESULT WINAPI h_DirectSoundCreate(void *guid, void **ds, void *outer)
{
    HRESULT r = o_DirectSoundCreate(guid, ds, outer);
    if (r >= 0 && ds && *ds) {
        patch_vtbl(*(void ***)*ds, 3, (void *)h_CreateSoundBuffer, (void **)&o_CreateSoundBuffer);
        logf("DirectSoundCreate: buffers muted\n");
    }
    return r;
}

/* ---- the state record (same "HOTDREC1" format as tools/mame_state_rec.lua; tools/state_diff.py reads both) ----
   "HOTDREC1", u32 range count, per range u32 address, u32 size; per tick "FRAM", u32 tick number, u32 frame counter,
   u32 game mode, u32 rand state, the ranges back to back, u32 task count (0 for now: task bodies to come). Ranges =
   the PC addresses of tools/state_map.tsv (lines without a PC address are left out). */
#define MAX_RANGES 512
static FILE *g_rec;
static DWORD g_rng_addr[MAX_RANGES], g_rng_size[MAX_RANGES];
static int g_nrng;

static void rec_open(const char *path, const char *map)
{
    FILE *m = fopen(map, "r");
    char line[1024];
    int i;
    if (!m) { logf("state map %s not found: no record\n", map); return; }
    while (fgets(line, sizeof line, m) && g_nrng < MAX_RANGES) {
        char name[256], a[32], as[32], pc[32], ps[32];
        if (line[0] == '#') continue;
        if (sscanf(line, "%255[^\t]\t%31[^\t]\t%31[^\t]\t%31[^\t]\t%31[^\t]", name, a, as, pc, ps) != 5) continue;
        if (pc[0] == '-' || ps[0] == '-') continue;
        g_rng_addr[g_nrng] = strtoul(pc, NULL, 16);
        g_rng_size[g_nrng] = strtoul(ps, NULL, 16);
        g_nrng++;
    }
    fclose(m);
    g_rec = fopen(path, "wb");
    if (!g_rec) return;
    fwrite("HOTDREC1", 1, 8, g_rec);
    fwrite(&g_nrng, 4, 1, g_rec);
    for (i = 0; i < g_nrng; i++) {
        fwrite(&g_rng_addr[i], 4, 1, g_rec);
        fwrite(&g_rng_size[i], 4, 1, g_rec);
    }
    logf("state record %s: %d ranges\n", path, g_nrng);
}

/* tasks run this tick, in run order (task_group_run's call at 0x43E386, see install_task_hook) */
static DWORD g_task_run[MAX_TASKS];
static unsigned g_ntask_run;

static void __stdcall note_task(DWORD task)
{
    if (g_ntask_run < MAX_TASKS) g_task_run[g_ntask_run++] = task;
}

static void rec_tasks(void)
{
    unsigned i;
    DWORD n = g_ntask_run;
    memcpy(g_last_tasks, g_task_run, n * sizeof(DWORD));   /* for the auto-aim at the next polls */
    g_nlast_tasks = n;
    fwrite(&n, 4, 1, g_rec);
    for (i = 0; i < n; i++) {
        DWORD t = g_task_run[i], size = 0;
        /* a task's block: the size given to task_create is kept at +0x10 (0x43E400); the task may have ended
           (freed) since it ran, so only dump what is readable */
        if (!IsBadReadPtr(VA(t), 0x14)) size = *(DWORD *)VA(t + 0x10);
        if (size > 0x10000 || IsBadReadPtr(VA(t), size)) size = 0;
        fwrite(&t, 4, 1, g_rec);
        fwrite(&size, 4, 1, g_rec);
        if (size) fwrite(VA(t), 1, size, g_rec);
    }
    g_ntask_run = 0;
}

static void rec_frame(void)
{
    DWORD hdr[4];
    int i;
    if (!g_rec) return;
    hdr[0] = g_ticks;
    hdr[1] = *(DWORD *)VA(PC_FRAME_COUNTER);
    hdr[2] = *(WORD *)VA(PC_GAME_MODE);
    hdr[3] = *(DWORD *)VA(PC_RAND_STATE);
    fwrite("FRAM", 1, 4, g_rec);
    fwrite(hdr, 4, 4, g_rec);
    for (i = 0; i < g_nrng; i++) {
        if (IsBadReadPtr(VA(g_rng_addr[i]), g_rng_size[i])) {
            static BYTE z[4096];
            DWORD left = g_rng_size[i];
            while (left) { DWORD n = left > sizeof z ? sizeof z : left; fwrite(z, 1, n, g_rec); left -= n; }
        } else
            fwrite(VA(g_rng_addr[i]), 1, g_rng_size[i], g_rec);
    }
    rec_tasks();
}

/* task_group_run (0x43E304) runs each task with "mov eax, edx; call [edx]; jmp 0x43E37D" at 0x43E386 (edx = the
   task): those 6 bytes become a jump to task_stub, which notes the task and then does the same */
static __declspec(naked) void task_stub(void)
{
    __asm {
        pushad
        push edx
        call note_task
        popad
        mov eax, edx
        call dword ptr [edx]
        push 0x43E37D
        ret
    }
}

static void install_task_hook(void)
{
    static const BYTE expect[6] = {0x89, 0xD0, 0xFF, 0x12, 0xEB, 0xF1};
    BYTE *at = (BYTE *)VA(0x43E386), jmp[6];
    if (memcmp(at, expect, 6) != 0) {
        logf("task_group_run bytes differ: no task hook\n");
        return;
    }
    jmp[0] = 0xE9;
    *(DWORD *)(jmp + 1) = (DWORD)task_stub - (DWORD)(at + 5);
    jmp[5] = 0x90;
    write_code(at, jmp, 6);
    logf("task hook at %p\n", (void *)at);
}

/* ---- per-routine capture (docs/proof.md "PA / PM routine captures") ----
   An int3 at each captured routine's entry and each boundary routine's entry; a vectored exception handler takes
   the breakpoint, records the state, puts the original byte back for one single step and re-arms it. The return
   address on the stack is swapped for a stub (an int3 of ours), so the return is caught without knowing the
   routine's ret instructions; the handler then continues at the real return address.
   Memory = the EXE's writable image (DGROUP + .bss), the heap blocks the harness hands out (VirtualAlloc /
   GlobalAlloc), and a window of the stack around the routine's esp, in 4 KB pages, written as byte runs against a
   shadow copy (the reader keeps the same shadow):
     "HOTDPCC1"; records, each "tag" + fields + u32 run count + runs (u32 address, u32 length, bytes):
     "CAPT" u32 routine, seq, tick, frontend_state, 9 regs (eax ecx edx ebx esp ebp esi edi eflags), 112-byte x87
            FLOATING_SAVE_AREA, runs = the state at entry (against the shadow)
     "PRE " runs = what the captured routine itself changed before the next boundary call
     "BND1" u32 routine, 9 regs at its entry, u32 eax at its return, x87 area at its return, runs = its effect
     "CEXT" 9 regs at the return (esp after the ret), x87 area, runs = what the routine changed after the last boundary
     "DROP" runs (none): the routine never returned (unwound by the task system); its records are to be ignored
     "MAPS" (before each CAPT) u32 count, (base, size) pairs: the committed writable ranges (all-zero pages included)
   Settings: HOTD_PC_CAP_FILE, HOTD_PC_CAP_FUNCS (hex list), HOTD_PC_CAP_BOUNDARY (hex list), HOTD_PC_CAP_MAX
   (per routine, default 30), HOTD_PC_CAP_IF ("addr:mask,..." u32 & mask != 0 at entry, one is enough),
   HOTD_PC_CAP_IF_ARG ("off:mask": the u32 at eax + off, eax = the first argument), HOTD_PC_CAP_FRONT (frontend_state
   to require, -1 = any), HOTD_PC_CAP_FROM (first tick). */
#define MAX_BP 512
typedef struct { DWORD addr; BYTE orig; int kind; unsigned count; } CapBp;   /* kind 1 captured, 2 boundary */
typedef struct { DWORD ret, esp, routine; int kind; DWORD regs[9]; } CapFrame;
static CapBp g_bp[MAX_BP];
static int g_nbp;
static CapFrame g_fr[8];
static int g_nfr;
static BYTE *g_ret_stub;
static DWORD g_rearm;
static unsigned g_cap_max = 30, g_cap_from, g_cap_total, g_cap_dropped;
static LONG g_cap_front = -1;
static DWORD g_if_addr[16], g_if_mask[16], g_ifarg_off, g_ifarg_mask;
static int g_nif, g_ifarg;
static DWORD g_blk_at[1024], g_blk_size[1024];
static int g_nblk;
static BYTE *g_shadow[0x100000];
static BYTE g_zero_page[4096];
typedef struct { BYTE *p; size_t n, cap; } CapBuf;
static CapBuf g_buf;
static DWORD g_nruns;

static void buf_put(CapBuf *b, const void *p, size_t n)
{
    if (b->n + n > b->cap) {
        b->cap = (b->n + n) * 2 + 65536;
        b->p = (BYTE *)realloc(b->p, b->cap);
    }
    memcpy(b->p + b->n, p, n);
    b->n += n;
}

static void cap_block(DWORD at, DWORD size)
{
    if (g_nblk < 1024) { g_blk_at[g_nblk] = at; g_blk_size[g_nblk] = size; g_nblk++; }
}

/* one page against its shadow: the differing runs (joined over gaps of under 8 equal bytes) into g_buf */
static void diff_page(DWORD page)
{
    const BYTE *cur = (const BYTE *)(page << 12), *sh = g_shadow[page] ? g_shadow[page] : g_zero_page;
    int i = 0;
    if (!memcmp(cur, sh, 4096)) return;
    while (i < 4096) {
        int j, eq;
        if (cur[i] == sh[i]) { i++; continue; }
        j = i;
        for (eq = 0; j < 4096 && eq < 8; j++) eq = cur[j] == sh[j] ? eq + 1 : 0;
        j -= eq;
        {
            DWORD a = (page << 12) + i, n = j - i;
            buf_put(&g_buf, &a, 4);
            buf_put(&g_buf, &n, 4);
            buf_put(&g_buf, cur + i, n);
            g_nruns++;
        }
        i = j;
    }
    if (!g_shadow[page]) g_shadow[page] = (BYTE *)malloc(4096);
    memcpy(g_shadow[page], cur, 4096);
}

static void diff_range(DWORD start, DWORD end)
{
    DWORD a = start & ~0xFFFu;
    while (a < end) {
        MEMORY_BASIC_INFORMATION mi;
        DWORD rend;
        if (!VirtualQuery((void *)a, &mi, sizeof mi)) break;
        rend = (DWORD)mi.BaseAddress + (DWORD)mi.RegionSize;
        if (rend > end) rend = end;
        if (mi.State == MEM_COMMIT && !(mi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
            (mi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) {
            DWORD p;
            for (p = a; p < rend; p += 4096) diff_page(p >> 12);
        }
        a = (rend + 0xFFF) & ~0xFFFu;
    }
}

/* the committed writable ranges of [start, end) into g_buf as (base, size) pairs (count in g_nruns) */
static void maps_range(DWORD start, DWORD end)
{
    DWORD a = start & ~0xFFFu;
    while (a < end) {
        MEMORY_BASIC_INFORMATION mi;
        DWORD rend;
        if (!VirtualQuery((void *)a, &mi, sizeof mi)) break;
        rend = (DWORD)mi.BaseAddress + (DWORD)mi.RegionSize;
        if (rend > end) rend = end;
        if (mi.State == MEM_COMMIT && !(mi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
            (mi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) {
            DWORD size = rend - a;
            buf_put(&g_buf, &a, 4);
            buf_put(&g_buf, &size, 4);
            g_nruns++;
        }
        a = (rend + 0xFFF) & ~0xFFFu;
    }
}

/* "MAPS": u32 count, (base, size) pairs: every committed writable range the state is taken from (pages that are
   still all zero never appear in the runs) */
static void cap_maps(DWORD esp)
{
    int i;
    DWORD lo = (esp - 0x1000) & ~0xFFFu, hi = (esp + 0x3000) & ~0xFFFu;
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb();
    if (hi > (DWORD)tib->StackBase) hi = (DWORD)tib->StackBase;
    g_buf.n = 0;
    g_nruns = 0;
    maps_range(0x5A0000, 0x1390000);
    for (i = 0; i < g_nblk; i++) maps_range(g_blk_at[i], g_blk_at[i] + g_blk_size[i]);
    maps_range(lo, hi);
    fwrite("MAPS", 1, 4, g_cap);
    fwrite(&g_nruns, 4, 1, g_cap);
    fwrite(g_buf.p, 1, g_buf.n, g_cap);
}

/* all state against the shadow: runs appended to g_buf (count in g_nruns), shadow updated */
static void diff_all(DWORD esp)
{
    int i;
    DWORD lo = (esp - 0x1000) & ~0xFFFu, hi = (esp + 0x3000) & ~0xFFFu;
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb();
    if (hi > (DWORD)tib->StackBase) hi = (DWORD)tib->StackBase;
    diff_range(0x5A0000, 0x1390000);   /* DGROUP + .bss */
    for (i = 0; i < g_nblk; i++) diff_range(g_blk_at[i], g_blk_at[i] + g_blk_size[i]);
    diff_range(lo, hi);
}

static void cap_record(const char *tag, const void *head, size_t headn, DWORD esp, int with_runs)
{
    g_buf.n = 0;
    g_nruns = 0;
    if (with_runs) diff_all(esp);
    fwrite(tag, 1, 4, g_cap);
    if (headn) fwrite(head, 1, headn, g_cap);
    fwrite(&g_nruns, 4, 1, g_cap);
    if (g_buf.n) fwrite(g_buf.p, 1, g_buf.n, g_cap);
}

/* the x87 state as the interrupted game code left it (the handler itself uses no x87): FNSAVE format, 108 bytes */
static void fpu_save(BYTE *out)
{
    BYTE b[112];
    __asm { fnsave b }
    __asm { frstor b }
    memset(b + 108, 0, 4);
    memcpy(out, b, 112);
}

static void ctx_regs(const CONTEXT *c, DWORD *r)
{
    r[0] = c->Eax; r[1] = c->Ecx; r[2] = c->Edx; r[3] = c->Ebx; r[4] = c->Esp; r[5] = c->Ebp; r[6] = c->Esi;
    r[7] = c->Edi; r[8] = c->EFlags;
}

static int cap_open(void) { return g_nfr > 0 && g_fr[0].kind == 1; }

/* Branch coverage (HOTD_PC_CAP_BRANCHES: tools/pc_branches.py's list of the ported routines' conditional jumps):
   while a capture is open, an int3 on every jump whose two edges have not both run; when hit, one single step and
   the new eip tells the edge. Written to HOTD_PC_CAP_COVER at the end (tools/branch_coverage.py). */
#define MAX_BR 8192
typedef struct { DWORD routine, at, target, fall; BYTE orig, armed, taken, fell; } CapBr;
static CapBr g_br[MAX_BR];
static int g_nbr;
static CapBr *g_br_pending;

static CapBr *br_find(DWORD at)
{
    int lo = 0, hi = g_nbr - 1;
    while (lo <= hi) {
        int m = (lo + hi) / 2;
        if (g_br[m].at == at) return &g_br[m];
        if (g_br[m].at < at) lo = m + 1; else hi = m - 1;
    }
    return NULL;
}
static void br_arm(int on)
{
    int i;
    for (i = 0; i < g_nbr; i++) {
        CapBr *b = &g_br[i];
        int want = on && !(b->taken && b->fell);
        if (want && !b->armed) { *(BYTE *)VA(b->at) = 0xCC; b->armed = 1; }
        else if (!want && b->armed) { *(BYTE *)VA(b->at) = b->orig; b->armed = 0; }
    }
}
static int br_cmp(const void *a, const void *b)
{
    DWORD x = ((const CapBr *)a)->at, y = ((const CapBr *)b)->at;
    return x < y ? -1 : x > y;
}
static void br_load(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[128];
    if (!f) return;
    while (fgets(line, sizeof line, f) && g_nbr < MAX_BR) {
        CapBr b;
        DWORD old;
        memset(&b, 0, sizeof b);
        if (sscanf(line, "%lx %lx %lx %lx", &b.routine, &b.at, &b.target, &b.fall) != 4) continue;
        VirtualProtect(VA(b.at), 1, PAGE_EXECUTE_READWRITE, &old);
        b.orig = *(BYTE *)VA(b.at);
        g_br[g_nbr++] = b;
    }
    fclose(f);
    qsort(g_br, g_nbr, sizeof g_br[0], br_cmp);
    logf("capture: %d conditional jumps for branch coverage\n", g_nbr);
}
static void br_write(const char *path)
{
    FILE *f = fopen(path, "w");
    int i;
    if (!f) return;
    fprintf(f, "# routine\tbranch\topcode\ttarget\tfall-through\ttarget ran\tfall-through ran\n");
    for (i = 0; i < g_nbr; i++)
        fprintf(f, "%lx\t%lx\t0\t%lx\t%lx\t%d\t%d\n", g_br[i].routine, g_br[i].at, g_br[i].target, g_br[i].fall,
                g_br[i].taken, g_br[i].fell);
    fclose(f);
}

/* frames whose routine is no longer on the stack (unwound by the task system): the capture is dropped */
static void cap_unwound(DWORD esp)
{
    while (g_nfr && esp > g_fr[g_nfr - 1].esp) {
        g_nfr--;
        if (g_nfr == 0) {
            cap_record("DROP", NULL, 0, 0, 0);
            g_cap_dropped++;
            br_arm(0);
        }
    }
}

static void cap_entry(CapBp *b, CONTEXT *c)
{
    DWORD regs[9];
    ctx_regs(c, regs);
    if (b->kind == 1) {
        int i, ok;
        if (g_nfr || b->count >= g_cap_max || g_ticks < g_cap_from) return;
        if (g_cap_front >= 0 && *(LONG *)VA(PC_FRONTEND_STATE) != g_cap_front) return;
        ok = g_nif == 0;
        for (i = 0; i < g_nif; i++) if (*(DWORD *)VA(g_if_addr[i]) & g_if_mask[i]) ok = 1;
        if (g_ifarg && (IsBadReadPtr(VA(c->Eax + g_ifarg_off), 4) || !(*(DWORD *)VA(c->Eax + g_ifarg_off) & g_ifarg_mask)))
            ok = 0;
        if (!ok) return;
        {
            BYTE head[16 + 36 + 112];
            DWORD h[4];
            h[0] = b->addr; h[1] = b->count + 1; h[2] = g_ticks; h[3] = *(DWORD *)VA(PC_FRONTEND_STATE);
            memcpy(head, h, 16);
            memcpy(head + 16, regs, 36);
            fpu_save(head + 52);
            cap_maps(c->Esp);
            cap_record("CAPT", head, sizeof head, c->Esp, 1);
        }
    } else {
        if (g_nfr != 1 || !cap_open()) return;   /* only directly inside an open capture, not nested in a boundary */
        cap_record("PRE ", NULL, 0, c->Esp, 1);
    }
    g_fr[g_nfr].ret = *(DWORD *)c->Esp;
    g_fr[g_nfr].esp = c->Esp;
    g_fr[g_nfr].routine = b->addr;
    g_fr[g_nfr].kind = b->kind;
    memcpy(g_fr[g_nfr].regs, regs, sizeof regs);
    g_nfr++;
    *(DWORD *)c->Esp = (DWORD)g_ret_stub;
    if (b->kind == 1) br_arm(1);
}

/* a clock read directly inside an open capture (the game calls timeGetTime / GetTickCount through its imports):
   recorded as a boundary call with that id (0xFFFFFF01 / 02), its value as the result and no memory effect */
static void cap_clock(DWORD routine, DWORD v)
{
    BYTE head[4 + 36 + 4 + 112];
    if (!g_cap || g_nfr != 1 || !cap_open()) return;
    memset(head, 0, sizeof head);
    memcpy(head, &routine, 4);
    memcpy(head + 40, &v, 4);
    cap_record("BND1", head, sizeof head, 0, 0);
}

static void cap_return(CONTEXT *c)
{
    CapFrame f;
    DWORD regs[9];
    if (!g_nfr) { logf("capture: return stub hit with no open frame\n"); TerminateProcess(GetCurrentProcess(), 3); }
    f = g_fr[--g_nfr];
    c->Eip = f.ret;
    ctx_regs(c, regs);
    if (f.kind == 2) {
        BYTE head[4 + 36 + 4 + 112];
        memcpy(head, &f.routine, 4);
        memcpy(head + 4, f.regs, 36);
        memcpy(head + 40, &c->Eax, 4);
        fpu_save(head + 44);
        cap_record("BND1", head, sizeof head, f.esp, 1);
    } else {
        BYTE head[36 + 112];
        int i;
        memcpy(head, regs, 36);
        fpu_save(head + 36);
        cap_record("CEXT", head, sizeof head, f.esp, 1);
        for (i = 0; i < g_nbp; i++) if (g_bp[i].addr == f.routine) g_bp[i].count++;
        g_cap_total++;
        br_arm(0);
    }
}

static LONG CALLBACK cap_veh(EXCEPTION_POINTERS *x)
{
    DWORD code = x->ExceptionRecord->ExceptionCode, at = (DWORD)x->ExceptionRecord->ExceptionAddress;
    CONTEXT *c = x->ContextRecord;
    int i;
    if (code == EXCEPTION_SINGLE_STEP && g_br_pending) {   /* one step after a conditional jump: its edge */
        CapBr *b = g_br_pending;
        g_br_pending = NULL;
        if (c->Eip == b->target) b->taken = 1;
        if (c->Eip == b->fall) b->fell = 1;
        if (cap_open() && !(b->taken && b->fell)) *(BYTE *)VA(b->at) = 0xCC;
        else b->armed = 0;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (code == EXCEPTION_SINGLE_STEP && g_rearm) {
        *(BYTE *)g_rearm = 0xCC;
        g_rearm = 0;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (code != EXCEPTION_BREAKPOINT) return EXCEPTION_CONTINUE_SEARCH;
    if (at == (DWORD)g_ret_stub) {
        cap_return(c);
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    {
        CapBr *b = g_nbr ? br_find(at) : NULL;
        if (b && b->armed) {
            *(BYTE *)at = b->orig;
            c->Eip = at;
            c->EFlags |= 0x100;
            g_br_pending = b;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }
    for (i = 0; i < g_nbp && g_bp[i].addr != at; i++) {}
    if (i == g_nbp) return EXCEPTION_CONTINUE_SEARCH;
    cap_unwound(c->Esp);
    cap_entry(&g_bp[i], c);
    *(BYTE *)at = g_bp[i].orig;   /* one step of the original instruction, then re-armed */
    c->Eip = at;
    c->EFlags |= 0x100;
    g_rearm = at;
    return EXCEPTION_CONTINUE_EXECUTION;
}

static void cap_add(const char *list, int kind)
{
    const char *p = list;
    while (p && *p && g_nbp < MAX_BP) {
        char *end;
        DWORD a = strtoul(p, &end, 16), old;
        if (end == p) { p++; continue; }
        p = end;
        VirtualProtect(VA(a), 1, PAGE_EXECUTE_READWRITE, &old);
        g_bp[g_nbp].addr = a;
        g_bp[g_nbp].orig = *(BYTE *)VA(a);
        g_bp[g_nbp].kind = kind;
        g_bp[g_nbp].count = 0;
        *(BYTE *)VA(a) = 0xCC;
        g_nbp++;
    }
}

static void cap_install(void)
{
    const char *s = getenv("HOTD_PC_CAP_FILE");
    if (!s || !*s) return;
    g_cap = fopen(s, "wb");
    if (!g_cap) return;
    fwrite("HOTDPCC1", 1, 8, g_cap);
    s = getenv("HOTD_PC_CAP_MAX");
    if (s && *s) g_cap_max = (unsigned)strtoul(s, NULL, 10);
    s = getenv("HOTD_PC_CAP_FROM");
    if (s && *s) g_cap_from = (unsigned)strtoul(s, NULL, 10);
    s = getenv("HOTD_PC_CAP_FRONT");
    if (s && *s) g_cap_front = strtol(s, NULL, 10);
    s = getenv("HOTD_PC_CAP_IF");
    while (s && *s && g_nif < 16) {
        char *end;
        g_if_addr[g_nif] = strtoul(s, &end, 16);
        if (*end != ':') break;
        g_if_mask[g_nif++] = strtoul(end + 1, &end, 16);
        s = *end == ',' ? end + 1 : end;
    }
    s = getenv("HOTD_PC_CAP_IF_ARG");
    if (s && *s) {
        char *end;
        g_ifarg_off = strtoul(s, &end, 16);
        if (*end == ':') { g_ifarg_mask = strtoul(end + 1, NULL, 16); g_ifarg = 1; }
    }
    g_ret_stub = (BYTE *)VirtualAlloc(NULL, 16, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    memset(g_ret_stub, 0xCC, 16);
    AddVectoredExceptionHandler(1, cap_veh);
    cap_add(getenv("HOTD_PC_CAP_FUNCS"), 1);
    cap_add(getenv("HOTD_PC_CAP_BOUNDARY"), 2);
    s = getenv("HOTD_PC_CAP_BRANCHES");
    if (s && *s) br_load(s);
    logf("capture: %d breakpoints, max %u per routine, front %ld, from tick %u\n", g_nbp, g_cap_max, g_cap_front,
         g_cap_from);
}

static void cap_close(void)
{
    int i;
    if (!g_cap) return;
    fclose(g_cap);
    g_cap = NULL;
    logf("capture: %u captures, %u dropped;", g_cap_total, g_cap_dropped);
    for (i = 0; i < g_nbp; i++) if (g_bp[i].kind == 1) logf(" %lx:%u", g_bp[i].addr, g_bp[i].count);
    logf("\n");
    {
        const char *s = getenv("HOTD_PC_CAP_COVER");
        if (s && *s && g_nbr) br_write(s);
    }
}

/* ---- the tick wrapper ---- */
static BYTE *g_tick_tramp;

static void __cdecl after_tick(void)
{
    g_ticks++;
    g_calls_since_tick = 0;
    {
        /* the x87 control word the game's tick runs with (precision control bits 8-9: 0 = 24-bit, 2 = 53-bit,
           3 = 64-bit; rounding bits 10-11): logged when it changes */
        static unsigned short last_cw = 0xFFFF;
        unsigned short cw;
        __asm fnstcw cw
        if (cw != last_cw) {
            logf("tick %u: x87 control word %04x (precision %s, rounding %u)\n", g_ticks, cw,
                 ((cw >> 8) & 3) == 0 ? "24-bit" : ((cw >> 8) & 3) == 2 ? "53-bit" : ((cw >> 8) & 3) == 3 ? "64-bit" : "?",
                 (cw >> 10) & 3);
            last_cw = cw;
        }
    }
    *(volatile DWORD *)VA(PC_TICK_MODE) = 0;
    if (g_inflives && *(LONG *)VA(PC_PLAYER0 + 0xC) < 3)   /* HOTD_PC_INFLIVES: player 1's lives kept at 3 or more */
        *(LONG *)VA(PC_PLAYER0 + 0xC) = 3;
    if (g_inflives2 && *(LONG *)VA(PC_PLAYER0 + 0xCC + 0xC) < 3)   /* HOTD_PC_TWO_PLAYERS: player 2's too */
        *(LONG *)VA(PC_PLAYER0 + 0xCC + 0xC) = 3;
    for (int r = 0; r < g_nroute; r++)   /* HOTD_PC_ROUTE: the other route out of the listed events */
        if (*(BYTE *)VA(PC_CHAPTER) == g_route_ch[r] && *(BYTE *)VA(PC_EVENT_INDEX) == g_route_ev[r]
            && *(BYTE *)VA(PC_ROUTE_CHOICE) != (BYTE)g_route_val[r]) {
            *(BYTE *)VA(PC_ROUTE_CHOICE) = (BYTE)g_route_val[r];
            logf("tick %u: route_choice forced to %d (chapter %d event %d)\n", g_ticks, g_route_val[r], g_route_ch[r] + 1,
                 g_route_ev[r]);
        }
    if (g_cap) { DWORD here = (DWORD)&here; cap_unwound(here); }
    rec_frame();
    logf("tick %u poll %u frame %lu mode %08lx rand %08lx front %ld\n", g_ticks, g_polls,
         *(DWORD *)VA(PC_FRAME_COUNTER),
         (DWORD)*(WORD *)VA(PC_GAME_MODE), *(DWORD *)VA(PC_RAND_STATE), *(LONG *)VA(PC_FRONTEND_STATE));
    if (g_stop_after && g_ticks >= g_stop_after) {
        logf("stop after %u ticks\n", g_ticks);
        harness_stop();
    }
}

/* ends the run: open capture, state record and log closed, the game process ended */
static void harness_stop(void)
{
    cap_close();
    if (g_rec) fclose(g_rec);
    g_rec = NULL;
    if (g_log) fclose(g_log);
    g_log = NULL;
    TerminateProcess(GetCurrentProcess(), 0);
}

/* frame_tick takes no stack arguments and the game keeps everything it needs in callee-saved registers
   around the call (Watcom register convention): keep all registers and flags as the original leaves them */
static __declspec(naked) void tick_hook(void)
{
    __asm {
        call dword ptr [g_tick_tramp]
        pushad
        pushfd
        call after_tick
        popfd
        popad
        ret
    }
}

static void install_tick_hook(void)
{
    /* frame_tick starts: 53 51 52 A1 xx xx xx xx (push ebx, push ecx, push edx, mov eax, [abs]):
       the first 8 bytes move to a trampoline unchanged (no relative operands) */
    static const BYTE expect[4] = {0x53, 0x51, 0x52, 0xA1};
    BYTE *f = (BYTE *)VA(PC_FRAME_TICK), jmp[5];
    if (memcmp(f, expect, 4) != 0) {
        logf("frame_tick bytes differ: no tick hook\n");
        return;
    }
    g_tick_tramp = (BYTE *)VirtualAlloc(NULL, 32, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    memcpy(g_tick_tramp, f, 8);
    g_tick_tramp[8] = 0xE9;
    *(DWORD *)(g_tick_tramp + 9) = (DWORD)(f + 8) - (DWORD)(g_tick_tramp + 13);
    jmp[0] = 0xE9;
    *(DWORD *)(jmp + 1) = (DWORD)tick_hook - (DWORD)(f + 5);
    write_code(f, jmp, 5);
    logf("tick hook at %p\n", (void *)f);
}

/* ---- the forwarded DirectInput entry ---- */
typedef HRESULT (WINAPI *DirectInputCreateA_t)(HINSTANCE, DWORD, void **, void *);
static DirectInputCreateA_t o_DirectInputCreateA;

HRESULT WINAPI DirectInputCreateA(HINSTANCE inst, DWORD ver, void **out, void *outer)
{
    if (!o_DirectInputCreateA) {
        char path[MAX_PATH];
        HMODULE real;
        GetSystemDirectoryA(path, MAX_PATH);   /* SysWOW64 for a 32-bit process */
        strcat(path, "\\dinput.dll");
        real = LoadLibraryA(path);
        o_DirectInputCreateA = real ? (DirectInputCreateA_t)GetProcAddress(real, "DirectInputCreateA") : NULL;
        logf("DirectInputCreateA -> %s (%p)\n", path, (void *)o_DirectInputCreateA);
    }
    if (!o_DirectInputCreateA) return E_FAIL;
    {
        HRESULT r = o_DirectInputCreateA(inst, ver, out, outer);
        if (r >= 0 && out && *out)
            patch_vtbl_slot(*(void ***)*out, 3, (void *)h_DICreateDevice, (void **)&o_DICreateDevice);
        return r;
    }
}

static void install(void)
{
    HMODULE game = GetModuleHandleA(NULL);
    const char *s;
    int n;
    s = getenv("HOTD_PC_LOG");
    g_log = fopen(s && *s ? s : "hotd_pc.log", "w");
    s = getenv("HOTD_PC_TICKS");
    g_stop_after = s ? (unsigned)strtoul(s, NULL, 10) : 0;
    logf("hotd_hook: game module %p, stop after %u ticks\n", (void *)game, g_stop_after);

    n = patch_iat(game, proc("winmm.dll", "timeGetTime"), (void *)h_timeGetTime);
    n += patch_iat(game, proc("kernel32.dll", "GetTickCount"), (void *)h_GetTickCount);
    logf("clock: %d import slots\n", n);
    patch_iat(game, proc("kernel32.dll", "VirtualAlloc"), (void *)h_VirtualAlloc);
    patch_iat(game, proc("kernel32.dll", "GlobalAlloc"), (void *)h_GlobalAlloc);
    patch_iat(game, proc("kernel32.dll", "GlobalLock"), (void *)h_GlobalLock);
    patch_iat(game, proc("kernel32.dll", "GlobalUnlock"), (void *)h_GlobalUnlock);
    patch_iat(game, proc("kernel32.dll", "GlobalSize"), (void *)h_GlobalSize);
    patch_iat(game, proc("kernel32.dll", "GlobalFree"), (void *)h_GlobalFree);
    patch_iat(game, proc("user32.dll", "SetCursorPos"), (void *)h_SetCursorPos);
    patch_iat(game, proc("user32.dll", "ClipCursor"), (void *)h_ClipCursor);
    patch_iat(game, proc("user32.dll", "GetCursorPos"), (void *)h_GetCursorPos);
    patch_iat(game, proc("user32.dll", "GetActiveWindow"), (void *)h_GetActiveWindow);
    patch_iat(game, proc("user32.dll", "GetAsyncKeyState"), (void *)h_GetAsyncKeyState);
    patch_iat(game, proc("winmm.dll", "joyGetPosEx"), (void *)h_joyGetPosEx);
    patch_iat(game, proc("winmm.dll", "joyGetPos"), (void *)h_joyGetPos);
    patch_iat(game, proc("user32.dll", "RegisterClassA"), (void *)h_RegisterClassA);
    patch_iat(game, proc("user32.dll", "CreateWindowExA"), (void *)h_CreateWindowExA);
    patch_iat(game, proc("kernel32.dll", "GlobalFindAtomA"), (void *)h_GlobalFindAtomA);
    patch_iat(game, proc("kernel32.dll", "GlobalAddAtomA"), (void *)h_GlobalAddAtomA);
    patch_iat(game, proc("kernel32.dll", "GlobalDeleteAtom"), (void *)h_GlobalDeleteAtom);
    patch_iat(game, proc("user32.dll", "FindWindowA"), (void *)h_FindWindowA);
    s = getenv("HOTD_PC_CFG_DIR");
    if (s && *s) {
        lstrcpynA(g_cfg_dir, s, MAX_PATH);
        n = patch_iat(game, proc("kernel32.dll", "CreateFileA"), (void *)h_CreateFileA);
        patch_iat(game, proc("kernel32.dll", "DeleteFileA"), (void *)h_DeleteFileA);
        patch_iat(game, proc("kernel32.dll", "MoveFileA"), (void *)h_MoveFileA);
        patch_iat(game, proc("kernel32.dll", "GetFileAttributesA"), (void *)h_GetFileAttributesA);
        logf("settings files in %s (%d CreateFileA slots)\n", g_cfg_dir, n);
    }
    s = getenv("HOTD_PC_MUTE");
    if (s && *s == '1') {
        o_DirectSoundCreate = (DirectSoundCreate_t)proc("dsound.dll", "DirectSoundCreate");
        patch_iat(game, (void *)o_DirectSoundCreate, (void *)h_DirectSoundCreate);
    }

    /* process-wide (the graphics wrapper calls these itself) */
    o_ShowWindow = (ShowWindow_t)hook_export("user32.dll", "ShowWindow", (void *)h_ShowWindow);
    hook_export("user32.dll", "SetForegroundWindow", (void *)h_SetForegroundWindow);
    hook_export("user32.dll", "SetActiveWindow", (void *)h_SetActiveWindow);
    hook_export("user32.dll", "BringWindowToTop", (void *)h_BringWindowToTop);
    hook_export("user32.dll", "SetFocus", (void *)h_SetFocus);
    hook_export("user32.dll", "GetForegroundWindow", (void *)h_GetForegroundWindow);
    hook_export("user32.dll", "SetCursorPos", (void *)h_SetCursorPos);
    hook_export("user32.dll", "ClipCursor", (void *)h_ClipCursor);

    *(volatile DWORD *)VA(PC_TICK_MODE) = 0;
    s = getenv("HOTD_PC_REC");
    if (s && *s) {
        const char *m = getenv("HOTD_PC_MAP");
        rec_open(s, m && *m ? m : "state_map.tsv");
        if (g_rec) install_task_hook();
    }
    s = getenv("HOTD_PC_INPUT");
    if (s && *s) input_load(s);
    s = getenv("HOTD_PC_INFLIVES");
    g_inflives = s && *s == '1';
    s = getenv("HOTD_PC_TWO_PLAYERS");
    g_inflives2 = g_inflives && s && *s == '1';
    s = getenv("HOTD_PC_ROUTE");   /* "chapter:event[=value],..." (chapter 1-based): route_choice forced (default 1) there */
    while (s && *s && g_nroute < 32) {
        int c = 0, e = 0, v = 1, n = sscanf(s, "%d:%d=%d", &c, &e, &v);
        if (n >= 2) { g_route_ch[g_nroute] = c - 1; g_route_ev[g_nroute] = e; g_route_val[g_nroute] = n == 3 ? v : 1; g_nroute++; }
        s = strchr(s, ',');
        if (s) s++;
    }
    s = getenv("HOTD_PC_AIM_SKIP");   /* "4ccf20,...": tasks running these routines are never aimed at */
    while (s && *s && g_naim_skip < 16) {
        g_aim_skip[g_naim_skip++] = (DWORD)strtoul(s, NULL, 16);
        s = strchr(s, ',');
        if (s) s++;
    }
    s = getenv("HOTD_PC_TRACE_MENU");
    g_trace_menu = s && *s == '1';
    install_tick_hook();
    cap_install();
}

/* Every visible top-level window of the game (its own and the one dgVoodoo presents in) is kept out of the way: no
   taskbar / Alt-Tab button, minimize requests refused (a minimized game stops for good: its display surface is
   lost), restored if minimized anyway, kept off screen; never activated. */
#define MAX_GUARDED 8
static HWND g_guarded[MAX_GUARDED];
static WNDPROC g_guarded_proc[MAX_GUARDED];
static int g_nguarded;
static LRESULT CALLBACK h_guard_wndproc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    int i;
    for (i = 0; i < g_nguarded; i++)
        if (g_guarded[i] == h) break;
    if (msg == WM_SYSCOMMAND && (wp & 0xFFF0) == SC_MINIMIZE) return 0;
    if (i == g_nguarded) return DefWindowProcA(h, msg, wp, lp);
    return CallWindowProcA(g_guarded_proc[i], h, msg, wp, lp);
}
static BOOL CALLBACK guard_window(HWND h, LPARAM lp)
{
    DWORD pid = 0;
    RECT r;
    LONG ex;
    int i;
    (void)lp;
    GetWindowThreadProcessId(h, &pid);
    if (pid != GetCurrentProcessId() || !IsWindowVisible(h)) return TRUE;
    for (i = 0; i < g_nguarded; i++)
        if (g_guarded[i] == h) break;
    if (i == g_nguarded && g_nguarded < MAX_GUARDED) {
        if (h != g_hwnd) {   /* the game's own window already refuses minimize (h_wndproc) */
            g_guarded_proc[g_nguarded] = (WNDPROC)SetWindowLongPtrA(h, GWLP_WNDPROC, (LONG_PTR)h_guard_wndproc);
            g_guarded[g_nguarded++] = h;
        }
    }
    ex = GetWindowLongA(h, GWL_EXSTYLE);   /* every pass: dgVoodoo restyles the window after it is created */
    if ((ex & WS_EX_APPWINDOW) || !(ex & WS_EX_TOOLWINDOW)) {
        SetWindowLongA(h, GWL_EXSTYLE, (ex & ~WS_EX_APPWINDOW) | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE);
        /* the taskbar drops the button when the window is shown again */
        SetWindowPos(h, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);
        SetWindowPos(h, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_FRAMECHANGED);
        logf("window %p: taskbar button removed\n", (void *)h);
    }
    if (IsIconic(h)) {
        ShowWindowAsync(h, SW_SHOWNOACTIVATE);
        logf("window %p was minimized: restored (no activation)\n", (void *)h);
    }
    if (GetWindowRect(h, &r) && r.left > -10000)
        SetWindowPos(h, NULL, -20000, -20000, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
    return TRUE;
}
static DWORD WINAPI window_watchdog(LPVOID arg)
{
    (void)arg;
    for (;;) {
        Sleep(250);
        EnumWindows(guard_window, 0);
    }
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    (void)inst; (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(inst);
        install();
        CreateThread(NULL, 0, window_watchdog, NULL, 0, NULL);
    }
    return TRUE;
}
