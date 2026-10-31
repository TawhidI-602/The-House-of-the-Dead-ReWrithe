// lang (rtl, user 2026-10-26: right-to-left languages): a line of text in display order. The Unicode Bidirectional
// Algorithm (UAX #9) for one paragraph without explicit embeddings (no LRE / RLO / isolates: UI texts have none):
// the character types, the weak rules W1-W7, the bracket pairs N0, the neutrals N1-N2, the levels I1-I2, the
// trailing spaces L1, the reversal L2 and the mirrored brackets L4. Arabic letters are shaped first into their
// contextual forms (isolated / initial / medial / final, the lam-alef ligatures) as the Unicode Arabic Presentation
// Forms (FB50-FDFF, FE70-FEFF; Noto Sans Arabic maps them all), so a font draws them with no GSUB of its own.
// Hebrew needs no shaping. A text without a right-to-left character is never touched (English: the same bytes).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hotd::render {

bool isRtlChar(uint32_t cp);                   // Hebrew, Arabic, Syriac, Thaana, N'Ko ... (strong R / AL)
bool hasRtl(const std::string& utf8);          // a right-to-left character in the text

// the paragraph direction for texts with a right-to-left character: true (set by a right-to-left language: its lines
// read from the right even when they start with a Latin word), false (by the first strong character, UAX #9 P2)
void setBaseRtl(bool rtl);
bool baseRtl();

// the code points of one line (no '\n') shaped (Arabic) and in display order, left to right; base: 1 right to left,
// 0 left to right, -1 by its first strong character
std::vector<uint32_t> visualLine(const std::vector<uint32_t>& logical, int base);
// the same for UTF-8 (each line of a text with '\n' on its own); a text with no right-to-left character comes back
// as it is
std::string visual(const std::string& utf8);

// Arabic shaping alone (logical order): the presentation forms for the letters (exposed for tests)
std::vector<uint32_t> shapeArabic(const std::vector<uint32_t>& logical);

} // namespace hotd::render
