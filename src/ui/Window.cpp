#include "trc/ui/Window.h"

#include <ranges>
#include <iostream>

#include "trc/ui/Element.h"



/**
 * @brief An element that does nothing.
 */
struct Root : trc::ui::Element {
    void draw(trc::ui::DrawList&) override {}
};

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
    root(new Root{})
{
    assert(this->windowBackend != nullptr);

    ioConfig.keyMap = createInfo.keyMap;
}

trc::ui::Window::~Window()
{
    onWindowDestruction(*this);
}

auto trc::ui::Window::draw() -> const DrawList&
{
    realignElements();

    drawList.clear();
    traverse([this](Element& elem) {
        elem.draw(drawList);
    });

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
    calcTransform(concat({}, root->getTransform(), *this), *root);
}

namespace trc::ui
{

template<std::derived_from<event::MouseEvent> EventType>
void Window::descendMouseEvent(EventType event)
{
    static constexpr auto isInside = [](const vec2 point, const Transform& t) -> bool {
        assert((t.posProp.format == _2D<Format>{ Format::eNorm, Format::eNorm }));
        assert((t.sizeProp.format == _2D<Format>{ Format::eNorm, Format::eNorm }));

        const vec2 diff = point - t.position;
        return diff.x >= 0.0f
            && diff.y >= 0.0f
            && diff.x <= t.size.x
            && diff.y <= t.size.y;
    };

    descendEvent(event, [event](Element& e) {
        return isInside(event.mousePosNormal, { e.globalPos, e.globalSize });
    });
}

template<
    std::derived_from<event::EventBase> EventType,
    typename F
>
void Window::descendEvent(EventType event, F breakCondition)
    requires std::is_same_v<bool, std::invoke_result_t<F, Element&>>
{
    std::function<void(Element&)> descend = [&](Element& e)
    {
        if (!breakCondition(e)) return;
        if (event.isPropagationStopped()) return;

        e.notify(event);
        e.foreachChild(descend);
    };

    descend(*root);
}

template<std::invocable<Element&> F>
void Window::traverse(F elemCallback)
{
    std::function<void(Element&)> traverseElement = [&](Element& node)
    {
        elemCallback(node);
        node.foreachChild(traverseElement);
    };

    traverseElement(*root);
}

} // namespace trc::ui
