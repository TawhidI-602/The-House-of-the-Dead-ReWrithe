# nan: the NaN draws of bot seeds 10 / 14 (2026-10-16)

Branch `claude/nan`, build `build-nan` (Release). Finding from final_check.md: seeds 10 and 14 (arcade models, 2P)
logged 6 / 5 `nan_draws` (geo_draw_model's guard, docs/teaser_status/robust.md fix 3).

## Cause (both seeds the same)

`HOTD_TRACE_NAN_DRAW=1` + a temporary trace in blood_burst: every skipped draw was a `blood_burst` task (0x44EF0) on a
body part of a creature in `enemy_dying` (0x76B80). The shot that killed it cut the part; the next frame
`body_free_cut_parts` (0x396C0) freed the node, and the burst kept reading the freed block's +0x5C..+0x74 (camera-space
place, radius) for the rest of its 29 frames: first the heap's new block header / ring words (denormal floats: drawn at
about the eye), then NaN bit patterns once the space was reused. Not a division by zero or an unported boundary: the
original reads freed memory the same way (robust.md fix 3); its TGP then draws nothing visible.

## Fix (UNVERIFIED (teaser), not an original routine)

`part_node_free(part)` (src/port/hit_effects.cpp) = `blood_burst_release(part)` + `mem_free(part)`: the blood_burst
tasks of the current task group still on that part get +0x54 = 0x23, so their next turn ends (task_exit) before drawing.
Every part-node free goes through it: part_tree_free_children, body_free_parts (enemy_helpers.cpp),
body_free_cut_parts (part_tree.cpp); the child-list blocks (+0x24) still use mem_free. The guard in geo_draw_model stays.

## Results

- Seeds 10 / 14: nan_draws 6 / 5 -> 0, same result and frame count as before (clear 25389 / clear 27049), also with
  `--render` (0 GL lines).
- `tools\robust_seeds.ps1 -Seeds 1..64 -Frames 40000` (headless, both sources, 1P / 2P): 64 / 64 exit 0, nan_draws 0
  in all, results and frame counts of 1..30 identical to final_check.md's table.
