#include "trc/base/event/EventHandler.h"

#include <iostream>
#include <thread>

#include "trc/base/VulkanDebug.h"



void trc::EventThread::start()
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

void trc::EventThread::terminate()
{
    shouldStop = true;
    notifyActiveHandler([]{});

    if (thread.joinable()) {
        thread.join();
    }
}

void trc::EventThread::notifyActiveHandler(void(*pollFunc)())
{
    pollFuncs.push(pollFunc);
}
