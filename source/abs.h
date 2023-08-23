#pragma once

#include <type_traits>

// simple custom absolute function with constexpr system 
namespace cexpr {

	template <typename T, class = 
		std::enable_if_t<std::is_arithmetic_v<T>>>
	constexpr auto abs(T x) noexcept {
		return x < 0 ? -x : x;
	}

}
