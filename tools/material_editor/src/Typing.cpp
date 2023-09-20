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

auto TypeRange::makeEq(trc::BasicType type) -> TypeRange
{
    return { .underlyingTypeUpperBound=type.type, .minChannels=type.channels, .maxChannels=type.channels };
}

auto TypeRange::makeMax(trc::BasicType upperBound) -> TypeRange
{
    return {
        .underlyingTypeUpperBound=upperBound.type,
        .minChannels=1,
        .maxChannels=upperBound.channels
    };
}

auto TypeRange::makeClamp(trc::BasicType lowerBound, trc::BasicType upperBound) -> TypeRange
{
    assert(lowerBound.channels <= upperBound.channels);
    return {
        .underlyingTypeUpperBound=std::max(lowerBound.type, upperBound.type),
        .minChannels=lowerBound.channels,
        .maxChannels=upperBound.channels
    };
}

bool satisfies(trc::BasicType type, const TypeConstraint& constraint)
{
    return std::visit(trc::util::VariantVisitor{
        [=](const trc::BasicType& t){ return type == t; },
        [=](const TypeRange& r){ return r.isConstructibleFrom(type); },
    }, constraint);
}
