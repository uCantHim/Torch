#include "event/EventHandler.h"

#include <iostream>
#include <thread>

#include "VulkanDebug.h"



void vkb::EventThread::start()
{
    terminate();
    shouldStop = false;

    thread = std::thread([]() {
        do
        {
            // Don't use range-based for-loop here because iterators
            // are not thread safe. Declare size independently because
            // it silences the clangtidy modernization warning
            const size_t size = handlers.size();
            for (size_t i = 0; i < size; i++)
            {
                EventThread::handlers[i]();
            }
        } while (!shouldStop);
    });

    if constexpr (enableVerboseLogging) {
        std::cout << "--- Event thread started\n";
    }
}

void vkb::EventThread::terminate()
{
    shouldStop = true;
    if (thread.joinable()) {
        thread.join();
    }
}

void vkb::EventThread::registerHandler(std::function<void()> pollFunc)
{
    std::lock_guard lock(handlerListLock);
    EventThread::handlers.push_back(std::move(pollFunc));
}
