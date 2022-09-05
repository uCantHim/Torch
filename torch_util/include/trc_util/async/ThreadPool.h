#pragma once

#include <functional>
#include <thread>
#include <future>

#include <trc_util/data/ThreadsafeQueue.h>

namespace trc::async
{
    class ThreadPool
    {
    public:
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) noexcept = delete;
        auto operator=(const ThreadPool&) -> ThreadPool& = delete;
        auto operator=(ThreadPool&&) noexcept -> ThreadPool& = delete;

        /**
         * @brief Create a thread pool
         *
         * Creates a thread pool with `std::thread::hardware_concurrency()`
         * worker threads.
         */
        ThreadPool();

        /**
         * @brief Create a thread pool
         *
         * @param uint32_t numThreads The number of worker threads in the pool
         */
        explicit ThreadPool(uint32_t numThreads);

        /**
         * Waits until current work has been completed, then stops all
         * threads.
         */
        ~ThreadPool();

        /**
         * @brief Execute a function asynchronously with low overhead
         *
         * This function can run into a deadlock if the maximum worker
         * count has been reached **and** no workers are idle **and** if
         * no existing worker can finish its task before the task supplied
         * to this function is completed. In this case, this function
         * would block indefinitely waiting for a worker to become
         * available and the work would never be executed.
         *
         * @return std::future Future with the result value of the executed
         *                     function.
         * @throw std::invalid_argument if the pool has been created with
         *                              a thread count of zero.
         */
        template<typename Func, typename ...Args>
            requires std::is_invocable_v<Func, Args...>
        auto async(Func&& func, Args&&... args) -> std::future<std::invoke_result_t<Func, Args...>>;

    private:
        /**
         * Execute a function asynchronously.
         *
         * Tries to find an idle thread to execute the function with. If
         * no such thread is available, it creates a new thread.
         */
        void execute(std::function<void()> func);

        struct Work
        {
            std::function<void()> work;
            bool terminateThread;
        };

        std::vector<std::thread> workers;
        data::ThreadsafeQueue<Work> workQueue;
    };



    template<typename Func, typename ...Args>
        requires std::is_invocable_v<Func, Args...>
    inline auto ThreadPool::async(Func&& func, Args&&... args)
        -> std::future<std::invoke_result_t<Func, Args...>>
    {
        using ReturnType = std::invoke_result_t<Func, Args...>;

        // Use a shared ptr because std::functions must be copyable, which
        // isn't the case for std::promise
        auto promise = std::make_shared<std::promise<ReturnType>>();
        execute([promise, func = std::forward<Func>(func), ...args = std::forward<Args>(args)]() mutable
        {
            if constexpr (std::is_same_v<ReturnType, void>)
            {
                func(std::forward<Args>(args)...);
                promise->set_value();
            }
            else {
                promise->set_value(func(std::forward<Args>(args)...));
            }
        });

        return promise->get_future();
    }
} // namespace trc::async
