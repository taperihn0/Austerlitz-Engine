#include "Search.h"
#include "MoveGeneration.h"
#include "Evaluation.h"

#define _SEARCH_DEBUG false

namespace Search {

	static constexpr int
		low_bound = -5000000,
		high_bound = 5000000;

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
		// use fully-legal moves generator
		const auto move_list = MoveGenerator::generateLegalMoves();

		// no legal moves detected
		if (!move_list.size()) {
			// is this situation a check?
			if (isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn)) {
				return low_bound + 100 - Depth;
			} 
			// stalemate case
			return 0;
		}

		const auto bbs_cpy = BBs;
		const auto gstate_cpy = game_state;
		int score;

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
				if (Root) {
					best_move = move;
				}
			}
		}

		// fail low cutoff (return best option)
		return alpha;
	}

#define CALL(d) alphaBeta<true, d>(low_bound, high_bound); \
				break;

	// display best move according to search algorithm
	MoveItem::iMove bestMove(int depth) {
		assert(depth > 0 && "Unvalid depth size");

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

		return best_move;
	}

#undef CALL
}