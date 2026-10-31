// The chapter's end (docs/teaser_plan.md R4): kind 176 (the chapter clear and its results: the hostages rescued, the
// bonus lives), game_play's chapter-end sub-modes 5 / 6, the letterbox task (scene_state and shot_enable), and the
// HUD (the 2D layer: src/game/sprites.cpp). Read from the typed
// decompilation (0x9B880..0x9BDC0, 0x25FF0 / 0x26020, 0x44840, 0x6BA90..0x6BC40); UNVERIFIED (teaser). Sprites are
// the placeholder HUD's (src/game/hud.cpp reads Runtime::clear / boss); kind 176 draws the researchers' model 0x65E
// (in the results' window).
#include "game/chapters.h"
#include "game/runtime.h"

namespace hotd::game {

using G = A::G;
using port::a::Player;

namespace {
constexpr uint32_t kChapterEndCount = 0x53E0B0;
} // namespace

// kind 176 (0x9B690 .. 0x9BDC0): the port's (src/port/chapter_clear.cpp, proofs-7); the runtime keeps its HUD's view
// (Runtime::clear: the placeholder HUD's results lines)
void Runtime::kind176_chapter_clear(uint32_t obj)
{
    clear = ChapterClear{};
    clear.active = true;
    clear.rescued = mem.at<int16_t>(G::hostages_rescued_chapter);
    clear.bonus = int32_t(kind176_bonus_lives());
    if (log)
        log("chapter clear: " + std::to_string(clear.rescued) + " hostages rescued, " + std::to_string(clear.bonus) + " bonus lives");
    port::Game<A>::kind176_chapter_clear(obj);
}

void Runtime::kind176_results(uint32_t obj)
{
    port::Game<A>::kind176_results(obj);
    clear.timer = int32_t(mem.u32(obj + 0x50));
}

void Runtime::kind176_results_hold(uint32_t obj)
{
    port::Game<A>::kind176_results_hold(obj);
    clear.held = true;
}

void Runtime::kind176_results_end(uint32_t obj)
{
    port::Game<A>::kind176_results_end(obj);
    clear.held = true;
}

// 0x26020 game_chapter_end_wait (sub-mode 6, the port's: src/port/game_flow.cpp): the original loads the next chapter
// when its count ends (sub-mode 1, chapter + 1; after Chapter 4 sub-mode 7, the ending). Here: the next chapter when
// it is playable (game/chapters.h; the port's sub-mode 1, game_chapter_start), else the game ends there: chapterDone (the
// host shows the end screen) or the name entry.
void Runtime::game_chapter_end_wait()
{
    if (mem.u32(kChapterEndCount) == 1) {
        mem.set32(kChapterEndCount, 0);
        const int next = mem.at<int8_t>(G::chapter) + 1;
        if (next == 4) {   // after Chapter 4: the original's (chapter + 1, sub-mode 7: the ending, src/game/ending.cpp)
            if (log)
                log("chapter 4 ended; the ending next");
            mem.set32(kChapterEndCount, 1);   // (the port's counts it down to 0 itself)
            port::Game<A>::game_chapter_end_wait();
            return;
        }
        if (next < 4 && (chapterPlayable(next) || continueUnplayable)) {
            if (log)
                log("chapter " + std::to_string(next) + " ended; chapter " + std::to_string(next + 1) + " next" +
                    (chapterPlayable(next) ? "" : " (developer: not marked playable)"));
            mem.set32(kChapterEndCount, 1);   // the port's: chapter + 1, sub-mode 1 (game_chapter_start the next frame)
            port::Game<A>::game_chapter_end_wait();
            return;
        }
        if (log)
            log("chapter " + std::to_string(next) + " ended (the game ends here: chapter " + std::to_string(next + 1) +
                " is not playable)");
        if (titleFlow || nameEntryOnClear)   // title-2: as the end of the game (rank_insert, the name entry)
            game_ending_rank();
        else
            chapterDone = true;
        return;
    }
    port::Game<A>::game_chapter_end_wait();
}

// 0x44840 letterbox_task: the cut-scene bars by scene_state (0 / 5: bars on at once, then 4 = cut-scene; 1: the bars
// slide away over 40 frames, then 2 = play; 3: they slide in over 40 frames, then 4; 6: off at once, then 2), and
// whether a shot counts (shot_enable). 7 / 8: the screen-wide sprite cover (the previous state comes back). The
// original draws the bars as sprites on layer 2 (0x8E full, 0xA0 / 0x97 rows and 0x98.. / 0x8F.. edges by the step);
// here the HUD draws them from Runtime::letterbox (0..0x28 lines). polish: the states were read the wrong way round
// (bars in play, none in the cut-scenes); MAME fullgame_us: scene_state 4 with bars (t00260, snap 0012), 2 in play
// without (t00360, snap 0017). UNVERIFIED (teaser) for the slide steps.
void Runtime::letterbox_task(uint32_t task)
{
    const uint32_t hudState = 0x51F4B5;
    mem.at<uint8_t>(0x512080) = 0;
    uint8_t s = mem.at<uint8_t>(G::scene_state);
    if (mem.at<uint8_t>(hudState) != s) {
        if (s == 3)
            mem.set32(task + 0x50, 0);
        else if (s == 1)
            mem.set32(task + 0x50, 0x28);
    }
    switch (s) {
    case 0:
        mem.at<uint8_t>(G::scene_state) = 4;
        mem.set32(G::shot_enable, 1);
        letterbox = 0x28;
        break;
    case 1: {
        mem.set32(G::shot_enable, 1);
        const int32_t v = int32_t(mem.u32(task + 0x50)) - 1;
        mem.set32(task + 0x50, uint32_t(v));
        if (v == -1) {
            mem.at<uint8_t>(0x512080) = 1;
            mem.at<uint8_t>(G::scene_state) = 2;
            mem.at<uint8_t>(hudState) = 2;
            letterbox = 0;
            return;
        }
        letterbox = v;
        break;
    }
    case 3: {
        const int32_t v = int32_t(mem.u32(task + 0x50)) + 1;
        mem.set32(task + 0x50, uint32_t(v));
        if (v == 0x29) {
            mem.at<uint8_t>(G::scene_state) = 4;
            mem.set32(G::shot_enable, 0);
            letterbox = 0x28;
            break;
        }
        letterbox = v;
        break;
    }
    case 5:
        mem.at<uint8_t>(G::scene_state) = 4;
        mem.set32(G::shot_enable, 0);
        letterbox = 0x28;
        break;
    case 6:
        mem.at<uint8_t>(G::scene_state) = 2;
        mem.at<uint8_t>(0x512080) = 1;
        mem.set32(G::shot_enable, 1);
        letterbox = 0;
        break;
    case 7:
    case 8:
        mem.at<uint8_t>(G::scene_state) = mem.at<uint8_t>(hudState);
        break;
    default:
        break;
    }
    mem.at<uint8_t>(hudState) = mem.at<uint8_t>(G::scene_state);
}


} // namespace hotd::game
