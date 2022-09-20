#include "trc/ui/Window.h"

#include <ranges>
#include <iostream>



void trc::ui::initUserCallbacks(
    std::function<void(trc::basic_types::ui32, const GlyphCache&)> onFontLoad,
    std::function<void(trc::basic_types::ui32)>)
{
    FontRegistry::setFontAddCallback(std::move(onFontLoad));
}



trc::ui::Window::Window(WindowCreateInfo createInfo)
    :
    onWindowDestruction(std::move(createInfo.onWindowDestruction)),
    windowBackend(std::move(createInfo.windowBackend)),
    drawList(*this)
{
    assert(this->windowBackend != nullptr);

    ioConfig.keyMap = createInfo.keyMap;
}

trc::ui::Window::~Window()
{
    isDuringDelete = true;
    onWindowDestruction(*this);
}

auto trc::ui::Window::draw() -> const DrawList&
{
    realignElements();

    drawList.clear();
    std::function<void(Element&)> drawRecursive = [&](Element& elem)
    {
        if (elem.style.restrictDrawArea)
        {
            drawList.pushScissorRect(
                { { elem.globalPos.x, Format::eNorm }, { elem.globalPos.y, Format::eNorm } },
                { { elem.globalSize.x, Format::eNorm }, { elem.globalSize.y, Format::eNorm } }
            );
        }

        elem.draw(drawList);
        elem.foreachChild(drawRecursive);

        if (elem.style.restrictDrawArea) {
            drawList.popScissorRect();
        }
    };

    drawRecursive(*root);

    return drawList;
}

auto trc::ui::Window::getSize() const -> vec2
{
    return windowBackend->getSize();
}

auto trc::ui::Window::getRoot() -> Element&
{
    return *root;
}

void trc::ui::Window::destroy(Element& elem)
{
    if (isDuringDelete) return;
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

void trc::ui::Window::signalKeyPress(int key)
{
    ioConfig.keysDown[key] = true;

    event::KeyPress event(key);
    traverse([&](Element& e) { e.notify(event); });
}

void trc::ui::Window::signalKeyRepeat(int key)
{
    signalKeyPress(key);
}

void trc::ui::Window::signalKeyRelease(int key)
{
    ioConfig.keysDown[key] = false;

    event::KeyRelease event(key);
    traverse([&](Element& e) { e.notify(event); });
}

void trc::ui::Window::signalCharInput(ui32 character)
{
    event::CharInput event(character);
    traverse([&](Element& e) { e.notify(event); });
}

auto trc::ui::Window::getIoConfig() -> IoConfig&
{
    return ioConfig;
}

auto trc::ui::Window::getIoConfig() const -> const IoConfig&
{
    return ioConfig;
}

auto trc::ui::Window::normToPixels(vec2 p) const -> vec2
{
    return glm::floor(p * windowBackend->getSize());
}

auto trc::ui::Window::pixelsToNorm(vec2 p) const -> vec2
{
    return p / windowBackend->getSize();
}

void trc::ui::Window::realignElements()
{
    using FuncType = std::function<std::pair<vec2, vec2>(Transform, Element&)>;
    FuncType calcTransform = [&](Transform parentTransform, Element& elem)
        -> std::pair<vec2, vec2>
    {
        vec2 pos = parentTransform.position;
        vec2 size = parentTransform.size;

        if (elem.style.dynamicSize) {
            size = vec2(0.0f);
        }

        // Apply padding
        const vec2 padding = elem.style.padding.calcNormalizedPadding(*this);

        elem.foreachChild([&, parentTransform](Element& child)
        {
            auto childTransform = concat(parentTransform, child.getTransform(), *this);
            childTransform.position += padding;
            auto [childPos, childSize] = calcTransform(childTransform, child);

            if (elem.style.dynamicSize) {
                size = glm::max(size, (childPos - pos) + (childSize + padding));
            }
        });

        return { (elem.globalPos = pos), (elem.globalSize = size) };
    };

    // concat once to ensure that globalTransform is normalized
    calcTransform(concat({}, root->getTransform(), *this), *root);
}
