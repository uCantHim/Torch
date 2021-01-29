#include "ui/Window.h"

#include <ranges>
#include <iostream>



trc::ui::Window::Window(WindowCreateInfo createInfo)
    :
    windowInfo(std::move(createInfo.windowProvider))
{
    assert(this->windowInfo != nullptr);
}

auto trc::ui::Window::draw() -> const DrawList&
{
    drawList.clear();

    traverse([this](Element& elem, vec2 globalPos, vec2 globalSize) {
        elem.draw(drawList, globalPos, globalSize);
    });

    return drawList;
}

auto trc::ui::Window::getSize() -> vec2
{
    return windowInfo->getSize();
}

auto trc::ui::Window::getRoot() -> Element&
{
    return root;
}

void trc::ui::Window::destroy(Element& elem)
{
    drawableElements.erase(std::remove_if(
        drawableElements.begin(), drawableElements.end(),
        [&elem](u_ptr<Element>& e) { return e.get() == &elem; }
    ));
}

auto trc::ui::Window::concat(const Transform parent, Transform child) -> Transform
{
    const vec2 windowSize = getSize();
    if (child.posProp.type == SizeType::ePixel) {
        child.position /= windowSize;
    }
    if (child.sizeProp.type == SizeType::ePixel) {
        child.position /= windowSize;
    }

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
