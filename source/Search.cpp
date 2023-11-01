#include "Search.h"
#include "MoveGeneration.h"
#include "Evaluation.h"
#include "Zobrist.h"
#include "MoveOrder.h"


namespace Search {

	 // aspiration window reduction size
	constexpr int asp_margin = static_cast<int>(0.45 * Eval::Value::PAWN_VALUE);

	// search data stucture
	struct SearchResults {
		int score;
		ULL nodes;
	} search_results;

	// global pre-alloc move list indexed by [ply][move_index]
	std::array<MoveList, max_Ply> ml;

	// forward declaration
	int qSearch(int alpha, int beta, int ply);

	enum depthNode {
		LEAF_NODE = 0,
		FRONTIER = 1,
		PRE_FRONTIER = 2
	};

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
		int tt_score;
		if (ply and HashEntry::isValid(tt_score = tt.read(alpha, beta, depth, ply)))
			return tt_score;
		// break condition and quiescence search
		else if (depth <= LEAF_NODE) {
			// init PV table lenght
			PV::pv_len[ply] = 0;
			const int qscore = qSearch(alpha, beta, ply);
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
		if constexpr (AllowNullMove) {
			if (!incheck and depth >= 3 and !game_state.isPawnEndgame()) {
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
		}

		// use fully-legal moves generator
		MoveGenerator::generateLegalMoves<MoveGenerator::LEGAL>(ml[ply]);
		const size_t mcount = ml[ply].size();

		// no legal moves detected - checkmate or stealmate
		if (!mcount)
			return incheck ? mate_score + ply : draw_score;
		// single-response extra time
		else if (incheck and mcount == 1) {
			depth++;

			// single-response extra time
			if (time_data.is_time and time_data.this_move > 300_ms
				and time_data.this_move < time_data.left / 10)
				time_data.this_move += 185_ms;
		}
		// check extension
		else if (incheck) depth++;

		const MoveItem::iMove my_prev = prev_move;
		const BitBoardsSet bbs_cpy = BBs;
		const gState gstate_cpy = game_state;
		const U64 hash_cpy = hash.key;
		int score, to, pc, prev_to = my_prev.getMask<MoveItem::iMask::TARGET>() >> 6, prev_pc;
		HashEntry::Flag hash_flag = HashEntry::Flag::HASH_ALPHA;
		bool checking_move;

		for (int i = 0; i < mcount; i++) {

			// move ordering
			Order::pickBest(ml[ply], i, ply);
			const auto& move = ml[ply][i];

			if (i >= 1 and mcount >= 8 and !incheck and !move.getMask<MoveItem::iMask::CAPTURE_F>()
				and (move.getMask<MoveItem::iMask::PROMOTION>() >> 20) != QUEEN
				and (alpha > mate_comp or alpha < -mate_comp) and (beta > mate_comp or beta < -mate_comp)) {

				// margin for pre-frontier node and for pre-pre-frontier node
				static constexpr int futility_margin = 80, ext_margin = 450;

				// pure futility pruning at pre-frontier nodes
				if (depth == FRONTIER
					and Eval::evaluate(alpha - futility_margin, Search::high_bound) <= alpha - futility_margin)
					return alpha;
				// extended futility pruning at pre-pre-frontier nodes
				else if (depth == PRE_FRONTIER
					and Eval::evaluate(alpha - ext_margin, Search::high_bound) <= alpha - ext_margin)
					return alpha;

			} // recapture extra time
			else if (ply and prev_to == (move.getMask<MoveItem::iMask::TARGET>() >> 6)
				and time_data.is_time and time_data.this_move > 300_ms
				and time_data.this_move < time_data.left / 12) {
				time_data.this_move += 50_ms;
			}

			rep_tt.posRegister();
			MovePerform::makeMove(move);
			prev_move = move;
			checking_move = isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn);

			// if pv is still left, save time by checking uninteresting moves using null window
			// if such node fails low, it's a sign we are offered good move (score > alpha)
			if (i > 1) {
				// late move reduction
				if (depth >= 3 and !checking_move
					and !isSquareAttacked(getLS1BIndex(BBs[nBlackKing - game_state.turn]), !game_state.turn)
					and !move.getMask<MoveItem::iMask::CAPTURE_F>()
					and (move.getMask<MoveItem::iMask::PROMOTION>() >> 20) != QUEEN)
					score = -alphaBeta(-alpha - 1, -alpha, depth - 2, ply + 1);
				else score = alpha + 1;

				// null window search
				if (score > alpha) {
					score = -alphaBeta(-alpha - 1, -alpha, depth - 1, ply + 1);

					if (score > alpha and score < beta)
						score = -alphaBeta(-beta, -alpha, depth - 1, ply + 1);
				}
			}
			else score = -alphaBeta(-beta, -alpha, depth - 1, ply + 1);

			MovePerform::unmakeMove(bbs_cpy, gstate_cpy);
			rep_tt.count--;
			hash.key = hash_cpy;
			prev_move = my_prev;

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

						prev_pc = my_prev.getMask<MoveItem::iMask::PIECE>() >> 12;

						// store countermove
						Order::countermove[prev_pc][prev_to] = move.raw();
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

	// quiescence search - protect from dangerous consequences of horizon effect
	int qSearch(int alpha, int beta, int ply) {
		if (time_data.is_time and !(search_results.nodes & time_check_modulo) and !time_data.checkTimeLeft()) {
			time_data.stop = true;
			return time_stop_sign;
		}

		const int eval = Eval::evaluate(low_bound, beta);
		search_results.nodes++;

		if (eval >= beta) return beta;
		else if (game_state.gamePhase() != gState::ENDGAME
			and !isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn)
			and eval + Eval::Value::QUEEN_VALUE <= alpha)
			return alpha;
		alpha = std::max(alpha, eval);

		// generate opponent capture moves
		MoveGenerator::generateLegalMoves<MoveGenerator::CAPTURES>(ml[ply]);
		const BitBoardsSet bbs_cpy = BBs;
		const gState gstate_cpy = game_state;
		const U64 hash_cpy = hash.key;
		int score, see_score;

		for (int i = 0; i < ml[ply].size(); i++) {

			// capture ordering
			see_score = Order::pickBestSEE(ml[ply], i);
			const auto& move = ml[ply][i];

			// bad captures pruning
			if (i >= 1 and see_score < 0)
				return alpha;

			MovePerform::makeMove(move);

			score = -qSearch(-beta, -alpha, ply + 1);

			MovePerform::unmakeMove(bbs_cpy, gstate_cpy);
			hash.key = hash_cpy;

			if (time_data.stop)
				return time_stop_sign;
			else if (score > alpha) {
				if (score >= beta)
					return beta;
				alpha = score;
			}
		}

		return alpha;
	}

	inline void clearHistory() {
		search_results.nodes = 0;
		InitState::clearKiller();
		InitState::clearButterfly();
		PV::clear();
		InitState::clearHistory();
	}

	// display best move according to search algorithm
	void bestMove(const int depth) {
		assert(depth > 0 && "Unvalid depth");

		// cleaning
		clearHistory();

		int lbound = low_bound,
			hbound = high_bound,
			curr_dpt = 1,
			prev_score = search_results.score;
		long long time;

		time_data.start = now();
		
		// aspiration window search
		while (curr_dpt <= depth) {
			prev_move = 0;
			search_results.score = alphaBeta<false>(lbound, hbound, curr_dpt, 0);

			if (search_results.score == time_stop_sign)
				search_results.score = prev_score;
			// failed aspiration window search
			else if (search_results.score <= lbound or search_results.score >= hbound) {
				lbound = low_bound, hbound = high_bound;
				continue;
			}

			// success - expand bounds for next search
			lbound = search_results.score - asp_margin;
			hbound = search_results.score + asp_margin;
			
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
				<< " time " << (time = sinceStart_ms(time_data.start))
				<< " nps " << static_cast<int>(search_results.nodes / (1. * (time + 1) / 1000))
				<< " pv ";

			for (int cnt = 0; cnt < PV::pv_len[0]; cnt++)
				PV::pv_line[0][cnt].print() << ' ';
			OS << '\n';

			if (time_data.stop 
				or (time_data.is_time and 5 * sinceStart_ms(time_data.start) / 2 > time_data.this_move))
				break;

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