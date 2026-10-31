"""Contact sheet for tools\\checkpoints.ps1: the checkpoint screenshots of each model source side by side with the MAME
reference frame of the same moment and, where one exists, the real PC game's capture, as an index.html in the output
folder (everything stays under private\\).

MAME references: the dump sets under private\\mame_dumps (a work RAM dump and a screenshot every 1-5 s,
snap\\hotd\\NNNN.png = the NNNN-th line of index.txt), searched in this order: fullgame_us (Chapter 1 on events 0 1 3 4
5 6 19, the results), inflives1 (events 9 / 11), life_scan (CONTINUE, the game over), ch1_run1. Each dump's state: the
mode 0x51F494, sub-mode 0x51F4C8, chapter 0x51F9D1, event 0x51F9D0 and the frame counter 0x51F428. A moment's start
is taken half-way between the last dump before it and the first dump in it (event 0 of fullgame_us: frame 1014 =
our frame 0, t1.md fix-logic); the reference is the dump whose frame counter is nearest to start + F. "clear F": the
results begin about 360 frames before Chapter 2's start (fullgame_us snaps 0171-0173). The replays were played by a
person on another timing, so the reference is the same place in the script, not the same frame.

Real PC: private\\screenshots\\pc_look\\fw2\\pc_tNNNNN.png (the harness's tick N ~ our frame N + 296 in event 0,
render.md fix-world-2 item 5); shown for checkpoints whose frame (from the run logs) is within 60 ticks of a capture.

  py tools/checkpoint_sheet.py <checkpoints.txt> <out folder> [<mame_dumps folder or one dump set>] [<pc capture folder>]
"""
import glob
import html
import os
import re
import sys

SETS = ('fullgame_us', 'inflives1', 'life_scan', 'ch1_run1')
PC_OFFSET = 296

SUMMARY = """<h1>HOTD ReWrithe: the whole game, checkpoint contact sheet</h1>
<p>@NAV@</p>
<p><b>What plays.</b> Chapters 1-4 and the ending are playable end to end with both model sources (arcade and PC). Chapter select
offers all four chapters; the bosses are the Chariot (Chapter 1, and again at the start of Chapter 4), the Hangedman (2), the
Hermit (3) and the Magician (4); after Chapter 4's clear the ending plays (Rogan's closing scene, the staff roll, END / GAME OVER).
Every run is the default route (<code>--autofire --lives-kept 1 --default-route</code>, one run per chapter from
<code>--chapter N</code>; the ending with <code>--chapter 4 --ending</code>). Columns: our arcade-model and PC-model render, the MAME
full-game capture of the same place (fullgame_us, a snapshot every ~2 s), and for Chapter 1's opening the real PC game. MAME was
played by a person on another route and timing: its snapshot is the same place in the script, scaled to the event's length;
"approx" marks a guess (event numbering mapped, or the event lengths far apart); "-" = MAME did not play that part (for example
Chapter 3's event 3 / 6, and the ending's end screen).</p>
<p><small>Chapter 1 rows c01-c12 and c19-c30 are from the "main" run re-shot on 2026-10-19 (ch1redo_20261019); c13-c18 (branch route, CONTINUE, game over) are the earlier "final" run (the same code for Chapter 1).</small></p>
<p><b>Known open issues</b> (docs/teaser_status: ch2_playtest.md, ch4.md, pcpolish4.md; all rough, partly UNVERIFIED):</p>
<ul>
<li>Hangedman (Chapter 2): wings missing in the played build (MAME shows big green bat wings in the moon scene, title card, fight).</li>
<li>End screens: the "Chapter clear" panel is drawn over the frozen name-entry picture (cosmetic); on the 16:9 results screen the 1P ammo / torches sit at the window edge.</li>
<li>Ending: the colour-curve fade-ins and window lines are not drawn; with the PC models the staff-roll credit sheets and the results card are mapped pictures and the PC's own ending was not compared with the real PC.</li>
<li>PC models: the Chapter 1 door before the Chariot room draws without its room (no 0xCE8 polygons in the PC files); a floating-skull report is not found yet; the attract ranking lacks the RANK cards. PC models are unlit, so brighter than the arcade's dark rooms (MAME is as dark).</li>
<li>Chapter 4: the Chariot rematch lands out of view in MAME and fights on the floor; ours carries a soft-lock guard (DEVIATION). Bots never pick the Chapter 4 branch.</li>
<li>Chapters 2-4 and the ending are read from the disassembly and only partly compared with MAME, so timing and details can differ.</li>
</ul>
"""


def u(b, a, n):
    return int.from_bytes(b[a:a + n], 'little')


def load_set(d):
    idx = os.path.join(d, 'index.txt')
    if not os.path.exists(idx):
        return None
    tags = [l.split()[0] for l in open(idx) if l.strip()]
    st = []
    for t in tags:
        with open(os.path.join(d, t + '_workram.bin'), 'rb') as fh:
            b = fh.read(0x20800)
        st.append({'mode': u(b, 0x1F494, 2), 'sub': u(b, 0x1F4C8, 4), 'ch': b[0x1F9D1], 'ev': b[0x1F9D0],
                   'frame': u(b, 0x1F428, 4), 'tmr': u(b, 0x20700, 4), 'tag': t})
    return {'dir': d, 'name': os.path.basename(os.path.normpath(d)), 'st': st}


def nearest(s, target):
    k = min(range(len(s['st'])), key=lambda i: abs(s['st'][i]['frame'] - target))
    snap = os.path.join(s['dir'], 'snap', 'hotd', '%04d.png' % k)
    return (snap, '%s %s (%d)' % (s['name'], s['st'][k]['tag'], k)) if os.path.exists(snap) else None


def start_of(s, pred):
    st = s['st']
    for i, x in enumerate(st):
        if pred(x):
            return x['frame'] if i == 0 else (st[i - 1]['frame'] + x['frame']) // 2
    return None


def reference(sets, cp):
    name, kind, a, b, f = cp
    for s in sets:
        if kind == 'event':
            if a == 0 and s['name'] == 'fullgame_us':
                start = 1014
            else:
                start = start_of(s, lambda x: x['mode'] == 4 and x['ch'] == 0 and x['ev'] == a and x['sub'] < 3)
        elif kind == 'mode':
            start = start_of(s, lambda x: x['mode'] == a and (x['sub'] == b or (a == 4 and b == 4 and x['sub'] == 3)))
            if a == 5 and b == 1:   # the game-over scene: its exact start from the timer 0x520700 (360 down)
                x = next((x for x in s['st'] if x['mode'] == 5 and x['sub'] == 1 and 0 < x['tmr'] <= 360), None)
                if x is not None:
                    start = x['frame'] - (360 - x['tmr'])
        else:   # clear
            end = start_of(s, lambda x: x['mode'] == 4 and x['ch'] == 1)
            start = end - 360 if end is not None else None
        if start is not None:
            r = nearest(s, start + f)
            if r:
                return r
    return None


# the chapters of the full game: run name -> (section title, MAME chapter byte 0x51F9D1)
CHAPTERS = {'main': ('Chapter 1', 0), 'branch': ('Chapter 1', 0), 'die': ('Chapter 1', 0),
            'ch2': ('Chapter 2', 1), 'ch3': ('Chapter 3', 2), 'ch4': ('Chapter 4', 3), 'ending': ('The ending', 6)}
EVMAP = {'ending': {4: 6}}   # our ending's event 4 (the closing room) is MAME's event 6 (snaps 778-788)


def span(s, pred):
    """First contiguous stretch of dump states with pred true: (start frame, end frame); half-way to the neighbours."""
    st = s['st']
    for i, x in enumerate(st):
        if pred(x):
            j = i
            while j + 1 < len(st) and pred(st[j + 1]):
                j += 1
            a = x['frame'] if i == 0 else (st[i - 1]['frame'] + x['frame']) // 2
            b = st[j]['frame'] if j + 1 >= len(st) else (st[j]['frame'] + st[j + 1]['frame']) // 2
            return a, b
    return None


def reference_chapter(sets, run, cp, ours_len):
    """MAME reference of a Chapter 2-4 / ending checkpoint: (png, label) or None. The event's MAME span is scaled to our
    event's length (ours_len): 'event E +F' maps to the same fraction of the event. Labelled approx when the lengths
    differ by more than a quarter or the event number was mapped."""
    name, kind, a, b, f = cp
    ch = CHAPTERS[run][1]
    s = next((x for x in sets if x['name'] == 'fullgame_us'), None)
    if s is None:
        return None
    if kind == 'event':
        ev = EVMAP.get(run, {}).get(a, a)
        sp = span(s, lambda x: x['mode'] == 4 and x['ch'] == ch and x['ev'] == ev)
        if sp is None:
            return None
        mlen = sp[1] - sp[0]
        if ours_len and ours_len > 0:
            ratio = mlen / ours_len
            target = sp[0] + min(f * ratio, mlen - 1)
            approx = not (0.75 <= ratio <= 1.33) or ev != a
        else:
            target = sp[0] + min(f, mlen - 1)
            approx = f >= mlen or ev != a
    elif kind == 'clear' and run != 'ending':
        nxt = span(s, lambda x: x['mode'] == 4 and x['ch'] == (6 if ch == 3 else ch + 1))
        if nxt is None:
            return None
        target = nxt[0] - 360 + f
        approx = False
    else:
        return None
    r = nearest(s, int(target))
    return (r[0], r[1] + (' approx' if approx else '')) if r else None


def main():
    cp_file, out = sys.argv[1], sys.argv[2]
    here = os.path.dirname(os.path.abspath(__file__))
    mame = sys.argv[3] if len(sys.argv) > 3 else os.path.join(here, '..', 'private', 'mame_dumps')
    pcdir = sys.argv[4] if len(sys.argv) > 4 else os.path.join(here, '..', 'private', 'screenshots', 'pc_look', 'fw2')
    cps = []
    run = 'main'
    runs = {}   # checkpoint name -> run name
    for line in open(cp_file, encoding='utf-8'):
        f = line.split()
        if len(f) >= 2 and f[0] == 'run':
            run = f[1]
            continue
        if len(f) < 3 or f[0].startswith('#'):
            continue
        runs[f[0]] = run
        if f[1] == 'event' and len(f) >= 4:
            cps.append((f[0], 'event', int(f[2]), 0, int(f[3])))
        elif f[1] == 'clear':
            cps.append((f[0], 'clear', 0, 0, int(f[2])))
        elif f[1] == 'mode' and len(f) >= 5:
            cps.append((f[0], 'mode', int(f[2]), int(f[3]), int(f[4])))
    one = load_set(mame)
    sets = [one] if one else [s for s in (load_set(os.path.join(mame, n)) for n in SETS) if s]
    # our frame and event of each screenshot (the run logs' "screenshot: <path> (frame N, event E)")
    shot_frame = {}
    for log in glob.glob(os.path.join(out, '*', 'run*.log')) + glob.glob(os.path.join(out, '*', 'run*_err.log')):
        for l in open(log, errors='replace'):
            m = re.search(r'screenshot: .*[\\/]([^\\/]+)\.png \(frame (\d+), event (-?\d+)\)', l)
            if m:
                shot_frame[(os.path.basename(os.path.dirname(log)), m.group(1))] = (int(m.group(2)), int(m.group(3)))
    # our events' starts per (source, run): the run logs' "checkpoint: frame N event E" lines (the first of an event)
    ours_ev = {}
    for log in glob.glob(os.path.join(out, '*', 'run_*.log')):
        m0 = re.match(r'run_(.+?)(_err)?\.log$', os.path.basename(log))
        lst = ours_ev.setdefault((os.path.basename(os.path.dirname(log)), m0.group(1)), [])
        for l in open(log, errors='replace'):
            m = re.search(r'checkpoint: frame (\d+) event (-?\d+)', l)
            if m:
                lst.append((int(m.group(1)), int(m.group(2))))
    pcs = {}
    for p in glob.glob(os.path.join(pcdir, 'pc_t*.png')):
        pcs[int(re.search(r'pc_t(\d+)', p).group(1))] = p
    sources = [d for d in ('arcade', 'pc') if os.path.isdir(os.path.join(out, d))]
    sections = []   # (title, [rows])
    nref = npc = 0
    refs_by_chapter = {}
    cnt_by_chapter = {}
    for cp in cps:
        name, kind, a, b, f = cp
        cells = []
        for s in sources:
            p = os.path.join(out, s, name + '.png')
            fr = shot_frame.get((s, name))
            cap = '<br>frame %d event %d' % fr if fr else ''
            cells.append('<td>%s</td>' % ('<img src="%s/%s.png">%s' % (s, name, cap) if os.path.exists(p) else 'not reached'))
        run = runs.get(name, 'main')
        title = CHAPTERS.get(run, (run,))[0]
        if run in ('main', 'branch', 'die'):
            r = reference(sets, cp)
        else:
            src = 'arcade' if 'arcade' in sources else sources[0]
            evs = ours_ev.get((src, run), [])
            ours_len = None
            if kind == 'event':
                for i, (fr, e) in enumerate(evs):
                    if e == a and i + 1 < len(evs):
                        ours_len = evs[i + 1][0] - fr
                        break
            r = reference_chapter(sets, run, cp, ours_len)
        nref += bool(r)
        refs_by_chapter[title] = refs_by_chapter.get(title, 0) + bool(r)
        cnt_by_chapter[title] = cnt_by_chapter.get(title, 0) + 1
        cells.append('<td>%s</td>' % ('<img src="file:///%s"><br>%s' % (r[0].replace('\\', '/'), r[1]) if r else '-'))
        pc = None
        fr = shot_frame.get(('pc', name)) or shot_frame.get(('arcade', name))
        if fr and fr[1] == 0 and pcs and kind == 'event' and a == 0:
            t = min(pcs, key=lambda t: abs(t - (fr[0] - PC_OFFSET)))
            if abs(t - (fr[0] - PC_OFFSET)) <= 60:
                pc = (pcs[t], 'tick %d' % t)
        npc += bool(pc)
        cells.append('<td>%s</td>' % ('<img src="file:///%s"><br>%s' % (pc[0].replace('\\', '/'), pc[1]) if pc else '-'))
        what = {'event': 'event %d +%d frames' % (a, f), 'clear': 'results +%d frames' % f,
                'mode': 'mode %d sub-mode %d +%d frames' % (a, b, f)}[kind]
        row = '<tr><th>%s<br><small>%s</small></th>%s</tr>' % (html.escape(name), what, ''.join(cells))
        if not sections or sections[-1][0] != title:
            sections.append((title, []))
        sections[-1][1].append(row)
    head = ''.join('<th>%s models</th>' % s for s in sources) + '<th>MAME (full game)</th><th>real PC (harness)</th>'
    body = []
    nav = []
    for k, (title, rs) in enumerate(sections):
        nav.append('<a href="#s%d">%s</a>' % (k, html.escape(title)))
        body.append('<h2 id="s%d">%s <small>(%d checkpoints, %d MAME references)</small></h2><table><tr><th></th>%s</tr>%s</table>'
                    % (k, html.escape(title), len(rs), refs_by_chapter.get(title, 0), head, ''.join(rs)))
    with open(os.path.join(out, 'index.html'), 'w', encoding='utf-8') as fh:
        fh.write('<!doctype html><meta charset="utf-8"><title>HOTD checkpoints: the whole game</title>'
                 '<style>body{background:#111;color:#ddd;font:14px sans-serif;margin:16px}img{width:400px}td,th{padding:4px;'
                 'vertical-align:top}a{color:#8cf}h2 small{font-weight:normal;color:#999}li{margin:3px 0}</style>'
                 + SUMMARY.replace('@NAV@', ' | '.join(nav)) + ''.join(body))
    print('contact sheet:', os.path.join(out, 'index.html'),
          '(%d checkpoints, %d MAME references, %d real-PC captures)' % (len(cps), nref, npc))
    for t in cnt_by_chapter:
        print('  %s: %d checkpoints, %d MAME references' % (t, cnt_by_chapter[t], refs_by_chapter.get(t, 0)))


if __name__ == '__main__':
    main()
