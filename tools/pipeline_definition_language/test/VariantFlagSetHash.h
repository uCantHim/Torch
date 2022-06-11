#pragma once

#include <algorithm>
#include <ranges>

#include "../src/FlagTable.h"

inline auto sorted(const VariantFlagSet& set)
{
    auto copy = set;
    std::ranges::sort(copy, [](auto&& a, auto&& b){ return a.flagId < b.flagId; });
    return copy;
}

namespace std
{
    template<>
    struct hash<VariantFlagSet>
    {
        auto operator()(const VariantFlagSet& set) const -> size_t
        {
            size_t hash{ 0 };
            std::hash<VariantFlag> hasher;
            for (const auto& flag : sorted(set)) {
                // Has combine as boost does it
                hash ^= hasher(flag) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
            return hash;
        }
    };
}

inline bool operator==(const VariantFlagSet& _a, const VariantFlagSet& _b)
{
    auto sorted = [&](auto& set){
        auto copy = set;
        std::ranges::sort(copy, [](auto&& a, auto&& b){ return a.flagId < b.flagId; });
        return copy;
    };

    auto a = sorted(_a);
    auto b = sorted(_b);

    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a.at(i) != b.at(i)) return false;
    }
    return true;
}
