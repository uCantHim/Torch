#include "trc_util/async/ThreadPool.h"

#include <cassert>
#include <ranges>
#include <algorithm>



trc::async::ThreadPool::ThreadPool()
    : ThreadPool(std::thread::hardware_concurrency())
{
}

trc::async::ThreadPool::ThreadPool(const uint32_t numThreads)
{
    workers.reserve(numThreads);
    for (uint32_t i = 0; i < numThreads; ++i)
    {
        workers.emplace_back([this]{
            while (true)
            {
                auto [work, terminate] = workQueue.wait_pop();
                if (terminate) break;
                work();
            }
        });
    }
}

trc::async::ThreadPool::~ThreadPool()
{
    for (size_t i = 0; i < workers.size(); ++i) {
        workQueue.push(Work{ .work=[]{}, .terminateThread=true });
    }
    for (auto& t : workers) {
        t.join();
    }

    assert(workQueue.empty());
}

void trc::async::ThreadPool::execute(std::function<void()> work)
{
    if (workers.empty()) {
        throw std::invalid_argument("A thread pool with 0 threads cannot execute work!");
    }

    workQueue.push(Work{ .work=std::move(work), .terminateThread=false });
}
