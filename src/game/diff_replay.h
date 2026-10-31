// The whole-game differential (developer; docs/proof.md "Whole-game differential"): the runtime replays a MAME run frame
// by frame and writes the same per-frame state record, so tools/state_diff.py names the first frame and field where the
// runtime leaves the arcade.
//
// The arcade side (tools/mame_diff_rec.lua, run_diff_rec.ps1) gives: snapshot.bin (work RAM at the end of the start
// frame S), inputs.bin (per frame F > S: every work RAM byte that changed between the end of frame F-1 and the start of
// frame F's logic: the vblank interrupt's guns and flash cycle, wait_vblank's switches, the session's lives / ammo
// writes), state.rec (HOTDREC1: tools/state_map.tsv's globals and the tasks run, per frame). `hotd --diff-replay <dir>`
// loads the snapshot (no chapter start), applies each frame's inputs where frame_tick increments frame_counter, runs the
// input task with the arcade's player_input_update in place of the mouse stand-in, and writes <dir>/ours.rec (and
// ours_full_<F>.bin next to the arcade's full_<F>.bin).
#pragma once

#include "port/mem.h"

#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

namespace hotd::game {

class DiffReplay {
public:
    bool load(const std::string& dir, const std::string& stateMap, std::string& err);
    ~DiffReplay();
    uint32_t startFrame() const { return start_; }
    uint32_t frames() const { return uint32_t(inputs_.size()); }
    uint32_t current() const { return start_ + 1 + done_; }   // the arcade frame number being run
    // where frame_tick increments frame_counter: the frame's external writes
    void apply(port::Mem& mem);
    // each value stored into the current-task word (task_group_run, task_unlink), as MAME's write tap sees them
    void noteTask(uint32_t t)
    {
        if (seen_.insert(t).second) {
            tasks_.push_back(t);
            midUpTo(uint32_t(tasks_.size() - 1));
        }
    }
    // the frame's end (frame_tick's write of 0xF0F to 0x8000F0): the record, then the next frame
    void record(port::Mem& mem);
    int fullEvery = 600;
    std::vector<uint8_t> palette2;   // palette RAM bank 2 at the snapshot (palette2.bin; empty in older recordings)
    // a vblank once-list routine (0x511FE0) the arcade's interrupt ran in the middle of the frame's logic (its slot
    // cleared by a mid write), with 0x511F04 as it was before those writes: the runtime does its palette RAM part (the
    // work RAM part comes with the writes)
    std::function<void(uint32_t routine, uint32_t listPtr)> onVblankRan;

private:
    struct Run {
        uint32_t addr;
        std::vector<uint8_t> bytes;
    };
    // an interrupt write in the middle of the frame's logic: after the first `tasks` tasks of the frame
    struct Mid {
        uint32_t tasks, addr, data, mask;
    };
    void midUpTo(uint32_t tasks);   // the interrupt writes that came after at most `tasks` tasks
    std::string dir_;
    uint32_t start_ = 0, done_ = 0;
    std::vector<std::vector<Run>> inputs_;
    std::vector<std::vector<Mid>> mids_;
    size_t midNext_ = 0;
    port::Mem* mem_ = nullptr;
    std::vector<std::pair<uint32_t, uint32_t>> ranges_;
    std::vector<uint32_t> tasks_;
    std::unordered_set<uint32_t> seen_;
    std::FILE* out_ = nullptr;
};

} // namespace hotd::game
