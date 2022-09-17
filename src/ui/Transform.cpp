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
            .positionFormat = { Format::eNorm, Format::eNorm },
            .sizeFormat     = { Format::eNorm, Format::eNorm },
            .scalingType    = t.scalingType
        };
    }
}

auto trc::ui::concat(Transform parent, Transform child, const Window& window) noexcept
    -> Transform
{
    assert(!(
        (child.scalingType.x == Scale::eRelativeToParent && child.sizeFormat.x == Format::ePixel)
        || (child.scalingType.y == Scale::eRelativeToParent && child.sizeFormat.y == Format::ePixel)
        )
        && "A relative size in pixel values does not make any sense. Don't do that!");

    const vec2 windowSize = window.getSize();

    // Normalize pixel values
    child = toNorm(child, windowSize);

    return {
        .position = {
            parent.position.x + child.position.x,
            parent.position.y + child.position.y,
        },
        .size = {
            child.scalingType.x == Scale::eRelativeToParent
                ? parent.size.x * child.size.x : child.size.x,
            child.scalingType.y == Scale::eRelativeToParent
                ? parent.size.y * child.size.y : child.size.y,
        },
        .positionFormat = { Format::eNorm, Format::eNorm },
        .sizeFormat     = { Format::eNorm, Format::eNorm },
        .scalingType    = { Scale::eAbsolute, Scale::eAbsolute },
    };
}
