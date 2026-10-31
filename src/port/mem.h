// The original address space as the ported code sees it, for either version (docs/port.md): regions mapped at their
// original addresses (arcade: work RAM 0x500000..0x5FFFFF, the program ROM at 0, main_data at 0x2000000; PC port: the
// EXE's data and bss, the heap blocks, a stack window). The port keeps the original data layouts (port/types.h) and
// stores pointers as 32-bit original addresses, so recorded memory can be loaded and compared byte for byte.
#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

namespace hotd::port {

// a pointer field / variable of the original: an address of a T
template <class T> struct Addr {
    uint32_t a = 0;
    explicit operator bool() const { return a != 0; }
};

class Mem {
public:
    static constexpr uint32_t kRam = 0x500000, kRamSize = 0x100000;   // arcade work RAM
    static constexpr uint32_t kData = 0x2000000;                       // arcade main_data

    struct Region {
        uint32_t base, size;
        std::unique_ptr<uint8_t[]> bytes;
    };

    // maps [base, base + size) (rounded out to 4 KB pages), zero filled; returns the bytes of `base`
    uint8_t* map(uint32_t base, uint32_t size)
    {
        const uint32_t b = base & ~0xFFFu, e = uint32_t((uint64_t(base) + size + 0xFFF) & ~uint64_t(0xFFF));
        Region r{b, e - b, std::make_unique<uint8_t[]>(size_t(e - b))};
        std::memset(r.bytes.get(), 0, r.size);
        for (uint32_t p = b; p != e; p += 0x1000)
            page_[p >> 12] = r.bytes.get() + (p - b);
        regions_.push_back(std::move(r));
        return page_[base >> 12] + (base & 0xFFF);
    }
    // maps a region and copies `bytes` to its start
    void load(uint32_t base, const std::vector<uint8_t>& bytes)
    {
        if (!bytes.empty())
            std::memcpy(map(base, uint32_t(bytes.size())), bytes.data(), bytes.size());
    }
    bool mapped(uint32_t a) const { return page_[a >> 12] != nullptr; }
    const std::vector<Region>& regions() const { return regions_; }

    uint8_t* ptr(uint32_t a)
    {
        uint8_t* p = page_[a >> 12];
        if (p)
            return p + (a & 0xFFF);
        if (!bad)
            badAddr = a;
        bad = true;
        return bad_;
    }
    template <class T> T& at(uint32_t a) { return *reinterpret_cast<T*>(ptr(a)); }
    template <class T> T& operator[](Addr<T> p) { return at<T>(p.a); }
    template <class T> T& at(uint32_t a, uint32_t index) { return at<T>(a + index * uint32_t(sizeof(T))); }
    uint32_t u32(uint32_t a) { uint32_t v; std::memcpy(&v, ptr(a), 4); return v; }
    float f32(uint32_t a) { float v; std::memcpy(&v, ptr(a), 4); return v; }
    void set32(uint32_t a, uint32_t v) { std::memcpy(ptr(a), &v, 4); }

    bool bad = false;        // a read or write outside the mapped regions happened (it went to a scratch block)
    uint32_t badAddr = 0;    // the first such address

private:
    std::vector<uint8_t*> page_ = std::vector<uint8_t*>(size_t(1) << 20);
    std::vector<Region> regions_;
    uint8_t bad_[0x10000] = {};
};

} // namespace hotd::port
