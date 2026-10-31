#!/usr/bin/env python3
# hang_phase_trace.py <state.rec> - the Hangedman's state changes in a whole-game differential record (docs/proof.md
# "hangfix"): the task running 0x4FAA0, its state +0x5C, pattern +0x320, hit points +0x70 / +0x72, not-hurt bit (+0x324 bit 3)
import sys, struct
sys.path.insert(0, __import__('os').path.dirname(__import__('os').path.abspath(__file__)))
import state_diff as sd
_, frames = sd.read_rec(sys.argv[1])
prev = None; first_low = None
for fr in frames:
    for a, b in fr.tasks:
        if len(b) >= 0x360 and struct.unpack_from('<I', b, 0)[0] == 0x4FAA0:
            st, = struct.unpack_from('<h', b, 0x5C); hp, mx = struct.unpack_from('<hh', b, 0x70)
            pat, fl = struct.unpack_from('<iI', b, 0x320)
            k = (st, pat, (fl >> 3) & 1)
            if k != prev:
                print(f'{fr.number}: state {st} pattern {pat} hp {hp}/{mx} not_hurt {(fl>>3)&1}')
                prev = k
