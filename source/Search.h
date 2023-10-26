#pragma once

#include "MoveItem.h"
#include "Timer.h"
#include <limits>


namespace Search {

	// general compile-time constans used in search
	static constexpr int
		// set some safe bufor at bounds values to prevent overflows
		low_bound = std::numeric_limits<int>::min() + 1000000,
		high_bound = std::numeric_limits<int>::max() - 1000000,

		draw_score = 0,
		mate_score = -std::numeric_limits<int>::max() / 4,
		mate_comp = mate_score + 1000,

		max_depth = 128,
		max_Ply = 128,
		time_check_modulo = 2048,

		time_stop_sign = low_bound + 10;

	extern MoveItem::iMove prev_move;
	extern Time time_data;
	
	// Principal Variation lookup tables
	namespace PV {

		using lenghtPV = std::array<int, Search::max_Ply>;
		extern lenghtPV pv_len;

		using linePV = std::array<std::array<MoveItem::iMove, Search::max_Ply>, Search::max_Ply>;
		extern linePV pv_line;

		// clear PV lookup
		inline void clear() {
			pv_len.fill(0);
			for (auto& x : PV::pv_line) x.fill(0);
		}

	} // namespace PV 

	// calculate best move using Iterative Deepening
	void bestMove(const int depth);

} // namespace Search