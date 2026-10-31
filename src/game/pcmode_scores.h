// PC Mode's own high-score table (docs/pcmode.md row 15): THOTD.EXE keeps two tables of ten 0x1C-byte rows at
// 0x5CE320 (table 0 Arcade Mode, table 1 PC Mode; saved in SCORES.CFG): the score, the character (0..6 of table
// 0x5CE348), the stage (0..3: "Stage 1".."Stage 4") and the name (15 characters). mode67_ranking_pc 0x44D0C4 makes a
// row per player at the game's end, FUN_0042593c decides who goes in, the front end's name entry (screen 15) names
// the row, InsertScore 0x425798 files it and screen 9 (screen_high_scores_pc_mode 0x4234E0) shows the table.
// ReWrithe keeps the PC Mode table in its own file (pcmode_scores.txt in the settings folder; the PC's SCORES.CFG is
// never written). UNVERIFIED (teaser): read from the disassembly, not compared with a PC harness capture.
#pragma once

#include <string>
#include <vector>

namespace hotd::game {

struct PcModeScores {
    struct Row {
        int score = 0, ch = 0, stage = 0;
        std::string name;
    };
    Row rows[10];
    PcModeScores();                                   // THOTD.EXE's defaults (table 1: BART 62000 Rogan Stage 4 ..)
    bool load(const std::string& path);
    bool save(const std::string& path) const;
    // FUN_0042593c (score, other, table): the table's scores with the other players' scores sorted in; the score goes
    // in when it is at least 1 and at least the tenth of that list
    bool isHigh(int score, const std::vector<int>& others) const;
    // InsertScore 0x425798 (table 1): the row over the tenth, then FUN_0042556c's sort (each row against every later
    // one, a higher score swapped up); the new row's place (0-based)
    int insert(const Row& row);
};

// mode67_ranking_pc 0x44D0C4: the stage of a player's row: the chapter (0-based, 3 at most; 3 also for the ending),
// only while the player has lives left; else the row keeps the last game's stage (the PC's static record 0xC72388)
int pcModeStage(int chapter, int lives, int& kept);

} // namespace hotd::game
