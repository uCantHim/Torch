#pragma once

#include <queue>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

namespace vkb
{
    class EventThread
    {
    public:
        static void start();
        static void terminate();

        static void registerHandler(std::function<void()> pollFunc);

    private:
        static inline bool shouldStop{ false };

        static inline std::thread thread;
        static inline std::mutex handlerListLock;
        static inline std::vector<std::function<void()>> handlers;
    };

    template<typename EventType, typename Derived>
    class EventListenerRegistryBase
    {
        friend Derived;

        /**
         * An event listener is defined as a function.
         */
        using ListenerFunc = std::function<void(const EventType&)>;

        /**
         * A listener entry. Associates a unique ID with an event listener
         * function.
         */
        struct ListenerEntry
        {
        public:
            friend EventListenerRegistryBase<EventType, Derived>;
            friend Derived;

            // Don't ask me why the constructor has to be public
            explicit ListenerEntry(ListenerFunc callback) : callback(std::move(callback)) {}

        private:
            ListenerFunc callback;
            uint32_t id{ ++nextId };
            static inline std::atomic<uint32_t> nextId;
        };

    public:
        using ListenerId = const ListenerEntry*;

        static inline auto addListener(ListenerFunc newListener) -> ListenerId
        {
            std::lock_guard lock(listenerListLock);
            return &listeners.emplace_back(std::move(newListener));
        }

        static inline void removeListener(ListenerId listener)
        {
            std::lock_guard lock(listenerListLock);
            listeners.erase(std::remove_if(
                listeners.begin(), listeners.end(),
                [&](ListenerEntry& entry) { return entry.id == listener->id; }
            ));
        }

    protected:
        static inline auto acquireListenerListLock() -> std::unique_lock<std::mutex>
        {
            return std::unique_lock(listenerListLock);
        }

        static inline std::mutex listenerListLock;
        static inline std::vector<ListenerEntry> listeners;
    };

    template<typename EventType>
    class EventHandler : public EventListenerRegistryBase<EventType, EventHandler<EventType>>
    {
        using Parent = EventListenerRegistryBase<EventType, EventHandler<EventType>>;

    public:
        using ListenerFunc = typename Parent::ListenerFunc;
        using ListenerId = typename Parent::ListenerId;

        static void notify(EventType event);
        static void notifySync(EventType event);

    private:
        static void pollEvents();
        static inline const bool _init = []() {
            EventThread::registerHandler(pollEvents);
            return true;
        }();

        static inline std::queue<EventType> eventQueue;
    };

    /**
     * @brief Helper to unregister a listener when the handle is destroyed
     *
     * Implicitly constructible from ListenerId types.
     *
     * @tparam EventType Type of event that the listener listens to.
     */
    template<typename EventType>
    class UniqueListenerId
    {
    public:
        using IdType = typename EventHandler<EventType>::ListenerId;

        UniqueListenerId() = default;
        UniqueListenerId(IdType id)
            :
            _id(new IdType(id), [](IdType* oldId) {
                EventHandler<EventType>::removeListener(*oldId);
                delete oldId;
            })
        {}

    private:
        std::unique_ptr<IdType, std::function<void(IdType*)>> _id;
    };



    template<typename EventType>
    void EventHandler<EventType>::notify(EventType event)
    {
        [[maybe_unused]]
        static bool _assert_init = _init;

        eventQueue.emplace(std::move(event));
    }

    template<typename EventType>
    void EventHandler<EventType>::notifySync(EventType event)
    {
        auto lock = Parent::acquireListenerListLock();
        for (auto& listener : Parent::listeners)
        {
            listener.callback(event);
        }
    }

    template<typename EventType>
    void EventHandler<EventType>::pollEvents()
    {
        auto lock = Parent::acquireListenerListLock();
        while (!eventQueue.empty())
        {
            const auto& event = eventQueue.front();
            for (auto& listener : Parent::listeners)
            {
                listener.callback(event);
            }

            eventQueue.pop();
        }
    }
} // namespace vkb
