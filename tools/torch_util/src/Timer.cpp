#include "trc_util/Timer.h"

using namespace std::chrono;



trc::Timer::Timer()
    :
	last_reset(ClockType::now())
{
}

auto trc::Timer::reset() noexcept -> float
{
	auto time = ClockType::now() - last_reset;
	last_reset = ClockType::now();

	return static_cast<float>(duration_cast<nanoseconds>(time).count()) / 1000000.0f;
}

auto trc::Timer::duration() const noexcept -> float
{
	return static_cast<float>(duration_cast<nanoseconds>(ClockType::now() - last_reset).count()) / 1000000.0f;
}
