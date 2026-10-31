#!/usr/bin/env python3
"""
i960dis.py - small Intel i960 (KB/SB, the Model 2 main CPU) disassembler for reading the House of the Dead program.

Formats (32-bit little-endian words; opcode = bits 24-31):
  CTRL 0x00-0x1F  branch/call, 24-bit signed word displacement
  COBR 0x20-0x3F  compare-and-branch / test: src1 (or literal), src2, 13-bit displacement
  REG  0x58-0x7F  register ops, opcode extended by bits 7-10
  MEM  0x80-0xFF  loads/stores/lda/callx: MEMA (12-bit offset) or MEMB (addressing modes, maybe a displacement word)
Register numbers 0-15 = r0-r15 (r0 pfp, r1 sp, r2 rip), 16-31 = g0-g15 (g15 = fp).
Literal operands (M bits) are shown as plain numbers. Float literals (0.0/1.0) are not decoded.

Usage:
  python i960dis.py <maincpu.bin> <start hex> <end hex>
  python i960dis.py <maincpu.bin> --xref <value hex>     words in the program equal to <value> (absolute address operands)
"""
import sys, struct

REGN = [f"r{i}" for i in range(16)] + [f"g{i}" for i in range(16)]
REGN[0], REGN[1], REGN[2], REGN[31] = "pfp", "sp", "rip", "fp"

CTRL = {0x08: "b", 0x09: "call", 0x0A: "ret", 0x0B: "bal", 0x10: "bno", 0x11: "bg", 0x12: "be", 0x13: "bge", 0x14: "bl",
        0x15: "bne", 0x16: "ble", 0x17: "bo", 0x18: "faultno", 0x19: "faultg", 0x1A: "faulte", 0x1B: "faultge",
        0x1C: "faultl", 0x1D: "faultne", 0x1E: "faultle", 0x1F: "faulto"}
COBR = {0x20: "testno", 0x21: "testg", 0x22: "teste", 0x23: "testge", 0x24: "testl", 0x25: "testne", 0x26: "testle",
        0x27: "testo", 0x30: "bbc", 0x31: "cmpobg", 0x32: "cmpobe", 0x33: "cmpobge", 0x34: "cmpobl", 0x35: "cmpobne",
        0x36: "cmpoble", 0x37: "bbs", 0x38: "cmpibno", 0x39: "cmpibg", 0x3A: "cmpibe", 0x3B: "cmpibge", 0x3C: "cmpibl",
        0x3D: "cmpibne", 0x3E: "cmpible", 0x3F: "cmpibo"}
REG = {0x580: "notbit", 0x581: "and", 0x582: "andnot", 0x583: "setbit", 0x584: "notand", 0x586: "xor", 0x587: "or",
       0x588: "nor", 0x589: "xnor", 0x58A: "not", 0x58B: "ornot", 0x58C: "clrbit", 0x58D: "notor", 0x58E: "nand",
       0x58F: "alterbit", 0x590: "addo", 0x591: "addi", 0x592: "subo", 0x593: "subi", 0x598: "shro", 0x59A: "shrdi",
       0x59B: "shri", 0x59C: "shlo", 0x59D: "rotate", 0x59E: "shli", 0x5A0: "cmpo", 0x5A1: "cmpi", 0x5A2: "concmpo",
       0x5A3: "concmpi", 0x5A4: "cmpinco", 0x5A5: "cmpinci", 0x5A6: "cmpdeco", 0x5A7: "cmpdeci", 0x5AC: "scanbyte",
       0x5AE: "chkbit", 0x5B0: "addc", 0x5B2: "subc", 0x5CC: "mov", 0x5DC: "movl", 0x5EC: "movt", 0x5FC: "movq",
       0x600: "synmov", 0x601: "synmovl", 0x602: "synmovq", 0x610: "atmod", 0x612: "atadd", 0x615: "synld",
       0x640: "spanbit", 0x641: "scanbit", 0x642: "daddc", 0x643: "dsubc", 0x644: "dmovt", 0x645: "modac",
       0x650: "modify", 0x651: "extract", 0x654: "modtc", 0x655: "modpc", 0x658: "intctl", 0x660: "calls",
       0x66B: "mark", 0x66C: "fmark", 0x66D: "flushreg", 0x66F: "syncf", 0x670: "emul", 0x671: "ediv",
       0x674: "cvtir", 0x675: "cvtilr", 0x676: "scalerl", 0x677: "scaler",
       0x680: "atanr", 0x681: "logepr", 0x682: "logr", 0x683: "remr", 0x684: "cmpor", 0x685: "cmpr", 0x688: "sqrtr",
       0x689: "expr", 0x68A: "logbnr", 0x68B: "roundr", 0x68C: "sinr", 0x68D: "cosr", 0x68E: "tanr", 0x68F: "classr",
       0x690: "atanrl", 0x691: "logeprl", 0x692: "logrl", 0x693: "remrl", 0x694: "cmporl", 0x695: "cmprl",
       0x698: "sqrtrl", 0x699: "exprl", 0x69A: "logbnrl", 0x69B: "roundrl", 0x69C: "sinrl", 0x69D: "cosrl",
       0x69E: "tanrl", 0x69F: "classrl", 0x6C0: "cvtri", 0x6C1: "cvtril", 0x6C2: "cvtzri", 0x6C3: "cvtzril",
       0x6C9: "movr", 0x6D9: "movrl", 0x6E1: "movre", 0x6E2: "cpysre", 0x6E3: "cpyrsre",
       0x701: "mulo", 0x708: "remo", 0x70B: "divo", 0x741: "muli", 0x748: "remi", 0x749: "modi", 0x74B: "divi",
       0x78B: "divr", 0x78C: "mulr", 0x78D: "subr", 0x78F: "addr", 0x79B: "divrl", 0x79C: "mulrl", 0x79D: "subrl",
       0x79F: "addrl"}
MEM = {0x80: "ldob", 0x82: "stob", 0x84: "bx", 0x85: "balx", 0x86: "callx", 0x88: "ldos", 0x8A: "stos", 0x8C: "lda",
       0x90: "ld", 0x92: "st", 0x98: "ldl", 0x9A: "stl", 0xA0: "ldt", 0xA2: "stt", 0xB0: "ldq", 0xB2: "stq",
       0xC0: "ldib", 0xC2: "stib", 0xC8: "ldis", 0xCA: "stis"}
FLOAT_OPS = {k for k, v in REG.items() if v.endswith("r") or v.endswith("rl") or v.startswith("cvt") or v.startswith("cpy")
             or v in ("scaler", "scalerl", "classr", "classrl", "movre")}


def reg_or_lit(n, lit, is_float=False):
    if not lit:
        return REGN[n]
    if is_float:
        return {0x10: "0.0", 0x16: "1.0"}.get(n, f"fp{n}")
    return str(n)


def dis(code, base, start, end):
    """Yields (address, length, text)."""
    a = start
    while a < end:
        w = struct.unpack_from("<I", code, a - base)[0]
        op = w >> 24
        n = 4
        if op < 0x20:
            d = w & 0xFFFFFC
            if d & 0x800000:
                d -= 0x1000000
            name = CTRL.get(op, f".word {w:#010x}")
            txt = name if op == 0x0A else f"{name} {a + d:#x}" if op in CTRL else name
        elif op < 0x40:
            s1, s2, m1 = (w >> 19) & 31, (w >> 14) & 31, (w >> 13) & 1
            d = w & 0x1FFC
            if d & 0x1000:
                d -= 0x2000
            name = COBR.get(op, f".word {w:#010x}")
            if op < 0x28:
                txt = f"{name} {REGN[s1]}"
            else:
                txt = f"{name} {reg_or_lit(s1, m1)}, {REGN[s2]}, {a + d:#x}"
        elif op < 0x80:
            ext = (op << 4) | ((w >> 7) & 15)
            dst, s2, s1 = (w >> 19) & 31, (w >> 14) & 31, w & 31
            m3, m2, m1 = (w >> 13) & 1, (w >> 12) & 1, (w >> 11) & 1
            name = REG.get(ext)
            if name is None:
                txt = f".word {w:#010x}  ; reg {ext:#x}"
            else:
                f = ext in FLOAT_OPS
                a1, a2, a3 = reg_or_lit(s1, m1, f), reg_or_lit(s2, m2, f), (reg_or_lit(dst, m3, f) if f else REGN[dst])
                if name in ("mov", "movl", "movt", "movq", "not", "movr", "movrl", "movre", "sqrtr", "sqrtrl", "cvtir",
                            "cvtilr", "cvtri", "cvtril", "cvtzri", "cvtzril", "roundr", "roundrl", "sinr", "cosr", "tanr",
                            "expr", "logbnr", "classr", "spanbit", "scanbit"):
                    txt = f"{name} {a1}, {a3}"
                elif name.startswith("cmp") or name in ("chkbit", "scanbyte"):
                    txt = f"{name} {a1}, {a2}"
                elif name in ("ret", "flushreg", "mark", "fmark", "syncf"):
                    txt = name
                elif name == "calls":
                    txt = f"calls {a1}"
                else:
                    txt = f"{name} {a1}, {a2}, {a3}"
        else:
            name = MEM.get(op, f"mem{op:#x}")
            r = REGN[(w >> 19) & 31]
            abase = REGN[(w >> 14) & 31]
            if not (w >> 12) & 1:   # MEMA
                off = w & 0xFFF
                ea = f"{off:#x}({abase})" if (w >> 13) & 1 else f"{off:#x}"
            else:
                mode = (w >> 10) & 15
                scale = 1 << ((w >> 7) & 7)
                idx = REGN[w & 31]
                disp = None
                if mode in (5, 12, 13, 14, 15):
                    disp = struct.unpack_from("<I", code, a - base + 4)[0]
                    n = 8
                if mode == 4:
                    ea = f"({abase})"
                elif mode == 5:
                    ea = f"{(a + 8 + disp) & 0xFFFFFFFF:#x}(ip)"
                elif mode == 7:
                    ea = f"({abase})[{idx}*{scale}]"
                elif mode == 12:
                    ea = f"{disp:#x}"
                elif mode == 13:
                    ea = f"{disp:#x}({abase})"
                elif mode == 14:
                    ea = f"{disp:#x}[{idx}*{scale}]"
                elif mode == 15:
                    ea = f"{disp:#x}({abase})[{idx}*{scale}]"
                else:
                    ea = f"?mode{mode}"
            if name in ("bx", "balx", "callx"):
                txt = f"{name} {ea}" + (f", {r}" if name == "balx" else "")
            elif name.startswith("st"):
                txt = f"{name} {r}, {ea}"
            else:
                txt = f"{name} {ea}, {r}"
        yield a, n, txt
        a += n


def main():
    code = open(sys.argv[1], "rb").read()
    if sys.argv[2] == "--xref":
        v = struct.pack("<I", int(sys.argv[3], 16))
        k = code.find(v)
        while k >= 0:
            print(f"{k:#x}")
            k = code.find(v, k + 1)
        return
    start, end = int(sys.argv[2], 16), int(sys.argv[3], 16)
    for a, n, txt in dis(code, 0, start, end):
        w = struct.unpack_from("<I", code, a)[0]
        print(f"{a:06x}: {w:08x}{'' if n == 4 else ' +'}  {txt}")


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print(__doc__); sys.exit(1)
    main()
