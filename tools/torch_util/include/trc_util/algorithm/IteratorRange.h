#pragma once

#include <memory>
#include <mutex>

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

    template<typename IterType, typename Mutex>
    class GuardedRange
    {
    public:
        GuardedRange(IterType first, IterType last, std::unique_lock<Mutex> lock)
            : first(std::move(first)), last(std::move(last)), lock(std::move(lock))
        {}

        auto begin() const -> IterType {
            return first;
        }

        auto end() const -> IterType {
            return last;
        }

    private:
        IterType first;
        IterType last;
        std::unique_lock<Mutex> lock;
    };
} // namespace trc::algorithm
