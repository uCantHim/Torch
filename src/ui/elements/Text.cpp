#include "trc/ui/elements/Text.h"



namespace trc::ui
{

Text::Text(Window& window)
    :
    Text(window, "")
{
}

Text::Text(Window& window, std::string str, ui32 fontIndex, ui32 fontSize)
    :
    Element(window),
    textElem(window.create<Letters>())
{
    style.fontIndex = fontIndex;
    style.fontSize = fontSize;
    style.dynamicSize = true;

    textElem->setSize(0.0f, getFontLayouter().getFontHeight(fontIndex));
    textElem->setSizing(Format::eNorm, Scale::eAbsolute);
    attach(*textElem);

    print(std::move(str));
}

void Text::print(std::string str)
{
    printedText = std::move(str);

    vec2 scaling = window->pixelsToNorm(vec2(style.fontSize));
    auto [text, size] = getFontLayouter().layoutText(printedText, style.fontIndex, scaling);

    textElem->setSize(size);
    textElem->setText(std::move(text));
}

auto Text::getFontLayouter() -> FontLayouter&
{
    return window->getFontLayouter();
}

Text::Letters::Letters(Window& window)
    :
    Element(window)
{
}

void Text::Letters::draw(DrawList& drawList)
{
    drawList.push(text, *this);
}

void Text::Letters::setText(types::Text newText)
{
    text = std::move(newText);
}

} // namespace trc::ui
