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
        while (!shouldStop) {
            pollFuncs.wait_pop()();
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

void vkb::EventThread::notifyActiveHandler(void(*pollFunc)())
{
    pollFuncs.push(pollFunc);
}
