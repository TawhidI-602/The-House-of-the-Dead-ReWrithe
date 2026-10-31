#!/usr/bin/env python3
"""
ram_diff.py - compare two work RAM dumps (0x500000, 1 MB) of the whole-game differential (tools/run_diff_rec.ps1's
full_<F>.bin against the runtime's ours_full_<F>.bin, docs/proof.md "Whole-game differential"): the differing byte
ranges, merged over gaps of less than 16 bytes, with the heap block (task / object) each falls in.

  py tools/ram_diff.py <a.bin> <b.bin> [--max N] [--ignore lo-hi,...]
"""
import struct, sys

BASE = 0x500000


def heap_owner(ram, addr):
    """the used heap block holding addr (blocks from 0x543B00: header = size in words, footer the same)"""
    p = 0x543B00
    end = BASE + len(ram)
    while p + 12 <= end:
        w = struct.unpack_from("<i", ram, p - BASE)[0]
        if w == 0:
            return None
        n = abs(w) * 4
        if p <= addr < p + n:
            if w < 0:
                return "free block %06x" % p
            start = p + 12
            r = struct.unpack_from("<I", ram, start - BASE)[0] if start + 4 <= end else 0
            return "block %06x (+0x%x) routine %06x" % (start, addr - start, r)
        p += n
    return None


def main():
    a = sys.argv[1:]
    maxn = 40
    ign = []
    if "--max" in a:
        i = a.index("--max"); maxn = int(a[i + 1]); del a[i:i + 2]
    if "--ignore" in a:
        i = a.index("--ignore")
        for r in a[i + 1].split(","):
            lo, hi = r.split("-"); ign.append((int(lo, 16), int(hi, 16)))
        del a[i:i + 2]
    x = open(a[0], "rb").read(); y = open(a[1], "rb").read()
    runs = []
    i = 0
    n = min(len(x), len(y))
    while i < n:
        if x[i] != y[i]:
            j = i
            k = i
            while k < n and k - j < 16:
                if x[k] != y[k]:
                    j = k
                k += 1
            runs.append((i, j + 1))
            i = j + 1
        else:
            i += 1
    runs = [(s, e) for s, e in runs if not any(lo <= BASE + s <= hi for lo, hi in ign)]
    total = sum(e - s for s, e in runs)
    print("%d differing ranges, %d bytes" % (len(runs), total))
    for s, e in runs[:maxn]:
        owner = heap_owner(x, BASE + s) if BASE + s >= 0x543B00 else ""
        print("  %06x..%06x (%d): %s / %s %s" % (BASE + s, BASE + e - 1, e - s, x[s:min(e, s + 12)].hex(), y[s:min(e, s + 12)].hex(), owner or ""))


if __name__ == "__main__":
    main()
