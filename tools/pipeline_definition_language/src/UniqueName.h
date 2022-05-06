#pragma once

#include <string>
#include <vector>

#include "FlagTable.h"

struct UniqueName
{
public:
    UniqueName(const UniqueName&) = default;
    UniqueName(UniqueName&&) noexcept = default;
    auto operator=(const UniqueName&) -> UniqueName& = default;
    auto operator=(UniqueName&&) noexcept -> UniqueName& = default;
    ~UniqueName() = default;

    UniqueName(std::string str);
    UniqueName(std::string str, VariantFlagSet flags);

    bool operator==(const UniqueName&) const = default;

    auto hash() const -> size_t;

    bool hasFlags() const;
    auto getFlags() const -> const VariantFlagSet&;
    auto getBaseName() const -> const std::string&;
    auto getUniqueName() const -> const std::string&;
    auto getUniqueExtension() const -> std::string;

private:
    std::string name;
    VariantFlagSet flags;

    std::string uniqueName;
};

namespace std
{
    template<>
    struct hash<UniqueName>
    {
        auto operator()(const UniqueName& name) const -> size_t {
            return name.hash();
        }
    };
}
