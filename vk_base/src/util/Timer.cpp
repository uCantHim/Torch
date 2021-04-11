#include "util/Timer.h"

using namespace std::chrono;



vkb::Timer::Timer()
    :
	last_reset(ClockType::now())
{
}

auto vkb::Timer::reset() noexcept -> float
{
	auto time = ClockType::now() - last_reset;
	last_reset = ClockType::now();

	return static_cast<float>(duration_cast<nanoseconds>(time).count()) / 1000000.0f;
}

auto vkb::Timer::duration() const noexcept -> float
{
	return static_cast<float>(duration_cast<nanoseconds>(ClockType::now() - last_reset).count()) / 1000000.0f;
}
