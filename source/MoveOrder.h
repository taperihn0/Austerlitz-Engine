#pragma once

#include "MoveItem.h"
#include "MoveGeneration.h"

// main move ordering resources
class mOrder {
public:
	mOrder() = default;

	enum MoveEvalConstans {
		HASH_SCORE = 15000,

		PROMOTION_SCORE = 950,

		FIRST_KILLER_SCORE = 900,
		SECOND_KILLER_SCORE = 890,

		EQUAL_CAPTURE_SCORE = FIRST_KILLER_SCORE + 1,
		RECAPTURE_BONUS = 5,

		RELATIVE_HISTORY_SCALE = 13,
	};

	// Most Valuable Victim - Least Valuable Attacker lookup data structure
	static constexpr std::array<std::array<int, 5>, 6> mvv_lva = {{
		{ 1050, 2050, 3050, 4050, 5050 },
		{ 1040, 2040, 3040, 4040, 5040 },
		{ 1038, 2038, 3038, 4038, 5038 },
		{ 1015, 2015, 3015, 4015, 5015 },
		{ 1009, 2009, 3009, 4009, 5009 },
		{ 1000, 2000, 3000, 4000, 5000 }
	}};

	// Killer Moves lookup table [killer_index][ply_index]
	using killerLookUp = std::array<std::array<MoveItem::iMove, 128>, 2>;
	killerLookUp killer;

	// History Moves lookup table in format [piece][to]
	using historyLookUp = std::array<std::array<int, 64>, 12>;
	historyLookUp history_moves;

	// Butterfly board of format [piece][to]
	using butterflyLookUp = std::array<std::array<int, 64>, 12>;
	butterflyLookUp butterfly;

	// butterfly-based countermove lookup
	butterflyLookUp countermove;

	static inline int recaptureBonus(const MoveItem::iMove move, const MoveItem::iMove prev_move) noexcept {
		return (prev_move.isCapture() and move.getTarget() == prev_move.getTarget()) * RECAPTURE_BONUS;
	}

	static inline int seeScore(const MoveItem::iMove capt) {
		return EQUAL_CAPTURE_SCORE + see(capt.getTarget());
	}

	static inline int promotionScore(const MoveItem::iMove promo_move) noexcept {
		return PROMOTION_SCORE + promo_move.getPromo();
	}

	// relative history and countermove heuristic evaluation:
	// adjust countermove bonus to ply level, since history score of a move 
	// is increasing as plies are also increasing -
	// just keep countermove bonus constans relative to history score
	inline int quietScore(const MoveItem::iMove move, const MoveItem::iMove prev_move, const int target, const int ply) noexcept {
		const int pc = move.getPiece(), counter_bonus = isCounterMove(move, prev_move) * (ply + 4);
		return RELATIVE_HISTORY_SCALE * history_moves[pc][target] / (butterfly[pc][target] + 1) + counter_bonus + 1;
	}

	inline bool isCounterMove(const MoveItem::iMove move, const MoveItem::iMove prev_move) {
		const int prev_to = prev_move.getTarget(), prev_pc = prev_move.getPiece();
		return countermove[prev_pc][prev_to] == move.raw();
	}

	// if move is capture return it's SEE score,
	// else if promotion to queen, return promotion score, currently equal to zero, sane as equal capture score
	static inline int tacticalScore(const MoveItem::iMove move) {
		return move.isCapture() ? mOrder::see(move.getTarget()) : 0;
	}

	// return value, also so called 'score' of a given move
	int moveScore(
		const MoveItem::iMove move, const int ply, const int depth, 
		const MoveItem::iMove tt_move, const MoveItem::iMove prev_move
	);

	// Static Exchange Evaluation for captures
	static int see(const int sq);

	// swap best move so it's on the i'th place
	int pickBest(
		MoveList& move_list, const int s, const int ply, 
		const int depth, const MoveItem::iMove prev_move
	);

	// pick best capture based on Static Exchange Evaluation
	int pickBestTactical(MoveList& capt_list, const int s);

	// clear butterfly
	inline void clearButterfly() {
		for (auto& x : butterfly) x.fill(0);
	}

	// clear countermove table
	inline void clearCountermove() {
		for (auto& x : countermove) x.fill(0);
	}

	// clear history heuristic table
	inline void clearHistory() {
		for (auto& x : history_moves) x.fill(0);
	}

	// clear killer history
	inline void clearKiller() {
		for (auto& x : killer) x.fill(0);
	}

}; // class mOrder