#pragma once

#include "MoveItem.h"
#include "MoveGeneration.h"


namespace Order {

	// Most Valuable Victim - Least Valuable Attacker lookup data structure
	namespace MVV_LVA {

		// in future: smarter mvv/lva values
		static constexpr std::array<std::array<int, 5>, 6> lookup = {{
			{ 105, 205, 305, 405, 505 }, 
			{ 104, 204, 304, 404, 504 },
			{ 103, 203, 303, 403, 503 },
			{ 102, 202, 302, 402, 502 },
			{ 101, 201, 301, 401, 501 },
			{ 100, 200, 300, 400, 500 }
		}};
	};

	// Killer Moves lookup table
	using killerLookUp = std::array<std::array<MoveItem::iMove, 100>, 2>;
	extern killerLookUp killer;
	
	// History Moves lookup table in format [side][from][to]
	using historyLookUp = std::array<std::array<std::array<int, 64>, 64>, 2>;
	extern historyLookUp history_moves;

	using butterflyLookUp = std::array<std::array<std::array<int, 64>, 64>, 2>;
	extern butterflyLookUp butterfly;

	// return value, also so called 'score' of given move
	int moveScore(const MoveItem::iMove& move, int ply);

	// sort given move list based on score of each move
	void sort(MoveList& move_list, int ply);

} // namespace Order