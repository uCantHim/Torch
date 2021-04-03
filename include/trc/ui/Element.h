#pragma once

#include <vector>

#include "Types.h"
#include "CRTPNode.h"
#include "DrawInfo.h"
#include "event/Event.h"
#include "event/EventListenerRegistryBase.h"

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
                                event::Click,
                                event::Release,
                                event::Hover
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
    private:
        friend class Window;

        vec2 globalPos;
        vec2 globalSize;
    };

    /**
     * @brief Base class of all UI elements
     */
    class Element : public TransformNode<Element>
                  , public GlobalTransformStorage
                  , public Drawable
                  , public ElementEventBase
    {
    public:
        ElementStyle style;
    };

    template<typename T>
    concept GuiElement = requires {
        std::is_base_of_v<Element, T>;
    };
} // namespace trc::ui
