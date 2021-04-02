#pragma once

#include <queue>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

namespace trc::ui
{
    template<typename EventType>
    class EventListenerRegistryBase
    {
    public:
        /**
         * An event listener is defined as a function.
         */
        using EventListener = std::function<void(EventType&)>;

    //private:
        /**
         * A listener entry. Associates a unique ID with an event listener
         * function.
         */
        struct ListenerEntry
        {
        public:
            friend EventListenerRegistryBase<EventType>;

            // Don't ask me why the constructor has to be public
            explicit ListenerEntry(EventListener callback) : callback(std::move(callback)) {}

        private:
            EventListener callback;
            uint32_t id{ ++nextId };
            static inline std::atomic<uint32_t> nextId;
        };

    public:
        using ListenerId = const ListenerEntry*;

        inline void notify(EventType& event)
        {
            std::lock_guard lock(listenerListLock);
            for (auto& listener : listeners)
            {
                listener.callback(event);
            }
        }

        /**
         * @brief Add an event listener
         *
         * Can be invoked like this:
         * ```cpp
         * element.addEventListener([](const ui::event::Click& event) {});
         * ```
         */
        inline auto addEventListener(EventListener newListener) -> ListenerId
        {
            std::lock_guard lock(listenerListLock);
            return &listeners.emplace_back(std::move(newListener));
        }

        inline void removeEventListener(ListenerId listener)
        {
            std::lock_guard lock(listenerListLock);

            auto it = std::remove_if(
                listeners.begin(), listeners.end(),
                [&](ListenerEntry& entry) { return entry.id == listener->id; }
            );
            if (it != listeners.end()) {
                listeners.erase(it);
            }
        }

    protected:
        std::mutex listenerListLock;
        std::vector<ListenerEntry> listeners;
    };
} // namespace trc::ui
