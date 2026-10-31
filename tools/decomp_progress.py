#!/usr/bin/env python3
"""
decomp_progress.py - how far is the decompilation? Prints the numbers reported to the user after each step.

- Systems: the progress table in docs/decompilation.md, per version: done = 1, partial = 0.5, found = 0.2, n/a ignored.
- Arcade code: bytes of arcade functions (maincpu < CODE_END = 0xA6588, the end of the code) that are paired with a PC function
  (private/decomp/match_functions.tsv), and that carry a name (tools/ghidra/names.tsv).
- Overall = average of the systems score, the paired-code share and the named-code share, with structural names
  (tools/name_routines.py) counted as half (user, 2026-10-14). A rough single figure: paired means "located in both
  versions", named means "understood well enough to give it a name".

  py tools/decomp_progress.py
"""
import os, re

# The i960 code ends at 0xA6588 (last ret of the C library); above it are tables, scripts and models. Ghidra still
# finds ~70 tiny "functions" in that data (0xA91B0..0x1B38FC): not counted (user, 2026-10-21).
CODE_END = 0xA6588
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")


def status_value(text):
    t = text.strip().lower()
    for word, v in (("done", 1.0), ("partial", 0.5), ("found", 0.2)):
        if t.startswith(word):
            return v
    if t.startswith("n/a"):
        return None
    return 0.0


def main():
    rows = []
    for line in open(os.path.join(ROOT, "docs", "decompilation.md"), encoding="utf-8"):
        c = [x.strip() for x in line.strip().strip("|").split("|")]
        if len(c) >= 6 and line.startswith("| ") and not line.startswith("| System") and not set(c[0]) <= set("-"):
            rows.append((c[0], status_value(c[1]), status_value(c[2])))
    vals = [v for _, p, a in rows for v in (p, a) if v is not None]
    systems = sum(vals) / len(vals) if vals else 0
    done = sum(1 for _, p, a in rows if all(v in (None, 1.0) for v in (p, a)))

    funcs = {}
    for line in open(os.path.join(ROOT, "private", "decomp", "arcade", "maincpu_index.tsv"), encoding="utf-8",
                     errors="replace"):
        f = line.split("\t")
        if len(f) > 2 and re.fullmatch("[0-9a-f]{8}", f[0]) and int(f[0], 16) < CODE_END:
            funcs[int(f[0], 16)] = int(f[2])
    # data_<addr>: Ghidra "functions" below CODE_END that are data (strings), named so in names.tsv: not counted
    for line in open(os.path.join(HERE, "ghidra", "names.tsv"), encoding="utf-8"):
        c = line.rstrip("\n").split("\t")
        if c[0] == "func" and len(c) > 3 and c[2] != "-" and c[3].startswith("data_"):
            funcs.pop(int(c[2], 16), None)
    total = sum(funcs.values())
    paired = set()
    for line in open(os.path.join(ROOT, "private", "decomp", "match_functions.tsv"), encoding="utf-8").read().splitlines()[1:]:
        paired.add(int(line.split("\t")[1], 16))
    named, generated, gen = set(), set(), False
    for line in open(os.path.join(HERE, "ghidra", "names.tsv"), encoding="utf-8"):
        c = line.rstrip("\n").split("\t")
        if line.startswith("# BEGIN name_routines.py"):
            gen = True
        elif line.startswith("# END name_routines.py"):
            gen = False
        if c[0] == "func" and len(c) > 3 and c[2] != "-":
            # unused_<addr>: dead code marked, not read: counted like a structural name
            (generated if gen or c[3].startswith("unused_") else named).add(int(c[2], 16))
    pb = sum(n for a, n in funcs.items() if a in paired)
    nb = sum(n for a, n in funcs.items() if a in named)
    gb = sum(n for a, n in funcs.items() if a in generated)
    print(f"systems: {systems * 100:.0f}% ({len(rows)} systems, {done} done in every version that has them)")
    print(f"arcade code paired with the PC: {pb * 100 / total:.0f}% of {total // 1024} KB "
          f"({sum(1 for a in funcs if a in paired)} of {len(funcs)} functions)")
    print(f"arcade code named by hand: {nb * 100 / total:.1f}% ({sum(1 for a in funcs if a in named)} functions); "
          f"structural names (tools/name_routines.py: owner + slot, not read yet): {gb * 100 / total:.1f}% "
          f"({sum(1 for a in funcs if a in generated)} functions)")
    print(f"overall (rough, structural names counted as half): {(systems + pb / total + (nb + gb / 2) / total) * 100 / 3:.0f}% "
          f"(hand names only: {(systems + pb / total + nb / total) * 100 / 3:.0f}%)")


if __name__ == "__main__":
    main()
