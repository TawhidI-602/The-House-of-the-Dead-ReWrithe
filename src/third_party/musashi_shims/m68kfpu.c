/* Our stand-in for Musashi's m68kfpu.c (the 68020+ FPU; not used: that file has no license notice). HOTD's sound CPU is
 * a plain 68000, which has no FPU: an F-line opcode takes the line-1111 exception, as on the real chip. m68kcpu.c
 * includes this file by name; the build puts this folder on the include path after the (trimmed) Musashi sources. */
void m68040_fpu_op0(void) { m68ki_exception_1111(); }
void m68040_fpu_op1(void) { m68ki_exception_1111(); }
void m68881_mmu_ops(void) { m68ki_exception_1111(); }
