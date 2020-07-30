#include "event/EventHandler.h"

#include <thread>

#include "VulkanDebug.h"



void vkb::EventThread::start()
{
    terminate();
    shouldStop = false;
    running = true;

    std::thread([]() {
        while (!shouldStop)
        {
            // Don't use range-based for-loop here because iterators
            // are not thread safe. Declare size independently because
            // it silences the clangtidy modernization warning
            const size_t size = handlers.size();
            for (size_t i = 0; i < size; i++)
            {
                EventThread::handlers[i]();
            }
        }
        running = false;
    }).detach();

    if constexpr (enableVerboseLogging) {
        std::cout << "--- Event thread started\n";
    }
}

void vkb::EventThread::terminate()
{
    shouldStop = true;
    while (running);
}

void vkb::EventThread::registerHandler(std::function<void()> pollFunc)
{
    std::lock_guard lock(handlerListLock);
    EventThread::handlers.push_back(std::move(pollFunc));
}
