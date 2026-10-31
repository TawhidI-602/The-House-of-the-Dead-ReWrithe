#include "assets/assets.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>

namespace hotd {

const char* sourceName(Source s)
{
    return s == Source::PC ? "PC" : "arcade";
}

const char* categoryName(Category c)
{
    switch (c) {
    case Category::World: return "world";
    case Category::Zombies: return "zombies";
    case Category::Bosses: return "bosses";
    case Category::Effects: return "weapons/effects";
    case Category::Humans: return "humans";
    }
    return "?";
}

size_t Model::triangleCount() const
{
    size_t n = 0;
    for (const Mesh& m : meshes)
        if (m.variant < 0 || m.variant == (preferredVariant < 0 ? 0 : preferredVariant)) n += m.vertexCount() / 3;
    return n;
}

void Model::computeBounds()
{
    bool first = true;
    for (const Mesh& m : meshes)
        for (const Vertex& v : m.vertices)
            for (int i = 0; i < 3; ++i) {
                if (first || v.pos[i] < boundsMin[i]) boundsMin[i] = v.pos[i];
                if (first || v.pos[i] > boundsMax[i]) boundsMax[i] = v.pos[i];
                if (i == 2) first = false;
            }
}

Category StageData::categoryOf(uint32_t id) const
{
    auto it = categories.find(id);
    return it == categories.end() ? Category::World : it->second;
}

int StageData::zoneVariant(const float pos[3]) const
{
    int best = -1;
    float bestD = 0;
    for (const Zone& z : zones) {
        float d = 0;
        for (int i = 0; i < 3; ++i) d += (z.pos[i] - pos[i]) * (z.pos[i] - pos[i]);
        if (best < 0 || d < bestD) { best = z.variant; bestD = d; }
    }
    return best;
}

Source SourceSelection::preferred(uint32_t id, Category c) const
{
    auto it = perModel.find(id);
    return it != perModel.end() ? it->second : category[static_cast<int>(c)];
}

void SourceSelection::setAll(Source s)
{
    for (Source& c : category)
        c = s;
    perModel.clear();
}

std::vector<DrawItem> resolveScene(const StageData* const stages[kSourceCount], const SourceSelection& sel)
{
    // Which canonical IDs each source can draw (has at least one placement with triangles).
    std::set<uint32_t> ids;
    std::set<uint32_t> drawable[kSourceCount];
    for (int s = 0; s < kSourceCount; ++s) {
        if (!stages[s]) continue;
        for (const Placement& p : stages[s]->placements) {
            auto m = stages[s]->models.find(p.modelKey);
            if (m != stages[s]->models.end() && m->second.triangleCount() > 0) {
                drawable[s].insert(p.id);
                ids.insert(p.id);
            }
        }
    }
    std::vector<DrawItem> out;
    for (uint32_t id : ids) {
        Category cat = Category::World;
        for (int s = 0; s < kSourceCount; ++s)
            if (stages[s] && drawable[s].count(id)) { cat = stages[s]->categoryOf(id); break; }
        int want = static_cast<int>(sel.preferred(id, cat));
        int use = drawable[want].count(id) ? want : (drawable[1 - want].count(id) ? 1 - want : -1);
        if (use < 0) continue;
        const StageData* st = stages[use];
        for (const Placement& p : st->placements)
            if (p.id == id) {
                auto m = st->models.find(p.modelKey);
                if (m != st->models.end() && m->second.triangleCount() > 0)
                    out.push_back({st, &m->second, &p, m->second.preferredVariant});
            }
    }
    return out;
}

bool readFile(const std::string& path, std::vector<uint8_t>& out, std::string& err)
{
    std::ifstream f(std::filesystem::path(path), std::ios::binary);
    if (!f) {
        err = "can't open " + path;
        return false;
    }
    f.seekg(0, std::ios::end);
    out.resize(static_cast<size_t>(f.tellg()));
    f.seekg(0);
    f.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(out.size()));
    if (!f) {
        err = "can't read " + path;
        return false;
    }
    return true;
}

bool fileExists(const std::string& path)
{
    std::error_code ec;
    return std::filesystem::exists(std::filesystem::path(path), ec);
}

} // namespace hotd
