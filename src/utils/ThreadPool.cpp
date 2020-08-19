#include "utils/ThreadPool.h"



trc::ThreadPool::~ThreadPool()
{
    stopAllThreads = true;
    for (auto& lock : threadLocks)
    {
        std::lock_guard lk(lock->mutex);
        lock->hasWork = true;
        lock->work = []() {};
        lock->cvar.notify_one();
    }

    for (auto& thread : threads)
    {
        thread.join();
    }
}

void trc::ThreadPool::spawnThread()
{
    auto& lock = *threadLocks.emplace_back(new ThreadLock);
    threads.emplace_back([this, &lock]() {
        while (!stopAllThreads)
        {
            std::unique_lock mutexLock(lock.mutex);
            lock.cvar.wait(mutexLock, [&lock]() { return lock.hasWork; });

            // Do the actual work
            lock.work();

            lock.hasWork = false;
        }
    });
}

void trc::ThreadPool::execute(std::function<void()> func)
{
    for (auto& threadLock : threadLocks)
    {
        if (threadLock->hasWork) continue;

        std::lock_guard lock(threadLock->mutex);
        threadLock->hasWork = true;
        threadLock->work = std::move(func);
        threadLock->cvar.notify_one();

        return;
    }

    // Did not find a thread ready for execution, create a new one
    spawnThread();
    execute(std::move(func));
}
