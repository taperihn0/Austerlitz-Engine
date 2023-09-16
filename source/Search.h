#pragma once

#include "MoveItem.h"


namespace Search {

	struct Results {
		int score;
		unsigned long long nodes;
	};

	extern Results search_results;

	MoveItem::iMove bestMove(int depth);

}