// menufb (user 2026-10-26): the settings lists' scroll bars and the Firing Range page (game/frontend.h), both menu styles.
//
// Scroll bars: "no indication that the list is scrollable so add a scroll bar to the right side": a thin track at the
// right of a settings page's rows (the Remake style: its scrolled rows; the Classic style: its pages) and of every
// dropdown list longer than its room, the thumb the part shown. The mouse drags the thumb or clicks the track (the
// thumb's middle goes there); the wheel and the keys scroll as before.
//
// The Firing Range: "take them to a different page so they can test their crosshairs and shoot ... player 1 2 3 4 want
// different options": the crosshair rows (per player: menu_pages.h menuRangeRows) above a range with targets (paper
// bullseyes on the game's picture). Every player aims there with their own input (the mouse; a gun as the mouse; the
// host hands in the pads' aims and triggers: rangeAim) and sees their own crosshair as the game draws it; a hit bursts
// the target in the player's colour with its points (10 the bull, 5, 2), a miss leaves a hole; hits / shots per player
// along the top. The targets come back elsewhere after a moment.
#include "game/frontend.h"
#include "game/lang.h"
#include "game/player_colours.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace hotd::game {

namespace {
uint32_t seatRgb(int s)
{
    const uint8_t* c = kPlayerRgb[s & 3];
    return (uint32_t(c[0]) << 16) | (uint32_t(c[1]) << 8) | c[2];
}
constexpr uint32_t kRangeHitMs = 700;   // a hit target's burst; then it comes back elsewhere
}

// ---- scroll bars ----

void Frontend::scrollBar(Widget* parent, int x, int y, int w, int h, int first, int vis, int total, std::function<void(int)> set,
                         bool append)
{
    if (total <= vis || vis <= 0 || h <= 8)
        return;
    ScrollBar b;
    b.x = x, b.y = y, b.w = w, b.h = h, b.first = std::clamp(first, 0, total - vis), b.vis = vis, b.total = total;
    b.set = std::move(set);
    sbars_.push_back(std::move(b));
    Widget* bw = widgetNew();
    if (append) {
        Widget** tail = &parent->child;
        while (*tail)
            tail = &(*tail)->next;
        *tail = bw;
    } else
        addChild(parent, bw);
    setRect(bw, x, y, w, h);
    bw->user = int(sbars_.size()) - 1;
    bw->seed = first * 1000 + total;   // (the static layer: drawn again when the thumb moves)
    bw->draw = sc_ > 1 ? &Frontend::drawRScroll : &Frontend::drawCScroll;
    bw->flags |= 0x100;
}

void Frontend::sbThumb(const ScrollBar& b, int& ty, int& th)
{
    th = std::max(std::min(12, b.h), b.h * b.vis / std::max(1, b.total));
    const int range = std::max(1, b.total - b.vis);
    ty = b.y + (b.h - th) * std::clamp(b.first, 0, range) / range;
}

bool Frontend::scrollBarInput(const FrontendInput& in)
{
    const int x = in.mouseMoved ? in.mouseX : px_, y = in.mouseMoved ? in.mouseY : py_;
    // menufb: a value bar: a click sets the value there, a drag follows the mouse
    auto slideTo = [&](int i) {
        const SliderBar& b = slbars_[size_t(i)];
        const float k = std::clamp(float(x - b.x - 4) / float(std::max(1, b.w - 8)), 0.0f, 1.0f);
        const int v = std::clamp(int(std::lround(k * float(b.max) / float(b.step))) * b.step, 0, b.max);
        if (v != b.value && b.set) {
            const auto set = b.set;   // (it builds the page again: slbars_ anew)
            set(v);
        }
    };
    if (slDrag_ >= 0) {
        if (!in.mouseHeld || slDrag_ >= int(slbars_.size())) {
            slDrag_ = -1;
            return false;
        }
        if (in.mouseMoved) {
            px_ = std::clamp(x, 0, cw_ - 3), py_ = std::clamp(y, 0, kHeight - 3);
            slideTo(slDrag_);
        }
        return true;
    }
    if (in.press && in.click)
        for (size_t i = 0; i < slbars_.size(); ++i) {
            const SliderBar& b = slbars_[i];
            if (x < b.x - 3 || x >= b.x + b.w + 3 || y < b.y || y >= b.y + b.h)
                continue;
            slDrag_ = int(i);
            slideTo(int(i));
            if (onCue)
                onCue(28);
            return true;
        }
    auto firstAt = [](const ScrollBar& b, int top) {   // the first item for the thumb's top at `top`
        int ty, th;
        sbThumb(b, ty, th);
        const int span = std::max(1, b.h - th);
        return std::clamp(int(std::lround(float(top - b.y) / float(span) * float(b.total - b.vis))), 0, b.total - b.vis);
    };
    if (sbDrag_ >= 0) {
        if (!in.mouseHeld || sbDrag_ >= int(sbars_.size())) {
            sbDrag_ = -1;
            return false;
        }
        if (in.mouseMoved) {
            px_ = std::clamp(x, 0, cw_ - 3), py_ = std::clamp(y, 0, kHeight - 3);   // (the hand follows)
            const ScrollBar& b = sbars_[size_t(sbDrag_)];
            const int f = firstAt(b, y - sbGrab_);
            if (f != b.first && b.set) {
                const auto set = b.set;   // (it builds the page again: sbars_ anew)
                set(f);
            }
        }
        return true;
    }
    if (!(in.press && in.click))
        return false;
    for (size_t i = 0; i < sbars_.size(); ++i) {
        const ScrollBar& b = sbars_[i];
        if (x < b.x - 4 || x >= b.x + b.w + 4 || y < b.y || y >= b.y + b.h)
            continue;
        int ty, th;
        sbThumb(b, ty, th);
        sbDrag_ = int(i);
        if (y >= ty && y < ty + th)
            sbGrab_ = y - ty;   // (the thumb: dragged from where it was taken)
        else {
            sbGrab_ = th / 2;   // (the track: the thumb's middle there, then dragged)
            const int f = firstAt(b, y - sbGrab_);
            if (f != b.first && b.set) {
                const auto set = b.set;
                set(f);
            }
        }
        if (onCue)
            onCue(28);
        return true;
    }
    return false;
}

// the Remake style's bar: a dark rounded groove, the thumb in the blood colour with a light edge
void Frontend::drawRScroll(Widget& w)
{
    if (w.user < 0 || w.user >= int(sbars_.size()))
        return;
    const ScrollBar& b = sbars_[size_t(w.user)];
    int ty, th;
    sbThumb(b, ty, th);
    const float S = ps_;
    const float cx = float(b.x) + float(b.w) / 2, r = float(b.w) / 2;
    auto capsule = [&](float y0, float y1, float rad, uint32_t rgb, float a) {
        for (int py = int((y0 - 1) * S); py <= int((y1 + 1) * S); ++py)
            for (int px = int((cx - rad - 1) * S); px <= int((cx + rad + 1) * S); ++px) {
                const float ux = float(px) / S + 0.5f / S, uy = float(py) / S + 0.5f / S;
                const float cy = std::clamp(uy, y0 + rad, std::max(y0 + rad, y1 - rad));
                const float d = std::sqrt((ux - cx) * (ux - cx) + (uy - cy) * (uy - cy));
                const float cov = std::clamp((rad - d) * S + 0.5f, 0.0f, 1.0f);
                if (cov > 0)
                    rBlend(px, py, rgb, a * cov);
            }
    };
    capsule(float(b.y), float(b.y + b.h), r * 0.55f, 0x050303, 0.7f);
    const bool drag = sbDrag_ == w.user;
    capsule(float(ty), float(ty + th), r, 0x1A0E0C, 0.9f);
    capsule(float(ty) + 1, float(ty + th) - 1, r - 1.2f, tint(drag ? 0xD0180E : 0xA00C08), 0.95f);
}

// the Classic style's bar: a grey groove, the thumb in the menu's grey (yellow while dragged)
void Frontend::drawCScroll(Widget& w)
{
    if (w.user < 0 || w.user >= int(sbars_.size()) || !canvas_)
        return;
    const ScrollBar& b = sbars_[size_t(w.user)];
    int ty, th;
    sbThumb(b, ty, th);
    std::vector<uint8_t>& c = *canvas_;
    const int W = cw_ * sc_, H = int(c.size() / 4) / std::max(1, W);
    const bool drag = sbDrag_ == w.user;
    for (int py = std::max(0, b.y); py < std::min(H, b.y + b.h); ++py)
        for (int px = std::max(0, b.x); px < std::min(W, b.x + b.w); ++px) {
            uint8_t* d = &c[(size_t(py) * size_t(W) + size_t(px)) * 4];
            const bool thumb = py >= ty && py < ty + th;
            const bool edge = px == b.x || px == b.x + b.w - 1;
            if (thumb) {
                const uint8_t v = edge ? 0x60 : 0xB0;
                d[0] = drag ? 0xF0 : v, d[1] = drag ? 0xD0 : v, d[2] = drag ? 0x30 : v;
            } else
                d[0] = d[1] = d[2] = edge ? 0x50 : 0x20;
            d[3] = 255;
        }
}

// ---- menufb: value bars (the volumes, the custom blood colour's red / green / blue) ----

void Frontend::sliderBar(Widget* parent, int x, int y, int w, int h, int value, int max, int step, std::function<void(int)> set)
{
    if (w < 20 || max <= 0)
        return;
    SliderBar b;
    b.x = x, b.y = y, b.w = w, b.h = h, b.value = std::clamp(value, 0, max), b.max = max, b.step = std::max(1, step);
    b.set = std::move(set);
    slbars_.push_back(std::move(b));
    Widget* bw = widgetNew();
    addChild(parent, bw);
    setRect(bw, x, y, w, h);
    bw->user = int(slbars_.size()) - 1;
    bw->seed = value;   // (the static layer: drawn again when the value changes)
    bw->draw = sc_ > 1 ? &Frontend::drawRSlider : &Frontend::drawCSlider;
    bw->over = sc_ > 1;   // (the Remake style: over a lit row's focus bar)
    bw->value = std::to_string(value);
    bw->flags |= 0x100;
}

// the Remake style's bar: a dark groove, the part up to the value in the blood colour, a light knob
void Frontend::drawRSlider(Widget& w)
{
    if (w.user < 0 || w.user >= int(slbars_.size()))
        return;
    const SliderBar& b = slbars_[size_t(w.user)];
    const float S = ps_, cy = float(b.y) + float(b.h) / 2, k = float(b.value) / float(b.max);
    const float x0 = float(b.x) + 4, x1 = float(b.x + b.w) - 4, kx = x0 + (x1 - x0) * k;
    for (int py = int((cy - 6) * S); py <= int((cy + 6) * S); ++py)
        for (int px = int((x0 - 5) * S); px <= int((x1 + 5) * S); ++px) {
            const float ux = (float(px) + 0.5f) / S, uy = (float(py) + 0.5f) / S;
            const float dgx = std::max({x0 - ux, ux - x1, 0.0f}), dg = std::sqrt(dgx * dgx + (uy - cy) * (uy - cy));
            const float groove = std::clamp((2.6f - dg) * S, 0.0f, 1.0f);
            if (groove > 0)
                rBlend(px, py, ux <= kx ? tint(0xC0140C) : 0x0A0606, groove * 0.95f);
            const float dk = std::sqrt((ux - kx) * (ux - kx) + (uy - cy) * (uy - cy));
            const float knob = std::clamp((5.0f - dk) * S, 0.0f, 1.0f);
            if (knob > 0)
                rBlend(px, py, dk < 3.6f ? 0xEDE4DA : 0x140A08, knob);
        }
}

// the Classic style's bar: a grey groove, the part up to the value in the menu's yellow, a white knob
void Frontend::drawCSlider(Widget& w)
{
    if (w.user < 0 || w.user >= int(slbars_.size()) || !canvas_)
        return;
    const SliderBar& b = slbars_[size_t(w.user)];
    const int cy = b.y + b.h / 2, x0 = b.x + 3, x1 = b.x + b.w - 3;
    const int kx = x0 + (x1 - x0) * b.value / b.max;
    for (int py = cy - 5; py <= cy + 5; ++py)
        for (int px = x0 - 3; px <= x1 + 3; ++px) {
            const bool knob = std::abs(px - kx) <= 2;
            const bool groove = std::abs(py - cy) <= 1 && px >= x0 && px <= x1;
            if (knob) rangePut(px, py, std::abs(px - kx) == 2 || std::abs(py - cy) == 5 ? 0x303030 : 0xF0F0F0, 1.0f);
            else if (groove) rangePut(px, py, px <= kx ? 0xF0D030 : 0x484848, 1.0f);
        }
}

// ---- the Firing Range ----

void Frontend::rangeReset()
{
    for (int s = 0; s < 4; ++s)
        rHits_[s] = rShots_[s] = rPoints_[s] = 0;
    rHoles_.clear();
    rTargetsSet_ = false;
    for (RangeTarget& t : rTargets_)
        t = RangeTarget{};
}

void Frontend::rangeTargetAt(int k, float& x, float& y) const
{
    const RangeTarget& t = rTargets_[std::clamp(k, 0, kRangeTargets - 1)];
    x = float(rRect_[0]) + t.u * float(rRect_[2]);
    y = float(rRect_[1]) + t.v * float(rRect_[3]);
}

// target k somewhere in the range, apart from the others (a fixed sequence: the same each time the page opens)
void Frontend::rangePlace(int k)
{
    const float R = std::min(float(rRect_[3]) * 0.17f, 24.0f);
    const float mu = (R + 4) / std::max(1.0f, float(rRect_[2])), mv = (R + 14) / std::max(1.0f, float(rRect_[3]));
    float bu = 0.5f, bv = 0.55f, best = -1;
    for (int tries = 0; tries < 24; ++tries) {
        rRng_ = rRng_ * 1103515245u + 12345u;
        const float u = mu + float((rRng_ >> 8) & 1023) / 1023.0f * std::max(0.0f, 1 - 2 * mu);
        rRng_ = rRng_ * 1103515245u + 12345u;
        const float v = mv + 0.04f + float((rRng_ >> 8) & 1023) / 1023.0f * std::max(0.0f, 1 - 2 * mv - 0.04f);
        float near = 1e9f;
        for (int j = 0; j < kRangeTargets; ++j)
            if (j != k && rTargetsSet_) {
                const float dx = (u - rTargets_[j].u) * float(rRect_[2]), dy = (v - rTargets_[j].v) * float(rRect_[3]);
                near = std::min(near, std::sqrt(dx * dx + dy * dy));
            }
        if (near > best)
            best = near, bu = u, bv = v;
        if (near > 3 * R)
            break;
    }
    rTargets_[k].u = bu, rTargets_[k].v = bv;
    rTargets_[k].hitMs = 0, rTargets_[k].hitBy = -1, rTargets_[k].points = 0;
}

void Frontend::rangeWidgets(Widget* parent, int x, int y, int w, int h)
{
    rRect_[0] = x, rRect_[1] = y, rRect_[2] = w, rRect_[3] = h;
    rangeOn_ = true;
    if (!rTargetsSet_) {
        rRng_ = 12345;
        for (int k = 0; k < kRangeTargets; ++k) {   // (spread across: one in each quarter's band first)
            rangePlace(k);
            rTargets_[k].u = (float(k) + 0.25f + rTargets_[k].u * 0.5f) / float(kRangeTargets);
        }
        rTargetsSet_ = true;
    }
    // (under an open dropdown: drawn in the page's layer, the list over it; else each its own part over the focus)
    const bool front = !ddOpen() && !cdOpen();
    rangeBg_ = widgetNew();
    addChild(parent, rangeBg_);
    setRect(rangeBg_, x, y, w, h);
    rangeBg_->draw = &Frontend::drawRangeBg;
    rangeBg_->flags |= 0x100;
    rangeBg_->over = front && sc_ > 1;
    rangeBg_->help = "The Firing Range: aim and shoot the targets (the mouse, a light gun, a pad: each player with their own)";
    for (int k = 0; k < kRangeTargets; ++k) {
        Widget* t = widgetNew();
        addChild(parent, t);
        t->user = k;
        t->draw = &Frontend::drawRangeTarget;
        t->flags |= 0x100;
        t->over = front && sc_ > 1;
        t->help = rangeBg_->help;
        rangeTw_[k] = t;
    }
    for (int s = 0; s < 4; ++s) {
        Widget* c = widgetNew();
        addChild(parent, c);
        c->user = s;
        c->draw = &Frontend::drawRangeSight;
        c->flags |= 0x100;
        c->over = sc_ > 1;
        c->live = sc_ > 1;   // (the Remake style: its part not kept, it moves every frame)
        rangeXw_[s] = c;
    }
    if (parent == root_) {   // (the walks draw in the list's order: the ground first, the crosshairs last)
        std::vector<Widget*> mine;
        for (int k = 0; k < 1 + kRangeTargets + 4 && root_->child; ++k) {
            Widget* hd = root_->child;
            root_->child = hd->next;
            hd->next = nullptr;
            mine.insert(mine.begin(), hd);
        }
        Widget** tail = &root_->child;
        while (*tail)
            tail = &(*tail)->next;
        for (Widget* m2 : mine) {
            *tail = m2;
            tail = &m2->next;
        }
    }
    rangeLayout();
}

const SightArt* Frontend::rangeSight(int seat, int wh)
{
    if (!rangeArt)
        return nullptr;
    const std::string key = (rangeSig ? rangeSig() : std::string()) + "#" + std::to_string(seat) + "#" + std::to_string(wh);
    auto it = rangeArt_.find(key);
    if (it == rangeArt_.end()) {
        if (rangeArt_.size() > 64)
            rangeArt_.clear();
        SightArt a;
        if (!rangeArt(seat, wh, a))
            a = SightArt{};
        it = rangeArt_.emplace(key, std::move(a)).first;
    }
    return it->second.w > 0 ? &it->second : nullptr;
}

void Frontend::rangeShot(int seat, float x, float y)
{
    if (x < rRect_[0] || y < rRect_[1] || x >= rRect_[0] + rRect_[2] || y >= rRect_[1] + rRect_[3])
        return;
    ++rShots_[seat];
    rFlashMs_[seat] = std::max(1u, rNowMs_);
    const float R = std::min(float(rRect_[3]) * 0.17f, 24.0f);
    for (int k = 0; k < kRangeTargets; ++k) {
        RangeTarget& t = rTargets_[k];
        if (t.hitMs)
            continue;
        float tx, ty;
        rangeTargetAt(k, tx, ty);
        const float d = std::sqrt((x - tx) * (x - tx) + (y - ty) * (y - ty));
        if (d > R)
            continue;
        t.points = d <= R * 0.25f ? 10 : d <= R * 0.6f ? 5 : 2;
        t.hitMs = std::max(1u, rNowMs_);
        t.hitBy = seat;
        ++rHits_[seat];
        rPoints_[seat] += t.points;
        return;
    }
    if (rHoles_.size() >= 24)   // a miss: a hole (the oldest goes)
        rHoles_.erase(rHoles_.begin());
    rHoles_.push_back({(x - float(rRect_[0])) / float(rRect_[2]), (y - float(rRect_[1])) / float(rRect_[3]), seat});
}

void Frontend::rangeTick(FrontendInput& in, uint32_t nowMs)
{
    rNowMs_ = nowMs;
    if (!rangeShown()) {
        rangeHand_ = false;
        return;
    }
    const bool listOpen = ddOpen() || cdOpen();
    const int mx = in.mouseMoved ? in.mouseX : px_, my = in.mouseMoved ? in.mouseY : py_;
    const bool mouseIn = !listOpen && (sc_ == 1 || handShown_ || in.mouseMoved) && mx >= rRect_[0] && my >= rRect_[1] &&
                         mx < rRect_[0] + rRect_[2] && my < rRect_[1] + rRect_[3] && sbDrag_ < 0;
    rangeHand_ = mouseIn;
    const int ms = std::clamp(rangeMouseSeat, 0, 3);
    for (int s = 0; s < 4; ++s) {
        RangeAim& a = rAimNow_[s];
        a = RangeAim{};
        if (listOpen)
            continue;
        if (rangeAim[s].on) {
            a.on = true, a.x = rangeAim[s].x, a.y = rangeAim[s].y;
            if (rangeAim[s].shot)
                rangeShot(s, a.x, a.y);
        }
        if (s == ms && mouseIn) {
            a.on = true, a.x = float(mx) + 0.5f, a.y = float(my) + 0.5f;
            if (in.press && in.click) {
                rangeShot(s, a.x, a.y);
                in.press = in.click = false;   // (the shot: no row clicked, no splatter)
            }
        }
        // (inside the range only)
        if (a.on && (a.x < rRect_[0] || a.y < rRect_[1] || a.x >= rRect_[0] + rRect_[2] || a.y >= rRect_[1] + rRect_[3]))
            a.on = false;
    }
    for (int k = 0; k < kRangeTargets; ++k)
        if (rTargets_[k].hitMs && nowMs - rTargets_[k].hitMs > kRangeHitMs)
            rangePlace(k);
    rangeLayout();
}

// the widgets onto the state: the targets' boxes (their burst's room), the crosshairs at the aims, the signatures the
// Remake parts are drawn again by
void Frontend::rangeLayout()
{
    if (!rangeOn_ || !rangeBg_)
        return;
    const float R = std::min(float(rRect_[3]) * 0.17f, 24.0f);
    std::string bg = std::to_string(rHoles_.size()) + "|" + std::to_string(rRect_[2]) + (rangeSig ? rangeSig() : std::string());
    for (int s = 0; s < 4; ++s)
        bg += "|" + std::to_string(rHits_[s]) + "," + std::to_string(rShots_[s]) + "," + std::to_string(rPoints_[s]);
    if (!rHoles_.empty())
        bg += "|" + std::to_string(int(rHoles_.back().u * 1000)) + "," + std::to_string(int(rHoles_.back().v * 1000));
    rangeBg_->value = bg + (rangeKeys ? "|keys" : "");
    for (int k = 0; k < kRangeTargets; ++k) {
        Widget* t = rangeTw_[k];
        if (!t)
            continue;
        float tx, ty;
        rangeTargetAt(k, tx, ty);
        const int box = int(R * 1.9f) + 4;
        t->x = int(tx) - box, t->y = int(ty) - box - 10, t->w = 2 * box, t->h = 2 * box + 10;
        const RangeTarget& g = rTargets_[k];
        const int phase = g.hitMs ? int(std::min<uint32_t>(rNowMs_ - g.hitMs, kRangeHitMs) / 35) + 1 : 0;   // (20 steps)
        t->value = std::to_string(phase) + "," + std::to_string(g.hitBy) + "," + std::to_string(int(tx)) + "," + std::to_string(int(ty));
    }
    for (int s = 0; s < 4; ++s) {
        Widget* c = rangeXw_[s];
        if (!c)
            continue;
        const RangeAim& a = rAimNow_[s];
        if (!a.on) {
            c->w = c->h = 0;
            c->value.clear();
            continue;
        }
        const SightArt* art = rangeSight(s, sc_ > 1 ? rph() : kHeight);
        const float S = sc_ > 1 ? ps_ : 1.0f;
        const int half = std::max(art ? int(std::ceil(float(std::max(art->dw, art->dh)) / S / 2)) + 3 : 12, 13);   // (the flash's room)
        c->x = int(a.x) - half, c->y = int(a.y) - half, c->w = c->h = 2 * half;
        const int ph = rFlashMs_[s] && rNowMs_ - rFlashMs_[s] < 200 ? int(rNowMs_ - rFlashMs_[s]) / 20 + 1 : 0;   // (the flash's steps)
        c->value = std::to_string(int(a.x * 4)) + "," + std::to_string(int(a.y * 4)) + "," + std::to_string(ph);
    }
}

// a canvas pixel blended over: the Remake style's rBlend (its tiles), the Classic page's canvas
void Frontend::rangePut(int px, int py, uint32_t rgb, float a)
{
    if (sc_ > 1) {
        rBlend(px, py, rgb, a);
        return;
    }
    std::vector<uint8_t>& cv = *canvas_;
    const int CW = cw_ * sc_, CH = int(cv.size() / 4) / std::max(1, CW);
    if (px < 0 || py < 0 || px >= CW || py >= CH || a <= 0)
        return;
    uint8_t* d = &cv[(size_t(py) * size_t(CW) + size_t(px)) * 4];
    a = std::min(a, 1.0f);
    d[0] = uint8_t(float((rgb >> 16) & 255) * a + d[0] * (1 - a));
    d[1] = uint8_t(float((rgb >> 8) & 255) * a + d[1] * (1 - a));
    d[2] = uint8_t(float(rgb & 255) * a + d[2] * (1 - a));
    d[3] = 255;
}

// the range's ground: the game's picture (the live attract's still, else the hall), darker, a frame; the holes; the
// players' scores along its top
void Frontend::drawRangeBg(Widget& w)
{
    int x, y, ww, hh;
    absRect(w, x, y, ww, hh);
    loadPreviewBg();
    const bool rm = sc_ > 1;
    const float S = rm ? ps_ : 1.0f;
    auto put = [this](int px, int py, uint32_t rgb, float a) { rangePut(px, py, rgb, a); };
    const int X0 = int(float(x) * S), Y0 = int(float(y) * S), X1 = int(float(x + ww) * S), Y1 = int(float(y + hh) * S);
    const float bw = float(X1 - X0), bh = float(Y1 - Y0);
    float sw = float(prevBgW_), sh = prevBgW_ > 0 ? sw * bh / bw : 0.0f;
    if (sh > float(prevBgH_))
        sh = float(prevBgH_), sw = sh * bw / bh;
    const float sx0 = (float(prevBgW_) - sw) / 2, sy0 = (float(prevBgH_) - sh) * 0.6f;
    for (int py = Y0; py < Y1; ++py)
        for (int px = X0; px < X1; ++px) {
            uint32_t c = 0x262220;
            if (prevBgW_ > 0) {
                const int sx = std::clamp(int(sx0 + (float(px - X0) + 0.5f) * sw / bw), 0, prevBgW_ - 1);
                const int sy = std::clamp(int(sy0 + (float(py - Y0) + 0.5f) * sh / bh), 0, prevBgH_ - 1);
                const uint8_t* s = &prevBg_[(size_t(sy) * size_t(prevBgW_) + size_t(sx)) * 4];
                c = (uint32_t(s[0] * 0.55f) << 16) | (uint32_t(s[1] * 0.55f) << 8) | uint32_t(s[2] * 0.55f);
            }
            put(px, py, c, 1.0f);
        }
    for (int py = Y0 - 1; py <= Y1; ++py)   // (the frame: dark outside, a light line inside)
        for (int px = X0 - 1; px <= X1; ++px) {
            const bool edge = px == X0 - 1 || px == X1 || py == Y0 - 1 || py == Y1;
            const bool in = px == X0 || px == X1 - 1 || py == Y0 || py == Y1 - 1;
            if (edge) put(px, py, 0x060404, 1.0f);
            else if (in) put(px, py, 0x9A8C80, 0.8f);
        }
    // the holes (a miss): a dark hole with a ring in the shooter's colour
    for (const RangeHole& hlo : rHoles_) {
        const float cx = (float(x) + hlo.u * float(ww)) * S, cy = (float(y) + hlo.v * float(hh)) * S, r = 2.2f * S;
        for (int py = int(cy - r - 2); py <= int(cy + r + 2); ++py)
            for (int px = int(cx - r - 2); px <= int(cx + r + 2); ++px) {
                const float d = std::sqrt((float(px) + 0.5f - cx) * (float(px) + 0.5f - cx) + (float(py) + 0.5f - cy) * (float(py) + 0.5f - cy));
                if (d <= r * 0.7f) put(px, py, 0x050404, 0.95f);
                else if (d <= r + 0.8f) put(px, py, seatRgb(hlo.seat), 0.8f);
            }
    }
    // the scores: "P1 20 (3/5)" for each player who shot; none yet: how to
    std::string line;
    bool any = false;
    for (int s = 0; s < 4; ++s)
        any = any || rShots_[s] > 0;
    // padpass: the keyboard players' aim (Tab): how to, or that it is on
    const char* keysLine = rangeKeys ? "The keys aim here  (Tab or Esc for the menu)" : "Press Tab to aim with the keys";
    if (rm)
        rText(keysLine, float(x) + 6, float(y + hh) - 14, 9.0f, rangeKeys ? 0xF0D060 : 0xB8B0A8, 0, 0.0f, 0, kTxOutline);
    else
        drawSmall(keysLine, y + hh - 12, !rangeKeys);
    if (rm) {
        float tx = float(x) + 6;
        if (!any)
            rText("Aim and shoot the targets", float(x) + 6, float(y) + 4, 10.0f, 0xD8D0C8, 0, 0.0f, 0, kTxOutline);
        else
            for (int s = 0; s < 4; ++s) {
                if (!rShots_[s])
                    continue;
                const std::string t = "P" + std::to_string(s + 1) + " " + std::to_string(rPoints_[s]) + " (" + std::to_string(rHits_[s]) + "/" +
                                      std::to_string(rShots_[s]) + ")";
                const RawText raw(this);
                rText(t, tx, float(y) + 4, 10.0f, seatRgb(s), 0, 0.0f, 0, kTxOutline);
                tx += rTextWidthRaw(t, 10.0f) + 14;
            }
    } else if (!any)
        drawSmall("Aim and shoot the targets", y + 3, false);
    else {
        for (int s = 0; s < 4; ++s)
            if (rShots_[s])
                line += (line.empty() ? "" : "   ") + ("P" + std::to_string(s + 1) + " " + std::to_string(rPoints_[s]) + " (" +
                                                       std::to_string(rHits_[s]) + "/" + std::to_string(rShots_[s]) + ")");
        drawSmallRaw(line, y + 3, false);
    }
}

// a target: a paper bullseye (red rings, the bull red) on a post; hit: it bursts in the shooter's colour, the points
// rising over it
void Frontend::drawRangeTarget(Widget& w)
{
    const int k = std::clamp(w.user, 0, kRangeTargets - 1);
    const RangeTarget& t = rTargets_[k];
    const bool rm = sc_ > 1;
    const float S = rm ? ps_ : 1.0f;
    auto put = [this](int px, int py, uint32_t rgb, float a) { rangePut(px, py, rgb, a); };
    float tx, ty;
    rangeTargetAt(k, tx, ty);
    const float R = std::min(float(rRect_[3]) * 0.17f, 24.0f);
    const float age = t.hitMs ? float(std::min<uint32_t>(rNowMs_ - t.hitMs, kRangeHitMs)) / float(kRangeHitMs) : 0.0f;
    const float cx = tx * S, cy = ty * S, r = R * S * (t.hitMs ? 1.0f - age * 0.6f : 1.0f);
    const float alpha = t.hitMs ? 1.0f - age : 1.0f;
    const float rimR = t.hitMs ? R * S * (1.0f + age * 0.9f) : 0.0f;   // (the burst's ring)
    const float reach = std::max(r, rimR) + 2;
    for (int py = int(cy - reach); py <= int(cy + reach); ++py)
        for (int px = int(cx - reach); px <= int(cx + reach); ++px) {
            const float dx = float(px) + 0.5f - cx, dy = float(py) + 0.5f - cy, d = std::sqrt(dx * dx + dy * dy);
            if (t.hitMs && std::fabs(d - rimR) < 1.6f * S)
                put(px, py, seatRgb(t.hitBy), (1.0f - age) * 0.9f);
            if (d > r + 0.5f)
                continue;
            const float e = std::clamp(r + 0.5f - d, 0.0f, 1.0f);
            const float q = d / std::max(1.0f, r);
            uint32_t c = q <= 0.25f ? 0xC81810 : q <= 0.42f ? 0xEDE6D8 : q <= 0.6f ? 0xC81810 : q <= 0.8f ? 0xEDE6D8 : q <= 0.93f ? 0xC81810 : 0x201010;
            if (t.hitMs)
                c = seatRgb(t.hitBy);
            put(px, py, c, e * alpha);
        }
    if (t.hitMs && rm) {   // the points rising
        const RawText raw(this);
        rText("+" + std::to_string(t.points), tx, ty - R - 6 - age * 8, 11.0f, seatRgb(t.hitBy), 1, 0.0f, 0, kTxOutline);
    }
}

// a player's crosshair where they aim, as the game draws it (the Remake style: its pixels at this window's height; the
// Classic page: at 480 lines), "P1".. under it
void Frontend::drawRangeSight(Widget& w)
{
    const int s = std::clamp(w.user, 0, 3);
    const RangeAim& a = rAimNow_[s];
    if (!a.on)
        return;
    const SightArt* art = rangeSight(s, sc_ > 1 ? rph() : kHeight);
    // the shot's flash at the aim (menufb: Teammate Crosshair Flashes: the mouse's player is this PC's, always full;
    // the others show the setting as players on another PC would: Full, Reduced (smaller, shorter) or Off)
    const bool own = s == std::clamp(rangeMouseSeat, 0, 3);
    const int mode = own || rangeFlash <= 1 ? 1 : rangeFlash;
    const uint32_t dur = mode == 2 ? 70 : 160;
    if (rFlashMs_[s] && mode != 3 && rNowMs_ - rFlashMs_[s] < dur) {
        const bool rm = sc_ > 1;
        const float S = rm ? ps_ : 1.0f, age = float(rNowMs_ - rFlashMs_[s]) / float(dur);
        const float R = (mode == 2 ? 5.0f : 10.0f) * S * (0.6f + 0.4f * age), cx = a.x * S, cy = a.y * S;
        const uint32_t pc = seatRgb(s);
        for (int py = int(cy - R - 1); py <= int(cy + R + 1); ++py)
            for (int px = int(cx - R - 1); px <= int(cx + R + 1); ++px) {
                const float dx = float(px) + 0.5f - cx, dy = float(py) + 0.5f - cy, d = std::sqrt(dx * dx + dy * dy) / R;
                if (d > 1) continue;
                const float ray = 0.6f + 0.4f * std::fabs(std::cos(std::atan2(dy, dx) * 4.0f));   // (a four-pointed burst)
                const float al = (1 - age) * std::clamp((ray - d) * 3.0f, 0.0f, 1.0f);
                rangePut(px, py, d < 0.35f ? 0xFFF6D0 : pc, al);
            }
    }
    if (sc_ > 1) {
        if (art)
            rSightArt(*art, a.x, a.y, 0.0f);
    } else if (art)
        cSightArt(*art, int(a.x), int(a.y), 0);
}

} // namespace hotd::game

namespace hotd::game {
// menufb: a colour square with a dark edge and a light inner line (the custom blood colour's), both styles
void Frontend::drawSwatch(Widget& w)
{
    int x, y, ww, hh;
    absRect(w, x, y, ww, hh);
    const float S = sc_ > 1 ? ps_ : 1.0f;
    const int X0 = int(float(x) * S), Y0 = int(float(y) * S), X1 = int(float(x + ww) * S), Y1 = int(float(y + hh) * S);
    for (int py = Y0; py < Y1; ++py)
        for (int px = X0; px < X1; ++px) {
            const bool edge = px == X0 || py == Y0 || px == X1 - 1 || py == Y1 - 1;
            const bool in = px == X0 + 1 || py == Y0 + 1 || px == X1 - 2 || py == Y1 - 2;
            rangePut(px, py, edge ? 0x080404 : in ? 0xD8D0C8 : w.tint, 1.0f);
        }
}
} // namespace hotd::game
