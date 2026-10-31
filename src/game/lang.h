// lang (translations, user 2026-10-26; docs/translating.md): the language files data\lang\<code>.txt (UTF-8,
// `key = text` lines, # comments) and the lookup every player-visible text of ours goes through.
//
// The key of a text is its English wording (as en.txt, written by --export-lang en, lists them), so the lookup sits
// where the texts are drawn (the menus' text drawing, the HUD's notes) and the code keeps its English strings. A key
// may hold {placeholders} ("P{n} PAUSED", "Chapter {n}"): a text that matches it gets the translation with the
// pieces put back in (a piece that is itself a key is translated too). A missing key falls back to English.
// Special keys: lang.name (the language's own name, shown in Options), lang.font (a TTF / OTF in data\fonts or next
// to the file; a comma list: fallbacks for the characters the first lacks), lang.font_scale, lang.sample (1: a sample
// file), lang.upper (0: no capitals in the Remake style's small caps), lang.rtl (1: right to left; by default from the
// letters); sub.<event>.<line>: a cut-scene subtitle.
// English (no file, or en.txt) changes nothing: the lookup returns the text as given.
#pragma once

#include "render/ttf.h"

#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace hotd::lang {

// the folders that hold data\lang and data\fonts (the package's, the checkout's): set once at start
void setDataDirs(const std::vector<std::string>& dirs);
const std::vector<std::string>& dataDirs();

struct Info {
    std::string code, name, file;   // "ko", "한국어", its path ("" for built-in English)
    bool sample = false;
    std::string preview;   // menufb: a line in the language (its "Difficulty, lives, volumes and all settings"; English: that)
    std::string english;   // relmenu: its name in English ("Korean"; lang.english, else known by its code; "": none)
};
std::vector<Info> available();      // English first, then data\lang\*.txt (en.txt itself is English)

// loads a language ("" / "en": English); false (err): the file is missing or bad (English then)
bool set(const std::string& code, std::string* err = nullptr);
const std::string& code();          // "en" for English
bool active();                      // a language other than English is loaded
bool on();                          // active or recording: the texts go through tr (English: not, the same bytes)
bool reload(std::string* err = nullptr);   // the same file again (F5 in the menu)
uint64_t gen();                     // changes with every load (caches of drawn text compare it)
std::string displayName(const std::string& code);   // lang.name of that file ("English" for en)

// the text in the language (English when the key has no translation); trimmed of spaces for the lookup, the spaces
// kept around the result
std::string tr(const std::string& english);
// a pattern with its pieces: fmt("P{n} PAUSED", {{"n", "2"}}): the translation of the pattern with the pieces in
std::string fmt(const std::string& pattern, const std::vector<std::pair<std::string, std::string>>& args);
// a cut-scene subtitle's translation (key sub.<event>.<line>); null: none (the game's own line)
const std::string* subtitle(int event, int line);

// the language's font (null: the PC bitmap fonts: English, or a file without lang.font)
const render::TtfText* font();
// the font for the text s: the language's; else (English, or no lang.font) the fonts in data/fonts when s has a
// character the PC fonts lack (a language's own name in Options); null: the PC fonts
const render::TtfText* fontFor(const std::string& s);
bool plain(const std::string& s);   // ASCII only (the PC fonts draw it as it is)
const render::TtfText* anyFont();   // the language's font, else the ones in data/fonts (null: none there)
int columns(const std::string& s);  // the text's width on a character grid (Hangul / kana / CJK: two columns each)
// lines of at most `width` columns, broken at spaces (CJK: between any two characters); English: as the callers'
// own word wrap (the same lines)
std::vector<std::string> wrapColumns(const std::string& s, int width);
// lines of s that fit maxW by width() (the caller's measure: a word, or a CJK character, at a time)
std::vector<std::string> wrapBy(const std::string& s, float maxW, const std::function<float(const std::string&)>& width);
float fontScale();                  // lang.font_scale (1)
bool upperCase();                   // lang.upper (1: the Remake style capitalises Latin letters as for English)
bool rtl();                         // (rtl) the language reads right to left: its menus right-aligned, mirrored rows
// the pixel size of the font's em for a text whose capitals are capPx pixels high (the bitmap fonts' 'A')
float pxForCap(float capPx, const render::TtfText* font);
// UTF-8 to the PC fonts' bytes (Latin-1: their accented glyphs at 0xBF..); false: a character they lack
bool toLatin1(const std::string& s, std::string& out);

// recording (--export-lang, --lang-check): every key looked up
void record(bool on);
const std::vector<std::string>& recorded();   // in the order first seen (less the lines noted below)
// the lines a text was cut into (each drawn, measured: not keys of their own; the whole text is)
const std::vector<std::string>& noteLines(const std::vector<std::string>& lines);
// the patterns the code builds texts from (the export lists them, not each number)
void addPattern(const std::string& pattern);
const std::vector<std::string>& patterns();
// a recorded text as its key: a known pattern it matches, else itself
std::string keyOf(const std::string& text);

// --export-lang: writes the keys (with English, or the current file's translations) to path; extra: keys given by
// the caller (subtitles: sub.<e>.<l> = English)
bool exportFile(const std::string& path, const std::vector<std::string>& keys,
                const std::vector<std::pair<std::string, std::string>>& extra, std::string& err);

// the language to start in on a first run: Windows' UI language (else LANG) when a file for it exists; "" none
std::string autoPick();

} // namespace hotd::lang
