// The work RAM at power-on, built from the program ROM (the standalone start, docs/teaser_status/s1.md): what the
// original's start-up (crt_copy_data 0x5F0, boot_start 0xD7C0, boot_hw_init 0x168C0, boot_globals_reset 0xDB00) and
// the way into a game leave in work RAM before chapter_load. Replaces the MAME work RAM snapshot the teaser started
// from (the snapshot stays as the developer option --snapshot).
#pragma once

namespace hotd::game {

class Runtime;

// fills work RAM 0x500000..0x5FFFFF (already mapped, zero) as the original has it when Chapter 1 is about to load
void boot_work_ram(Runtime& rt);

} // namespace hotd::game
