#pragma once

#include <atomic>
#include <algorithm>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include <trc_util/data/DeferredInsertVector.h>
#include <trc_util/data/ThreadsafeQueue.h>

#include "trc/base/InputProcessor.h"

namespace trc
{
    class Swapchain;

    /**
     * @brief An input processor that generates events at the `EventHandler`
     */
    class InputEventSpawner : public InputProcessor
    {
    public:
        void onCharInput(Swapchain&, uint32_t charcode) override;
        void onKeyInput(Swapchain&, Key key, InputAction action, KeyModFlags mods) override;
        void onMouseInput(Swapchain&, MouseButton button, InputAction action, KeyModFlags mods) override;
        void onMouseMove(Swapchain&, double x, double y) override;
        void onMouseScroll(Swapchain&, double xOffset, double yOffset) override;
        void onWindowResize(Swapchain&, uint newX, uint newY) override;
        void onWindowClose(Swapchain&) override;
    };

    class EventThread
    {
    public:
        static void start();
        static void terminate();

    private:
        template<typename T>
        friend class EventHandler;

        /**
         * Registers a poll function at the event thread which will be
         * called exactly once.
         */
        static void notifyActiveHandler(void(*pollFunc)());

    private:
        static inline bool shouldStop{ false };
        static inline std::thread thread;

        static inline trc::data::ThreadsafeQueue<void(*)(void)> pollFuncs;
    };

    template<typename EventType>
    class EventHandler
    {
    public:
        using ListenerEntry = std::function<void(const EventType&)>;
        using ListenerId = const ListenerEntry*;

        static void notify(EventType event);

        static auto addListener(std::function<void(const EventType&)> newListener) -> ListenerId;
        static void removeListener(ListenerId listener);

    private:
        static void pollEvents();

        static inline std::atomic_flag isBeingPolled = ATOMIC_FLAG_INIT;

        static inline trc::data::DeferredInsertVector<std::unique_ptr<ListenerEntry>> listeners;
        static inline std::mutex removedListenersListLock;
        static inline std::vector<ListenerId> removedListeners;

        static inline trc::data::ThreadsafeQueue<EventType> eventQueue;
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

        explicit UniqueListenerId(IdType id)
            :
            _id(new IdType(id), [](IdType* oldId) {
                EventHandler<EventType>::removeListener(*oldId);
                delete oldId;
            })
        {}

    private:
        std::unique_ptr<IdType, void(IdType*)> _id;
    };



    template<typename EventType>
    void EventHandler<EventType>::notify(EventType event)
    {
        if (listeners.empty() && listeners.none_pending()) return;

        // Add event to queue
        eventQueue.emplace(std::move(event));

        // Notify the event thread that the handler has new events and is
        // ready to be processed
        if (!isBeingPolled.test_and_set()) {
            EventThread::notifyActiveHandler(pollEvents);
        }
    }

    template<typename EventType>
    auto EventHandler<EventType>::addListener(std::function<void(const EventType&)> newListener)
        -> ListenerId
    {
        auto listener = std::make_unique<ListenerEntry>(std::move(newListener));
        auto ptr = listener.get();
        listeners.emplace_back(std::move(listener));

        return ptr;
    }

    template<typename EventType>
    void EventHandler<EventType>::removeListener(ListenerId listener)
    {
        // Can't use the deferred insert vector directly for removes because
        // I'd have to search it for the removed listener's index, which
        // would require a lock on the vector. That means I'd still get a
        // deadlock if `removeListener` was called from another listener.
        std::scoped_lock lock(removedListenersListLock);
        removedListeners.emplace_back(listener);
    }

    template<typename EventType>
    void EventHandler<EventType>::pollEvents()
    {
        {
            auto range = listeners.iter();
            std::scoped_lock lock(removedListenersListLock);
            for (ListenerId id : removedListeners)
            {
                auto it = std::ranges::find_if(range, [&](auto& entry) { return entry.get() == id; });
                if (it != range.end()) {
                    listeners.erase(it);
                }
            }
            removedListeners.clear();
        }
        listeners.update();

        // Now poll events
        while (auto event = eventQueue.try_pop())
        {
            for (auto& listener : listeners.iter()) {
                std::invoke(*listener, *event);
            }
        }

        isBeingPolled.clear();
    }
} // namespace trc
