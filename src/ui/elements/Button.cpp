#include "trc/ui/elements/Button.h"

#include "trc/ui/Window.h"



trc::ui::Button::Button(std::string label)
{
    setLabel(std::move(label));
}

void trc::ui::Button::draw(DrawList& list)
{
    auto [text, size] = layoutText(label, fontIndex, window->pixelsToNorm(vec2(fontSize)));
    const vec2 normPadding = getNormPadding(size, *window);

    globalSize = size + normPadding * 2.0f;
    Quad::draw(list);

    // Draw text
    list.push_back(DrawInfo{
        .pos = globalPos + normPadding,
        .size = globalSize,
        .style = style,
        .type = std::move(text)
    });
}

void trc::ui::Button::setLabel(std::string newLabel)
{
    label = std::move(newLabel);
}
