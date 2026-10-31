// hotd_proof: differential checks of the C++ port against recorded runs (docs/proof.md).
//   hotd_proof capture <folder> [regions]          routines: captures of tools\run_capture.ps1 replayed through the
//                                                 port (RAM after the routine and its coprocessor calls compared)
//   hotd_proof tgp <copro.txt> [copro_data.bin]   coprocessor: a full from-boot call trace (tools\run_copro_probe.ps1
//                                                 -Funcs all) replayed through arcade::Tgp, every output word compared
// Exit code 0 = no differences.
#include "port/game.h"
#include "port/tgp.h"
#include "proof/x86emu.h"
#include "util/file64.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <map>
#include <string>
#include <string>
#include <vector>

namespace {

std::vector<uint32_t> readWords(const char* path)
{
    std::vector<uint32_t> w;
    FILE* f = std::fopen(path, "rb");
    if (!f)
        return w;
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    w.resize(size_t(n) / 4);
    size_t got = std::fread(w.data(), 4, w.size(), f);
    w.resize(got);
    std::fclose(f);
    return w;
}

// tgp_trace.py match(): the model's words in order in the logged reads; extra logged words may only be 0 (reads of an
// empty FIFO that the i960 repeats); an unknown model word matches any one read
bool match(const hotd::port::Tgp::Out& o, const std::vector<uint32_t>& outs, int i, size_t j)
{
    if (i == o.n) {
        for (size_t k = j; k < outs.size(); ++k)
            if (outs[k] != 0)
                return false;
        return true;
    }
    if (j == outs.size())
        return false;
    bool unk = (o.unknown >> i) & 1;
    if ((unk || outs[j] == o.w[i]) && match(o, outs, i + 1, j + 1))
        return true;
    return outs[j] == 0 && match(o, outs, i, j + 1);
}

int tgp(const char* trace, const char* rom)
{
    std::vector<uint32_t> data = readWords(rom);
    if (data.empty()) {
        std::fprintf(stderr, "can't read %s\n", rom);
        return 2;
    }
    FILE* f = std::fopen(trace, "r");
    if (!f) {
        std::fprintf(stderr, "can't read %s\n", trace);
        return 2;
    }
    hotd::port::Tgp t(data.data(), data.size());
    struct Stat { long calls = 0, checked = 0, bad = 0, unk = 0; };
    std::map<int, Stat> stats;
    std::map<int, long> unmod;
    static char line[8192];
    long lineno = 0, shown = 0;
    std::vector<uint32_t> ins, outs;
    while (std::fgets(line, sizeof line, f)) {
        ++lineno;
        char* p = line;
        char* end;
        uint32_t cmd = uint32_t(std::strtoul(p, &end, 16));
        if (end == p)
            continue;
        p = end;
        char* pin = std::strstr(p, " in");
        char* pout = std::strstr(p, " out");
        if (!pin || !pout)
            continue;
        ins.clear();
        outs.clear();
        for (p = pin + 3; p < pout;) {
            uint32_t v = uint32_t(std::strtoul(p, &end, 16));
            if (end == p || end > pout)
                break;
            ins.push_back(v);
            p = end;
        }
        for (p = pout + 4;;) {
            uint32_t v = uint32_t(std::strtoul(p, &end, 16));
            if (end == p)
                break;
            outs.push_back(v);
            p = end;
        }
        int n = int(cmd & 0xFF);
        hotd::port::Tgp::Out o;
        if (!t.call(n, ins.data(), int(ins.size()), o)) {
            ++unmod[n];
            continue;
        }
        Stat& s = stats[n];
        ++s.calls;
        if (o.n == 0)
            continue;
        if (o.unknown) {
            ++s.unk;
            if (o.unknown == (1u << o.n) - 1)
                continue;
        }
        ++s.checked;
        if (!match(o, outs, 0, 0)) {
            ++s.bad;
            if (shown++ < 8) {
                std::printf("line %ld differs: %.160s   model", lineno, line);
                for (int i = 0; i < o.n; ++i)
                    std::printf(((o.unknown >> i) & 1) ? " ????????" : " %08x", o.w[i]);
                std::printf("\n");
            }
        }
    }
    std::fclose(f);
    long bad = 0, calls = 0, checked = 0;
    for (auto& [n, s] : stats) {
        std::printf("0x%02X: %8ld calls, %8ld with outputs checked, %ld differ", n, s.calls, s.checked, s.bad);
        if (s.unk)
            std::printf(", %ld with unknown words", s.unk);
        std::printf("\n");
        bad += s.bad;
        calls += s.calls;
        checked += s.checked;
    }
    for (auto& [n, c] : unmod)
        std::printf("not modelled: 0x%02X x %ld\n", n, c);
    std::printf("tgp: %ld calls, %ld checked, %ld differ\n", calls, checked, bad);
    return bad || !unmod.empty() ? 1 : 0;
}

// ---------------------------------------------------------------------------------------------------------------
// Routine captures (tools\run_capture.ps1): per call of a captured routine the work RAM and registers at entry and at
// its ret, and the coprocessor call count at both points; copro.txt is the full trace of the same run.

// a capture file mapped read-only (captures run to many GB: the pages load as read and stay evictable)
struct MappedFile {
    const uint8_t* p = nullptr;
    size_t n = 0;
    void* file = nullptr;
    void* map = nullptr;
#ifndef _WIN32
    explicit MappedFile(const std::string& path)
    {
        const int fd = ::open(path.c_str(), O_RDONLY);
        if (fd < 0)
            return;
        struct stat st{};
        if (::fstat(fd, &st) == 0 && st.st_size > 0) {
            void* m = ::mmap(nullptr, size_t(st.st_size), PROT_READ, MAP_PRIVATE, fd, 0);
            if (m != MAP_FAILED) {
                p = static_cast<const uint8_t*>(m);
                n = size_t(st.st_size);
            }
        }
        ::close(fd);   // the mapping stays
    }
    ~MappedFile()
    {
        if (p)
            ::munmap(const_cast<uint8_t*>(p), n);
    }
#else
    explicit MappedFile(const std::string& path)
    {
        file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                           FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            file = nullptr;
            return;
        }
        LARGE_INTEGER size;
        if (!GetFileSizeEx(file, &size) || size.QuadPart == 0)
            return;
        map = CreateFileMappingA(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!map)
            return;
        p = static_cast<const uint8_t*>(MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0));
        if (p)
            n = size_t(size.QuadPart);
    }
    ~MappedFile()
    {
        if (p)
            UnmapViewOfFile(p);
        if (map)
            CloseHandle(map);
        if (file)
            CloseHandle(file);
    }
#endif
    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;
    size_t size() const { return n; }
    const uint8_t* data() const { return p; }
    const uint8_t& operator[](size_t i) const { return p[i]; }
};

std::vector<uint8_t> readFile(const std::string& path)
{
    std::vector<uint8_t> b;
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f)
        return b;
    std::fseek(f, 0, SEEK_END);
    long long n = hotd::ftell64(f);
    std::fseek(f, 0, SEEK_SET);
    b.resize(size_t(n));
    size_t got = std::fread(b.data(), 1, b.size(), f);
    b.resize(got);
    std::fclose(f);
    return b;
}

struct Call {
    int n = 0;
    std::vector<uint32_t> ins, outs;
};

// copro.txt, one line per call, parsed on demand
struct Trace {
    std::vector<uint8_t> text;
    std::vector<size_t> line;
    bool load(const std::string& path)
    {
        text = readFile(path);
        if (text.empty())
            return false;
        text.push_back(0);
        line.push_back(0);
        for (size_t i = 0; i + 1 < text.size(); ++i)
            if (text[i] == '\n' && text[i + 1] != 0)
                line.push_back(i + 1);
        return true;
    }
    Call get(size_t k) const
    {
        Call c;
        const char* p = reinterpret_cast<const char*>(&text[line[k]]);
        char* end;
        c.n = int(std::strtoul(p, &end, 16) & 0xFF);
        const char* eol = std::strchr(p, '\n');
        const char* pin = std::strstr(p, " in");
        const char* pout = std::strstr(p, " out");
        if (!pin || !pout || (eol && pout > eol))
            return c;
        for (p = pin + 3; p < pout;) {
            uint32_t v = uint32_t(std::strtoul(p, &end, 16));
            if (end == p || end > pout)
                break;
            c.ins.push_back(v);
            p = end;
        }
        for (p = pout + 4; !eol || p < eol;) {
            uint32_t v = uint32_t(std::strtoul(p, &end, 16));
            if (end == p || (eol && end > eol))
                break;
            c.outs.push_back(v);
            p = end;
        }
        return c;
    }
};

using hotd::port::Mem;
using hotd::port::Tgp;

// The boundaries of both versions: routines not ported yet, replayed from the record (`boundary` of the version's
// proof class) at the version's address of that routine (port/globals.h F; 0 = the version has none).
template <class P> class ProofBase : public hotd::port::Game<P> {
public:
    using F = typename P::F;
    using hotd::port::Game<P>::Game;
    std::string error;
    // proofs-8: a case that returns a value sets it here; compared with the original's g0 at its return
    bool hasResult = false;
    uint32_t result = 0;
    void setResult(uint32_t v) { hasResult = true; result = v; }
    // one argument of a boundary call, as the port passes it (checked against the original's where recorded)
    struct Arg {
        uint32_t v;
        bool f;
        Arg(uint32_t x) : v(x), f(false) {}
        Arg(int32_t x) : v(uint32_t(x)), f(false) {}
        Arg(float x) : v(0), f(true) { std::memcpy(&v, &x, 4); }
    };
    virtual uint32_t boundary(uint32_t routine, std::initializer_list<Arg> args) = 0;
    // true when the capture's next recorded boundary call is `routine`: a ported routine that an older capture recorded
    // as a boundary (its callees were not recorded) is then replayed from the record instead of run (bounds/*.inc:
    // `if (recorded_next(F::x)) boundary(...); else Game<P>::x(...)`); in a capture that lists it in -Funcs / -Ported
    // it is never a recorded boundary, so the port runs and is checked
    virtual bool recorded_next(uint32_t routine) const = 0;
    // where the port's locals passed by address go (Game::local_alloc); arguments pointing there are not compared
    static constexpr uint32_t kLocalsSize = 0x10000;
    uint32_t localBase = 0;
    // a boundary whose result is a float (x87 ST0 on the PC)
    virtual float boundary_float(uint32_t routine, std::initializer_list<Arg> args)
    {
        boundary(routine, args);
        if (error.empty())
            error = "a float boundary result is not recorded for this version";
        return 0.0f;
    }

    // a boundary whose arguments are not compared (cdecl / variadic on the PC)
    void boundary_noargs(uint32_t routine) { skipArgs = true; boundary(routine, {}); skipArgs = false; }
    bool skipArgs = false;
    static constexpr uint32_t kTimeGetTime = 0xFFFFFF01;   // the hook's id of a timeGetTime call (hotd_hook.c)
    static float bitsf(uint32_t v) { float f; std::memcpy(&f, &v, 4); return f; }
    // the boundary overrides, one file per system (docs/port.md "Adding a kind")
#include "proof/bounds/core.inc"
#include "proof/bounds/molb.inc"
#include "proof/bounds/scene_zombie.inc"
#include "proof/bounds/researchers.inc"
#include "proof/bounds/kit_zombie.inc"
#include "proof/bounds/bats.inc"
#include "proof/bounds/dogs.inc"
#include "proof/bounds/frog.inc"
#include "proof/bounds/agile.inc"
#include "proof/bounds/event.inc"
#include "proof/bounds/camera.inc"
#include "proof/bounds/creature.inc"
#include "proof/bounds/player.inc"
#include "proof/bounds/walkers.inc"
#include "proof/bounds/scene_objects.inc"
#include "proof/bounds/props.inc"
#include "proof/bounds/chariot.inc"
#include "proof/bounds/sophie.inc"
#include "proof/bounds/ending.inc"
};

// The arcade port, with every coprocessor call checked against the trace (function, inputs, outputs) in order.
class ProofArcade;
bool runRoutine(ProofArcade& a, uint32_t routine, const uint32_t* regs);

class ProofArcade : public ProofBase<hotd::port::A> {
public:
    // a routine not ported yet, called inside the capture: its coprocessor calls and the RAM bytes it changed
    struct Boundary {
        uint32_t routine = 0, cin = 0, cout = 0, result = 0;
        uint32_t args[8] = {};   // g0..g7 at its entry ("BND3" records)
        bool hasArgs = false;
        const uint8_t* runs = nullptr;
        uint32_t nruns = 0;
    };
    ProofArcade(Mem& m, Tgp& t, const Trace& tr, size_t pos) : ProofBase(m, t), trace(tr), pos(pos) {}
    uint32_t captureFp = 0;
    uint32_t orig_fp() override { return captureFp; }

    // a boundary's float result: g0's bits on the arcade
    float boundary_float(uint32_t routine, std::initializer_list<Arg> args) override
    {
        const uint32_t r = boundary(routine, args);
        float v;
        std::memcpy(&v, &r, 4);
        return v;
    }

    // after a replayed boundary: its output through the pointer argument `arg` (a stack local of the original) copied
    // to the port's own local at `to`
    void relocate(int arg, uint32_t to, uint32_t size) override
    {
        if (nextBoundary == 0 || !error.empty())
            return;
        const Boundary& b = boundaries[nextBoundary - 1];
        if (!b.hasArgs) {
            error = "a boundary with an output argument needs a capture with arguments (BND3)";
            return;
        }
        std::memmove(mem.ptr(to), mem.ptr(b.args[arg]), size);
    }
    const Trace& trace;
    size_t pos;
    std::vector<Boundary> boundaries;
    size_t nextBoundary = 0;
    bool recorded_next(uint32_t routine) const override
    {
        return nextBoundary < boundaries.size() && boundaries[nextBoundary].routine == routine;
    }
    // (proofs-5) a routine called through its address: a boundary when the capture recorded it as one, else the
    // port's when it has a proof case (player_camdr_run -> player_body_camdr 0x62C0)
    void call_routine(uint32_t routine, uint32_t arg) override
    {
        if (!recorded_next(routine)) {
            uint32_t regs[18] = {arg};
            if (runRoutine(*this, routine, regs))
                return;
        }
        boundary(routine, {arg});
    }

    // replays the next recorded boundary call of this routine: its coprocessor calls (state only) and its RAM effect
    uint32_t boundary(uint32_t routine, std::initializer_list<Arg>) override   // arguments: not recorded on the arcade yet
    {
        char buf[160];
        if (nextBoundary >= boundaries.size() || boundaries[nextBoundary].routine != routine) {
            std::snprintf(buf, sizeof buf, "port called boundary 0x%X that the original did not call here (it called 0x%X)", routine,
                          nextBoundary < boundaries.size() ? boundaries[nextBoundary].routine : 0u);
            if (error.empty())
                error = buf;
            return 0;
        }
        const Boundary& b = boundaries[nextBoundary++];
        if (pos != b.cin && error.empty()) {
            std::snprintf(buf, sizeof buf, "boundary 0x%X called after %zu coprocessor calls, the original after %u",
                          routine, pos, b.cin);
            error = buf;
        }
        Tgp::Out o;
        for (; pos < b.cout; ++pos) {
            Call c = trace.get(pos);
            tgp.call(c.n, c.ins.data(), int(c.ins.size()), o);
        }
        const uint8_t* r = b.runs;
        for (uint32_t i = 0; i < b.nruns; ++i) {
            uint32_t addr, len;
            std::memcpy(&addr, r, 4);
            std::memcpy(&len, r + 4, 4);
            std::memcpy(mem.ptr(addr), r + 8, len);
            r += 8 + len;
        }
        return b.result;
    }

    void copro(int n, const uint32_t* in, int count, Tgp::Out& out) override
    {
        tgp.call(n, in, count, out);
        if (!error.empty())
            return;
        if (pos >= trace.line.size()) {
            error = "coprocessor call past the end of the trace";
            return;
        }
        Call c = trace.get(pos++);
        // a data word shaped like a command word (k << 23 | k << 8 | k: e.g. the float 0x42008484) makes the probe
        // log the rest of the call as a new call k: joined again here
        if (c.n == n && int(c.ins.size()) == count - 1 && count > 0 && pos < trace.line.size()) {
            const uint32_t w = in[count - 1], k = w & 0xFF;
            const Call d = trace.get(pos);
            if (w == (k << 23 | k << 8 | k) && d.n == int(k) && d.ins.empty()) {
                c.ins.push_back(w);
                c.outs = d.outs;
                ++pos;
            }
        }
        bool same = c.n == n && int(c.ins.size()) == count;
        for (int i = 0; same && i < count; ++i)
            same = c.ins[size_t(i)] == in[i];
        char buf[200];
        if (!same) {
            int k = 0;
            while (k < count && size_t(k) < c.ins.size() && c.ins[size_t(k)] == in[k])
                ++k;
            std::snprintf(buf, sizeof buf, "coprocessor call %zu: port made 0x%02X (%d words), trace has 0x%02X (%zu words); "
                          "word %d: port %08X, trace %08X", pos - 1, n, count, c.n, c.ins.size(), k,
                          k < count ? in[k] : 0u, size_t(k) < c.ins.size() ? c.ins[size_t(k)] : 0u);
            error = buf;
        } else if (out.n && !match(out, c.outs, 0, 0)) {
            std::snprintf(buf, sizeof buf, "coprocessor call %zu (0x%02X): outputs differ", pos - 1, n);
            error = buf;
        } else if (out.unknown) {
            // words the model cannot know (MA0 after an untracked function, e.g. the angles of a zero vector): the
            // port goes on with the chip's recorded value
            size_t j = 0;
            for (int i = 0; i < out.n; ++i) {
                while (!((out.unknown >> i) & 1) && j < c.outs.size() && c.outs[j] != out.w[i])
                    ++j;
                if (((out.unknown >> i) & 1) && j < c.outs.size())
                    out.w[i] = c.outs[j];
                ++j;
            }
            out.unknown = 0;
        }
    }
};

bool runRoutine(ProofArcade& a, uint32_t routine, const uint32_t* regs)
{
    auto f = [&](int i) { float v; std::memcpy(&v, &regs[i], 4); return v; };
    switch (routine) {
#include "proof/cases/core_arcade.inc"
#include "proof/cases/molb_arcade.inc"
#include "proof/cases/scene_zombie_arcade.inc"
#include "proof/cases/researchers_arcade.inc"
#include "proof/cases/kit_zombie_arcade.inc"
#include "proof/cases/bats_arcade.inc"
#include "proof/cases/dogs_arcade.inc"
#include "proof/cases/frog_arcade.inc"
#include "proof/cases/agile_arcade.inc"
#include "proof/cases/event_arcade.inc"
#include "proof/cases/camera_arcade.inc"
#include "proof/cases/creature_arcade.inc"
#include "proof/cases/player_arcade.inc"
#include "proof/cases/walkers_arcade.inc"
#include "proof/cases/scene_objects_arcade.inc"
#include "proof/cases/props_arcade.inc"
#include "proof/cases/chariot_arcade.inc"
#include "proof/cases/hangedman_fight_arcade.inc"
#include "proof/cases/sophie_arcade.inc"
#include "proof/cases/support_arcade.inc"
#include "proof/cases/support_b_arcade.inc"
#include "proof/cases/player_body_arcade.inc"
#include "proof/cases/ch2_kinds_arcade.inc"
#include "proof/cases/props_ch2_arcade.inc"
#include "proof/cases/hostages_ch2_arcade.inc"
#include "proof/cases/ch3_kinds_arcade.inc"
#include "proof/cases/props_ch3_arcade.inc"
#include "proof/cases/hostages_ch3_arcade.inc"
#include "proof/cases/hermit_arcade.inc"
#include "proof/cases/ch4_kinds_arcade.inc"
#include "proof/cases/magician_arcade.inc"
#include "proof/cases/ending_arcade.inc"
#include "proof/cases/tasks_arcade.inc"
    default: return false;
    }
}

int capture(const std::string& dir, const std::string& regions)
{
    const MappedFile cap(dir + "/capture.bin");
    Trace trace;
    std::vector<uint32_t> data = readWords((regions + "/copro_data.bin").c_str());
    static Mem mem;
    const std::vector<uint8_t> rom = readFile(regions + "/maincpu.bin");
    mem.load(0, rom);
    // main_data (motion frames, skin templates: the creature core reads them) at 0x2000000, its upper half again at
    // 0x6000000 (the motion data pointers point there), as the runtime maps them (game/host.cpp)
    if (const std::vector<uint8_t> md = readFile(regions + "/main_data.bin"); md.size() > 0x1000000) {
        mem.load(0x2000000, md);
        mem.load(0x6000000, std::vector<uint8_t>(md.begin() + 0x1000000, md.end()));
    }
    uint8_t* const ram = mem.map(Mem::kRam, Mem::kRamSize);
    if (cap.size() < 8 || std::memcmp(cap.data(), "HOTDCAP1", 8) != 0 || !trace.load(dir + "/copro.txt") ||
        data.empty() || rom.empty()) {
        std::fprintf(stderr, "can't read the capture, trace or ROM regions\n");
        return 2;
    }
    const size_t kRegs = 18 * 4, kRam = Mem::kRamSize;
    Tgp ref(data.data(), data.size());   // the coprocessor along the trace
    size_t refPos = 0;
    size_t p = 8;
    int total = 0, failed = 0, unsupported = 0;
    long coproCalls = 0, changedBytes = 0, boundaryCalls = 0;
    std::map<uint32_t, long> boundaryCount, perRoutine, failedPer;
    while (p + 16 + kRegs + kRam <= cap.size() && std::memcmp(&cap[p], "CAPT", 4) == 0) {
        uint32_t routine, seq, cin;
        std::memcpy(&routine, &cap[p + 4], 4);
        std::memcpy(&seq, &cap[p + 8], 4);
        std::memcpy(&cin, &cap[p + 12], 4);
        const uint32_t* regsIn = reinterpret_cast<const uint32_t*>(&cap[p + 16]);
        const uint8_t* ramIn = &cap[p + 16 + kRegs];
        p += 16 + kRegs + kRam;
        // "XRAM" (proofs-4): memory outside work RAM at entry (u32 address, u32 length, bytes; run_capture.ps1 -XRam)
        std::vector<std::pair<uint32_t, std::pair<uint32_t, const uint8_t*>>> xrams;
        while (p + 12 <= cap.size() && std::memcmp(&cap[p], "XRAM", 4) == 0) {
            uint32_t xa, xl;
            std::memcpy(&xa, &cap[p + 4], 4);
            std::memcpy(&xl, &cap[p + 8], 4);
            xrams.push_back({xa, {xl, &cap[p + 12]}});
            p += 12 + xl;
        }
        std::vector<ProofArcade::Boundary> bounds;
        // "BNDY" (older captures), "BND2" (with the boundary's result g0) or "BND3" (also its arguments g0..g7)
        while (p + 20 <= cap.size() && (std::memcmp(&cap[p], "BNDY", 4) == 0 || std::memcmp(&cap[p], "BND2", 4) == 0 ||
                                         std::memcmp(&cap[p], "BND3", 4) == 0)) {
            ProofArcade::Boundary b;
            const bool v2 = cap[p + 3] == '2' || cap[p + 3] == '3', v3 = cap[p + 3] == '3';
            std::memcpy(&b.routine, &cap[p + 4], 4);
            std::memcpy(&b.cin, &cap[p + 8], 4);
            std::memcpy(&b.cout, &cap[p + 12], 4);
            if (v2)
                std::memcpy(&b.result, &cap[p + 16], 4);
            if (v3)
                std::memcpy(b.args, &cap[p + 20], 32);
            b.hasArgs = v3;
            const size_t head = v3 ? 56 : v2 ? 24 : 20;
            std::memcpy(&b.nruns, &cap[p + head - 4], 4);
            p += head;
            b.runs = &cap[p];
            for (uint32_t i = 0; i < b.nruns; ++i) {
                uint32_t len;
                std::memcpy(&len, &cap[p + 4], 4);
                p += 8 + len;
            }
            bounds.push_back(b);
        }
        if (p + 8 + kRegs + kRam > cap.size() || std::memcmp(&cap[p], "CEXT", 4) != 0) {
            std::printf("capture %x #%u: no exit record\n", routine, seq);
            break;
        }
        uint32_t cout;
        std::memcpy(&cout, &cap[p + 4], 4);
        const uint8_t* ramOut = &cap[p + 8 + kRegs];
        p += 8 + kRegs + kRam;
        // "XOUT" (proofs-5): the -XRam ranges at exit, compared with the port's below
        std::vector<std::pair<uint32_t, std::pair<uint32_t, const uint8_t*>>> xouts;
        while (p + 12 <= cap.size() && std::memcmp(&cap[p], "XOUT", 4) == 0) {
            uint32_t xa, xl;
            std::memcpy(&xa, &cap[p + 4], 4);
            std::memcpy(&xl, &cap[p + 8], 4);
            xouts.push_back({xa, {xl, &cap[p + 12]}});
            p += 12 + xl;
        }

        Tgp::Out o;
        for (; refPos < cin && refPos < trace.line.size(); ++refPos) {
            Call c = trace.get(refPos);
            ref.call(c.n, c.ins.data(), int(c.ins.size()), o);
        }
        std::memcpy(ram, ramIn, kRam);
        for (const auto& x : xrams) {
            if (!mem.mapped(x.first) || !mem.mapped(x.first + x.second.first - 1))
                mem.map(x.first, x.second.first);
            for (uint32_t i = 0; i < x.second.first; ++i)
                *mem.ptr(x.first + i) = x.second.second[i];
        }
        mem.bad = false;
        Tgp tgp = ref;
        ProofArcade a(mem, tgp, trace, cin);
        a.boundaries = bounds;
        boundaryCalls += long(bounds.size());
        for (const auto& b : bounds)
            ++boundaryCount[b.routine];
        // the port's locals passed by address go high in the routine's stack window (not compared, below)
        a.localTop = regsIn[15] + 0x6000;
        a.captureFp = regsIn[15];
        if (!runRoutine(a, routine, regsIn)) {
            ++unsupported;
            continue;
        }
        ++total;
        ++perRoutine[routine];
        // the routine's own frame and its callees' frames (the i960 stack grows upward from fp) are not compared
        uint32_t fp = regsIn[15];
        size_t diffs = 0, first = 0, last = 0;
        // HOTD_PROOF_VBLANK=1 (chstart): the work RAM a vblank interrupt writes is not compared, for routines that
        // busy-wait for the vblank (chapter_load 0x25DA0 waits for 0x51F706, so the interrupt always runs inside its
        // capture; any short routine can be hit by one too): the interrupt's frames 0x510430..0x5105FF, the once-list
        // 0x511F00.. (its callbacks run and are cleared), 0x517CA0.., the palette / colour row queues 0x51DCB0..0x51E18F
        // (uploaded and emptied), coin_update's prompt records and counters 0x51E820..0x51E8EF, 0x51F560, 0x51F790..,
        // and vblank_words_upload's colours 0x520660.. (texture_palette_upload_vblank)
        static const bool skipVblank = std::getenv("HOTD_PROOF_VBLANK") != nullptr;
        auto vblankOwned = [](uint32_t a) {
            return (a >= 0x510430 && a < 0x510600) || (a >= 0x511F00 && a < 0x512000) || (a >= 0x517CA0 && a < 0x517CB0) ||
                   (a >= 0x51DCB0 && a < 0x51E190) || (a >= 0x51E820 && a < 0x51E8F0) ||
                   (a >= 0x51F560 && a < 0x51F564) || (a >= 0x51F790 && a < 0x51F7A0) || (a >= 0x520660 && a < 0x5206E4);
        };
        size_t vblankBytes = 0;
        for (size_t i = 0; i < kRam; ++i) {
            uint32_t addr = Mem::kRam + uint32_t(i);
            if (addr >= fp && addr < fp + 0x8000)
                continue;
            if (skipVblank && ram[i] != ramOut[i] && vblankOwned(addr)) {
                ++vblankBytes;
                continue;
            }
            if (ram[i] != ramOut[i]) {
                if (!diffs)
                    first = addr;
                last = addr;
                ++diffs;
            }
        }
        if (vblankBytes)
            std::printf("capture %x #%u: %zu vblank-owned bytes not compared (HOTD_PROOF_VBLANK)\n", routine, seq, vblankBytes);
        static const bool showRange = std::getenv("HOTD_PROOF_RANGE") != nullptr;   // debugging: the differing range
        if (showRange && diffs) {
            std::printf("capture %x #%u: differing bytes 0x%zX..0x%zX, fp 0x%X:", routine, seq, first, last, fp);
            for (size_t i = 0, prev = 0; i < kRam; ++i) {   // the differing runs (16-byte granules)
                const uint32_t addr = Mem::kRam + uint32_t(i);
                if (addr >= fp && addr < fp + 0x8000)
                    continue;
                if (ram[i] != ramOut[i] && (prev == 0 || (addr & ~0xFu) != prev)) {
                    prev = addr & ~0xFu;
                    std::printf(" %X", unsigned(prev));
                }
            }
            std::printf("\n");
        }
        static const bool showBytes = std::getenv("HOTD_PROOF_BYTES") != nullptr;   // debugging: every differing byte
        if (showBytes && diffs)
            for (size_t i = 0; i < kRam; ++i) {
                const uint32_t addr = Mem::kRam + uint32_t(i);
                if (!(addr >= fp && addr < fp + 0x8000) && ram[i] != ramOut[i])
                    std::printf("  byte %X: entry %02x port %02x original %02x\n", unsigned(addr), ramIn[i], ram[i], ramOut[i]);
            }
        for (const auto& x : xouts)
            for (uint32_t i = 0; i < x.second.first; ++i)
                if (mem.mapped(x.first + i) && *mem.ptr(x.first + i) != x.second.second[i]) {
                    if (!diffs)
                        first = x.first + i;
                    ++diffs;
                }
        coproCalls += long(a.pos - cin);
        for (size_t i = 0; i < kRam; ++i)
            changedBytes += ramIn[i] != ramOut[i];
        if (a.error.empty() && mem.bad) {
            char buf[120];
            std::snprintf(buf, sizeof buf, "the port accessed unmapped address 0x%X", mem.badAddr);
            a.error = buf;
        }
        if (a.error.empty() && a.nextBoundary != a.boundaries.size()) {
            char nb[120];
            std::snprintf(nb, sizeof nb, "the original called a boundary routine the port did not call (0x%X)",
                          a.boundaries[a.nextBoundary].routine);
            a.error = nb;
        }
        if (a.error.empty() && a.hasResult) {
            uint32_t g0;
            std::memcpy(&g0, ramOut - kRegs, 4);
            if (g0 != a.result) {
                char buf[120];
                std::snprintf(buf, sizeof buf, "returned 0x%X, the original 0x%X", a.result, g0);
                a.error = buf;
            }
        }
        if (a.error.empty() && a.pos != cout) {
            char buf[120];
            std::snprintf(buf, sizeof buf, "made %zu coprocessor calls, the original %u", a.pos - cin, cout - cin);
            a.error = buf;
        }
        if (diffs || !a.error.empty()) {
            ++failed;
            ++failedPer[routine];
            if (failedPer[routine] <= 3) {   // the first three of each routine
                std::printf("capture %x #%u: %zu bytes differ (first 0x%zX)%s%s%s\n", routine, seq, diffs, first,
                            a.error.empty() ? "" : "; ", a.error.c_str(), "");
                const uint32_t objOff = regsIn[0] - Mem::kRam;   // g0 = the object for the enemy routines
                if (objOff + 0x3C8 <= kRam) {
                    int16_t st, sub, act;
                    std::memcpy(&st, &ramIn[objOff + 0x5C], 2);
                    std::memcpy(&sub, &ramIn[objOff + 0x60], 2);
                    std::memcpy(&act, &ramIn[objOff + 0x64], 2);
                    std::printf("    object 0x%X at entry: state %d, sub-step %d, action %d\n", regsIn[0], st, sub, act);
                }
                for (size_t i = first - Mem::kRam, shown = 0; diffs && i < kRam && shown < 6; ++i)
                    if (ram[i] != ramOut[i] && !(Mem::kRam + i >= fp && Mem::kRam + i < fp + 0x8000)) {
                        std::printf("    0x%zX: port %02x original %02x\n", Mem::kRam + i, ram[i], ramOut[i]);
                        ++shown;
                    }
            }
        }
    }
    std::printf("captures: %d checked (%ld coprocessor calls, %ld bytes changed by the original, %ld boundary calls "
                "replayed), %d differ%s\n", total, coproCalls, changedBytes, boundaryCalls, failed, unsupported ? " (some routines not ported: skipped)" : "");
    std::printf("per routine:");
    for (auto& [r, n] : perRoutine)
        std::printf(" %X:%ld", r, n);
    std::printf("\n");
    if (!failedPer.empty()) {
        std::printf("differ per routine:");
        for (auto& [r, n] : failedPer)
            std::printf(" %X:%ld", r, n);
        std::printf("\n");
    }
    if (!boundaryCount.empty()) {
        std::printf("boundary calls:");
        for (auto& [r, n] : boundaryCount)
            std::printf(" %X:%ld", r, n);
        std::printf("\n");
    }
    return failed ? 1 : 0;
}

// ---------------------------------------------------------------------------------------------------------------
// PC routine captures (tools\pcharness\run_pc.ps1 -CapFuncs, hotd_hook.c): the state at entry as byte runs against a
// shadow image kept along the file, then the routine's own changes ("PRE "), each boundary call's effect ("BND1") and
// the rest ("CEXT"); "DROP" = the routine never returned. The port runs on the entry image, boundaries replayed in
// order, and its memory is compared with the expected image (everything but the stack below the return's esp).

using hotd::port::Pc;
constexpr uint32_t kPcLocals = 0x7F000000;   // an address range the PC game never uses

class ProofPc : public ProofBase<Pc> {
public:
    struct Boundary {
        uint32_t routine = 0, result = 0;
        const uint8_t* runs = nullptr;
        uint32_t nruns = 0;
        uint32_t regs[9] = {};    // at its entry: eax ecx edx ebx esp ebp esi edi eflags
        uint32_t stack[8] = {};   // the words from esp + 4 at its entry (stack arguments)
        const uint8_t* fpu = nullptr;   // the x87 state at its return (FNSAVE: ST0 at +28, 80 bits)
    };
    float lastFloat = 0.0f;
    // ST0 of an FNSAVE image as a float (80-bit extended -> double -> float)
    static float st0(const uint8_t* f)
    {
        uint64_t mant;
        uint16_t se;
        std::memcpy(&mant, f + 28, 8);
        std::memcpy(&se, f + 36, 2);
        const int e = se & 0x7FFF;
        double v;
        if (e == 0 && mant == 0)
            v = 0.0;
        else if (e == 0x7FFF)
            v = (mant << 1) ? std::nan("") : HUGE_VAL;
        else
            v = std::ldexp(double(mant), e - 16383 - 63);
        return float((se & 0x8000) ? -v : v);
    }
    float boundary_float(uint32_t routine, std::initializer_list<Arg> args) override
    {
        const size_t k = nextBoundary;
        boundary(routine, args);
        return k < boundaries.size() && boundaries[k].fpu ? st0(boundaries[k].fpu) : 0.0f;
    }
    ProofPc(Mem& m, Tgp& t) : ProofBase(m, t) {}
    std::vector<uint32_t> lastArgs;   // the original's arguments of the last boundary call (by the port's argument order)
    void relocate(int arg, uint32_t to, uint32_t size) override
    {
        if (!error.empty())
            return;
        if (size_t(arg) >= lastArgs.size()) {
            error = "relocate: no such boundary argument";
            return;
        }
        std::memmove(mem.ptr(to), mem.ptr(lastArgs[size_t(arg)]), size);
    }
    std::vector<Boundary> boundaries;
    size_t nextBoundary = 0;
    bool recorded_next(uint32_t routine) const override
    {
        return nextBoundary < boundaries.size() && boundaries[nextBoundary].routine == routine;
    }
    static void apply(Mem& m, const uint8_t* r, uint32_t n)
    {
        for (uint32_t i = 0; i < n; ++i) {
            uint32_t addr, len;
            std::memcpy(&addr, r, 4);
            std::memcpy(&len, r + 4, 4);
            for (uint32_t k = 0; k < len; ++k)
                *m.ptr(addr + k) = r[8 + k];
            r += 8 + len;
        }
    }
    // the Watcom register convention: the first four integer arguments in eax, edx, ebx, ecx, until one has to go on
    // the stack (a float, or a fifth integer): from there on every argument is on the stack from esp + 4, in order
    uint32_t boundary(uint32_t routine, std::initializer_list<Arg> args) override
    {
        if (!skipArgs && nextBoundary < boundaries.size() && boundaries[nextBoundary].routine == routine && error.empty()) {
            const Boundary& b = boundaries[nextBoundary];
            static const int kReg[4] = {0, 2, 3, 1};
            int nreg = 0, nstack = 0, i = 0;
            for (const Arg& x : args) {
                uint32_t orig = 0;
                if (!x.f && nreg < 4 && nstack == 0)
                    orig = b.regs[kReg[nreg++]];
                else if (nstack < 8)
                    orig = b.stack[nstack++];
                if (lastArgs.size() <= size_t(i))
                    lastArgs.resize(size_t(i) + 1);
                lastArgs[size_t(i)] = orig;
                // a pointer to one of the port's own locals: its original was a stack local somewhere else
                const bool local = localBase && x.v >= localBase && x.v < localBase + kLocalsSize;
                if (orig != x.v && !local) {
                    char buf[200];
                    std::snprintf(buf, sizeof buf, "boundary 0x%X argument %d: port %08X, original %08X", routine, i,
                                  x.v, orig);
                    error = buf;
                    break;
                }
                ++i;
            }
        }
        if (nextBoundary >= boundaries.size() || boundaries[nextBoundary].routine != routine) {
            char buf[160];
            std::snprintf(buf, sizeof buf, "port called boundary 0x%X that the original did not call here (it called 0x%X)", routine,
                          nextBoundary < boundaries.size() ? boundaries[nextBoundary].routine : 0u);
            if (error.empty())
                error = buf;
            return 0;
        }
        const Boundary& b = boundaries[nextBoundary++];
        apply(mem, b.runs, b.nruns);
        return b.result;
    }
};

// regs: eax ecx edx ebx esp ebp esi edi eflags at entry (Watcom register convention: arguments in eax, edx, ebx,
// ecx, the rest and floats on the stack from esp + 4). Returns false for a routine not ported for the PC.
bool runRoutinePc(ProofPc& a, uint32_t routine, const uint32_t* regs)
{
    using F = Pc::F;
    switch (routine) {
#include "proof/cases/core_pc.inc"
#include "proof/cases/molb_pc.inc"
#include "proof/cases/scene_zombie_pc.inc"
#include "proof/cases/researchers_pc.inc"
#include "proof/cases/kit_zombie_pc.inc"
#include "proof/cases/bats_pc.inc"
#include "proof/cases/dogs_pc.inc"
#include "proof/cases/frog_pc.inc"
#include "proof/cases/agile_pc.inc"
#include "proof/cases/event_pc.inc"
#include "proof/cases/camera_pc.inc"
#include "proof/cases/creature_pc.inc"
#include "proof/cases/player_pc.inc"
#include "proof/cases/walkers_pc.inc"
#include "proof/cases/scene_objects_pc.inc"
#include "proof/cases/props_pc.inc"
#include "proof/cases/chariot_pc.inc"
#include "proof/cases/hangedman_fight_pc.inc"
#include "proof/cases/sophie_pc.inc"
    default: return false;
    }
}

int pccapture(const std::string& dir, const std::string& exePath)
{
    const MappedFile cap(dir + "/capture.bin");
    if (cap.size() < 8 || std::memcmp(cap.data(), "HOTDPCC1", 8) != 0) {
        std::fprintf(stderr, "can't read %s/capture.bin\n", dir.c_str());
        return 2;
    }
    // records: tag, a header by tag, then u32 run count and the runs
    struct Rec {
        char tag[5] = {};
        const uint8_t* head = nullptr;
        const uint8_t* runs = nullptr;
        uint32_t nruns = 0;
    };
    std::vector<Rec> recs;
    std::vector<uint32_t> pages;
    for (size_t p = 8; p + 8 <= cap.size();) {
        Rec r;
        std::memcpy(r.tag, &cap[p], 4);
        if (!std::memcmp(r.tag, "MAPS", 4)) {   // committed ranges: mapped, not applied
            uint32_t n;
            std::memcpy(&n, &cap[p + 4], 4);
            for (uint32_t i = 0; i < n; ++i) {
                uint32_t base, size;
                std::memcpy(&base, &cap[p + 8 + i * 8], 4);
                std::memcpy(&size, &cap[p + 12 + i * 8], 4);
                for (uint32_t q = base >> 12; q < (base + size + 0xFFF) >> 12; ++q)
                    pages.push_back(q);
            }
            p += 8 + size_t(n) * 8;
            continue;
        }
        const size_t hs = !std::memcmp(r.tag, "CAPT", 4)   ? 16 + 36 + 112
                          : !std::memcmp(r.tag, "BND1", 4) ? 4 + 36 + 4 + 112
                          : !std::memcmp(r.tag, "CEXT", 4) ? 36 + 112
                                                           : 0;
        if (p + 4 + hs + 4 > cap.size())
            break;   // a record still being written
        r.head = &cap[p + 4];
        p += 4 + hs;
        std::memcpy(&r.nruns, &cap[p], 4);
        p += 4;
        r.runs = &cap[p];
        bool whole = true;
        std::vector<uint32_t> recPages;
        for (uint32_t i = 0; i < r.nruns; ++i) {
            if (p + 8 > cap.size()) {
                whole = false;
                break;
            }
            uint32_t addr, len;
            std::memcpy(&addr, &cap[p], 4);
            std::memcpy(&len, &cap[p + 4], 4);
            if (p + 8 + len > cap.size()) {
                whole = false;
                break;
            }
            for (uint32_t q = addr >> 12; q <= (addr + len - 1) >> 12; ++q)
                recPages.push_back(q);
            p += 8 + len;
        }
        if (!whole)
            break;
        pages.insert(pages.end(), recPages.begin(), recPages.end());
        recs.push_back(r);
    }
    // the same regions in the expected image and the port's: runs of recorded pages (gaps up to 64 KB joined)
    std::sort(pages.begin(), pages.end());
    pages.erase(std::unique(pages.begin(), pages.end()), pages.end());
    static Mem exp, work;
    // the EXE's code section holds the read-only tables (e.g. part_roles 0x523A58): loaded from the game copy
    const std::vector<uint8_t> exe = readFile(exePath);
    if (exe.size() < 0x1000) {
        std::fprintf(stderr, "can't read %s\n", exePath.c_str());
        return 2;
    }
    {
        uint32_t pe, n, opt;
        std::memcpy(&pe, &exe[0x3C], 4);
        n = uint32_t(exe[pe + 6] | (exe[pe + 7] << 8));
        opt = uint32_t(exe[pe + 20] | (exe[pe + 21] << 8));
        for (uint32_t i = 0; i < n; ++i) {
            const uint8_t* sh = &exe[pe + 24 + opt + 40 * i];
            uint32_t va, rs, rp, ch;
            std::memcpy(&va, sh + 12, 4);
            std::memcpy(&rs, sh + 16, 4);
            std::memcpy(&rp, sh + 20, 4);
            std::memcpy(&ch, sh + 36, 4);
            if ((ch & 0x20000000) && !(ch & 0x80000000) && rp + rs <= exe.size()) {   // executable, not writable
                const std::vector<uint8_t> sec(exe.begin() + rp, exe.begin() + rp + rs);
                exp.load(0x400000 + va, sec);
                work.load(0x400000 + va, sec);
            }
        }
    }
    for (size_t i = 0; i < pages.size();) {
        size_t j = i;
        while (j + 1 < pages.size() && pages[j + 1] - pages[j] <= 16)
            ++j;
        exp.map(pages[i] << 12, uint32_t(pages[j] - pages[i] + 1) << 12);
        work.map(pages[i] << 12, uint32_t(pages[j] - pages[i] + 1) << 12);
        i = j + 1;
    }
    // the port's locals passed by address (not in the original's memory: mapped in the port's image only, last, so
    // the region lists stay paired and it is not compared)
    work.map(kPcLocals, ProofPc::kLocalsSize);
    std::vector<uint32_t> dummy(16);
    Tgp tgp(dummy.data(), dummy.size());
    int total = 0, failed = 0, dropped = 0, unsupported = 0;
    long boundaryCalls = 0;
    std::map<uint32_t, long> perRoutine;
    for (size_t k = 0; k < recs.size(); ++k) {
        if (std::memcmp(recs[k].tag, "CAPT", 4) != 0) {
            ProofPc::apply(exp, recs[k].runs, recs[k].nruns);
            continue;
        }
        uint32_t h[4], regs[9];
        std::memcpy(h, recs[k].head, 16);
        std::memcpy(regs, recs[k].head + 16, 36);
        ProofPc::apply(exp, recs[k].runs, recs[k].nruns);
        for (size_t r = 0; r < exp.regions().size(); ++r)
            std::memcpy(work.regions()[r].bytes.get(), exp.regions()[r].bytes.get(), exp.regions()[r].size);
        ProofPc a(work, tgp);
        a.localBase = a.localTop = kPcLocals;
        size_t e = k + 1;
        for (; e < recs.size(); ++e) {
            const Rec& r = recs[e];
            if (!std::memcmp(r.tag, "BND1", 4)) {
                ProofPc::Boundary b;
                std::memcpy(&b.routine, r.head, 4);
                std::memcpy(&b.result, r.head + 40, 4);
                std::memcpy(b.regs, r.head + 4, 36);
                b.fpu = r.head + 44;
                for (uint32_t w = 0; w < 8; ++w)   // the stack as it was at the call (this record's runs not applied yet)
                    if (exp.mapped(b.regs[4] + 4 + w * 4))
                        b.stack[w] = exp.u32(b.regs[4] + 4 + w * 4);
                b.runs = r.runs;
                b.nruns = r.nruns;
                a.boundaries.push_back(b);
            }
            ProofPc::apply(exp, r.runs, r.nruns);
            if (!std::memcmp(r.tag, "CEXT", 4) || !std::memcmp(r.tag, "DROP", 4))
                break;
        }
        if (e >= recs.size() || !std::memcmp(recs[e].tag, "DROP", 4)) {
            ++dropped;
            k = e;
            continue;
        }
        uint32_t xregs[9];
        std::memcpy(xregs, recs[e].head, 36);
        k = e;
        work.bad = false;
        if (!runRoutinePc(a, h[0], regs)) {
            ++unsupported;
            continue;
        }
        ++total;
        ++perRoutine[h[0]];
        boundaryCalls += long(a.boundaries.size());
        const uint32_t stackLo = (regs[4] - 0x1000) & ~0xFFFu, stackHi = xregs[4];
        size_t diffs = 0;
        uint32_t first = 0;
        for (size_t r = 0; r < exp.regions().size(); ++r) {
            const auto& R = exp.regions()[r];
            const uint8_t* w = work.regions()[r].bytes.get();
            for (uint32_t i = 0; i < R.size; ++i) {
                const uint32_t addr = R.base + i;
                if (addr >= stackLo && addr < stackHi)
                    continue;
                if (w[i] != R.bytes[i]) {
                    if (!diffs)
                        first = addr;
                    ++diffs;
                }
            }
        }
        if (a.error.empty() && work.bad) {
            char buf[120];
            std::snprintf(buf, sizeof buf, "the port accessed unmapped address 0x%X", work.badAddr);
            a.error = buf;
        }
        if (a.error.empty() && a.nextBoundary != a.boundaries.size()) {
            char nb[120];
            std::snprintf(nb, sizeof nb, "the original called a boundary routine the port did not call (0x%X)",
                          a.boundaries[a.nextBoundary].routine);
            a.error = nb;
        }
        if (diffs || !a.error.empty()) {
            ++failed;
            if (failed <= 10) {
                std::printf("capture %x #%u (tick %u, front %u): %zu bytes differ (first 0x%X)%s%s\n", h[0], h[1], h[2],
                            h[3], diffs, first, a.error.empty() ? "" : "; ", a.error.c_str());
                for (uint32_t addr = first, shown = 0; diffs && shown < 6 && addr < first + 0x10000; ++addr)
                    if (exp.mapped(addr) && *work.ptr(addr) != *exp.ptr(addr) && !(addr >= stackLo && addr < stackHi)) {
                        std::printf("    0x%X: port %02x original %02x\n", addr, *work.ptr(addr), *exp.ptr(addr));
                        ++shown;
                    }
            }
        }
    }
    std::printf("pc captures: %d checked (%ld boundary calls replayed), %d differ, %d dropped (never returned)%s\n", total,
                boundaryCalls, failed, dropped, unsupported ? "; some routines not ported for the PC were skipped" : "");
    for (auto& [r, n] : perRoutine)
        std::printf("  %X: %ld\n", r, n);
    return failed ? 1 : 0;
}

#include "proof/pmmaths_exe.inc"

// PC Mode's turn and shot maths (port/pm_maths.h, docs/pcmode.md row 10): (1) the rebuilt sin / cos table (8192 floats)
// and arctangent table (1025 ints) and the identity matrix entry by entry against THOTD.EXE 0x5697B0 / 0xC653B8 /
// 0x5CE4F4; (2) a differential run: pm::enemy_turn against Game<Pc>::enemy_turn_pc and pm::shot_ray / pm::ray_sign
// against Game<Pc>::shot_ray_setup / ray_sign_test (the PC routines the PC captures prove, here on THOTD.EXE's own
// tables and matrix stack) on generated cases; (3) pm::copro_matrix's layout against the coprocessor's transform.
template <class At> int pmmaths_trig(At at)
{
    using namespace hotd::port;
    using GP = typename Game<Pc>::G;
    const uint8_t* trigExe = at(0x5697B0);
    const uint8_t* atanExe = at(0xC653B8);
    const uint8_t* identExe = at(0x5CE4F4);
    if (!trigExe || !atanExe || !identExe) {
        std::fprintf(stderr, "THOTD.EXE: the trig tables are not where expected\n");
        return 1;
    }
    int trigBad = 0, atanBad = 0, identBad = 0;
    for (int i = 0; i < 8192; ++i)
        if (std::memcmp(trigExe + 4 * i, &pm::trig_table()[size_t(i)], 4) != 0 && trigBad++ < 4)
            std::printf("  sin / cos entry %d (%s) differs\n", i / 2, (i & 1) ? "cos" : "sin");
    for (int i = 0; i <= 1024; ++i)
        if (std::memcmp(atanExe + 4 * i, &pm::atan_table()[size_t(i)], 4) != 0 && atanBad++ < 4)
            std::printf("  arctangent entry %d differs\n", i);
    const pm::Matrix ident;
    identBad = std::memcmp(identExe, ident.e, sizeof ident.e) != 0;
    std::printf("pm tables: sin / cos 8192 entries, %d differ; arctangent 1025 entries, %d differ; identity %s\n", trigBad,
                atanBad, identBad ? "differs" : "same");

    // the PC's memory for Game<Pc>: the EXE's sections with raw data (tables, identity), the matrix stack, the
    // stored matrices, the shot rays and one enemy
    static Mem m;
    for (uint32_t va = 0x410000; va < 0xC6F000; va += 0x1000)
        if (const uint8_t* p = at(va))
            if (at(va + 0xFFF))
                std::memcpy(m.map(va, 0x1000), p, 0x1000);
    m.map(GP::pc_matrix_ptr & ~0xFFFu, 0x1000);
    m.map(GP::pc_stored_matrices & ~0xFFFu, 0x2000);
    m.map(GP::shot_rays & ~0xFFFu, 0x1000);
    const uint32_t stack = 0x30000000, obj = 0x30010000;
    m.map(stack, 0x10000);
    m.map(obj, 0x1000);
    m.map(kPcLocals, ProofPc::kLocalsSize);
    m.set32(GP::pc_matrix_ptr, stack);
    m.set32(GP::frontend_state, 0);
    std::vector<uint32_t> dummy(16);
    Tgp tgp(dummy.data(), dummy.size());
    ProofPc pcg(m, tgp);
    pcg.localBase = pcg.localTop = kPcLocals;

    uint64_t seed = 0x48C0FFEE12345ull;
    auto rnd = [&]() {
        seed = seed * 6364136223846793005ull + 1442695040888963407ull;
        return uint32_t(seed >> 33);
    };
    auto frand = [&](float lo, float hi) { return lo + (hi - lo) * float(rnd() % 1000001u) / 1000000.0f; };
    // enemy turn: yaw, positions (with equal / axis / tiny / far offsets), backwards and minimum slowdown both ways
    long turnCases = 0, turnBad = 0;
    for (int n = 0; n < 2000000; ++n) {
        pc::Enemy& e = m.at<pc::Enemy>(obj);
        const int32_t yaw0 = (n & 7) == 7 ? int32_t(rnd() % 0x30000u) - 0x10000 : int32_t(rnd() & 0xFFFF);
        float pos[3] = {frand(-400.0f, 400.0f), frand(-5.0f, 5.0f), frand(-400.0f, 400.0f)};
        float tgt[3];
        const float scale = (n % 5 == 0) ? 0.001f : (n % 5 == 1) ? 2.0f : (n % 5 == 2) ? 30.0f : 300.0f;
        for (int k = 0; k < 3; ++k)
            tgt[k] = pos[k] + frand(-scale, scale);
        if (n % 97 == 0)
            tgt[0] = pos[0];
        if (n % 89 == 0)
            tgt[2] = pos[2];
        if (n % 1009 == 0)
            tgt[0] = pos[0], tgt[2] = pos[2];
        const bool back = (n & 1) != 0, min4 = (n & 2) != 0;
        e.ang[1] = yaw0;
        for (int k = 0; k < 3; ++k)
            e.pos[k] = pos[k], e.target_pos[k] = tgt[k];
        pcg.enemy_turn_pc(obj, back, min4);
        const int32_t want = m.at<pc::Enemy>(obj).ang[1];
        const int32_t got = pm::enemy_turn(yaw0, pos, tgt, back, min4);
        ++turnCases;
        if (got != want && turnBad++ < 4)
            std::printf("  turn: yaw 0x%X pos (%g, %g) target (%g, %g)%s%s: PC 0x%X, PM 0x%X\n", yaw0, pos[0], pos[2],
                        tgt[0], tgt[2], back ? " backwards" : "", min4 ? " min 4" : "", want, got);
    }
    // shot ray: camera matrices (rotations by random angles, translations), aim points over the screen and past it
    long rayCases = 0, rayBad = 0, signCases = 0, signBad = 0;
    const double pi = 3.14159265358979323846;
    for (int n = 0; n < 300000; ++n) {
        const double ya = double(rnd() % 65536u) * 2 * pi / 65536, pa = double(rnd() % 65536u) * 2 * pi / 65536,
                     ra = (n & 1) ? double(rnd() % 65536u) * 2 * pi / 65536 : 0.0;
        const double cy = std::cos(ya), sy = std::sin(ya), cp = std::cos(pa), sp = std::sin(pa), cr = std::cos(ra),
                     sr = std::sin(ra);
        const double R[9] = {cy * cr + sy * sp * sr, -cy * sr + sy * sp * cr, sy * cp, cp * sr, cp * cr, -sp,
                             -sy * cr + cy * sp * sr, sy * sr + cy * sp * cr, cy * cp};
        float cam[12];
        for (int k = 0; k < 9; ++k)
            cam[k] = float(R[k]);
        for (int k = 9; k < 12; ++k)
            cam[k] = frand(-600.0f, 600.0f);
        for (int k = 0; k < 12; ++k)
            m.set32(GP::pc_stored_matrices + 0x30 + 4 * uint32_t(k), fbits(cam[k]));
        const float x = float(int32_t(rnd() % 640u) - 320), y = float(int32_t(rnd() % 520u) - 260);
        const int player = n & 1;
        pcg.shot_ray_setup(player, x, y, 496.0f);
        const pc::ShotRay& want = m.at<pc::ShotRay>(GP::shot_rays + uint32_t(player) * uint32_t(sizeof(pc::ShotRay)));
        pc::ShotRay got{};
        pm::shot_ray(x, y, 496.0f, cam, got);
        ++rayCases;
        if (std::memcmp(&got, &want, sizeof got) != 0 && rayBad++ < 4)
            std::printf("  shot ray: aim (%g, %g) case %d differs\n", x, y, n);
        for (int j = 0; j < 4; ++j) {
            const float px = frand(-300.0f, 300.0f), py = frand(-300.0f, 300.0f), pz = frand(1.0f, 900.0f),
                        rad = frand(0.5f, 120.0f);
            ++signCases;
            if (pcg.ray_sign_test(player, px, py, pz, rad) != pm::ray_sign(want, px, py, pz, rad) && signBad++ < 4)
                std::printf("  ray test: (%g, %g, %g) r %g differs\n", px, py, pz, rad);
        }
    }
    // the coprocessor's column layout -> the PC's rows: small integers, so both summation orders are exact
    long layoutBad = 0;
    for (int n = 0; n < 1000; ++n) {
        uint32_t in[12];
        for (int k = 0; k < 12; ++k)
            in[k] = fbits(float(int32_t(rnd() % 41u) - 20));
        Tgp::Out out, none;
        tgp.call(0x07, in, 12, none);
        uint32_t slot[1] = {1};
        tgp.call(0x75, slot, 1, none);
        const float p[3] = {float(int32_t(rnd() % 61u) - 30), float(int32_t(rnd() % 61u) - 30),
                            float(int32_t(rnd() % 61u) - 30)};
        const uint32_t pin[3] = {fbits(p[0]), fbits(p[1]), fbits(p[2])};
        tgp.call(0x1A, pin, 3, out);
        pm::Matrix pmm;
        pm::copro_matrix(tgp, 1, pmm.e);
        float o[3];
        pmm.transform(p[0], p[1], p[2], o[0], o[1], o[2]);
        if (fbits(o[0]) != out.w[0] || fbits(o[1]) != out.w[1] || fbits(o[2]) != out.w[2])
            ++layoutBad;
    }
    std::printf("pm turn / shot maths vs Game<Pc>: enemy turn %ld cases, %ld differ; shot ray %ld cases, %ld differ; "
                "ray test %ld cases, %ld differ; coprocessor matrix layout 1000 cases, %ld differ\n",
                turnCases, turnBad, rayCases, rayBad, signCases, signBad, layoutBad);
    // pmboss: PC Mode runs the PC's maths on matrices only the arcade's body drawing makes (a part's matrix read with
    // 0x11, a kept node matrix): pm::from_copro_words takes their coprocessor layout, and the PC's products
    // (mul_stored with stored 1 for the Chariot / Hermit part hooks and armour pieces, pc_matrix_mul8 on stored 1 for
    // the axe release) must compose as the coprocessor's 0x77 / 0x09 do. Not bit-exact (other float operations): the
    // largest difference of the transformed point, relative to its size, is reported (tolerance 1e-5)
    double worstComp = 0.0;
    long compBad = 0;
    for (int n = 0; n < 2000; ++n) {
        auto randRot = [&](uint32_t w[12]) {
            pm::Matrix r;
            r.rotate_z(rnd() & 0xFFFF);
            r.rotate_x(rnd() & 0xFFFF);
            r.rotate_y(rnd() & 0xFFFF);
            for (int row = 0; row < 3; ++row)
                for (int col = 0; col < 3; ++col)
                    w[col * 3 + row] = fbits(r.e[row * 3 + col]);   // the coprocessor's layout
            for (int k = 9; k < 12; ++k)
                w[k] = fbits(frand(-500.0f, 500.0f));
        };
        uint32_t cw[12], aw[12];
        randRot(cw);
        randRot(aw);
        Tgp::Out out, none;
        tgp.call(0x07, cw, 12, none);
        uint32_t slot[1] = {1};
        tgp.call(0x75, slot, 1, none);
        const float p[3] = {frand(-10.0f, 10.0f), frand(-10.0f, 10.0f), frand(-10.0f, 10.0f)};
        const uint32_t pin[3] = {fbits(p[0]), fbits(p[1]), fbits(p[2])};
        float cam[12];
        pm::copro_matrix(tgp, 1, cam);
        for (int kind = 0; kind < 2; ++kind) {
            float o[3];
            if (kind == 0) {   // part x stored 1 (0x77 1) vs from_copro_words(part).mul_stored(stored 1)
                tgp.call(0x07, aw, 12, none);
                tgp.call(0x77, slot, 1, none);
                pm::Matrix pmm = pm::from_copro_words(aw);
                pmm.mul_stored(cam);
                pmm.transform(p[0], p[1], p[2], o[0], o[1], o[2]);
            } else {   // stored 1, then 0x09 with the node, vs stored 1 .mul(from_copro_words(node))
                tgp.call(0x73, slot, 1, none);
                tgp.call(0x09, aw, 12, none);
                pm::Matrix pmm = pm::from_floats(cam);
                pmm.mul(pm::from_copro_words(aw).e);
                pmm.transform(p[0], p[1], p[2], o[0], o[1], o[2]);
            }
            tgp.call(0x1A, pin, 3, out);
            double size = 1.0, diff = 0.0;
            for (int k = 0; k < 3; ++k) {
                size = std::max(size, std::fabs(double(ffrom(out.w[k]))));
                diff = std::max(diff, std::fabs(double(ffrom(out.w[k])) - double(o[k])));
            }
            worstComp = std::max(worstComp, diff / size);
            compBad += diff / size > 1e-5;
        }
    }
    std::printf("pm composition with the coprocessor's matrices (0x77 / 0x09 vs mul_stored / mul): 4000 cases, %ld over "
                "1e-5 (largest relative difference %.3g)\n",
                compBad, worstComp);
    const int exeBad = pmmaths_exe(m, seed);
    return trigBad || atanBad || identBad || turnBad || rayBad || signBad || layoutBad || compBad || exeBad ? 1 : 0;
}

// PC Mode on the arcade logic (docs/pcmode.md): the runtime's copies of PC data and maths against THOTD.EXE itself:
// (1) Game<A>::pm_table (the character table 0x5CE348: type, model, pictures, hit points, rounds, damage, reload, hit
// size); (2) Game<A>::pm_fast_sqrt, whose table is rebuilt from a formula, against the PC's pc_fast_sqrt (0x4FC76C:
// table 0x5CCEFC read from the EXE) for every one of the 2^32 float bit patterns
int pmmaths(const std::string& exePath)
{
    const std::vector<uint8_t> exe = readFile(exePath);
    if (exe.size() < 0x1000) {
        std::fprintf(stderr, "can't read %s\n", exePath.c_str());
        return 2;
    }
    auto at = [&](uint32_t va) -> const uint8_t* {   // a virtual address in the EXE's sections (raw data)
        uint32_t pe, n, opt;
        std::memcpy(&pe, &exe[0x3C], 4);
        n = uint32_t(exe[pe + 6] | (exe[pe + 7] << 8));
        opt = uint32_t(exe[pe + 20] | (exe[pe + 21] << 8));
        for (uint32_t i = 0; i < n; ++i) {
            const uint8_t* sh = &exe[pe + 24 + opt + 40 * i];
            uint32_t sva, rs, rp;
            std::memcpy(&sva, sh + 12, 4);
            std::memcpy(&rs, sh + 16, 4);
            std::memcpy(&rp, sh + 20, 4);
            if (va >= 0x400000 + sva && va < 0x400000 + sva + rs && rp + rs <= exe.size())
                return &exe[rp + (va - 0x400000 - sva)];
        }
        return nullptr;
    };
    using GA = hotd::port::Game<hotd::port::A>;
    const uint8_t* ct = at(0x5CE348);
    const uint8_t* st = at(0x5CCEFC);
    if (!ct || !st) {
        std::fprintf(stderr, "THOTD.EXE: the tables are not where expected\n");
        return 2;
    }
    int bad = 0;
    for (int c = 0; c < 7; ++c) {
        const uint8_t* r = ct + c * 0x34;
        int32_t w[9];
        std::memcpy(w, r + 0x10, sizeof w);
        float dmg, hit;
        std::memcpy(&dmg, r + 0x28, 4);
        std::memcpy(&hit, r + 0x30, 4);
        const GA::PmStats& s = GA::pm_table(c);
        const bool ok = std::strncmp(reinterpret_cast<const char*>(r), s.name, 16) == 0 && w[0] == s.type &&
                        w[1] == s.model && w[2] == s.portrait && w[3] == s.sprite2 && w[4] == s.hp && w[5] == s.rounds &&
                        dmg == s.damage && w[7] == s.reload_ms && hit == s.hit_size;
        if (!ok) {
            std::printf("character %d (%s): differs from THOTD.EXE 0x%X\n", c, s.name, 0x5CE348 + c * 0x34);
            ++bad;
        }
    }
    uint32_t table[256];
    std::memcpy(table, st, sizeof table);
    uint64_t sqrtBad = 0;
    uint32_t first = 0;
    for (uint64_t u = 0; u <= 0xFFFFFFFFull; ++u) {
        const uint32_t b = uint32_t(u);
        const int32_t e = int32_t(b - 0x3F800000u);
        const uint32_t index = ((b & 0x7FFFFFu) | (uint32_t(e) & 0x800000u)) >> 16;
        const int32_t half = (int32_t(uint32_t(e) & 0xFF000000u) >> 1) + 0x3F800000;
        const uint32_t pc = table[index] + uint32_t(half);
        float x;
        std::memcpy(&x, &b, 4);
        const float m = GA::pm_fast_sqrt(x);
        uint32_t mb;
        std::memcpy(&mb, &m, 4);
        if (mb != pc && sqrtBad++ == 0)
            first = b;
    }
    std::printf("pm maths: characters %d of 7 as THOTD.EXE; pm_fast_sqrt: 4294967296 inputs, %llu differ%s\n", 7 - bad,
                (unsigned long long)sqrtBad, sqrtBad ? "" : " (bit-exact with pc_fast_sqrt)");
    if (sqrtBad)
        std::printf("  first: 0x%08X\n", first);
    const int trigBad = pmmaths_trig(at);
    return bad || sqrtBad || trigBad ? 1 : 0;
}

} // namespace

int main(int argc, char** argv)
{
    const char* regions = "private/arcade_exports/regions";
    if (argc >= 3 && std::strcmp(argv[1], "tgp") == 0)
        return tgp(argv[2], argc > 3 ? argv[3] : "private/arcade_exports/regions/copro_data.bin");
    if (argc >= 3 && std::strcmp(argv[1], "capture") == 0)
        return capture(argv[2], argc > 3 ? argv[3] : regions);
    if (argc >= 3 && std::strcmp(argv[1], "pccapture") == 0)
        return pccapture(argv[2], argc > 3 ? argv[3] : "private/pcharness/game/THOTD.EXE");
    if (argc >= 2 && std::strcmp(argv[1], "pmmaths") == 0)
        return pmmaths(argc > 2 ? argv[2] : "private/pcharness/game/THOTD.EXE");
    std::fprintf(stderr, "usage: hotd_proof tgp <copro.txt> [copro_data.bin]\n"
                         "       hotd_proof capture <capture folder> [regions folder]\n"
                         "       hotd_proof pccapture <PC capture folder> [THOTD.EXE]\n"
                         "       hotd_proof pmmaths [THOTD.EXE]\n");
    return 2;
}
