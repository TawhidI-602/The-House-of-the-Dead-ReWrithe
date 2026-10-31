// The whole-game differential's runtime side (diff_replay.h, docs/proof.md "Whole-game differential").
#include "game/diff_replay.h"

#include <cstring>
#include <fstream>
#include <sstream>

namespace hotd::game {

namespace {
std::vector<uint8_t> readAll(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
uint32_t rd32(const uint8_t* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }
void put32(std::vector<uint8_t>& out, uint32_t v)
{
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&v);
    out.insert(out.end(), p, p + 4);
}
} // namespace

bool DiffReplay::load(const std::string& dir, const std::string& stateMap, std::string& err)
{
    dir_ = dir;
    palette2 = readAll(dir + "/palette2.bin");
    const std::vector<uint8_t> d = readAll(dir + "/inputs.bin");
    if (d.size() < 8 || std::memcmp(d.data(), "HOTDINP1", 8) != 0) {
        err = dir + "/inputs.bin: missing or not HOTDINP1";
        return false;
    }
    size_t pos = 8;
    uint32_t first = 0;
    while (pos + 12 <= d.size()) {
        if (std::memcmp(d.data() + pos, "INPT", 4) != 0) {
            err = "inputs.bin: bad frame at byte " + std::to_string(pos);
            return false;
        }
        const uint32_t f = rd32(&d[pos + 4]), n = rd32(&d[pos + 8]);
        pos += 12;
        if (inputs_.empty())
            first = f;
        if (f != first + inputs_.size())
            break;   // (a gap: the record stops there)
        std::vector<Run> runs;
        bool cut = false;
        for (uint32_t k = 0; k < n; ++k) {
            if (pos + 8 > d.size()) { cut = true; break; }
            Run r;
            r.addr = rd32(&d[pos]);
            const uint32_t len = rd32(&d[pos + 4]);
            pos += 8;
            if (pos + len > d.size()) { cut = true; break; }
            r.bytes.assign(d.begin() + pos, d.begin() + pos + len);
            pos += len;
            runs.push_back(std::move(r));
        }
        std::vector<Mid> mids;
        if (!cut && pos + 4 <= d.size()) {
            const uint32_t m = rd32(&d[pos]);
            pos += 4;
            if (pos + size_t(m) * 16 > d.size())
                cut = true;
            else
                for (uint32_t k = 0; k < m; ++k, pos += 16)
                    mids.push_back({rd32(&d[pos]), rd32(&d[pos + 4]), rd32(&d[pos + 8]), rd32(&d[pos + 12])});
        } else
            cut = true;
        if (cut)
            break;
        inputs_.push_back(std::move(runs));
        mids_.push_back(std::move(mids));
    }
    if (inputs_.empty()) {
        err = "inputs.bin: no frames";
        return false;
    }
    start_ = first - 1;
    std::ifstream m(stateMap);
    std::string line;
    while (std::getline(m, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream ss(line);
        std::string name, a, s;
        if (std::getline(ss, name, '\t') && std::getline(ss, a, '\t') && std::getline(ss, s, '\t'))
            ranges_.emplace_back(uint32_t(std::stoul(a, nullptr, 16)), uint32_t(std::stoul(s, nullptr, 16)));
    }
    if (ranges_.empty()) {
        err = stateMap + ": no state map";
        return false;
    }
    out_ = std::fopen((dir + "/ours.rec").c_str(), "wb");
    if (!out_) {
        err = dir + "/ours.rec: cannot write";
        return false;
    }
    std::vector<uint8_t> head{'H', 'O', 'T', 'D', 'R', 'E', 'C', '1'};
    put32(head, uint32_t(ranges_.size()));
    for (const auto& [a, s] : ranges_) {
        put32(head, a);
        put32(head, s);
    }
    std::fwrite(head.data(), 1, head.size(), out_);
    return true;
}

DiffReplay::~DiffReplay()
{
    if (out_)
        std::fclose(out_);
}

void DiffReplay::apply(port::Mem& mem)
{
    tasks_.clear();
    seen_.clear();
    mem_ = &mem;
    midNext_ = 0;
    if (done_ >= inputs_.size())
        return;
    for (const Run& r : inputs_[done_])
        for (size_t k = 0; k < r.bytes.size(); ++k)
            mem.at<uint8_t>(r.addr + uint32_t(k)) = r.bytes[k];
    midUpTo(0);
}

void DiffReplay::midUpTo(uint32_t tasks)
{
    if (!mem_ || done_ >= mids_.size())
        return;
    const std::vector<Mid>& m = mids_[done_];
    const uint32_t listPtr = mem_->u32(0x511F04);   // texture_palette_upload_vblank's list (see onVblankRan)
    for (; midNext_ < m.size() && m[midNext_].tasks <= tasks; ++midNext_) {
        const Mid& w = m[midNext_];
        const uint32_t slot = w.addr & ~3u;
        const uint32_t before = slot >= 0x511FE0 && slot < 0x512060 ? mem_->u32(slot) : 0;
        for (uint32_t b = 0; b < 4; ++b)
            if ((w.mask >> (8 * b)) & 0xFF)
                mem_->at<uint8_t>(w.addr + b) = uint8_t(w.data >> (8 * b));
        if (before != 0 && mem_->u32(slot) == 0 && onVblankRan)
            onVblankRan(before, listPtr);
    }
}

void DiffReplay::record(port::Mem& mem)
{
    constexpr uint32_t kFrameCounter = 0x51F428, kGameMode = 0x51F494, kRand = 0x53E380, kMagic = 0x80018691;
    const uint32_t f = current();
    midUpTo(0xFFFFFFFFu);   // (those after the last task)
    std::vector<uint8_t> o{'F', 'R', 'A', 'M'};
    put32(o, f);
    put32(o, mem.u32(kFrameCounter));
    put32(o, mem.u32(kGameMode));
    put32(o, mem.u32(kRand));
    for (const auto& [a, s] : ranges_)
        for (uint32_t k = 0; k < s; ++k)
            o.push_back(mem.at<uint8_t>(a + k));
    std::vector<uint8_t> blocks;
    uint32_t nb = 0;
    for (uint32_t t : tasks_) {
        if (t < 0x500000 || t >= 0x600000 || mem.u32(t + 0xC) != kMagic)
            continue;
        uint32_t size = (mem.u32(t - 12) - 4) * 4;
        if (size < 0x10 || size > 0x2000)
            size = 0x10;
        put32(blocks, t);
        put32(blocks, size);
        for (uint32_t k = 0; k < size; ++k)
            blocks.push_back(mem.at<uint8_t>(t + k));
        ++nb;
    }
    put32(o, nb);
    o.insert(o.end(), blocks.begin(), blocks.end());
    std::fwrite(o.data(), 1, o.size(), out_);
    if (fullEvery > 0 && (f - start_) % uint32_t(fullEvery) == 0) {
        char name[64];
        std::snprintf(name, sizeof name, "/ours_full_%06u.bin", f);
        if (std::FILE* full = std::fopen((dir_ + name).c_str(), "wb")) {
            std::fwrite(mem.ptr(port::Mem::kRam), 1, port::Mem::kRamSize, full);   // (one region: contiguous)
            std::fclose(full);
        }
    }
    ++done_;
}

} // namespace hotd::game
