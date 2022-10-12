#pragma once

#include <cassert>
#include <concepts>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <ranges>

namespace trc::data
{
    template<typename T, typename ContainerType = typename std::queue<T>::container_type>
    class ThreadsafeQueue
    {
    private:
        template<typename It>
        static constexpr bool pointsToContainedType =
            std::same_as<std::remove_cvref_t<decltype(*std::declval<It>())>, T>
            || std::same_as<decltype(*std::declval<It>()), T>;

    public:
        using value_type      = typename std::queue<T, ContainerType>::value_type;
        using reference       = typename std::queue<T, ContainerType>::reference;
        using const_reference = typename std::queue<T, ContainerType>::const_reference;
        using size_type       = typename std::queue<T, ContainerType>::size_type;
        using container_type  = typename std::queue<T, ContainerType>::container_type;

        ThreadsafeQueue() = default;
        ~ThreadsafeQueue() noexcept = default;

        /** Copy constructor */
        ThreadsafeQueue(const ThreadsafeQueue& other)
        {
            std::scoped_lock lock(other.mutex);
            queue = other.queue;
        }

        /** Move constructor */
        ThreadsafeQueue(ThreadsafeQueue&& other) noexcept
        {
            std::scoped_lock lock(other.mutex);
            queue = std::move(other.queue);
        }

        /** Copy assignment operator */
        ThreadsafeQueue& operator=(const ThreadsafeQueue& other)
        {
            if (this != &other)
            {
                {
                    std::scoped_lock lock(mutex, other.mutex);
                    queue = other.queue;
                }
                cvar.notify_all();
            }
            return *this;
        }

        /** Move assignment operator */
        ThreadsafeQueue& operator=(ThreadsafeQueue&& other) noexcept
        {
            {
                std::scoped_lock lock(mutex, other.mutex);
                queue = std::move(other.queue);
            }
            cvar.notify_all();
            return *this;
        }

        /** Construct from a copy of the underlying container */
        ThreadsafeQueue(const ContainerType& container)        : queue(container) {}
        /** Construct from the underlying container */
        ThreadsafeQueue(ContainerType&& container)             : queue(std::move(container)) {}
        /** Construct from a copy of a std::queue */
        ThreadsafeQueue(const std::queue<T, ContainerType>& q) : queue(q) {}
        /** Construct from a std::queue */
        ThreadsafeQueue(std::queue<T, ContainerType>&& q)      : queue(std::move(q)) {}

        /** Construct with an allocator */
        template<typename Alloc>
            requires std::uses_allocator_v<ContainerType, Alloc>
        ThreadsafeQueue(const Alloc& alloc) : queue(alloc) {}

        /** Construct from a copy of the underlying container with an allocator */
        template<typename Alloc>
            requires std::uses_allocator_v<ContainerType, Alloc>
        ThreadsafeQueue(const ContainerType& container, const Alloc& alloc)
            : queue(container, alloc)
        {}

        /** Construct from the underlying container with an allocator */
        template<typename Alloc>
        ThreadsafeQueue(ContainerType&& container, const Alloc& alloc)
            : queue(std::move(container), alloc)
        {}

        /** Construct with elements from an iterator range */
        template<std::input_iterator Iter>
            requires pointsToContainedType<Iter>
        ThreadsafeQueue(Iter first, Iter last) : ThreadsafeQueue(ContainerType(first, last)) {}

        /** Construct with elements from any range */
        template<std::ranges::range R>
        ThreadsafeQueue(R range) : ThreadsafeQueue(std::begin(range), std::end(range)) {}

        bool operator==(const ThreadsafeQueue& other) const
        {
            std::scoped_lock lock(mutex, other.mutex);
            return queue == other.queue;
        }

        auto operator<=>(const ThreadsafeQueue& other) const
        {
            std::scoped_lock lock(mutex, other.mutex);
            return queue <=> other.queue;
        }

        bool operator==(const std::queue<T, ContainerType>& q) const
        {
            std::scoped_lock lock(mutex);
            return queue == q;
        }

        auto operator<=>(const std::queue<T, ContainerType>& q) const
        {
            std::scoped_lock lock(mutex);
            return queue <=> q;
        }

        auto empty() const -> bool
        {
            std::scoped_lock lock(mutex);
            return queue.empty();
        }

        auto size() const -> size_type
        {
            std::scoped_lock lock(mutex);
            return queue.size();
        }

        /**
         * @brief Push an item into the queue
         */
        void push(const T& item);

        /**
         * @brief Push an item into the queue
         */
        void push(T&& item);

        /**
         * @brief Push multiple items in a batch operation
         *
         * Push all items in an iterator range [first, last).
         *
         * This allows the queue to lock its mutex only once for multiple
         * operations, thus decreasing overhead.
         */
        template<std::input_iterator Iter>
            requires pointsToContainedType<Iter>
        void push(Iter first, Iter last)
        {
            {
                std::scoped_lock lock(mutex);
                for (; first != last; ++first) {
                    queue.push(*first);
                }
            }
            cvar.notify_all();
        }

        /**
         * @brief Push multiple items in a batch operation
         *
         * This allows the queue to lock its mutex only once for multiple
         * operations, thus decreasing overhead.
         */
        template<std::ranges::range R>
        void push(const R& range)
        {
            push(std::begin(range), std::end(range));
        }

        /**
         * @brief Construct an element in-place
         */
        template<typename... Args>
        void emplace(Args&&... args);

        /**
         * @brief Try to erase and retrieve the first element in the queue
         *
         * @return std::optional<T> None if the queue is empty. Otherwise
         *                          the front element.
         */
        auto try_pop() -> std::optional<T>;

        /**
         * @brief Erase and retrieve the first element in the queue
         *
         * Waits until an element is available. Returns immediately if the
         * queue already contains at least one element.
         *
         * @return T
         */
        auto wait_pop() -> T;

    private:
        /**
         * The internal predicate used to test whether we can pop an
         * element from the queue.
         */
        bool canPop() const {
            return !queue.empty();
        }

        auto doPop() -> T
        {
            assert(canPop());

            T item = std::move(queue.front());
            queue.pop();
            return item;
        }

        mutable std::mutex mutex;
        std::condition_variable cvar;
        std::queue<T, ContainerType> queue;
    };



    template<typename T, typename ContainerType>
    inline void ThreadsafeQueue<T, ContainerType>::push(const T& item)
    {
        {
            std::scoped_lock lock(mutex);
            queue.push(item);
        }
        cvar.notify_one();
    }

    /**
     * @brief Push an item into the queue
     */
    template<typename T, typename ContainerType>
    inline void ThreadsafeQueue<T, ContainerType>::push(T&& item)
    {
        {
            std::scoped_lock lock(mutex);
            queue.push(std::move(item));
        }
        cvar.notify_one();
    }

    template<typename T, typename ContainerType>
    template<typename... Args>
    inline void ThreadsafeQueue<T, ContainerType>::emplace(Args&&... args)
    {
        {
            std::scoped_lock lock(mutex);
            queue.emplace(std::forward<Args>(args)...);
        }
        cvar.notify_one();
    }

    template<typename T, typename ContainerType>
    inline auto ThreadsafeQueue<T, ContainerType>::try_pop() -> std::optional<T>
    {
        std::scoped_lock lock(mutex);
        if (canPop()) {
            return doPop();
        }
        return std::nullopt;
    }

    template<typename T, typename ContainerType>
    inline auto ThreadsafeQueue<T, ContainerType>::wait_pop() -> T
    {
        std::unique_lock lock(mutex);

        // Try to return immediately
        if (canPop()) {
            return doPop();
        }

        // Wait for an element to be available
        cvar.wait(lock, [this]{ return canPop(); });
        return doPop();
    }
} // namespace trc::data
