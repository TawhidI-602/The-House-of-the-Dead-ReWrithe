// Mode 4, the game (proofs-7): mode4_game 0x25C20 and its sub-modes 2-6 (table 0xAB1E0: play, continue begin /
// countdown, chapter end / wait), game_frame 0x262A0 (the frame's task group, the shot test, the attack slots, the
// difficulty) and player_continue_begin 0x5230; first written in the runtime (src/game/flow.cpp, clear.cpp,
// runtime.cpp), moved here for their capture proof. Arcade forms only (the PC's are replayed). Declarations:
// decl/support_b.h.
#include "port/game.h"

namespace hotd::port {

namespace flow {
constexpr uint32_t kStateTable = 0xA8790;     // per player state: start routine, "out" byte (+4), "can start" (+5)
constexpr uint32_t kCredits = 0x51F4B8;       // 16 bytes: the coin / credit counters the countdowns watch
constexpr uint32_t kCountdown = 0x52071C;     // both players out: the shared continue countdown (0x9FFF = 9)
constexpr uint32_t kCreditsSaved = 0x53E090;
constexpr uint32_t kChapterEndCount = 0x53E0B0;
constexpr uint32_t kChapterGroup = 0x53E080;  // the chapter's task group (chapter_load's argument)
constexpr uint32_t kSubmodes = 0xAB1E0;       // mode4_game's table
} // namespace flow

// player p is out (state table +4)
template <class P> bool Game<P>::flow_player_out(int p)
{
    return mem.template at<uint8_t>(flow::kStateTable + mem.u32(player_rec(p)) * 8 + 4) != 0;   // 4P: seats 2 / 3 too
}

// 0x25C20 mode4_game: the sub-mode's routine (table 0xAB1E0; 0 game_new, 1 game_chapter_start and 7 the ending are
// not ported: called through their address)
template <class P> void Game<P>::mode4_game()
{
    if constexpr (!P::pc) {
        const int16_t s = mem.template at<int16_t>(G::game_submode);
        switch (s) {
        case 2: game_play(); break;
        case 3: game_continue_begin(); break;
        case 4: game_continue_countdown(); break;
        case 5: game_chapter_end(); break;
        case 6: game_chapter_end_wait(); break;
        default: call_routine(mem.u32(flow::kSubmodes + uint32_t(int32_t(s)) * 4), 0); break;
        }
    }
}

// 0x25F40 game_play (sub-mode 2): the play-time counters, both players out -> sub-mode 3, who plays alone
// (solo_player_is_p2: player 1 not in play), the frame (0x34AE0 before it: an empty routine)
template <class P> void Game<P>::game_play()
{
    if constexpr (!P::pc) {
        mem.set32(0x51F9D4, mem.u32(0x51F9D4) + 1);
        if (mem.u32(0x51F9D8) != 0)
            mem.set32(0x51F9DC, mem.u32(0x51F9DC) + 1);
        if (flow_player_out(0) && flow_player_out(1) && (!coop4_on() || (flow_player_out(2) && flow_player_out(3))))
            mem.template at<int16_t>(G::game_submode) = 3;
        if (coop4_on()) {
            // 4P (phase 3): the arcade's count and solo player by sides (side 0 = seats 0 / 2, side 1 = seats 1 / 3), so
            // the enemies attack while any player is in play; the hits go round the seats (coop4_victim)
            // (state 2: players 1 / 2 back at their start for a frame after game_chapter_start: still in play)
            auto in = [&](int s) { const uint32_t st = mem.u32(player_rec(s)); return st == 5 || st == 2; };
            const bool s0 = in(0) || in(2);
            const bool s1 = in(1) || in(3);
            mem.template at<int16_t>(G::players_in_game) = int16_t((s0 ? 1 : 0) + (s1 ? 1 : 0));
            // players_in_game_b (the attack slots' tables, the bodies' offsets) as the arcade keeps it: a player out of
            // lives still counts until the continue runs out (player_continue_frame: - 1 only at game over), so a side
            // counts while a seat of it is in the game (not game over / waiting / idle). Counting only the players in
            // play put attack positions 0 / 1 on the camera when everyone was out (table row 0), so the zombie that
            // finished its attack walked round the players' spot, also after the continue (MAME 2P: it stays 2 while
            // both wait at CONTINUE; the zombie waits at its position and attacks again after the continue)
            auto inb = [&](int s) { const uint32_t st = mem.u32(player_rec(s)); return st <= 5 || st == 10; };
            const bool b0 = inb(0) || inb(2), b1 = inb(1) || inb(3);
            mem.template at<int16_t>(G::players_in_game_b) = int16_t((b0 ? 1 : 0) + (b1 ? 1 : 0));
            if (s0 != s1)
                mem.template at<uint8_t>(G::solo_player_is_p2) = s0 ? 0 : 1;
            coop4_boss_frame();
        } else if (mem.template at<uint16_t>(G::players_in_game) == 1)
            mem.template at<uint8_t>(G::solo_player_is_p2) = mem.u32(0x51F284) != 5 ? 1 : 0;
        game_frame(flow::kChapterGroup);
    }
}

// 0x262A0 game_frame (the group pointer): geo_frame_begin, the geometrizer's frame set-up (viewport, light: hardware
// writes, the runtime's renderer does them), every task of the group, the shot test, the attack slots, the difficulty
// level, the 0x520714 timer (while 0x520718 and not 0x520798), the players' invulnerability countdowns
template <class P> void Game<P>::game_frame(uint32_t groupPtr)
{
    if constexpr (!P::pc) {
        geo_frame_begin();
        // (the geometrizer's viewport words 0x800030 / 0x800090 and the window at [0x790]: hardware, the runtime's
        // renderer does them) the coprocessor's focal length, 496 both ways (FIFO command 0x7D)
        Tgp::Out none;
        const uint32_t focal[2] = {0x43F80000u, 0x43F80000u};
        copro(0x7D, focal, 2, none);
        task_group_run(mem.u32(groupPtr));
        shot_resolve();
        attack_slots_update();
        difficulty_level();
        if (mem.u32(0x520718) != 0 && mem.template at<uint16_t>(0x520798) == 0)
            mem.set32(0x520714, mem.u32(0x520714) + 1);
        for (uint32_t p = 0; p < 2; ++p) {
            const int32_t v = int32_t(mem.u32(G::player_invulnerable + 4 * p)) - 1;
            mem.set32(G::player_invulnerable + 4 * p, uint32_t(v < 0 ? 0 : v));
        }
    } else {
        (void)groupPtr;
    }
}

// 0x25FF0 game_chapter_end (sub-mode 5, set when the last event ends): the wait count 1, sub-mode 6, one more frame
template <class P> void Game<P>::game_chapter_end()
{
    if constexpr (!P::pc) {
        mem.set32(flow::kChapterEndCount, 1);
        mem.template at<int16_t>(G::game_submode) = int16_t(mem.template at<int16_t>(G::game_submode) + 1);
        game_play();
    }
}

// 0x26020 game_chapter_end_wait (sub-mode 6): at the end of the count the next chapter (sub-mode 1; after Chapter 4
// sub-mode 7, the ending), else a frame of play
template <class P> void Game<P>::game_chapter_end_wait()
{
    if constexpr (!P::pc) {
        const uint32_t n = mem.u32(flow::kChapterEndCount) - 1;
        mem.set32(flow::kChapterEndCount, n);
        if (n == 0) {
            const uint8_t c = uint8_t(mem.template at<uint8_t>(G::chapter) + 1);
            mem.template at<uint8_t>(G::chapter) = c;
            mem.template at<int16_t>(G::game_submode) = c == 4 ? 7 : 1;
            return;
        }
        game_play();
    }
}

// 0x25D40 game_chapter_start (sub-mode 1, after game_chapter_end_wait put chapter + 1): players 1 / 2 in play
// (state 5) back to state 2 (their task: player_st2_rejoin, PlStartInit's row 2: a full gun, 90 frames without hits;
// lives and score stay), chapter_load at the event NEXTEV left, the next sub-mode (2, the play); no frame of play.
// First written in the runtime (Runtime::chapter_continue: the players kept in state 5), moved here (worker chstart)
template <class P> void Game<P>::game_chapter_start()
{
    if constexpr (!P::pc) {
        if (mem.u32(0x51F284) == 5)
            mem.set32(0x51F284, 2);
        if (mem.u32(0x51F350) == 5)
            mem.set32(0x51F350, 2);
        chapter_load(flow::kChapterGroup);
        mem.template at<int16_t>(G::game_submode) = int16_t(mem.template at<int16_t>(G::game_submode) + 1);
    }
}

// 0x25DA0 chapter_load (the group pointer; game_new, game_chapter_start, demo_start): the grey look off, the heap, the
// 2D screen (not in the attract demo / mode 8: chapter_screen_reset's black cover, the sprite slots marked), the
// characters' bodies, the chapter's texture area for the event (table 0xAB0B0; its cover sprite 0xA1, -1 = none),
// the region's sprite banks (0x27 and the chapter's from table 0xAB240, none for slot 4; another region: 0x2E), the
// HUD's banks, the chapter's task group (table 0xAB210), the camera / shot lists / counters reset, the prompts. The
// original busy-waits for the vblank to clear 0x51F706 (a palette upload pending) after the texture area: not here
// (the runtime has no vblank; it is 0 without a fade). First written in the runtime (Runtime::chapter_load_tasks)
template <class P> void Game<P>::chapter_load(uint32_t groupPtr)
{
    if constexpr (!P::pc) {
        if (mem.template at<int16_t>(G::palette_grey_on) != 0)
            palette_restore_vblank();
        heap_reset();
        const int16_t mode = mem.template at<int16_t>(G::game_mode);
        if (mode != 3 && mode != 8) {
            chapter_screen_reset();
            sprite_slots_mark();
        }
        player_char_models_set();
        const int16_t mode2 = mem.template at<int16_t>(G::game_mode);
        const int32_t cover = mode2 == 3 || mode2 == 8 ? -1 : 0xA1;
        const uint32_t ch = uint32_t(int32_t(mem.template at<int8_t>(G::chapter)));
        const uint32_t ev = uint32_t(int32_t(mem.template at<int8_t>(G::event_index)));
        texture_area_load(mem.u32(mem.u32(0xAB0B0 + ch * 4) + ev * 4), cover);
        if (mem.template at<uint8_t>(G::region) == 0) {
            sprite_bank_load(0x27, 0x127);
            if (mem.template at<uint8_t>(G::chapter) != 4) {
                const uint32_t s = mem.u32(0xAB240 + uint32_t(int32_t(mem.template at<int8_t>(G::chapter))) * 4);
                sprite_bank_load(mem.template at<int16_t>(0xA9B60 + s * 2), int32_t(s));
            }
        } else if (mem.template at<uint8_t>(G::chapter) != 4) {
            sprite_bank_load(0x2E, 0x1E5);
        }
        hud_sprites_load();
        mem.set32(groupPtr, task_group_create(0xAB210));
        camera_reset();
        shot_lists_clear();
        chapter_reset_53e1b0();
        chapter_counters_reset();
        hud_mode_init();
    } else {
        (void)groupPtr;
    }
}

// 0x26080 game_continue_begin (sub-mode 3): a frame, the shared continue countdown at 9, the credits snapshot, both
// players' CONTINUE? labels off, CONTINUE at the centre (sprite 0x108), sub-mode 4
template <class P> void Game<P>::game_continue_begin()
{
    if constexpr (!P::pc) {
        game_frame(flow::kChapterGroup);
        mem.set32(flow::kCountdown, 0x9FFF);
        for (uint32_t i = 0; i < 16; i += 4)
            mem.set32(flow::kCreditsSaved + i, mem.u32(flow::kCredits + i));
        hud_continue_label(0);
        hud_continue_label(1);
        // PC boss mode: no CONTINUE (0x458808); PC Mode: CONTINUE only when a player out of lives can_continue
        // (0x45B8F0: credits left; the credits' snapshot 0xDA0EF0)
        pm_credits_seen = pm_credits;
        const bool pmNone = pm_on() && !pm_can_continue();
        if (!bossNoContinue && !pmNone)
            sprite_place(0x1000000, 1, 0, 6, 0, 0x108);
        mem.template at<int16_t>(G::game_submode) = int16_t(mem.template at<int16_t>(G::game_submode) + 1);
    }
}

// 0x26100 game_continue_countdown (sub-mode 4): -0x2D a frame (about 1.5 s a digit); a trigger pull (a player in
// state 4) skips to the next digit; a coin restarts it at 9; a player back in -> sub-mode 2; at 0 -> mode 5
template <class P> void Game<P>::game_continue_countdown()
{
    if constexpr (!P::pc) {
        game_frame(flow::kChapterGroup);
        if (bossNoContinue) {   // PC boss mode (0x4588A0): every player out -> game over at once
            mem.template at<uint16_t>(0x51F4A8) = mem.template at<uint16_t>(G::players_in_game_b);
            set_mode(5);
            return;
        }
        if (pm_on()) {
            // PC Mode (0x4588A0): no player out of lives can continue (no credits left) -> game over at once; the
            // countdown restarts at 9 when the credits change (0xD9F790 vs the snapshot 0xDA0EF0). Only a player still
            // out of lives (state 4) counts: one who took the last credit this frame is back in (state 1 / 5) and
            // plays on (hunt2, 2026-10-20: ours ended the game there, the walkers' game-over map then had no player
            // in state 6 and drew a NaN walker)
            bool waiting = false;
            for (int s = 0; s < seat_count(); ++s)
                waiting |= mem.u32(player_rec(s)) == 4;
            if (waiting && !pm_can_continue()) {
                mem.template at<uint16_t>(0x51F4A8) = mem.template at<uint16_t>(G::players_in_game_b);
                set_mode(5);
                return;
            }
            if (pm_credits != pm_credits_seen) {
                pm_credits_seen = pm_credits;
                mem.set32(flow::kCountdown, 0x9FFF);
            }
        }
        bool changed = false;
        for (uint32_t i = 0; i < 16; i += 4)
            changed |= mem.u32(flow::kCreditsSaved + i) != mem.u32(flow::kCredits + i);
        if (changed)
            mem.set32(flow::kCountdown, 0x9FFF);
        for (uint32_t i = 0; i < 16; i += 4)
            mem.set32(flow::kCreditsSaved + i, mem.u32(flow::kCredits + i));
        uint32_t pressed = 0;
        if (mem.u32(0x51F284) == 4)
            pressed = mem.template at<uint16_t>(0x51F2CC);
        if (mem.u32(0x51F350) == 4)
            pressed |= mem.template at<uint16_t>(0x51F398);
        int32_t c = int32_t(mem.u32(flow::kCountdown));
        if ((pressed & 1) && c <= 0x7FFF)
            c = int32_t((uint32_t(c) & 0xFFFFF000u) + 1);
        mem.set32(flow::kCountdown, uint32_t(c));
        sprite_place(0x1000000, 1, 0, 0x2B, 0xF, mem.template at<int16_t>(0xAB260 + uint32_t(c >> 12) * 2));
        if (!flow_player_out(0) || !flow_player_out(1) || (coop4_on() && (!flow_player_out(2) || !flow_player_out(3)))) {
            mem.template at<int16_t>(G::game_submode) = 2;   // (4P: players 3 / 4 continuing too)
            player_continue_begin(0);
            player_continue_begin(1);
            continue_screen_draw();
            return;
        }
        c -= 0x2D;
        mem.set32(flow::kCountdown, uint32_t(c));
        if (c > 0)
            return;
        mem.template at<uint16_t>(0x51F4A8) = mem.template at<uint16_t>(G::players_in_game_b);
        set_mode(5);
    }
}

// 0x5230 player_continue_begin (player): one still out (state 4) counts down on its own (its task: 0x52C0
// player_continue_start)
template <class P> void Game<P>::player_continue_begin(int p)
{
    if constexpr (!P::pc) {
        const uint32_t rec = 0x51F280 + uint32_t(p) * 0xCC;
        if (mem.u32(rec + 4) == 4)
            mem.set32(mem.u32(rec + 0xA8), 0x52C0);
    } else {
        (void)p;
    }
}

// 0x5260 player_st4_out_of_lives (task): the credits snapshot (player +0x68), the player's own countdown at 9 (+0xA4),
// the task to player_continue_frame, the HUD's full redraw and CONTINUE?. PC Mode (THOTD.EXE 0x434F38): the snapshot
// is of the PC's continue credits (0xD9F790 / 0xD9F794: the shared pool pm_credits, slot 1 unused) and the countdown
// starts with or without credits (no can_continue test here); only CONTINUE? needs one (hud_continue_sprite 0x50BC24)
template <class P> void Game<P>::player_st4_out_of_lives(uint32_t task)
{
    if constexpr (!P::pc) {
        const int p = mem.template at<int8_t>(task + 0x20);
        const uint32_t rec = 0x51F280 + uint32_t(p) * 0xCC;
        for (uint32_t i = 0; i < 16; i += 4)
            mem.set32(rec + 0x68 + i, pm_on() ? (i == 0 ? uint32_t(pm_credits) : 0u) : mem.u32(flow::kCredits + i));
        mem.set32(rec + 0xA4, 0x9FFF);
        mem.set32(task, 0x5300);
        hud_player_full_draw(task);
        hud_continue_sprite(mem.template at<int8_t>(task + 0x20));
    } else {
        (void)task;
    }
}

// 0x52C0 player_continue_start (task): the player's countdown at 9, CONTINUE?, then player_continue_frame
template <class P> void Game<P>::player_continue_start(uint32_t task)
{
    if constexpr (!P::pc) {
        const int p = mem.template at<int8_t>(task + 0x20);
        if (pm_on() && !pm_can_continue()) {   // PC Mode (0x434F94): no credits left -> out (state 6)
            mem.template at<int16_t>(G::players_in_game_b) = int16_t(mem.template at<int16_t>(G::players_in_game_b) - 1);
            player_state_set(6, 1, task);
            hud_continue_label(p);
            return;
        }
        mem.set32(0x51F324 + uint32_t(p) * 0xCC, 0x9FFF);
        hud_continue_sprite(mem.template at<int8_t>(task + 0x20));
        mem.set32(task, 0x5300);
        player_continue_frame(task);
    } else {
        (void)task;
    }
}

// 0x5300 player_continue_frame (task): the player's own countdown while the other plays on (sub-mode 4 counts for
// both): -0x2D a frame, a trigger pull skips to the next digit, a coin restarts it at 9; start = continue
// (player_start_check), at 0 -> state 6 (game over), players_in_game_b - 1. The HUD: CONTINUE? off when the player
// is back or out, else the digit (own countdown) and the start prompt (player_continue_nop 0x94F20 first: empty)
template <class P> void Game<P>::player_continue_frame(uint32_t task)
{
    if constexpr (!P::pc) {
        const uint32_t rec = 0x51F280 + uint32_t(int32_t(mem.template at<int8_t>(task + 0x20))) * 0xCC;
        player_continue_nop(task);
        hud_popup_update(task);
        // PC Mode (0x435018): the countdown restarts at 9 when the PC's continue credits change (the pool: another
        // player's join or continue), not the arcade's coins
        auto creditsAt = [&](uint32_t i) { return pm_on() ? (i == 0 ? uint32_t(pm_credits) : 0u) : mem.u32(flow::kCredits + i); };
        bool changed = false;
        for (uint32_t i = 0; i < 16; i += 4)
            changed |= mem.u32(rec + 0x68 + i) != creditsAt(i);
        if (changed) {
            for (uint32_t i = 0; i < 16; i += 4)
                mem.set32(rec + 0x68 + i, creditsAt(i));
            mem.set32(rec + 0xA4, 0x9FFF);
        }
        bool ended = false, waiting = true;
        if (mem.template at<int16_t>(G::game_submode) != 4) {
            uint32_t t = mem.u32(rec + 0xA4);
            if ((mem.template at<uint8_t>(rec + 0x4C) & 1) && int32_t(t) <= 0x7FFF)
                t = (t & 0xFFFFF000u) + 1;
            t -= 0x2D;
            mem.set32(rec + 0xA4, t);
            ended = int32_t(t) <= 0;
        }
        if (player_start_check(task, 1) != 0) {
            hud_continue_label(mem.template at<int8_t>(task + 0x20));
            waiting = false;
        } else if (ended) {
            mem.set32(rec + 0xA4, 0);
            mem.template at<int16_t>(G::players_in_game_b) = int16_t(mem.template at<int16_t>(G::players_in_game_b) - 1);
            player_state_set(6, 1, task);
            hud_continue_label(mem.template at<int8_t>(task + 0x20));
            waiting = false;
        }
        if (mem.template at<int16_t>(G::game_submode) != 4 && waiting)
            hud_continue_digit(mem.template at<int8_t>(task + 0x20), int32_t(mem.u32(rec + 0xA4)) / 4096);
        hud_start_prompt(mem.template at<uint8_t>(rec), 1, waiting ? 1 : 0);
    } else {
        (void)task;
    }
}

// 0xDE70 set_mode (mode): the player counts and a few flags cleared, the camera's bit 1, the mode with sub-mode 0;
// a mode other than 4 / 5 puts both players in state 9 (waiting); the frame's two words 0x51F610 cleared
template <class P> void Game<P>::set_mode(int mode)
{
    if constexpr (!P::pc) {
        mem.template at<int16_t>(G::players_in_game_b) = 0;
        mem.template at<int16_t>(G::players_in_game) = 0;
        mem.template at<int16_t>(0x51F4A6) = 0;
        mem.template at<int16_t>(0x51F4A2) = 0;
        mem.set32(0x51F404, 0);
        mem.set32(0x51F338, 0);
        mem.set32(0x51F60C, 0);
        mem.template at<int16_t>(0x51F778) = 0;
        mem.template at<int16_t>(G::game_mode) = int16_t(mode);
        mem.template at<int16_t>(G::game_submode) = 0;
        mem.set32(G::camera_status, mem.u32(G::camera_status) | 2);
        if (mode > 5 || mode < 4) {
            mem.set32(0x51F284, 9);
            mem.set32(0x51F350, 9);
        }
        mem.set32(0x51F610, 0);
        mem.set32(0x51F614, 0);
    } else {
        (void)mode;
    }
}

} // namespace hotd::port
