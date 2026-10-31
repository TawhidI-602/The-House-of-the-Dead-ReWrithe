// Small archive helpers for the launcher's files check (docs/teaser_status/r4.md): CRC-32, SHA-1, a zip reader
// (central directory, stored and deflated entries) and an inflater. Our own code: no third-party library.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace hotd {

uint32_t crc32(const uint8_t* data, size_t n, uint32_t crc = 0);
std::string sha1Hex(const uint8_t* data, size_t n);   // 40 lower-case hex digits

// raw deflate (RFC 1951) of `in` into `out` (its size known: the zip entry's); false on a broken stream
bool inflateRaw(const uint8_t* in, size_t inSize, uint8_t* out, size_t outSize);

// The PC files the launcher's check found (game/files_check.h): a path relative to the PC folder (forward slashes, any
// case) -> the checked file. Loaders of PC files ask here first; empty = not mapped (look in the folder as before).
void setPcFileMap(std::map<std::string, std::string> m);
std::string pcFileMapped(const std::string& rel);

class ZipFile {
public:
    struct Entry {
        std::string name;            // as stored (any folder prefix)
        uint32_t crc = 0;
        uint64_t compSize = 0, size = 0;
        uint16_t method = 0;         // 0 stored, 8 deflated
        uint64_t localOffset = 0;
    };
    bool open(const std::string& path, std::string& err);
    const std::vector<Entry>& entries() const { return entries_; }
    bool read(const Entry& e, std::vector<uint8_t>& out, std::string& err) const;
    // where an entry's (stored) bytes start in the zip file, after its local header: read in place, no copy
    bool dataOffset(const Entry& e, uint64_t& off, std::string& err) const;
    const std::string& path() const { return path_; }

private:
    std::string path_;
    std::vector<Entry> entries_;
};

} // namespace hotd
