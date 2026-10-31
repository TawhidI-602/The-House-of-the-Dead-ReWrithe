#!/usr/bin/env python3
"""
pc_model.py - bit-exact models of PC-port (THOTD.EXE) maths helpers, for the PA / PM behaviours (the arcade's are in
tools/tgp_model.py). Reads its tables from the read-only PC executable C:\\Games\\HOTD\\THOTD.EXE.orig.

  py tools/pc_model.py error        largest relative error of pc_fast_sqrt (random inputs)

pc_fast_sqrt: three identical copies in the PC code (0x4FC76C, 0x41347C = pc_fast_sqrt2, 0x4D7934 = pc_fast_sqrt3),
decoded from the bytes (50 53 52 83 ec 04 8b 44 24 14 ...): with b = the float's bits,
  e = b - 0x3F800000; result bits = table[(b & 0x7FFFFF | e & 0x800000) >> 16] + (((e & 0xFF000000) >> 1, signed) +
  0x3F800000), table = 256 words at 0x5CCEFC. Only the exponent's parity and the top 7 mantissa bits are used: up to
  0.39 % error (sqrt(4900) = 69.9714, sqrt(625) = 24.98).
"""
import math, random, struct, sys

EXE = r"C:\Games\HOTD\THOTD.EXE.orig"
_tab = None


def _va(data, va):
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    n = struct.unpack_from("<H", data, pe + 6)[0]
    opt = struct.unpack_from("<H", data, pe + 20)[0]
    base = struct.unpack_from("<I", data, pe + 52)[0]
    for i in range(n):
        s = pe + 24 + opt + 40 * i
        vs, va0, rs, ro = struct.unpack_from("<IIII", data, s + 8)
        if base + va0 <= va < base + va0 + max(vs, rs):
            return ro + va - base - va0
    raise ValueError(hex(va))


def table():
    global _tab
    if _tab is None:
        data = open(EXE, "rb").read()
        _tab = struct.unpack_from("<256I", data, _va(data, 0x5CCEFC))
    return _tab


def pc_fast_sqrt(x):
    """the PC's table square root (float in, float out)"""
    b = struct.unpack("<I", struct.pack("<f", x))[0]
    e = (b - 0x3F800000) & 0xFFFFFFFF
    hi = e & 0xFF000000
    if hi & 0x80000000:
        hi -= 0x100000000
    r = (table()[((b & 0x7FFFFF) | (e & 0x800000)) >> 16] + (hi >> 1) + 0x3F800000) & 0xFFFFFFFF
    return struct.unpack("<f", struct.pack("<I", r))[0]


if __name__ == "__main__":
    if sys.argv[1:] == ["error"]:
        random.seed(1)
        worst = max(abs(pc_fast_sqrt(x) / math.sqrt(x) - 1) for x in (random.uniform(0.01, 1e6) for _ in range(200000)))
        print("pc_fast_sqrt: largest relative error %.4f %%" % (worst * 100))
    else:
        print(__doc__)
