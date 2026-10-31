// The chapter's resets and two of its start-table tasks (proofs-7): shot_lists_clear 0x453C0, chapter_reset_53e1b0
// 0x32290, chapter_counters_reset 0x67480 (chapter_load's resets), attack_slots_task 0x31C00 and depth_lists_task
// 0x53480 (every frame); first written in the runtime (src/game/chapter.cpp, stand_ins.cpp), moved here for their
// capture proof. Arcade forms only (the PC's are replayed). Declarations: decl/support_b.h.
#include "port/game.h"

namespace hotd::port {

// 0x453C0 shot_lists_clear: 256 entries of the shot sort, the shot candidates and the depth list; both counts 0
template <class P> void Game<P>::shot_lists_clear()
{
    if constexpr (!P::pc) {
        for (uint32_t i = 0; i < 256; ++i) {
            mem.set32(G::shot_sort + 8 * i, 0xFFFFFFFFu);
            for (uint32_t k = 0; k < 8; ++k)
                mem.set32(G::shot_candidates + 0x24 * i + 4 * k, 0);
            mem.set32(G::shot_sort + 8 * i + 4, 0);
            mem.set32(G::depth_list + 12 * i + 4, 0);
            mem.set32(G::depth_list + 12 * i + 8, 0);
            mem.set32(G::depth_list + 12 * i, 0xFFFFFFFFu);
        }
        mem.set32(G::depth_list_count, 0);
        mem.set32(G::shot_candidate_count, 0);
    }
}

// 0x32290 chapter_reset_53e1b0: 0x53E1C0 .. 0x53E227, 0x53E230 and 0x53E1B0 cleared
template <class P> void Game<P>::chapter_reset_53e1b0()
{
    if constexpr (!P::pc) {
        for (uint32_t a = 0x53E1C0; a < 0x53E228; a += 4)
            mem.set32(a, 0);
        mem.set32(0x53E230, 0);
        mem.set32(0x53E1B0, 0);
    }
}

// 0x67480 chapter_counters_reset: the chapter's counters, the event flags (0x40 words), scene_state, 0x51F440 ..
// 0x51F474, 0x520790; the play-time flag 0x51F9D8 = 1
template <class P> void Game<P>::chapter_counters_reset()
{
    if constexpr (!P::pc) {
        for (uint32_t a : {0x520794u, 0x520792u, 0x51FB30u, 0x520796u})
            mem.template at<int16_t>(a) = 0;
        for (uint32_t a : {0x51F794u, 0x51F9D4u, 0x51F9DCu, 0x51F60Cu, 0x51F63Cu, 0x51F640u})
            mem.set32(a, 0);
        mem.set32(0x51F9D8, 1);
        for (uint32_t i = 0; i < 0x40; ++i)
            mem.set32(G::event_flags + 4 * i, 0);
        mem.template at<uint8_t>(G::scene_state) = 0;
        for (uint32_t a = 0x51F474; a >= 0x51F440; a -= 4)
            mem.set32(a, 0);
        mem.template at<uint8_t>(0x520790) = 0;
    }
}

// 0x31C00 attack_slots_task: attack positions 0 and 1 = the camera position plus an offset (table 0x31A20 by
// attack_side and the player count) turned by the camera's yaw (0x51F260)
template <class P> void Game<P>::attack_slots_task(uint32_t task)
{
    (void)task;
    if constexpr (!P::pc) {
        Tgp::Out none;
        copro(0x05, nullptr, 0, none);
        const int32_t side = mem.template at<int8_t>(G::attack_side);
        const int32_t players = mem.template at<int16_t>(G::players_in_game_b);
        const uint32_t table = 0x31A20 + uint32_t(side) * 0xC0 + uint32_t(players) * 0x60;
        const float cx = mem.f32(G::camera_pos), cz = mem.f32(G::camera_pos + 8);
        for (uint32_t i = 0; i < 2; ++i) {
            copro(0x10, nullptr, 0, none);
            const uint32_t a[1] = {mem.u32(0x51F260)};
            copro(0x15, a, 1, none);
            const uint32_t in[3] = {mem.u32(table + i * 0xC), 0, mem.u32(table + i * 0xC + 8)};
            Tgp::Out o;
            copro(0x1A, in, 3, o);
            a::AttackSlot& s = mem.template at<a::AttackSlot>(G::attack_slots, i);
            s.pos[2] = ffrom(o.w[2]) + cz;
            s.pos[0] = ffrom(o.w[0]) + cx;
        }
        copro(0x06, nullptr, 0, none);
    }
}

// 0x53480 depth_lists_task: when the scene's shot-test lists change (pointers 0x520934 / 0x52093C against their
// last values 0x520938 / 0x520940), shot_list_a / _b are cleared to -1 and refilled from the new list (-1
// terminated); the counts are the task's +0x60 / +0x61 bytes; the pointer ends past the list's -1
template <class P> void Game<P>::depth_lists_task(uint32_t task)
{
    if constexpr (!P::pc) {
        struct List { uint32_t cur, last, out, count; };
        const List lists[2] = {{0x520934, 0x520938, G::shot_list_a, task + 0x60},
                               {0x52093C, 0x520940, G::shot_list_b, task + 0x61}};
        for (const List& l : lists) {
            if (mem.u32(l.cur) == mem.u32(l.last))
                continue;
            for (uint32_t i = 0; i < mem.template at<uint8_t>(l.count); ++i)
                mem.set32(l.out + 4 * i, 0xFFFFFFFFu);
            mem.template at<uint8_t>(l.count) = 0;
            uint32_t p = mem.u32(l.cur);
            while (mem.u32(p) != 0xFFFFFFFFu) {
                const uint8_t n = mem.template at<uint8_t>(l.count);
                mem.template at<uint8_t>(l.count) = uint8_t(n + 1);
                mem.set32(l.out + 4 * n, mem.u32(p));
                p += 4;
            }
            p += 4;
            mem.set32(l.cur, p);
            mem.set32(l.last, p);
        }
    } else {
        (void)task;
    }
}

// --- the chapter's start table 0xAB210 (task_group_create runs it once at chapter_load; worker chstart, first written
// in the runtime: Runtime::chapter_task_start). evt_chapter_start 0x66A10 and camera_task_start 0x2B3A0 are ported
// elsewhere (event.cpp, camera.cpp) ---

// 0x34130 sky_task_start: the sky task, the sky drawn last 0
template <class P> void Game<P>::sky_task_start()
{
    if constexpr (!P::pc) {
        task_create(0x33F50, 0x58);
        mem.template at<uint8_t>(0x53E384) = 0;
    }
}

// 0x5DA0 input_task_start
template <class P> void Game<P>::input_task_start()
{
    if constexpr (!P::pc)
        task_create(0x5D90, 0x20);
}

// 0x5D30 InitSetPlayerTask: both players' tasks, each with the start routine of its state (table 0xA8790) and the
// player at +0x20; input_source 0 (the guns, not a recording)
template <class P> void Game<P>::InitSetPlayerTask()
{
    if constexpr (!P::pc) {
        uint32_t t = task_create(mem.u32(0xA8790 + mem.u32(0x51F284) * 8), 0x24);
        mem.template at<uint8_t>(t + 0x20) = 0;
        const uint32_t s2 = mem.u32(0x51F350);
        mem.set32(0x51F328, t);
        t = task_create(mem.u32(0xA8790 + s2 * 8), 0x24);
        mem.template at<uint8_t>(t + 0x20) = 1;
        mem.set32(G::input_source, 0);
        mem.set32(0x51F3F4, t);
    }
}

// 0x5DD0 shake_task_start
template <class P> void Game<P>::shake_task_start()
{
    if constexpr (!P::pc)
        task_create(0x5DC0, 0x20);
}

// 0x4B650 world_init: the world task (world_draw_task), draw zone 0, the zone lists / placement table (0x5430B0 /
// 0x5430B4: the chapter's from tables 0xC0BB0 / 0xC0BD0; the title mode 2's 0xC0C20 / 0xBEF00), and per placement
// (0x50) its model-list position 0 (0x5430D0..) and its state (0x543210..: the placement's +0x10 half-word; 0xFF for
// all in the attract demo, mode 3)
template <class P> void Game<P>::world_init()
{
    if constexpr (!P::pc) {
        task_create(0x4B300, 0x20);
        const int16_t mode = mem.template at<int16_t>(G::game_mode);
        mem.template at<uint8_t>(G::draw_zone) = 0;
        uint32_t lists, placements;
        if (mode == 2) {
            lists = 0xC0C20;
            placements = 0xBEF00;
        } else {
            const uint32_t ch = uint32_t(int32_t(mem.template at<int8_t>(G::chapter)));
            lists = mem.u32(0xC0BB0 + ch * 4);
            placements = mem.u32(0xC0BD0 + ch * 4);
        }
        mem.set32(0x5430B0, lists);
        if ((uint32_t(int32_t(mode)) & 0xFFFF) == 3) {
            for (uint32_t i = 0; i < 0x50; ++i) {
                mem.set32(0x5430D0 + i * 4, 0);
                mem.set32(0x543210 + i * 4, 0xFF);
            }
        } else {
            for (uint32_t i = 0; i < 0x50; ++i) {
                mem.set32(0x5430D0 + i * 4, 0);
                mem.set32(0x543210 + i * 4, uint32_t(int32_t(mem.template at<int16_t>(placements + 0x10 + i * 0x18))));
            }
        }
        mem.set32(0x5430B4, placements);
    }
}

// 0x31D40 attack_slots_task_start: the attack slots' task, the 16 slots free
template <class P> void Game<P>::attack_slots_task_start()
{
    if constexpr (!P::pc) {
        task_create(0x31C00, 0x58);
        for (uint32_t i = 0; i < 16; ++i)
            mem.template at<uint8_t>(G::attack_slots + i * 0x14) = 0;
    }
}

// 0x44AF0 letterbox_task_start: the letterbox task, its slide count 0
template <class P> void Game<P>::letterbox_task_start()
{
    if constexpr (!P::pc)
        mem.set32(task_create(0x44840, 0x58) + 0x50, 0);
}

// 0x53370 depth_lists_task_start: the depth lists' task (+0x58 itself, +0x5C 1, +0x60 / +0x61 0; scene_quads), the
// four list counts 0x520934.. 0, both shot lists (256 words each) -1
template <class P> void Game<P>::depth_lists_task_start()
{
    if constexpr (!P::pc) {
        const uint32_t t = task_create(0x53480, 0x64);
        mem.set32(t + 0x58, t);
        mem.template at<uint8_t>(t + 0x60) = 0;
        mem.template at<uint8_t>(t + 0x61) = 0;
        mem.set32(t + 0x5C, 1);
        for (uint32_t a : {0x520934u, 0x520938u, 0x52093Cu, 0x520940u})
            mem.set32(a, 0);
        mem.set32(G::scene_quads, t);
        for (uint32_t i = 0; i < 0x100; ++i) {
            mem.set32(G::shot_list_a + 4 * i, 0xFFFFFFFFu);
            mem.set32(G::shot_list_b + 4 * i, 0xFFFFFFFFu);
        }
    }
}

// 0x23CA0 demo_timer_start: the attract demo's timer task (mode 3 only)
template <class P> void Game<P>::demo_timer_start()
{
    if constexpr (!P::pc)
        if (mem.template at<int16_t>(G::game_mode) == 3)
            task_create(0x23C70, 0x58);
}

} // namespace hotd::port
