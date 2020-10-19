#pragma once

/**
 * Includes all common event-related headers.
 *
 * Also defines some convenience functions to deal with events more
 * expressively.
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
     *
     * Is implicitly castable to either the default- or the unique handle.
     *
     * Object of this class are not meant to be stored, so all methods can
     * can only be used on r-values of MaybeUniqueListener.
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
        inline operator IdType() && {
            return id;
        }

        /**
         * Allow convenient conversion to the unique handle type.
         */
        inline operator UniqueListenerId<EventType>() && {
            // Can't use makeUnique() here because the rvalue qualifer is discarded
            return { id };
        }

        /**
         * @brief Create a unique handle from the stored non-unique
         *        listener handle.
         *
         * @return UniqueListenerId<EventType>
         */
        inline auto makeUnique() && -> UniqueListenerId<EventType> {
            return { id };
        }

    private:
        IdType id;
    };

    /**
     * @brief Conveniently add an event listener
     *
     * @tparam EventType The type of event that the listener listens to.
     *                   Cannot be deduced in most cases.
     *
     * Template argument deduction doesn't quite work here, though I think
     * that explicitly stating the argument is actually more expressive:
     *
     *     on<SwapchainResizeEvent>([](const auto& e) {
     *         // ...
     *     });
     *
     * The return type MaybeUniqueListener allows you to decide quite
     * intuitively if you want to get a unique handle to the created
     * listener or just a plain handle that you have to call delete on
     * yourself. In order to create a permanent listener that may never be
     * destroyed, just throw away the result.
     *
     * @return MaybeUniqueListener<EventType>
     */
    template<typename EventType>
    inline auto on(std::function<void(const EventType&)> callback) -> MaybeUniqueListener<EventType>
    {
        return { EventHandler<EventType>::addListener(std::move(callback)) };
    }

    /**
     * @brief Fire an event
     *
     * @tparam EventType Type of event fired. Can be deduced by the
     *                   compiler.
     */
    template<typename EventType>
    inline void fire(EventType event)
    {
        EventHandler<EventType>::notify(std::move(event));
    }

    /**
     * @brief Construct and fire an event
     *
     * emplace-like overload of the fire template.
     *
     * @tparam EventType Type of event fired. Can obviously not be deduced
     *                   in this case.
     * @tparam ...Args   Argument types to the constructor of EventType.
     *                   Can be deduced.
     */
    template<typename EventType, typename... Args>
    inline void fire(Args&&... args)
        requires (std::is_constructible_v<EventType, Args...>)
    {
        EventHandler<EventType>::notify(EventType(std::forward<Args>(args)...));
    }

    /**
     * @brief Fire an event synchronously
     *
     * @tparam EventType Type of event fired. Can be deduced by the
     *                   compiler.
     *
     * All registered listeners for the event are invoked synchronously
     * in the same thread that this function is called in.
     */
    template<typename EventType>
    inline void fireSync(EventType event)
    {
        EventHandler<EventType>::notifySync(std::move(event));
    }

    /**
     * @brief Construct and fire an event synchronously
     *
     * emplace-like overload of the fireSync template.
     *
     * @tparam EventType Type of event fired. Can obviously not be deduced
     *                   in this case.
     * @tparam ...Args   Argument types to the constructor of EventType.
     *                   Can be deduced.
     *
     * All registered listeners for the event are invoked synchronously
     * in the same thread that this function is called in.
     */
    template<typename EventType, typename... Args>
    inline void fireSync(Args&&... args)
        requires (std::is_constructible_v<EventType, Args...>)
    {
        EventHandler<EventType>::notifySync(EventType(std::forward<Args>(args)...));
    }
}
