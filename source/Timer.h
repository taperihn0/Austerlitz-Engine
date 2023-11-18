#pragma once

#include <chrono>


inline auto now() {
	static std::chrono::system_clock clock;
	return clock.now();
}

inline constexpr auto sinceStart_ms(std::chrono::system_clock::time_point start) {
	return std::chrono::duration_cast<std::chrono::milliseconds>(now() - start).count();
}


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

inline constexpr int operator "" _ms(const ULL a) noexcept {
	return static_cast<int>(a);
}

struct Time {

	inline bool checkTimeLeft() noexcept {
		return sinceStart_ms(start) < this_move;
	}

	// fixed amount of given time - zero fixed time means no time control
	inline void setFixedTime(int fixed_time) noexcept {
		is_time = static_cast<bool>(fixed_time), stop = false;
		this_move = fixed_time;
	}

	// calculate time for single move
	void calcMoveTime(int time_left, int time_inc) noexcept {
		is_time = true, stop = false;

		left = time_left;
		inc = time_inc;
		this_move = (time_left / 42) + (inc / 2);

		if (this_move >= left)
			this_move = left / 12;

		if (this_move < 0_ms)
			this_move = 5_ms;
	}

	bool is_time, stop;
	int left, inc,
		this_move;
	decltype(now()) start;
};
