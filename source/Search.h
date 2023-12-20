#pragma once

#include "MoveItem.h"
#include "Timer.h"
#include "MoveOrder.h"
#include <limits>

// main search class.
class mSearch {
public:
	mSearch() = default;

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

	// calculate best move using Iterative Deepening
	void bestMove(const int depth);

	Time time_data;
	mOrder move_order;
	MoveItem::iMove prev_move;

private:
	class NodesResources;

	inline int dynamicReductionLMR(const int i, const MoveItem::iMove move);
	void clearSearchHistory();

	// generate game tree, fill node resources and return positional score
	template <bool AllowNullMove = true>
	int alphaBeta(int alpha, int beta, int depth, const int ply);

	int qSearch(int alpha, int beta, const int ply);

	ULL nodes;

}; // class mSearch


extern mSearch m_search;