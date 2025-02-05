#pragma once

#include <cmath>
#include <compare>
#include <variant>

#include <trc/material/shader/BasicType.h>
#include <trc/material/shader/ShaderModuleBuilder.h>

using namespace trc::basic_types;
namespace trc {  // HACK
    using namespace shader;
}

/**
 * @brief A value's type on the abstraction level of material node inputs
 *        and outputs.
 */
enum class SemanticType
{
    eRgb,
    eRgba,

    eFloat,
    eVec2,
    eVec3,
};

/**
 * @brief Get the shader type corresponding to a user input field type
 */
constexpr auto toShaderType(SemanticType t) -> trc::BasicType;

constexpr auto toSemanticType(trc::BasicType t) -> SemanticType;

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

    /**
     * @brief Create a type range that matches a high-level type exactly
     */
    TypeRange(SemanticType type) : TypeRange(toShaderType(type)) {}

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



// -----------------------------------------------------------------------------
// Implementations

constexpr auto toShaderType(SemanticType f) -> trc::BasicType
{
    switch (f) {
        case SemanticType::eRgb: return vec3{};
        case SemanticType::eRgba: return vec4{};
        case SemanticType::eFloat: return float{};
        case SemanticType::eVec2: return vec2{};
        case SemanticType::eVec3: return vec3{};
    }
}

constexpr auto toSemanticType(trc::BasicType t) -> SemanticType
{
    if (t.type >= trc::BasicType::Type::eFloat)
    {
        switch (t.channels)
        {
        case 1: return SemanticType::eFloat;
        case 2: return SemanticType::eVec2;
        case 3: return SemanticType::eVec3;
        default: break;
        }
    }

    throw std::logic_error("Unable to convert basic type " + trc::code::types::to_string(t)
                           + " to a semantic type.");
}
