#pragma once

#include <chrono>

namespace vkb
{
    using DefaultTimeType = std::chrono::milliseconds;

    /**
     * Counts elapsed time
     */
    template<typename TimeType = DefaultTimeType>
    class Timer
    {
    public:
        Timer();

        auto reset() noexcept -> uint32_t;
        auto duration() const noexcept -> uint32_t;

    private:
        using ClockType = std::chrono::system_clock;
        using TimePoint = std::chrono::time_point<ClockType, TimeType>;

        TimePoint last_reset;
    };

#include "Timer.inl"
} // namespace vkb
