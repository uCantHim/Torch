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
        static inline const bool _init = []() {
            EventThread::registerHandler(pollEvents);
            return true;
        }();

        static inline std::mutex listenerListLock;
        static inline std::vector<ListenerEntry> listeners;

        static inline std::queue<EventType> eventQueue;
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
        std::lock_guard lock(listenerListLock);
        for (auto& listener : listeners)
        {
            listener.callback(event);
        }
    }

    template<typename EventType>
    auto EventHandler<EventType>::addListener(EventCallback newListener) -> ListenerId
    {
        std::lock_guard lock(listenerListLock);
        auto& newEntry = listeners.emplace_back(std::move(newListener));

        return &newEntry;
    }

    template<typename EventType>
    void EventHandler<EventType>::removeListener(ListenerId listener)
    {
        std::lock_guard lock(listenerListLock);
        listeners.erase(std::remove_if(
            listeners.begin(), listeners.end(),
            [&](ListenerEntry& entry) { return entry.id == listener->id; }
        ));
    }

    template<typename EventType>
    void EventHandler<EventType>::pollEvents()
    {
        std::lock_guard lock(listenerListLock);
        while (!eventQueue.empty())
        {
            const auto& event = eventQueue.front();
            for (auto& listener : listeners)
            {
                listener.callback(event);
            }

            eventQueue.pop();
        }
    }
} // namespace vkb
