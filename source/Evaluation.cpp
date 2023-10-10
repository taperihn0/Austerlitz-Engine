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
	int evalPawnStructure() {
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
		}

		return eval;
	}

	template <enumSide SIDE>
	int templEval() {
		U64 piece_bb;
		enumPiece epiece;

		// process pawn structure
		int pos_eval = evalPawnStructure<SIDE>() - evalPawnStructure<!SIDE>();

		// own position score
		for (auto piece = nWhiteKnight + SIDE; piece <= nBlackKing; piece += 2) {
			piece_bb = BBs[piece];
			epiece = toPieceType(piece);

			while (piece_bb) 
				pos_eval += Value::position_score[epiece][properSquare<SIDE>(popLS1B(piece_bb))];
		}

		// opponent position score
		for (auto piece = nBlackKnight - SIDE; piece <= nBlackKing; piece += 2) {
			piece_bb = BBs[piece];
			epiece = toPieceType(piece);

			while (piece_bb) 
				pos_eval -= Value::position_score[epiece][properSquare<!SIDE>(popLS1B(piece_bb))];
		}

		return pos_eval + game_state.material[SIDE] - game_state.material[!SIDE];
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