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
                concat(globalTransform, child.getTransform()),
                child
            );
        });
    };

    traverseElement(root.getTransform(), root);
}
