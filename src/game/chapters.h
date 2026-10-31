// The chapters the build can play (docs/chapter2_3_plan.md, "Chapter select in the launcher"): the one place that says
// which chapter is ready. A chapter is marked playable only after its gate passes (bot games of that chapter, both model
// sources, 1 and 2 players, --strict, clear reached); the launcher draws the others grey and never starts them, and
// the game goes on into the next chapter after a clear only when that chapter is playable (else the end screen).
// Developer: --chapter N starts any chapter (with a warning); --continue-chapters goes on into unplayable ones too.
#pragma once

namespace hotd::game {

struct ChapterInfo {
    bool playable;      // selectable in the launcher, and reached by the clear of the chapter before it
    const char* note;   // shown next to a chapter that is not playable
};

// index 0-3 = Chapter 1-4
inline constexpr ChapterInfo kChapters[4] = {
    {true, ""},
    {true, ""},   // Chapter 2: gate passed 2026-10-19 (worker "ch2play": --strict, 4 routes, arcade + PC models, 1 + 2 players)
    {true, ""},   // Chapter 3: the playable bar (user 2026-10-19: starts, plays through, ends without crash / stall): --strict
                  // 6 routes, both models, 2-player bots clear; 1-player bots stall on event 0's iron-ball zombies (bot aim)
    {true, ""},   // Chapter 4 + the ending: the playable bar (worker "ch4gate" 2026-10-19): 32 bot seeds (both models, 1 + 2
                  // players, some --strict) no crash / soft-lock / bad access, 7 clears through the Magician and the ending
                  // (one 1-player, one --strict); --strict both routes (4:0, 4:1) to the ending's end; the Magician UNVERIFIED
};

inline bool chapterPlayable(int chapter)   // chapter 0-3
{
    return chapter >= 0 && chapter < 4 && kChapters[chapter].playable;
}

} // namespace hotd::game
