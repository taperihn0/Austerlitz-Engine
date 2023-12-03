#include "Search.h"
#include "MoveGeneration.h"
#include "Evaluation.h"
#include "Zobrist.h"
#include "MoveOrder.h"


namespace Search {

	// aspiration window reduction size
	constexpr int asp_margin = static_cast<int>(0.45 * Eval::Value::PAWN_VALUE);

	MoveItem::iMove bm;

	enum plyNode {
		ROOT = 0,
		ROOT_CHILD = 1,
	};

	// search data stucture
	struct SearchResults {
		int score;
		ULL nodes;
	} search_results;

	// node data variables
	struct NodeDataAggregator {
		MoveItem::iMove my_prev, node_best_move;
		BitBoardsSet bbs_cpy;
		gState gstate_cpy;
		U64 hash_cpy;
		int score, to, pc, prev_to, prev_pc, m_score;
		HashEntry::Flag hash_flag;
		bool checking_move;
		MoveList ml;
		size_t mcount;
	};

	// global pre-alloc resource variable of every main seach node indexed by [node_ply]
	std::array<NodeDataAggregator, max_Ply> node;

	// forward declaration
	int qSearch(int alpha, int beta, int ply);

	enum depthNode {
		LEAF = 0,
		FRONTIER = 1,
		PRE_FRONTIER = 2,
		PRE_PRE_FRONTIER = 3
	};

	inline void initNodeData(int ply) {
		node[ply].my_prev = prev_move;
		node[ply].bbs_cpy = BBs;
		node[ply].gstate_cpy = game_state;
		node[ply].hash_cpy = hash.key;
		node[ply].prev_to = prev_move.getMask<MoveItem::iMask::TARGET>() >> 6;
		node[ply].hash_flag = HashEntry::Flag::HASH_ALPHA;
		if (ply) node[ply].node_best_move = MoveItem::iMove::no_move;
	}

	// negamax algorithm as an extension of minimax algorithm with alpha-beta pruning framework
	template <bool AllowNullMove = true>
	int alphaBeta(int alpha, int beta, int depth, int ply) {
		// (nodes & check_modulo == 0) is an alternative operation to (nodes % check_modulo == 0)
		if (time_data.is_time and !(search_results.nodes & time_check_modulo) and !time_data.checkTimeLeft()) {
			time_data.stop = true;
			return time_stop_sign;
		}
		else if (ply != ROOT and (rep_tt.isRepetition() or game_state.is50moveDraw()))
			return draw_score;

		const bool pv_node = beta - alpha > 1;

		// do not use tt in root
		static int tt_score;
		if (ply != ROOT and HashEntry::isValid(tt_score = tt.read(alpha, beta, depth, ply)))
			return tt_score;
		// break condition and quiescence search
		else if (depth <= LEAF) {
			node[ply].score = qSearch(alpha, beta, ply);

			if (!time_data.stop)
				tt.write(0, node[ply].score,
					node[ply].score == alpha ? HashEntry::Flag::HASH_ALPHA :
					node[ply].score == beta ? HashEntry::Flag::HASH_BETA :
					HashEntry::Flag::HASH_EXACT, ply, MoveItem::iMove::no_move);

			return node[ply].score;
		}

		search_results.nodes++;
		const bool incheck = isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn);

		// Null Move Pruning method
		if constexpr (AllowNullMove) {
			if (!incheck and depth >= 3 and !game_state.isPawnEndgame()) {
				static constexpr int R = 2;

				const auto ep_cpy = game_state.ep_sq;
				node[ply].hash_cpy = hash.key;

				rep_tt.posRegister();
				MovePerform::makeNull();

				// set allow_null_move to false - prevent from double move passing, it makes no sense then
				node[ply].score = -alphaBeta<false>(-beta, -beta + 1, depth - 1 - R, ply + 1);

				MovePerform::unmakeNull(node[ply].hash_cpy, ep_cpy);
				rep_tt.count--;

				if (node[ply].score >= beta) return beta;
			}
		}

		// use fully-legal moves generator
		MoveGenerator::generateLegalMoves<MoveGenerator::LEGAL>(node[ply].ml);
		node[ply].mcount = node[ply].ml.size();

		// no legal moves detected - checkmate or stealmate
		if (!node[ply].mcount)
			return incheck ? mate_score + ply : draw_score;
		// check extension
		else if (incheck) {
			depth++;

			// single-response extra time
			if (node[ply].mcount == 1 and time_data.is_time and time_data.this_move > 300_ms
				and time_data.this_move < time_data.left / 10)
				time_data.this_move += 185_ms;
		}

		initNodeData(ply);
		bool is_pruned = false;

		for (int c = 0, i = 0; i < node[ply].mcount; i++, is_pruned = false) {
			// move ordering
			node[ply].m_score = Order::pickBest(node[ply].ml, i, ply, depth);
			const auto& move = node[ply].ml[i];

			// futility pruning and reduction routines
			if (i >= 1 and node[ply].mcount >= 8 and !incheck and move.isWeak(node[ply].m_score, Order::FIRST_KILLER_SCORE)
				and (alpha > mate_comp or alpha < -mate_comp) and (beta > mate_comp or beta < -mate_comp) and ply != ROOT) {
				
				// margins for each depth
				static constexpr int futility_margin = 80, ext_margin = 450, razor_margin = 950;

				// pure futility pruning at frontiers
				if (depth == FRONTIER
					and Eval::evaluate(alpha - futility_margin, alpha - futility_margin + 1) <= alpha - futility_margin)
					return alpha;
				// extended futility pruning at pre-frontiers
				else if (depth == PRE_FRONTIER
					and Eval::evaluate(alpha - ext_margin, alpha - ext_margin + 1) <= alpha - ext_margin)
					return alpha;
				// razoring reduction at pre-pre-frontiers
				else if (depth == PRE_PRE_FRONTIER
					and Eval::evaluate(alpha - razor_margin, alpha - razor_margin + 1) <= alpha - razor_margin)
					depth = PRE_FRONTIER;

			} // recapture extra time
			else if (node[ply].prev_to == (move.getMask<MoveItem::iMask::TARGET>() >> 6)
				and time_data.is_time and time_data.this_move > 300_ms
				and time_data.this_move < time_data.left / 12) {
				time_data.this_move += 50_ms;
			}

			rep_tt.posRegister();
			MovePerform::makeMove(move);
			prev_move = move;
			node[ply].checking_move = isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn);
			
			// late move pruning using previous fail-low moves in nullwindow search and non-late moves count
			if (c > 7 and node[ply].mcount >= 10 and depth >= 4 and !node[ply].checking_move and move.isWeak(node[ply].m_score, Order::FIRST_KILLER_SCORE))
				is_pruned = true;
			// if PV move is already processed, save time by checking uninteresting moves 
			// using null window and late move reduction (PV Search) -
			// however, if such 'late' node fails low, it's a sign we are offered a good move (score > alpha)
			else if (i >= 1) {
				// late move reduction in null move search
				if (depth >= 3 and !node[ply].checking_move and !incheck and move.isWeak(node[ply].m_score, Order::FIRST_KILLER_SCORE))
					node[ply].score = -alphaBeta(-alpha - 1, -alpha, depth - 2 - (i >= 6), ply + 1);
				else node[ply].score = alpha + 1;
				
				if (node[ply].score > alpha) {
					node[ply].score = -alphaBeta(-beta, -alpha, depth - 1, ply + 1);
					c++;
				}
			}
			else node[ply].score = -alphaBeta(-beta, -alpha, depth - 1, ply + 1);

			MovePerform::unmakeMove(node[ply].bbs_cpy, node[ply].gstate_cpy);
			rep_tt.count--;
			hash.key = node[ply].hash_cpy;
			prev_move = node[ply].my_prev;

			if (time_data.stop) {
				tt.write(depth, alpha, node[ply].hash_flag, ply, node[ply].node_best_move);
				return time_stop_sign;
			}
			else if (is_pruned)
				break;

			// register move appearance in butterfly board
			node[ply].to = move.getMask<MoveItem::iMask::TARGET>() >> 6;
			node[ply].pc = move.getMask<MoveItem::iMask::PIECE>() >> 12;
			Order::butterfly[node[ply].pc][node[ply].to] += depth;

			if (node[ply].score > alpha) {
				//if (ply == ROOT)
				//	bm = move;
				node[ply].node_best_move = move;

				// fail hard beta-cutoff
				if (node[ply].score >= beta) {
					if (!move.isCapture()) {
						// store killer move
						Order::killer[1][ply] = Order::killer[0][ply];
						Order::killer[0][ply] = move;

						// store move as a history move
						Order::history_moves[node[ply].pc][node[ply].to] += depth * depth;

						node[ply].prev_pc = node[ply].my_prev.getMask<MoveItem::iMask::PIECE>() >> 12;

						// store a countermove
						Order::countermove[node[ply].prev_pc][node[ply].prev_to] = move.raw();
					}

					tt.write(depth, beta, HashEntry::Flag::HASH_BETA, ply, move);
					return beta;
				}

				node[ply].hash_flag = HashEntry::Flag::HASH_EXACT;
				alpha = node[ply].score;
			}
		}

		tt.write(depth, alpha, node[ply].hash_flag, ply, node[ply].node_best_move);
		// fail-low cutoff (return best option)
		return alpha;
	}

	// quiescence search - protect from dangerous consequences of horizon effect
	int qSearch(int alpha, int beta, int ply) {
		if (time_data.is_time and !(search_results.nodes & time_check_modulo) and !time_data.checkTimeLeft()) {
			time_data.stop = true;
			return time_stop_sign;
		} 

		const int eval = Eval::evaluate(alpha - Eval::Value::QUEEN_VALUE, beta);
		search_results.nodes++;

		if (eval >= beta) 
			return beta;

		const bool incheck = isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn),
			is_endgame = game_state.gamePhase() == gState::ENDGAME;

		// delta pruning
		if (!is_endgame and !incheck and eval + Eval::Value::QUEEN_VALUE <= alpha)
			return alpha;
		alpha = std::max(alpha, eval);

		MoveGenerator::generateLegalMoves<MoveGenerator::TACTICAL>(node[ply].ml);
		node[ply].bbs_cpy = BBs;
		node[ply].gstate_cpy = game_state;
		node[ply].hash_cpy = hash.key;

		// losing material indication flag
		const bool minus_matdelta = (game_state.material[game_state.turn] - game_state.material[!game_state.turn]) < 0;

		for (int i = 0; i < node[ply].ml.size(); i++) {
			// capture ordering
			node[ply].m_score = Order::pickBestTactical(node[ply].ml, i);
			const auto& move = node[ply].ml[i];

			if (!is_endgame and !incheck and !move.isPromo()) {
				// equal captures pruning margin
				static constexpr int equal_margin = 120;

				// bad captures pruning
				if (node[ply].m_score <= -200)
					return alpha;
				// equal captures pruning if losing material
				else if (minus_matdelta and !node[ply].m_score and eval + equal_margin <= alpha)
					return alpha;
			}

			MovePerform::makeMove(move);

			node[ply].score = -qSearch(-beta, -alpha, ply + 1);

			MovePerform::unmakeMove(node[ply].bbs_cpy, node[ply].gstate_cpy);
			hash.key = node[ply].hash_cpy;

			if (time_data.stop)
				return time_stop_sign;
			else if (node[ply].score > alpha) {
				if (node[ply].score >= beta) return beta;
				alpha = node[ply].score;
			}
		}

		return alpha;
	}

	inline void clearHistory() {
		search_results.nodes = 0;
		InitState::clearKiller();
		InitState::clearButterfly();
		InitState::clearHistory();
	}

	// display best move according to search algorithm
	void bestMove(int depth) {
		assert(depth > 0 && "Unvalid depth");

		// cleaning
		clearHistory();
		tt.increaseAge();
		
		int lbound = low_bound,
			hbound = high_bound,
			curr_dpt = 1,
			prev_score = search_results.score;
		long long time;
		MoveItem::iMove ponder;

		time_data.start = now();

		// aspiration window search
		while (curr_dpt <= depth) {
			ponder = MoveItem::iMove::no_move;
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

			OS  << " depth " << curr_dpt
				<< " nodes " << search_results.nodes
				<< " time " << (time = sinceStart_ms(time_data.start))
				<< " nps " << static_cast<int>(search_results.nodes / (1. * (time + 1) / 1000))
				<< " pv ";

			tt.recreatePV(curr_dpt++, node[ROOT].node_best_move, ponder);

			if (time_data.stop 
				or (time_data.is_time and 5 * sinceStart_ms(time_data.start) / 2 > time_data.this_move))
				break;

			prev_score = search_results.score;
		}

		OS << "bestmove ";
		node[ROOT].node_best_move.print() << ' ';

		if (ponder != MoveItem::iMove::no_move) {
			OS << "ponder ";
			ponder.print() << ' ';
		}

		OS << '\n';
	}

} // namespace Search