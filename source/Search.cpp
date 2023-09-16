#include "Search.h"
#include "MoveGeneration.h"
#include "Evaluation.h"

#define _SEARCH_DEBUG false

namespace Search {

	static constexpr int
		low_bound = -5000000,
		high_bound = 5000000;

	MoveItem::iMove best_move, best_so_far;

	// break recursion and return evalutation of current position
	template <bool Root, int Depth>
	inline auto alphaBeta(int alpha, int beta) -> std::enable_if_t<!Depth, int> {
		search_results.nodes++;
		return Eval::qSearch(alpha, beta);
	}

	// negamax algorithm as an extension of minimax algorithm with alpha-beta pruning framework
	// only root node sets best move
	template <bool Root, int Depth>
	auto alphaBeta(int alpha, int beta) -> std::enable_if_t<Depth, int> {
		// use fully-legal moves generator
		const auto move_list = MoveGenerator::generateLegalMoves<MoveGenerator::LEGAL>();
		const auto legal = move_list.size();

		// no legal moves detected
		if (!legal) {
			// is this situation a check?
			if (isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn))
				return low_bound + 100 - Depth;
			// stalemate case
			return 0;
		}

		search_results.nodes += legal;

		const auto bbs_cpy = BBs;
		const auto gstate_cpy = game_state;
		int score, old_alpha = alpha;

		for (const auto& move : move_list) {
#if _SEARCH_DEBUG
			move.print() << '\n';
#endif

			MovePerform::makeMove(move);
			score = -alphaBeta<false, Depth - 1>(-beta, -alpha);
			MovePerform::unmakeMove(bbs_cpy, gstate_cpy);

			// fail hard beta-cutoff
			if (score >= beta) {
				return beta;
			}
			else if (score > alpha) {
				alpha = score;

				if constexpr (Root)
					best_so_far = move;
			}
		}

		if constexpr (Root) {
			if (old_alpha != alpha)
				best_move = best_so_far;
		}

		// fail low cutoff (return best option)
		return alpha;
	}

#define CALL(d) search_results.score = -alphaBeta<true, d>(low_bound, high_bound); \
				break;

	// display best move according to search algorithm
	MoveItem::iMove bestMove(int depth) {
		assert(depth > 0 && "Unvalid depth size");
		search_results.nodes = 0;

		switch (depth) {
		case 1:  CALL(1);
		case 2:  CALL(2);
		case 3:  CALL(3);
		case 4:  CALL(4);
		case 5:  CALL(5);
		case 6:  CALL(6);
		case 7:  CALL(7);
		case 8:  CALL(8);
		case 9:  CALL(9);
		case 10: CALL(10);
		default:
			TOGUI_S << "Depth not supported";
			break;
		}

		if (!game_state.turn ^ (depth % 2 == 0))
			search_results.score = -search_results.score;

		return best_move;

		//game_state.turn == BLACK and depth % 2 == 0 -> (-) 1 1
		//game_state.turn == BLACK and depth % 2 != 0 -> (+) 1 0
		//game_state.turn == WHITE and depth % 2 == 0 -> (+) 0 1
		//game_state.turn == WHITE and depth % 2 != 0 -> (-) 0 0
	}

#undef CALL
}