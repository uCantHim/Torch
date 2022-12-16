#include "trc/base/event/EventHandler.h"

#include "trc/base/Logging.h"



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

    log::info << "--- Event thread started\n";
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
