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

    template<typename EventType>
    class EventHandler
    {
        struct ListenerEntry
        {
        public:
            template<typename T>
            friend class EventHandler;

            // Don't ask me why the constructor has to be public
            ListenerEntry(std::function<void(const EventType&)> callback)
                : callback(std::move(callback)) {}

        private:
            std::function<void(const EventType&)> callback;
            uint32_t id{ ++nextId };
            static inline std::atomic<uint32_t> nextId;
        };

    public:
        using EventCallback = std::function<void(const EventType&)>;
        using ListenerId = const ListenerEntry*;

        static void notify(EventType event);
        static void notifySync(EventType event);

        static auto addListener(EventCallback newListener) -> ListenerId;
        static void removeListener(ListenerId listener);

    private:
        static void pollEvents();
        static void updateListeners();
        static inline const bool _init = []() {
            EventThread::registerHandler(pollEvents);
            return true;
        }();

        static inline std::mutex listenerListLock;
        static inline std::vector<std::unique_ptr<ListenerEntry>> listeners;

        static inline std::mutex newListenersLock;
        static inline std::vector<std::unique_ptr<ListenerEntry>> newListeners;
        static inline std::mutex removedListenersLock;
        static inline std::vector<ListenerId> removedListeners;

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
        updateListeners();

        std::lock_guard lock(listenerListLock);
        for (auto& listener : listeners)
        {
            listener->callback(event);
        }
    }

    template<typename EventType>
    auto EventHandler<EventType>::addListener(EventCallback newListener) -> ListenerId
    {
        std::lock_guard lock(newListenersLock);
        return newListeners.emplace_back(new ListenerEntry(std::move(newListener))).get();
    }

    template<typename EventType>
    void EventHandler<EventType>::removeListener(ListenerId listener)
    {
        std::lock_guard lock(removedListenersLock);
        removedListeners.emplace_back(listener);
    }

    template<typename EventType>
    void EventHandler<EventType>::pollEvents()
    {
        updateListeners();

        // Now poll events
        std::lock_guard lock(listenerListLock);
        while (!eventQueue.empty())
        {
            const auto& event = eventQueue.front();
            for (auto& listener : listeners)
            {
                listener->callback(event);
            }

            eventQueue.pop();
        }
    }

    template<typename EventType>
    void EventHandler<EventType>::updateListeners()
    {
        // Add and remove listeners now in order to avoid deadlocks when
        // adding a listener inside of another listener's callback
        if (!newListeners.empty() || !removedListeners.empty())
        {
            std::unique_lock lock(listenerListLock, std::defer_lock);
            std::unique_lock lock_(newListenersLock, std::defer_lock);
            std::unique_lock lock__(removedListenersLock, std::defer_lock);
            std::lock(lock, lock_, lock__);

            // Add any new listeners
            while (!newListeners.empty())
            {
                listeners.emplace_back(std::move(newListeners.back()));
                newListeners.pop_back();
            }

            // Remove any old listeners
            while (!removedListeners.empty())
            {
                auto it = std::remove_if(
                    listeners.begin(), listeners.end(),
                    [&](auto& entry) { return entry->id == removedListeners.back()->id; }
                );
                if (it != listeners.end())
                {
                    listeners.erase(it);
                    removedListeners.pop_back();
                }
            }
        }
    }
} // namespace vkb
