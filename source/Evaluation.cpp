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
	int templEval() {
		int pos_eval = 0;
		U64 piece_bb;
		enumPiece epiece;

		// own position score
		for (auto piece = nWhitePawn + SIDE; piece <= nBlackKing; piece += 2) {
			piece_bb = BBs[piece];
			epiece = toPieceType(piece);

			while (piece_bb) 
				pos_eval += Value::position_score[epiece][properSquare<SIDE>(popLS1B(piece_bb))];
		}

		// opponent position score
		for (auto piece = nBlackPawn - SIDE; piece <= nBlackKing; piece += 2) {
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