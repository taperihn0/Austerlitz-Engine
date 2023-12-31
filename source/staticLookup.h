#pragma once

#include <type_traits>
#include <cassert>


namespace {
	namespace cexpr {

		// resources for initialization compile-time array
		template <bool TwoDimension, typename T, int S1, int S2 = 0>
		struct CexprArr {
			using dataTable = std::conditional_t<TwoDimension, T[S2], T>;

			template <typename InitF>
			constexpr CexprArr(InitF init)
				: arr{} {
				helper<TwoDimension>(init);
			}

			template <bool D = TwoDimension>
			inline constexpr auto get(int i1, int i2 = 0) const -> std::enable_if_t<D, T> {
				assert(i1 >= 0 and i1 < S1 and i2 >= 0 and i2 < S2 && "Index overflow");
				return arr[i1][i2];
			}

			template <bool D = TwoDimension>
			inline constexpr auto get(int i1, int i2 = 0) const -> std::enable_if_t<!D, T> {
				assert(i1 >= 0 and i1 < S1 && "Index overflow");
				return arr[i1];
			}

			template <bool D, typename InitF>
			inline constexpr auto helper(InitF init) -> std::enable_if_t<D, void> {
				for (int i = 0; i < S1; i++)
					for (int j = 0; j < S2; j++)
						arr[i][j] = init(i, j);
			}

			template <bool D, typename InitF>
			inline constexpr auto helper(InitF init) -> std::enable_if_t<!D, void> {
				for (int i = 0; i < S1; i++)
					arr[i] = init(i);
			}

			dataTable arr[S1];
		};

	}
}