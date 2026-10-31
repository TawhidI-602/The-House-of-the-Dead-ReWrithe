# The port: one routine per original function, for the arcade and the PC port

The rewrite reproduces three behaviours exactly: the arcade (**A**, the Model 2 program), the PC port's Arcade Mode
(**PA**) and its PC Mode (**PM**). PA and PM are the same PC code: the PC routines test the front-end state
(`frontend_state`, 0x5CED98; 5 = PC Mode) where the two modes differ. So there are two programs to port, the arcade's
and the PC's, and they are clearly two compilations of one source: the same routines in the same order, the same
tables, the same logic, with platform differences (no geometry coprocessor on the PC, the PC's own sound numbers,
some behaviour changed or added for the PC).

This page records how the C++ is organised so that it stays one routine per original function while matching both
programs byte for byte. The proof method is in [proof.md](proof.md).

## Decision: one template routine, instantiated per version (2026-10-14)

```cpp
template <class P> class Game { ... };   // src/port/game.h
using Arcade = Game<A>;                  // A
using PcGame = Game<Pc>;                 // PA and PM
```

- **Every original routine is one member function** of `Game<P>`, written once. `P` is the version's *layout
  traits* (`A` or `Pc`, generated into `src/port/types.h`): its struct types, its data addresses (`G`) and its routine
  addresses (`F`).
- **Where the two programs differ, both forms stay in the one routine**, as the developers' own source most likely
  had them (one file, platform conditionals): `if constexpr (P::pc) { ...PC code... } else { ...arcade code... }`,
  or `P::pc ? 0x37 : 0xA0122A` for a constant such as a sound number. PA versus PM is not a template question: the PC
  code's own run-time test of `frontend_state` is ported as it is.
- **Each version keeps its own data layout, in its own address space.** The port reads and writes the original
  structures at their original addresses (`port/mem.h`: memory mapped region by region at the original addresses;
  pointers are 32-bit original addresses). So recorded memory of either program loads into the port as it is, and
  the port's result is compared with the original's byte for byte. Nothing is translated between layouts.
- **Struct layouts come from one description**, `tools/ghidra/types.tsv`, which already gives each field's arcade and
  PC offset (`pcshift`: from which arcade offset a struct's fields move on the PC, e.g. every object field from +0x10
  on is 8 bytes further on the PC, because the PC's task header has two more words). `tools/gen_cpp_types.py`
  generates both views (`namespace a`, `namespace pc`) with the same field names, so `obj.part_root` means +0xC8 in
  `Game<A>` and +0xD0 in `Game<Pc>`. A struct the PC packs differently gets its own PC size (part roles: 8 bytes in
  the arcade, 5 on the PC); a struct one version lacks is only declared there, so using it in that version's code is a
  compile error. Fields not named yet are reached with `ob(<arcade offset>)` (the version's offset by the pcshift).
- **Data and routine addresses come from `tools/ghidra/names.tsv`**: `G::hit_damage` is 0x51FDB2 in the arcade and
  0xDA0CC4 on the PC; `F::sound_cue` is 0x154D0 / 0x412000. A datum the PC lacks is missing from `Pc::G` (compile
  error if PC code uses it); a routine the PC lacks has `F` = 0.
- **Instantiation:** `Game<A>` is instantiated whole; for `Game<Pc>` only the routines whose PC captures match are
  instantiated (listed in `src/port/game.cpp`), so arcade-only code that has not been checked against the PC cannot
  be used for the PC by accident.
- **Platform maths:** the arcade's maths goes through the coprocessor model (`port/tgp.*`, `copro()`), the PC's is
  plain float32 C++ in the PC code's operation order (the PC's x87 runs at 24-bit precision: docs/pc-harness.md), and
  the PC's own helpers (its table square roots `pc_fast_sqrt*`, its sin / cos table) become PC-only members.

### Why this and not the alternatives

- *Two separate ports (arcade C++ and PC C++):* doubles every routine, and the shared logic (most of it) would be
  proven twice and could drift. Rejected.
- *One port on the arcade layout, PC states translated into it for the proof:* every byte of the PC state (heap
  objects, pointers, PC-only fields, the PC's packed tables) would need a complete mapping both ways before anything
  could be compared; any unmapped field makes a proof meaningless, and PC-only fields have no arcade home. Rejected.
- *A run-time layout table (offsets in a struct read at run time):* works, but every access pays for it, the compiler
  cannot catch a field missing in one version, and PC-only code paths would need run-time flags. The template does
  the same with compile-time checks. Chosen: the template.

### What it costs

Existing arcade code needed only mechanical changes (`Arcade::` to `Game<P>::`, `g::` to `G::`, address constants
moved into names.tsv); the arcade proofs were re-run unchanged afterwards (all 0 differences). Each routine ported for
the PC then needs its PC disassembly read for the differences (addresses are free: they come from names.tsv once the
PC names are right; layout is free where types.tsv has the PC offsets). The first PC routines (damage) showed what
that reading finds: wrong PC addresses in names.tsv (`hit_damage` is the u16 at 0xDA0CC4, `hit_class` the byte at
0xDA0CE3), tables in the PC's code section, a packed struct, and behaviour differences that the decompilation notes
already listed (no game-mode test before model changes and hit effects, the PM damage factor, the PC's kill credit).

## Files

| File | What |
|---|---|
| `src/port/game.h` | `Game<P>`: one member per original routine; boundaries (routines not ported yet) are virtual stubs |
| `src/port/decl/*.h` | the members of `Game<P>`, one file per system (included inside the class body by game.h) |
| `src/port/game.cpp` | compiles the system files together and instantiates `Game<A>` and the proven `Game<Pc>` routines |
| `src/port/inst/*.inc` | the proven `Game<Pc>` instantiations, one file per system (included by game.cpp) |
| `src/port/damage.cpp`, `enemy*.cpp`, `shooting*.cpp` | the routines, one file per system |
| `src/proof/cases/*_arcade.inc`, `*_pc.inc` | the routines `hotd_proof` can run, one file per system and version |
| `src/proof/bounds/*.inc` | the boundary overrides of `ProofBase` (replay from the record), one file per system |
| `src/port/types.h`, `globals.h` | generated by `tools/gen_cpp_types.py` from types.tsv / names.tsv: both layouts |
| `src/port/mem.h` | the original address space: regions at their original addresses, 4 KB pages |
| `src/port/tgp.*` | the arcade's geometry coprocessor (A only) |
| `src/proof/main.cpp` | `hotd_proof`: the checks against MAME (A) and PC-harness (PA / PM) captures |

## Adding a kind

Adding a kind = a new .cpp + 4 include files + one line in each registry. Each kind (or other system) keeps its pieces
in files of its own, so sessions porting different kinds at the same time edit different files; the shared files only
list them (the registries), one line per kind. For a kind `<k>` (existing ones: molb, scene_zombie, researchers,
kit_zombie, bats, dogs, frog; the shared systems are `core`, `core_enemy`, `core_bounds`):

| File | What goes in it | Registry (one `#include` line) |
|---|---|---|
| `src/port/enemy_<k>.cpp` | the routines (`template <class P> ... Game<P>::name(...)`) | `src/port/game.cpp`, the `.cpp` list at the top |
| `src/port/decl/<k>.h` | their declarations, as in a class body; also the kind's own new boundary stubs (`virtual ... {}`, 4100 is off there) | `src/port/game.h`, after the last kind (before `core_enemy.h`) |
| `src/port/inst/<k>.inc` | `template void Game<Pc>::name(...);` for each routine once its PC captures match | `src/port/game.cpp`, the `inst/` list |
| `src/proof/cases/<k>_arcade.inc` | `case 0xADDR: a.name(regs[0]); return true;` (arcade: g0.. in `regs`, `f(i)` a float) | `src/proof/main.cpp`, `runRoutine`'s switch |
| `src/proof/cases/<k>_pc.inc` | `case F::name: a.name(regs[0]); return true;` (Watcom registers eax, edx, ebx, ecx = `regs[0], [2], [3], [1]`) | `src/proof/main.cpp`, `runRoutinePc`'s switch |
| `src/proof/bounds/<k>.inc` | `void x(...) override { boundary(F::x, {...}); }` for the boundaries only this kind calls | `src/proof/main.cpp`, the `bounds/` list in `ProofBase` |

A boundary that more than one system calls goes in `decl/core_bounds.h` / `bounds/core.inc` instead (and when a kind
ports a boundary routine, its stub leaves `core_bounds.h` and its override leaves the bounds file). The include files
are textual pieces of the class body or switch, so they have no include guards and no includes of their own; an empty
one (a kind with no PC routines proven yet) is just its header comment. Order matters only in game.h (the members are
declared in registry order); new kinds go at the end of each list.
