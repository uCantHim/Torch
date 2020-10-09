#pragma once

/**
 * Convenience header that includes all common event related headers
 */

#include "EventHandler.h"

#include "InputEvents.h"
#include "WindowEvents.h"

#include "Keys.h"
#include "InputState.h"

namespace vkb
{
    /**
     * @brief A wrapper around ListenerIds.
     *
     * Conveniently decide whether to create a unique listener handle
     * or to keep/destroy the non-managing default handle.
     */
    template<typename EventType>
    class MaybeUniqueListener
    {
    public:
        using IdType = typename EventHandler<EventType>::ListenerId;

        inline MaybeUniqueListener(IdType id) : id(id) {}

        /**
         * Allow convenient conversion to the default handle type.
         */
        inline operator IdType() {
            return id;
        }

        /**
         * Allow convenient conversion to the unique handle type.
         */
        inline operator UniqueListenerId<EventType>() {
            return makeUnique();
        }

        /**
         * @brief Create a unique handle from the stored non-unique
         *        listener handle.
         *
         * @return UniqueListenerId<EventType>
         */
        inline auto makeUnique() -> UniqueListenerId<EventType> {
            return { id };
        }

    private:
        IdType id;
    };

    template<typename EventType>
    inline auto on(std::function<void(const EventType&)> callback) -> MaybeUniqueListener<EventType>
    {
        return { EventHandler<EventType>::addListener(std::move(callback)) };
    }
}
