#pragma once

#include <chrono>


inline auto now() {
	static std::chrono::system_clock clock;
	return clock.now();
}

inline constexpr auto sinceStart_ms(std::chrono::system_clock::time_point start) {
	return std::chrono::duration_cast<std::chrono::milliseconds>(now() - start).count();
}


// timer class
class Timer {
public:
	Timer() = default;
	
	void go() noexcept;
	void stop() noexcept;

	auto duration();
private:
	std::chrono::system_clock::time_point _start, _stop;
};

inline void Timer::go() noexcept {
	_start = now();
}

inline void Timer::stop() noexcept {
	_stop = now();
}

inline auto Timer::duration() {
	return sinceStart_ms(_start);
}

struct Time {

	inline bool checkTimeLeft() noexcept {
		return sinceStart_ms(start) < this_move;
	}

	bool is_time, stop;
	int left, inc,
		this_move;
	decltype(now()) start;
};

extern Time time_data;

inline constexpr int operator "" _ms(const ULL a) noexcept {
	return static_cast<int>(a);
}