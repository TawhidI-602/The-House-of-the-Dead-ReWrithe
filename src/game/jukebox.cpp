// ReWrithe's Jukebox (jukebox.h). The names and places come from docs/formats.md section 4 (the music cue table, the
// Saturn / PC track mapping) and the PC's sound file names; the sound itself is the game's (its sound board commands,
// its PC cues and music files).
#include "game/jukebox.h"

#include "game/audio.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <map>

namespace hotd::game {

namespace {

const std::pair<uint32_t, int> kCueMap[] = {
#include "game/cue_map.inc"
};

// the music cues 8..22 (THOTD.EXE's cue table; the arcade's 0xA01000 + n = cue 8 + n): the internal name, where the
// game plays it, and whether it loops (formats.md section 4; the arcade-only cues have no PC track)
struct Song { const char* name; const char* place; bool loops; };
const Song kSongs[15] = {
    {"MUSIC_ADV1", "Title and attract", true},
    {"MUSIC_STG1", "Chapter 1", true},
    {"MUSIC_STG2", "Chapter 2", true},
    {"MUSIC_OVER", "Game over", false},
    {"MUSIC_BOSS1", "Boss: Chariot, Hanged Man, Hermit", true},
    {"MUSIC_STGCLEAR", "Chapter clear", false},
    {"MUSIC_ENDING", "Ending and credits", true},
    {"MUSIC_STG3", "Chapter 3 (first cue; not on the PC)", true},
    {"MUSIC_STG3_2", "Chapter 3", true},
    {"MUSIC_STG4", "Chapter 4 (first cue)", true},
    {"MUSIC_STG4_2", "Chapter 4, second cue (not on the PC)", true},
    {"MUSIC_STG4BOSS", "Boss: Magician", true},
    {"MUSIC_STG4BOSS_V2", "Boss: Magician, second cue (not on the PC)", true},
    {"MUSIC_NAMEENTRY", "Name entry, High Scores", true},
    {"MUSIC_STG4BOSS_3", "Chapter 4, Menu", true},
};
// PC CD track -> the Saturn disc's track (formats.md section 4, the user's Music\README.txt)
int saturnOf(int track)
{
    static const int k[14] = {0, 0, 2, 3, 4, 10, 7, 9, 11, 5, 8, 12, 6, 6};
    return track >= 0 && track < 14 ? k[track] : 0;
}

// the PC's O_ file names (romaji / English abbreviations) as words; the others keep their file name
const char* friendlyName(const std::string& f)
{
    static const std::map<std::string, const char*> k = {
        {"BREAK5", "Break"}, {"WOODBR", "Wood breaking"}, {"WATER_", "Water"}, {"SHOUT3", "Shout"},
        {"GLASSW", "Glass"}, {"GLASS2", "Glass 2"}, {"THUNDE", "Thunder"}, {"THUND2", "Thunder 2"},
        {"ZISIN", "Earthquake"}, {"FROG2", "Frog"}, {"SUICH2", "Switch"}, {"KEMON5", "Beast"},
        {"ENEMY1", "Enemy 1"}, {"ENE2_2", "Enemy 2"}, {"ENE7_2", "Enemy 7"}, {"ENEB11", "Enemy B 11"},
        {"ENEMB4", "Enemy B 4"}, {"ENEMB7", "Enemy B 7"}, {"ENEMB9", "Enemy B 9"}, {"HEL1_2", "Help!"},
        {"NO_F3", "No!"}, {"OHNO", "Oh no!"}, {"KYAAA", "Scream"}, {"HELP2", "Help! 2"}, {"NONONO", "No no no!"},
        {"OHNO_2", "Oh no! 2"}, {"THAN_2", "Thank you"}, {"AAAH", "Aaah!"}, {"HELP", "Help!"}, {"NO_2", "No! 2"},
        {"UAAH_2", "Uaah!"}, {"AOH2", "Aoh!"}, {"KILLM2", "Kill me"}, {"LIFEUP", "Life up"}, {"START4", "Start"},
        {"INU3", "Dog 1"}, {"INU4", "Dog 2"}, {"INU5", "Dog 3"}, {"CARDOO", "Car door"}, {"ELEV_2", "Elevator"},
        {"ELEVA2", "Elevator 2"}, {"KAEN", "Flame"}, {"TELEPH", "Telephone"}, {"BAKUH2", "Explosion"},
        {"KIA_1P", "Player scream"}, {"ST4BO1", "Chapter 4 boss"}, {"GUCYA3", "Squelch"},
    };
    auto it = k.find(f);
    return it == k.end() ? nullptr : it->second;
}

// the category of a PC cue (and the arcade code of the same place in the list): the voices are the VOC files (cues
// 158..240) and the people's cries (112..126); the rest after the music are effects
int categoryOf(int cue)
{
    if (cue >= 8 && cue <= 22)
        return Jukebox::kMusic;
    if ((cue >= 112 && cue <= 126) || (cue >= 158 && cue <= 240))
        return Jukebox::kVoices;
    return Jukebox::kEffects;
}

std::string hex(uint32_t code)
{
    char b[16];
    std::snprintf(b, sizeof b, "0x%06X", code);
    return b;
}

std::string clock(double s)
{
    const int t = int(std::max(0.0, s));
    char b[16];
    std::snprintf(b, sizeof b, "%d:%02d", t / 60, t % 60);
    return b;
}

std::string seconds(double s)
{
    char b[24];
    std::snprintf(b, sizeof b, "%.2f s", s);
    return b;
}

constexpr double kBoardBoot = 0.25;   // after a reset the board's program starts before it reads commands (tested)
constexpr float kQuiet = 1.0e-4f;     // the board's peak below this: silence

} // namespace

const std::vector<std::pair<uint32_t, int>>& Jukebox::cueMap()
{
    static const std::vector<std::pair<uint32_t, int>> k(std::begin(kCueMap), std::end(kCueMap));
    return k;
}

const char* Jukebox::modeName(int m)
{
    static const char* const k[kModes] = {"Play once", "Loop one", "Loop all", "Shuffle"};
    return m >= 0 && m < kModes ? k[m] : "?";
}

const char* Jukebox::categoryName(int c)
{
    static const char* const k[kCategories] = {"Music", "Sound Effects", "Voices"};
    return c >= 0 && c < kCategories ? k[c] : "?";
}

void Jukebox::build(Audio& audio, bool arcadeThere)
{
    audio_ = &audio;
    arcadeThere_ = arcadeThere;
    for (auto& s : lists_)
        for (auto& l : s)
            l.clear();
    std::map<int, uint32_t> codeOf;   // PC cue -> arcade code
    for (const auto& [code, cue] : kCueMap)
        if (code >= 0xA01000 && code <= 0xA014FF)
            codeOf.emplace(cue, code);
    auto effectTitle = [&](int cue) -> std::string {
        const int t = audio.cueType(cue);
        if (t == 4) {
            const std::string f = audio.cueFile(cue);
            const char* n = friendlyName(f);
            if (f.rfind("VOC", 0) == 0)
                return "Voice " + f.substr(3);
            return n ? n : f;
        }
        if (t == 1)
            return (audio.cueValue(cue) & 0xFFFF) == 13 ? std::string("Reload") : "Sample " + std::to_string(audio.cueValue(cue) & 0xFFFF);
        return "Arcade only";
    };
    auto pcSource = [&](int cue) -> std::string {
        const int t = audio.cueType(cue);
        if (t == 4)
            return "SOUND\\O_" + audio.cueFile(cue) + ".RBH";
        if (t == 1)
            return "GAMESNDS.RBH sample " + std::to_string(audio.cueValue(cue) & 0xFFFF);
        return "no PC sound";
    };

    // PC music: the CD tracks 2..12 (13: patch 7's copy, when its file is there), named by the cue that plays them
    for (int track = 2; track <= 13; ++track) {
        int cue = -1;
        for (int c = 8; c <= 22; ++c)
            if (audio.cueType(c) == 3 && audio.cueValue(c) == track) { cue = c; break; }
        if (track == 13 && !audio.hasMusicTrack(13))
            continue;
        JukeboxItem it;
        const Song* s = cue >= 0 ? &kSongs[cue - 8] : nullptr;
        it.title = track == 13 ? "Chapter 4 (patch 7's copy)" : s ? s->place : "?";
        char num[16];
        std::snprintf(num, sizeof num, "Track %02d", track);
        it.number = num;
        it.track = track;
        it.cue = cue;
        it.loops = cue != 11 && cue != 13;
        char inf[160];
        std::snprintf(inf, sizeof inf, "Music\\Track%02d, Saturn disc %02d%s%s%s, %s", track, saturnOf(track),
                      s ? ", cue " : "", s ? std::to_string(cue).c_str() : "", s ? (std::string(" ") + s->name).c_str() : "",
                      it.loops ? "loops" : "plays once");
        it.info = inf;
        if (!audio.hasMusicTrack(track))
            it.info = "No file for Track" + std::string(num + 6) + ": see README, the music files";
        lists_[kPc][kMusic].push_back(it);
    }
    // PC effects and voices: the cues that play a sample (types 1 and 4; not the stops)
    for (int cue = 23; cue < 269; ++cue) {
        const int t = audio.cueType(cue);
        if (t != 1 && t != 4)
            continue;
        JukeboxItem it;
        it.cue = cue;
        it.title = effectTitle(cue);
        it.number = "Cue " + std::to_string(cue);
        it.loops = audio.cueLoops(cue);
        it.length = audio.cueLength(cue);
        it.info = pcSource(cue) + ", " + seconds(it.length) + (it.loops ? ", loops" : "");
        if (codeOf.count(cue))
            it.info += ", arcade " + hex(codeOf[cue]);
        lists_[kPc][categoryOf(cue)].push_back(it);
    }
    // arcade: the arcade's sound list in its order (0xA01000.., the codes the cue map pairs with PC cues; the stops
    // and commands left out)
    std::vector<std::pair<uint32_t, int>> codes(std::begin(kCueMap), std::end(kCueMap));
    std::sort(codes.begin(), codes.end());
    for (const auto& [code, cue] : codes) {
        if (code < 0xA01000 || code > 0xA014FF)
            continue;
        const int t = audio.cueType(cue);
        if (t == 2 || t == 5)
            continue;
        JukeboxItem it;
        it.code = code;
        it.cue = t == 0 ? -1 : cue;
        it.number = hex(code);
        const int cat = categoryOf(cue);
        if (cat == kMusic) {
            const Song& s = kSongs[cue - 8];
            it.title = s.place;
            it.loops = s.loops;
            it.info = std::string("Sound command ") + hex(code) + ", " + s.name;
            if (t == 3 && audio.cueValue(cue) > 0) {
                char b[48];
                std::snprintf(b, sizeof b, ", the PC's Track%02d", audio.cueValue(cue));
                it.info += b;
            } else
                it.info += ", not on the PC";
            it.track = t == 3 ? audio.cueValue(cue) : -1;
        } else {
            it.title = t == 0 ? "Arcade only" : effectTitle(cue);
            it.loops = audio.cueLoops(cue);
            it.info = "Sound command " + hex(code) + (t == 0 ? std::string(", no PC sound") : ", the PC's cue " + std::to_string(cue));
        }
        it.baseInfo = it.info;
        lists_[kArcade][cat].push_back(it);
    }
    setArcadeLengths(arcadeLengths_, measuring_);
    if (!arcadeThere_)
        source = kPc;
    selected = std::clamp(selected, 0, std::max(0, int(current().size()) - 1));
}

void Jukebox::select(int i)
{
    const int n = int(current().size());
    selected = n ? std::clamp(i, 0, n - 1) : 0;
}

const JukeboxItem* Jukebox::nowItem() const
{
    if (playing_ < 0)
        return nullptr;
    const auto& l = lists_[playingSource_][playingCategory_];
    return playing_ < int(l.size()) ? &l[size_t(playing_)] : nullptr;
}

void Jukebox::play(int i)
{
    if (!audio_)
        return;
    select(i);
    if (current().empty())
        return;
    stop();
    status_.clear();
    playing_ = selected;
    playingCategory_ = category;
    playingSource_ = source;
    start(current()[size_t(selected)]);
}

void Jukebox::start(const JukeboxItem& it)
{
    Audio& a = *audio_;
    elapsed_ = 0;
    quiet_ = 0;
    pendingCode_ = 0;
    seekTo_ = -1;
    a.stopPcSound();
    if (playingSource_ == kPc) {
        if (a.boardOn())
            a.resetBoard();
        if (it.track > 0) {
            if (!a.playMusicTrack(it.track, mode == kLoopOne)) {
                playing_ = -1;
                status_ = "No music file for " + it.number + " (see README: the music files)";
            }
        } else {
            a.pcCue(it.cue);
        }
        return;
    }
    if (!a.boardOn() && !(startBoard && startBoard())) {
        playing_ = -1;
        status_ = "The arcade sound board could not start (the sound ROMs: see the log)";
        return;
    }
    // the PC file's length of the same song (Loop all moves on after it); read here, while nothing sounds (it reads the file)
    songLength_ = playingCategory_ == kMusic && it.track > 0 ? a.trackLength(it.track) : 0;
    a.resetBoard();   // nothing else sounds; the command goes in once the board's program runs
    a.setBoardVolume((playingCategory_ == kMusic ? musicVolume : sfxVolume) / 100.0f);
    pendingCode_ = it.code;
    boardWait_ = kBoardBoot;
}

void Jukebox::stop()
{
    if (!audio_)
        return;
    if (playing_ >= 0) {
        audio_->stopPcSound();
        if (audio_->boardOn())
            audio_->resetBoard();
    }
    audio_->setBoardVolume(-1);
    playing_ = -1;
    pendingCode_ = 0;
    seekTo_ = -1;
    elapsed_ = 0;
}

void Jukebox::volumesChanged()
{
    if (audio_ && playing_ >= 0 && playingSource_ == kArcade && seekTo_ < 0)
        audio_->setBoardVolume((playingCategory_ == kMusic ? musicVolume : sfxVolume) / 100.0f);
}

void Jukebox::step(int dir)
{
    const int n = int(current().size());
    if (!n)
        return;
    const bool was = playing();
    select((selected + dir + n) % n);
    if (was)
        play(selected);
}

void Jukebox::setArcadeLengths(const std::map<uint32_t, ArcadeLength>& lengths, bool measuring)
{
    if (&lengths != &arcadeLengths_)
        arcadeLengths_ = lengths;
    measuring_ = measuring;
    for (auto& l : lists_[kArcade])
        for (JukeboxItem& it : l) {
            auto f = arcadeLengths_.find(it.code);
            it.info = it.baseInfo;
            it.length = it.intro = it.loopLength = 0;
            if (f == arcadeLengths_.end()) {
                if (measuring_)
                    it.info += ", length: being measured";
                continue;
            }
            const ArcadeLength& a = f->second;
            if (a.loops && a.loop > 0 && a.loop < 2.0) {   // a held sound (a hum, a ring): no bar, it plays on
                it.loops = true;
                it.loopLength = a.loop;
                it.info += ", loops (every " + seconds(a.loop) + ")";
            } else if (a.loops && a.loop > 0) {   // a song that goes round: one round is its length
                it.loops = true;
                it.intro = a.intro;
                it.loopLength = a.loop;
                it.length = a.intro + a.loop;
                it.info += ", " + clock(it.length) + ", loops (intro " + seconds(a.intro) + ", loop " + clock(a.loop) + ")";
            } else if (a.loops) {
                it.loops = true;
                it.info += ", goes on";
            } else if (a.length > 0) {
                it.loops = false;
                it.length = a.length;
                it.info += ", " + (a.length >= 10 ? clock(a.length) : seconds(a.length));
            }
        }
}

void Jukebox::setCategory(int c)
{
    if (c == category)
        return;
    category = (c + kCategories) % kCategories;
    selected = 0;
}

void Jukebox::setSource(int s)
{
    if (s == source || !hasSource(s))
        return;
    stop();
    // the same row where both lists have it: music by its song, effects by the PC cue
    const JukeboxItem* was = selected < int(current().size()) ? &current()[size_t(selected)] : nullptr;
    const int cue = was ? was->cue : -1, track = was ? was->track : -1;
    source = s;
    int pick = 0;
    for (int i = 0; i < int(current().size()); ++i) {
        const JukeboxItem& it = current()[size_t(i)];
        if ((category == kMusic && track > 0 && it.track == track) || (category != kMusic && cue >= 0 && it.cue == cue)) {
            pick = i;
            break;
        }
    }
    select(pick);
}

void Jukebox::seek(double fraction)
{
    if (!audio_ || playing_ < 0)
        return;
    const JukeboxItem* it = nowItem();
    if (playingSource_ == kArcade) {
        // the board cannot jump: the code again from a reset, then the board run muted up to the place (update)
        if (!it || it->length <= 0)
            return;
        const double to = std::clamp(fraction, 0.0, 1.0) * it->length;
        start(*it);
        if (playing_ >= 0 && to > 0.05) {
            seekTo_ = to;
            audio_->setBoardVolume(0);
        }
        return;
    }
    double pos, len;
    int loops;
    if (it && it->track > 0 && audio_->musicTime(pos, len, loops) && len > 0)
        audio_->seekMusic(std::clamp(fraction, 0.0, 1.0) * len);
}

double Jukebox::length() const
{
    const JukeboxItem* it = nowItem();
    if (!it || !audio_)
        return 0;
    if (playingSource_ == kPc && it->track > 0) {
        double pos, len;
        int loops;
        return audio_->musicTime(pos, len, loops) ? len : 0;
    }
    return it->length;   // (arcade: the measured length, one round of a looping one; 0 until measured)
}

double Jukebox::position() const
{
    const JukeboxItem* it = nowItem();
    if (!it)
        return 0;
    double pos = seekTo_ >= 0 ? seekTo_ : elapsed_;
    if (playingSource_ == kArcade && it->loopLength > 0 && it->length > 0 && pos > it->length)   // went round: in the loop
        pos = it->intro + std::fmod(pos - it->intro, it->loopLength);
    return pos;
}

int Jukebox::progress() const
{
    const JukeboxItem* it = nowItem();
    if (!it || !audio_)
        return -1;
    double pos = position(), len = length();
    if (playingSource_ == kPc && it->track > 0) {
        int loops;
        audio_->musicTime(pos, len, loops);
    }
    if (len <= 0)
        return -1;
    if (it->loops && playingSource_ == kPc && it->track <= 0)
        pos = std::fmod(pos, len);
    return std::clamp(int(pos / len * 1000.0), 0, 1000);
}

std::string Jukebox::timeText() const
{
    const JukeboxItem* it = nowItem();
    if (!it || !audio_)
        return "";
    double pos = position(), len = length();
    if (playingSource_ == kPc && it->track > 0) {
        int loops;
        audio_->musicTime(pos, len, loops);
    }
    if (len >= 1.0)
        return clock(pos) + " / " + clock(len);
    if (len > 0) {
        char b[48];
        std::snprintf(b, sizeof b, "%.1f / %.1f s", std::min(pos, len), len);
        return b;
    }
    return clock(pos);
}

std::string Jukebox::nowText() const
{
    const JukeboxItem* it = nowItem();
    if (!it)
        return status_.empty() ? "Stopped" : status_;
    std::string s = std::string(seekTo_ >= 0 ? "Seeking: " : pendingCode_ ? "Starting: " : "Playing: ") + it->title + " (" +
                    it->number + ")";
    if (playingSource_ == kArcade && it->loopLength > 0)
        s += ", loops " + (it->loopLength >= 10 ? clock(it->loopLength + 0.5) : seconds(it->loopLength));   // (measured)
    return s;
}

void Jukebox::update(double dt)
{
    if (!audio_ || playing_ < 0)
        return;
    Audio& a = *audio_;
    const JukeboxItem* it = nowItem();
    if (!it) {
        stop();
        return;
    }
    if (pendingCode_) {   // arcade: the board's start-up after the reset, then the game's command
        boardWait_ -= dt;
        if (boardWait_ > 0)
            return;
        a.cue(pendingCode_);
        pendingCode_ = 0;
        elapsed_ = 0;
        cmdSample_ = a.boardSamples();
        return;
    }
    if (playingSource_ == kArcade) {
        // the board's own clock (the time it has run since the command; a seek's fast run too)
        elapsed_ = double(a.boardSamples() - cmdSample_) / 44100.0;
        if (seekTo_ >= 0) {   // a seek: the board run muted up to the place, at most ~25 ms of this frame at a time
            const auto t0 = std::chrono::steady_clock::now();
            while (elapsed_ < seekTo_ && std::chrono::steady_clock::now() - t0 < std::chrono::milliseconds(25)) {
                a.boardSkip(std::min(4410, int((seekTo_ - elapsed_) * 44100.0) + 1));
                elapsed_ = double(a.boardSamples() - cmdSample_) / 44100.0;
            }
            if (elapsed_ < seekTo_)
                return;
            seekTo_ = -1;
            quiet_ = 0;
            a.setBoardVolume((playingCategory_ == kMusic ? musicVolume : sfxVolume) / 100.0f);
            return;
        }
    } else
        elapsed_ += dt;
    bool done = false;
    if (playingSource_ == kPc) {
        if (it->track > 0)
            done = !a.musicPlaying();
        else {
            done = elapsed_ > 0.05 && !a.cueSounding(it->cue);
            if (it->loops && mode != kLoopOne && elapsed_ >= std::max(4.0, it->length * 2))
                done = true;   // a looping sample once round (twice for the short ones) in the other modes
        }
    } else {
        quiet_ = a.boardPeak() < kQuiet ? quiet_ + dt : 0;
        done = elapsed_ > 1.0 && quiet_ > 1.5;   // (the board says nothing about its sounds: its output)
        if (!done && mode != kLoopOne && it->loops) {
            // a looping song / sound: once round. Measured (jukebox_measure.cpp): intro + loop; else the PC's file of
            // the same song, else 4 min (a sound 4 s)
            double cap = playingCategory_ == kMusic ? 240.0 : 4.0;
            if (it->loopLength > 0)
                cap = std::max(it->length, playingCategory_ == kMusic ? 1.0 : 4.0);
            else if (songLength_ > 1.0)
                cap = songLength_ + 0.5;
            done = elapsed_ >= cap;
        }
    }
    if (done)
        ended();
}

void Jukebox::ended()
{
    const auto& l = lists_[playingSource_][playingCategory_];
    const int n = int(l.size());
    int nextRow = -1;
    if (mode == kLoopOne)
        nextRow = playing_;
    else if (mode == kLoopAll)
        nextRow = (playing_ + 1) % std::max(n, 1);
    else if (mode == kShuffle && n > 0) {
        rng_ = rng_ * 1103515245u + 12345u;
        nextRow = n > 1 ? (playing_ + 1 + int((rng_ >> 8) % uint32_t(n - 1))) % n : 0;
    }
    if (nextRow < 0 || n == 0) {
        stop();
        return;
    }
    if (playingCategory_ == category && playingSource_ == source)
        selected = nextRow;   // the list follows what plays
    playing_ = nextRow;
    start(l[size_t(nextRow)]);
}

// ---- the headless test ----
namespace {

struct Wav {
    std::FILE* f = nullptr;
    uint64_t frames = 0;
    bool open(const std::string& p)
    {
        f = std::fopen(p.c_str(), "wb");
        uint8_t h[44] = {};
        if (f) std::fwrite(h, 1, 44, f);
        return f != nullptr;
    }
    void put(const std::vector<float>& s)
    {
        if (!f) return;
        std::vector<int16_t> pcm(s.size());
        for (size_t i = 0; i < s.size(); ++i)
            pcm[i] = int16_t(std::lround(std::clamp(s[i], -1.0f, 1.0f) * 32767.0f));
        std::fwrite(pcm.data(), 2, pcm.size(), f);
        frames += s.size() / 2;
    }
    void close()
    {
        if (!f) return;
        auto p32 = [](uint8_t* p, uint32_t v) { for (int i = 0; i < 4; ++i) p[i] = uint8_t(v >> (8 * i)); };
        const uint32_t data = uint32_t(frames * 4);
        uint8_t h[44] = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 16, 0, 0, 0, 1, 0, 2, 0};
        p32(h + 4, 36 + data);
        p32(h + 24, 44100);
        p32(h + 28, 44100 * 4);
        h[32] = 4;
        h[34] = 16;
        std::memcpy(h + 36, "data", 4);
        p32(h + 40, data);
        std::fseek(f, 0, SEEK_SET);
        std::fwrite(h, 1, 44, f);
        std::fclose(f);
        f = nullptr;
    }
};

double rms(const std::vector<float>& s, size_t from = 0, size_t to = SIZE_MAX)
{
    to = std::min(to, s.size());
    if (from >= to) return 0;
    double sum = 0;
    for (size_t i = from; i < to; ++i) sum += double(s[i]) * s[i];
    return std::sqrt(sum / double(to - from));
}

} // namespace

int Jukebox::test(Audio& audio, bool arcadeThere, const std::function<bool()>& startBoard, const std::string& outDir,
                  const std::string& lengthsDir)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(outDir, ec);
    std::FILE* rep = std::fopen((fs::path(outDir) / "jukebox_test.txt").string().c_str(), "w");
    auto say = [&](const char* fmt, auto... args) {
        char b[512];
        std::snprintf(b, sizeof b, fmt, args...);
        std::printf("%s\n", b);
        if (rep) std::fprintf(rep, "%s\n", b);
    };
    Jukebox jb;
    jb.startBoard = startBoard;
    jb.build(audio, arcadeThere);
    bool lengthsComplete = false;
    if (arcadeThere)   // the arcade lengths (jukebox_measure.cpp) as the menu has them
        jb.setArcadeLengths(readArcadeLengthsAll(lengthsDir, &lengthsComplete), false);
    jb.mode = kLoopOne;   // (the test moves on itself)
    constexpr int kBlock = 735;   // 1/60 s
    std::vector<float> block;
    // dt-stepped: the jukebox's clock and the mix advance together, as the menu's frames and the audio thread do
    auto run = [&](double secs, std::vector<float>& out) {
        out.clear();
        for (int b = 0; b < int(secs * 60); ++b) {
            jb.update(1.0 / 60.0);
            audio.renderTo(block, kBlock);
            out.insert(out.end(), block.begin(), block.end());
        }
    };
    int failures = 0, tested = 0;
    std::vector<float> s;
    for (int src : {kPc, kArcade}) {
        if (src == kArcade && !arcadeThere) {
            say("arcade: no sound board (ROMs) - skipped");
            continue;
        }
        jb.source = src;
        for (int cat = 0; cat < kCategories; ++cat) {
            jb.category = cat;
            const auto& l = jb.current();
            Wav all;
            if (cat != kMusic)
                all.open((fs::path(outDir) / (std::string(sourceName(src)) + "_" + (cat == kEffects ? "effects" : "voices") + ".wav")).string());
            int ok = 0;
            for (int i = 0; i < int(l.size()); ++i) {
                const JukeboxItem& it = l[size_t(i)];
                const double secs = cat == kMusic ? (src == kArcade ? 8.0 : 6.0)
                                    : src == kArcade ? 2.5 : std::clamp(it.length + 0.3, 0.6, 4.0);
                jb.play(i);
                run(secs, s);
                const double r = rms(s);
                const bool sounded = r > 1e-5;
                ok += sounded;
                ++tested;
                if (!sounded) ++failures;
                if (cat == kMusic) {
                    Wav w;
                    char name[64];
                    std::snprintf(name, sizeof name, "%s_music_%02d.wav", sourceName(src), i + 1);
                    if (w.open((fs::path(outDir) / name).string())) { w.put(s); w.close(); }
                } else
                    all.put(s);
                say("%-6s %-13s %3d %-10s %-40s rms %.5f %s", sourceName(src), categoryName(cat), i + 1, it.number.c_str(),
                    it.title.c_str(), r, sounded ? "ok" : "SILENT");
                jb.stop();
                run(0.1, s);   // (the tails)
            }
            all.close();
            say("== %s %s: %d / %d sounded", sourceName(src), categoryName(cat), ok, int(l.size()));
        }
    }
    // the loop points. PC music: the last 2 s of each looping track, then 2 s more: it must go round (to its start,
    // sound on both sides); the jingles must end. PC samples that loop: still sounding after their length
    jb.source = kPc;
    jb.category = kMusic;
    for (int i = 0; i < int(jb.current().size()); ++i) {
        const JukeboxItem& it = jb.current()[size_t(i)];
        if (!audio.hasMusicTrack(it.track))
            continue;
        jb.mode = it.loops ? kLoopOne : kOnce;
        jb.play(i);
        double pos, len, len0;
        int loops;
        audio.musicTime(pos, len0, loops);
        if (it.loops) {
            audio.seekMusic(len0 - 2.0);
            run(4.0, s);
        } else {
            run(len0 + 2.0, s);   // a jingle: the whole of it, then 2 s
        }
        const bool playing = audio.musicTime(pos, len, loops);
        len = len0;
        const size_t half = it.loops ? s.size() / 2 : size_t(len0 * 44100) * 2;
        // (the files' own ends: some fade to silence before the end; that is the recording, so only the wrap and the
        // sound after it count for a song, and for a jingle that it sounded and then ended)
        const double before = rms(s, 0, half), after = rms(s, half);
        bool good;
        if (it.loops)
            good = playing && loops == 1 && std::fabs(pos - 2.0) < 0.1 && after > 1e-5;
        else
            good = !playing && before > 1e-5 && after < 1e-6;
        if (!good) ++failures;
        say("loop   PC Track %02d %-34s len %7.2f s: %s (then: %s, pos %.2f s, rms before %.4f, after %.4f) %s",
            it.track, it.title.c_str(), len, it.loops ? "loops" : "ends", playing ? "playing" : "ended", pos, before, after,
            good ? "ok" : "BAD");
        jb.stop();
    }
    for (int cat : {kEffects, kVoices}) {
        jb.category = cat;
        jb.mode = kLoopOne;
        for (int i = 0; i < int(jb.current().size()); ++i) {
            const JukeboxItem& it = jb.current()[size_t(i)];
            if (!it.loops || it.length <= 0)
                continue;
            jb.play(i);
            run(it.length * 1.5 + 0.2, s);
            const double tail = rms(s, size_t(s.size() * 0.75));
            const bool good = audio.cueSounding(it.cue) && tail > 1e-5;
            if (!good) ++failures;
            say("loop   PC %-8s %-30s len %.2f s: %s after 1.5 x its length (rms %.4f) %s", it.number.c_str(), it.title.c_str(),
                it.length, audio.cueSounding(it.cue) ? "still sounding" : "ended", tail, good ? "ok" : "BAD");
            jb.stop();
        }
    }
    // arcade: the measured lengths (jukebox_measure.cpp), checked on the Jukebox's own output as an ear would: the
    // sounds that end must go quiet at their length (the last 10 ms above the quiet level within 0.3 s of it); the
    // looping songs' RMS envelope (100 ms steps) one loop later must follow the first round's (correlation > 0.9)
    // and not half a loop later; a seek (the board run muted up to the place) must land there and sound like it
    if (arcadeThere) {
        say("arcade lengths: %d codes measured%s", int(jb.arcadeLengths_.size()), lengthsComplete ? "" : " - NOT COMPLETE");
        if (!lengthsComplete) ++failures;
        auto startArcade = [&](int i) {   // play, then the board's start-up: the recording starts at the command
            jb.play(i);
            while (jb.pendingCode_) {
                jb.update(1.0 / 60.0);
                if (!jb.pendingCode_)
                    break;
                audio.renderTo(block, kBlock);
            }
        };
        auto lastSoundOf = [&](const std::vector<float>& v) {   // the end of the last 10 ms above the quiet level
            double last = 0;
            for (size_t b = 0; b + 882 <= v.size(); b += 882)
                if (rms(v, b, b + 882) > 2e-4) last = double(b / 2 + 441) / 44100.0;
            return last;
        };
        auto envelope = [&](const std::vector<float>& v, double from, double to) {
            std::vector<double> e;
            for (double t = from; t + 0.1 <= to; t += 0.1)
                e.push_back(rms(v, size_t(t * 44100) * 2, size_t((t + 0.1) * 44100) * 2));
            return e;
        };
        auto corr = [](const std::vector<double>& a, const std::vector<double>& b) {
            const size_t n = std::min(a.size(), b.size());
            if (n < 2) return 0.0;
            double ma = 0, mb = 0;
            for (size_t i = 0; i < n; ++i) { ma += a[i]; mb += b[i]; }
            ma /= double(n);
            mb /= double(n);
            double ab = 0, aa = 0, bb = 0;
            for (size_t i = 0; i < n; ++i) {
                ab += (a[i] - ma) * (b[i] - mb);
                aa += (a[i] - ma) * (a[i] - ma);
                bb += (b[i] - mb) * (b[i] - mb);
            }
            return aa > 0 && bb > 0 ? ab / std::sqrt(aa * bb) : 0.0;
        };
        jb.source = kArcade;
        int checked = 0, bad = 0;
        for (int cat = 0; cat < kCategories; ++cat) {
            jb.category = cat;
            for (int i = 0; i < int(jb.current().size()); ++i) {
                const JukeboxItem& it = jb.current()[size_t(i)];
                say("length Arcade %-13s %s %-34s %s", categoryName(cat), it.number.c_str(), it.title.c_str(),
                    jb.arcadeLengths_.count(it.code) ? jb.arcadeLengths_[it.code].text().c_str() : "not measured");
                // HOTD_JUKEBOX_EXPORT=<folder>: every arcade song recorded whole (a looping one: intro + two rounds), for
                // the player's own videos (their own ROMs; nothing is shipped)
                if (const char* exp = std::getenv("HOTD_JUKEBOX_EXPORT"); exp && cat == kMusic && it.length > 0) {
                    jb.mode = it.loops ? kLoopOne : kOnce;
                    startArcade(i);
                    run(it.loops && it.loopLength > 0 ? it.intro + 2 * it.loopLength : it.length + 1.0, s);
                    std::string name = it.title;
                    for (char& c : name)
                        if (std::strchr("\\/:*?\"<>|", c)) c = '-';
                    char file[160];
                    std::snprintf(file, sizeof file, "Arcade %02d - %s.wav", i + 1, name.c_str());
                    Wav wv;
                    if (wv.open((fs::path(exp) / file).string())) { wv.put(s); wv.close(); }
                }
                if (it.loops && it.loopLength > 0 && cat == kMusic) {
                    jb.mode = kLoopOne;
                    startArcade(i);
                    // (the lag whose envelope fits best, looked for from half a loop to one and a half, from the middle of
                    // the first round (the first bars after an intro can differ a little the first time): it must be
                    // the measured loop)
                    const double w = std::min(20.0, it.loopLength / 2), L = it.loopLength, a = it.intro + L / 2;
                    run(a + 1.5 * L + w + 0.5, s);
                    // (in 10 ms steps: the 100 ms envelope from 10 ms sums of squares)
                    std::vector<double> e10(s.size() / 882);
                    for (size_t j = 0; j < e10.size(); ++j)
                        for (size_t q = j * 882; q < j * 882 + 882; ++q)
                            e10[j] += double(s[q]) * s[q];
                    auto env10 = [&](double from) {
                        std::vector<double> e;
                        for (size_t j = size_t(std::lround(from * 100)); j + 10 <= e10.size() && e.size() < size_t(w * 10); j += 10) {
                            double sum = 0;
                            for (size_t q = j; q < j + 10; ++q)
                                sum += e10[q];
                            e.push_back(std::sqrt(sum / 8820));
                        }
                        return e;
                    };
                    const auto first = env10(a);
                    double bestLag = 0, bestC = -2;
                    for (double lag = std::round(L * 50) / 100; lag <= 1.5 * L; lag += 0.01)
                        if (const double c = corr(first, env10(a + lag)); c > bestC) {
                            bestC = c;
                            bestLag = lag;
                        }
                    const double c = corr(first, env10(a + L));
                    const bool good = std::fabs(bestLag - L) <= 0.05 && c > 0.8;
                    ++checked;
                    bad += !good;
                    say("  loop: %.0f s from the middle of the first round fit best %.2f s later (lags %.2f-%.2f s tried; correlation %.3f), measured loop %.2f s (%.3f) %s",
                        w, bestLag, std::round(L * 50) / 100, 1.5 * L, bestC, L, c, good ? "ok" : "BAD");
                    if (i == 1) {   // the seek: Chapter 1's song to 60 % (the board run muted up to there)
                        Wav wv;
                        if (wv.open((fs::path(outDir) / "Arcade_music_02_whole.wav").string())) { wv.put(s); wv.close(); }
                        const std::vector<float> whole = s;
                        const auto t0 = std::chrono::steady_clock::now();
                        jb.seek(0.6);
                        int frames = 0;
                        while ((jb.seekTo_ >= 0 || jb.pendingCode_) && frames < 3600) {
                            jb.update(1.0 / 60.0);
                            audio.renderTo(block, kBlock);
                            ++frames;
                        }
                        const double took = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                        const double at = jb.elapsed(), want = 0.6 * it.length;
                        run(w, s);
                        if (wv.open((fs::path(outDir) / "Arcade_music_02_seek60.wav").string())) { wv.put(s); wv.close(); }
                        const double cs = corr(envelope(s, 0, w), envelope(whole, at, at + w));
                        const bool sg = std::fabs(at - want) < 0.1 && cs > 0.9;
                        ++checked;
                        bad += !sg;
                        say("  seek to 60 %%: at %.2f s (wanted %.2f s) after %.2f s (%d frames); the next %.0f s vs the same place played through: correlation %.3f %s",
                            at, want, took, frames, w, cs, sg ? "ok" : "BAD");
                    }
                    jb.stop();
                    run(0.1, s);
                } else if (!it.loops && it.length > 0) {
                    jb.mode = kOnce;
                    startArcade(i);
                    run(it.length + 2.5, s);
                    const double last = lastSoundOf(s);
                    const bool good = std::fabs(last - it.length) < 0.3;
                    ++checked;
                    bad += !good;
                    if (!good || cat == kMusic)
                        say("  ends: sound until %.2f s (measured %.2f s) %s", last, it.length, good ? "ok" : "BAD");
                    jb.stop();
                    run(0.1, s);
                }
            }
        }
        failures += bad;
        say("== arcade lengths: %d checks, %d bad", checked, bad);
    }
    say("JUKEBOX TEST: %d items, %d failed", tested, failures);
    if (rep) std::fclose(rep);
    return failures;
}

} // namespace hotd::game
