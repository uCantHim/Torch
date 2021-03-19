#include "ui/Transform.h"

#include "ui/Window.h"



auto trc::ui::concat(Transform parent, Transform child, const Window& window) noexcept
    -> Transform
{
    const vec2 windowSize = window.getSize();

    // Normalize pixel values
    if (child.posProp.type == SizeType::ePixel)   { child.position /= windowSize; }
    if (child.sizeProp.type == SizeType::ePixel)  { child.size /= windowSize; }
    if (parent.posProp.type == SizeType::ePixel)  { parent.position /= windowSize; }
    if (parent.sizeProp.type == SizeType::ePixel) { parent.size /= windowSize; }

    return {
        .position = child.posProp.alignment == Align::eRelative
            ? parent.position + child.position
            : child.position,
        .size = child.sizeProp.alignment == Align::eRelative
            ? parent.size * child.size
            : child.size,
        .posProp = { .type = SizeType::eNorm, .alignment = Align::eAbsolute },
        .sizeProp = { .type = SizeType::eNorm, .alignment = Align::eAbsolute },
    };
}
