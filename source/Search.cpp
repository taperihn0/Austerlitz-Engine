#include "Search.h"
#include "MoveGeneration.h"
#include "Evaluation.h"
#include "Zobrist.h"
#include "Timer.h"


namespace Search {

	// negamax algorithm as an extension of minimax algorithm with alpha-beta pruning framework
	template <bool AllowNullMove = true>
	int alphaBeta(int alpha, int beta, int depth, int ply) {

		// (nodes & check_modulo == 0) is an alternative operation to (nodes % check_modulo == 0)
		if (time_data.is_time and !(search_results.nodes & time_check_modulo) and !time_data.checkTimeLeft()) {
			time_data.stop = true;
			return time_stop_sign;
		}
		else if (ply and (rep_tt.isRepetition() or game_state.is50moveDraw()))
			return draw_score;
		
		// do not use tt in root
		static int tt_score;
		if (ply and HashEntry::isValid(tt_score = tt.read(alpha, beta, depth, ply)))
			return tt_score;
		// break condition and quiescence search
		else if (depth <= 0) {
			// init PV table lenght
			PV::pv_len[ply] = 0;
			const int qscore = Eval::qSearch(alpha, beta, ply);
			if (!time_data.stop)
				tt.write(0, qscore, 
					qscore == alpha ? HashEntry::Flag::HASH_ALPHA : 
					qscore == beta ? HashEntry::Flag::HASH_BETA : 
					HashEntry::Flag::HASH_EXACT, ply);
			return qscore;
		}

		search_results.nodes++;
		const bool incheck = isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn);

		// Null Move Pruning method
		if (AllowNullMove and !incheck and depth >= 3 and game_state.gamePhase() != game_state.ENDGAME) {
			static constexpr int R = 2;

			const auto ep_cpy = game_state.ep_sq;
			const auto hash_cpy = hash.key;

			rep_tt.posRegister();
			MovePerform::makeNull();

			// set allow_null_move to false - prevent from double move passing, it makes no sense then
			const int score = -alphaBeta<false>(-beta, -beta + 1, depth - 1 - R, ply + 1);

			MovePerform::unmakeNull(hash_cpy, ep_cpy);
			rep_tt.count--;

			if (score >= beta) return beta;
		}

		// use fully-legal moves generator
		auto move_list = MoveGenerator::generateLegalMoves<MoveGenerator::LEGAL>();
		const size_t mcount = move_list.size();

		// no legal moves detected - checkmate or stealmate
		if (!mcount)
			return incheck ? mate_score + ply : draw_score;
		else if (incheck and mcount == 1) // single-response extension
			depth++;

		const auto bbs_cpy = BBs;
		const auto gstate_cpy = game_state;
		const auto hash_cpy = hash.key;
		static int score, to, pc;
		HashEntry::Flag hash_flag = HashEntry::Flag::HASH_ALPHA;

		for (int i = 0; i < mcount; i++) {

			// move ordering
			Order::pickBest(move_list, i, ply);
			const auto& move = move_list[i];

			// futility pruning
			static constexpr int margin = Eval::Value::PAWN_VALUE;
			if (i > 1 and mcount >= 8 and !move.getMask<MoveItem::iMask::CAPTURE_F>()
				and depth == 1 and !isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn)
				and beta < -mate_comp and alpha > mate_comp
				and Eval::evaluate(alpha - margin, Search::high_bound) <= alpha - margin) {
				return alpha;
			}

			rep_tt.posRegister();
			MovePerform::makeMove(move);

			// if pv is still left, save time by checking uninteresting moves using null window
			// if such node fails low, it's a sign we are offered good move (score > alpha)
			if (i > 1) {
				// late move reduction
				if (
					depth >= 3
					and !isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn)
					and !isSquareAttacked(getLS1BIndex(BBs[nBlackKing - game_state.turn]), !game_state.turn)
					and !move.getMask<MoveItem::iMask::CAPTURE_F>()
					and (move.getMask<MoveItem::iMask::PROMOTION>() >> 20) != QUEEN
					)
					score = -alphaBeta(-alpha - 1, -alpha, depth - 2, ply + 1);
				else score = alpha + 1;

				// null window search
				if (score > alpha) {
					score = -alphaBeta(-alpha - 1, -alpha, depth - 1, ply + 1);

					if (score > alpha and score < beta)
						score = -alphaBeta(-beta, -alpha, depth - 1, ply + 1);
				}
			}
			else
				score = -alphaBeta(-beta, -alpha, depth - 1, ply + 1);

			MovePerform::unmakeMove(bbs_cpy, gstate_cpy);
			rep_tt.count--;
			hash.key = hash_cpy;

			if (time_data.stop) return time_stop_sign;

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

					tt.write(depth, beta, HashEntry::Flag::HASH_BETA, ply);
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

		tt.write(depth, alpha, hash_flag, ply);
		// fail low cutoff (return best option)
		return alpha;
	}

	inline void clearHistory() {
		search_results.nodes = 0;
		Search::clearKiller();
		InitState::clearButterfly();
		PV::clear();
		InitState::clearHistory();
	}

	// display best move according to search algorithm
	void bestMove(const int depth) {
		assert(depth > 0 && "Unvalid depth");

		// cleaning
		clearHistory();

		// aspiration window reduction size
		static constexpr int window = static_cast<int>(0.45 * Eval::Value::PAWN_VALUE);

		int lbound = low_bound,
			hbound = high_bound,
			curr_dpt = 1,
			prev_score;

		time_data.start = now();
		
		// aspiration window search
		while (curr_dpt <= depth) {
			search_results.score = alphaBeta<false>(lbound, hbound, curr_dpt, 0);

			// fail
			if (search_results.score <= lbound or search_results.score >= hbound) {
				lbound = low_bound, hbound = high_bound;
				continue;
			}

			// success - expand bounds for next search
			lbound = search_results.score - window, hbound = search_results.score + window;
			
			if (search_results.score == Search::time_stop_sign)
				search_results.score = prev_score;

			// (-) search result - opponent checkmating
			if (search_results.score < mate_comp and search_results.score > mate_score)
				OS << "info score mate " << (mate_score - search_results.score) / 2 - 1;
			// (+) search result - engine checkmating
			else if (search_results.score > -mate_comp and search_results.score < -mate_score)
				OS << "info score mate " << (-search_results.score - mate_score) / 2 + 1;
			// no checkmate
			else OS << "info score cp " << search_results.score;

			OS  << " depth " << curr_dpt++
				<< " nodes " << search_results.nodes
				<< " time " << sinceStart_ms(time_data.start)
				<< " pv ";

			for (int cnt = 0; cnt < PV::pv_len[0]; cnt++)
				PV::pv_line[0][cnt].print() << ' ';
			OS << '\n';

			if (time_data.stop) break;
			prev_score = search_results.score;
		}

		OS << "bestmove ";
		PV::pv_line[0][0].print() << ' ';

		if (depth >= 2) {
			OS << "ponder ";
			PV::pv_line[1][0].print() << ' ';
		}

		OS << '\n';
	}

} // namespace Search