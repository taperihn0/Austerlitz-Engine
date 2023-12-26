#include "MoveOrder.h"
#include "Zobrist.h"
#include "Evaluation.h"
#include "Search.h"


// get least valuable attacker bbs index from given attackers set
inline size_t leastValuableAtt(U64 att, bool side) {
	for (auto pc = nWhitePawn + side; pc <= nBlackKing; pc += 2)
		if (att & BBs[pc]) return pc;

	return nEmpty;
}

// get initial material of piece occuping given square
inline size_t getCapturedMaterial(int sq) {
	for (auto pc = nBlackPawn - game_state.turn; pc <= nBlackKing; pc += 2)
		if (bitU64(sq) & BBs[pc]) return pc;

	// en passant capture scenario
	static constexpr std::array<int, 2> ep_shift = { Compass::nort, Compass::sout };
	return (game_state.ep_sq + ep_shift[game_state.turn] == sq) ? (nBlackPawn - game_state.turn) : nEmpty;
}


int mOrder::see(const int sq) {
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
		gain[i] = -gain[i - 1] + Eval::params.piece_material[toPieceType(weakest_att)];

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
int mOrder::moveScore(
	const MoveItem::iMove move, const int ply, const int depth, 
	const MoveItem::iMove tt_move, const MoveItem::iMove prev_move
) {
	const int target = move.getTarget();

	// PV move detected
	if (move == tt_move)
		return HASH_SCORE;
	// distinguish between quiets and captures
	else if (move.isCapture()) {
		// evaluate good and equal captures at depth >= 5 slightly above killer moves,
		// but keep bad captures scoring less than killers
		if (depth >= 5) 
			return seeScore(move);
		else if (move.isEnPassant()) 
			return mvv_lva[PAWN][PAWN];

		const int att = move.getPiece();
		int victim;
		const bool side = move.getSide();

		// find victim piece
		for (auto pc = nBlackPawn - side; pc <= nBlackQueen; pc += 2) {
			if (getBit(BBs[pc], target)) {
				victim = toPieceType(pc);
				break;
			}
		}

		// recapture moves are treated slightly better than same victim-attacker captures
		const int recapture_bonus = recaptureBonus(move, prev_move);

		return mvv_lva[att][victim] + recapture_bonus;
	}

	if (move == killer[0][ply])
		return FIRST_KILLER_SCORE;
	else if (move == killer[1][ply])
		return SECOND_KILLER_SCORE;
	else if (move.getPromo())
		return promotionScore(move);

	return quietScore(move, prev_move, target, ply);
}

// pick best based on normal moveScore() eval function
int mOrder::pickBest(
	MoveList& move_list, const int s, const int ply, 
	const int depth, const MoveItem::iMove prev_move
) {
	static MoveItem::iMove tmp;
	const MoveItem::iMove tt_move = tt.hashMove();
	int cmp_score = moveScore(move_list[s], ply, depth, tt_move, prev_move), i_score;

	for (int i = s + 1; i < move_list.size(); i++) {
		i_score = moveScore(move_list[i], ply, depth, tt_move, prev_move);

		if (i_score > cmp_score) {
			cmp_score = i_score;
			tmp = move_list[i];
			move_list[i] = move_list[s];
			move_list[s] = tmp;
		}
	}

	return cmp_score;
}

// pick best tactical move using SEE and promotion ordering
int mOrder::pickBestTactical(MoveList& capt_list, const int s) {
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
