#include "MoveOrder.h"
#include "Zobrist.h"
#include "Evaluation.h"
#include "Search.h"


namespace Order {

	// get least valuable attacker bbs index from given attackers set
	inline size_t leastValuableAtt(U64 att, bool side) {
		for (auto pc = nWhitePawn + side; pc <= nBlackKing; pc += 2)
			if (att & BBs[pc]) return pc;

		return nEmpty;
	}

	// get initial material of piece occuping given square
	size_t getCapturedMaterial(int sq) {
		for (auto pc = nBlackPawn - game_state.turn; pc <= nBlackKing; pc += 2)
			if (bitU64(sq) & BBs[pc]) return pc;

		// en passant capture scenario
		static constexpr std::array<int, 2> ep_shift = { Compass::nort, Compass::sout };
		return (game_state.ep_sq + ep_shift[game_state.turn] == sq) ? (nBlackPawn - game_state.turn) : nEmpty;
	}

	int see(int sq) {
		std::array<int, 33> gain;

		bool side = game_state.turn;
		std::array<U64, 2> attackers;
		U64 processed = eU64;

		int i = 0;
		gain[i] = 0;
		size_t weakest_att = getCapturedMaterial(sq);
		attackers[side] = attackTo(sq, !side);

		while (attackers[side] and weakest_att < nWhiteKing) {
			i++;
			gain[i] = -gain[i - 1] + Eval::Value::piece_material[toPieceType(weakest_att)];

			weakest_att = leastValuableAtt(attackers[side], side);
			processed |= bitU64(getLS1BIndex(BBs[weakest_att] & ~processed));

			side = !side;
			attackers[side] = attackTo(sq, !side, BBs[nOccupied] ^ processed) & ~processed;
		}

		while (i >= 2) {
			gain[i - 1] = -std::max(-gain[i - 1], gain[i]);
			i--;
		}

		return gain[1];
	}

	// evaluate move
	int moveScore(const MoveItem::iMove& move, int ply, int depth) {
		const int target = move.getMask<MoveItem::iMask::TARGET>() >> 6;

		// PV move detected
		if (Search::PV::pv_line[ply][0] == move)
			return PV_SCORE;
		// distinguish between quiets and captures
		else if (move.isCapture()) {
			// evaluate good and equal captures at depth >= 5 slightly above killer moves,
			// but keep bad captures scoring less than killers
			if (depth >= 5) 
				return FIRST_KILLER_SCORE + 1 + see(move.getMask<MoveItem::iMask::TARGET>() >> 6);

			const int att = move.getMask<MoveItem::iMask::PIECE>() >> 12;
			int victim;
			const bool side = move.getMask<MoveItem::iMask::SIDE_F>();

			if (move.isEnPassant())
				return MVV_LVA::lookup[PAWN][PAWN];

			// find victim piece
			for (auto pc = nBlackPawn - side; pc <= nBlackQueen; pc += 2) {
				if (getBit(BBs[pc], target)) {
					victim = toPieceType(pc);
					break;
				}
			}
			
			return MVV_LVA::lookup[att][victim];
		}

		if (move == killer[0][ply])
			return FIRST_KILLER_SCORE;
		else if (move == killer[1][ply])
			return SECOND_KILLER_SCORE;
		static int promo_pc;
		if ((promo_pc = move.getPromo()))
			return PROMOTION_SCORE + promo_pc;

		// relative history and countermove heuristic evaluation:
		// adjust countermove bonus to ply level, since history score of a move 
		// is increasing as plies are also increasing -
		// just keep countermove bonus constans relative to history score
		const int
			pc = move.getMask<MoveItem::iMask::PIECE>() >> 12,
			prev_to = Search::prev_move.getMask<MoveItem::iMask::TARGET>() >> 6,
			prev_pc = Search::prev_move.getMask<MoveItem::iMask::PIECE>() >> 12,
			counter_bonus = (Order::countermove[prev_pc][prev_to] == move.raw()) * (4 + ply);
		
		return (RELATIVE_HISTORY_SCALE * history_moves[pc][target]) / (butterfly[pc][target] + 1) + 1 + counter_bonus;
	}

	// simple move sorting function
	void sort(MoveList& move_list, int ply, int depth) {
		std::sort(move_list.begin(), move_list.end(), [ply, depth](const MoveItem::iMove& a, const MoveItem::iMove& b) {
			return moveScore(a, ply, depth) > moveScore(b, ply, depth);
		});
	}

	// pick best based on normal moveScore() eval function
	int pickBest(MoveList& move_list, int s, int ply, int depth) {
		static MoveItem::iMove tmp;
		int cmp_score = moveScore(move_list[s], ply, depth), i_score;

		for (int i = s + 1; i < move_list.size(); i++) {
			i_score = moveScore(move_list[i], ply, depth);

			if (i_score > cmp_score) {
				cmp_score = i_score;
				tmp = move_list[i];
				move_list[i] = move_list[s];
				move_list[s] = tmp;
			}
		}

		return cmp_score;
	}

	// if capture move return it's SEE score,
	// else if promotion to queen, return promotion score, currently equal to capture score
	inline int tacticalScore(const MoveItem::iMove& move) {
		return move.isCapture() ? see(move.getMask<MoveItem::iMask::TARGET>() >> 6) : 0;
	}

	// pick best tactical move using SEE and promotion ordering
	int pickBestTactical(MoveList& capt_list, int s) {
		static MoveItem::iMove tmp;
		int cmp_score = 
			tacticalScore(capt_list[s]), i_score;

		for (int i = s + 1; i < capt_list.size(); i++) {
			i_score = tacticalScore(capt_list[i]);

			if (i_score > cmp_score) {
				cmp_score = i_score;
				tmp = capt_list[i];
				capt_list[i] = capt_list[s];
				capt_list[s] = tmp;
			}
		}

		return cmp_score;
	}

} // namespace Order
