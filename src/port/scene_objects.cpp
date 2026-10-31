// Chapter 1's scene objects, teaser L5: kind 161 the voice lines (voice_message_task), the doors 162-164 / 166 / 167,
// kind 173 (the drops of Chapter 1 events 0 / 1), kind 180 (the room object of event 4 that kind 70 rides on), the
// opening title 172, the chapter card 177 and the scene skip 178. Read from the arcade disassembly; arcade forms only.
// The 2D layer (sprites, tile text, the gun flash, the palette) stays boundaries: the engine draws it.
#include "port/game.h"

namespace hotd::port {

// --- kind 161: a voice line with its subtitle (started by ENTB2 and by voice_message) ---

// 0x9B380: the line for who plays (+0x54: 0 player 1 alone, 1 player 2 alone, 2 both; table voice_message_task_p,
// 3 bytes per line +0x70; 0 = no line for them: ended); its sound, the subtitle sprite (Japan) or text, the frames
template <class P> void Game<P>::voice_message_task(uint32_t task)
{
    const int32_t n = mem.at<int16_t>(G::players_in_game);
    const uint32_t row = F::voice_message_task_p + uint32_t(int32_t(of<int16_t>(task, 0x70))) * 3;
    if (n != 1) {
        if (mem.at<uint8_t>(row + 2) == 0) {
            task_exit();
            return;
        }
        of<uint32_t>(task, 0x54) = 2;
    } else {
        const int8_t solo = mem.at<int8_t>(G::solo_player_is_p2);
        if (solo == 0) {
            if (mem.at<uint8_t>(row) == 0) {
                task_exit();
                return;
            }
            of<uint32_t>(task, 0x54) = 0;
        } else if (solo == 1) {
            if (mem.at<uint8_t>(row + 1) == 0) {
                task_exit();
                return;
            }
            of<uint32_t>(task, 0x54) = 1;
        }
    }
    const uint32_t e = mem.at<uint8_t>(row + of<uint32_t>(task, 0x54));
    const uint32_t rec = G::voice_messages + e * 12;
    const uint32_t snd = mem.u32(rec + 8);
    if (snd != 0xA00000)
        sound_cue(snd);
    if (text_region() == 0) {
        sprite_draw_anim(mem.at<uint8_t>(rec + 6) != 0 ? 0x1004000 : 0x1000000, 1, 0, 53, 5, mem.at<uint16_t>(rec));
    } else {
        const uint32_t s = G::voice_subtitles + e * 96;
        tile_text_draw(mem.at<int8_t>(s), 40, s + 1);
        of<int16_t>(task, 0x66) = 0;
    }
    of<uint32_t>(task, 0) = F::voice_message_frame;
    of<uint32_t>(task, 0x50) = mem.at<uint16_t>(rec + 2);
}

// 0x9B4C0: counts the frames down; the second subtitle line at its frame (+0x2E); at 0 or a skip (evt_skip) the
// subtitle is taken off and the task ends
template <class P> void Game<P>::voice_message_frame(uint32_t task)
{
    const uint32_t e = mem.at<uint8_t>(F::voice_message_task_p + uint32_t(int32_t(of<int16_t>(task, 0x70))) * 3 +
                                       of<uint32_t>(task, 0x54));
    if (text_region() == 0) {
        const int32_t t = of<int32_t>(task, 0x50) - 1;
        of<int32_t>(task, 0x50) = t;
        const uint32_t rec = G::voice_messages + e * 12;
        if (t == 0 || mem.u32(G::evt_skip) != 0) {
            sprite_draw(mem.at<uint8_t>(rec + 6) != 0 ? 0x1004000 : 0x1000000, 53, 5, int32_t(mem.at<uint16_t>(rec)));
            task_exit();
        }
        return;
    }
    const uint32_t s = G::voice_subtitles + e * 96;
    const int32_t t = of<int32_t>(task, 0x50) - 1;
    of<int32_t>(task, 0x50) = t;
    const uint16_t k = mem.at<uint16_t>(s + 0x2E);
    if (k != 0 && t == int32_t(k)) {
        voice_message_frame_sub(mem.at<int8_t>(s), 40, s + 1);
        tile_text_draw(mem.at<int8_t>(s + 0x30), 40, s + 0x31);
        of<int16_t>(task, 0x66) = 1;
    }
    if (of<int32_t>(task, 0x50) == 0 || mem.u32(G::evt_skip) != 0) {
        if (of<uint16_t>(task, 0x66) != 0)
            voice_message_frame_sub(mem.at<int8_t>(s + 0x30), 40, s + 0x31);
        else
            voice_message_frame_sub(mem.at<int8_t>(s), 40, s + 1);
        task_exit();
    }
}

// --- kind 162: a scripted door (ENTBE: +0x35C the record's +0x24.., +0x70 the door number) ---

// 0x52E50: the model and sound of door +0x70 (kind162_doors), kind162_door_frame
template <class P> void Game<P>::kind162_door(uint32_t obj)
{
    of<uint32_t>(obj, 0x368) = of<uint32_t>(obj, 0x35C);
    const int16_t n = of<int16_t>(obj, 0x70);
    of<int16_t>(obj, 0x64) = n;
    of<uint32_t>(obj, 0x54) = mem.u32(G::kind162_doors + uint32_t(int32_t(n)) * 8);
    of<uint32_t>(obj, 0x31C) = mem.u32(G::kind162_doors + uint32_t(int32_t(of<int16_t>(obj, 0x64))) * 8 + 4);
    of<int16_t>(obj, 0x5C) = 0;
    of<int16_t>(obj, 0x60) = 0;
    of<uint32_t>(obj, 0) = F::kind162_door_frame;
}

// 0x52EA0: state 1 (opens, its sound) at the record's camera frame (+8) or event flag (byte +0x10), state 2 (ends)
// at frame +0xC or flag +0x11; the state routine (kind162_states); drawn (scale 1.112 x for door 27, 10 for 66 / 67)
template <class P> void Game<P>::kind162_door_frame(uint32_t obj)
{
    const uint32_t rec = of<uint32_t>(obj, 0x368);
    const float frame = mem.f32(G::camera_frame);
    if (float(mem.at<int32_t>(rec + 8)) == frame || mem.at<uint8_t>(G::event_flags + mem.at<uint8_t>(rec + 0x10)) == 1) {
        if (((of<uint32_t>(obj, 0x58) >> 30) & 1) == 0) {
            of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | (1u << 30);
            sound_cue(of<uint32_t>(obj, 0x31C));
            of<int16_t>(obj, 0x60) = 0;
            of<int16_t>(obj, 0x5C) = 1;
        }
    }
    if (float(mem.at<int32_t>(rec + 0xC)) == mem.f32(G::camera_frame) ||
        mem.at<uint8_t>(G::event_flags + mem.at<uint8_t>(rec + 0x11)) == 1) {
        if (of<int32_t>(obj, 0x58) >= 0) {
            of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | (1u << 31);
            of<int16_t>(obj, 0x60) = 0;
            of<int16_t>(obj, 0x5C) = 2;
        }
    }
    const uint32_t st = mem.u32(G::kind162_states + uint32_t(int32_t(of<int16_t>(obj, 0x5C))) * 4);
    switch (st) {
    case F::kind162_idle: kind162_idle(obj); break;
    case F::obj_kind162_turn: obj_kind162_turn(obj); break;
    case F::kind162_end: kind162_end(obj); return;   // does not return
    default: call_routine(st, obj); break;
    }
    Tgp::Out none;
    copro(0x05, nullptr, 0, none);
    const uint32_t t[3] = {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)};
    copro(0x12, t, 3, none);
    const uint32_t ry[1] = {of<uint32_t>(obj, 0x30)};
    copro(0x15, ry, 1, none);
    const uint32_t rx[1] = {of<uint32_t>(obj, 0x2C)};
    copro(0x14, rx, 1, none);
    const uint32_t rz[1] = {of<uint32_t>(obj, 0x34)};
    copro(0x16, rz, 1, none);
    const int32_t a = of<int16_t>(obj, 0x64);
    uint32_t s[3];
    if (a == 27) {
        s[0] = 0x3F8E5604;
        s[1] = s[2] = fbits(1.0f);
    } else if (a >= 66 && a <= 67) {
        s[0] = s[1] = s[2] = fbits(10.0f);
    } else {
        s[0] = s[1] = s[2] = fbits(1.0f);
    }
    copro(0x13, s, 3, none);
    geo_draw_model(of<uint32_t>(obj, 0x54));
    copro(0x06, nullptr, 0, none);
}

// 0x52E40 (state 0)
template <class P> void Game<P>::kind162_idle(uint32_t obj) {}

// 0x53360 (state 2)
template <class P> void Game<P>::kind162_end(uint32_t obj)
{
    task_exit();
}

// 0x530A0 (state 1): by the door number (the jump table after this routine's start): turns by the record's step
// (+0) until its limit (+4) going down or up, or moves toward the record's point (+0x14) at its speed (+0x20); done
// (flags bit 29): state 0
template <class P> void Game<P>::obj_kind162_turn(uint32_t obj)
{
    const int32_t a = of<int16_t>(obj, 0x64);
    const uint32_t rec = of<uint32_t>(obj, 0x368);
    const uint32_t c = uint32_t(a) > 0x4E ? 0x531F8 : mem.u32(F::obj_kind162_turn + 0x1C + uint32_t(a) * 4);
    bool done = false;
    if (c == 0x531F8) {
        const int32_t v = of<int32_t>(obj, 0x30) + mem.at<int32_t>(rec);
        of<int32_t>(obj, 0x30) = v;
        done = !(v > mem.at<int32_t>(rec + 4));
    } else if (c == 0x53218) {
        const int32_t v = of<int32_t>(obj, 0x30) + mem.at<int32_t>(rec);
        of<int32_t>(obj, 0x30) = v;
        done = !(v < mem.at<int32_t>(rec + 4));
    } else if (c == 0x53238) {
        const float dx = mem.f32(rec + 0x14) - of<float>(obj, 0x20);
        const float dy = mem.f32(rec + 0x18) - of<float>(obj, 0x24);
        const float dz = mem.f32(rec + 0x1C) - of<float>(obj, 0x28);
        const uint32_t d[3] = {fbits(dx), fbits(dy), fbits(dz)};
        Tgp::Out o;
        copro(0x0C, d, 3, o);
        if (pc_path()) {   // PC Mode / Boss Mode: THOTD.EXE 0x49F2C3: times 1 / table square root (x87, 24 bits; pmch34)
            const float r = 1.0f / pm_fast_sqrt((dx * dx + dy * dy) + dz * dz);
            o.w[0] = fbits(dx * r);
            o.w[1] = fbits(dy * r);
            o.w[2] = fbits(dz * r);
        }
        const float s = mem.f32(rec + 0x20);
        const float vx = ffrom(o.w[0]) * s, vy = ffrom(o.w[1]) * s, vz = ffrom(o.w[2]) * s;
        of<float>(obj, 0x7C) = vy;
        of<float>(obj, 0x80) = vz;
        of<float>(obj, 0x78) = vx;
        of<float>(obj, 0x20) = of<float>(obj, 0x20) + vx;
        of<float>(obj, 0x24) = of<float>(obj, 0x24) + vy;
        of<float>(obj, 0x28) = of<float>(obj, 0x28) + vz;
        done = !(std::fabs(dx) > s) && !(std::fabs(dy) > s) && !(std::fabs(dz) > s);
    }
    if (done)
        of<uint32_t>(obj, 0x58) = of<uint32_t>(obj, 0x58) | (1u << 29);
    if ((of<uint32_t>(obj, 0x58) >> 29) & 1) {
        of<int16_t>(obj, 0x5C) = 0;
        of<int16_t>(obj, 0x60) = 0;
    }
}

// --- kind 163 (Chapter 1 event 4): the double door PN_to0a / 0b; +0x2EC the wings' angle, +0x2F0 a count ---

// 0x963F0: closed; event flag 0 starts the 59-frame wait; a skipped scene opens it at once
template <class P> void Game<P>::kind163_door(uint32_t obj)
{
    if (mem.at<uint8_t>(G::event_flags) != 0) {
        of<uint32_t>(obj, 0) = F::kind163_door_wait;
        of<int32_t>(obj, 0x2EC) = 0;
        of<int32_t>(obj, 0x2F0) = 59;
    }
    kind163_door_draw(obj);
    if (mem.u32(G::evt_skip_end) != 0) {
        of<int32_t>(obj, 0x2EC) = 0;
        of<uint32_t>(obj, 0) = F::kind163_door_open_now;
    }
}

// 0x96440
template <class P> void Game<P>::kind163_door_wait(uint32_t obj)
{
    const int32_t n = of<int32_t>(obj, 0x2F0) - 1;
    of<int32_t>(obj, 0x2F0) = n;
    if (n == 0 || mem.u32(G::evt_skip_end) != 0) {
        of<uint32_t>(obj, 0) = F::kind163_door_open;
        sound_cue(0xA01221);
    }
    kind163_door_draw(obj);
}

// 0x96490: swings open 0x71C a frame up to 0x6000
template <class P> void Game<P>::kind163_door_open(uint32_t obj)
{
    const int32_t a = of<int32_t>(obj, 0x2EC) + 0x71C;
    of<int32_t>(obj, 0x2EC) = a;
    if (a > 0x5FFF) {
        of<int32_t>(obj, 0x2F0) = 128;
        of<uint32_t>(obj, 0) = F::kind163_door_open_wait;
    }
    kind163_door_draw(obj);
}

// 0x964D0: 128 frames open, then its sound and the end
template <class P> void Game<P>::kind163_door_open_wait(uint32_t obj)
{
    const int32_t n = of<int32_t>(obj, 0x2F0) - 1;
    of<int32_t>(obj, 0x2F0) = n;
    if (n == 0) {
        sound_cue(0xA01217);
        task_exit();
        return;
    }
    kind163_door_draw(obj);
}

// 0x96510
template <class P> void Game<P>::kind163_door_open_now(uint32_t obj)
{
    of<uint32_t>(obj, 0) = F::kind163_door_open;
    sound_cue(0xA01221);
    kind163_door_draw(obj);
}

// 0x96540: the two wings (0x73A at (15, 0, -78) turned -angle, 0x73B at (-15, 0, -78) turned +angle), scale 10
template <class P> void Game<P>::kind163_door_draw(uint32_t obj)
{
    const int32_t a = of<int32_t>(obj, 0x2EC);
    Tgp::Out none;
    const uint32_t s[3] = {fbits(10.0f), fbits(10.0f), fbits(10.0f)};
    copro(0x05, nullptr, 0, none);
    const uint32_t t0[3] = {fbits(15.0f), 0, fbits(-78.0f)};
    copro(0x12, t0, 3, none);
    const uint32_t r0[1] = {uint32_t(0 - a)};
    copro(0x15, r0, 1, none);
    copro(0x13, s, 3, none);
    geo_draw_model(0x73A);
    copro(0x06, nullptr, 0, none);
    copro(0x05, nullptr, 0, none);
    const uint32_t t1[3] = {fbits(-15.0f), 0, fbits(-78.0f)};
    copro(0x12, t1, 3, none);
    const uint32_t r1[1] = {uint32_t(a)};
    copro(0x15, r1, 1, none);
    copro(0x13, s, 3, none);
    geo_draw_model(0x73B);
    copro(0x06, nullptr, 0, none);
}

// --- kind 164 (Chapter 1 events 4 / 5, ENTB2 value 27): the double door PN_todoor02a / 02b bursting open ---

// 0x96660: the two wings as tasks (0x73E at x 9.7 / 0x742 at x -9.7, z 71.5, turned half round, 110 frames), ends
template <class P> void Game<P>::kind164_door_burst(uint32_t obj)
{
    uint32_t t = task_create(F::kind164_wing, 0x3C8);
    task_clear_work(t);
    of<uint8_t>(t, 0x2E6) = 0;
    of<uint16_t>(t, 0x70) = of<uint16_t>(obj, 0x70);
    of<uint32_t>(t, 0x54) = 0x73E;
    of<uint32_t>(t, 0x20) = 0x411B3333;
    of<uint32_t>(t, 0x28) = 0x428F0000;
    of<int32_t>(t, 0x30) = 0x8000;
    of<uint32_t>(t, 0x24) = 0;
    of<int32_t>(t, 0x2F0) = 110;
    of<int32_t>(t, 0x2EC) = 0;
    kind164_door_model_draw(t);
    t = task_create(F::kind164_wing, 0x3C8);
    task_clear_work(t);
    of<uint8_t>(t, 0x2E6) = 1;
    of<uint32_t>(t, 0x24) = 0;
    of<uint32_t>(t, 0x54) = 0x742;
    of<uint32_t>(t, 0x20) = 0xC11B3333;
    of<uint32_t>(t, 0x28) = 0x428F0000;
    of<int32_t>(t, 0x30) = 0x8000;
    of<int32_t>(t, 0x2F0) = 110;
    of<int32_t>(t, 0x2EC) = 0;
    of<uint16_t>(t, 0x70) = of<uint16_t>(obj, 0x70);
    kind164_door_model_draw(t);
    task_exit();
}

// 0x96760: waits for event flag 1 (-> kind164_wing_wait); gone when the draw zone is its +0x70
template <class P> void Game<P>::kind164_wing(uint32_t obj)
{
    if (mem.at<uint8_t>(G::event_flags + 1) != 0) {
        of<uint32_t>(obj, 0) = F::kind164_wing_wait;
    } else if (of<uint16_t>(obj, 0x70) == (uint32_t(int32_t(mem.at<int8_t>(G::draw_zone))) & 0xFFFF)) {
        task_exit();
        return;
    }
    kind164_door_model_draw(obj);
}

// 0x967B0: at 94 frames left three debris pieces fly (kind164_debris, at angles 0 / 0x5555 / 0xAAAA on a circle of 2
// around (0, 10, 70.5)); at 0 the wing bursts: turned at random, 0.6 a frame along it, a random roll step,
// kind164_wing_fly (sound 0xA01221)
template <class P> void Game<P>::kind164_wing_wait(uint32_t obj)
{
    if (of<int32_t>(obj, 0x2F0) == 0x5E) {
        for (uint32_t a = 0; a <= 0xAAAA; a += 0x5555) {
            const uint32_t t = task_create(F::kind164_debris, 0x3C8);
            task_clear_work(t);
            Tgp::Out s, c;
            const uint32_t in[1] = {a};
            copro(0x1C, in, 1, s);
            of<float>(t, 0x20) = (ffrom(s.w[0]) + ffrom(s.w[0])) + 0.0f;
            copro(0x1B, in, 1, c);
            of<uint32_t>(t, 0x28) = 0x428D0000;
            of<float>(t, 0x24) = (ffrom(c.w[0]) + ffrom(c.w[0])) + 10.0f;
            of<uint32_t>(t, 0x2C) = rand() & 0xFFFF;
            of<uint32_t>(t, 0x30) = rand() & 0xFFFF;
            of<uint32_t>(t, 0x34) = rand() & 0xFFFF;
        }
    }
    const int32_t n = of<int32_t>(obj, 0x2F0) - 1;
    of<int32_t>(obj, 0x2F0) = n;
    if (n == 0) {
        Tgp::Out none, o;
        copro(0x21, nullptr, 0, none);
        uint32_t r[1];
        if (of<uint8_t>(obj, 0x2E6) == 0)
            r[0] = (rand() & 0x3FF) + 0x9800;
        else
            r[0] = 0x4400 - (rand() & 0x3FF);
        copro(0x15, r, 1, none);
        const uint32_t v[3] = {0, 0, 0x3F19999A};
        copro(0x1A, v, 3, o);
        of<uint32_t>(obj, 0x78) = o.w[0];
        of<uint32_t>(obj, 0x80) = o.w[2];
        copro(0x06, nullptr, 0, none);
        const uint32_t k = rand();
        of<uint32_t>(obj, 0) = F::kind164_wing_fly;
        of<int32_t>(obj, 0x50) = 0;
        of<uint32_t>(obj, 0x4C) = (k & 0x3F) + 0xA0;
        sound_cue(0xA01221);
    }
    kind164_door_model_draw(obj);
}

// 0x969D0: 120 frames: pitches down 0x100 a frame, yaws 0x16C more, rolls by +0x4C, moves along its velocity
template <class P> void Game<P>::kind164_wing_fly(uint32_t obj)
{
    const int32_t t = of<int32_t>(obj, 0x50) + 1;
    of<int32_t>(obj, 0x50) = t;
    if (t == 0x79) {
        task_exit();
        return;
    }
    const int32_t y = of<int32_t>(obj, 0x2EC) + 0x16C;
    of<int32_t>(obj, 0x2C) = of<int32_t>(obj, 0x2C) - 0x100;
    if (of<uint8_t>(obj, 0x2E6) == 0) {
        of<int32_t>(obj, 0x30) = y + 0x8000;
        of<int32_t>(obj, 0x34) = of<int32_t>(obj, 0x34) + of<int32_t>(obj, 0x4C);
    } else {
        of<int32_t>(obj, 0x30) = 0x8000 - y;
        of<int32_t>(obj, 0x34) = of<int32_t>(obj, 0x34) - of<int32_t>(obj, 0x4C);
    }
    of<float>(obj, 0x20) = of<float>(obj, 0x20) + of<float>(obj, 0x78);
    of<float>(obj, 0x28) = of<float>(obj, 0x28) + of<float>(obj, 0x80);
    of<int32_t>(obj, 0x2EC) = y;
    kind164_door_model_draw(obj);
}

// 0x96A80: model +0x54 (+3 while fewer than 94 frames are left: the burst model) at its place, scale 10
template <class P> void Game<P>::kind164_door_model_draw(uint32_t obj)
{
    Tgp::Out none;
    copro(0x05, nullptr, 0, none);
    const uint32_t t[3] = {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)};
    copro(0x12, t, 3, none);
    const uint32_t ry[1] = {of<uint32_t>(obj, 0x30)};
    copro(0x15, ry, 1, none);
    const uint32_t rx[1] = {of<uint32_t>(obj, 0x2C)};
    copro(0x14, rx, 1, none);
    const uint32_t rz[1] = {of<uint32_t>(obj, 0x34)};
    copro(0x16, rz, 1, none);
    const uint32_t s[3] = {fbits(10.0f), fbits(10.0f), fbits(10.0f)};
    copro(0x13, s, 3, none);
    geo_draw_model(of<uint32_t>(obj, 0x54) + (of<int32_t>(obj, 0x2F0) < 0x5E ? 3 : 0));
    copro(0x06, nullptr, 0, none);
}

// 0x96B40: a debris piece (model 41, scale 0.1 / 0.7 / 0.7) flying toward the camera for 60 frames, spinning
template <class P> void Game<P>::kind164_debris(uint32_t obj)
{
    const int32_t t = of<int32_t>(obj, 0x50) + 1;
    of<int32_t>(obj, 0x50) = t;
    if (t > 59) {
        task_exit();
        return;
    }
    const float y = of<float>(obj, 0x24) - ffrom(0x3DA740DA);
    const float z = of<float>(obj, 0x28) - ffrom(0x3F19999A);
    Tgp::Out none;
    copro(0x05, nullptr, 0, none);
    of<float>(obj, 0x24) = y;
    of<float>(obj, 0x28) = z;
    const uint32_t p[3] = {of<uint32_t>(obj, 0x20), fbits(y), fbits(z)};
    copro(0x12, p, 3, none);
    of<int32_t>(obj, 0x34) = of<int32_t>(obj, 0x34) + 0x200;
    const uint32_t rz[1] = {of<uint32_t>(obj, 0x34)};
    copro(0x16, rz, 1, none);
    of<int32_t>(obj, 0x30) = of<int32_t>(obj, 0x30) + 0x200;
    const uint32_t ry[1] = {of<uint32_t>(obj, 0x30)};
    copro(0x15, ry, 1, none);
    of<int32_t>(obj, 0x2C) = of<int32_t>(obj, 0x2C) + 0x200;
    const uint32_t rx[1] = {of<uint32_t>(obj, 0x2C)};
    copro(0x14, rx, 1, none);
    const uint32_t s[3] = {0x3DCCCCCD, 0x3F333333, 0x3F333333};
    copro(0x13, s, 3, none);
    geo_draw_model(41);
    copro(0x06, nullptr, 0, none);
}

// --- kinds 166 / 167: single doors ---

// 0x96C80 (kind 166, door_n6a1a 0x168): placed; event flag 0x21 swings it open (sound 0xA01221); slides away (x - 0.8,
// turning) until x -14.5
template <class P> void Game<P>::kind166_door(uint32_t obj)
{
    int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        of<uint32_t>(obj, 0x20) = 0x4177BA5E;
        of<int32_t>(obj, 0x2C) = 0;
        of<int32_t>(obj, 0x30) = 0;
        of<int32_t>(obj, 0x34) = 0;
        of<uint32_t>(obj, 0x24) = 0x41F00831;
        of<uint32_t>(obj, 0x28) = 0x434B4000;
        of<int16_t>(obj, 0x60) = sub = 1;
    }
    if (sub == 1) {
        if (mem.at<uint8_t>(G::event_flags + 0x21) == 1) {
            sound_cue(0xA01221);
            of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
        }
    } else if (sub == 2) {
        if (!(of<float>(obj, 0x20) > ffrom(0xC16845A2))) {
            task_exit();
            return;
        }
        const float x = of<float>(obj, 0x20) - ffrom(0x3F4CCCCD);
        of<int32_t>(obj, 0x30) = of<int32_t>(obj, 0x30) + 0x200;
        of<int32_t>(obj, 0x2C) = of<int32_t>(obj, 0x2C) + 5;
        of<float>(obj, 0x20) = x;
    }
    Tgp::Out none;
    copro(0x05, nullptr, 0, none);
    const uint32_t t[3] = {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)};
    copro(0x12, t, 3, none);
    const uint32_t rx[1] = {of<uint32_t>(obj, 0x2C)};
    copro(0x14, rx, 1, none);
    const uint32_t ry[1] = {of<uint32_t>(obj, 0x30)};
    copro(0x15, ry, 1, none);
    geo_draw_model(0x168);
    copro(0x06, nullptr, 0, none);
}

// 0x96DC0 (kind 167, door_n7a 0x169): placed; event flag 0x22 swings it open (sound 0xA01221) 0x400 a frame to -0x6000
template <class P> void Game<P>::kind167_door(uint32_t obj)
{
    int16_t sub = of<int16_t>(obj, 0x60);
    if (sub == 0) {
        of<int32_t>(obj, 0x2C) = 0;
        of<int32_t>(obj, 0x30) = 0;
        of<uint32_t>(obj, 0x20) = 0x4306D53F;
        of<uint32_t>(obj, 0x24) = 0x41F00E56;
        of<int32_t>(obj, 0x34) = 0;
        of<uint32_t>(obj, 0x28) = 0x43AFE74C;
        of<int16_t>(obj, 0x60) = sub = 1;
    }
    if (sub == 1) {
        if (mem.at<uint8_t>(G::event_flags + 0x22) == 1) {
            sound_cue(0xA01221);
            of<int16_t>(obj, 0x60) = int16_t(of<int16_t>(obj, 0x60) + 1);
        }
    } else if (sub == 2) {
        if (!(of<int32_t>(obj, 0x30) > -0x6000)) {
            task_exit();
            return;
        }
        of<int32_t>(obj, 0x30) = of<int32_t>(obj, 0x30) - 0x400;
    }
    Tgp::Out none;
    copro(0x05, nullptr, 0, none);
    const uint32_t t[3] = {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)};
    copro(0x12, t, 3, none);
    const uint32_t ry[1] = {of<uint32_t>(obj, 0x30)};
    copro(0x15, ry, 1, none);
    geo_draw_model(0x169);
    copro(0x06, nullptr, 0, none);
}

// --- kind 173 (Chapter 1 events 0 / 1): 20 drops (model 0x7C4) falling from 22 up around the camera ---

// 0x35E0: a chain of 20 work blocks (+0x35C next, +0x31C its number 1..20, +0x50 a random delay 0..70)
template <class P> void Game<P>::kind173_list(uint32_t obj)
{
    of<uint32_t>(obj, 0x31C) = 0;
    of<uint32_t>(obj, 0) = F::kind173_list_frame;
    uint32_t prev = obj;
    uint32_t n = 0;
    do {
        const uint32_t b = mem_alloc(0x3C8);
        ++n;
        task_clear_work(b);
        of<uint32_t>(prev, 0x35C) = b;
        of<uint32_t>(b, 0x31C) = n;
        const uint32_t r = rand();
        of<int16_t>(b, 0x60) = 0;
        of<uint32_t>(b, 0x35C) = 0;
        of<uint32_t>(b, 0x50) = (r & 7) * 10;
        prev = b;
    } while (n <= 19);
}

// 0x3650: event flag 5 (0x51E945; T1: was 0x85, so the rain never stopped indoors) frees the chain and ends; else per drop: 0 its delay; 1 placed at random in front of the
// camera (stored matrix 0 / 1), scale 1 / 3 / 5 / 7; 2 falls 2 a frame; on the floor a splash (unless hidden), again;
// drops 11..20 are hidden while event flag 0xB0 is set without 0xB4
template <class P> void Game<P>::kind173_list_frame(uint32_t obj)
{
    if (mem.at<uint8_t>(G::event_flags + 5) != 0) {
        uint32_t p = of<uint32_t>(obj, 0x35C);
        while (p != 0) {
            const uint32_t next = of<uint32_t>(p, 0x35C);
            mem_free(p);
            p = next;
        }
        task_exit();
        return;
    }
    for (uint32_t b = of<uint32_t>(obj, 0x35C); b != 0; b = of<uint32_t>(b, 0x35C)) {
        bool hidden = false;
        if (mem.at<uint8_t>(G::event_flags + 0xB0) == 1 && mem.at<uint8_t>(G::event_flags + 0xB4) == 0)
            hidden = of<int32_t>(b, 0x31C) >= 11;
        int16_t sub = of<int16_t>(b, 0x60);
        if (sub == 0) {
            const int32_t t = of<int32_t>(b, 0x50) - 1;
            of<int32_t>(b, 0x50) = t;
            if (t <= 0) {
                of<int16_t>(b, 0x60) = sub = 1;
            }
        }
        if (sub == 1) {
            Tgp::Out none, o, w;
            const uint32_t m0[1] = {0};
            copro(0x74, m0, 1, none);
            const uint32_t p[3] = {mem.u32(G::camera_pos), fbits(22.0f), mem.u32(G::camera_pos + 8)};
            copro(0x1A, p, 3, o);
            of<uint32_t>(b, 0x38) = o.w[0];
            of<uint32_t>(b, 0x3C) = o.w[1];
            copro(0x06, nullptr, 0, none);
            of<uint32_t>(b, 0x40) = o.w[2];
            const uint32_t r1 = rand();
            of<float>(b, 0x38) = (35.0f - float(int32_t(r1 & 7)) * 10.0f) + of<float>(b, 0x38);
            const uint32_t r2 = rand();
            of<float>(b, 0x40) = float(int32_t((r2 & 7) + 1)) * 10.0f + of<float>(b, 0x40);
            const uint32_t m1[1] = {1};
            copro(0x74, m1, 1, none);
            const uint32_t q[3] = {of<uint32_t>(b, 0x38), of<uint32_t>(b, 0x3C), of<uint32_t>(b, 0x40)};
            copro(0x1A, q, 3, w);
            of<uint32_t>(b, 0x20) = w.w[0];
            of<uint32_t>(b, 0x24) = w.w[1];
            copro(0x06, nullptr, 0, none);
            of<uint32_t>(b, 0x28) = w.w[2];
            if (pc_path()) {
                // PC Mode / Boss Mode: THOTD.EXE 0x431678 does the same on its own stored matrices 0 / 1 (the camera's
                // inverse and the camera) with the barrel file's transform copy 0x42F520 (pmbody)
                float v0[3], v1[3];
                pm_m0().transform(mem.f32(G::camera_pos), 22.0f, mem.f32(G::camera_pos + 8), v0[0], v0[1], v0[2]);
                v0[0] = (35.0f - float(int32_t(r1 & 7)) * 10.0f) + v0[0];
                v0[2] = float(int32_t((r2 & 7) + 1)) * 10.0f + v0[2];
                pm_m1().transform(v0[0], v0[1], v0[2], v1[0], v1[1], v1[2]);
                for (uint32_t k = 0; k < 3; ++k) {
                    of<float>(b, 0x38 + 4 * k) = v0[k];
                    of<float>(b, 0x20 + 4 * k) = v1[k];
                }
            }
            const uint32_t r3 = rand();
            const float v = float(int32_t(r3 & 3));
            of<int16_t>(b, 0x60) = sub = 2;
            of<float>(b, 0x94) = (v + v) + 1.0f;
        }
        if (sub == 2) {
            const float y = of<float>(b, 0x24) - 2.0f;
            of<float>(b, 0x24) = y;
            const float floor = mem.f32(G::floor_y);
            if (!(y > floor)) {
                if (!hidden)
                    splash_9f1_spawn(of<uint32_t>(b, 0x20), fbits(floor), of<uint32_t>(b, 0x28));
                of<int16_t>(b, 0x60) = 1;
            }
        }
        if (hidden)
            continue;
        Tgp::Out none;
        copro(0x05, nullptr, 0, none);
        const uint32_t t[3] = {of<uint32_t>(b, 0x20), of<uint32_t>(b, 0x24), of<uint32_t>(b, 0x28)};
        copro(0x12, t, 3, none);
        const uint32_t rx[1] = {0x600};
        copro(0x14, rx, 1, none);
        const uint32_t s[3] = {fbits(3.0f), of<uint32_t>(b, 0x94), fbits(3.0f)};
        copro(0x13, s, 3, none);
        geo_draw_model(0x7C4);
        copro(0x06, nullptr, 0, none);
    }
}

// --- kind 180 (Chapter 1 event 4): the room object PN_room1c that kind 70 rides on (ride_pos / ride_prev) ---

// 0x2F50: at (0, 30, 5), 120 frames, kind180_room_object_frame
template <class P> void Game<P>::kind180_room_object(uint32_t obj)
{
    mem.set32(G::ride_pos_x, 0);
    mem.set32(G::ride_pos_y, fbits(30.0f));
    mem.set32(G::ride_pos_z, fbits(5.0f));
    of<uint32_t>(obj, 0x24) = fbits(30.0f);
    of<uint32_t>(obj, 0x28) = fbits(5.0f);
    of<int32_t>(obj, 0x50) = 120;
    of<uint32_t>(obj, 0x20) = 0;
    of<int16_t>(obj, 0x5C) = 0;
    of<uint32_t>(obj, 0) = F::kind180_room_object_frame;
    obj_kind180_draw(obj);
}

// coprocessor 0x1B (the sine) of in[0]; PC Mode / Boss Mode: the PC's table sine (THOTD.EXE 0x4310B0 indexes
// pc_trig_table by the angle / 16; pmbody)
template <class P> void Game<P>::pm_sin_1b(uint32_t angle, Tgp::Out& o)
{
    if (pc_path()) {
        o = Tgp::Out();
        o.putf(pc_trig(angle, 0));
        return;
    }
    const uint32_t in[1] = {angle};
    copro(0x1B, in, 1, o);
}

// 0x2FC0: 0 sways (x 1.2 cos, z 0.05 cos) until event flag 3 (0x51E943; T1: was 0x83) (sounds 0xA0121F / 0xA01323); 1 shakes harder for 85
// frames, then falls (gravity -0.0272, event flag 4); 2 tumbles to the floor (sounds 0xA0122B / 0xA01231); 3
// blinks out over 120 frames; ride_pos follows it (ride_prev = last frame's)
template <class P> void Game<P>::kind180_room_object_frame(uint32_t obj)
{
    mem.set32(G::ride_prev_x, mem.u32(G::ride_pos_x));
    mem.set32(G::ride_prev_y, mem.u32(G::ride_pos_y));
    mem.set32(G::ride_prev_z, mem.u32(G::ride_pos_z));
    int16_t st = of<int16_t>(obj, 0x5C);
    Tgp::Out o;
    if (st == 0) {
        of<uint32_t>(obj, 0x54) = 0xCE7;
        const int32_t a = of<int32_t>(obj, 0x44) + 0x200;
        of<int32_t>(obj, 0x44) = a;
        const uint32_t in[1] = {uint32_t(a)};
        pm_sin_1b(in[0], o);
        const float x = ffrom(o.w[0]) * ffrom(0x3F99999A);
        const int32_t b = of<int32_t>(obj, 0x4C) + 0x200;
        of<int32_t>(obj, 0x4C) = b;
        of<float>(obj, 0x20) = x;
        const uint32_t in2[1] = {uint32_t(b)};
        pm_sin_1b(in2[0], o);
        of<float>(obj, 0x28) = ffrom(o.w[0]) * ffrom(0x3D4CCCCD);
        if (mem.at<uint8_t>(G::event_flags + 3) != 0) {
            of<int32_t>(obj, 0x44) = 0;
            of<int32_t>(obj, 0x50) = 0x55;
            of<int32_t>(obj, 0x4C) = 0;
            of<int32_t>(obj, 0x320) = 0;
            of<int32_t>(obj, 0x324) = 16;
            sound_cue(0xA0121F);
            sound_cue(0xA01323);
            of<int16_t>(obj, 0x5C) = st = 1;
        }
    }
    if (st == 1) {
        const int32_t a = of<int32_t>(obj, 0x44) - 0x300;
        of<int32_t>(obj, 0x44) = a;
        const uint32_t in[1] = {uint32_t(a)};
        pm_sin_1b(in[0], o);
        const float x = ffrom(o.w[0]) * ffrom(0x3ECCCCCD);
        const int32_t k = of<int32_t>(obj, 0x324);
        of<int32_t>(obj, 0x324) = k + 1;
        const int32_t b = (of<int32_t>(obj, 0x4C) - 0x300) - k;
        of<int32_t>(obj, 0x4C) = b;
        of<float>(obj, 0x20) = x;
        const uint32_t in2[1] = {uint32_t(b)};
        pm_sin_1b(in2[0], o);
        const float z = ffrom(o.w[0]) * 15.0f;
        const int32_t n = of<int32_t>(obj, 0x50) - 1;
        of<int32_t>(obj, 0x50) = n;
        of<float>(obj, 0x28) = z;
        if (n <= 0) {
            of<uint32_t>(obj, 0x88) = 0xBCDF0123;
            mem.at<uint8_t>(G::event_flags + 4) = 1;
            of<int16_t>(obj, 0x5C) = st = 2;
        }
    }
    if (st == 2) {
        of<int32_t>(obj, 0x2C) = of<int32_t>(obj, 0x2C) - 0x100;
        if (of<float>(obj, 0x24) > mem.f32(G::floor_y)) {
            const float vy = of<float>(obj, 0x7C) + of<float>(obj, 0x88);
            of<float>(obj, 0x28) = of<float>(obj, 0x28) - 0.75f;
            const float y = of<float>(obj, 0x24) + vy;
            of<float>(obj, 0x7C) = vy;
            of<float>(obj, 0x24) = y;
            const float floor = mem.f32(G::floor_y);
            if (!(y > floor)) {
                of<int32_t>(obj, 0x4C) = 0x100;
                of<int32_t>(obj, 0x50) = 120;
                of<float>(obj, 0x24) = floor;
                sound_cue(0xA0122B);
                sound_cue(0xA01231);
                of<int16_t>(obj, 0x5C) = int16_t(of<int16_t>(obj, 0x5C) + 1);
            }
        }
    }
    mem.set32(G::ride_pos_x, of<uint32_t>(obj, 0x20));
    mem.set32(G::ride_pos_y, of<uint32_t>(obj, 0x24));
    mem.set32(G::ride_pos_z, of<uint32_t>(obj, 0x28));
    if (of<uint16_t>(obj, 0x5C) == 3) {
        const int32_t n = of<int32_t>(obj, 0x50) - 1;
        of<int32_t>(obj, 0x50) = n;
        if (n % 2 != 0)
            obj_kind180_draw(obj);
        if (of<int32_t>(obj, 0x50) < 0)
            task_exit();
        return;
    }
    obj_kind180_draw(obj);
}

// 0x32B0: PN_room1c (0xCE7) and its own model (+0x54) at its place, scale 1.3 (the second with light 3)
template <class P> void Game<P>::obj_kind180_draw(uint32_t obj)
{
    Tgp::Out none;
    copro(0x05, nullptr, 0, none);
    const uint32_t t[3] = {of<uint32_t>(obj, 0x20), of<uint32_t>(obj, 0x24), of<uint32_t>(obj, 0x28)};
    copro(0x12, t, 3, none);
    const uint32_t rz[1] = {of<uint32_t>(obj, 0x34)};
    copro(0x16, rz, 1, none);
    const uint32_t ry[1] = {of<uint32_t>(obj, 0x30)};
    copro(0x15, ry, 1, none);
    const uint32_t rx[1] = {of<uint32_t>(obj, 0x2C)};
    copro(0x14, rx, 1, none);
    const uint32_t s[3] = {0x3FA66666, 0x3FA66666, 0x3FA66666};
    copro(0x13, s, 3, none);
    geo_draw_model(0xCE7);
    draw_fade_begin(0);
    const uint32_t l[1] = {3};
    copro(0x77, l, 1, none);
    geo_draw_model(of<uint32_t>(obj, 0x54));
    geo_send_words(0, 31, 0x7A0);
    copro(0x06, nullptr, 0, none);
}

// --- kind 172: the opening title (Chapter 1 event 0) ---

// 0x9AFB0: the title sprite 0x81 (Japan) on a grey palette; 180 frames from 120 on a skip (60 / 30 elsewhere)
template <class P> void Game<P>::kind172_opening_title(uint32_t obj)
{
    if (text_region() == 0) {
        sprite_bank_load(47, 0x81);
        sprite_draw_anim(0x1000000, 1, 0, 0, 0, 0x81);
    }
    mem.set32(G::evt_clrgfst_flags, mem.u32(G::evt_clrgfst_flags) | 0x10);
    palette_grey_build();
    if (mem.at<uint8_t>(G::region) == 0) {
        of<int32_t>(obj, 0x50) = 180;
        of<int32_t>(obj, 0x54) = 120;
    } else {
        of<int32_t>(obj, 0x50) = 60;
        of<int32_t>(obj, 0x54) = 30;
    }
    of<uint32_t>(obj, 0) = F::kind172_title_wait;
}

// 0x9B040: counts down (a skip ends it below +0x54); then the title off, event flag 0xF9 (title done), the colour
// palette back at the next vblank
template <class P> void Game<P>::kind172_title_wait(uint32_t obj)
{
    const int32_t t = of<int32_t>(obj, 0x50) - 1;
    of<int32_t>(obj, 0x50) = t;
    const bool skip = t < of<int32_t>(obj, 0x54) && mem.at<uint8_t>(G::skip_request) != 0;
    if (!skip && of<int32_t>(obj, 0x50) > 0)
        return;
    if (text_region() == 0)
        sprite_draw(0x1000000, 0, 0, 0x81);
    if (text_region() == 0)
        sprite_bank_free(47);
    mem.at<uint8_t>(G::event_flags + 0xF9) = 1;
    mem.set32(G::evt_clrgfst_flags, mem.u32(G::evt_clrgfst_flags) & ~0x10u);
    vblank_callback_add(F::palette_restore_vblank);
    task_exit();
}

// --- kind 177: the chapter title card ---

// 0x9B0E0: the card sprite of the chapter and region (chapter_card_sprites), the gun flash layer, the flickering
// bank 44 sprite 0x7D; 180 frames
template <class P> void Game<P>::kind177_chapter_card(uint32_t obj)
{
    const uint32_t row = G::chapter_card_sprites + uint32_t(int32_t(mem.at<int8_t>(G::chapter)) * 5) * 4;
    const uint32_t sprite = mem.u32(row + text_region() * 4);
    mem.set32(G::evt_clrgfst_flags, mem.u32(G::evt_clrgfst_flags) | 0x20);
    sprite_bank_load(mem.at<int16_t>(G::sprite_bank_of + sprite * 2), int32_t(sprite));
    sprite_draw_anim(0x1004000, 1, 0, 0, 0,
                     mem.u32(G::chapter_card_sprites + uint32_t(int32_t(mem.at<int8_t>(G::chapter)) * 5) * 4 +
                             text_region() * 4));
    gun_flash_on(0x100C000);
    sprite_bank_load(44, 0x7D);
    sprite_draw_anim(0x1002000, 1, 0, 28, 1, 0x7D);
    mem.at<uint8_t>(G::skip_request) = 0;
    of<int32_t>(obj, 0x50) = 180;
    of<uint32_t>(obj, 0) = F::kind177_chapter_card_frame;
}

// 0x9B1C0: the flicker sprite (0x7D + (frame % 6) / 2) in the lower half on even frames, the upper half on odd ones;
// a wait for the raster (hardware: nothing here); after 60 frames a skip ends it; at the end the sprites off,
// event flag 0xF8 (card done)
template <class P> void Game<P>::kind177_chapter_card_frame(uint32_t obj)
{
    const int32_t f = mem.at<int32_t>(G::frame_counter);
    if (f % 2 == 0)
        sprite_draw_clipped(0x1002000, 1, 0, 0, 0, 0, 61, 24, 47, uint32_t((f % 6) / 2 + 0x7D));
    // the original waits here until the raster register (0xF00000 + 4) is at most 0x927C0
    const int32_t g = f + 1;
    if (g % 2 == 0)
        sprite_draw_clipped(0x1002000, 1, 0, 0, 0, 0, 61, 0, 23, uint32_t((g % 6) / 2 + 0x7D));
    const int32_t t = of<int32_t>(obj, 0x50) - 1;
    of<int32_t>(obj, 0x50) = t;
    const bool skip = !(t > 0x77) && mem.at<uint8_t>(G::skip_request) != 0;
    if (!skip && of<int32_t>(obj, 0x50) > 0)
        return;
    const uint32_t row = G::chapter_card_sprites + uint32_t(int32_t(mem.at<int8_t>(G::chapter)) * 5) * 4;
    sprite_draw(0x1004000, 0, 0, int32_t(mem.u32(row + text_region() * 4)));
    gun_flash_off(0x100C000);
    sprite_draw_anim(0x1002000, 1, 0, 0, 0, 0xA2);
    const uint32_t row2 = G::chapter_card_sprites + uint32_t(int32_t(mem.at<int8_t>(G::chapter)) * 5) * 4;
    sprite_bank_free(mem.at<int16_t>(G::sprite_bank_of + mem.u32(row2 + text_region() * 4) * 2));
    sprite_bank_free(44);
    mem.at<uint8_t>(G::event_flags + 0xF8) = 1;
    mem.set32(G::evt_clrgfst_flags, mem.u32(G::evt_clrgfst_flags) & ~0x20u);
    task_exit();
}

// --- kind 178: the scene skip ---

// 0x9C230
template <class P> void Game<P>::kind178_scene_skip(uint32_t obj)
{
    mem.at<uint8_t>(G::skip_request) = 0;
    of<uint32_t>(obj, 0) = F::kind178_scene_skip_frame;
    kind178_scene_skip_frame(obj);
}

// 0x9C250: while the scene is skippable (evt_skip_on) a skip request sets evt_skip and evt_skip_end (the camera's
// skip frame taken over), kind178_scene_skip_end; not skippable any more: ends
template <class P> void Game<P>::kind178_scene_skip_frame(uint32_t obj)
{
    if (mem.u32(G::evt_skip_on) == 0) {
        task_exit();
        return;
    }
    if (mem.at<uint8_t>(G::skip_request) == 0)
        return;
    mem.set32(G::evt_skip, 1);
    mem.set32(G::camera_skip_frame_set, mem.u32(G::camera_skip_frame));
    mem.at<uint8_t>(G::skip_request) = 0;
    mem.set32(G::evt_skip_end, 1);
    of<uint32_t>(obj, 0) = F::kind178_scene_skip_end;
}

// 0x9C2C0
template <class P> void Game<P>::kind178_scene_skip_end(uint32_t obj)
{
    mem.set32(G::evt_skip_end, 0);
    task_exit();
}

} // namespace hotd::port
