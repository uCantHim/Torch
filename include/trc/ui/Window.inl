#include "Window.h"



template<trc::ui::GuiElement E>
trc::ui::ElementHandleFactory<E>::ElementHandleFactory(E& element, Window& window)
    :
    element(&element),
    window(&window)
{
}

template<trc::ui::GuiElement E>
inline trc::ui::ElementHandleFactory<E>::operator E&() &&
{
    return *element;
}

template<trc::ui::GuiElement E>
inline auto trc::ui::ElementHandleFactory<E>::makeRef() && -> E&
{
    return *element;
}

template<trc::ui::GuiElement E>
inline auto trc::ui::ElementHandleFactory<E>::makeShared() && -> SharedHandle
{
    return SharedHandle(element, [window=window](E* elem) { window->destroy(*elem); });
}

template<trc::ui::GuiElement E>
inline auto trc::ui::ElementHandleFactory<E>::makeUnique() && -> UniqueHandle
{
    return UniqueHandle(element, [window=window](E* elem) { window->destroy(*elem); });
}




template<trc::ui::GuiElement E, typename... Args>
    requires std::is_constructible_v<E, Args...>
inline auto trc::ui::Window::create(Args&&... args) -> ElementHandleFactory<E>
{
    E& newElem = static_cast<E&>(
        *drawableElements.emplace_back(new E(std::forward<Args>(args)...))
    );

    return { newElem, *this };
}

template<std::derived_from<trc::ui::event::MouseEvent> EventType>
void trc::ui::Window::descendEvent(EventType event)
{
    using FuncType = std::function<void(Element&, Transform)>;

    static constexpr auto isInside = [](const vec2 point, const Transform& t) -> bool {
        assert(t.posProp.type == SizeType::eNorm && t.sizeProp.type == SizeType::eNorm);

        const vec2 diff = point - t.position;
        return diff.x >= 0.0f
            && diff.y >= 0.0f
            && diff.x <= t.size.x
            && diff.y <= t.size.y;
    };

    /**
     * All elements in the tree are tested against the event, regardless
     * of whether their parent is eligible for the event or not. Therefore,
     * child elements can be outside of their parent's hitbox and still
     * receive click events.
     *
     * This approach gives the user full flexibility regarding element
     * positioning as no size constraints exist.
     */
    FuncType descend = [&, this](Element& e, Transform global)
    {
        if (isInside(event.mousePosNormal, global)) {
            e.notify(event);
        }
        if (event.isPropagationStopped()) {
            return;
        }

        e.foreachChild([&, this, global](Element& child)
        {
            const auto childTransform = concat(global, child.getTransform(), *this);
            descend(child, childTransform);
        });
    };

    if (isInside(event.mousePosNormal, root.getTransform())) {
        descend(root, root.getTransform());
    }
}

template<std::invocable<trc::ui::Element&, trc::vec2, trc::vec2> F>
inline void trc::ui::Window::traverse(F elemCallback)
{
    using FuncType = std::function<void(Transform, Element&)>;
    FuncType traverseElement = [&](Transform globalTransform, Element& node)
    {
        elemCallback(node, globalTransform.position, globalTransform.size);
        node.foreachChild([&, globalTransform](Element& child)
        {
            traverseElement(
                concat(globalTransform, child.getTransform(), *this),
                child
            );
        });
    };

    traverseElement(root.getTransform(), root);
}
