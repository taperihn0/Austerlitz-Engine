#include "MoveOrder.h"
#include "Zobrist.h"
#include "Evaluation.h"
#include "Search.h"


namespace Order {
	
	static constexpr int
		pv_score = 15000,
		relative_history_scale = 13;

	// get least valuable attacker bbs index
	inline size_t leastValuableAtt(U64 att, bool side) {
		for (auto pc = nWhitePawn + side; pc <= nBlackKing; pc += 2)
			if (att & BBs[pc]) return pc;

		return nEmpty;
	}

	// get capture material
	int getCapturedMaterial(int sq) {
		int cap_val = 0;

		for (auto pc = nBlackPawn - game_state.turn; pc <= nBlackKing; pc += 2)
			if (bitU64(sq) & BBs[pc])
				cap_val = Eval::Value::piece_material[toPieceType(pc)];

		return cap_val;
	}

	int see(int sq) {
		std::array<int, 32> gain;
		bool side = game_state.turn;
		std::array<U64, 2> attackers;
		U64 processed = eU64, single_att;

		attackers[side] = attackTo(sq, !side);

		int i = 0;
		gain[i] = getCapturedMaterial(sq);

		auto weakest_att = leastValuableAtt(attackers[side], side);
		processed = bitU64(getLS1BIndex(BBs[weakest_att]));
		attackers[side] ^= processed;

		side = !side;
		attackers[side] = attackTo(sq, !side, BBs[nOccupied] ^ processed);

		while (attackers[side]) {
			i++;
			gain[i] = -gain[i - 1] + Eval::Value::piece_material[toPieceType(weakest_att)];

			auto weakest_att = leastValuableAtt(attackers[side], side);
			single_att = bitU64(getLS1BIndex(BBs[weakest_att] & ~processed));

			// pin detection
			if (isBySliderAttacked(getLS1BIndex(BBs[nWhiteKing + side]), 
				side, BBs[nOccupied] ^ (processed | single_att))) {
				attackers[side] ^= single_att;
				i--;
				break;
			}

			processed |= single_att;

			side = !side;
			attackers[side] = attackTo(sq, !side, BBs[nOccupied] ^ processed) & ~processed;
		}

		while (i >= 1) {
			gain[i - 1] = -std::max(-gain[i - 1], gain[i]);
			i--;
		}

		return gain[0];
	}

	// evaluate move
	int moveScore(const MoveItem::iMove& move, int ply) {
		const int target = move.getMask<MoveItem::iMask::TARGET>() >> 6;

		// pv move detected
		if (Search::PV::pv_line[ply][0] == move)
			return pv_score;
		// distinguish between quiets and captures
		else if (move.getMask<MoveItem::iMask::CAPTURE_F>()) {
			const int att = move.getMask<MoveItem::iMask::PIECE>() >> 12;
			int victim;
			const bool side = move.getMask<MoveItem::iMask::SIDE_F>();

			if (move.getMask<MoveItem::iMask::EN_PASSANT_F>())
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

		// killer moves score less than basic captures
		if (move == killer[0][ply])
			return 900;
		else if (move == killer[1][ply])
			return 895;

		// relative history move score
		const int
			pc = move.getMask<MoveItem::iMask::PIECE>() >> 12,
			prev_to = Search::prev_move.getMask<MoveItem::iMask::TARGET>() >> 6,
			prev_pc = Search::prev_move.getMask<MoveItem::iMask::PIECE>() >> 12,
			counter_bonus = static_cast<bool>(ply and Order::countermove[prev_pc][prev_to] == move.raw()) * (4 + ply);

		return (relative_history_scale * history_moves[pc][target]) / (butterfly[pc][target] + 1) + 1 + counter_bonus;
	}

	// move sorting
	void sort(MoveList& move_list, int ply) {
		std::sort(move_list.begin(), move_list.end(), [ply](const MoveItem::iMove& a, const MoveItem::iMove& b) {
			return moveScore(a, ply) > moveScore(b, ply);
		});
	}

	// pick best based on normal moveScore() eval function
	void pickBest(MoveList& move_list, int s, int ply) {
		static MoveItem::iMove tmp;
		int cmp_score = moveScore(move_list[s], ply), i_score;

		for (int i = s + 1; i < move_list.size(); i++) {
			i_score = moveScore(move_list[i], ply);

			if (i_score > cmp_score) {
				cmp_score = i_score;
				tmp = move_list[i];
				move_list[i] = move_list[s];
				move_list[s] = tmp;
			}
		}
	}

	// pick best using SEE
	int pickBestSEE(MoveList& capt_list, int s) {
		static MoveItem::iMove tmp;
		int cmp_score = 
			see(capt_list[s].getMask<MoveItem::iMask::TARGET>() >> 6), i_score;

		for (int i = s + 1; i < capt_list.size(); i++) {
			i_score = see(capt_list[i].getMask<MoveItem::iMask::TARGET>() >> 6);

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
