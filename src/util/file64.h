// 64-bit file seeks on every system (docs/porting.md): MSVC's _fseeki64 / _ftelli64 on Windows, POSIX fseeko / ftello
// (64-bit off_t on 64-bit Linux / macOS) elsewhere. Files over 2 GB (captures, big zips) need them.
#pragma once

#include <cstdint>
#include <cstdio>
#ifndef _WIN32
#include <sys/types.h>
#endif

namespace hotd {

inline int fseek64(std::FILE* f, int64_t off, int origin)
{
#ifdef _WIN32
    return _fseeki64(f, off, origin);
#else
    return fseeko(f, off_t(off), origin);
#endif
}

inline int64_t ftell64(std::FILE* f)
{
#ifdef _WIN32
    return _ftelli64(f);
#else
    return int64_t(ftello(f));
#endif
}

} // namespace hotd
