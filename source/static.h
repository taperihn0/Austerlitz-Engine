#pragma once

#include <type_traits>

// compile-time custom helper functions
namespace {
	namespace cexpr {

		// simple custom absolute function with constexpr system 
		template <typename T, class =
			std::enable_if_t<std::is_arithmetic_v<T>>>
		constexpr auto abs(T x) noexcept {
			return x < 0 ? -x : x;
		}

		void static_for(U64&, U64, std::integral_constant<int, nWhite>) {}
		void static_for(U64&, U64, std::integral_constant<int, nBlack>) {}

		template <int It>
		void static_for(U64& dst, U64 occ, std::integral_constant<int, It>) {
			U64 tmp = BBs[It];
			
			while (tmp) {
				dst |= attack<static_cast<enumPiece>(It / 2)>(occ, popLS1B(tmp));
			}

			static_for(dst, occ, std::integral_constant<int, It + 2>());
		}
	}
}
