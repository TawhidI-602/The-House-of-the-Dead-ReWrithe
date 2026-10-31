// Event script interpreter (docs/formats.md §28-29): the PC's EVT\ALLSTAGE\EVTALL.RBH (the arcade ROM holds the same
// scripts). First version for the playable slice: the rail (CCTB 64 = play frames of a camera path), the waits between
// segments, and enemy spawns (ENTB* records). Every other opcode is skipped with its operands.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hotd {

struct SpawnRecord {
    int type = 0;            // creature type (record ID & 0xFF)
    uint32_t id = 0;         // the record's ID word (flags in the high bits)
    float pos[3] = {};       // world position
    float facing = 0;        // radians (16-bit angle)
    int frame = 0;           // camera frame of the event's path when it appears (0 = right away)
    int path = -1;           // camera path current when the record was queued (runner)
};

// A camera segment (CCTB 64) and the draw zone in force while it plays (the last BGTB before it).
struct ZoneSpan { int path = 0, start = 0, end = 0, zone = 0, area = -1; };   // area: last TEXCHG (-1 = the chapter's first)

class EventScripts {
public:
    std::vector<ZoneSpan> zoneSpans(int list) const;   // every event of the list, in order
    bool load(const std::string& path, std::string& err);
    int eventCount(int list) const;
    uint32_t eventOffset(int list, int event) const;   // BODY offset
    uint32_t word(uint32_t bodyOffset) const;          // 0xFFFFFFFF past the end
    bool record(uint32_t bodyOffset, SpawnRecord& out) const;   // an enemy record (kind 0, 96 or 99), false otherwise
private:
    std::vector<uint8_t> data_;
    std::vector<std::vector<uint32_t>> lists_;
};

class EventRunner {
public:
    // Runs the given events of one list in order (e.g. Chapter 1: 0, 1, 3, 4, 5, 6 = the replay's branch).
    void start(const EventScripts* scripts, int list, std::vector<int> order);
    // One game frame. enemiesAlive: how many spawned enemies are still alive (WAIT_ENNUM / WAIT_ENALV).
    void step(int enemiesAlive);

    bool running() const { return scripts_ && !done_; }
    int path() const { return path_; }
    int zone() const { return zone_; }              // draw zone (BGTB), -1 before the first
    int area() const { return area_; }              // texture area (TEXCHG), -1 before the first
    float frame() const { return frame_; }
    int event() const { return orderPos_ < order_.size() ? order_[orderPos_] : -1; }
    std::vector<SpawnRecord> takeSpawns();   // spawns released since the last call

private:
    void beginEvent();
    bool execute(int enemiesAlive);           // runs instructions until one blocks; false = event ended

    const EventScripts* scripts_ = nullptr;
    int list_ = 0;
    std::vector<int> order_;
    size_t orderPos_ = 0;
    uint32_t pc_ = 0, end_ = 0;               // BODY offsets
    bool done_ = true;
    int path_ = 0, segEnd_ = 0, zone_ = -1, area_ = -1;
    float frame_ = 0;
    bool camBusy_ = false;
    int waitFrames_ = 0;
    std::vector<SpawnRecord> pending_, released_;
};

} // namespace hotd
