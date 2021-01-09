#pragma once

#include <functional>
#include <thread>
#include <future>

namespace vkb
{
    class ThreadPool
    {
    public:
        ThreadPool() = default;
        ~ThreadPool();

        ThreadPool(ThreadPool&&) noexcept = default;
        auto operator=(ThreadPool&&) noexcept -> ThreadPool& = default;
        ThreadPool(const ThreadPool&) = delete;
        auto operator=(const ThreadPool&) -> ThreadPool& = delete;

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
        auto async(Func&& func, Args&&... args) -> std::future<std::invoke_result_t<Func, Args...>>
            requires std::is_invocable_v<Func, Args...>
                     && (!std::is_same_v<std::invoke_result_t<Func, Args...>, void>);

        template<typename Func, typename ...Args>
        void async(Func&& func, Args&&... args)
            requires std::is_invocable_v<Func, Args...>
                     && std::is_same_v<std::invoke_result_t<Func, Args...>, void>;

    private:
        /** Spawn a new thread. Is called in execute(). */
        void spawnThread();

        /**
         * Execute a function asynchronously.
         *
         * Tries to find an idle thread to execute the function with. If
         * no such thread is available, it creates a new thread.
         */
        void execute(std::function<void()> func);

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

        std::vector<std::thread> threads;
        // Use unique_ptrs to make the thread pool reliably movable
        std::vector<std::unique_ptr<ThreadLock>> threadLocks;

        // Guards the list of thread locks
        std::mutex threadListLock;

        // Used to end all threads when the pool is destroyed
        bool stopAllThreads{ false };
    };

    /**
     * @brief Get a global thread pool
     *
     * A vkb::getThreadPool function in the same fashion as vkb::getDevice,
     * vkb::getSwapchain, etc.
     */
    extern auto getThreadPool() noexcept -> ThreadPool&;



    template<typename Func, typename ...Args>
    inline auto ThreadPool::async(Func&& func, Args&&... args)
        -> std::future<std::invoke_result_t<Func, Args...>>
        requires std::is_invocable_v<Func, Args...>
              && (!std::is_same_v<std::invoke_result_t<Func, Args...>, void>)
    {
        // Use a shared ptr because std::functions must be copyable, which
        // isn't the case for std::promise
        auto promise = std::make_shared<std::promise<std::invoke_result_t<Func, Args...>>>();
        execute(
            [
                promise,
                func = std::forward<Func>(func),
                ...args = std::forward<Args>(args)
            ]() {
                promise->set_value(func(std::forward<Args>(args)...));
            }
        );

        return promise->get_future();
    }

    template<typename Func, typename ...Args>
    inline void ThreadPool::async(Func&& func, Args&&... args)
        requires std::is_invocable_v<Func, Args...>
                 && std::is_same_v<std::invoke_result_t<Func, Args...>, void>
    {
        execute(
            [
                func = std::forward<Func>(func),
                ...args = std::forward<Args>(args)
            ]() {
                func(std::forward<Args>(args)...);
            }
        );
    }
} // namespace vkb
