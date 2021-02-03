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
        using EventListenerRegistryBase<EventTypes>::foreachEventListener...;
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
     * @brief Base class of all UI elements
     */
    class Element : public CRTPNode<Element>
                  , public Drawable
                  , public ElementEventBase
    {
    };

    template<typename T>
    concept GuiElement = requires {
        std::is_base_of_v<Element, T>;
    };
} // namespace trc::ui
