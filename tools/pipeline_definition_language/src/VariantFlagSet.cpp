#include "VariantFlagSet.h"

#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_set>



VariantFlagSet::VariantFlagSet(std::initializer_list<VariantFlag> list)
    :
    flags(list.begin(), list.end())
{
    auto flagTypeEq = [](auto&& a, auto&& b){ return a.flagId == b.flagId; };
    auto flagTypeHash = [](auto&& v) -> size_t { return v.flagId; };
    using Set = std::unordered_set<VariantFlag, decltype(flagTypeHash), decltype(flagTypeEq)>;

    // Check for duplicates
    if (flags.size() != Set(flags.begin(), flags.end()).size())
    {
        throw std::invalid_argument("[In VariantFlagSet::VariantFlagSet]: The supplied list "
                                    " of variant flags contains duplicate flag types.");
    }

    // Sort the range
    std::ranges::sort(flags, compareFlagTypeLess);
}

bool VariantFlagSet::empty() const
{
    return flags.empty();
}

auto VariantFlagSet::size() const -> size_t
{
    return flags.size();
}

void VariantFlagSet::emplace(VariantFlag flag)
{
    auto it = std::ranges::lower_bound(flags, flag, compareFlagTypeLess);
    if (it != flags.end() && it->flagId == flag.flagId)
    {
        throw std::invalid_argument("[In VariantFlagSet::emplace]: The set already contains a flag"
                                    " of type " + std::to_string(flag.flagId));
    }
    flags.insert(it, flag);
}

bool VariantFlagSet::contains(size_t flagType) const
{
    return std::ranges::find_if(flags, [=](auto&& v){ return v.flagId == flagType; })
           != flags.end();
}

bool VariantFlagSet::contains(const VariantFlag& flag) const
{
    return std::ranges::find(flags, flag) != flags.end();
}

auto VariantFlagSet::at(size_t index) const -> const VariantFlag&
{
    return flags.at(index);
}

auto VariantFlagSet::operator[](size_t index) const -> const VariantFlag&
{
    return flags.at(index);
}

bool VariantFlagSet::compareFlagTypeLess(const VariantFlag& a, const VariantFlag& b)
{
    return a.flagId < b.flagId;
}
