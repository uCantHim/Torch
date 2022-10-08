#pragma once

#include <vector>

#include "trc/Types.h"
#include "trc/ui/CRTPNode.h"
#include "trc/ui/DrawInfo.h"
#include "trc/ui/event/Event.h"
#include "trc/ui/event/EventListenerRegistryBase.h"
#include "trc/ui/event/InputEvent.h"

namespace trc::ui
{
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

    protected:
        friend class Window;

        Element() = default;
        explicit Element(Window& window)
            : window(&window)
        {}

        Window* window;
    };

    template<typename T>
    concept GuiElement = requires {
        std::is_base_of_v<Element, T>;
    };
} // namespace trc::ui
