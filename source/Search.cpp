#include "Search.h"
#include "MoveGeneration.h"
#include "Evaluation.h"
#include "Zobrist.h"
#include <limits>

#define _SEARCH_DEBUG false

namespace Search {

	// set some safe bufor to prevent overflows
	static constexpr int
		low_bound = std::numeric_limits<int>::min() + 1000000,
		high_bound = std::numeric_limits<int>::max() - 1000000;

	// negamax algorithm as an extension of minimax algorithm with alpha-beta pruning framework
	int alphaBeta(int alpha, int beta, int depth, int ply) {
		// break recursion and return evalutation of current position
		if (depth <= 0) {
			// init PV table lenght
			PV::pv_len[ply] = 0;
			return Eval::qSearch(alpha, beta, ply);
		}

		// use fully-legal moves generator
		auto move_list = MoveGenerator::generateLegalMoves<MoveGenerator::LEGAL>();

		// do not use tt in root (no best move in pv_line set)
		if (ply) {
			int tt_score;
			if (HashEntry::isValid(tt_score = tt.read(alpha, beta, depth)))
				return tt_score;
		}

		search_results.nodes++;

		// no legal moves detected
		if (!move_list.size()) {
			// is this situation a checkmate?
			if (isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn))
				return low_bound + 1000 - depth;
			// stalemate case
			return 0;
		}

		const auto bbs_cpy = BBs;
		const auto gstate_cpy = game_state;
		const auto hash_cpy = hash.key;
		int score, to, pc;
		HashEntry::Flag hash_flag = HashEntry::Flag::HASH_ALPHA;

		bool pv_left = true;

		// move ordering
		Order::sort(move_list, ply);

		for (int i = 0; i < move_list.size(); i++) {
			const auto& move = move_list[i];
#if _SEARCH_DEBUG
			move.print() << '\n';
#endif
			MovePerform::makeMove(move);

			// principal variation search
			if (pv_left and !i) {
				score = -alphaBeta(-beta, -alpha, depth - 1, ply + 1);
			}
			// if pv is still left, save time by checking uninteresting moves using null window
			// if such node fails low, it's a sign we are offered good move (score > alpha)
			// this technic is just Late Move Reduction
			else if (pv_left and i) {
				if (
					ply >= 2
					and !isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn)
					and !isSquareAttacked(getLS1BIndex(BBs[nBlackKing - game_state.turn]), !game_state.turn)
					and !move.getMask<MoveItem::iMask::CAPTURE_F>() 
					and move.getMask<MoveItem::iMask::PROMOTION>() != QUEEN
					)
					score = -alphaBeta(-alpha - 1, -alpha, depth - 2, ply + 1);
				else score = alpha + 1;

				if (score > alpha) {
					score = -alphaBeta(-alpha - 1, -alpha, depth - 1, ply + 1);

					if (score > alpha and score < beta)
						score = -alphaBeta(-beta, -alpha, depth - 1, ply + 1);
				}
			} 
			// if pv was found (assuming all other nodes are bad)
			else {
				score = -alphaBeta(-alpha - 1, -alpha, depth - 1, ply + 1);

				if (score > alpha and score < beta)
					score = -alphaBeta(-beta, -alpha, depth - 1, ply + 1);
			}

			MovePerform::unmakeMove(bbs_cpy, gstate_cpy);
			hash.key = hash_cpy;

			// register move appearance in butterfly board
			to = move.getMask<MoveItem::iMask::TARGET>() >> 6;
			pc = move.getMask<MoveItem::iMask::PIECE>() >> 12;
			Order::butterfly[pc][to] += depth;

			if (score > alpha) {
				// fail hard beta-cutoff
				if (score >= beta) {
					if (!move.getMask<MoveItem::iMask::CAPTURE_F>()) {
						// store killer move
						Order::killer[1][ply] = Order::killer[0][ply];
						Order::killer[0][ply] = move;
						// store move as a history move
						Order::history_moves[pc][to] += depth * depth;
					}

					tt.write(depth, beta, HashEntry::Flag::HASH_BETA);
					return beta;
				}

				hash_flag = HashEntry::Flag::HASH_EXACT;
				alpha = score;

				pv_left = false;
				PV::pv_line[ply][0] = move;

				for (int i = 0; i < PV::pv_len[ply + 1]; i++)
					PV::pv_line[ply][i + 1] = PV::pv_line[ply + 1][i];

				PV::pv_len[ply] = PV::pv_len[ply + 1] + 1;
			}
		}

		// save current position in tt
		tt.write(depth, alpha, hash_flag);

		// fail low cutoff (return best option)
		return alpha;
	}

#define CALL(d) search_results.score = -alphaBeta<d>(low_bound, high_bound, 0); \
				break;

	// display best move according to search algorithm
	void bestMove(int depth) {
		assert(depth > 0 && "Unvalid depth");

		// cleaning
		search_results.nodes = 0;
		Search::clearKiller();
		InitState::clearButterfly();
		PV::clear();
		InitState::clearHistory();


		for (int d = 1; d <= depth; d++) {
			search_results.score = -alphaBeta(low_bound, high_bound, d, 0);

			OS << "info score cp " << Search::search_results.score << " depth " << d
				<< " nodes " << Search::search_results.nodes
				<< " pv ";

			for (int cnt = 0; cnt < PV::pv_len[0]; cnt++)
				PV::pv_line[0][cnt].print() << ' ';
			OS << '\n';
		}

		OS << "bestmove ";
		PV::pv_line[0][0].print() << ' ';

		if (depth >= 2) {
			OS << "ponder ";
			PV::pv_line[1][0].print() << ' ';
		}

		OS << '\n';
	}

#undef CALL
}