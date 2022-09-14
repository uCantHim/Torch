#include "trc/ui/elements/BaseElements.h"

#include "trc/ui/Window.h"



trc::ui::TextBase::TextBase(ui32 font, ui32 size)
    :
    fontIndex(font),
    fontSize(size)
{
}

void trc::ui::TextBase::setFont(ui32 fontIndex)
{
    this->fontIndex = fontIndex;
}

void trc::ui::TextBase::setFontSize(ui32 fontSize)
{
    this->fontSize = fontSize;
}



trc::ui::Paddable::Paddable(float x, float y, Vec2D<Format> format)
    :
    padding(x, y),
    format(format)
{
}

void trc::ui::Paddable::setPadding(vec2 pad, Vec2D<Format> format)
{
    padding = pad;
    this->format = format;
}

void trc::ui::Paddable::setPadding(float x, float y, Vec2D<Format> format)
{
    padding = { x, y };
    this->format = format;
}

void trc::ui::Paddable::setPadding(pix_or_norm x, pix_or_norm y)
{
    padding = { x.value, y.value };
    format = { x.format, y.format };
}

void trc::ui::Paddable::setPadding(Vec2D<pix_or_norm> v)
{
    padding = { v.x.value, v.y.value };
    format = { v.x.format, v.y.format };
}

auto trc::ui::Paddable::getPadding() const -> vec2
{
    return padding;
}

auto trc::ui::Paddable::getPaddingFormat() const -> Vec2D<Format>
{
    return format;
}

auto trc::ui::Paddable::getNormPadding(vec2 elemSize, const Window& window) const -> vec2
{
    return {
        format.x == Format::eNorm ? padding.x * elemSize.x : padding.x / window.getSize().x,
        format.y == Format::eNorm ? padding.y * elemSize.y : padding.y / window.getSize().y,
    };
}
