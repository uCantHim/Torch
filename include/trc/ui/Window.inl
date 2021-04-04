#include "Window.h"



template<trc::ui::GuiElement E>
trc::ui::ElementHandleProxy<E>::ElementHandleProxy(E& element, Window& window)
    :
    element(&element),
    window(&window)
{
}

template<trc::ui::GuiElement E>
inline trc::ui::ElementHandleProxy<E>::operator E&() &&
{
    return *element;
}

template<trc::ui::GuiElement E>
inline auto trc::ui::ElementHandleProxy<E>::makeRef() && -> E&
{
    return *element;
}

template<trc::ui::GuiElement E>
inline auto trc::ui::ElementHandleProxy<E>::makeShared() && -> SharedHandle
{
    return SharedHandle(element, [window=window](E* elem) { window->destroy(*elem); });
}

template<trc::ui::GuiElement E>
inline auto trc::ui::ElementHandleProxy<E>::makeUnique() && -> UniqueHandle
{
    return UniqueHandle(element, [window=window](E* elem) { window->destroy(*elem); });
}




template<trc::ui::GuiElement E, typename... Args>
    requires std::is_constructible_v<E, Args...>
inline auto trc::ui::Window::create(Args&&... args) -> ElementHandleProxy<E>
{
    E& newElem = static_cast<E&>(
        *drawableElements.emplace_back(new E(std::forward<Args>(args)...))
    );

    return { newElem, *this };
}

template<std::derived_from<trc::ui::event::MouseEvent> EventType>
void trc::ui::Window::descendMouseEvent(EventType event)
{
    static constexpr auto isInside = [](const vec2 point, const Transform& t) -> bool {
        assert(t.posProp.type == SizeType::eNorm && t.sizeProp.type == SizeType::eNorm);

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
    std::derived_from<trc::ui::event::EventBase> EventType,
    typename F
>
void trc::ui::Window::descendEvent(EventType event, F breakCondition)
    requires std::is_same_v<bool, std::invoke_result_t<F, trc::ui::Element&>>
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

template<std::invocable<trc::ui::Element&, trc::vec2, trc::vec2> F>
inline void trc::ui::Window::traverse(F elemCallback)
{
    std::function<void(Element&)> traverseElement = [&](Element& node)
    {
        elemCallback(node, node.globalPos, node.globalSize);
        node.foreachChild(traverseElement);
    };

    traverseElement(*root);
}
