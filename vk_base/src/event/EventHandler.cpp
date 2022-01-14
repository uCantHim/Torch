#include "event/EventHandler.h"

#include <iostream>
#include <thread>

#include "VulkanDebug.h"



void vkb::EventThread::start()
{
    terminate();
    shouldStop = false;

    thread = std::thread([]()
    {
        while (!shouldStop)
        {
            std::unique_lock lock(cvarLock);
            cvar.wait(lock, [] { return cvarFlag; });
            cvarFlag = false;

            std::scoped_lock listLock(activeHandlerListLock);
            while (!activeHandlers.empty())
            {
                activeHandlers.front()();
                activeHandlers.pop();
            }
        }
    });

    if constexpr (enableVerboseLogging) {
        std::cout << "--- Event thread started\n";
    }
}

void vkb::EventThread::terminate()
{
    shouldStop = true;
    notifyActiveHandler([]{});

    if (thread.joinable()) {
        thread.join();
    }
}

void vkb::EventThread::notifyActiveHandler(void(*pollFunc)(void))
{
    {
        std::scoped_lock lock(activeHandlerListLock);
        activeHandlers.emplace(pollFunc);
    }

    std::unique_lock cvLock(cvarLock);
    cvarFlag = true;

    cvLock.unlock();
    cvar.notify_one();
}
