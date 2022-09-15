#include "trc/ui/elements/Button.h"

#include "trc/ui/Window.h"



trc::ui::Button::Button(Window& window, std::string label)
    :
    Quad(window),
    text(window.create<Text>())
{
    this->setSize(0.0f, 0.0f);
    text.style.background = vec4(0, 0, 0, 1);
    text.style.foreground = vec4(0, 0, 0, 1);
    text.setFontSize(40);
    attach(text);
    setLabel(std::move(label));
}

void trc::ui::Button::setLabel(std::string newLabel)
{
    text.print(std::move(newLabel));
}
