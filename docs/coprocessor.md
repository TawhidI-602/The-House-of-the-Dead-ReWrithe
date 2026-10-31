# The geometry coprocessor (Model 2C "TGP") and how the rewrite reproduces it

The arcade House of the Dead runs on Sega Model 2C. Its main CPU (Intel i960) does almost no 3D or game maths by
itself: distances, directions, angles, sin / cos, the camera matrix and every object's transform are computed by
the **geometry coprocessor**, a Fujitsu MB86235 ("TGP"), running a program that the game itself uploads at boot.
For the rewrite's arcade behaviour (**A**) to be exact, every coprocessor function the game uses has to give the
same float32 bits as the chip. This page describes the hardware interface, the program, every function the game
calls, the coprocessor's internal state, the tools that prove the models, and the obstacles met on the way.

"A" here means the arcade **as MAME 0.289 emulates it**. MAME computes the chip's FRSQ / FRCP (reciprocal square
root / reciprocal seeds) as exact IEEE 1 / sqrt(x) and 1 / x; the real chip uses table seeds, so real hardware may
differ in the last bits. Everything below was checked against MAME, not against a board.

## 1. Hardware interface (i960 side)

- **Program upload:** at boot `copro_program_load` (arcade 0xA36A0, called from `hw_init_3d`) sends 0x2000 32-bit
  words from the game's data ROM to the coprocessor FIFO while the control word at 0x980000 has bit 31 set. Two
  words make one 64-bit instruction: 4096 instructions. MAME's `unidasm -arch mb86235` disassembles them.
- **Calling a function n:** either write `n | n << 8` to the function port `0x880000 + n * 0x10`, or push the
  encoded command word `n << 23 | n << 8 | n` straight into the FIFO at 0x884000 (e.g. `0x0D001A1A` = function
  0x1A). Then write the inputs to the FIFO and read the results back from 0x884000. A read of an empty FIFO stalls
  the i960 until the coprocessor has written the result (the probe sees those repeated reads as extra zero words).
- **Dispatcher** (coprocessor address 0x198): checks that the three copies of n in the command word agree and jumps
  to the routine whose address the start-up code stored in A-RAM[n] (0x90 slots; unused slots point to an
  idle loop).

## 2. MB86235 details that matter for exact results

Learned from the disassembly and confirmed by the bit-exact matches below:
- Floating point is IEEE single precision; each ALU / multiplier result is rounded to float32.
- `FSUB a, b, d` computes `d = b - a`; the same operand order holds for `SUB`, `FCMP a, b` (flags of `b - a`),
  `CMP` and `FABC a, b` (flags of `|b| - |a|`, no result written).
- One instruction can hold an ALU operation, a multiplier operation and a move; all of them read the register
  values from **before** the instruction.
- Conditional branches in an instruction test the flags set by the **previous** instruction.
- Every jump, call and return has one **delay slot**: the next instruction runs before the jump takes effect.
- `CFI` (float -> int) **truncates** toward zero.
- The P registers (`PR`) are a small register file read through PRP and written through PWP; `PR++` reads and
  increments, `PR#0` reads and resets the pointer to 0. The current matrix lives there.
- `FRCP` / `FRSQ` = reciprocal / reciprocal square root seed (MAME: exact 1 / x, 1 / sqrt x), always refined by
  Newton steps in the program.

## 3. State the coprocessor keeps between calls

This is the main obstacle for checking: many functions return nothing and only change state, and some results
depend on what an earlier call left behind.
- **Current matrix** `PR0..11`: three rotation rows (PR0..2, PR3..5, PR6..8) and a translation (PR9..11).
  A point transforms as `((x * PR0 + y * PR3) + z * PR6) + PR9` (and likewise for the other two columns).
- **Matrix stack** in B-RAM: depth B[0], pointer B[1], base 0x20, 12 words per entry, 15 entries. A push past
  15 still counts the depth but copies nothing; pops then only count down until the depth is back to 15.
- **Stored matrices** at B-RAM 0xF8 + 12 n (slot 0 is the camera; functions 0x73 - 0x77, 0x7B).
- **The data ROM** (MAME region `copro_data`, two 512 KB chips epr-19707 (low 16 bits) and epr-19706 (high 16 bits)
  of each 32-bit word) appears at 0x800000 in the chip's data space. Functions 0x62 and 0x6E read it: word
  0x100 + k points to quad list k (for 0x62); entry i points to 8 curve pointers (for 0x6E). The rewrite loads it
  from the user's ROM set like every other asset.
- **Projection constants** B[2..5] (function 0x7D, used by the visibility test 0x7B).
- **The multiplier register MA0.** An arctangent of 0 / 0 returns MA0 unchanged, so the yaw of a zero vector is
  whatever the previous call left in MA0 (see 0x0F). A port must carry this value across calls.

## 4. The functions the game calls

Seen in full call traces of the attract loop (`tools/run_copro_probe.ps1 -Funcs all`) and at the i960 call sites.
"Checked" = number of calls whose every output word equals the model's, bit for bit; state-only functions are
proven through the outputs of the later calls that read the state (0x11 / 0x1A / 0x20 / 0x6F / 0x7B).

| Fn | What it does | Model | Checked |
|----|--------------|-------|---------|
| 0x05 | push the current matrix | `Tgp.push` | via later outputs (85 k calls) |
| 0x06 | pop | `Tgp.pop` | via later outputs (96 k) |
| 0x07 | load matrix (12 words) | `Tgp.call` | via later outputs |
| 0x08 | reset the matrix stack | `Tgp.call` | via later outputs |
| 0x09 | current = input matrix x current | `Tgp.mul` | via later outputs |
| 0x0A | angle of a 2D vector: CFI(atan2(b, a) x 65536 / 2 pi) | `tgp_atan2` | 1273 / 1273 |
| 0x0C | normalize | `f0c_normalize` | 1272 / 1272 (+ 36 in a trace) |
| 0x0F | direction -> yaw, pitch | `f0f_angles` | 3000 / 3000; 5919 / 5919 in traces |
| 0x10 | identity | `Tgp.call` | via later outputs |
| 0x11 | read the current matrix (12 words) | - | 275258 / 275258 |
| 0x12 | translate by a local offset | `Tgp.xform` | via later outputs (72 k) |
| 0x13 | scale the rows | `Tgp.call` | via later outputs |
| 0x14 / 0x15 / 0x16 | rotate about X / Y / Z by a 16-bit angle | `Tgp.rot` | via later outputs |
| 0x1A | transform a point | `Tgp.xform` | 60495 / 60495 |
| 0x1B / 0x1C | sin / cos of a 16-bit angle | `tgp_sincos` | 604 / 604 each |
| 0x1D / 0x1E | r x sin / r x cos | `Tgp.call`, `tgp_model` | 0x1E 141 / 141; 0x1D not seen yet |
| 0x1F | distance of two points | `f1f_distance` | 5000 / 5000 (+ 1609 in a trace) |
| 0x20 | read the translation | - | 80484 / 80484 |
| 0x21 | push + identity | `Tgp.call` | via later outputs |
| 0x2D | square root of one input (code 0x144: routine 0x1D9, as 0x1F's last step; only `dog_set3_fall`) | `tgp_sqrt` | 10 / 10 (dog_set3_fall captures, k97_r15b) |
| 0x62 | ray against a quad list in the data ROM (shots, floor probe) | `ray_test` | 5077 / 5077 (4390 hits) |
| 0x6E | 8 keyframed Hermite curves at time t (paths) in the data ROM | `path_table` | 5703 / 5703 |
| 0x6F | matrix -> yaw, pitch, roll (Z forward) | `Tgp.matrix_angles` | 4045 / 4045 |
| 0x71 | matrix -> yaw, pitch, roll (X forward) | `Tgp.matrix_angles_x` | 14 / 14 |
| 0x73 / 0x74 / 0x75 | load / push + load / store stored matrix n | `Tgp.call` | via later outputs |
| 0x76 / 0x77 | current = stored n x current / current x stored n | `Tgp.mul` | via later outputs |
| 0x7A | invert the current matrix | `Tgp.inverse` | via later outputs (3463) |
| 0x7B | is a sphere in view (camera = stored 0) -> +1.0 / -1.0 | `Tgp.sphere_visible` | 5180 / 5180 |
| 0x7D | set the projection constants | `Tgp.call` | via 0x7B |
| 0x7F | push + load matrix | `Tgp.call` | via later outputs |
| 0x86 | rotate Z, then Y, then X | `Tgp.rot` x 3 | via later outputs (27 k) |
| 0x87 / 0x88 | load the rotation (9 words) / the translation (3) | `Tgp.call` | via later outputs |
| 0x89 | cosine of the angle between two vectors | `Tgp.call` | 1348 / 1348 |

Traces checked: boot to 28 s of the attract loop (600 k calls), 80-105 s (demo 1, 995 k calls), a recorded game
(`rectest_us`, 25-60 s: 2.5 M calls) and every 0x62 / 0x6E call of that game: **every output word of every function
equal**. All functions the game was seen to use are modelled; 0x1D (r x sin) has a model but was not called in any
log yet.

Models: `tools/tgp_model.py` (stateless functions) and `tools/tgp_trace.py` (the state and the matrix functions).

## 5. The algorithms

All operations float32, in exactly this order (each product and sum rounded).
- **Square root** (routine 0x1D9): 0 stays 0; y = FRSQ(s); twice y = (3 - s (y y)) (y 0.5); result s y.
  Distance 0x1F: s = (dx^2 + dy^2) + dz^2 with d = second - first. Normalize 0x0C: the same y, result (x y, ...).
- **Division** (0x1CD): r = FRCP(b); t = b r; e1 = 2 - t; q = (r a) e1; e2 = 2 - e1 t; result e2 q.
- **sin / cos** (0x21D): at boot the program builds three 128-entry tables (coarse sin of j x 128 units, fine sin
  and cos of i units) with its own odd polynomial of degree 13 (Cody-Waite reduction, pi split in two parts; the
  float -> int step truncates). A call splits the 16-bit angle into quadrant, j and i and uses the angle-addition
  formula sin = cF sC + sF cC, cos = cF cC - sF sC, then the quadrant's signs. So **cos(0) = 0.99999988, not 1**.
- **Arctangent** (0x31A, Cody & Waite): |den| < 9.7e-24 -> +-pi/2 (0 / 0: MA0 unchanged); q = |num / den|;
  q > 1 -> q = 1 / q (FRCP + three Newton steps), n = 2; q >= tan 15 deg -> f = (q sqrt3 - 1) / (q + sqrt3),
  n + 1; |f| > 2^-12 -> f + f g (p1 g + p0) / ((g + q1) g + q0) with g = f^2; n >= 2 -> negate; add
  {0, pi/6, pi/2, pi/3}[n]; den <= 0 -> pi - a; num < 0 -> -a. Angles leave the chip as CFI(a x 65536 / 2 pi).
- **Direction -> yaw, pitch** (0x0F, routine 0x713): yaw from atan2(-x, z); the horizontal length h = z / cos(yaw)
  or -x / sin(yaw) (whichever is larger by octant); pitch from atan2(y, h).
- **Matrix product** (0x09, 0x76, 0x77): row i of the result = (n[3i] row0 + n[3i+1] row1) + n[3i+2] row2, the
  translation = ((n9 row0 + n10 row1) + n11 row2) + t.
- **Rotations** (0x14 / 0x16 / 0x86): r1' = c r1 - s r2, r2' = s r1 + c r2 (X: rows 1, 2; Z: rows 0, 1);
  0x15 (Y, rows 0, 2): r0' = c r0 + s r2, r2' = c r2 - s r0. sin / cos from the table routine.
- **Inverse** (0x7A): cofactors (each 2x2 product rounded, then the difference), det = (a A + d D) + g G,
  1 / det by FRCP + two Newton steps, translation -R^-1 t from the cofactors (see `Tgp.inverse`).
- **Ray vs quads** (0x62): list = ROM[ROM[0x100 + k]]: count, 6 words, then 18 words per quad: plane (n, d), type
  (projection axis), 4 vertices, attribute. Per quad: n.D (|n.D| <= 1e-4 -> skip), t = (n.P + d) / n.D, s = -t > 0,
  hit = P + D s, nearest so far; inside test in the plane of the two other axes with the four edge cross products
  (vertex order reversed by the sign of the dropped normal component), all sign bits clear. Output -1.0, or 1.0,
  hit point, s^2, normal, attribute (the hit material / owner).
- **Path curves** (0x6E): per curve a header (binary-search steps, first step) and keys of 4 words (time, value,
  out tangent, in tangent); binary search on t, then a cubic Hermite with 1 / dt, 1 / dt^2, 1 / dt^3 each by FRCP +
  Newton (`curve_eval`). 8 curves per entry; the 7th leaves as a 16-bit integer (an angle).
- **Visibility** (0x7B): the point in camera space; in front: inside the pyramid 248 w / a by 192 w / a with
  w = r B4 + z (sign tests on the float bits); behind: inside the sphere |v| < r B5.

## 6. Tools

- `tools/run_copro_probe.ps1 -Out <folder in private> -Funcs "0c,0f" | all [-Start s] [-Seconds s] [-Total n]`
  runs MAME headless on the attract loop with `tools/mame_copro_probe.lua`: write / read taps on the function
  ports and the FIFO log each call as `<command word> in <words> out <words>`. `-Funcs all` logs every call in
  order (needed for the state); `-Start` begins logging later (the state then starts unknown).
  Every run writes `recording.txt` (MAME version, command line, script hashes, settings).
- `py tools/tgp_model.py check <log>`: compares the stateless functions of a log with the models.
- `py tools/tgp_trace.py <log>`: replays a full trace through the state model and checks every output word.

## 7. Obstacles and how they were solved

- **The game's maths is not in the game code.** Ghidra's C of routines like `shot_ray_setup` showed only FIFO
  writes of constants. Solution: find the program upload, extract the program, disassemble it with MAME's
  unidasm, and read the routines the function table points to.
- **IEEE maths is not enough.** The coprocessor's distance is 1-2 ULP away from float32 sqrt in about half the
  calls; its cos(0) is not 1. Solution: model each routine operation by operation and prove it on logged calls.
- **No MB86235 manual or core source at hand.** Operand order, parallel-read and delay-slot rules, CFI rounding
  and flag timing were inferred from the code and confirmed when the models matched (section 2).
- **State-only functions can't be checked one by one.** Solution: log every call in order from boot and replay
  the whole sequence through a model of the coprocessor's state (`tgp_trace.py`); a wrong state model shows up in
  the next matrix read.
- **Garbage that matters.** atan2(0, 0) returns a register left over from the previous call, and the game does ask
  for the direction of zero vectors. Tracking MA0 through every modelled function resolved all but one such call.
- **Stalled FIFO reads** appear in the log as extra zero words; the checker accepts extra zeros only.
- **Tables outside the program.** Two functions read memory at 0x800000 that the program never writes. MAME's
  Model 2 driver source (memory map of the MB86235's data space) showed it is a data ROM on the board, so the
  models read the same ROM.
- **Rare paths need gameplay.** The attract loop never made a shot hit a wall; the probe can replay a recorded
  `.inp` game (`-Name`) to reach them.

## 8. What the rewrite does

Option (a), chosen by the project owner: each coprocessor function the game uses becomes a C++ routine with the same
float32 operation order (the models above are the reference), plus a small `Tgp` state object (current matrix,
stack, stored matrices, projection constants, MA0). The PC port (PA / PM) does not use any of this: it has its own
software matrices on x87, a 4096-entry sin / cos table and a table square root, which the rewrite reproduces
separately for PA / PM.

**Done (2026-10-13):** `src/arcade/tgp.h / tgp.cpp` (class `hotd::arcade::Tgp`, one routine per coprocessor routine,
named by entry point) is the C++ port of the models. Proof: `hotd_proof tgp <copro.txt>` replays a from-boot trace
through it ([proof.md](proof.md)): attract from boot (602 k calls) and boot through a recorded game (2.7 M calls)
plus the 4851 ray tests of `copro_rectest62`: every output word equal. A trace that starts late (`-Start`) or logs
only some functions can only check the stateless functions (0x0C, 0x1B, 0x1C, 0x1E, 0x1F, 0x62, 0x6E).
