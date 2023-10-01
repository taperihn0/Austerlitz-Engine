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
	int alphaBeta(int alpha, int beta, int depth, int ply, bool allow_nullm = true) {
		// break recursion and return evalutation of current position
		if (depth <= 0) {
			// init PV table lenght
			PV::pv_len[ply] = 0;
			return Eval::qSearch(alpha, beta, ply);
		}

		// do not use tt in root (no best move in pv_line set)
		static int tt_score;
		if (ply and HashEntry::isValid(tt_score = tt.read(alpha, beta, depth)))
			return tt_score;

		search_results.nodes++;
		const bool incheck = isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn);

		// Null Move Pruning method
		if (allow_nullm and !incheck and depth >= 2) {
			static constexpr int R = 2;

			const auto ep_cpy = game_state.ep_sq;
			const auto hash_cpy = hash.key;
			MovePerform::makeNull();
			
			// set allow_null_move to false - prevent from double move passing, it makes no sense then
			const int score = -alphaBeta(-beta, -beta + 1, depth - 1 - R, ply + 1, false);

			MovePerform::unmakeNull(hash_cpy, ep_cpy);
			if (score >= beta) return beta;
		}

		// use fully-legal moves generator
		auto move_list = MoveGenerator::generateLegalMoves<MoveGenerator::LEGAL>();

		// no legal moves detected
		if (!move_list.size())
			              // checkmate : stealmate score
			return incheck ? low_bound + 1000 - depth : 0;
		// reverse futility pruning
		else if (depth == 1 and !incheck) {
			static constexpr int margin = Eval::Value::PAWN_VALUE;
			bool no_capt = true;

			for (const auto& m : move_list) {
				if (m.getMask<MoveItem::iMask::CAPTURE_F>()) {
					no_capt = false;
					break;
				}
			}

			if (no_capt and Eval::evaluate() - margin >= beta)
				return beta;
		}

		const auto bbs_cpy = BBs;
		const auto gstate_cpy = game_state;
		const auto hash_cpy = hash.key;
		static int score, to, pc;
		HashEntry::Flag hash_flag = HashEntry::Flag::HASH_ALPHA;

		// move ordering
		Order::sort(move_list, ply);

		for (int i = 0; i < move_list.size(); i++) {
			const auto& move = move_list[i];
#if _SEARCH_DEBUG
			move.print() << '\n';
#endif
			MovePerform::makeMove(move);

			// if pv is still left, save time by checking uninteresting moves using null window
			// if such node fails low, it's a sign we are offered good move (score > alpha)
			if (i) {
				if (
					depth >= 3
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
			else {
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

				PV::pv_line[ply][0] = move;

				for (int j = 0; j < PV::pv_len[ply + 1]; j++)
					PV::pv_line[ply][j + 1] = PV::pv_line[ply + 1][j];

				PV::pv_len[ply] = PV::pv_len[ply + 1] + 1;
			}
		}

		// save current position in tt
		tt.write(depth, alpha, hash_flag);

		// fail low cutoff (return best option)
		return alpha;
	}

	// display best move according to search algorithm
	void bestMove(int depth) {
		assert(depth > 0 && "Unvalid depth");

		// cleaning
		search_results.nodes = 0;
		Search::clearKiller();
		InitState::clearButterfly();
		PV::clear();
		InitState::clearHistory();

		// aspiration window reduction size
		static constexpr int window = static_cast<int>(0.45 * Eval::Value::PAWN_VALUE);

		int lbound = low_bound,
			hbound = high_bound,
			curr_dpt = 1;
		
		// aspiration window search
		while (curr_dpt <= depth) {
			search_results.score = alphaBeta(lbound, hbound, curr_dpt, 0, false);

			// fail
			if (search_results.score <= lbound or search_results.score >= hbound) {
				lbound = low_bound, hbound = high_bound;
				continue;
			}
			
			// success - expand bounds for next search
			lbound = search_results.score - window, hbound = search_results.score + window;

			OS << "info score cp " << Search::search_results.score << " depth " << curr_dpt++
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
}