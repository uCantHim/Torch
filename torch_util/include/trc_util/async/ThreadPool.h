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

        /**
         * @brief Create a thread pool
         *
         * Does not create any threads before work is submitted. Can have
         * up to UINT32_MAX threads active at the same time.
         */
        ThreadPool() = default;

        /**
         * @brief Create a thread pool
         *
         * Does not create any threads before work is submitted.
         *
         * @param uint32_t maxThreads It could be a good idea to pass
         *        std::thread::hardware_concurrency() here.
         */
        explicit ThreadPool(uint32_t maxThreads);

        /**
         * Waits until current work has been completed, then stops all
         * threads.
         */
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

        /**
         * @brief Remove all idle threads
         */
        void trim();

    private:
        /**
         * A worker thread that can execute a single piece of work
         * asynchronously.
         */
        class WorkerThread
        {
        public:
            explicit WorkerThread(std::function<void(WorkerThread&)> work);

            void signalWork(std::function<void(WorkerThread&)> work);
            bool isWorking() const;

            /** Signal the thread to stop after completing its current work */
            void stop();
            /** Stop and join the thread. Returns after the thread has stopped. */
            void join();

        private:
            std::thread thread;
            bool stopThread{ false };

            std::function<void(WorkerThread&)> work;

            bool hasWork{ false };
            std::condition_variable cvar;
            std::mutex mutex;
        };

        /** Spawn a new thread. Is called in execute(). */
        auto spawnThread(std::function<void(WorkerThread&)> initialWork) -> WorkerThread&;

        /**
         * Execute a function asynchronously.
         *
         * Tries to find an idle thread to execute the function with. If
         * no such thread is available, it creates a new thread.
         */
        void execute(std::function<void()> func);

        // Maximum number of worker threads in the pool
        uint32_t maxThreads{ UINT32_MAX };

        // Use unique_ptrs to make the thread pool reliably movable
        std::vector<std::unique_ptr<WorkerThread>> workers;
        std::vector<WorkerThread*> idleWorkers;
        std::mutex workerListLock;
        std::mutex idleWorkerListLock;
    };



    template<typename Func, typename ...Args>
        requires std::is_invocable_v<Func, Args...>
    inline auto ThreadPool::async(Func func, Args&&... args)
        -> std::future<std::invoke_result_t<Func, Args...>>
    {
        using ReturnType = std::invoke_result_t<Func, Args...>;

        // Use a shared ptr because std::functions must be copyable, which
        // isn't the case for std::promise
        auto promise = std::make_shared<std::promise<ReturnType>>();
        execute([promise, func = std::move(func), ...args = std::forward<Args>(args)]() mutable
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
