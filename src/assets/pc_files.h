// The PC game's files, read only, from a folder or from a .zip (pczip, user 2026-10-17: the PC files may stay zipped).
// A small virtual file system over our own zip reader (assets/archive.h): a path is a real file, or
// "<a .zip file>/<path inside it>" (the PC folder chosen as a zip: <zip> + "/" + "ART/STAGE1/..."). Inside a zip the
// names match whatever their case and separators, and the install may sit one folder down ("HOTD DUMP/THOTD.EXE"):
// paths relative to that folder find its files. Stored entries are read in place (seeks, no copy); deflated ones are
// inflated on demand (the last few kept). Every PC reader goes through here (pc_loader, sprites_pc, audio, the files
// check, the window icon). RAR / 7z are not read (README: convert to .zip).
#pragma once

#include <cstdint>
#include <istream>
#include <memory>
#include <string>
#include <vector>

namespace hotd::pcfs {

// `path` is a regular file named *.zip (any case)
bool isZip(const std::string& path);
// `path` names a file inside a zip ("<x.zip>/<inside>")
bool inZip(const std::string& path);
// an archive this reader cannot read (.rar, .7z): the message for the player, else empty
std::string unsupportedArchive(const std::string& path);

bool exists(const std::string& path);
int64_t size(const std::string& path);   // -1: missing
bool readFile(const std::string& path, std::vector<uint8_t>& out, std::string* err = nullptr);

// a stream over the file for the VMC page readers (std::ifstream for a real file; a zip's stored entry read in place
// in 64 KB blocks, a deflated one from memory); seekg / tellg work. null when missing
std::unique_ptr<std::istream> openStream(const std::string& path);

// the files of a zip for the files check: `rel` relative to the install's folder in the zip ("" or one folder down),
// lower case, '/' (entries outside that folder: their whole name); `path` the virtual path to read it by
struct ZipItem {
    std::string rel, path;
    uint64_t size = 0;
    uint32_t crc = 0;
};
bool listZip(const std::string& zip, std::vector<ZipItem>& out, std::string& err);

} // namespace hotd::pcfs
