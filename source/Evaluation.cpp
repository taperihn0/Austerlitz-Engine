#include "Evaluation.h"
#include "MoveGeneration.h"
#include "Search.h"
#include "MoveOrder.h"
#include "Zobrist.h"
#include "Timer.h"


namespace Eval {
	
	template <enumSide SIDE>
	int properSquare(int square);

	template <>
	inline int properSquare<WHITE>(int square) {
		return mirrored_square[square];
	}

	template <>
	inline int properSquare<BLACK>(int square) {
		return square;
	}

	template <enumSide SIDE>
	int spawnScore() {
		U64 pawns = BBs[nWhitePawn + SIDE];
		int sq, eval = 0;

		while (pawns) {
			sq = popLS1B(pawns);
			eval += Value::position_score[PAWN][properSquare<SIDE>(sq)];

			// if backward pawn...
			if (!(LookUp::back_file.get(SIDE, sq) & BBs[nWhitePawn + SIDE])) {
				eval -= 7;

				// if isolated pawn...
				if (!(LookUp::n_file.get(SIDE, sq) & BBs[nWhitePawn + SIDE])) 
					// consider pawn location - center is dangerous for isolated pawn
					eval -= 10 + 5 * static_cast<bool>(bitU64(sq) & Constans::center);
			}

			// if double pawn...
			if (!(LookUp::sf_file.get(SIDE, sq) & BBs[nWhitePawn + SIDE])) eval -= 10;
			// if passed pawn...
			else if (!((LookUp::nf_file.get(SIDE, sq) | LookUp::sf_file.get(SIDE, sq)) & BBs[nBlackPawn - SIDE])) 
				eval += Value::passed_score[sq / 8];

			// if defending...
			if (PawnAttacks::anyAttackPawn<!SIDE>(bitU64(sq), BBs[nWhitePawn + SIDE])) eval += 4;
		}

		return eval;
	}

	// process pawn structure
	template <enumSide SIDE>
	int relativePawnScore() {
		int p_eval = spawnScore<SIDE>() - spawnScore<!SIDE>();

		// compare pawn islands when endgame
		if (game_state.gamePhase() == game_state.ENDGAME) {
			const U64 fileset_own = nortFill(BBs[nWhitePawn + SIDE]) & Constans::r8_rank,
				fileset_opp = nortFill(BBs[nBlackPawn - SIDE]) & Constans::r8_rank;
			p_eval += bitCount(islandsEastFile(fileset_own)) <= bitCount(islandsEastFile(fileset_opp)) ? 4 : -4;
		}

		return p_eval;
	}

	template <enumSide SIDE, enumPiece PC>
	auto spcScore(int& c_sq) -> std::enable_if_t<PC == KNIGHT, int> {
		int sq, eval = 0;
		U64 k_msk = BBs[nWhiteKnight + SIDE], opp_contr, k_att, t_att = eU64;
		const U64 opp_p_att = PawnAttacks::anyAttackPawn<!SIDE>(BBs[nBlackPawn - SIDE], UINT64_MAX);

		while (k_msk) {
			sq = popLS1B(k_msk);
			eval += Value::position_score[KNIGHT][properSquare<SIDE>(sq)];

			// penalty for squares controled by enemy pawns
			t_att |= (k_att = attack<KNIGHT>(BBs[nOccupied], sq));
			opp_contr = k_att & opp_p_att;
			eval -= 2 * bitCount(opp_contr);

			// outpos check
			if (bitU64(sq) & 
				(Constans::board_side[!SIDE] & PawnAttacks::anyAttackPawn<SIDE>(BBs[nWhitePawn + SIDE], UINT64_MAX
				& ~opp_p_att)))
				eval += 15;
		}

		// mobility: undefended minor pieces and legal moves squares
		eval -= 2 * bitCount(~t_att & (BBs[nWhiteKnight + SIDE] | BBs[nWhiteBishop + SIDE]));
		c_sq += bitCount(t_att);
		return eval;
	}

	template <enumSide SIDE, enumPiece PC>
	auto spcScore(int& c_sq) -> std::enable_if_t<PC == BISHOP, int> {
		int sq, eval = 0;
		U64 b_msk = BBs[nWhiteBishop + SIDE], t_att = eU64;

		// bishop pair bonus
		eval += (bitCount(b_msk) == 2) * 50;

		while (b_msk) {
			sq = popLS1B(b_msk);
			eval += Value::position_score[BISHOP][properSquare<SIDE>(sq)];
			t_att |= attack<BISHOP>(BBs[nOccupied], sq);
		}

		eval -= 2 * bitCount(~t_att & (BBs[nWhiteKnight + SIDE] | BBs[nWhiteBishop + SIDE]));
		c_sq += bitCount(t_att & BBs[nEmpty]);
		return eval;
	}

	template <enumSide SIDE, enumPiece PC>
	auto spcScore(int& c_sq) -> std::enable_if_t<PC == ROOK, int> {
		int sq, eval = 0;
		U64 r_msk = BBs[nWhiteRook + SIDE], r_att;
		const U64 open_f = ~fileFill(BBs[nWhitePawn] | BBs[nBlackPawn]);

		while (r_msk) {
			sq = popLS1B(r_msk);
			eval += Value::position_score[ROOK][properSquare<SIDE>(sq)];
			r_att = attack<ROOK>(BBs[nOccupied], sq);
			c_sq += bitCount(r_att & BBs[nEmpty]);

			// open file and rook connection score
			if (bitU64(sq) & open_f) eval += 10;
			if (r_att & BBs[nWhiteRook + SIDE]) eval += 10;
		}

		return eval;
	}

	template <enumSide SIDE, enumPiece PC>
	auto spcScore(int& c_sq) -> std::enable_if_t<PC == QUEEN, int> {
		int sq, eval = 0;
		U64 q_msk = BBs[nWhiteQueen + SIDE];

		while (q_msk) {
			sq = popLS1B(q_msk);
			eval += Value::position_score[QUEEN][properSquare<SIDE>(sq)];
			c_sq += bitCount(attack<QUEEN>(BBs[nOccupied], sq) & BBs[nEmpty]);
		}

		return eval;
	}

	template <enumSide SIDE>
	int relativeKingScore(int& c_sq1, int& c_sq2) {
		int sq1, sq2,
		eval = Value::position_score[KING][properSquare<SIDE>(sq1 = getLS1BIndex(BBs[nWhiteKing + SIDE]))]
			- Value::position_score[KING][properSquare<!SIDE>(sq2 = getLS1BIndex(BBs[nBlackKing - SIDE]))];
		c_sq1 += bitCount(attack<KING>(BBs[nOccupied], sq1));
		c_sq2 += bitCount(attack<KING>(BBs[nOccupied], sq2));
		return eval;
	}

	template <enumSide SIDE>
	int templEval() {
		int eval = 0, c_sq1 = 0, c_sq2 = 0;

		eval += relativePawnScore<SIDE>();
		eval += relativeKingScore<SIDE>(c_sq1, c_sq2);
		eval += spcScore<SIDE, KNIGHT>(c_sq1) - spcScore<!SIDE, KNIGHT>(c_sq2);
		eval += spcScore<SIDE, BISHOP>(c_sq1) - spcScore<!SIDE, BISHOP>(c_sq2);
		eval += spcScore<SIDE, ROOK>(c_sq1)   - spcScore<!SIDE, ROOK>(c_sq2);
		eval += spcScore<SIDE, QUEEN>(c_sq1)  - spcScore<!SIDE, QUEEN>(c_sq2);

		return game_state.material[SIDE] - game_state.material[!SIDE] + eval + 2 * (c_sq1 / (c_sq2 + 1));
	}
	
	template int templEval<WHITE>();
	template int templEval<BLACK>();

	int qSearch(int alpha, int beta, int ply) {
		if (time_data.is_time and !(Search::search_results.nodes & Search::time_check_modulo) and !time_data.checkTimeLeft()) {
			time_data.stop = true;
			return Search::time_stop_sign;
		}

		const int eval = evaluate();
		Search::search_results.nodes++;

		if (eval >= beta) return beta;
		else if (game_state.gamePhase() != game_state.ENDGAME and !isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn)
			and eval + Eval::Value::QUEEN_VALUE < alpha)
			return alpha;
		alpha = std::max(alpha, eval);

		// generate opponent capture moves
		auto capt_list = MoveGenerator::generateLegalMoves<MoveGenerator::CAPTURES>();
		const auto bbs_cpy = BBs;
		const auto gstate_cpy = game_state;
		const auto hash_cpy = hash.key;
		int score;

		// capture ordering
		Order::sort(capt_list, ply);

		//for (const auto& capt : capt_list) {
		for (const auto& move : capt_list) {
			MovePerform::makeMove(move);

			score = -qSearch(-beta, -alpha, ply + 1);

			MovePerform::unmakeMove(bbs_cpy, gstate_cpy);
			hash.key = hash_cpy;

			if (time_data.stop) 
				return Search::time_stop_sign;
			else if (score > alpha) {
				if (score >= beta) {
					return beta;
				}
				alpha = score;
			}
		}

		return alpha;
	}
}