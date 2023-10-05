#pragma once

#include "MoveItem.h"
#include "MoveOrder.h"


namespace PV {
	// Principal Variation database tables
	using lenghtPV = std::array<int, Order::max_Ply>;
	extern lenghtPV pv_len;

	using linePV = std::array<std::array<MoveItem::iMove, Order::max_Ply>, Order::max_Ply>;
	extern linePV pv_line;

	// clear PV database
	inline void clear() {
		pv_len.fill(0);
		for (auto& x : PV::pv_line) x.fill(0);
	}
};


namespace Search {

	// set some safe bufor at bounds values to prevent overflows
	static constexpr int
		low_bound = std::numeric_limits<int>::min() + 1000000,
		high_bound = std::numeric_limits<int>::max() - 1000000,
		draw_score = 0,
		mate_score = -std::numeric_limits<int>::max() / 4,
		mate_comp = mate_score + 1000;

	struct Results {
		int score;
		unsigned long long nodes;
	};

	extern Results search_results;

	void bestMove(int depth);

	// clear killer history
	inline void clearKiller() {
		for (auto& x : Order::killer) x.fill(0);
	}

}