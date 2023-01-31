#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>

namespace trc::algorithm
{
    template<typename T>
    class IteratorRange
    {
    public:
        using value_type = typename T::value_type;

        constexpr inline IteratorRange(T begin, T end)
            : _begin(std::move(begin)), _end(std::move(end))
        {}

        constexpr inline auto begin() const -> T {
            return _begin;
        }

        constexpr inline auto end() const -> T {
            return _end;
        }

    private:
        T _begin;
        T _end;
    };

    namespace impl
    {
        template<typename IterType, typename Lock>
        class GuardedRangeBase
        {
        protected:
            GuardedRangeBase(IterType first, IterType last, Lock lock)
                : first(std::move(first)), last(std::move(last)), lock(std::move(lock))
            {}

        public:
            auto begin() const -> IterType {
                return first;
            }

            auto end() const -> IterType {
                return last;
            }

        private:
            IterType first;
            IterType last;
            Lock lock;
        };
    } // namespace impl

    /**
     * @brief An iterator range guarded by an exclusive lock
     */
    template<typename IterType, typename Mutex>
    class GuardedRange : public impl::GuardedRangeBase<IterType, std::unique_lock<Mutex>>
    {
    public:
        using Base = impl::GuardedRangeBase<IterType, std::unique_lock<Mutex>>;

        GuardedRange(IterType first, IterType last, std::unique_lock<Mutex> lock)
            : Base(std::move(first), std::move(last), std::move(lock))
        {}
    };

    /**
     * @brief An iterator range guarded by a shared lock
     */
    template<typename IterType, typename Mutex>
    class GuardedSharedRange : public impl::GuardedRangeBase<IterType, std::shared_lock<Mutex>>
    {
    public:
        using Base = impl::GuardedRangeBase<IterType, std::shared_lock<Mutex>>;

        GuardedSharedRange(IterType first, IterType last, std::shared_lock<Mutex> lock)
            : Base(std::move(first), std::move(last), std::move(lock))
        {}
    };
} // namespace trc::algorithm
