#pragma once

#include <concepts>
#include <functional>

#include <componentlib/Table.h>
#include <trc_util/data/IdPool.h>

namespace trc
{
    /**
     * The constructor is disabled. Use `EventDispatcher<>::make()` to create
     * instances of this class.
     */
    template<typename EventType>
    class EventDispatcher
    {
    private:
        using ListenerID = std::uintptr_t;

        struct Nothing_ {};
        struct HandleDeleter
        {
            void operator()(Nothing_* _id)
            {
                const auto id = reinterpret_cast<ListenerID>(_id);
                if (auto parent = dispatcher.lock())
                {
                    parent->listeners.erase(id);
                    parent->idPool.free(id);
                }
                // Don't delete anything!
            }
            std::weak_ptr<EventDispatcher> dispatcher;
        };

    public:
        using ListenerFunc = std::function<void(const EventType&)>;
        using ListenerHandle = std::unique_ptr<Nothing_, HandleDeleter>;

        /**
         * @brief Create an EventDispatcher object.
         */
        static auto make() -> std::shared_ptr<EventDispatcher>
        {
            std::shared_ptr<EventDispatcher> obj{ new EventDispatcher };
            obj->self = obj;
            return obj;
        }

        /**
         * @brief Notify all listeners that an event has occured.
         */
        void notify(const EventType& event)
        {
            for (ListenerFunc& listener : listeners) {
                listener(event);
            }
        }

        /**
         * @brief Register an event listener.
         *
         * @return A handle to the newly registered event listener. The listener
         *         is removed from the EventDispatcher when the handle is
         *         destroyed.
         *         Note: A handle *can* outlive the event dispatcher.
         */
        template<std::invocable<const EventType&> F>
        [[nodiscard]]
        auto registerListener(F&& listener) -> ListenerHandle
        {
            auto id = idPool.generate();
            listeners.emplace(id, std::forward<F>(listener));

            // With a little hack (it's not actually hacky, it just looks that way),
            // we can avoid dynamic memory allocation entirely. We cast the
            // pointer back to ListenerID in the unique_ptr's destructor and use
            // it to index the `listeners` table.
            return { reinterpret_cast<Nothing_*>(ListenerID{id}), HandleDeleter{ self } };
        }

    private:
        using TableImpl = componentlib::IndirectTableImpl<ListenerFunc, uint32_t>;

        EventDispatcher() = default;

        std::weak_ptr<EventDispatcher> self;

        trc::data::IdPool<ListenerID> idPool;
        componentlib::Table<ListenerFunc, uint32_t, TableImpl> listeners;
    };

    template<typename EventType>
    using EventListener = EventDispatcher<EventType>::ListenerHandle;
} // namespace trc
