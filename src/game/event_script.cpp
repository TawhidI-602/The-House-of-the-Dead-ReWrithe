#include "game/event_script.h"

#include "assets/assets.h"

#include <cmath>
#include <cstring>

namespace hotd {

namespace {

constexpr size_t kBody = 0x28;   // BODY data in the file; every offset is relative to it
constexpr uint32_t kEnd = 0xFFFFFFFF;

// operand counts (tools/evt_dis.py SPEC): -1 = list ended by -1, -2 = CCTB (by sub-operation)
int operands(uint32_t op)
{
    switch (op) {
    case 93: case 16: case 83: case 92: case 66: case 26: return 0;
    case 90: case 20: case 72: case 73: case 70: case 71: case 82: case 81: case 84: case 80: case 75: case 88: case 15:
    case 77: case 85: case 40: case 79: case 91: case 67: case 65: return 1;
    case 68: case 76: case 89: case 52: return 2;
    case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10: case 11: case 12: case 50: case 51:
    case 87: return -1;
    case 60: return -2;
    default: return -3;   // unknown: data (after the last NEXTEV)
    }
}

int32_t s32(uint32_t v) { return int32_t(v); }

} // namespace

bool EventScripts::load(const std::string& path, std::string& err)
{
    if (!readFile(path, data_, err)) return false;
    if (data_.size() < kBody + 8 || std::memcmp(data_.data(), "PIFF", 4) != 0) { err = path + ": not an RBH file"; return false; }
    lists_.clear();
    std::vector<uint32_t> heads;
    for (uint32_t o = 0; word(o) != kEnd && heads.size() < 16; o += 4) heads.push_back(word(o));
    for (uint32_t h : heads) {
        std::vector<uint32_t> evs;
        for (uint32_t o = h; word(o) != kEnd && evs.size() < 256; o += 4) evs.push_back(word(o));
        lists_.push_back(evs);
    }
    return !lists_.empty();
}

int EventScripts::eventCount(int list) const { return list >= 0 && list < int(lists_.size()) ? int(lists_[list].size()) : 0; }
uint32_t EventScripts::eventOffset(int list, int event) const { return lists_[list][event]; }

uint32_t EventScripts::word(uint32_t o) const
{
    uint32_t v = kEnd;
    if (kBody + o + 4 <= data_.size()) std::memcpy(&v, data_.data() + kBody + o, 4);
    return v;
}

// 40-byte enemy record (formats.md §29): kind, flags, x, y, z, 0, facing, 0, frame << 16, ID (type = ID & 0xFF)
bool EventScripts::record(uint32_t o, SpawnRecord& r) const
{
    const uint32_t kind = word(o);
    if (kind != 0 && kind != 96 && kind != 99) return false;
    for (int i = 0; i < 3; ++i) { uint32_t v = word(o + 8 + 4 * i); std::memcpy(&r.pos[i], &v, 4); }
    for (float p : r.pos) if (!std::isfinite(p) || std::fabs(p) > 5000) return false;
    r.facing = float(word(o + 24) & 0xFFFF) * 3.14159265f / 32768.0f;
    r.frame = int(word(o + 32) >> 16);
    r.id = word(o + 36);
    r.type = int(r.id & 0xFF);
    return r.type < 94;
}

void EventRunner::start(const EventScripts* scripts, int list, std::vector<int> order)
{
    scripts_ = scripts;
    list_ = list;
    order_ = std::move(order);
    orderPos_ = 0;
    pending_.clear();
    released_.clear();
    done_ = false;
    beginEvent();
}

void EventRunner::beginEvent()
{
    while (orderPos_ < order_.size() && order_[orderPos_] >= scripts_->eventCount(list_)) ++orderPos_;
    if (orderPos_ >= order_.size()) { done_ = true; return; }
    const int ev = order_[orderPos_];
    pc_ = scripts_->eventOffset(list_, ev);
    end_ = ev + 1 < scripts_->eventCount(list_) ? scripts_->eventOffset(list_, ev + 1) : pc_ + 0x4000;
    if (end_ <= pc_) end_ = pc_ + 0x4000;
    while (scripts_->word(pc_) != kEnd) pc_ += 4;   // section offsets
    pc_ += 4;
    camBusy_ = false;
    waitFrames_ = 0;
}

std::vector<ZoneSpan> EventScripts::zoneSpans(int list) const
{
    std::vector<ZoneSpan> out;
    for (int e = 0; e < eventCount(list); ++e) {
        // skip the section offsets, then walk the code up to the event's data
        uint32_t pc = eventOffset(list, e);
        while (word(pc) != kEnd) pc += 4;
        pc += 4;
        int zone = 0, last = 0, area = out.empty() ? -1 : out.back().area;
        for (int guard = 0; guard < 20000; ++guard) {
            const uint32_t op = word(pc);
            const int n = operands(op);
            if (n == -3) break;
            if (n == -1) {
                uint32_t o = pc + 4;
                while (word(o) != kEnd && guard++ < 20000) o += 4;
                pc = o + 4;
                continue;
            }
            if (n == -2) {
                const uint32_t sub = word(pc + 4);
                if (sub == 64) {
                    ZoneSpan z;
                    z.start = s32(word(pc + 8)) >= 0 ? s32(word(pc + 8)) : last;
                    z.end = s32(word(pc + 12));
                    z.path = s32(word(pc + 16));
                    z.zone = zone;
                    z.area = area;
                    last = z.end;
                    out.push_back(z);
                }
                pc += 4 * (sub == 64 ? 6u : sub == 33 ? 4u : 3u);
                continue;
            }
            if (op == 20) zone = s32(word(pc + 4));
            if (op == 79) area = s32(word(pc + 4));
            pc += 4 * uint32_t(1 + n);
        }
    }
    return out;
}

bool EventRunner::execute(int alive)
{
    for (int guard = 0; guard < 10000; ++guard) {
        if (pc_ >= end_) return false;
        const uint32_t op = scripts_->word(pc_);
        const int n = operands(op);
        auto arg = [&](int k) { return scripts_->word(pc_ + 4 * uint32_t(k)); };
        if (n == -3) return false;   // the event's data: end of code
        if (n == -1) {               // list ended by -1
            uint32_t o = pc_ + 4;
            while (scripts_->word(o) != kEnd && o < end_ + 0x10000) {
                SpawnRecord r;
                if (op >= 9 && op <= 12 && scripts_->record(scripts_->word(o), r)) { r.path = path_; pending_.push_back(r); }
                o += 4;
            }
            pc_ = o + 4;
            continue;
        }
        if (n == -2) {               // CCTB: sub-operation 64 start end path mode = play a camera segment
            const uint32_t sub = arg(1);
            if (sub == 64) {
                const int s = s32(arg(2));
                path_ = s32(arg(4));
                if (s >= 0) frame_ = float(s);
                segEnd_ = s32(arg(3));
                camBusy_ = true;
            }
            pc_ += 4 * (sub == 64 ? 6u : sub == 33 ? 4u : 3u);
            continue;
        }
        switch (op) {
        case 20: zone_ = s32(arg(1)); break;                                            // BGTB: draw zone
        case 79: area_ = s32(arg(1)); break;                                            // TEXCHG: texture area
        case 83: if (camBusy_) return true; break;                                     // WAIT_CAMJOB
        case 84: if (camBusy_ && frame_ < float(s32(arg(1)))) return true; break;      // WAIT_CAMFRM
        case 80: case 85: if (alive > s32(arg(1))) return true; break;                 // WAIT_ENNUM / WAIT_ENALV
        case 81:                                                                        // WAIT_FRM
            if (waitFrames_ == 0) { waitFrames_ = s32(arg(1)) + 1; }
            if (--waitFrames_ > 0) return true;
            break;
        default: break;                                                                 // not interpreted yet
        }
        pc_ += 4 * uint32_t(1 + n);
    }
    return true;
}

void EventRunner::step(int alive)
{
    if (!running()) return;
    if (camBusy_) {
        frame_ += 1;
        if (frame_ >= float(segEnd_)) { frame_ = float(segEnd_); camBusy_ = false; }
    }
    for (size_t i = 0; i < pending_.size();)
        // at the record's camera frame on its path (or right away once the path changed: the frame can't come any more)
        if (pending_[i].path != path_ || float(pending_[i].frame) <= frame_) { released_.push_back(pending_[i]); pending_.erase(pending_.begin() + long(i)); }
        else ++i;
    while (!execute(alive)) {
        ++orderPos_;
        beginEvent();
        if (done_) return;
    }
}

std::vector<SpawnRecord> EventRunner::takeSpawns()
{
    std::vector<SpawnRecord> out;
    out.swap(released_);
    return out;
}

} // namespace hotd
