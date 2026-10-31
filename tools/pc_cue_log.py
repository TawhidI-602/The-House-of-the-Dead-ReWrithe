"""pc_cue_log.py: the PC's sound_cue (0x412000) calls from a harness capture (run_pc.ps1 -CapFuncs 412000).

  py tools/pc_cue_log.py <capture.bin>   ->   one line per call: "<tick> <cue>" (cue = eax, the first argument)

The capture format is hotd_hook.c "per-routine capture": "HOTDPCC1", then records: tag, head, u32 count, then runs.
MAPS holds (base, size) pairs; the other tags hold runs (u32 address, u32 length, bytes).
"""
import struct
import sys

HEAD = {b'CAPT': 164, b'PRE ': 0, b'BND1': 156, b'CEXT': 148, b'DROP': 0}


def main(path, routine=0x412000):
    with open(path, 'rb') as f:
        if f.read(8) != b'HOTDPCC1':
            sys.exit('not a capture file')
        while True:
            tag = f.read(4)
            if len(tag) < 4:
                break
            if tag == b'MAPS':
                (n,) = struct.unpack('<I', f.read(4))
                f.seek(8 * n, 1)
                continue
            head = f.read(HEAD[tag])
            (n,) = struct.unpack('<I', f.read(4))
            for _ in range(n):
                _, ln = struct.unpack('<II', f.read(8))
                f.seek(ln, 1)
            if tag == b'CAPT':
                r, seq, tick, front = struct.unpack_from('<IIII', head)
                eax = struct.unpack_from('<I', head, 16)[0]
                if r == routine:
                    print(tick, eax)


if __name__ == '__main__':
    main(sys.argv[1])
