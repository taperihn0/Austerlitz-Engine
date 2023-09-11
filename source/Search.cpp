#include "Search.h"
#include "MoveGeneration.h"
#include "Evaluation.h"


namespace Search {

	MoveItem::iMove best_move;

	// break recursion and return evalutation of current position
	template <bool Root, int Depth>
	inline auto alphaBeta(int alpha, int beta) -> std::enable_if_t<!Depth, int> {
		return Eval::evaluate();
	}

	// negamax algorithm as an extension of minimax algorithm with alpha-beta pruning framework
	// only root node sets best move
	template <bool Root, int Depth>
	auto alphaBeta(int alpha, int beta) -> std::enable_if_t<Depth, int> {
		const auto move_list = MoveGenerator::generateLegalMoves();
		const auto bbs_cpy = BBs;
		const auto gstate_cpy = game_state;
		int score;

		for (const auto& move : move_list) {
			MovePerform::makeMove(move);
			score = -alphaBeta<false, Depth - 1>(-beta, -alpha);
			MovePerform::unmakeMove(bbs_cpy, gstate_cpy);

			// fail hard beta-cutoff
			if (score >= beta) {
				return beta;
			}
			else if (score > alpha) {
				alpha = score;
				if (Root) {
					best_move = move;
				}
			}
		}

		// fail low cutoff (return best option)
		return alpha;
	}

	// display best move according to search algorithm
	MoveItem::iMove bestMove() {
		static constexpr int
			low_bound = -5000000,
			high_bound = 5000000;

		alphaBeta<true, 2>(low_bound, high_bound);
		return best_move;
	}
}