#pragma once

#include <chrono>

// timer class
class Timer {
public:
	Timer() = default;
	
	void go() noexcept;
	void stop() noexcept;

	auto duration();
private:
	std::chrono::system_clock clock;
	std::chrono::system_clock::time_point _start, _stop;
};

inline void Timer::go() noexcept {
	_start = clock.now();
}

inline void Timer::stop() noexcept {
	_stop = clock.now();
}

inline auto Timer::duration() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(_stop - _start).count();
}