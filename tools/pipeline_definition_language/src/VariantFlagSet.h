#pragma once

#include <initializer_list>
#include <vector>
#include <functional>

struct VariantFlag
{
    size_t flagId;
    size_t flagBitId;

    inline bool operator==(const VariantFlag& a) const {
        return flagId == a.flagId && flagBitId == a.flagBitId;
    }
};

/**
 * @brief A combination of different variant flags
 *
 * The range of flags is always sorted by flag type.
 *
 * Does not allow different flag bits of the same flag type.
 */
class VariantFlagSet
{
public:
    VariantFlagSet() = default;

    /**
     * @throw std::invalid_argument if the initializer list contains flags of the same type
     */
    VariantFlagSet(std::initializer_list<VariantFlag> list);

    inline auto begin()       { return flags.begin(); }
    inline auto begin() const { return flags.begin(); }
    inline auto end()         { return flags.end(); }
    inline auto end()   const { return flags.end(); }

    bool operator==(const VariantFlagSet& rhs) const = default;
    bool operator!=(const VariantFlagSet& rhs) const = default;

    /**
     * @return bool True if the set contains no flag bits.
     */
    bool empty() const;

    /**
     * @return size_t The number of flag bits present in the set
     */
    auto size() const -> size_t;

    /**
     * @brief Add a flag to the set
     *
     * @param VariantFlag flag The flag to be inserted.
     *
     * @throw std::invalid_argument if the set already contains a flag with
     *        the same flag type as `flag`
     */
    void emplace(VariantFlag flag);

    /**
     * @return bool True if the set contains a variant flag of type `flagType`.
     */
    bool contains(size_t flagType) const;

    /**
     * @return bool True if the set contains the exact flag bit `flag`.
     */
    bool contains(const VariantFlag& flag) const;

    auto at(size_t index) const -> const VariantFlag&;
    auto operator[](size_t index) const -> const VariantFlag&;

private:
    static bool compareFlagTypeLess(const VariantFlag& a, const VariantFlag& b);

    std::vector<VariantFlag> flags;
};

namespace std
{
    template<>
    struct hash<VariantFlag>
    {
        auto operator()(const VariantFlag& f) const -> size_t
        {
            return hash<size_t>{}((f.flagId << (sizeof(size_t) / 2)) + f.flagBitId);
        }
    };

    template<>
    struct hash<VariantFlagSet>
    {
        auto operator()(const VariantFlagSet& set) const -> size_t
        {
            size_t hash{ 0 };
            for (const auto& flag : set)
            {
                // Has combine as boost does it
                hash ^= std::hash<VariantFlag>{}(flag) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
            return hash;
        }
    };
}
