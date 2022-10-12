#include "trc/ui/Window.h"



namespace trc::ui
{

template<GuiElement E>
ElementHandleProxy<E>::ElementHandleProxy(E& element, Window& window)
    :
    element(&element),
    window(&window)
{
}

template<GuiElement E>
inline ElementHandleProxy<E>::operator E&() &&
{
    return *element;
}

template<GuiElement E>
inline auto ElementHandleProxy<E>::makeRef() && -> E&
{
    return *element;
}

template<GuiElement E>
inline auto ElementHandleProxy<E>::makeShared() && -> SharedHandle
{
    return SharedHandle(element, [window=window](E* elem) { window->destroy(*elem); });
}

template<GuiElement E>
inline auto ElementHandleProxy<E>::makeUnique() && -> UniqueHandle
{
    return UniqueHandle(element, [window=window](E* elem) { window->destroy(*elem); });
}




template<GuiElement E, typename... Args>
inline auto Window::create(Args&&... args) -> ElementHandleProxy<E>
{
    // Construct with Window in constructor if possible
    if constexpr (std::is_constructible_v<E, Window&, Args...>)
    {
        E& newElem = static_cast<E&>(
            *drawableElements.emplace_back(new E(*this, std::forward<Args>(args)...))
        );

        return { newElem, *this };
    }
    // Construct with args only and set window member later
    else
    {
        E& newElem = static_cast<E&>(
            *drawableElements.emplace_back(new E(std::forward<Args>(args)...))
        );
        newElem.window = this;

        return { newElem, *this };
    }
}

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
