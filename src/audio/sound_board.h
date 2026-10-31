// HOTD's Model 2 sound board (68000 + SCSP YMF292, docs/teaser_status/sound_board.md), emulated from the player's
// arcade ROM set: Musashi's 68000 runs the board's program (epr-19720.31), MAME's SCSP plays the samples
// (mpr-19721.32 / mpr-19722.34), the game's sound commands go in as MIDI bytes. Built only with HOTD_ARCADE_SOUND
// (HOTD_HAVE_ARCADE_SOUND); one board per process (Musashi keeps one CPU).
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace hotd::audio {

class SoundBoard {
public:
    SoundBoard();
    ~SoundBoard();
    SoundBoard(const SoundBoard&) = delete;
    SoundBoard& operator=(const SoundBoard&) = delete;

    // program: epr-19720.31 as in the ROM set (512 KB, little-endian words); samples: mpr-19721.32 then mpr-19722.34
    // (8 MB, little-endian words); or both big-endian (MAME's region export; told apart by the reset PC). Resets the board.
    bool load(const std::vector<uint8_t>& program, const std::vector<uint8_t>& samples, std::string& err);
    void reset();
    // a game sound command (0xA0xxyy / 0xBExxyy): 3 bytes, high first, into the SCSP's MIDI input at the MIDI rate
    void command(uint32_t code);
    // frames stereo samples at 44.1 kHz, added to out (interleaved) times gain; runs the board for that long
    void render(float* out, int frames, float gain);
    bool loaded() const;
    uint64_t samplesRun() const;

    struct Impl;   // sound_board.cpp (public for Musashi's C callbacks there)

private:
    std::unique_ptr<Impl> impl_;
};

} // namespace hotd::audio
