// PC Mode's high-score table (pcmode_scores.h). From THOTD.EXE: the defaults (table 1 at 0x5CDC74), FUN_0042593c,
// InsertScore 0x425798 / FUN_0042556c, mode67_ranking_pc 0x44D0C4. UNVERIFIED (teaser).
#include "game/pcmode_scores.h"
#include "util/utf8.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>

namespace hotd::game {

PcModeScores::PcModeScores()
{
    static const char* const kNames[10] = {"BART", "BEN", "JOHN", "PAUL", "SCOTTIE", "STEVE", "BRETT", "SHANE", "ANDY", "ROHAN"};
    static const int kScore[10] = {62000, 60000, 50000, 40000, 30000, 20000, 10000, 8000, 6000, 4000};
    static const int kChar[10] = {1, 2, 3, 4, 5, 6, 1, 2, 3, 4};
    static const int kStage[10] = {3, 3, 3, 2, 2, 1, 1, 0, 0, 0};
    for (int i = 0; i < 10; ++i)
        rows[i] = {kScore[i], kChar[i], kStage[i], kNames[i]};
}

bool PcModeScores::load(const std::string& path)
{
    std::ifstream f(path);
    if (!f)
        return false;
    PcModeScores t;
    int n = 0;
    std::string line;
    while (n < 10 && std::getline(f, line)) {
        std::istringstream s(line);
        Row r;
        if (!(s >> r.score >> r.ch >> r.stage >> r.name))
            continue;
        if (r.name == "_")
            r.name.clear();
        std::replace(r.name.begin(), r.name.end(), '_', ' ');   // (a space in a name is kept as '_')
        r.name = util::utf8Head(r.name, 15);
        r.ch = std::clamp(r.ch, 0, 19);   // (pmextra: the Extra Characters 7..19, port::Game::kPmCharCount)
        r.stage = std::clamp(r.stage, 0, 3);
        r.score = std::clamp(r.score, 0, 99999999);
        t.rows[n++] = r;
    }
    *this = t;
    return true;
}

bool PcModeScores::save(const std::string& path) const
{
    std::ofstream f(path, std::ios::trunc);
    if (!f)
        return false;
    for (const Row& r : rows) {
        std::string n = r.name;
        std::replace(n.begin(), n.end(), ' ', '_');
        f << r.score << ' ' << r.ch << ' ' << r.stage << ' ' << (n.empty() ? "_" : n) << '\n';
    }
    return bool(f);
}

bool PcModeScores::isHigh(int score, const std::vector<int>& others) const
{
    std::vector<int> list;
    for (const Row& r : rows)
        list.push_back(r.score);
    list.insert(list.end(), others.begin(), others.end());
    std::sort(list.begin(), list.end(), [](int a, int b) { return a > b; });
    return score >= 1 && score >= list[9];
}

int PcModeScores::insert(const Row& row)
{
    rows[9] = row;
    int at = 9;
    for (int i = 0; i < 10; ++i)
        for (int j = i; j < 10; ++j)
            if (rows[i].score < rows[j].score) {
                std::swap(rows[i], rows[j]);
                if (at == i) at = j;
                else if (at == j) at = i;
            }
    return at;
}

int pcModeStage(int chapter, int lives, int& kept)
{
    if (lives > 0)
        kept = chapter < 0 ? 0 : std::min(chapter, 3);
    return kept;
}

} // namespace hotd::game
