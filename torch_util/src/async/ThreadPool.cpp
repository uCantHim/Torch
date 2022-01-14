#include "trc_util/async/ThreadPool.h"

#include <cassert>
#include <ranges>
#include <algorithm>



trc::async::ThreadPool::ThreadPool(uint32_t maxThreads)
    : maxThreads(maxThreads)
{
}

trc::async::ThreadPool::ThreadPool(ThreadPool&& other) noexcept
    :
    maxThreads(other.maxThreads)
{
    std::scoped_lock lock(workerListLock, other.workerListLock);
    workers = std::move(other.workers);
}

auto trc::async::ThreadPool::operator=(ThreadPool&& rhs) noexcept -> ThreadPool&
{
    if (this != &rhs)
    {
        std::scoped_lock lock(workerListLock, rhs.workerListLock);
        maxThreads = rhs.maxThreads;
        workers = std::move(rhs.workers);
    }
    return *this;
}

trc::async::ThreadPool::~ThreadPool()
{
    std::scoped_lock lock(workerListLock);

    for (auto& worker : workers) worker->stop();
    for (auto& worker : workers) worker->join();
}

void trc::async::ThreadPool::trim()
{
    std::scoped_lock lock(idleWorkerListLock, workerListLock);
    for (WorkerThread* thread : idleWorkers)
    {
        assert(thread != nullptr);
        thread->join();

        auto it = std::remove_if(workers.begin(), workers.end(),
                                 [&](const auto& other) { return thread == other.get(); });
        assert(it != workers.end());
        workers.erase(it);
    }
    idleWorkers.clear();
}

auto trc::async::ThreadPool::spawnThread(std::function<void()> initialWork)
    -> WorkerThread&
{
    assert(workers.size() < maxThreads);

    std::scoped_lock _lock(workerListLock);
    return *workers.emplace_back(new WorkerThread(std::move(initialWork), this));
}

void trc::async::ThreadPool::execute(std::function<void()> work)
{
    // Spawn a new thread if none are idle and if there is enough space
    if (idleWorkers.empty() && workers.size() < maxThreads)
    {
        spawnThread(std::move(work));
        return;
    }

    // maxThreads has been reached, wait for thread to become available
    auto canProceed = [&] {
        while (true)
        {
            while (idleWorkers.empty());

            std::unique_lock lock(idleWorkerListLock);
            if (!idleWorkers.empty()) {
                return lock;
            }
        }
    }();
    assert(!idleWorkers.empty());
    assert(idleWorkers.back() != nullptr);
    assert(!idleWorkers.back()->isWorking());

    // Thread is available - dispatch work to it
    idleWorkers.back()->signalWork(std::move(work));
    idleWorkers.pop_back();
}



// ------------------------ //
//      Worker Thread       //
// ------------------------ //

trc::async::ThreadPool::WorkerThread::WorkerThread(
    std::function<void(void)> initialWork,
    ThreadPool* pool)
    :
    owningPool(pool)
{
    thread = std::thread([this, initialWork=std::move(initialWork)]
    {
        hasWork = true;
        // Execute initial work, no matter in which state the stopAllThreads
        // flag is.
        initialWork();
        hasWork = false;

        // Add worker to idle list
        {
            std::scoped_lock _lock(owningPool->idleWorkerListLock);
            owningPool->idleWorkers.emplace_back(this);
        }

        while (!stopThread)
        {
            std::unique_lock lock(mutex);
            cvar.wait(lock, [this]() { return hasWork || stopThread; });
            if (stopThread) break;

            // Do the actual work
            work();

            hasWork = false;

            // Add worker to idle list
            std::scoped_lock _lock(owningPool->idleWorkerListLock);
            owningPool->idleWorkers.emplace_back(this);
        }

        hasWork = false;
    });
}

void trc::async::ThreadPool::WorkerThread::signalWork(std::function<void(void)> newWork)
{
    assert(!isWorking());

    std::unique_lock lock(mutex);
    work = std::move(newWork);
    hasWork = true;

    lock.unlock();
    cvar.notify_one();
}

bool trc::async::ThreadPool::WorkerThread::isWorking() const
{
    return hasWork;
}

void trc::async::ThreadPool::WorkerThread::stop()
{
    stopThread = true;
    std::scoped_lock lock(mutex);
    cvar.notify_one();
}

void trc::async::ThreadPool::WorkerThread::join()
{
    stop();
    thread.join();
}
