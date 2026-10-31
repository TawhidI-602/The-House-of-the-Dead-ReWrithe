// A small 32-bit x86 + x87 interpreter for `hotd_proof pmmaths` (docs/proof.md): it runs THOTD.EXE's own maths helpers
// (the matrix stack copies, pc_vec_angles copies, pc_atan2, the table square roots ...) on a PC memory image built
// from the EXE, so the PC Mode copies in port/pm_maths.h can be checked against the PC's instructions themselves
// (nothing is captured, THOTD.EXE is not run). Only what those leaf routines use: the integer instructions of Watcom
// code, the x87 with its precision control (the PC runs at 24 bits, control word 0x107F: every result rounded to a
// float; the stack is kept as doubles, so float operands round exactly once) and rounding control for fist / fistp.
// Unknown opcodes stop the run with an error (never a guess).
#pragma once

#include "port/mem.h"
#include "port/pm_maths.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace hotd::proof {

class X86 {
public:
    explicit X86(hotd::port::Mem& m) : mem(m) {}
    hotd::port::Mem& mem;
    uint32_t r[8] = {};   // eax ecx edx ebx esp ebp esi edi
    enum { EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI };
    uint32_t eip = 0;
    bool cf = false, zf = false, sf = false, of = false, pf = false;
    double st[8] = {};
    int top = 0;          // st(i) = st[(top + i) & 7]
    uint16_t cw = 0x107F, sw = 0;
    std::string error;
    long steps = 0;
    uint32_t stopAt = 0xFFFFFFFFu;   // a run also ends when it reaches this address (a callee not wanted)

    // calls `addr` with the Watcom register arguments (eax, edx, ebx, ecx) and the stack words (pushed right to left
    // so stackArgs[0] is at esp + 4); runs until it returns. False on an error (error says what)
    bool call(uint32_t addr, const uint32_t regs[4], const uint32_t* stackArgs, int nstack, uint32_t stackTop)
    {
        r[EAX] = regs[0];
        r[EDX] = regs[1];
        r[EBX] = regs[2];
        r[ECX] = regs[3];
        r[ESP] = stackTop;
        for (int i = nstack - 1; i >= 0; --i)
            push(stackArgs[i]);
        push(kReturn);
        eip = addr;
        steps = 0;
        error.clear();
        while (eip != kReturn && eip != stopAt && error.empty()) {
            if (++steps > 2000000) {
                error = "too many steps";
                break;
            }
            step();
        }
        return error.empty();
    }
    double& ST(int i) { return st[(top + i) & 7]; }
    float st0f() { return float(ST(0)); }

private:
    static constexpr uint32_t kReturn = 0xFFFFFFF0u;
    uint32_t u32(uint32_t a)
    {
        uint32_t v = 0;
        for (int k = 0; k < 4; ++k)
            v |= uint32_t(*mem.ptr(a + uint32_t(k))) << (8 * k);
        return v;
    }
    void w32(uint32_t a, uint32_t v)
    {
        for (int k = 0; k < 4; ++k)
            *mem.ptr(a + uint32_t(k)) = uint8_t(v >> (8 * k));
    }
    uint8_t u8(uint32_t a) { return *mem.ptr(a); }
    uint16_t u16(uint32_t a) { return uint16_t(u8(a) | (u8(a + 1) << 8)); }
    void w16(uint32_t a, uint16_t v)
    {
        *mem.ptr(a) = uint8_t(v);
        *mem.ptr(a + 1) = uint8_t(v >> 8);
    }
    uint8_t fetch8() { return u8(eip++); }
    uint32_t fetch32()
    {
        const uint32_t v = u32(eip);
        eip += 4;
        return v;
    }
    void push(uint32_t v)
    {
        r[ESP] -= 4;
        w32(r[ESP], v);
    }
    uint32_t pop()
    {
        const uint32_t v = u32(r[ESP]);
        r[ESP] += 4;
        return v;
    }
    void fail(const char* what)
    {
        char b[160];
        std::snprintf(b, sizeof b, "%s at 0x%X", what, eip);
        error = b;
    }

    // ModRM: either a register (isReg, reg index) or a memory address
    struct Rm {
        bool isReg = false;
        int reg = 0;
        uint32_t addr = 0;
    };
    int modrmReg = 0;
    Rm decode(uint32_t segBase = 0)
    {
        (void)segBase;
        const uint8_t m = fetch8();
        const int mod = m >> 6, rm = m & 7;
        modrmReg = (m >> 3) & 7;
        Rm o;
        if (mod == 3) {
            o.isReg = true;
            o.reg = rm;
            return o;
        }
        uint32_t a = 0;
        if (rm == 4) {
            const uint8_t sib = fetch8();
            const int sc = sib >> 6, idx = (sib >> 3) & 7, base = sib & 7;
            if (base == 5 && mod == 0)
                a = fetch32();
            else
                a = r[base];
            if (idx != 4)
                a += r[idx] << sc;
        } else if (rm == 5 && mod == 0) {
            a = fetch32();
        } else {
            a = r[rm];
        }
        if (mod == 1)
            a += uint32_t(int32_t(int8_t(fetch8())));
        else if (mod == 2)
            a += fetch32();
        o.addr = a;
        return o;
    }
    uint32_t get32(const Rm& o) { return o.isReg ? r[o.reg] : u32(o.addr); }
    void set32(const Rm& o, uint32_t v)
    {
        if (o.isReg)
            r[o.reg] = v;
        else
            w32(o.addr, v);
    }
    uint8_t reg8(int i) { return i < 4 ? uint8_t(r[i]) : uint8_t(r[i - 4] >> 8); }
    void setReg8(int i, uint8_t v)
    {
        if (i < 4)
            r[i] = (r[i] & ~0xFFu) | v;
        else
            r[i - 4] = (r[i - 4] & ~0xFF00u) | (uint32_t(v) << 8);
    }
    uint8_t get8(const Rm& o) { return o.isReg ? reg8(o.reg) : u8(o.addr); }
    void set8(const Rm& o, uint8_t v)
    {
        if (o.isReg)
            setReg8(o.reg, v);
        else
            *mem.ptr(o.addr) = v;
    }
    uint16_t get16(const Rm& o) { return o.isReg ? uint16_t(r[o.reg]) : u16(o.addr); }
    void set16(const Rm& o, uint16_t v)
    {
        if (o.isReg)
            r[o.reg] = (r[o.reg] & ~0xFFFFu) | v;
        else
            w16(o.addr, v);
    }

    void flagsLogic(uint32_t v)
    {
        cf = of = false;
        zf = v == 0;
        sf = (v >> 31) != 0;
        setPf(v);
    }
    void setPf(uint32_t v)
    {
        uint8_t b = uint8_t(v);
        b ^= b >> 4;
        b ^= b >> 2;
        b ^= b >> 1;
        pf = (b & 1) == 0;
    }
    // ALU op 0 add, 1 or, 2 adc, 3 sbb, 4 and, 5 sub, 6 xor, 7 cmp
    uint32_t alu(int op, uint32_t a, uint32_t b)
    {
        uint64_t w;
        uint32_t v;
        switch (op) {
        case 0:
        case 2: {
            const uint32_t c = (op == 2 && cf) ? 1u : 0u;
            w = uint64_t(a) + b + c;
            v = uint32_t(w);
            cf = (w >> 32) != 0;
            of = ((~(a ^ b) & (a ^ v)) >> 31) != 0;
            break;
        }
        case 3:
        case 5:
        case 7: {
            const uint32_t c = (op == 3 && cf) ? 1u : 0u;
            w = uint64_t(a) - b - c;
            v = uint32_t(w);
            cf = uint64_t(a) < uint64_t(b) + c;
            of = (((a ^ b) & (a ^ v)) >> 31) != 0;
            break;
        }
        case 1: v = a | b; flagsLogic(v); return v;
        case 4: v = a & b; flagsLogic(v); return v;
        default: v = a ^ b; flagsLogic(v); return v;
        }
        zf = v == 0;
        sf = (v >> 31) != 0;
        setPf(v);
        return v;
    }
    uint32_t shift(int op, uint32_t v, int n)
    {
        n &= 31;
        if (n == 0)
            return v;
        uint32_t res;
        switch (op) {
        case 0: res = (v << n) | (v >> (32 - n)); cf = res & 1; return res;                  // rol
        case 1: res = (v >> n) | (v << (32 - n)); cf = (res >> 31) != 0; return res;         // ror
        case 4:
        case 6: cf = ((v >> (32 - n)) & 1) != 0; res = v << n; break;                       // shl
        case 5: cf = ((v >> (n - 1)) & 1) != 0; res = v >> n; break;                        // shr
        case 7: cf = ((int32_t(v) >> (n - 1)) & 1) != 0; res = uint32_t(int32_t(v) >> n); break;   // sar
        default: fail("rcl / rcr"); return v;
        }
        zf = res == 0;
        sf = (res >> 31) != 0;
        of = false;
        setPf(res);
        return res;
    }
    bool cond(int c)
    {
        bool v;
        switch (c >> 1) {
        case 0: v = of; break;
        case 1: v = cf; break;
        case 2: v = zf; break;
        case 3: v = cf || zf; break;
        case 4: v = sf; break;
        case 5: v = pf; break;
        case 6: v = sf != of; break;
        default: v = zf || (sf != of); break;
        }
        return (c & 1) ? !v : v;
    }

    // x87
    bool single() const { return ((cw >> 8) & 3) == 0; }
    double rnd(double v) const
    {
        if (((cw >> 8) & 3) == 0)
            return double(float(v));
        return v;
    }
    void fpush(double v)
    {
        top = (top - 1) & 7;
        st[top] = v;
    }
    double fpop()
    {
        const double v = st[top];
        top = (top + 1) & 7;
        return v;
    }
    double arith(int op, double a, double b)   // op: 0 add, 1 mul, 4 sub (a - b), 5 subr (b - a), 6 div, 7 divr
    {
        switch (op) {
        case 0: return single() ? double(hotd::port::pm::add_round(a, b)) : a + b;
        case 1: return rnd(a * b);
        case 4: return single() ? double(hotd::port::pm::add_round(a, -b)) : a - b;
        case 5: return single() ? double(hotd::port::pm::add_round(b, -a)) : b - a;
        case 6: return rnd(a / b);
        default: return rnd(b / a);
        }
    }
    void fcom(double a, double b)
    {
        sw &= uint16_t(~0x4500);
        if (std::isnan(a) || std::isnan(b))
            sw |= 0x4500;
        else if (a < b)
            sw |= 0x0100;
        else if (a == b)
            sw |= 0x4000;
    }
    int64_t fround(double v)
    {
        switch ((cw >> 10) & 3) {
        case 0: return int64_t(std::nearbyint(v));
        case 1: return int64_t(std::floor(v));
        case 2: return int64_t(std::ceil(v));
        default: return int64_t(std::trunc(v));
        }
    }
    int32_t fist32(double v)
    {
        if (!(v > -2147483649.0 && v < 2147483648.0) || std::isnan(v))
            return INT32_MIN;
        const int64_t i = fround(v);
        if (i < INT32_MIN || i > INT32_MAX)
            return INT32_MIN;
        return int32_t(i);
    }
    float f32at(uint32_t a)
    {
        const uint32_t b = u32(a);
        float f;
        std::memcpy(&f, &b, 4);
        return f;
    }
    double f64at(uint32_t a)
    {
        const uint64_t b = uint64_t(u32(a)) | (uint64_t(u32(a + 4)) << 32);
        double d;
        std::memcpy(&d, &b, 8);
        return d;
    }
    void wf32(uint32_t a, double v)
    {
        const float f = float(v);
        uint32_t b;
        std::memcpy(&b, &f, 4);
        w32(a, b);
    }
    void wf64(uint32_t a, double v)
    {
        uint64_t b;
        std::memcpy(&b, &v, 8);
        w32(a, uint32_t(b));
        w32(a + 4, uint32_t(b >> 32));
    }

    void x87(uint8_t op)
    {
        const uint8_t m = u8(eip);
        if (m >= 0xC0) {
            ++eip;
            const int i = m & 7, sub = (m >> 3) & 7;
            switch (op) {
            case 0xD8:
                if (sub == 2 || sub == 3) {
                    fcom(ST(0), ST(i));
                    if (sub == 3)
                        fpop();
                } else
                    ST(0) = arith(sub, ST(0), ST(i));
                return;
            case 0xD9:
                if (sub == 0) {
                    const double v = ST(i);
                    fpush(v);
                } else if (sub == 1) {
                    std::swap(ST(0), ST(i));
                } else if (m == 0xE0)
                    ST(0) = -ST(0);
                else if (m == 0xE1)
                    ST(0) = std::fabs(ST(0));
                else if (m == 0xE4)
                    fcom(ST(0), 0.0);
                else if (m == 0xE8)
                    fpush(1.0);
                else if (m == 0xEE)
                    fpush(0.0);
                else if (m == 0xFA)
                    ST(0) = rnd(std::sqrt(ST(0)));
                else if (m == 0xFC) {   // frndint by the rounding control (__CHP: chop)
                    const double v = ST(0);
                    switch ((cw >> 10) & 3) {
                    case 0: ST(0) = std::nearbyint(v); break;
                    case 1: ST(0) = std::floor(v); break;
                    case 2: ST(0) = std::ceil(v); break;
                    default: ST(0) = std::trunc(v); break;
                    }
                }
                else if (m == 0xD0) {
                } else
                    fail("x87 D9 register form");
                return;
            case 0xDC:
            case 0xDE: {
                // st(i) = st(i) op st0; DC / DE swap the sense of sub and div (E0 fsubr, E8 fsub, F0 fdivr, F8 fdiv)
                if (op == 0xDE && m == 0xD9) {
                    fcom(ST(0), ST(1));
                    fpop();
                    fpop();
                    return;
                }
                int o = sub;
                if (o == 4)
                    o = 5;
                else if (o == 5)
                    o = 4;
                else if (o == 6)
                    o = 7;
                else if (o == 7)
                    o = 6;
                if (o == 2 || o == 3) {
                    fail("x87 DC / DE compare form");
                    return;
                }
                ST(i) = arith(o, ST(i), ST(0));
                if (op == 0xDE)
                    fpop();
                return;
            }
            case 0xDD:
                if (sub == 3) {   // fstp st(i)
                    ST(i) = ST(0);
                    fpop();
                } else if (sub == 2)
                    ST(i) = ST(0);
                else if (sub == 0) {
                } else
                    fail("x87 DD register form");
                return;
            case 0xDF:
                if (m == 0xE0) {
                    sw = uint16_t((sw & ~0x3800) | (top << 11));
                    r[EAX] = (r[EAX] & ~0xFFFFu) | sw;
                } else
                    fail("x87 DF register form");
                return;
            case 0xDB:
                if (m == 0xE2 || m == 0xE3)
                    return;   // fnclex / fninit (the stack is not checked)
                fail("x87 DB register form");
                return;
            default: fail("x87 register form"); return;
            }
        }
        const Rm o = decode();
        const int sub = modrmReg;
        switch (op) {
        case 0xD8: {
            const double v = f32at(o.addr);
            if (sub == 2 || sub == 3) {
                fcom(ST(0), v);
                if (sub == 3)
                    fpop();
            } else
                ST(0) = arith(sub, ST(0), v);
            return;
        }
        case 0xDC: {
            const double v = f64at(o.addr);
            if (sub == 2 || sub == 3) {
                fcom(ST(0), v);
                if (sub == 3)
                    fpop();
            } else
                ST(0) = arith(sub, ST(0), v);
            return;
        }
        case 0xDA: {
            const double v = double(int32_t(u32(o.addr)));
            if (sub == 2 || sub == 3) {
                fcom(ST(0), v);
                if (sub == 3)
                    fpop();
            } else
                ST(0) = arith(sub, ST(0), v);
            return;
        }
        case 0xD9:
            switch (sub) {
            case 0: fpush(f32at(o.addr)); return;
            case 2: wf32(o.addr, ST(0)); return;
            case 3: wf32(o.addr, fpop()); return;
            case 5: cw = u16(o.addr); return;
            case 7: w16(o.addr, cw); return;
            default: fail("x87 D9 memory form"); return;
            }
        case 0xDB:
            switch (sub) {
            case 0: fpush(double(int32_t(u32(o.addr)))); return;
            case 2: w32(o.addr, uint32_t(fist32(ST(0)))); return;
            case 3: w32(o.addr, uint32_t(fist32(fpop()))); return;
            default: fail("x87 DB memory form"); return;
            }
        case 0xDD:
            switch (sub) {
            case 0: fpush(f64at(o.addr)); return;
            case 2: wf64(o.addr, ST(0)); return;
            case 3: wf64(o.addr, fpop()); return;
            case 7: w16(o.addr, sw); return;
            default: fail("x87 DD memory form"); return;
            }
        case 0xDF:
            switch (sub) {
            case 0: fpush(double(int16_t(u16(o.addr)))); return;
            case 5: fpush(double(int64_t(uint64_t(u32(o.addr)) | (uint64_t(u32(o.addr + 4)) << 32)))); return;
            case 7: {
                const double v = fpop();
                const int64_t i = std::isnan(v) ? INT64_MIN : fround(v);
                w32(o.addr, uint32_t(uint64_t(i)));
                w32(o.addr + 4, uint32_t(uint64_t(i) >> 32));
                return;
            }
            default: fail("x87 DF memory form"); return;
            }
        case 0xDE: {
            const double v = double(int16_t(u16(o.addr)));
            ST(0) = arith(sub, ST(0), v);
            return;
        }
        default: fail("x87 memory form"); return;
        }
    }

    void step()
    {
        uint8_t op = fetch8();
        bool o16 = false, rep = false;
        for (;;) {
            if (op == 0x66)
                o16 = true;
            else if (op == 0xF3 || op == 0xF2)
                rep = true;
            else if (op == 0x9B || op == 0x2E || op == 0x3E || op == 0x26 || op == 0x36) {
                // fwait; the cs / ds / es / ss overrides (the flat model: jump tables read through cs)
            } else
                break;
            op = fetch8();
        }
        if (op >= 0xD8 && op <= 0xDF) {
            x87(op);
            return;
        }
        if (o16 && op != 0x89 && op != 0x8B && op != 0xC7 && op != 0x81 && op != 0x83 && op != 0x3B && op != 0x39 &&
            op != 0x85) {
            fail("operand-size prefix");
            return;
        }
        if (op < 0x40 && (op & 7) < 6) {   // the 8 ALU ops in their 6 forms
            const int a = op >> 3, form = op & 7;
            if (form == 0 || form == 2) {   // r/m8, r8 / r8, r/m8
                const Rm o = decode();
                const uint8_t x = get8(o), y = reg8(modrmReg);
                const uint32_t v = form == 0 ? alu(a, uint32_t(x) << 24, uint32_t(y) << 24) >> 24
                                             : alu(a, uint32_t(y) << 24, uint32_t(x) << 24) >> 24;
                if (a != 7) {
                    if (form == 0)
                        set8(o, uint8_t(v));
                    else
                        setReg8(modrmReg, uint8_t(v));
                }
                if (a == 2 || a == 3)
                    fail("8-bit adc / sbb");
                return;
            }
            if (form == 1 || form == 3) {
                const Rm o = decode();
                if (o16) {
                    const uint32_t x = uint32_t(get16(o)) << 16, y = (r[modrmReg] & 0xFFFF) << 16;
                    const uint32_t v = (form == 1 ? alu(a, x, y) : alu(a, y, x)) >> 16;
                    if (a != 7) {
                        if (form == 1)
                            set16(o, uint16_t(v));
                        else
                            r[modrmReg] = (r[modrmReg] & ~0xFFFFu) | v;
                    }
                    return;
                }
                const uint32_t x = get32(o), y = r[modrmReg];
                if (form == 1) {
                    const uint32_t v = alu(a, x, y);
                    if (a != 7)
                        set32(o, v);
                } else {
                    const uint32_t v = alu(a, y, x);
                    if (a != 7)
                        r[modrmReg] = v;
                }
                return;
            }
            if (form == 4) {   // al, imm8
                const uint8_t i = fetch8();
                const uint32_t v = alu(a, (r[EAX] & 0xFF) << 24, uint32_t(i) << 24) >> 24;
                if (a != 7)
                    r[EAX] = (r[EAX] & ~0xFFu) | v;
                return;
            }
            const uint32_t i = fetch32();   // eax, imm32
            const uint32_t v = alu(a, r[EAX], i);
            if (a != 7)
                r[EAX] = v;
            return;
        }
        if (op >= 0x40 && op <= 0x4F) {   // inc / dec r32 (cf kept)
            const bool c = cf;
            r[op & 7] = alu(op < 0x48 ? 0 : 5, r[op & 7], 1);
            cf = c;
            return;
        }
        if (op >= 0x50 && op <= 0x57) {
            push(r[op & 7]);
            return;
        }
        if (op >= 0x58 && op <= 0x5F) {
            const uint32_t v = pop();
            r[op & 7] = v;
            return;
        }
        if (op >= 0x70 && op <= 0x7F) {
            const int32_t d = int8_t(fetch8());
            if (cond(op & 15))
                eip += uint32_t(d);
            return;
        }
        if (op >= 0xB8 && op <= 0xBF) {
            r[op & 7] = fetch32();
            return;
        }
        if (op >= 0xB0 && op <= 0xB7) {
            setReg8(op & 7, fetch8());
            return;
        }
        if (op >= 0x91 && op <= 0x97) {
            std::swap(r[EAX], r[op & 7]);
            return;
        }
        switch (op) {
        case 0x0F: {
            const uint8_t b = fetch8();
            if (b >= 0x80 && b <= 0x8F) {
                const int32_t d = int32_t(fetch32());
                if (cond(b & 15))
                    eip += uint32_t(d);
                return;
            }
            if (b >= 0x90 && b <= 0x9F) {
                const Rm o = decode();
                set8(o, cond(b & 15) ? 1 : 0);
                return;
            }
            const Rm o = decode();
            switch (b) {
            case 0xBF: r[modrmReg] = uint32_t(int32_t(int16_t(get16(o)))); return;
            case 0xB7: r[modrmReg] = get16(o); return;
            case 0xBE: r[modrmReg] = uint32_t(int32_t(int8_t(get8(o)))); return;
            case 0xB6: r[modrmReg] = get8(o); return;
            case 0xAF: {
                const int64_t p = int64_t(int32_t(r[modrmReg])) * int32_t(get32(o));
                r[modrmReg] = uint32_t(p);
                cf = of = p != int64_t(int32_t(p));
                return;
            }
            default: fail("0F opcode"); return;
            }
        }
        case 0x69:
        case 0x6B: {
            const Rm o = decode();
            const int32_t i = op == 0x69 ? int32_t(fetch32()) : int32_t(int8_t(fetch8()));
            const int64_t p = int64_t(int32_t(get32(o))) * i;
            r[modrmReg] = uint32_t(p);
            cf = of = p != int64_t(int32_t(p));
            return;
        }
        case 0x68: push(fetch32()); return;
        case 0x6A: push(uint32_t(int32_t(int8_t(fetch8())))); return;
        case 0x80:
        case 0x81:
        case 0x83: {
            const Rm o = decode();
            const int a = modrmReg;
            if (op == 0x80) {
                const uint8_t i = fetch8();
                const uint32_t v = alu(a, uint32_t(get8(o)) << 24, uint32_t(i) << 24) >> 24;
                if (a != 7)
                    set8(o, uint8_t(v));
                return;
            }
            if (o16) {
                const uint32_t i = op == 0x81 ? (u16(eip) & 0xFFFFu) : uint32_t(int32_t(int8_t(u8(eip))));
                eip += op == 0x81 ? 2 : 1;
                const uint32_t v = alu(a, uint32_t(get16(o)) << 16, i << 16) >> 16;
                if (a != 7)
                    set16(o, uint16_t(v));
                return;
            }
            const uint32_t i = op == 0x81 ? fetch32() : uint32_t(int32_t(int8_t(fetch8())));
            const uint32_t v = alu(a, get32(o), i);
            if (a != 7)
                set32(o, v);
            return;
        }
        case 0x84: {
            const Rm o = decode();
            alu(4, uint32_t(get8(o)) << 24, uint32_t(reg8(modrmReg)) << 24);
            return;
        }
        case 0x85: {
            const Rm o = decode();
            if (o16)
                alu(4, uint32_t(get16(o)) << 16, (r[modrmReg] & 0xFFFF) << 16);
            else
                alu(4, get32(o), r[modrmReg]);
            return;
        }
        case 0x87: {
            const Rm o = decode();
            const uint32_t t = get32(o);
            set32(o, r[modrmReg]);
            r[modrmReg] = t;
            return;
        }
        case 0x88: {
            const Rm o = decode();
            set8(o, reg8(modrmReg));
            return;
        }
        case 0x89: {
            const Rm o = decode();
            if (o16)
                set16(o, uint16_t(r[modrmReg]));
            else
                set32(o, r[modrmReg]);
            return;
        }
        case 0x8A: {
            const Rm o = decode();
            setReg8(modrmReg, get8(o));
            return;
        }
        case 0x8B: {
            const Rm o = decode();
            if (o16)
                r[modrmReg] = (r[modrmReg] & ~0xFFFFu) | get16(o);
            else
                r[modrmReg] = get32(o);
            return;
        }
        case 0x8D: {
            const Rm o = decode();
            if (o.isReg)
                fail("lea of a register");
            r[modrmReg] = o.addr;
            return;
        }
        case 0x90: return;
        case 0x99: r[EDX] = (r[EAX] >> 31) ? 0xFFFFFFFFu : 0u; return;
        case 0x9E: {   // sahf
            const uint8_t ah = uint8_t(r[EAX] >> 8);
            cf = ah & 1;
            pf = (ah & 4) != 0;
            zf = (ah & 0x40) != 0;
            sf = (ah & 0x80) != 0;
            return;
        }
        case 0xA4:   // movsb (rep: ecx times)
            do {
                if (rep && !r[ECX])
                    break;
                *mem.ptr(r[EDI]) = u8(r[ESI]);
                ++r[ESI];
                ++r[EDI];
                if (rep)
                    --r[ECX];
            } while (rep);
            return;
        case 0xA5:   // movsd (rep: ecx times)
            if (rep) {
                while (r[ECX]) {
                    w32(r[EDI], u32(r[ESI]));
                    r[ESI] += 4;
                    r[EDI] += 4;
                    --r[ECX];
                }
            } else {
                w32(r[EDI], u32(r[ESI]));
                r[ESI] += 4;
                r[EDI] += 4;
            }
            return;
        case 0xAB:   // stosd
            do {
                if (rep && !r[ECX])
                    break;
                w32(r[EDI], r[EAX]);
                r[EDI] += 4;
                if (rep)
                    --r[ECX];
            } while (rep);
            return;
        case 0xA9: alu(4, r[EAX], fetch32()); return;
        case 0xA8: alu(4, (r[EAX] & 0xFF) << 24, uint32_t(fetch8()) << 24); return;
        case 0xA0: r[EAX] = (r[EAX] & ~0xFFu) | u8(fetch32()); return;
        case 0xA1: r[EAX] = u32(fetch32()); return;
        case 0xA3: w32(fetch32(), r[EAX]); return;
        case 0xC0:
        case 0xC1:
        case 0xD0:
        case 0xD1:
        case 0xD2:
        case 0xD3: {
            const Rm o = decode();
            const int n = (op == 0xC0 || op == 0xC1) ? fetch8() : (op == 0xD0 || op == 0xD1) ? 1 : int(r[ECX] & 0xFF);
            if (op == 0xC0 || op == 0xD0 || op == 0xD2) {
                const uint32_t v = shift(modrmReg, uint32_t(get8(o)) << 24, n);
                set8(o, uint8_t(v >> 24));
                if (modrmReg == 5 || modrmReg == 7 || modrmReg < 2)
                    fail("8-bit shift right / rotate");
                return;
            }
            set32(o, shift(modrmReg, get32(o), n));
            return;
        }
        case 0xC2: {
            const uint16_t n = u16(eip);
            eip = pop();
            r[ESP] += n;
            return;
        }
        case 0xC3: eip = pop(); return;
        case 0xC6: {
            const Rm o = decode();
            set8(o, fetch8());
            return;
        }
        case 0xC7: {
            const Rm o = decode();
            if (o16) {
                set16(o, u16(eip));
                eip += 2;
            } else
                set32(o, fetch32());
            return;
        }
        case 0xE8: {
            const int32_t d = int32_t(fetch32());
            push(eip);
            eip += uint32_t(d);
            return;
        }
        case 0xE9: {
            const int32_t d = int32_t(fetch32());
            eip += uint32_t(d);
            return;
        }
        case 0xEB: {
            const int32_t d = int8_t(fetch8());
            eip += uint32_t(d);
            return;
        }
        case 0xF6: {
            const Rm o = decode();
            const uint8_t v = get8(o);
            switch (modrmReg) {
            case 0: alu(4, uint32_t(v) << 24, uint32_t(fetch8()) << 24); return;
            case 2: set8(o, uint8_t(~v)); return;
            case 3: set8(o, uint8_t(alu(5, 0, uint32_t(v) << 24) >> 24)); cf = v != 0; return;
            default: fail("F6 opcode"); return;
            }
        }
        case 0xF7: {
            const Rm o = decode();
            const uint32_t v = get32(o);
            switch (modrmReg) {
            case 0: alu(4, v, fetch32()); return;
            case 2: set32(o, ~v); return;
            case 3: set32(o, alu(5, 0, v)); cf = v != 0; return;
            case 4: {
                const uint64_t p = uint64_t(r[EAX]) * v;
                r[EAX] = uint32_t(p);
                r[EDX] = uint32_t(p >> 32);
                cf = of = r[EDX] != 0;
                return;
            }
            case 5: {
                const int64_t p = int64_t(int32_t(r[EAX])) * int32_t(v);
                r[EAX] = uint32_t(p);
                r[EDX] = uint32_t(uint64_t(p) >> 32);
                cf = of = p != int64_t(int32_t(p));
                return;
            }
            case 6: {
                if (!v) {
                    fail("divide by zero");
                    return;
                }
                const uint64_t n = (uint64_t(r[EDX]) << 32) | r[EAX];
                r[EAX] = uint32_t(n / v);
                r[EDX] = uint32_t(n % v);
                return;
            }
            default: {
                if (!v) {
                    fail("divide by zero");
                    return;
                }
                const int64_t n = int64_t((uint64_t(r[EDX]) << 32) | r[EAX]);
                r[EAX] = uint32_t(int32_t(n / int32_t(v)));
                r[EDX] = uint32_t(int32_t(n % int32_t(v)));
                return;
            }
            }
        }
        case 0xFF: {
            const Rm o = decode();
            switch (modrmReg) {
            case 0:
            case 1: {
                const bool c = cf;
                set32(o, alu(modrmReg == 0 ? 0 : 5, get32(o), 1));
                cf = c;
                return;
            }
            case 2: {
                const uint32_t t = get32(o);
                push(eip);
                eip = t;
                return;
            }
            case 4: eip = get32(o); return;
            case 6: push(get32(o)); return;
            default: fail("FF opcode"); return;
            }
        }
        default: {
            char b[64];
            std::snprintf(b, sizeof b, "opcode 0x%02X", op);
            --eip;
            fail(b);
            return;
        }
        }
    }
};

} // namespace hotd::proof
