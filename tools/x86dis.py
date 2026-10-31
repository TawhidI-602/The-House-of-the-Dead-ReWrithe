#!/usr/bin/env python3
"""x86dis.py - disassembles a range of the PC port's THOTD.EXE (32-bit, image base 0x400000) with capstone.
Usage: py tools/x86dis.py <THOTD.EXE> <start hex> <end hex>   (or <start hex> +<length hex>)"""
import struct, sys
import capstone


def load(path):
    d = open(path, "rb").read()
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    n = struct.unpack_from("<H", d, pe + 6)[0]
    opt = struct.unpack_from("<H", d, pe + 20)[0]
    base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
    secs = []
    for i in range(n):
        o = pe + 24 + opt + 40 * i
        _, vs, va, rs, rp = struct.unpack_from("<8sIIII", d, o)
        secs.append((base + va, rs, rp))
    return d, secs


def read(d, secs, a, n):
    for va, rs, rp in secs:
        if va <= a < va + rs:
            return d[rp + a - va: rp + min(a - va + n, rs)]
    return b""


def main():
    d, secs = load(sys.argv[1])
    a = int(sys.argv[2], 16)
    e = a + int(sys.argv[3][1:], 16) if sys.argv[3].startswith("+") else int(sys.argv[3], 16)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    for i in md.disasm(read(d, secs, a, e - a), a):
        print(f"{i.address:08x}  {i.bytes.hex():<20} {i.mnemonic} {i.op_str}")


if __name__ == "__main__":
    main()
