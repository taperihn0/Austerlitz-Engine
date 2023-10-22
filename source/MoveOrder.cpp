#include "MoveOrder.h"
#include "Zobrist.h"
#include "Search.h"
#include "Evaluation.h"


namespace Order {
	
	inline size_t leastValuableAtt(U64 att) {
		if (att == eU64) 
			return nEmpty;

		for (auto pc = nWhitePawn + game_state.turn; pc <= nBlackKing; pc += 2)
			if (att & BBs[pc]) return pc;

		return nEmpty;
	}

	int see(int sq) {
		const U64 
			npin_att_d = pinnedDiagonal(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn) ^ (
				attack<BISHOP>(BBs[nOccupied], sq)
				& pinnedDiagonal(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn)
			),
			npin_att_hv = 
			pinnedHorizonVertic(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn) ^ (
				attack<ROOK>(BBs[nOccupied], sq) 
				& pinnedHorizonVertic(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn)
			);

		const U64 att = attackTo(sq, !game_state.turn) & ~(npin_att_d | npin_att_hv);

		const auto lpc = leastValuableAtt(att);
		int cap_val = 0, res = 0;

		if (lpc != nEmpty) {
			const auto bbs_cpy = BBs;
			const auto gstate_cpy = game_state;
			const auto hash_cpy = hash.key;

			for (auto pc = nBlackPawn - game_state.turn; pc <= nBlackKing; pc += 2)
				if (bitU64(sq) & BBs[pc]) cap_val = Eval::Value::piece_material[toPieceType(pc)];

			MovePerform::makeMove(MoveItem::encode<MoveItem::encodeType::CAPTURE>(
				getLS1BIndex(att & BBs[lpc]), sq, toPieceType(lpc), game_state.turn
			));

			res = cap_val - see(sq);

			MovePerform::unmakeMove(bbs_cpy, gstate_cpy);
			hash.key = hash_cpy;
		}

		return res;
	}

	int moveScore(const MoveItem::iMove& move, int ply) {
		const int target = move.getMask<MoveItem::iMask::TARGET>() >> 6;

		// pv move detected
		if (PV::pv_line[ply][0] == move)
			return pv_score;
		// distinguish between quiets and captures
		else if (move.getMask<MoveItem::iMask::CAPTURE_F>()) {
			const int att = move.getMask<MoveItem::iMask::PIECE>() >> 12;
			int victim;
			const bool side = move.getMask<MoveItem::iMask::SIDE_F>();;

			if (move.getMask<MoveItem::iMask::EN_PASSANT_F>())
				return MVV_LVA::lookup[PAWN][PAWN];

			// find victim piece
			for (auto pc = nBlackPawn - side; pc <= nBlackQueen; pc += 2) {
				if (getBit(BBs[pc], target)) {
					victim = toPieceType(pc);
					break;
				}
			}

			return MVV_LVA::lookup[att][victim] + 1000000;
		}

		// killer moves score less than basic captures
		if (move == killer[0][ply])
			return 900000;
		else if (move == killer[1][ply])
			return 800000;

		// relative history move score
		const int pc = move.getMask<MoveItem::iMask::PIECE>() >> 12;
		static constexpr int scale = 13;
		return (scale * history_moves[pc][target]) / (butterfly[pc][target] + 1) + 1;
	}

	void sort(MoveList& move_list, int ply) {
		std::sort(move_list.begin(), move_list.end(), [ply](const MoveItem::iMove& a, const MoveItem::iMove& b) {
			return moveScore(a, ply) > moveScore(b, ply);
		});
	}

	void pickBest(MoveList& move_list, int s, int ply) {
		static MoveItem::iMove tmp;

		for (int i = s + 1; i < move_list.size(); i++) {
			if (moveScore(move_list[i], ply) > moveScore(move_list[s], ply)) {
				tmp = move_list[i];
				move_list[i] = move_list[s];
				move_list[s] = tmp;
			}
		}
	}

} // namespace Order
