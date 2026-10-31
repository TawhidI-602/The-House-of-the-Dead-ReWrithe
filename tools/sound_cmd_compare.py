import sys, re
# MAME: "<t> <frame> <code> mode m sub s"; ours: "[sound N] 0xCODE"
m = []
for ln in open(sys.argv[1]):
    p = ln.split()
    m.append((int(p[1]), p[2].upper(), float(p[0])))
o = []
for ln in open(sys.argv[2], errors='ignore'):
    r = re.match(r'\[sound (\d+)\] 0x([0-9A-F]{6})', ln)
    if r:
        o.append((int(r.group(1)), r.group(2)))
# align at the first BE5000 on both sides
mi = next(i for i, x in enumerate(m) if x[1] == 'BE5000')
oi = next(i for i, x in enumerate(o) if x[1] == 'BE5000')
m0, o0 = m[mi][0], o[oi][0]
mm = [(f - m0, c) for f, c, _ in m[mi:]]
oo = [(f - o0, c) for f, c in o[oi:]]
limit = int(sys.argv[3]) if len(sys.argv) > 3 else 10**9
mm = [x for x in mm if x[0] <= limit]
oo = [x for x in oo if x[0] <= limit]
# greedy match: same code, nearest frame within 120
used = set()
same = 0; diffs = []; missing = []
for f, c in mm:
    best = None
    for j, (g, d) in enumerate(oo):
        if j in used or d != c or abs(g - f) > 120:
            continue
        if best is None or abs(g - f) < abs(oo[best][0] - f):
            best = j
    if best is None:
        missing.append((f, c))
    else:
        used.add(best)
        diffs.append(oo[best][0] - f)
        if oo[best][0] == f: same += 1
extra = [oo[j] for j in range(len(oo)) if j not in used]
print('MAME commands %d, ours %d; matched %d (same frame %d); missing in ours %d; extra in ours %d' % (len(mm), len(oo), len(diffs), same, len(missing), len(extra)))
if diffs:
    from collections import Counter
    print('frame offset (ours - MAME) histogram:', sorted(Counter(diffs).items())[:20])
print('missing (MAME frame, code):', missing[:25])
print('extra (our frame, code):', extra[:25])
