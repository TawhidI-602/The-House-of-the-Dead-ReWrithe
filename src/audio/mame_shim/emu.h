// The smallest stand-in for the parts of MAME's device framework that MAME's SCSP (scsp.cpp / scspdsp.cpp, supplied
// at build time, BSD-3-Clause) uses: device_t, the sound stream, the ROM / RAM space, timers, callbacks, the serial
// (MIDI) port. Our own code (MIT, this project); the board (src/audio/sound_board.cpp) drives it one 44.1 kHz sample at
// a time. Not MAME's implementation: just enough for one SCSP on HOTD's Model 2 sound board.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;
using offs_t = u32;

#define ATTR_COLD
#define NAME(x) x, #x
#define FUNC(x) &x, #x
#define TIMER_CALLBACK_MEMBER(name) void name(s32 param)
#define COMBINE_DATA(varptr) (*(varptr) = (*(varptr) & ~mem_mask) | (data & mem_mask))
#define DECLARE_DEVICE_TYPE(Type, Class) extern const int Type;
#define DEFINE_DEVICE_TYPE(Type, Class, ShortName, FullName) const int Type = 0;

enum { CLEAR_LINE = 0, ASSERT_LINE = 1, HOLD_LINE = 2 };
enum endianness_t { ENDIANNESS_LITTLE, ENDIANNESS_BIG };

template <typename T> constexpr bool BIT(T x, int n) { return (x >> n) & 1; }
namespace util {
template <typename T> constexpr T sext(T value, int width)
{
    using U = std::make_unsigned_t<T>;
    const U mask = U(1) << (width - 1);
    const U v = U(value) & ((width >= int(sizeof(T) * 8)) ? ~U(0) : ((U(1) << width) - 1));
    return T((v ^ mask) - mask);
}
} // namespace util

template <typename... A> inline void logerror(const char*, A...) {}
template <typename... A> inline void popmessage(const char*, A...) {}

struct machine_config {};

// time in seconds (enough for the SCSP's timers: whole 44.1 kHz samples)
struct attotime {
    double s = 0;
    static attotime from_ticks(u64 ticks, u32 freq) { return attotime{freq ? double(ticks) / double(freq) : 1e30}; }
    static attotime never() { return attotime{1e30}; }
};

// the SCSP's space (dirom.h, scspdsp.h): the board's sound RAM, big-endian bytes, byte addresses; unmapped reads 0
class address_space {
public:
    u8* ram = nullptr;
    u32 size = 0;
    u8 read_byte(offs_t a) const { return a < size ? ram[a] : 0; }
    u16 read_word(offs_t a) const
    {
        a &= ~1u;
        return a + 1 < size ? u16((ram[a] << 8) | ram[a + 1]) : 0;
    }
    void write_word(offs_t a, u16 v)
    {
        a &= ~1u;
        if (a + 1 < size) {
            ram[a] = u8(v >> 8);
            ram[a + 1] = u8(v);
        }
    }
};

class emu_timer {
public:
    std::function<void(s32)> cb;
    bool enabled = false;
    double remaining = 0;   // seconds
    s32 param = 0;
    void adjust(attotime t, s32 p = 0, attotime = attotime::never())
    {
        remaining = t.s;
        param = p;
        enabled = t.s < 1e29;
    }
    void enable(bool e = true) { enabled = e; }
};

class running_machine {
public:
    u32 rand() { seed_ = seed_ * 1103515245u + 12345u; return seed_ >> 8; }
    const char* describe_context() const { return "scsp"; }
private:
    u32 seed_ = 0x1234567;
};

class device_t {
public:
    using feature_type = u32;
    struct feature { static constexpr feature_type SOUND = 1; };

    device_t(const machine_config&, int, const char*, device_t*, u32 clock) : clock_(clock) {}
    virtual ~device_t() = default;

    u32 clock() const { return clock_; }
    running_machine& machine() { return machine_; }

    // the board: start / reset the device, run its timers by dt seconds
    void start() { device_start(); }
    void reset() { device_reset(); }
    void run_timers(double dt)
    {
        for (auto& t : timers_) {
            if (!t->enabled)
                continue;
            t->remaining -= dt;
            if (t->remaining <= 1e-12) {
                t->enabled = false;
                t->cb(t->param);
            }
        }
    }

protected:
    virtual void device_start() {}
    virtual void device_reset() {}
    virtual void device_post_load() {}
    virtual void device_clock_changed() {}

    template <typename T> emu_timer* timer_alloc(void (T::*fn)(s32), const char*, T* obj)
    {
        timers_.push_back(std::make_unique<emu_timer>());
        timers_.back()->cb = [obj, fn](s32 p) { (obj->*fn)(p); };
        return timers_.back().get();
    }
    template <typename T> void save_item(T&&, const char*, int = 0) {}

private:
    u32 clock_;
    running_machine machine_;
    std::vector<std::unique_ptr<emu_timer>> timers_;
};

// one block of output: the board asks for n samples, two channels, floats in -1..1
class sound_stream {
public:
    int samples() const { return n_; }
    float get(int, int) const { return 0.0f; }   // no external input (EXTS)
    void put_int_clamp(int ch, int s, s32 v, s32 range)
    {
        v = std::clamp(v, -range, range - 1);
        out_[size_t(s) * 2 + size_t(ch)] = float(v) / float(range);
    }
    void set_sample_rate(u32) {}
    void update() {}

    void begin(float* out, int n) { out_ = out; n_ = n; }

private:
    float* out_ = nullptr;
    int n_ = 0;
};

class device_sound_interface {
public:
    device_sound_interface(const machine_config&, device_t&) {}
    virtual ~device_sound_interface() = default;
    // the board: n stereo samples into out (interleaved), before the output gain (gain())
    void render(float* out, int n)
    {
        stream_.begin(out, n);
        sound_stream_update(stream_);
    }
    float gain(int ch) const { return gain_[ch & 1]; }

protected:
    sound_stream* stream_alloc(int, int, u32) { return &stream_; }
    void set_output_gain(int ch, double g) { if (ch >= 0 && ch < 2) gain_[ch] = float(g); }
    virtual void sound_stream_update(sound_stream& stream) = 0;

private:
    sound_stream stream_;
    float gain_[2] = {1.0f, 1.0f};
};

// write callbacks (the board sets them)
class devcb_write8 {
public:
    explicit devcb_write8(device_t&) {}
    // bind(): a handle (scsp.h returns it by value: auto irq_cb() { return m_irq_cb.bind(); })
    struct binder { devcb_write8* d; void set(std::function<void(offs_t, u8)> f) { d->f_ = std::move(f); } };
    binder bind() { return binder{this}; }
    void set(std::function<void(offs_t, u8)> f) { f_ = std::move(f); }
    bool isunset() const { return !f_; }
    void operator()(offs_t o, u8 d) { if (f_) f_(o, d); }
private:
    std::function<void(offs_t, u8)> f_;
};
class devcb_write_line {
public:
    explicit devcb_write_line(device_t&) {}
    struct binder { devcb_write_line* d; void set(std::function<void(int)> f) { d->f_ = std::move(f); } };
    binder bind() { return binder{this}; }
    void set(std::function<void(int)> f) { f_ = std::move(f); }
    bool isunset() const { return !f_; }
    void operator()(int s) { if (f_) f_(s); }
private:
    std::function<void(int)> f_;
};
