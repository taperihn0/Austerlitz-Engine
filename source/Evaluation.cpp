#include "Evaluation.h"
#include "MoveGeneration.h"
#include "Search.h"
#include "MoveOrder.h"
#include "Zobrist.h"


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
		int eval = 0, sq;
		U64 piece_bb;
		enumPiece epiece;

		// own material and position score
		for (auto piece = nWhitePawn + SIDE; piece <= nBlackKing; piece += 2) {
			piece_bb = BBs[piece];
			epiece = toPieceType(piece);

			while (piece_bb) {
				sq = popLS1B(piece_bb);
				eval +=
					Value::piece_material[epiece] + Value::position_score[epiece][properSquare<SIDE>(sq)];
			}
		}

		// opponent evaluation score
		for (auto piece = nBlackPawn - SIDE; piece <= nBlackKing; piece += 2) {
			piece_bb = BBs[piece];
			epiece = toPieceType(piece);

			while (piece_bb) {
				sq = popLS1B(piece_bb);
				eval -=
					Value::piece_material[epiece] + Value::position_score[epiece][properSquare<!SIDE>(sq)];
			}
		}

		return eval;
	}

	template int templEval<WHITE>();
	template int templEval<BLACK>();

	int qSearch(int alpha, int beta, int ply) {
		const int eval = evaluate();
		Search::search_results.nodes++;

		if (eval >= beta) 
			return beta;
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
		for (int i = 0; i < capt_list.size(); i++) {
			MovePerform::makeMove(capt_list[i]);

			score = -qSearch(-beta, -alpha, ply + 1);

			MovePerform::unmakeMove(bbs_cpy, gstate_cpy);
			hash.key = hash_cpy;

			if (score >= beta)
				return beta;
			alpha = std::max(alpha, score);
		}

		return alpha;
	}
}