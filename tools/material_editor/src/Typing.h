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

    TypeRange() = default;

    TypeRange(const trc::BasicType& t);
    template<std::convertible_to<trc::BasicType> T>
        requires (!std::same_as<std::decay_t<T>, trc::BasicType>)
    TypeRange(T&& t) : TypeRange(trc::BasicType{ t }) {}

    TypeRange(trc::BasicType::Type t, ui8 minChannels, ui8 maxChannels);

    trc::BasicType::Type underlyingTypeUpperBound{ trc::BasicType::Type::eFloat };
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

    auto getUpperBoundType() const -> trc::BasicType;

    static auto makeEq(trc::BasicType type) -> TypeRange;
    static auto makeMax(trc::BasicType upperBound) -> TypeRange;
    static auto makeClamp(trc::BasicType lowerBound, trc::BasicType upperBound) -> TypeRange;
};

using TypeConstraint = TypeRange;

/**
 * @return bool True if `type` satisfies the type constraint `constraint`.
 */
bool satisfies(trc::BasicType type, const TypeConstraint& constraint);

/**
 * @return std::optional<TypeRange> The intersection set of two type ranges,
 *                                  if one exists.
 */
auto intersect(const TypeRange& a, const TypeRange& b) -> std::optional<TypeRange>;

/**
 * @brief Test if a type constraint results in a single possible type
 */
auto toConcreteType(const TypeConstraint& constr) -> std::optional<trc::BasicType>;
