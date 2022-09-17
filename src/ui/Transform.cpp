#include "trc/ui/Transform.h"

#include "trc/ui/Window.h"



namespace trc::ui
{
    auto toNorm(Transform t, vec2 windowSize) -> Transform
    {
        return {
            .position = {
                t.positionFormat.x == Format::ePixel ? (t.position.x / windowSize.x) : t.position.x,
                t.positionFormat.y == Format::ePixel ? (t.position.y / windowSize.y) : t.position.y
            },
            .size = {
                t.sizeFormat.x == Format::ePixel ? (t.size.x / windowSize.x) : t.size.x,
                t.sizeFormat.y == Format::ePixel ? (t.size.y / windowSize.y) : t.size.y
            },
            .positionFormat  = { Format::eNorm, Format::eNorm },
            .positionScaling = t.positionScaling,
            .sizeFormat      = { Format::eNorm, Format::eNorm },
            .sizeScaling     = t.sizeScaling
        };
    }
}

auto trc::ui::concat(Transform parent, Transform child, const Window& window) noexcept
    -> Transform
{
    assert(!(
        (child.sizeScaling.x == Scale::eRelativeToParent && child.sizeFormat.x == Format::ePixel)
        || (child.sizeScaling.y == Scale::eRelativeToParent && child.sizeFormat.y == Format::ePixel)
        )
        && "A relative size in pixel values does not make any sense. Don't do that!");

    const vec2 windowSize = window.getSize();

    const Vec2D<bool> relativePos = {
        !(child.positionFormat.x == Format::ePixel) && child.positionScaling.x == Scale::eRelativeToParent,
        !(child.positionFormat.y == Format::ePixel) && child.positionScaling.y == Scale::eRelativeToParent
    };

    // Normalize pixel values
    child = toNorm(child, windowSize);

    return {
        .position = {
            parent.position.x + (relativePos.x ? parent.size.x * child.position.x : child.position.x),
            parent.position.y + (relativePos.y ? parent.size.y * child.position.y : child.position.y),
        },
        .size = {
            child.sizeScaling.x == Scale::eRelativeToParent
                ? parent.size.x * child.size.x : child.size.x,
            child.sizeScaling.y == Scale::eRelativeToParent
                ? parent.size.y * child.size.y : child.size.y,
        },
        .positionFormat  = { Format::eNorm, Format::eNorm },
        .positionScaling = { Scale::eAbsolute, Scale::eAbsolute },
        .sizeFormat      = { Format::eNorm, Format::eNorm },
        .sizeScaling     = { Scale::eAbsolute, Scale::eAbsolute },
    };
}
