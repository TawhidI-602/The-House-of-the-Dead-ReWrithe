#!/usr/bin/env python3
"""
rand_pairs.py - compare random-number calls between paired arcade / PC routines (names.tsv pairs, Ghidra exports).

For every arcade routine that calls rand, counts the rand calls in its C and in its PC pair's C (PC calls of the
per-file "pick 1 of 2 / 3 / 4" helpers count as rand calls: Watcom kept them as functions, ~160 copies). Counts are
textual (a call in two branches counts twice), so a difference is a lead, not proof: a wrong pair, or a real A / PA
difference in the random sequence. Run from the repository root.

  py tools/rand_pairs.py [helpers.txt]     differing pairs + summary; optionally writes the helper list
"""
import os, re, sys
sys.path.insert(0, 'tools')
import decomp_query as dq

RAND = re.compile(r'\brand\(')
pcf = dq.load('pc')[0]
arf = dq.load('arcade')[0]
helpers = {}
for a, name, size, text in pcf:
    t = ''.join(text)
    if size <= 90 and len(RAND.findall(t)) == 1 and re.search(r'% ([234])', t) and name != 'rand':
        helpers[name] = int(re.search(r'% ([234])', t).group(1))


def count(t, extra):
    n = len(RAND.findall(t))
    for h in extra:
        n += len(re.findall(r'\b%s\(' % re.escape(h), t))
    return n


A = {a: (name, count(''.join(t), [])) for a, name, s, t in arf}
P = {a: (name, count(''.join(t), helpers)) for a, name, s, t in pcf if name not in helpers}
pairs = {}
for l in open('tools/ghidra/names.tsv', encoding='utf-8'):
    c = l.rstrip('\n').split('\t')
    if len(c) > 3 and c[0] == 'func' and c[1] != '-' and c[2] != '-':
        pairs[int(c[2], 16)] = int(c[1], 16)
same = diff = 0
rows = []
for a, (name, n) in sorted(A.items()):
    if not n or a not in pairs:
        continue
    m = P.get(pairs[a], ('', 0))[1]
    if m == n:
        same += 1
    else:
        diff += 1
        rows.append('%-36s A %d PC %d' % (name, n, m))
print('\n'.join(rows))
print('helpers', len(helpers), '| same', same, 'different', diff)
with open(sys.argv[1] if len(sys.argv) > 1 else os.devnull, "w") as f:
    for a, name, size, text in pcf:
        if name in helpers:
            f.write('%x %s %d\n' % (a, name, helpers[name]))
