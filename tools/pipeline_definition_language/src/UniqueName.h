#pragma once

#include <filesystem>
#include <string>
#include <vector>
namespace fs = std::filesystem;

#include "FlagTable.h"

struct UniqueName;

/**
 * @brief Insert a unique extension into a file path to make it unique
 */
auto addUniqueExtension(fs::path file, const UniqueName& uniqueName) -> fs::path;

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

    /**
     * Calculate the UniqueName's flag combination's unique index as is
     * done in the resulting FlagCombination type.
     *
     * @param const FlagTable& flagTable The flag table at which the
     *        UniqueName's flag type is registered.
     */
    auto calcFlagIndex(const FlagTable& flagTable) const -> size_t;

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
