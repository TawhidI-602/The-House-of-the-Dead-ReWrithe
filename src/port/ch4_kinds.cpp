// Chapter 4's own kinds (worker "ch2proof" for Chapter 4, docs/chapter2_3_plan.md): the route object 191, the event
// model 22, the prop group 154 and Curien's scene body 25. Read from the arcade disassembly (the decompilation as a
// guide); arcade forms only. UNVERIFIED (teaser) unless marked proven below.
#include "port/game.h"

namespace hotd::port {

// 0x4D50 kind191_route_by_rescues: route_choice = 1 when every hostage that appeared was rescued
// (hostages_rescued == hostages_appeared), else 0; the task ends
// Proven on A (vf_c4c: 1 captures, 0 differ)
template <class P> void Game<P>::kind191_route_by_rescues(uint32_t obj)
{
    (void)obj;
    mem.at<uint8_t>(G::route_choice) =
        mem.at<int16_t>(G::hostages_rescued) == mem.at<int16_t>(G::hostages_appeared) ? 1 : 0;
    task_exit();
}

// 0x31390 event_model_fade (kind 22): the record (+0x35C) gives the scale (x, y, z: words 0 / 4 / 8) and the fade
// level (+0xC); model 0x765 drawn at its place (turned z, y, x, scaled) with that fade until flag 213 (0x51EA15)
// Proven on A (ch4p_full: 16 captures, 0 differ)
template <class P> void Game<P>::event_model_fade(uint32_t obj)
{
    const int16_t s = of<int16_t>(obj, 0x60);
    const uint32_t rec = of<uint32_t>(obj, 0x35C);
    bool check = s == 1;
    if (s == 0) {
        of<uint32_t>(obj, 0x33C) = mem.u32(rec);
        of<uint32_t>(obj, 0x340) = mem.u32(rec + 4);
        of<uint32_t>(obj, 0x344) = mem.u32(rec + 8);
        const int16_t f = mem.at<int16_t>(rec + 0xC);
        of<int32_t>(obj, 0x50) = 0;
        of<uint32_t>(obj, 0x54) = 0x765;
        of<int16_t>(obj, 0x60) = int16_t(s + 1);
        of<int16_t>(obj, 0x2E0) = f;
        check = true;
    }
    if (check && mem.at<uint8_t>(G::event_flags + 213) == 1) {
        task_exit();
        return;
    }
    cam_copro(0x05, {});
    cam_copro(0x12, {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)});
    cam_copro(0x16, {of<uint32_t>(obj, 0x34)});
    cam_copro(0x15, {of<uint32_t>(obj, 0x30)});
    cam_copro(0x14, {of<uint32_t>(obj, 0x2C)});
    cam_copro(0x13, {of<uint32_t>(obj, 0x33C), of<uint32_t>(obj, 0x340), of<uint32_t>(obj, 0x344)});
    draw_fade_begin(of<int16_t>(obj, 0x2E0));
    geo_draw_model(of<uint32_t>(obj, 0x54));
    geo_send_words(0, 0x1F, 0x7A0);
    cam_copro(0x06, {});
}

// 0x3B020 kind154_prop_group: five prop090_drop props (sub-type and hit points from the group, models and places
// from the table 0xAF8D0, 7 words each: model, x / y / z offsets from (750.365, -18.9159, -1158.45), angles x / y / z),
// scale 1, hit radius 60, the group's action; the task ends
// Proven on A (ch4p_size: 1 captures, 0 differ)
template <class P> void Game<P>::kind154_prop_group(uint32_t obj)
{
    for (uint32_t i = 0; i < 5; ++i) {
        const uint32_t t = task_create(F::prop090_drop, 0x3C8);
        task_clear_work(t);
        of<int16_t>(t, 0xCC) = of<int16_t>(obj, 0xCC);
        prop_init(t);
        const uint32_t e = 0xAF8D0 + i * 7 * 4;
        of<int16_t>(t, 0x70) = of<int16_t>(obj, 0x70);
        of<uint32_t>(t, 0x54) = mem.u32(e);
        of<float>(t, 0x20) = float(double(mem.f32(e + 4)) - 750.365);
        of<float>(t, 0x24) = float(double(mem.f32(e + 8)) + 18.9159);
        of<float>(t, 0x28) = float(double(mem.f32(e + 12)) + 1158.45);
        of<uint32_t>(t, 0x2C) = mem.u32(e + 16);
        of<uint32_t>(t, 0x30) = mem.u32(e + 20);
        of<float>(t, 0x94) = 1.0f;
        of<uint32_t>(t, 0x7C) = 0;
        of<float>(t, 0x90) = 60.0f;
        of<uint32_t>(t, 0x34) = mem.u32(e + 24);
        of<int16_t>(t, 0x64) = of<int16_t>(obj, 0x64);
    }
    task_exit();
}

// --- kind 25: Curien in Chapter 4's last event (scene object): stands, points, is struck and collapses as the event
// flags 128-131 (0x51E9C0..) say ---

// 0x87F10 kind25_kyurianb_curien: type 0x4E, body (motion 0x12A), at (-1112.55, 0, 2027.59) facing 0xC000
// Proven on A (ch4p_full: 1 captures, 0 differ)
template <class P> void Game<P>::kind25_kyurianb_curien(uint32_t obj)
{
    of<int16_t>(obj, 0xCC) = 0x4E;
    mem.set32(G::hit_type, 0x4E);
    body_init(obj, 0x4E, 0x12A);
    creature_type_setup(obj);
    of<uint32_t>(obj, 0x20) = 0xC48B119Au;   // -1112.55
    of<uint32_t>(obj, 0x28) = 0x44FD72E1u;   // 2027.59
    of<uint32_t>(obj, 0x30) = 0xC000;
    of<uint32_t>(obj, 0) = F::kyurianb_wait_scene;
    of<uint32_t>(obj, 0x24) = 0;
    shadow_setup(0.0f);
    object_draw(obj);
}

// the common end of the kyurianb states: shadow at his height, drawn
template <class P> void Game<P>::kyurianb_draw(uint32_t obj, bool step)
{
    shadow_setup(of<float>(obj, 0x24));
    object_draw(obj);
    if (step)
        of<int32_t>(obj, 0xF8) += 1;
}

// 0x87F80 kyurianb_wait_scene: until flag 128
// Proven on A (ch4p_full: 34 captures, 0 differ)
template <class P> void Game<P>::kyurianb_wait_scene(uint32_t obj)
{
    if (mem.at<uint8_t>(G::event_flags + 128) != 0)
        of<uint32_t>(obj, 0) = F::kyurianb_wait_point;
    kyurianb_draw(obj, false);
}

// 0x87FB0 kyurianb_wait_point: flag 129: the pointing motion 0x128
// Proven on A (ch4p_full: 35 captures, 0 differ)
template <class P> void Game<P>::kyurianb_wait_point(uint32_t obj)
{
    if (mem.at<uint8_t>(G::event_flags + 129) != 0) {
        set_motion_blend(obj, 0x128, 0, 5);
        of<uint32_t>(obj, 0) = F::kyurianb_pointing;
    }
    kyurianb_draw(obj, true);
}

// 0x88000 kyurianb_pointing: held at frame 75
// Proven on A (ch4p_full: 5 captures, 0 differ)
template <class P> void Game<P>::kyurianb_pointing(uint32_t obj)
{
    if (of<int32_t>(obj, 0xF8) == 0x4C) {
        of<int32_t>(obj, 0xF8) = 0x4B;
        of<uint32_t>(obj, 0) = F::kyurianb_wait_struck;
    }
    kyurianb_draw(obj, true);
}

// 0x88050 kyurianb_wait_struck: flag 130: the struck motion 0x12B
// Proven on A (ch4p_full: 64 captures, 0 differ)
template <class P> void Game<P>::kyurianb_wait_struck(uint32_t obj)
{
    if (mem.at<uint8_t>(G::event_flags + 130) != 0) {
        set_motion_blend(obj, 0x12B, 0, 8);
        of<uint32_t>(obj, 0) = F::kyurianb_struck;
    }
    kyurianb_draw(obj, false);
}

// 0x88090 kyurianb_struck: at the motion's end (MotFinish) motion 0x299
// Proven on A (ch4p_full: 6 captures, 0 differ)
template <class P> void Game<P>::kyurianb_struck(uint32_t obj)
{
    if ((of<uint8_t>(obj, 0x3C5) >> 4) & 1) {
        set_motion(obj, 0x299);
        of<uint32_t>(obj, 0) = F::kyurianb_collapse_a;
    }
    kyurianb_draw(obj, true);
}

// 0x880D0 kyurianb_collapse_a: at the motion's end motion 0x124, 350 frames
// Proven on A (ch4p_full: 3 captures, 0 differ)
template <class P> void Game<P>::kyurianb_collapse_a(uint32_t obj)
{
    if ((of<uint8_t>(obj, 0x3C5) >> 4) & 1) {
        set_motion_blend(obj, 0x124, 0, 5);
        of<int32_t>(obj, 0x50) = 0x15E;
        of<uint32_t>(obj, 0) = F::kyurianb_collapse_b;
    }
    kyurianb_draw(obj, true);
}

// 0x88120 kyurianb_collapse_b: after them motion 0x125
// Proven on A (ch4p_full: 18 captures, 0 differ)
template <class P> void Game<P>::kyurianb_collapse_b(uint32_t obj)
{
    const int32_t t = of<int32_t>(obj, 0x50) - 1;
    of<int32_t>(obj, 0x50) = t;
    if (t == 0) {
        set_motion_blend(obj, 0x125, 0, 5);
        of<uint32_t>(obj, 0) = F::kyurianb_collapse_c;
    }
    kyurianb_draw(obj, true);
}

// 0x88180 kyurianb_collapse_c: held at frame 24
// Proven on A (ch4p_full: 2 captures, 0 differ)
template <class P> void Game<P>::kyurianb_collapse_c(uint32_t obj)
{
    if (of<int32_t>(obj, 0xF8) == 0x19) {
        of<int32_t>(obj, 0xF8) = 0x18;
        of<uint32_t>(obj, 0) = F::kyurianb_wait_end;
    }
    kyurianb_draw(obj, true);
}

// 0x881C0 kyurianb_wait_end: flag 131: freed
// Proven on A (ch4p_full: 18 captures, 0 differ)
template <class P> void Game<P>::kyurianb_wait_end(uint32_t obj)
{
    if (mem.at<uint8_t>(G::event_flags + 131) != 0) {
        body_free_parts(obj);
        enemy_free_extras(obj);
        task_exit();
        return;
    }
    kyurianb_draw(obj, false);
}

// --- the player's Chapter 4 cut-scene actions (CCTB 18; the task's +0x20 = the player; record 0x51F280 + 0xCC p) ---

// 0x4DF50 player_scene17_dash: the body at the table 0xC1058 ([players in game][player]: x, y, z), facing 0xC000;
// player 2 of two waits 25 frames (0x4E0B0), else motion 0x180 at once (0x4E040); camera flag on
// Proven on A (ch4p_full: 1 captures, 0 differ)
template <class P> void Game<P>::player_scene17_dash(uint32_t task)
{
    const uint32_t p = uint32_t(int32_t(mem.at<int8_t>(task + 0x20)));
    Player& pl = mem.at<Player>(G::players, p);
    const uint32_t body = pl.body.a;
    const uint32_t at = 0xC1058 + p * 0xC + uint32_t(int32_t(mem.at<int16_t>(G::players_in_game))) * 0x18;
    Obj& o = mem.at<Obj>(body);
    o.pos[0] = mem.f32(at);
    o.pos[1] = mem.f32(at + 4);
    o.ang[0] = 0;
    o.ang[2] = 0;
    o.ang[1] = 0xC000;
    o.pos[2] = mem.f32(at + 8);
    if (mem.at<int16_t>(G::players_in_game) == 2 && mem.at<uint8_t>(task + 0x20) == 1) {
        pl.timer = 25;
        pl.scene_routine = 0x4E0B0;
        pl.cam_flags = uint8_t(pl.cam_flags & ~1);
        return;
    }
    pl.cam_flags = uint8_t(pl.cam_flags | 1);
    set_motion(body, 0x180);
    mem.at<Player>(G::players, p).scene_routine = 0x4E040;
}

// 0x4E040: anim_time 14: sound 0xA01223; at the motion's end motion 0x153 blended in (2), then nothing (0x4E0A0)
// Proven on A (ch4p_full: 5 captures, 0 differ)
template <class P> void Game<P>::player_scene17_dash_p1(uint32_t task)
{
    Player& pl = mem.at<Player>(G::players, uint32_t(int32_t(mem.at<int8_t>(task + 0x20))));
    const uint32_t body = pl.body.a;
    Obj& o = mem.at<Obj>(body);
    if (o.anim_time == 14) {
        sound_cue(0xA01223);
        return;
    }
    if ((mem.at<uint8_t>(body + 0x3C5) >> 4) & 1) {
        set_motion_blend(body, 0x153, 0, 2);
        mem.at<Player>(G::players, uint32_t(int32_t(mem.at<int8_t>(task + 0x20)))).scene_routine = 0x4E0A0;
    }
}

// 0x4E0A0 / 0x4E150 / 0x4E270: the scene actions' end (ret)
// UNVERIFIED (teaser)
template <class P> void Game<P>::player_scene_nop(uint32_t task)
{
    (void)task;
}

// 0x4E0B0: player 2 after its wait: camera flag on, motion 0xC7, 0x4E100
// Proven on A (vf_c4c: 3 captures, 0 differ)
template <class P> void Game<P>::player_scene17_dash_p2(uint32_t task)
{
    Player& pl = mem.at<Player>(G::players, uint32_t(int32_t(mem.at<int8_t>(task + 0x20))));
    const uint32_t body = pl.body.a;
    pl.timer = pl.timer - 1;
    if (pl.timer != 0)
        return;
    pl.cam_flags = uint8_t(pl.cam_flags | 1);
    set_motion(body, 0xC7);
    mem.at<Player>(G::players, uint32_t(int32_t(mem.at<int8_t>(task + 0x20)))).scene_routine = 0x4E100;
}

// 0x4E100: anim_time 10: sound 0xA01225; at the motion's end motion 0xBC, then nothing (0x4E150)
// Proven on A (vf_c4c: 8 captures, 0 differ)
template <class P> void Game<P>::player_scene17_dash_p2_p(uint32_t task)
{
    Player& pl = mem.at<Player>(G::players, uint32_t(int32_t(mem.at<int8_t>(task + 0x20))));
    const uint32_t body = pl.body.a;
    Obj& o = mem.at<Obj>(body);
    if (o.anim_time == 10) {
        sound_cue(0xA01225);
        return;
    }
    if ((mem.at<uint8_t>(body + 0x3C5) >> 4) & 1) {
        set_motion(body, 0xBC);
        mem.at<Player>(G::players, uint32_t(int32_t(mem.at<int8_t>(task + 0x20)))).scene_routine = 0x4E150;
    }
}

// 0x4E160 player_body_at_scene_pos: the body on the floor (y 0) at the table 0xC1088 (x, z), facing 0x4000, the
// player's motion from 0xC10D0 started at a random frame ((rand % 50) x (player + 1)); then 0x4E220
// Proven on A (ch4p_full: 1 captures, 0 differ)
template <class P> void Game<P>::player_body_at_scene_pos(uint32_t task)
{
    const uint32_t p = uint32_t(int32_t(mem.at<int8_t>(task + 0x20)));
    const uint32_t body = mem.at<Player>(G::players, p).body.a;
    const uint32_t at = 0xC1088 + p * 0xC + uint32_t(int32_t(mem.at<int16_t>(G::players_in_game))) * 0x18;
    mem.set32(body + 0x24, 0);
    mem.set32(body + 0x20, mem.u32(at));
    mem.set32(body + 0x2C, 0);
    mem.set32(body + 0x34, 0);
    mem.set32(body + 0x30, 0x4000);
    mem.set32(body + 0x28, mem.u32(at + 8));
    set_motion(body, mem.u32(0xC10D0 + p * 4));
    const int32_t r = int32_t(rand()) % 50;
    mem.set32(body + 0xF8, uint32_t(r * int32_t(mem.at<int8_t>(task + 0x20) + 1)));
    mem.at<Player>(G::players, p).scene_routine = 0x4E220;
}

// 0x4E220: when the camera's 0x51EB9C reaches 990: camera flag off, then nothing (0x4E270)
// Proven on A (ch4p_size: 134 captures, 0 differ)
template <class P> void Game<P>::player_body_at_scene_pos_wait(uint32_t task)
{
    Player& pl = mem.at<Player>(G::players, uint32_t(int32_t(mem.at<int8_t>(task + 0x20))));
    if (mem.f32(0x51EB9C) == ffrom(0x44778000u)) {
        pl.scene_routine = 0x4E270;
        pl.cam_flags = uint8_t(pl.cam_flags & ~1);
    }
}

} // namespace hotd::port