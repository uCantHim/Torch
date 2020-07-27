using namespace std::chrono;



template<typename TimeType>
vkb::Timer<TimeType>::Timer()
    :
	last_reset(time_point_cast<TimeType>(ClockType::now()))
{
}

template<typename TimeType>
auto vkb::Timer<TimeType>::reset() noexcept -> uint32_t
{
	auto time = duration_cast<TimeType>(ClockType::now() - last_reset);
	last_reset = time_point_cast<TimeType>(ClockType::now());

	return static_cast<uint32_t>(time.count());
}

template<typename TimeType>
auto vkb::Timer<TimeType>::duration() const noexcept -> uint32_t
{
	return static_cast<uint32_t>(duration_cast<TimeType>(ClockType::now() - last_reset).count());
}
