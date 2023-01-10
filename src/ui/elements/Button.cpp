#include "trc/ui/elements/Button.h"

#include "trc/ui/Window.h"



trc::ui::Button::Button(Window& window, std::string label)
    :
    Quad(window),
    text(window.makeUnique<Text>())
{
    this->style.dynamicSize = true;
    text->style.background = vec4(0, 0, 0, 1);
    text->style.foreground = vec4(0, 0, 0, 1);
    attach(*text);
    setLabel(std::move(label));
}

void trc::ui::Button::setLabel(std::string newLabel)
{
    text->print(std::move(newLabel));
}
