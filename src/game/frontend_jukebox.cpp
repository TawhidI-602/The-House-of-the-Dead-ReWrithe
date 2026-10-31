// ReWrithe's Jukebox page (screen 20, ours; frontend.h screenJukebox, game/jukebox.h): built the way the Gallery's
// page is (the hall without the logo, white 24 labels, the PC's page arrows 0xD8 / 0xD9, the help line), from the
// host's texts (jukeboxText) and actions (onJukebox).
#include "game/frontend.h"
#include "game/lang.h"
#include "util/utf8.h"

#include <algorithm>
#include <string>

namespace hotd::game {

namespace {
constexpr int kRowsY = 0x5A, kRowH = 22, kRows = 10;   // the list: ten rows from y 0x5A
constexpr int kPanel[4] = {32, 0x142, 576, 0x3E};       // menu3: the "now playing" panel (x, y, w, h)
constexpr int kBarRect[4] = {40, 0x162, 440, 12};      // the progress bar (x, y, w, h)
bool gInfo = false;   // menu2: the Info toggle (the number column and the file / code line shown)
}

// the bar: a dark red frame, the played part bright red (w.user / 1000 of it); w.user -1 (menu3): the "now playing"
// panel behind the title, bar and time (a dim frame over the hall, darkened)
void Frontend::drawJukeboxBar(Widget& w)
{
    int x, y, ww, hh;
    absRect(w, x, y, ww, hh);
    if (w.user < 0) {
        for (int j = std::max(0, y); j < std::min(kHeight, y + hh); ++j)
            for (int i = std::max(0, x); i < std::min(cw_, x + ww); ++i) {
                uint8_t* p = &(*canvas_)[size_t(j * cw_ + i) * 4];
                const bool edge = j == y || j == y + hh - 1 || i == x || i == x + ww - 1;
                for (int c = 0; c < 3; ++c)
                    p[c] = edge ? uint8_t(c ? 30 : 110) : uint8_t(p[c] / 3);
                p[3] = 255;
            }
        return;
    }
    const int fill = ww * std::clamp(w.user, 0, 1000) / 1000;
    for (int j = std::max(0, y); j < std::min(kHeight, y + hh); ++j)
        for (int i = std::max(0, x); i < std::min(cw_, x + ww); ++i) {
            uint8_t* p = &(*canvas_)[size_t(j * cw_ + i) * 4];
            const bool edge = j == y || j == y + hh - 1 || i == x || i == x + ww - 1;
            const bool played = i - x < fill;
            p[0] = edge ? 150 : played ? 205 : 30;
            p[1] = edge ? 24 : played ? 22 : 6;
            p[2] = edge ? 24 : played ? 22 : 6;
            p[3] = 255;
        }
}

void Frontend::screenJukebox()
{
    if (style == kStyleRemake) {   // remakepages: the Remake layout (frontend_remake_pages.cpp)
        remakeJukebox();
        return;
    }
    screen_ = 20;
    freeTree();
    texts_.clear();
    const JukeboxText t = jukeboxText ? jukeboxText() : JukeboxText{};
    Widget* root = widgetNew();
    root_ = root;
    setRect(root, 0, 0, 0, 0);
    setImage(root, 0xD1, -1, 0x20000A);   // the hall without the logo
    auto text = [&](const std::string& s, int x, int y, int w, int h, uint32_t flags) {
        Widget* wd = widgetNew();
        addChild(root, wd);
        setText(wd, s, flags);
        setRect(wd, x, y, w, h);
        return wd;
    };
    auto act = [this](int a) {
        return [this, a](Widget&) {
            walking_ = false;
            if (onJukebox) {
                const auto f = onJukebox;
                f(a);
            }
            if (a == kJbExit)
                screenMainMenu();
            else if (screen_ == 20)
                screenJukebox();
        };
    };
    // a text cut to fit w pixels in its font
    auto fit = [&](std::string s, uint32_t flags, int w) {
        // (misc18: a whole letter at a time, util/utf8.h: was 3 bytes, half a Korean / Japanese letter)
        return util::utf8Fit(s, "..", [&](const std::string& t) { int tw = 0, th = 0; measure(t, flags, tw, th); return tw <= w; });
    };
    text("Jukebox", 0, 0x0C, 0, 0, 0x10020);
    if (Widget* src = text("Sound: " + t.source, 470, 0x12, 160, 24, t.otherSource ? 0x20020 : 0xA0020); t.otherSource) {
        src->help = "The arcade sound board (the ROMs' own sound program) or the PC release's sounds and music files. Key: Tab";
        setHandlers(src, act(kJbSource), &Frontend::hitText);
    } else {
        src->help = "Only one sound source is there";
    }
    // the categories as tabs
    {
        const int n = int(t.tabs.size());
        std::vector<int> wid(size_t(n), 0);
        int total = 0, th = 0;
        for (int i = 0; i < n; ++i) {
            measure(t.tabs[size_t(i)], 0x20000, wid[size_t(i)], th);
            total += wid[size_t(i)];
        }
        const int gap = 40;
        int x = (cw_ - (total + gap * std::max(0, n - 1))) / 2;
        // ipolish (rtl): mirrored as the Remake's: the first list at the right (Left: the next list; the host's keys)
        const bool rtl = lang::rtl();
        for (int k = 0; k < n; ++k) {
            const int i = rtl ? n - 1 - k : k;
            Widget* tw = text(t.tabs[size_t(i)], x, 0x36, wid[size_t(i)], th, 0x20000);
            if (i == t.tab)
                tw->draw = &Frontend::drawTextLit;
            tw->help = i == t.tab ? t.tabs[size_t(i)] + " (this list)" : "The " + t.tabs[size_t(i)] + " list. Keys: Left / Right";
            setHandlers(tw, act(kJbTab + i), &Frontend::hitRect);
            x += wid[size_t(i)] + gap;
        }
    }
    // the list's page: ten rows, the chosen one yellow, the one playing marked
    const int count = int(t.rows.size());
    const int pages = std::max(1, (count + kRows - 1) / kRows);
    const int page = std::clamp(t.selected / kRows, 0, pages - 1);
    for (int r = 0; r < kRows; ++r) {
        const int i = page * kRows + r;
        if (i >= count)
            break;
        const int y = kRowsY + r * kRowH;
        const bool sel = i == t.selected, on = i == t.playing;
        // (leftovers, rtl: mirrored: the rows right-aligned at the right, the numbers at the left; the mark "<")
        const bool rtl = lang::rtl();
        // (sweep: the name translated before the mark goes on: "> Track 3" / "Track 3 <" matched no key, the playing row
        // stayed in English)
        const std::string name = lang::tr(t.rows[size_t(i)].first);
        Widget* w = rtl ? text(fit(name + (on ? " <" : "  "), 0x20000, 420), 640 - 40 - 430, y, 430, kRowH, 0x20004)
                        : text(fit((on ? "> " : "  ") + name, 0x20000, 420), 40, y, 430, kRowH, 0x20000);
        if (sel)
            w->draw = &Frontend::drawTextLit;
        w->help = on ? "Playing: click to start it again" : "Play this one (Enter plays the chosen row; Up / Down choose)";
        setHandlers(w, act(kJbRow + i), &Frontend::hitRect);
        if (gInfo)
            text(t.rows[size_t(i)].second, rtl ? 640 - 470 - 130 : 470, y, 130, kRowH, rtl ? 0xA0000 : 0xA0004);   // the number, grey, right (Info on)
    }
    if (count == 0)
        text("Nothing in this list", 0, kRowsY + 40, 0, 24, 0xA0020);
    if (pages > 1) {   // the PC's page arrows beside the list (0xD8 / 0xD9, yellow 0xDA / 0xDB)
        Widget* l = widgetNew();
        addChild(root, l);
        setRect(l, 6, kRowsY + 4 * kRowH, spr_.width(0xD8), spr_.height(0xD8));
        setImage(l, 0xD8, 0xDA, 0x100002);
        l->help = "The page before (Page Up)";
        setHandlers(l, act(kJbPageUp), &Frontend::hitImage);
        Widget* rr = widgetNew();
        addChild(root, rr);
        setRect(rr, 634 - spr_.width(0xD9), kRowsY + 4 * kRowH, spr_.width(0xD9), spr_.height(0xD9));
        setImage(rr, 0xD9, 0xDB, 0x100004);
        rr->help = "The next page (Page Down)";
        setHandlers(rr, act(kJbPageDown), &Frontend::hitImage);
    }
    text(std::to_string(page + 1) + " / " + std::to_string(pages), 0, kRowsY + kRows * kRowH + 2, 0, 12, 0x800020);   // menu2: shorter
    // menu3: one "now playing" panel: what plays (its name = its place in the game), the bar with the time beside it,
    // the info line (Info on)
    text(fit(t.now, 0x20000, kPanel[2] - 16), kBarRect[0], kPanel[1] + 2, kPanel[2] - 16, 24, 0x20000);
    Widget* bar = widgetNew();
    addChild(root, bar);
    setRect(bar, kBarRect[0], kBarRect[1], kBarRect[2], kBarRect[3]);
    bar->user = std::max(0, t.progress);
    bar->flags |= 0x100;
    if (t.progress >= 0) {
        bar->draw = &Frontend::drawJukeboxBar;
        bar->help = "Click to jump there";
        setHandlers(bar, [this, act](Widget& w) {
            int x, y, ww, hh;
            absRect(w, x, y, ww, hh);
            act(kJbSeek + std::clamp((px_ - x) * 1000 / std::max(1, ww), 0, 1000))(w);
        }, &Frontend::hitRect);
    } else if (!t.time.empty()) {
        text("Length not known yet", kBarRect[0], kBarRect[1], kBarRect[2], 12, 0x800020);
    }
    text(t.time, kBarRect[0] + kBarRect[2] + 10, kBarRect[1] - 6, kPanel[0] + kPanel[2] - 8 - (kBarRect[0] + kBarRect[2] + 10), 24, 0x20004);
    if (gInfo)   // menu2: the file / codes only with Info on (menu3: inside the panel, under the bar)
        text(fit(t.info, 0x800000, kPanel[2] - 16), kBarRect[0], kBarRect[1] + kBarRect[3] + 3, kPanel[2] - 16, 12, 0x800000);
    {   // the panel itself: added after its texts (addChild puts a widget first: it is drawn under them)
        Widget* panel = widgetNew();
        addChild(root, panel);
        setRect(panel, kPanel[0], kPanel[1], kPanel[2], kPanel[3]);
        panel->user = -1;
        panel->draw = &Frontend::drawJukeboxBar;
    }
    // the buttons
    struct Button { std::string label, help; int action; };
    const Button buttons[5] = {
        {"<<", "Previous (P; pad: LB)", kJbPrev},
        {"Play", "Play (Enter; pad: A)", kJbPlay},
        {"Stop", "Stop (S; pad: X)", kJbStop},
        {">>", "Next (N; pad: RB)", kJbNext},
        {t.mode, "Once, Loop one, Loop all, Shuffle (L; pad: Y)", kJbMode},
    };
    for (int c = 0; c < 5; ++c) {
        Widget* w = text(buttons[c].label, 8 + c * 104, 0x182, 104, 24, 0x20020);
        w->help = buttons[c].help;
        setHandlers(w, act(buttons[c].action), &Frontend::hitText);
    }
    // menu2: Info (the number column and the file / codes line), the last of the row
    {
        Widget* w = text(gInfo ? "Info: On" : "Info", 8 + 5 * 104, 0x182, 104, 24, 0x20020);
        w->help = "Show the file, codes and numbers";
        setHandlers(w, [this](Widget&) {
            walking_ = false;
            gInfo = !gInfo;
            screenJukebox();
        }, &Frontend::hitText);
    }
    // the volumes (the game's Music / Sound Effects settings) and Exit
    auto volume = [&](int x, const std::string& name, int v, int down, int up) {
        Widget* m = text("-", x, 0x1A2, 24, 24, 0x20020);
        m->help = name + " volume down (- key)";
        setHandlers(m, act(down), &Frontend::hitRect);
        text(name + " " + std::to_string(v) + "%", x + 24, 0x1A2, 150, 24, 0x20020);
        Widget* p = text("+", x + 174, 0x1A2, 24, 24, 0x20020);
        p->help = name + " volume up (+ key)";
        setHandlers(p, act(up), &Frontend::hitRect);
    };
    volume(16, "Music", t.musicVolume, kJbMusicDown, kJbMusicUp);
    volume(226, "Effects", t.sfxVolume, kJbSfxDown, kJbSfxUp);
    Widget* exit = text("Exit", 470, 0x1A2, 150, 24, 0x20020);
    exit->help = "Back to the main menu (Escape)";
    exit->tag = 1;   // Escape
    setHandlers(exit, act(kJbExit), &Frontend::hitText);
    // the texts this page shows (refreshJukebox)
    jukeboxSig_ = jukeboxSigOf(t);
}

// the texts the Jukebox page shows (refreshJukebox: a change builds it again)
std::string Frontend::jukeboxSigOf(const JukeboxText& t)
{
    return t.now + "|" + t.time + "|" + t.info + "|" + t.mode + "|" + t.source + "|" + std::to_string(t.progress / 4) + "|" +
           std::to_string(t.selected) + "|" + std::to_string(t.playing) + "|" + std::to_string(t.tab) + "|" +
           std::to_string(int(t.rows.size())) + "|" + std::to_string(t.musicVolume) + "|" + std::to_string(t.sfxVolume);
}

// menuperf: the Remake Jukebox's texts but its time, its progress and the chosen row on the same page of the list
// (those change in place: jukeboxInPlace)
std::string Frontend::jukeboxFixedSig(const JukeboxText& t)
{
    return t.now + "|" + t.info + "|" + t.mode + "|" + t.source + "|" + (t.progress >= 0 ? "bar" : "nobar") +
           (t.time.empty() ? "|notime|" : "|time|") + std::to_string(t.selected / kRows) + "|" + std::to_string(t.playing) +
           "|" + std::to_string(t.tab) + "|" + std::to_string(int(t.rows.size())) + "|" + std::to_string(t.musicVolume) + "|" +
           std::to_string(t.sfxVolume) + (gInfo ? "|info" : "");
}

// menuperf (user 2026-10-26: "the Jukebox page lags"): the Remake page with only its time / progress / chosen row
// changed (each second, each 0.4 % of the song, Up / Down): those widgets updated in place (each its own small part,
// Frontend::Widget::live), not a new page (the whole page drawn again: 20-65 ms each time). False: build it again
bool Frontend::jukeboxInPlace(const JukeboxText& t)
{
    if (screen_ != 20 || style != kStyleRemake || sc_ <= 1 || !jbTime_ || !jbBar_ || cw_ != wideW_ || jukeboxFixedSig(t) != jbFixedSig_)
        return false;
    jbTime_->text = t.time;
    jbBar_->user = std::max(0, t.progress);
    for (const auto& [w, i] : jbRows_)
        w->user = (i == t.selected ? 1 : 0) | (i == t.playing ? 2 : 0);
    jukeboxSig_ = jukeboxSigOf(t);
    return true;
}

void Frontend::refreshJukebox()
{
    if (screen_ != 20 || !jukeboxText)
        return;
    const JukeboxText t = jukeboxText();
    if (jukeboxSigOf(t) != jukeboxSig_ && !jukeboxInPlace(t))
        screenJukebox();
}

// ---- remakepages: the Remake style's Jukebox (docs/menu_remake.md): the same layout, lists, buttons and keys on a
// torn panel: the lists as parchment tabs (Q / E), the chosen row on a red bar, the "now playing" box torn with a
// blood-red bar, the buttons as parchment strips, the prompts at the bottom right ----
void Frontend::remakeJukebox()
{
    const JukeboxText t = jukeboxText ? jukeboxText() : JukeboxText{};
    if (jukeboxInPlace(t))   // menuperf: the same page, its time / bar / chosen row changed: no new page
        return;
    remakePage(20, "jukebox", false);
    rebuild_ = [this]() { remakeJukebox(); };
    const std::function<void(int)> act = [this](int a) {
        walking_ = false;
        if (onJukebox) {
            const auto f = onJukebox;
            f(a);
        }
        if (a == kJbExit)
            screenMainMenu();
        else if (screen_ == 20)
            screenJukebox();
    };
    auto fit = [&](std::string s, float h, float w) {   // a text cut to fit w units
        return util::utf8Fit(s, "..", [&](const std::string& t) { return rTextWidth(t, h) <= w; });   // (misc18: by letters)
    };
    const int pw = std::min(720, cw_ - 40), px = (cw_ - pw) / 2;
    Widget* title = rWidget(0, 0, cw_, 40, &Frontend::drawRLabel, "Jukebox");
    title->user = 0xFFF2E6, title->seed = 47;
    {
        const std::string label = "Sound: " + t.source;
        const int sw = std::max(120, int(rTextWidth(label, 13.0f)) + 30);
        Widget* src = rStripButton(px + pw - sw, 8, sw, 22, label,
                                   t.otherSource ? "The arcade sound board (the ROMs' own sound program) or the PC release's sounds and music files. Key: Tab"
                                                 : "Only one sound source is there",
                                   t.otherSource ? std::function<void()>([act]() { act(kJbSource); }) : nullptr, 71);
        src->nav = false;
    }
    // the lists as tabs over the panel (the current one raised)
    const int n = int(t.tabs.size()), tabW = std::min(170, pw / std::max(1, n)), tx0 = (cw_ - n * tabW) / 2;
    rTabsRtl_ = lang::rtl();   // (rtl: the first list at the right)
    for (int i = 0; i < n; ++i) {
        const bool cur = i == t.tab;
        Widget* tw = rWidget(tx0 + (rTabsRtl_ ? n - 1 - i : i) * tabW, cur ? 36 : 42, tabW, cur ? 42 : 36, &Frontend::drawRTab, t.tabs[size_t(i)]);
        tw->user = cur ? 1 : 0;
        tw->seed = 500 + i * 13;
        tw->help = cur ? t.tabs[size_t(i)] + " (this list)" : "The " + t.tabs[size_t(i)] + " list. Keys: Left / Right, Q / E";
        if (!cur)
            setHandlers(tw, [act, i](Widget&) { act(kJbTab + i); }, &Frontend::hitRect);
    }
    remakeTabs_ = n, rTabCur_ = t.tab;
    rTabFn_ = [act](int i) { act(kJbTab + i); };
    // the panel: the list's page, ten rows, the chosen one on a red bar, the one playing marked; the page arrows
    const int py = 76, rowH = 20, ph = 10 * rowH + 34;
    rPage_[0] = px, rPage_[1] = py, rPage_[2] = pw, rPage_[3] = ph, rPage_[4] = 9;
    const int count = int(t.rows.size()), pages = std::max(1, (count + kRows - 1) / kRows);
    const int page = std::clamp(t.selected / kRows, 0, pages - 1);
    const int rx = px + 40, rw = pw - 80;
    for (int r = 0; r < kRows; ++r) {
        const int i = page * kRows + r;
        if (i >= count)
            break;
        const bool sel = i == t.selected, on = i == t.playing;
        const int numW = gInfo ? 80 : 0;
        Widget* w = rWidget(rx, py + 10 + r * rowH, rw, rowH - 1, &Frontend::drawRJRow,
                            fit(lang::rtl() ? lang::tr(t.rows[size_t(i)].first) + (on ? " <" : "") : (on ? "> " : "") + lang::tr(t.rows[size_t(i)].first),
                                14.0f, float(rw - 12 - numW)));
        w->rtlRow = lang::rtl();   // (leftovers: mirrored, drawRJRow)
        w->user = (sel ? 1 : 0) | (on ? 2 : 0);
        w->value = gInfo ? t.rows[size_t(i)].second : std::string();
        w->seed = 800 + r * 7;
        w->live = true;   // menuperf: the chosen row's bar moves in place (jukeboxInPlace)
        jbRows_.push_back({w, i});
        w->help = on ? "Playing: click to start it again" : "Play this one (Enter plays the chosen row; Up / Down choose)";
        setHandlers(w, [act, i](Widget&) { act(kJbRow + i); }, &Frontend::hitRect);
    }
    if (count == 0) {
        Widget* none = rWidget(px, py + 80, pw, 30, &Frontend::drawRLabel, "Nothing in this list");
        none->user = 0x6A625C, none->seed = 53;
    }
    if (pages > 1)
        for (int d = 0; d < 2; ++d) {
            Widget* a = rWidget(d ? px + pw - 36 : px + 8, py + 10 + 4 * rowH, 28, 28, &Frontend::drawRSide);
            a->user = d;
            a->help = d ? "The next page (Page Down)" : "The page before (Page Up)";
            setHandlers(a, [act, d](Widget&) { act(d ? kJbPageDown : kJbPageUp); }, &Frontend::hitRect);
        }
    Widget* pg = rWidget(px, py + ph - 22, pw, 15, &Frontend::drawRLabel, std::to_string(page + 1) + " / " + std::to_string(pages));
    pg->user = 0x8A827A, pg->seed = 57;
    // the "now playing" box: what plays, the bar with the time beside it, the info line (Info on)
    const int by = py + ph + 6, bh = gInfo ? 60 : 48, inner = pw - 28, timeW = 96;
    Widget* now = rWidget(px + 14, by + 7, inner, 14, &Frontend::drawRText, fit(t.now, 14.0f, float(inner)));
    now->user = 0xFFF2E6, now->id = 0, now->seed = 61;
    Widget* bar = rWidget(px + 14, by + 29, inner - timeW - 10, 10, &Frontend::drawRJBar);
    bar->user = std::max(0, t.progress);
    if (t.progress >= 0) {
        bar->help = "Click to jump there";
        setHandlers(bar, [this, act](Widget& w) { act(kJbSeek + std::clamp((px_ - w.x) * 1000 / std::max(1, w.w), 0, 1000)); },
                    &Frontend::hitRect);
    } else {
        bar->draw = nullptr;
        if (!t.time.empty()) {
            Widget* nk = rWidget(px + 14, by + 28, inner - timeW - 10, 11, &Frontend::drawRText, "Length not known yet");
            nk->user = 0x8A827A, nk->id = 0;
        }
    }
    Widget* time = rWidget(px + pw - 14 - timeW, by + 27, timeW, 14, &Frontend::drawRText, t.time);
    time->user = 0xE2DAD2, time->id = 2, time->seed = 63;
    time->live = bar->live = true;   // menuperf: these two change as it plays: their own parts (refreshJukebox)
    jbTime_ = time, jbBar_ = bar;
    jbFixedSig_ = jukeboxFixedSig(t);
    if (gInfo) {
        Widget* info = rWidget(px + 14, by + 45, inner, 10, &Frontend::drawRText, fit(t.info, 10.0f, float(inner)));
        info->user = 0x8A827A, info->id = 0, info->seed = 65;
    }
    Widget* box = rWidget(px, by, pw, bh, &Frontend::drawRBox);   // (added after its texts: drawn under them)
    box->seed = 67;
    // the buttons: Previous, Play, Stop, Next, the mode, Info
    const int y1 = by + bh + 7, gap = 8, bw = (pw - 5 * gap) / 6;
    struct Button { std::string label, help; int action; };
    const Button buttons[5] = {
        {"<<", "Previous (P; pad: LB)", kJbPrev},
        {"Play", "Play (Enter; pad: A)", kJbPlay},
        {"Stop", "Stop (S; pad: X)", kJbStop},
        {">>", "Next (N; pad: RB)", kJbNext},
        {t.mode, "Once, Loop one, Loop all, Shuffle (L; pad: Y)", kJbMode},
    };
    for (int c = 0; c < 5; ++c) {
        const int a = buttons[c].action;
        rStripButton(px + c * (bw + gap), y1, bw, 22, buttons[c].label, buttons[c].help, [act, a]() { act(a); }, 820 + c * 5);
    }
    rStripButton(px + 5 * (bw + gap), y1, bw, 22, gInfo ? "Info: On" : "Info", "Show the file, codes and numbers", [this]() {
        gInfo = !gInfo;
        screenJukebox();
    }, 850);
    // the volumes (the game's Music / Sound Effects settings) and Exit
    const int y2 = y1 + 28, vw = 118, sw = 26, groupW = 2 * (sw + vw + sw) + 16 + 12 + 110;
    int vx = px + (pw - groupW) / 2;
    auto volume = [&](const std::string& name, int v, int down, int up, int seed) {
        rStripButton(vx, y2, sw, 22, "-", name + " volume down (- key)", [act, down]() { act(down); }, seed);
        Widget* l = rWidget(vx + sw, y2 + 5, vw, 13, &Frontend::drawRText, name + " " + std::to_string(v) + "%");
        l->user = 0xE2DAD2, l->id = 1, l->seed = seed + 1;
        rStripButton(vx + sw + vw, y2, sw, 22, "+", name + " volume up (+ key)", [act, up]() { act(up); }, seed + 2);
        vx += sw + vw + sw + 16;
    };
    volume("Music", t.musicVolume, kJbMusicDown, kJbMusicUp, 860);
    volume("Effects", t.sfxVolume, kJbSfxDown, kJbSfxUp, 870);
    rStripButton(vx - 4, y2, 110, 22, "Exit", "Back to the main menu (Escape)", [act]() { act(kJbExit); }, 300);
    rPanel_[0] = px, rPanel_[1] = 0, rPanel_[2] = pw, rPanel_[3] = y2 + 22;   // (the prompts: a row when they would meet)
    rHelp_[1] = y2 + 28;
    remakePrompts(root_, n > 1, [act]() { act(kJbExit); });
    // the texts this page shows (refreshJukebox)
    jukeboxSig_ = jukeboxSigOf(t);
}

} // namespace hotd::game
