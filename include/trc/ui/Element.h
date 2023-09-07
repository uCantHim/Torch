#pragma once

#include <concepts>

#include "trc/Types.h"
#include "trc/ui/CRTPNode.h"
#include "trc/ui/event/Event.h"
#include "trc/ui/event/EventListenerRegistryBase.h"
#include "trc/ui/event/InputEvent.h"
#include "trc/ui/Window.h"

namespace trc::ui
{
    class Element;

    template<typename ...EventTypes>
    struct InheritEventListener : public EventListenerRegistryBase<EventTypes>...
    {
        using EventListenerRegistryBase<EventTypes>::addEventListener...;
        using EventListenerRegistryBase<EventTypes>::removeEventListener...;
        using EventListenerRegistryBase<EventTypes>::notify...;
    };

    struct ElementEventBase : public InheritEventListener<
                                // Low-level events
                                event::KeyPress, event::KeyRelease,
                                event::CharInput,

                                // High-level user events
                                event::Click, event::Release,
                                event::Hover,
                                event::Input
                            >
    {
    };

    /**
     * Used internally by the window to store global transformations.
     * The transformation calculations are complex enough to justify
     * violating my rule against state in this regard.
     */
    struct GlobalTransformStorage
    {
    protected:
        friend class Window;

        vec2 globalPos;
        vec2 globalSize;
    };

    /**
     * @brief Base class of all UI elements
     *
     * Contains a reference to its parent window
     */
    class Element : public TransformNode<Element>
                  , public GlobalTransformStorage
                  , public Drawable
                  , public ElementEventBase
    {
    public:
        ElementStyle style;

        /**
         * @brief Create a UI element and attach it as a child
         */
        template<GuiElement E, typename ...Args>
        auto createChild(Args&&... args) -> E&
        {
            E& elem = window->create<E>(std::forward<Args>(args)...).makeRef();
            this->attach(elem);
            return elem;
        }

    protected:
        friend class Window;

        Element() = default;
        explicit Element(Window& window)
            : window(&window)
        {}

        Window* window;
    };

    /**
     * Typed unique handle for UI elements
     */
    template<GuiElement E>
    using SharedElementHandle = s_ptr<E>;

    /**
     * Typed shared handle for UI elements
     */
    template<GuiElement E>
    using UniqueElementHandle = u_ptr<E, std::function<void(E*)>>;

    /**
     * Temporary proxy that creates either an unmanaged reference or a
     * smart handle (i.e. unique or shared).
     */
    template<GuiElement E>
    class ElementHandleProxy
    {
        friend class Window;
        ElementHandleProxy(E& element, Window& window);

    public:
        using SharedHandle = SharedElementHandle<E>;
        using UniqueHandle = UniqueElementHandle<E>;

        ElementHandleProxy(const ElementHandleProxy<E>&) = delete;
        ElementHandleProxy(ElementHandleProxy<E>&&) noexcept = delete;
        auto operator=(const ElementHandleProxy<E>&) -> ElementHandleProxy<E>& = delete;
        auto operator=(ElementHandleProxy<E>&&) noexcept -> ElementHandleProxy<E>& = delete;

        inline operator E&() &&;
        inline auto makeRef() && -> E&;
        inline auto makeShared() && -> SharedHandle;
        inline auto makeUnique() && -> UniqueHandle;

    private:
        E* element;
        Window* window;
    };



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
} // namespace trc::ui
