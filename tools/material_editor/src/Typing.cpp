#include "Typing.h"

#include <trc_util/TypeUtils.h>



auto operator<=>(const trc::BasicType::Type a, const trc::BasicType::Type b)
    -> std::strong_ordering
{
    return static_cast<ui8>(a) <=> static_cast<ui8>(b);
}

auto max(trc::BasicType a, trc::BasicType b)
{
    return trc::BasicType{ std::max(a.type, b.type), std::max(a.channels, b.channels) };
}

TypeRange::TypeRange(const trc::BasicType& t)
{
    *this = makeEq(t);
}

TypeRange::TypeRange(trc::BasicType::Type t, ui8 minChannels, ui8 maxChannels)
    :
    underlyingTypeUpperBound(t),
    minChannels(minChannels),
    maxChannels(maxChannels)
{
}

bool TypeRange::isConstructibleFrom(trc::BasicType srcType) const
{
    return srcType.type < underlyingTypeUpperBound
        && srcType.channels <= maxChannels;
}

auto TypeRange::findMinCommonType(trc::BasicType type) const -> std::optional<trc::BasicType>
{
    if (type.type < underlyingTypeUpperBound && type.channels <= maxChannels) {
        return type;
    }
    return std::nullopt;
}

auto TypeRange::getUpperBoundType() const -> trc::BasicType
{
    return { underlyingTypeUpperBound, maxChannels };
}

auto TypeRange::makeEq(trc::BasicType type) -> TypeRange
{
    return { type.type, type.channels, type.channels };
}

auto TypeRange::makeMax(trc::BasicType upperBound) -> TypeRange
{
    return { upperBound.type, 1, upperBound.channels };
}

auto TypeRange::makeClamp(trc::BasicType lowerBound, trc::BasicType upperBound) -> TypeRange
{
    assert(lowerBound.channels <= upperBound.channels);
    return {
        std::max(lowerBound.type, upperBound.type),
        lowerBound.channels,
        upperBound.channels
    };
}

bool satisfies(trc::BasicType type, const TypeConstraint& constraint)
{
    return constraint.isConstructibleFrom(type);
}

auto intersect(const TypeRange& a, const TypeRange& b) -> std::optional<TypeRange>
{
    if (a.maxChannels < b.minChannels || b.maxChannels < a.minChannels) {
        return std::nullopt;
    }

    return TypeRange{
        std::min(a.underlyingTypeUpperBound, b.underlyingTypeUpperBound),
        std::max(a.minChannels, b.minChannels),
        std::min(a.maxChannels, b.maxChannels),
    };
}

auto toConcreteType(const TypeConstraint& c) -> std::optional<trc::BasicType>
{
    if (c.minChannels == c.maxChannels) {
        return trc::BasicType{ c.underlyingTypeUpperBound, c.minChannels };
    }
    return std::nullopt;
}
