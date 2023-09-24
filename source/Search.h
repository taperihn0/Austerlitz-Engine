#pragma once

#include "MoveItem.h"
#include "MoveOrder.h"


namespace PV {
	// Principal Variation database tables
	using lenghtPV = std::array<int, Order::max_Ply>;
	extern lenghtPV pv_len;

	using linePV = std::array<std::array<MoveItem::iMove, Order::max_Ply>, Order::max_Ply>;
	extern linePV pv_line;

	inline void clear() {
		pv_len.fill(0);

		for (auto& row : pv_line)
			row.fill(0);
	}
};


namespace Search {

	struct Results {
		int score;
		unsigned long long nodes;
	};

	extern Results search_results;

	void bestMove(int depth);

	inline void killerClear() {
		Order::killer = {};
	}

}