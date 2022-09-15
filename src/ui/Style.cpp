#include "trc/ui/Style.h"

#include "ui/Window.h"



trc::ui::Padding::Padding(float x, float y)
    :
    padding(x, y)
{
}

void trc::ui::Padding::set(vec2 pad)
{
    padding = pad;
}

void trc::ui::Padding::set(float x, float y)
{
    padding = { x, y };
}

auto trc::ui::Padding::get() const -> vec2
{
    return padding;
}

auto trc::ui::Padding::calcNormalizedPadding(const Window& window) const -> vec2
{
    return padding / window.getSize();
}
