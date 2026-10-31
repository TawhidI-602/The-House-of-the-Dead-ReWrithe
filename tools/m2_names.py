#!/usr/bin/env python3
"""
m2_names.py - the developers' names for creature types ("BO_...") and motions ("MO_..."), from the program ROM
(maincpu 0xC8A60 / 0xC8BE0, docs/formats.md §23; THOTD.EXE has the same tables at file offsets 0x1D84E0 / 0x1D8658).

  py tools/m2_names.py <maincpu.bin> [out.txt]        default out: private/decomp/names_bodies_motions.txt
"""
import os, struct, sys


def table(rom, at, n):
    out = []
    for i in range(n):
        p = struct.unpack_from("<I", rom, at + 4 * i)[0]
        out.append(rom[p:rom.index(b"\0", p)].decode("latin1"))
    return out


def main():
    rom = open(sys.argv[1], "rb").read()
    out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(__file__), "..", "private", "decomp",
                                                            "names_bodies_motions.txt")
    with open(out, "w", encoding="utf-8") as f:
        for i, n in enumerate(table(rom, 0xC8A60, 94)):
            f.write(f"type {i:3d} {n}\n")
        for i, n in enumerate(table(rom, 0xC8BE0, 674)):
            f.write(f"motion {i:3d} 0x{i:03x} {n}\n")
    print("written", out)


if __name__ == "__main__":
    main()
