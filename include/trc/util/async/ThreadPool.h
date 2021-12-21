#pragma once

#include <functional>
#include <thread>
#include <future>

namespace trc::async
{
    class ThreadPool
    {
    public:
        ThreadPool(const ThreadPool&) = delete;
        auto operator=(const ThreadPool&) -> ThreadPool& = delete;

        ThreadPool() = default;
        explicit ThreadPool(uint32_t maxThreads);
        ~ThreadPool();

        ThreadPool(ThreadPool&&) noexcept;
        auto operator=(ThreadPool&&) noexcept -> ThreadPool&;

        /**
         * @brief Execute a function asynchronously with low overhead
         *
         * Well, relatively low when compared with the effort of creating
         * a new thread for the task.
         *
         * @return std::future Future with the result value of the executed
         *                     function.
         */
        template<typename Func, typename ...Args>
            requires std::is_invocable_v<Func, Args...>
        auto async(Func func, Args&&... args) -> std::future<std::invoke_result_t<Func, Args...>>;

    private:
        /**
         * Locking and work-distribution mechanism for a single thread.
         * This is very complicated because the usage of condition_variables
         * is ridiculously complicated.
         */
        struct ThreadLock
        {
            std::function<void()> work;

            bool hasWork{ false };
            std::condition_variable cvar;
            std::mutex mutex;
        };

        /** Spawn a new thread. Is called in execute(). */
        auto spawnThread() -> ThreadLock&;

        /**
         * Execute a function asynchronously.
         *
         * Tries to find an idle thread to execute the function with. If
         * no such thread is available, it creates a new thread.
         */
        void execute(std::function<void()> func);

        std::vector<std::thread> threads;
        // Use unique_ptrs to make the thread pool reliably movable
        std::vector<std::unique_ptr<ThreadLock>> threadLocks;
        std::mutex listLock;

        // Maximum number of threads that can run simultaneously
        uint32_t maxThreads{ UINT32_MAX };
        // Used to end all threads when the pool is destroyed
        bool stopAllThreads{ false };
    };



    template<typename Func, typename ...Args>
        requires std::is_invocable_v<Func, Args...>
    inline auto ThreadPool::async(Func func, Args&&... args)
        -> std::future<std::invoke_result_t<Func, Args...>>
    {
        // Use a shared ptr because std::functions must be copyable, which
        // isn't the case for std::promise
        auto promise = std::make_shared<std::promise<std::invoke_result_t<Func, Args...>>>();
        execute([promise, func = std::move(func), ...args = std::forward<Args>(args)]() mutable
        {
            if constexpr (std::is_same_v<std::invoke_result_t<Func, Args...>, void>)
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
