#pragma once

#include <chrono>

namespace vkb
{
    /**
     * Counts elapsed time in milliseconds
     */
    class Timer
    {
    public:
        Timer();

        auto reset() noexcept -> float;
        auto duration() const noexcept -> float;

    private:
        using ClockType = std::chrono::system_clock;

        ClockType::time_point last_reset;
    };
} // namespace vkb
