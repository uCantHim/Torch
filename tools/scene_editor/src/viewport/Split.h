#pragma once

#include <variant>

#include <trc/Types.h>
#include <trc_util/TypeUtils.h>
using namespace trc::basic_types;

#include "viewport/Viewport.h"

struct SplitLocation
{
    static auto makePixel(ui32 pos) -> SplitLocation {
        return SplitLocation{ pos };
    }

    static auto makeNormalized(float pos) -> SplitLocation {
        return SplitLocation{ pos };
    }

    auto getPosInPixels(ui32 parentSize) const -> ui32
    {
        return std::visit(trc::util::VariantVisitor{
            [](ui32 p){ return p; },
            [=](float n){ return static_cast<ui32>(n * static_cast<float>(parentSize)); }
        }, pos);
    }

private:
    explicit SplitLocation(std::variant<ui32, float> pos) : pos(pos) {}
    std::variant<ui32, float> pos;
};

struct SplitInfo
{
    // True if the split is along the horizontal axis, false if it is along
    // the vertical axis.
    bool horizontal;

    // Location of the split on the split axis.
    SplitLocation location;
};

inline constexpr auto splitArea(const ViewportArea& area, const SplitInfo& split)
    -> std::pair<ViewportArea, ViewportArea>
{
    if (split.horizontal)
    {
        const ui32 loc = split.location.getPosInPixels(area.size.y);
        return {
            { area.pos,                 uvec2{ area.size.x, loc } },
            { ivec2{ area.pos.x, loc }, uvec2{ area.size.x, area.size.y - loc } },
        };
    }
    else /* !split.horizontal */
    {
        const ui32 loc = split.location.getPosInPixels(area.size.x);
        return {
            { area.pos,                 uvec2{ loc, area.size.y } },
            { ivec2{ loc, area.pos.y }, uvec2{ area.size.x - loc, area.size.y } },
        };
    }
}
