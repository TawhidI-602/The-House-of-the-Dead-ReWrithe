/* Our stand-in for SoftFloat's milieu.h (SoftFloat is not used: the 68000 has no FPU; see ../m68kfpu.c): only the
 * integer type names Musashi's m68kcpu.h takes from it. */
#pragma once
#include <stdint.h>
typedef int flag;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
