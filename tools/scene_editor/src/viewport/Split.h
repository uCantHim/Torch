#pragma once

#include <utility>
#include <variant>

#include <trc/Types.h>
#include <trc_util/Assert.h>
#include <trc_util/TypeUtils.h>
using namespace trc::basic_types;

#include "viewport/Viewport.h"

/**
 * @brief Position on a line.
 *
 * Can be specified either in units of pixels or relative to the parent's size.
 */
struct SplitLocation
{
    /**
     * @brief Create a split location specified in units of pixels.
     *
     * @param pos The split location as an offset from the start of the first
     *            child (the left or the top one, respectively) in pixels. Can
     *            be negative to indicate an offset from the opposite side.
     */
    static auto makePixel(i32 pos) -> SplitLocation {
        return SplitLocation{ pos };
    }

    /**
     * @brief Create a split location relative to the split's total size.
     *
     * @param pos The split location as an offset from the start of the first
     *            child (the left or the top one, respectively) in percent of
     *            the total split size. Must be in `(0, 1)`.
     *
     * @throw std::invalid_argument if pos not in `(0, 1)`.
     */
    static auto makeRelative(float pos) -> SplitLocation {
        assert_arg(0.0f < pos && pos < 1.0f);
        return SplitLocation{ pos };
    }

    /**
     * @brief Calculate the concrete split location on a line in pixels.
     *
     * Resolves relative locations to pixel units.
     *
     * @return A pixel value in `[0, parentSize)`.
     */
    auto getPosInPixels(ui32 parentSize) const -> ui32
    {
        const i32 pixels = std::visit(trc::util::VariantVisitor{
            [=](i32 loc){ return loc < 0 ? parentSize + loc : loc; },
            [=](float n){ return static_cast<ui32>(n * static_cast<float>(parentSize)); }
        }, pos);
        return glm::clamp(pixels, 0, static_cast<i32>(parentSize) - 1);
    }

private:
    explicit SplitLocation(std::variant<i32, float> pos) : pos(pos) {}
    std::variant<i32, float> pos;
};

struct SplitInfo
{
    // True if the split is along the horizontal axis, false if it is along
    // the vertical axis.
    bool horizontal;

    // Location of the split on the split axis. Interpreted as an offset from
    // the split area's origin.
    SplitLocation location;
};

/**
 * @brief Split a viewport area into two according to a splitting description.
 *
 * @return A pair `(fst, snd)`. `fst` is the left area if the split is along the
 *         vertical axis, the upper area for horizontal splits.
 */
inline constexpr auto splitArea(const ViewportArea& area, const SplitInfo& split)
    -> std::pair<ViewportArea, ViewportArea>
{
    if (split.horizontal)
    {
        const ui32 off = split.location.getPosInPixels(area.size.y);
        return {
            { area.pos,                              uvec2{ area.size.x, off } },
            { ivec2{ area.pos.x, area.pos.y + off }, uvec2{ area.size.x, area.size.y - off } },
        };
    }
    else /* !split.horizontal */
    {
        const ui32 off = split.location.getPosInPixels(area.size.x);
        return {
            { area.pos,                              uvec2{ off, area.size.y } },
            { ivec2{ area.pos.x + off, area.pos.y }, uvec2{ area.size.x - off, area.size.y } },
        };
    }
}
