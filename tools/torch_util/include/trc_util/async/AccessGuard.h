#pragma once

#include <concepts>
#include <mutex>
#include <shared_mutex>

namespace trc::async
{
    /**
     * @brief A wrapper that provides mutex-guarded access to a value
     *
     * Has value semantics; invalidating pointers to the AccessGuard object
     * also invalidates pointers to the wrapped value. `ReadAccess` and
     * `WriteAccess` objects must not outlive the `AccessGuard` object.
     */
    template<typename T>
    class AccessGuard
    {
    public:
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;

        struct ReadAccess
        {
            ReadAccess(const_reference value, std::shared_lock<std::shared_mutex> lock)
                : ptr(&value), lock(std::move(lock))
            {}

            auto operator*() & -> const_reference { return *ptr; }
            auto operator->() -> const_pointer { return ptr; }

        private:
            const_pointer ptr;
            std::shared_lock<std::shared_mutex> lock;
        };

        struct WriteAccess
        {
            WriteAccess(reference value, std::unique_lock<std::shared_mutex> lock)
                : ptr(&value), lock(std::move(lock))
            {}

            auto operator*() & -> reference { return *ptr; }
            auto operator->() -> pointer { return ptr; }

        private:
            pointer ptr;
            std::unique_lock<std::shared_mutex> lock;
        };

        AccessGuard(const AccessGuard&) = delete;
        AccessGuard& operator=(const AccessGuard&) = delete;

        /**
         * @brief Default-construct the wrapped value
         */
        AccessGuard() requires std::default_initializable<value_type> = default;

        /**
         * @brief Construct the wrapped value
         */
        template<typename ...Args>
            requires std::constructible_from<value_type, Args...>
        explicit AccessGuard(Args&&... args)
            : value(std::forward<Args>(args)...)
        {}

        AccessGuard(AccessGuard&&) noexcept
            requires std::move_constructible<value_type>
            = default;

        ~AccessGuard() noexcept = default;

        AccessGuard& operator=(AccessGuard&&) noexcept
            requires std::is_move_assignable_v<value_type>
            = default;

        auto operator<=>(const AccessGuard& other) const
            requires std::three_way_comparable<AccessGuard>
        {
            std::shared_lock lhs(this->mutex, std::defer_lock);
            std::shared_lock rhs(other.mutex, std::defer_lock);
            std::lock(lhs, rhs);
            return this->value <=> other.value;
        }

        /**
         * @brief Acquire shared read access to the value
         */
        auto read() const -> ReadAccess;

        /**
         * @brief Acquire unique write access to the value
         */
        auto modify() -> WriteAccess;

    private:
        value_type value;
        mutable std::shared_mutex mutex;
    };

    template<typename T>
    auto AccessGuard<T>::read() const -> ReadAccess
    {
        return ReadAccess{ value, std::shared_lock(mutex) };
    }

    template<typename T>
    auto AccessGuard<T>::modify() -> WriteAccess
    {
        return WriteAccess{ value, std::unique_lock(mutex) };
    }
} // namespace trc::async
