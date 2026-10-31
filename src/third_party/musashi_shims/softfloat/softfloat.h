/* Our stand-in for SoftFloat's softfloat.h: only the type of Musashi's FPU registers (m68kcpu.h: floatx80 fpr[8]),
 * which a 68000 never uses. */
#pragma once
#include <stdint.h>
typedef struct { uint64_t low; uint16_t high; } floatx80;
