#include "util/ThreadPool.h"



vkb::ThreadPool::~ThreadPool()
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

void vkb::ThreadPool::spawnThread()
{
    std::lock_guard lk(threadListLock);

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

void vkb::ThreadPool::execute(std::function<void()> func)
{
    {
        std::lock_guard lock(threadListLock);
        for (auto& threadLock : threadLocks)
        {
            if (threadLock->hasWork) continue;

            std::lock_guard lock(threadLock->mutex);
            threadLock->hasWork = true;
            threadLock->work = std::move(func);
            threadLock->cvar.notify_one();

            return;
        }
    }

    // Did not find a thread ready for execution, create a new one
    spawnThread();
    execute(std::move(func));
}

auto vkb::getThreadPool() noexcept -> ThreadPool&
{
    static ThreadPool threadPool;
    return threadPool;
}
