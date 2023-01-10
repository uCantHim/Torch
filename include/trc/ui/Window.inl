#include "trc/ui/Window.h"



namespace trc::ui
{

template<GuiElement E, typename... Args>
inline auto Window::make(Args&&... args) -> E*
{
    static_assert(std::is_constructible_v<E, Window&, Args...>);
    // Construct with Window in constructor if possible
    if constexpr (std::is_constructible_v<E, Window&, Args...>)
    {
        E& newElem = static_cast<E&>(
            *drawableElements.emplace_back(new E(*this, std::forward<Args>(args)...))
        );

        return &newElem;
    }
    // Construct with args only and set window member later
    else
    {
        E& newElem = static_cast<E&>(
            *drawableElements.emplace_back(new E(std::forward<Args>(args)...))
        );
        newElem.window = this;

        return &newElem;
    }
}

template<GuiElement E, typename... Args>
inline auto Window::makeUnique(Args&&... args) -> UniqueElement<E>
{
    auto elem = make<E>(std::forward<Args>(args)...);
    return UniqueElement<E>{ elem, _ElementDeleter<E>{ this } };
}

template<GuiElement E, typename... Args>
inline auto Window::makeShared(Args&&... args) -> SharedElement<E>
{
    auto elem = make<E>(std::forward<Args>(args)...);
    return SharedElement<E>{ elem, _ElementDeleter<E>{ this } };
}

template<std::derived_from<event::MouseEvent> EventType>
void Window::descendMouseEvent(EventType event)
{
    static constexpr auto isInside = [](const vec2 point, const Transform& t) -> bool {
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
