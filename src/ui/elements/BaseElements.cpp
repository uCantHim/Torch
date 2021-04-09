#include "ui/elements/BaseElements.h"

#include "ui/Window.h"



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



trc::ui::Paddable::Paddable(float x, float y, _2D<Format> format)
    :
    padding(x, y),
    format(format)
{
}

void trc::ui::Paddable::setPadding(vec2 pad, _2D<Format> format)
{
    padding = pad;
    this->format = format;
}

void trc::ui::Paddable::setPadding(float x, float y, _2D<Format> format)
{
    padding = { x, y };
    this->format = format;
}

void trc::ui::Paddable::setPadding(_pix x, _pix y)
{
    padding = { x.value, y.value };
    format = Format::ePixel;
}

void trc::ui::Paddable::setPadding(_pix x, _norm y)
{
    padding = { x.value, y.value };
    format = { Format::ePixel, Format::eNorm };
}

void trc::ui::Paddable::setPadding(_norm x, _pix y)
{
    padding = { x.value, y.value };
    format = { Format::eNorm, Format::ePixel };
}

void trc::ui::Paddable::setPadding(_norm x, _norm y)
{
    padding = { x.value, y.value };
    format = Format::eNorm;
}

auto trc::ui::Paddable::getPadding() const -> vec2
{
    return padding;
}

auto trc::ui::Paddable::getPaddingFormat() const -> _2D<Format>
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
