#include "trc/ui/elements/BaseElements.h"



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
