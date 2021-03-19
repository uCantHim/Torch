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
    realignElements();

    drawList.clear();
    traverse([this](Element& elem, vec2 globalPos, vec2 globalSize) {
        elem.draw(drawList, globalPos, globalSize);
    });

    return drawList;
}

auto trc::ui::Window::getSize() const -> vec2
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

void trc::ui::Window::signalMouseClick(float posPixelsX, float posPixelsY)
{
    event::Click event;
    event.mousePosPixels = vec2{ posPixelsX, posPixelsY };
    event.mousePosNormal = vec2{ posPixelsX, posPixelsY } / getSize();

    descendMouseEvent(event);
}

void trc::ui::Window::realignElements()
{
    using FuncType = std::function<std::pair<vec2, vec2>(Transform, Element&)>;
    FuncType calcTransform = [&](Transform globalTransform, Element& elem)
        -> std::pair<vec2, vec2>
    {
        vec2 pos = globalTransform.position;
        vec2 size = globalTransform.size;
        elem.foreachChild([&, globalTransform](Element& child)
        {
            auto [cPos, cSize] = calcTransform(
                concat(globalTransform, child.getTransform(), *this),
                child
            );
            pos = glm::min(pos, cPos);
            size = glm::max(size, cPos - globalTransform.position + cSize);
        });

        return { (elem.globalPos = pos), (elem.globalSize = size) };
    };

    // concat once to ensure that globalTransform is normalized
    calcTransform(concat({}, root.getTransform(), *this), root);
}
