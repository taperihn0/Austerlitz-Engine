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

	// return true when marked attacker cannot perform legal capture of piece on 'sq' square
	// else return false - 'blocked' means 'pinned' in such detection
	inline bool blockedAttacker(int sq, U64 occ, int att_sq, bool side, int k_sq) {
		// get possible pinner of piece
		const U64 pinner = pinnersPiece(k_sq, occ, BBs[nWhite + side] & occ, side) &
			attack<QUEEN>(occ, att_sq) & occ;

		// if no pinner found for attacker then return false, else check whether capture is still legal move
		return pinner and !((inBetween(getLS1BIndex(pinner), k_sq) | pinner) & bitU64(sq));
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

	enum MoveScoreParams {
		PV_SCORE = 15000,
		KILLER_1_SCORE = 900,
		KILLER_2_SCORE = 890,
		RELATIVE_HISTORY_SCALE = 13,
	};

	// evaluate move
	int moveScore(const MoveItem::iMove& move, int ply) {
		const int target = move.getMask<MoveItem::iMask::TARGET>() >> 6;

		// pv move detected
		if (Search::PV::pv_line[ply][0] == move)
			return PV_SCORE;
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
			return KILLER_1_SCORE;
		else if (move == killer[1][ply])
			return KILLER_2_SCORE;
		static int promo;
		if ((promo = move.getMask<MoveItem::iMask::PROMOTION>() >> 20))
			return promo;

		// relative history move score
		const int
			pc = move.getMask<MoveItem::iMask::PIECE>() >> 12,
			prev_to = Search::prev_move.getMask<MoveItem::iMask::TARGET>() >> 6,
			prev_pc = Search::prev_move.getMask<MoveItem::iMask::PIECE>() >> 12,
			counter_bonus = (Order::countermove[prev_pc][prev_to] == move.raw()) * (4 + ply);
		
		return (RELATIVE_HISTORY_SCALE * history_moves[pc][target]) / (butterfly[pc][target] + 1) + 1 + counter_bonus;
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
