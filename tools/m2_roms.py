#!/usr/bin/env python3
"""
m2_roms.py - verify a House of the Dead (Model 2C, Revision A) ROM zip and rebuild its memory regions.

The layout follows the "hotd" entry in MAME's src/mame/sega/model2.cpp (BSD-3-Clause):
  ROM_LOAD32_WORD       file's 16-bit words go to every other word of the region (offset 0 or 2), bytes unchanged
  ROM_LOAD16_WORD_SWAP  file's 16-bit words are byte-swapped
  ROM_COPY              main_data 0x1800000-0x18FFFFF is mirrored up to 0x1FFFFFF

Usage:
  python m2_roms.py verify  <hotd.zip>
  python m2_roms.py extract <hotd.zip> <out_folder>

extract writes one file per region (maincpu.bin, main_data.bin, copro_data.bin, polygons.bin, textures.bin,
audiocpu.bin, samples.bin). Only write into private\\arcade_exports\\: the output is ROM data.
"""
import sys, os, zipfile, hashlib, zlib

# region name -> (size, [(file, region offset, file size, crc32, sha1, method)])
W32, SWAP16 = "load32_word", "load16_word_swap"
REGIONS = {
    "maincpu": (0x200000, [
        ("epr-19696a.15", 0x000000, 0x080000, 0x42adc32e, "92931e8339dbba23d7ade178fcc56d37291411cb", W32),
        ("epr-19697a.16", 0x000002, 0x080000, 0x1e247cd5, "693e929d543bad880ea69d781155949f0aa246de", W32),
        ("epr-19694.13",  0x100000, 0x080000, 0xe85ca1a3, "3d688be98f78fe40c2af1e91df6decd500400ae9", W32),
        ("epr-19695.14",  0x100002, 0x080000, 0xcd52b461, "bc96ab2a4ba7f30c0b89814acc8931c8bf800a82", W32),
    ]),
    "main_data": (0x2000000, [
        ("mpr-19704.11", 0x0000000, 0x400000, 0xaa80dbb0, "24e63f4392847f288971469cd10448536eb435d4", W32),
        ("mpr-19705.12", 0x0000002, 0x400000, 0xf906843b, "bee4f43b3ad15d93a2f9f07b873c9cf5d228e2f9", W32),
        ("mpr-19702.9",  0x0800000, 0x400000, 0xfc8aa3b7, "b64afb17d9c97277d8c4f20811f14f65a61cbb56", W32),
        ("mpr-19703.10", 0x0800002, 0x400000, 0x208d993d, "e5c45ea5621f99661a87ffe88e24764d2bbcb51e", W32),
        ("mpr-19700.7",  0x1000000, 0x400000, 0x0558cfd3, "94440839d3325176c2d03f39a78949d0ef040bba", W32),
        ("mpr-19701.8",  0x1000002, 0x400000, 0x224a8929, "933770546d46abca400e7f524eff2ae89241e56d", W32),
        ("epr-19698.5",  0x1800000, 0x080000, 0xe7a7b6ea, "77cb53f8730fdb55080b70910ab8c750d79acb02", W32),
        ("epr-19699.6",  0x1800002, 0x080000, 0x8160b3d9, "9dab483c60624dddba8085e94a4325739592ec17", W32),
    ]),
    "copro_data": (0x800000, [
        ("epr-19707.29", 0x000000, 0x080000, 0x384fd133, "6d060378d0f801b04d12e7ee874f2fa0572992d9", W32),
        ("epr-19706.30", 0x000002, 0x080000, 0x1277531c, "08d3e733ba9989fcd32290634171c73f26ab6e2b", W32),
    ]),
    "polygons": (0x2000000, [
        ("mpr-19715.17", 0x0000000, 0x400000, 0x3ff7dda7, "0a61b091bb0bc659f0cbca8ad401d0925a1dc2ea", W32),
        ("mpr-19711.21", 0x0000002, 0x400000, 0x080d13f1, "4167428a2a903aea2c14631ccf924afb81338b89", W32),
        ("mpr-19714.18", 0x0800000, 0x400000, 0x3e55ab49, "70b4c1627db80e6734112c02265495e2b4a53278", W32),
        ("mpr-19710.22", 0x0800002, 0x400000, 0x80df1036, "3cc59bb4910aa5382e95762f63325c06b763bd23", W32),
        ("mpr-19713.19", 0x1000000, 0x400000, 0x4d092cd3, "b6d0be283c25235249186751c7f025a7c38d2f36", W32),
        ("mpr-19709.23", 0x1000002, 0x400000, 0xd08937bf, "c92571e35960f27dc8b0b059f12167026d0666d1", W32),
        ("mpr-19712.20", 0x1800000, 0x400000, 0x41577943, "25a0d921c8662043c5860dc7a226d4895ff9fff6", W32),
        ("mpr-19708.24", 0x1800002, 0x400000, 0x5cb790f2, "d3cae450186bc62fd746b14d6a05cb397efcfe40", W32),
    ]),
    "textures": (0x1000000, [
        ("mpr-19718.27", 0x0000000, 0x400000, 0xa9de5924, "3ebac2aeb1467939337c9a5c87ad9c293560dae2", W32),
        ("mpr-19716.25", 0x0000002, 0x400000, 0x45c7dcce, "f602cabd879c69afee544848feafb9fb9f5d51e2", W32),
        ("mpr-19719.28", 0x0800000, 0x400000, 0x838f8343, "fe6622b5917f9a99c097fd60d9446ac6b481fa75", W32),
        ("mpr-19717.26", 0x0800002, 0x400000, 0x393e440b, "927ac9cad22f87b339cc86043678470ff139ce1f", W32),
    ]),
    "audiocpu": (0x080000, [
        ("epr-19720.31", 0x000000, 0x080000, 0xb367d21d, "1edaed489a3518ddad85728e416319f940ea02bb", SWAP16),
    ]),
    "samples": (0x800000, [
        ("mpr-19721.32", 0x000000, 0x400000, 0xf5d8fa9a, "6836973a687c59dd80f8e6c30d33155e306be199", SWAP16),
        ("mpr-19722.34", 0x400000, 0x400000, 0xa56fa539, "405a892bc368ba862ba71bda7525b421d6973c0e", SWAP16),
    ]),
}


def read_verified(zip_path):
    """Return {file name: bytes} for every ROM of the set, after checking size, CRC32 and SHA-1."""
    files, bad = {}, 0
    with zipfile.ZipFile(zip_path) as z:
        names = {os.path.basename(n).lower(): n for n in z.namelist()}
        for region, (_, roms) in REGIONS.items():
            for name, _, size, crc, sha1, _ in roms:
                if name not in names:
                    print(f"MISSING  {name}"); bad += 1; continue
                data = z.read(names[name])
                ok = len(data) == size and zlib.crc32(data) == crc and hashlib.sha1(data).hexdigest() == sha1
                print(f"{'ok      ' if ok else 'BAD     '}{name:15} {region}")
                if not ok:
                    bad += 1
                files[name] = data
    if bad:
        raise SystemExit(f"{bad} ROM(s) missing or wrong: this is not a verified hotd (Revision A) set")
    return files


def build_regions(files):
    out = {}
    for region, (size, roms) in REGIONS.items():
        buf = bytearray(size)
        for name, off, rsize, _, _, method in roms:
            data = files[name]
            if method == W32:
                # file word k lands at off + 4*k: copy its byte 0 and byte 1 with stride 4
                for i in range(2):
                    buf[off + i:off + i + 2 * rsize:4] = data[i::2]
            else:  # SWAP16
                buf[off:off + rsize:2] = data[1::2]
                buf[off + 1:off + rsize:2] = data[0::2]
        out[region] = buf
    md = out["main_data"]
    for dst in range(0x1900000, 0x2000000, 0x100000):
        md[dst:dst + 0x100000] = md[0x1800000:0x1900000]
    return out


if __name__ == "__main__":
    if len(sys.argv) < 3 or sys.argv[1] not in ("verify", "extract") or (sys.argv[1] == "extract" and len(sys.argv) != 4):
        print(__doc__); sys.exit(1)
    roms = read_verified(sys.argv[2])
    print("all ROMs verified (hotd, Revision A)")
    if sys.argv[1] == "extract":
        os.makedirs(sys.argv[3], exist_ok=True)
        for region, data in build_regions(roms).items():
            path = os.path.join(sys.argv[3], region + ".bin")
            with open(path, "wb") as f:
                f.write(data)
            print(f"wrote {path} ({len(data):#x} bytes)")
