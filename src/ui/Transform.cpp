#include "ui/Transform.h"

#include "ui/Window.h"



auto trc::ui::concat(Transform parent, Transform child, const Window& window) noexcept
    -> Transform
{
    const vec2 windowSize = window.getSize();

    // Normalize pixel values
    if (child.posProp.format == Format::ePixel)   { child.position /= windowSize; }
    if (child.sizeProp.format == Format::ePixel)  { child.size /= windowSize; }
    if (parent.posProp.format == Format::ePixel)  { parent.position /= windowSize; }
    if (parent.sizeProp.format == Format::ePixel) { parent.size /= windowSize; }

    return {
        .position = child.posProp.alignment == Align::eRelative
            ? parent.position + child.position
            : child.position,
        .size = child.sizeProp.alignment == Align::eRelative
            ? parent.size * child.size
            : child.size,
        .posProp = { .format = Format::eNorm, .alignment = Align::eAbsolute },
        .sizeProp = { .format = Format::eNorm, .alignment = Align::eAbsolute },
    };
}
