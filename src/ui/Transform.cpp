#include "trc/ui/Transform.h"

#include "trc/ui/Window.h"



namespace trc::ui
{
    auto toNorm(Transform t, vec2 windowSize) -> Transform
    {
        return {
            .position = {
                t.posProp.format.x == Format::ePixel ? (t.position.x / windowSize.x) : t.position.x,
                t.posProp.format.y == Format::ePixel ? (t.position.y / windowSize.y) : t.position.y
            },
            .size = {
                t.sizeProp.format.x == Format::ePixel ? (t.size.x / windowSize.x) : t.size.x,
                t.sizeProp.format.y == Format::ePixel ? (t.size.y / windowSize.y) : t.size.y
            },
            .posProp = {
                { Format::eNorm, Format::eNorm },
                t.posProp.align
                //{ Format::eNorm, t.posProp.x.align },
                //{ Format::eNorm, t.posProp.y.align }
            },
            .sizeProp = {
                { Format::eNorm, Format::eNorm },
                t.sizeProp.align
                //{ Format::eNorm, t.sizeProp.x.align },
                //{ Format::eNorm, t.sizeProp.y.align }
            }
        };
    }
}

auto trc::ui::concat(Transform parent, Transform child, const Window& window) noexcept
    -> Transform
{
    const vec2 windowSize = window.getSize();

    // Normalize pixel values
    child = toNorm(child, windowSize);

    return {
        .position = {
            child.posProp.align.x == Align::eRelative
                ? parent.position.x + child.position.x : child.position.x,
            child.posProp.align.y == Align::eRelative
                ? parent.position.y + child.position.y : child.position.y,
        },
        .size = {
            child.sizeProp.align.x == Align::eRelative
                ? parent.size.x * child.size.x : child.size.x,
            child.sizeProp.align.y == Align::eRelative
                ? parent.size.y * child.size.y : child.size.y,
        },
        .posProp = {
            { Format::eNorm, Format::eNorm },
            { Align::eAbsolute, Align::eAbsolute },
        },
        .sizeProp = {
            { Format::eNorm, Format::eNorm },
            { Align::eAbsolute, Align::eAbsolute },
        }
    };
}
