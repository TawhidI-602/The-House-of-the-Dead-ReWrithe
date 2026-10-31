"""Lists every routine marked `UNVERIFIED (teaser)` in src/ (written for the teaser without a capture proof).

A marker names its routine in one of three ways, tried in order:
  1. a declaration on the same line:            void walk_to(...);   // 0x1D2D0 UNVERIFIED (teaser)
  2. the comment block it ends or starts:       // 0x31E60 attack_slot_take_waiting: ... UNVERIFIED (teaser)
  3. otherwise it is a file-level note (whole file / section unverified): listed as "(file note)".
Routines found once per (name, address); the arcade address when the marker gives one.

    py tools/unverified_list.py [--tsv] [--counts]
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
MARK = "UNVERIFIED (teaser)"
DECL = re.compile(r"^\s*(?:virtual\s+)?[\w:<>\*&\s]+?\b(~?\w+)\s*\([^;{]*\)\s*(?:const\s*)?(?:override\s*)?[;{].*//\s*(0x[0-9A-Fa-f]+)?")
DECL_NOADDR = re.compile(r"^\s*(?:virtual\s+)?[\w:<>\*&\s]+?\b(\w+)\s*\([^;{]*\)\s*(?:const\s*)?(?:override\s*)?[;{]")
HEAD = re.compile(r"//\s*(0x[0-9A-Fa-f]+)(?:\s*/\s*0x[0-9A-Fa-f]+)*\s+(\w+)")
CASE = re.compile(r"case\s+(0x[0-9A-Fa-f]+):\s*(\w+)")


def scan():
    out = []
    for f in sorted((ROOT / "src").rglob("*")):
        if f.suffix not in (".cpp", ".h", ".inc") or not f.is_file():
            continue
        lines = f.read_text(encoding="utf-8", errors="replace").splitlines()
        rel = f.relative_to(ROOT).as_posix()
        for i, line in enumerate(lines):
            if MARK not in line:
                continue
            name = addr = None
            m = DECL.match(line) or DECL_NOADDR.match(line)
            if m and "//" in line and line.index("//") > m.start(1):
                name = m.group(1)
                am = re.search(r"//\s*(0x[0-9A-Fa-f]+)", line)
                addr = am.group(1) if am else None
            if name is None:
                m = CASE.search(line)
                if m:
                    addr, name = m.group(1), m.group(2)
            if name is None:
                # the comment block around the marker: its first "// 0xADDR name" line
                j = i
                while j > 0 and lines[j - 1].lstrip().startswith("//"):
                    j -= 1
                k = i
                while k + 1 < len(lines) and lines[k + 1].lstrip().startswith("//"):
                    k += 1
                for t in lines[j:k + 1]:
                    m = HEAD.search(t)
                    if m:
                        addr, name = m.group(1), m.group(2)
                        break
            out.append((rel, name or "(file note)", (addr or "-").upper().replace("0X", "0x"), i + 1))
    return out


def main():
    rows = scan()
    seen = set()
    uniq = []
    for r in rows:
        key = (r[1], r[2]) if r[1] != "(file note)" else (r[0], r[3])
        if key in seen:
            continue
        seen.add(key)
        uniq.append(r)
    routines = [r for r in uniq if r[1] != "(file note)"]
    if "--counts" in sys.argv:
        files = {}
        for r in routines:
            files[r[0]] = files.get(r[0], 0) + 1
        for f, n in sorted(files.items()):
            print(f"{n:4}  {f}")
    else:
        for r in uniq:
            if "--tsv" in sys.argv:
                print(f"{r[0]}\t{r[1]}\t{r[2]}\t{r[3]}")
            else:
                print(f"{r[0]}:{r[3]}  {r[1]}  {r[2]}")
    print(f"{len(routines)} UNVERIFIED (teaser) routines, {len(uniq) - len(routines)} file / section notes "
          f"({len(rows)} markers)")


if __name__ == "__main__":
    main()
