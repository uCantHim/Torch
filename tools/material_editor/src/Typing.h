#pragma once

#include <cmath>
#include <compare>
#include <variant>

#include <trc/material/BasicType.h>
#include <trc/material/ShaderModuleBuilder.h>

using namespace trc::basic_types;

/**
 * @brief Subtype relationship among basic types
 */
auto operator<=>(const trc::BasicType::Type a, const trc::BasicType::Type b)
    -> std::strong_ordering;

auto max(trc::BasicType a, trc::BasicType b);

struct TypeRange
{
    static constexpr i32 kChannelDefaultValue{ 0 };

    trc::BasicType::Type underlyingTypeUpperBound;
    ui8 minChannels{ 1 };
    ui8 maxChannels{ 1 };

    /**
     * @brief Test if we can find a minimal type in the type range to which
     *        a type can be up-cast.
     *
     * Equivalent to `findMinCommonType(srcType).has_value()`.
     */
    bool isConstructibleFrom(trc::BasicType srcType) const;

    auto findMinCommonType(trc::BasicType type) const -> std::optional<trc::BasicType>;

    static auto makeEq(trc::BasicType type) -> TypeRange;
    static auto makeMax(trc::BasicType upperBound) -> TypeRange;
    static auto makeClamp(trc::BasicType lowerBound, trc::BasicType upperBound) -> TypeRange;
};

struct EqualTo {
    trc::BasicType type;
};
struct InRange {
    TypeRange range;
};

using TypeConstraint = std::variant<trc::BasicType, TypeRange>;

/**
 * @return bool True if `type` satisfies the type constraint `constraint`.
 */
bool satisfies(trc::BasicType type, const TypeConstraint& constraint);
