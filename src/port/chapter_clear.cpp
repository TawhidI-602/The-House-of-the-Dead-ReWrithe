// Kind 176, the chapter clear and its results (proofs-7): 0x9B690 .. 0x9BDC0, first written in the runtime
// (src/game/clear.cpp, R4), moved here for their capture proof. Arcade forms only (the PC's are replayed). The
// runtime's overrides add its HUD's view (Runtime::clear). Declarations: decl/support_b.h.
#include "port/game.h"

namespace hotd::port {

namespace k176 {
constexpr uint32_t kResultsDone = 0x51EA3E;    // byte: the results are over (the event script waits on it)
constexpr uint32_t kBonusGiven = 0x543AA0;     // byte: the bonus lives were added
constexpr uint32_t kBonusTable = 0xCD510;      // [chapter][hostages rescued 0..7] bonus lives
constexpr uint32_t kSkip = 0x51F5F4;           // byte: a trigger pull skips the results
} // namespace k176

// 0x9B690 kind176_score_draw (player, x, y, x offset): the score in sprite digits 0x181.. (leading zeros left out)
template <class P> void Game<P>::kind176_score_draw(int p, int x, int y, int dx)
{
    if constexpr (!P::pc) {
        const int32_t s = int32_t(mem.u32(player_rec(p) + 8));   // 0x51F28C + p x 0xCC (4P: seats 2 / 3 in the block)
        const int x0 = int(uint16_t(x)) + int(uint16_t(dx)) * (p & 1);   // (4P: players 3 / 4 above 1 / 2)
        const int yy = int(uint16_t(y));
        const int32_t div[5] = {100000, 10000, 1000, 100, 10};
        for (int k = 0; k < 5; ++k)
            if (s > div[k] - 1)
                sprite_draw_anim(0x1000000, 1, 0, x0 + 2 * k, yy, uint32_t(s / div[k] % 10 + 0x181));
        sprite_draw_anim(0x1000000, 1, 0, x0 + 10, yy, uint32_t(s % 10 + 0x181));
    } else {
        (void)p, (void)x, (void)y, (void)dx;
    }
}

// the bonus lives for the hostages rescued in this chapter (table 0xCD510: 8 a chapter; more than 7: the last)
template <class P> uint32_t Game<P>::kind176_bonus_lives()
{
    const int16_t n = mem.template at<int16_t>(G::hostages_rescued_chapter);
    const uint32_t ch = mem.template at<uint8_t>(G::chapter);
    if (n > 7)
        return mem.template at<uint8_t>(k176::kBonusTable + ch * 8 + 7);
    return mem.template at<uint8_t>(k176::kBonusTable + ch * 8 + uint16_t(n));
}

// the bonus line (inline in 0x9B9F0 / 0x9BC00): its digit, both players' lives + the bonus (at most max_lives; a
// player not in play too)
template <class P> void Game<P>::kind176_bonus_add()
{
    const uint32_t bonus = kind176_bonus_lives();
    sprite_draw_anim(0x1000000, 1, 0, 0x2A, 0x19, 0xD0 + bonus);
    const int32_t maxLives = mem.template at<uint8_t>(G::max_lives);
    for (int p = 0; p < seat_count(); ++p) {   // 4P: players 3 / 4 too
        const uint32_t a = player_rec(p) + 0xC;
        int32_t v = int32_t(mem.u32(a)) + int32_t(bonus);
        mem.set32(a, uint32_t(v));
        if (v >= maxLives)
            mem.set32(a, uint32_t(maxLives));
    }
    mem.template at<uint8_t>(k176::kBonusGiven) = 1;
}

// the tail of 0x9B9F0 / 0x9BC00 / 0x9BDC0: model 0x65E (the rescued researchers in the window over the area picture
// 0xA6) in camera space (coprocessor 0x21) at (0, -10, 40), turned 0x8000 about y
template <class P> void Game<P>::kind176_backdrop_draw()
{
    Tgp::Out none;
    copro(0x21, nullptr, 0, none);
    const uint32_t at[3] = {0, 0xC1200000u, 0x42200000u};
    copro(0x12, at, 3, none);
    const uint32_t ry[1] = {0x8000};
    copro(0x15, ry, 1, none);
    geo_draw_model(0x65E);
    copro(0x06, nullptr, 0, none);
}

// 0x9B880 kind176_chapter_clear: the grey look off, the clear music, the results' sprites (the metal frame 0xA7, the
// next area's picture 0xA6, SCORE and each playing player's score), the shots off, 480 frames of results
template <class P> void Game<P>::kind176_chapter_clear(uint32_t obj)
{
    if constexpr (!P::pc) {
        if (mem.template at<uint16_t>(G::palette_grey_on) != 0)
            palette_restore_vblank();
        sound_send3(0xA01005);
        sprite_bank_load(0x2B, 0xA7);
        sprite_bank_load(0x2A, 0xA6);
        sprite_bank_load(0x3A, 0xC6);
        sprite_bank_load(0x36, 0x123);
        sprite_bank_load(0x3D, 0x181);
        sprite_draw_anim(0x1004000, 1, 0, 0, 0, 0xA7);
        sprite_draw_anim(0x1004000, 0, 0, 0xD, 7, 0xA6);
        if (mem.u32(0x51F284) == 5) {
            sprite_draw_anim(0x1000000, 1, 0, 8, 0x20, 0x123);
            kind176_score_draw(0, 10, 0x23, 0);
        }
        if (mem.u32(0x51F350) == 5) {
            sprite_draw_anim(0x1000000, 1, 0, 0x29, 0x20, 0x123);
            kind176_score_draw(1, 10, 0x23, 0x21);
        }
        if (coop4_on())   // 4P: players 3 / 4's scores at the top (their HUD corners), the same SCORE sprite and digits
            for (int p = 2; p < 4; ++p)
                if (mem.u32(player_rec(p)) == 5) {
                    sprite_draw_anim(0x1000000, 1, 0, p == 2 ? 8 : 0x29, 1, 0x123);
                    kind176_score_draw(p, 10, 4, 0x21);
                }
        mem.template at<uint8_t>(k176::kBonusGiven) = 0;
        mem.template at<uint8_t>(k176::kSkip) = 0;
        mem.set32(G::shot_enable, 0);
        mem.set32(G::camera_status, mem.u32(G::camera_status) | 0x10);
        mem.set32(obj + 0x50, 0x1E0);
        mem.set32(obj, 0x9B9F0);
        kind176_results(obj);
    } else {
        (void)obj;
    }
}

// 0x9B9F0 kind176_results: counts down; from 419 on a trigger (the skip byte) shows everything at once
// (kind176_results_hold); the lines appear at 420 (RESCUED), 360 (their number), 300 (LIFE BONUS), 240 (the bonus
// lives, added then); at 0 the results are done
template <class P> void Game<P>::kind176_results(uint32_t obj)
{
    if constexpr (!P::pc) {
        const int32_t t = int32_t(mem.u32(obj + 0x50)) - 1;
        mem.set32(obj + 0x50, uint32_t(t));
        if (t <= 0x1A3 && mem.template at<uint8_t>(k176::kSkip) != 0) {
            mem.set32(obj, 0x9BC00);
        } else if (t <= 0) {
            mem.template at<uint8_t>(k176::kResultsDone) = 1;
            mem.set32(G::camera_status, mem.u32(G::camera_status) & ~0x10u);
        }
        if (t == 0x1A4)
            sprite_draw_anim(0x1000000, 1, 0, 0x11, 0xF, 0xDA);
        else if (t == 0x168)
            sprite_draw_anim(0x1000000, 1, 0, 0x29, 0xD, uint32_t(int32_t(mem.template at<int16_t>(G::hostages_rescued_chapter)) + 0xC6));
        else if (t == 300)
            sprite_draw_anim(0x1000000, 1, 0, 0x12, 0x19, 0xDB);
        else if (t == 0xF0)
            kind176_bonus_add();
        kind176_backdrop_draw();
    } else {
        (void)obj;
    }
}

// 0x9BC00 kind176_results_hold (skipped): everything shown, the bonus added if not yet, the results done
template <class P> void Game<P>::kind176_results_hold(uint32_t obj)
{
    if constexpr (!P::pc) {
        mem.set32(obj, 0x9BDC0);
        mem.template at<uint8_t>(k176::kResultsDone) = 1;
        mem.set32(G::camera_status, mem.u32(G::camera_status) & ~0x10u);
        sprite_draw_anim(0x1000000, 1, 0, 0x11, 0xF, 0xDA);
        sprite_draw_anim(0x1000000, 1, 0, 0x29, 0xD, uint32_t(int32_t(mem.template at<int16_t>(G::hostages_rescued_chapter)) + 0xC6));
        sprite_draw_anim(0x1000000, 1, 0, 0x12, 0x19, 0xDB);
        if (mem.template at<uint8_t>(k176::kBonusGiven) == 0)
            kind176_bonus_add();
        kind176_backdrop_draw();
    } else {
        (void)obj;
    }
}

// 0x9BDC0 kind176_results_end: only the backdrop until the chapter ends
template <class P> void Game<P>::kind176_results_end(uint32_t obj)
{
    (void)obj;
    if constexpr (!P::pc)
        kind176_backdrop_draw();
}

} // namespace hotd::port
