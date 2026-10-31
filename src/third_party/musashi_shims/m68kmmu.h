/* Our stand-in for Musashi's m68kmmu.h (the 68030 PMMU; not used: that file is under MAME's old license). The 68000 has
 * no MMU: addresses go out untranslated. */
uint pmmu_translate_addr(uint addr_in) { return addr_in; }
