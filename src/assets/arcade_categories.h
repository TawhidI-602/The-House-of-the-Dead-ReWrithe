// Character categories per object ID (docs/formats.md §25): zombies (every enemy), bosses, humans (scientists, agents,
// Sophie) from a table of the 94 arcade creature types (each type's skeleton lists its part IDs), plus name rules for
// boss parts that no skeleton lists (the bosses draw some parts themselves). The PC uses the same object IDs, so the table
// serves both sources. IDs it doesn't know keep whatever category the caller had.
#pragma once

#include "assets/arcade_anim.h"
#include "assets/assets.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace hotd {

class CreatureCategories {
public:
    void build(const CreatureAnimations& anim);
    std::optional<Category> of(uint32_t id) const;
    std::string name(uint32_t id) const;                 // arcade object name without "PN_" ("" if none)
    static Category ofType(int type);                    // the table
    static const char* typeName(int type);               // short description of a creature type

private:
    std::vector<std::string> names_;
    std::map<uint32_t, Category> byId_;
};

} // namespace hotd
