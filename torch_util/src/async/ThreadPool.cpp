#include "trc_util/async/ThreadPool.h"



trc::async::ThreadPool::ThreadPool(uint32_t maxThreads)
    : maxThreads(maxThreads)
{
}

trc::async::ThreadPool::ThreadPool(ThreadPool&& other) noexcept
    :
    maxThreads(other.maxThreads)
{
    std::scoped_lock lock(other.listLock);

    threads = std::move(other.threads);
    threadLocks = std::move(other.threadLocks);
}

auto trc::async::ThreadPool::operator=(ThreadPool&& rhs) noexcept -> ThreadPool&
{
    if (this != &rhs)
    {
        std::scoped_lock lock(listLock, rhs.listLock);

        maxThreads = rhs.maxThreads;
        threads = std::move(rhs.threads);
        threadLocks = std::move(rhs.threadLocks);
    }
    return *this;
}

trc::async::ThreadPool::~ThreadPool()
{
    std::scoped_lock lock(listLock);
    stopAllThreads = true;
    for (auto& lock : threadLocks)
    {
        std::lock_guard lk(lock->mutex);
        if (lock->hasWork) continue;
        lock->work = []() {};
        lock->hasWork = true;
        lock->cvar.notify_one();
    }

    for (auto& thread : threads)
    {
        thread.join();
    }
}

auto trc::async::ThreadPool::spawnThread() -> ThreadLock&
{
    std::scoped_lock _lock(listLock);

    auto& lock = *threadLocks.emplace_back(new ThreadLock);
    threads.emplace_back([this, &lock]() {
        do {
            std::unique_lock mutexLock(lock.mutex);
            lock.cvar.wait(mutexLock, [&lock]() { return lock.hasWork; });

            // Do the actual work
            lock.work();

            lock.work = []{};
            lock.hasWork = false;
        } while (!stopAllThreads);
    });
    return lock;
}

void trc::async::ThreadPool::execute(std::function<void()> func)
{
    {
        std::scoped_lock _lock(listLock);
        for (auto& threadLock : threadLocks)
        {
            if (threadLock->hasWork) continue;

            std::lock_guard lock(threadLock->mutex);
            threadLock->work = std::move(func);
            threadLock->hasWork = true;
            threadLock->cvar.notify_one();

            return;
        }
    }

    // Did not find a thread ready for execution, create a new one
    if (maxThreads > threads.size())
    {
        auto& lock = spawnThread();

        std::lock_guard _lock(lock.mutex);
        lock.work = std::move(func);
        lock.hasWork = true;
        lock.cvar.notify_one();
    }
    else {
        execute(std::move(func));
    }
}
