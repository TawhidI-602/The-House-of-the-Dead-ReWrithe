#!/usr/bin/env python3
r"""
evt_dis.py - disassemble the PC event scripts (EVT\ALLSTAGE\EVTALL.RBH), formats.md §29.

Layout: "PIFF" container, BODY data at file offset 0x28; all offsets below are relative to it.
  BODY +0: u32 offsets of 7 event lists (Chapters 1-4, then three special lists), ended by -1.
  Event list: u32 offsets of events, ended by -1.
  Event: u32 section offsets (resume points: each follows a NEXTEV), ended by -1; then code; after the last NEXTEV the
  event's data tables (enemy tables etc., referenced by the code's offsets).
  Instruction: int32 opcode (names from THOTD.EXE's debug table, §28) + int32 operands; operand counts in SPEC below
  (from the handlers' code, arcade 0x66A10.. / PC 0x4C0CF0..; first worked out from the data: every one of the 60 events decodes to its end, 399 of 467 section offsets fall on instruction
  starts, the others point into code shared with another event). "L" = a list ended by -1.
  CCTB (0x3C) has sub-operations: 64 start end a b (play the event's camera path from frame start to end; start -1 =
  from the current frame), 33 x y, and 16-21 with one value.

Usage:
  python evt_dis.py <EVTALL.RBH> <exe for the opcode names, THOTD.EXE> <list 0-6> <event>
"""
import struct, re, sys

d = W = NAMES = None


def load(evt_path, exe_path):
    global d, W, NAMES
    d = open(evt_path, "rb").read()
    W = struct.unpack_from(f"<{(len(d) - B) // 4}I", d, B)
    ex = open(exe_path, "rb").read()
    NAMES = {int(m.group(1), 16): m.group(2).decode().split("/")[0]
             for m in re.finditer(rb"0x([0-9a-fA-F]{2}) : ([ -~]{1,40})", ex)}


B = 0x28

def s32(v): return v - 2**32 if v >= 2**31 else v

# operand specs: int = fixed count; "L" = list ending in -1; callable(words, i) -> count
def cctb(ws, i):
    sub = ws[i + 1]
    return {64: 5, 33: 3}.get(sub, 2) - 1 + 1   # sub-op + operands
SPEC = {90: 1, 20: 1, 72: 1, 73: 1, 70: 1, 71: 1, 68: 2, 82: 1, 11: "L", 10: "L", 12: "L", 9: "L",
        83: 0, 81: 1, 84: 1, 80: 1, 75: 1, 88: 1, 15: 1, 60: cctb, 93: 0, 16: 0, 77: 1, 76: 2, 85: 1, 89: 2, 87: 2,
        1: "L", 2: "L", 3: "L", 4: "L", 5: "L", 6: "L", 7: "L", 8: "L", 50: "L", 51: "L", 40: 1, 79: 1, 92: 0,
        91: 1, 66: 1, 67: 1, 52: 2, 26: 2, 65: 1}


def events(lst):
    heads, o = [], 0
    while W[o] != 0xFFFFFFFF: heads.append(W[o]); o += 1
    evs, o = [], heads[lst] // 4
    while W[o] != 0xFFFFFFFF: evs.append(W[o]); o += 1
    return evs

def dis(start, end, out=print):
    i = start // 4
    sects = []
    while W[i] != 0xFFFFFFFF: sects.append(W[i]); i += 1
    i += 1
    out(f"sections {sects}")
    while i < end // 4:
        op = W[i]
        spec = SPEC.get(op)
        rel = 4 * i - start
        mark = " <== section" if rel in sects else ""
        if spec is None and i > 0 and W[i - 1] == 93:
            out(f"{rel:6d}: (end of code; data follows)")
            return True
        if spec is None:
            out(f"{rel:6d}: ??? {op} ({NAMES.get(op, '')}) next: {[s32(x) for x in W[i + 1:i + 8]]}{mark}")
            return False
        if spec == "L":
            j = i + 1
            while W[j] != 0xFFFFFFFF: j += 1
            args = [s32(x) for x in W[i + 1:j]]; n = j - i + 1
        else:
            k = spec(W, i) if callable(spec) else spec
            args = [s32(x) for x in W[i + 1:i + 1 + k]]; n = 1 + k
        out(f"{rel:6d}: {NAMES.get(op, '?'):12s} {args}{mark}")
        i += n
    return True



if __name__ == "__main__":
    if len(sys.argv) != 5:
        print(__doc__); sys.exit(1)
    load(sys.argv[1], sys.argv[2])
    lst, ev = int(sys.argv[3]), int(sys.argv[4])
    evs = events(lst)
    end = evs[ev + 1] if ev + 1 < len(evs) else evs[ev] + 4000
    dis(evs[ev], end)
