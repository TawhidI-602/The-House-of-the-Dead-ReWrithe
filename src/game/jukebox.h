// ReWrithe's Jukebox (ours, not in either original; the main menu's "Jukebox"): every song, sound effect and voice of
// the game from the player's own files, from either sound source:
//   Arcade: the arcade sound board (src/audio/sound_board.h, the ROM set's sound program and samples) gets the game's
//           own sound commands (0xA01000.. music, 0xA012xx.. effects, 0xA014xx.. voices: the arcade's sound list,
//           docs/runtime.md "Sound", src/game/cue_map.inc);
//   PC:     the 1998 PC release's music files (Music\TrackNN, the CD tracks; docs/formats.md section 4) and its cues
//           (THOTD.EXE's cue table: GAMESNDS.RBH samples, SOUND\O_*.RBH files; formats.md section 30).
// Three lists (Music, Sound Effects, Voices), Play / Stop / Next / Previous, Once / Loop one / Loop all / Shuffle, the
// time and a progress bar where the length is known (the PC's files and samples; the arcade's as measured from the ROMs:
// jukebox_measure.cpp). The front end draws the page
// (Frontend::screenJukebox); this class keeps the lists and what plays.
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace hotd::audio { class SoundBoard; }

namespace hotd::game {

class Audio;

struct JukeboxItem {
    std::string title;    // "Chapter 1", "Thunder", "Voice 02"
    std::string number;   // the right column: "Track 03", "0xA01001", "Cue 65"
    std::string info;     // the line under the bar: the file / code, the places
    int cue = -1;         // PC: the cue (effects, voices); arcade: the PC cue of the same sound (-1: none)
    int track = -1;       // PC music: the CD track
    uint32_t code = 0;    // arcade: the sound command
    bool loops = false;   // the sound goes round by itself (music that loops, looping samples)
    double length = 0;    // seconds (0: not known); arcade: measured (Jukebox::ArcadeLength), a looping one intro + loop
    double intro = 0, loopLength = 0;   // arcade, measured: a looping sound's intro and loop (loopLength 0: not known)
    std::string baseInfo; // arcade: info without the measured length
};

class Jukebox {
public:
    enum Category { kMusic, kEffects, kVoices, kCategories };
    enum SourceId { kArcade, kPc };
    enum Mode { kOnce, kLoopOne, kLoopAll, kShuffle, kModes };
    static constexpr int kPerPage = 10;

    // the lists from the PC cue table (audio must be init'ed with the PC files) and the music files found; arcade:
    // the arcade sound board can be started (startBoard: called when the arcade source plays; false: it cannot)
    void build(Audio& audio, bool arcadeThere);
    bool built() const { return audio_ != nullptr; }
    std::function<bool()> startBoard;

    const std::vector<JukeboxItem>& list(int src, int cat) const { return lists_[src][cat]; }
    const std::vector<JukeboxItem>& current() const { return lists_[source][category]; }
    bool hasSource(int s) const { return s == kArcade ? arcadeThere_ : true; }

    int source = kPc, category = kMusic;
    int selected = 0;   // the row chosen in the current list
    int mode = kLoopAll;
    int musicVolume = 100, sfxVolume = 100;   // the game's settings (0-100): the board's music level here

    void select(int i);                 // the chosen row (clamped)
    void play(int i);                   // that row of the current list
    void play() { play(selected); }
    void stop();
    void step(int dir);                 // the next / previous row, playing it when something plays
    void setCategory(int c);            // stops; the list's first row
    void setSource(int s);              // stops; the same row where the lists match
    void toggleSource() { setSource(source == kPc ? kArcade : kPc); }
    void nextMode() { mode = (mode + 1) % kModes; }
    void volumesChanged();              // musicVolume / sfxVolume changed: the board's level now
    void seek(double fraction);         // the PC's music only
    void update(double dt);             // the time, the end of a sound, the next one

    bool playing() const { return playing_ >= 0; }
    int playingRow() const { return playingCategory_ == category && playingSource_ == source ? playing_ : -1; }
    const JukeboxItem* nowItem() const;
    double elapsed() const { return elapsed_; }
    double length() const;             // 0: not known
    double position() const;           // the time in the sound (arcade: in the loop once it went round)
    int progress() const;              // 0-1000, -1: no bar
    std::string timeText() const;      // "1:23 / 3:45", "0:12"
    std::string nowText() const;       // "Now playing: Chapter 1 (Track 03)" / "Stopped"
    static const char* modeName(int m);
    static const char* categoryName(int c);
    static const char* sourceName(int s) { return s == kArcade ? "Arcade" : "PC"; }

    // the headless test (--jukebox-test): every item of every list of both sources played for a while, its RMS and the
    // loop points checked, WAVs of the music; the arcade lengths (from lengthsDir's cache) listed and checked on the
    // output (the ends, the loops' envelopes, a seek); returns the failures (0 = all good)
    static int test(Audio& audio, bool arcadeThere, const std::function<bool()>& startBoard, const std::string& outDir,
                    const std::string& lengthsDir);

    // ---- the arcade lengths (jukebox_measure.cpp): the board says nothing about its sounds, so each code is played
    // once offline (a board of its own, as fast as it runs, muted) and its output looked at: where it ends, or where
    // it repeats itself (intro + loop). Made on the player's PC from their ROMs into a cache file (settings folder).
    struct ArcadeLength {
        bool measured = false;
        double length = 0;            // seconds from the command: to the last sound; a looping one: intro + one loop
        double intro = 0, loop = 0;   // a looping one (loop 0: it goes on, no repeat found)
        bool loops = false;
        std::string text() const;     // "loops: intro 1.20 s + loop 95.40 s (1:35)", "6.95 s, once"
    };
    static const std::vector<std::pair<uint32_t, int>>& cueMap();         // the arcade code -> PC cue map
    static std::vector<std::pair<uint32_t, bool>> arcadeCodes();          // the codes measured (music first), music?
    static ArcadeLength measureArcade(audio::SoundBoard& board, uint32_t code, bool music, std::string* diag = nullptr);
    static std::string romKey(const std::vector<uint8_t>& prog, const std::vector<uint8_t>& samples);
    // the cache: kMeasureParts files (lengthsFile), each one process's share (every parts-th code, so the songs spread
    // over them); measureArcadeFile measures one share anew unless its file is complete for these ROMs (each: a code
    // measured); returns the codes measured, -1: no board
    static constexpr int kMeasureParts = 4;
    static std::string lengthsFile(const std::string& dir, int part);
    static int measureArcadeFile(const std::vector<uint8_t>& prog, const std::vector<uint8_t>& samples, const std::string& file,
                                 int part, int parts,
                                 const std::function<void(uint32_t, const ArcadeLength&, const std::string&)>& each = {});
    static std::map<uint32_t, ArcadeLength> readArcadeLengths(const std::string& file, std::string* key = nullptr,
                                                              bool* complete = nullptr);
    static std::map<uint32_t, ArcadeLength> readArcadeLengthsAll(const std::string& dir, bool* complete);
    // the lengths known (the menu: read from the cache as the measuring process writes it); measuring: not all yet
    void setArcadeLengths(const std::map<uint32_t, ArcadeLength>& lengths, bool measuring);
    bool measuring() const { return measuring_; }

private:
    void start(const JukeboxItem& it);
    void ended();                       // the sound ended (or its time is up): again, the next one or stop
    Audio* audio_ = nullptr;
    bool arcadeThere_ = false;
    std::vector<JukeboxItem> lists_[2][kCategories];
    int playing_ = -1, playingCategory_ = 0, playingSource_ = 0;
    double elapsed_ = 0;
    double quiet_ = 0;                  // arcade: seconds the board has been silent
    double boardWait_ = 0;              // arcade: the board's start-up after a reset, before the command goes in
    uint32_t pendingCode_ = 0;
    double songLength_ = 0;             // arcade music: the PC file's length of the same song (0: none)
    uint32_t rng_ = 0x2468ACE1u;
    std::string status_;                // why the last play did not start (shown instead of "Stopped")
    uint64_t cmdSample_ = 0;            // arcade: the board's clock when the command went in (the time is the board's)
    double seekTo_ = -1;                // arcade: the place a seek runs the board up to (muted), -1: none
    std::map<uint32_t, ArcadeLength> arcadeLengths_;
    bool measuring_ = false;
};

} // namespace hotd::game
