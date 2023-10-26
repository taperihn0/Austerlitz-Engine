#pragma once

#include "MoveItem.h"
#include "MoveGeneration.h"
#include "Search.h"


namespace Order {

	// Most Valuable Victim - Least Valuable Attacker lookup data structure
	namespace MVV_LVA {

		// in future: smarter mvv/lva values
		static constexpr std::array<std::array<int, 5>, 6> lookup = {{
			{ 1050, 2050, 3050, 4050, 5050 }, 
			{ 1040, 2040, 3040, 4040, 5040 },
			{ 1030, 2030, 3030, 4030, 5030 },
			{ 1020, 2020, 3020, 4020, 5020 },
			{ 1010, 2010, 3010, 4010, 5010 },
			{ 1000, 2000, 3000, 4000, 5000 }
		}};
	};

	// Killer Moves lookup table [killer_index][ply_index]
	using killerLookUp = std::array<std::array<MoveItem::iMove, 128>, 2>;
	extern killerLookUp killer;
	
	// History Moves lookup table in format [piece][to]
	using historyLookUp = std::array<std::array<int, 64>, 12>;
	extern historyLookUp history_moves;

	// Butterfly board of format [piece][to]
	using butterflyLookUp = std::array<std::array<int, 64>, 12>;
	extern butterflyLookUp butterfly;

	// butterfly-based countermove lookup
	extern butterflyLookUp countermove;

	// return value, also so called 'score' of given move
	int moveScore(const MoveItem::iMove& move, int ply);

	/* move sorting */

	// sort given move list based on score of each move
	void sort(MoveList& move_list, int ply);

	/* move ordering techniques */

	// swap best move so it's on the i'th place
	void pickBest(MoveList& move_list, int s, int ply);

	// pick best capture based on Static Exchange Evaluation
	int pickBestSEE(MoveList& capt_list, int s);

} // namespace Order


namespace InitState {

	// clear butterfly
	inline void clearButterfly() {
		for (auto& x : Order::butterfly) x.fill(0);
		for (auto& x : Order::countermove) x.fill(0);
	}
	
	// clear history heuristic table
	inline void clearHistory() {
		for (auto& x : Order::history_moves) x.fill(0);
	}

	// clear killer history
	inline void clearKiller() {
		for (auto& x : Order::killer) x.fill(0);
	}
}