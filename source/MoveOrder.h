#pragma once

#include "MoveItem.h"
#include "MoveGeneration.h"


namespace Order {
	
	static constexpr int
		max_Ply = 128,
		pv_score = 1500000;

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
	using killerLookUp = std::array<std::array<MoveItem::iMove, Order::max_Ply>, 2>;
	extern killerLookUp killer;
	
	// History Moves lookup table in format [piece][to]
	using historyLookUp = std::array<std::array<int, 64>, 12>;
	extern historyLookUp history_moves;

	// Butterfly board of format [piece][to]
	using butterflyLookUp = std::array<std::array<int, 64>, 12>;
	extern butterflyLookUp butterfly;

	// return value, also so called 'score' of given move
	int moveScore(const MoveItem::iMove& move, int ply);

	int see(int sq);

	// sort given move list based on score of each move
	void sort(MoveList& move_list, int ply);

	// swap best move so it's on the i'th place
	void pickBest(MoveList& move_list, int s, int ply);

	int pickBestSEE(MoveList& capt_list, int s);

} // namespace Order


namespace InitState {

	// clear butterfly
	inline void clearButterfly() {
		for (auto& x : Order::butterfly) x.fill(0);
	}
	
	// clear history heuristic table
	inline void clearHistory() {
		for (auto& x : Order::history_moves) x.fill(0);
	}
}