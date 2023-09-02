#pragma once

#include <chrono>

// no-instance timer class
class Timer {
public:
	Timer(const Timer&) = delete;
	
	static std::chrono::system_clock clock;
	static std::chrono::system_clock::time_point _start, _stop;

	static void go() noexcept;
	static void stop() noexcept;

	static auto duration();
private:
	Timer() {};
};

std::chrono::system_clock Timer::clock;
std::chrono::system_clock::time_point Timer::_start, Timer::_stop;

inline void Timer::go() noexcept {
	_start = clock.now();
}

inline void Timer::stop() noexcept {
	_stop = clock.now();
}

inline auto Timer::duration() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(_stop - _start).count();
}