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
inline auto trc::ui::ElementHandleFactory<E>::makeShared() && -> SharedHandle
{
    return std::shared_ptr(element, [window=window](E* elem) { window->destroy(*elem); });
}

template<trc::ui::GuiElement E>
inline auto trc::ui::ElementHandleFactory<E>::makeUnique() && -> UniqueHandle
{
    return std::unique_ptr(element, [window=window](E* elem) { window->destroy(*elem); });
}



template<trc::ui::GuiElement E, typename... Args>
    requires std::is_constructible_v<E, Args...>
auto trc::ui::Window::create(Args&&... args) -> ElementHandleFactory<E>
{
    E& newElem = drawableElements.emplace_back(new E(std::forward<Args>(args)...));
    return { newElem, *this };
}
