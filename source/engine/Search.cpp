#include "Search.h"
#include "MoveGeneration.h"
#include "Evaluation.h"
#include "Zobrist.h"
#include "MoveOrder.h"
#include "../tuning/TexelTuning.h"


enum plyNode {
	ROOT = 0,
	ROOT_CHILD = 1,
};

enum depthNode {
	LEAF = 0,
	FRONTIER = 1,
	PRE_FRONTIER = 2,
	PRE_PRE_FRONTIER = 3
};

class mSearch::NodesResources {
public:
	NodesResources() = default;

	class NodeDataEntry {
	public:
		NodeDataEntry() = default;

		void initNodeData(const MoveItem::iMove& prev_move) {
			my_prev = prev_move;
			bbs_cpy = BBs;
			gstate_cpy = game_state;
			hash_cpy = hash.key;
			prev_to = my_prev.getTarget();
			prev_pc = my_prev.getPiece();
			hash_flag = HashEntry::Flag::HASH_ALPHA;
			node_best_move = MoveItem::iMove::no_move;
		}

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

	inline NodeDataEntry& operator[](const int ply) {
		assert(ply < max_Ply);
		return node_data[ply];
	}

private:
	// global pre-alloc resource variable of every main seach node indexed by [node_ply]
	std::array<NodeDataEntry, max_Ply> node_data;
} node;

inline int mSearch::dynamicReductionLMR(const int i, const MoveItem::iMove move) {
	return 1 + (i >= 6 and !move_order.isCounterMove(move, prev_move));
}

// negamax algorithm as an extension of minimax algorithm with alpha-beta pruning framework
template <bool AllowNullMove>
int mSearch::alphaBeta(int alpha, int beta, int depth, const int ply) {
	// (nodes & check_modulo == 0) is an alternative operation to (nodes % check_modulo == 0)
	if (ply != ROOT and time_data.is_time and !(nodes & time_check_modulo) and !time_data.checkTimeLeft()) {
		time_data.stop = true;
		return time_stop_sign;
	}
	else if (ply != ROOT and (rep_tt.isRepetition() or game_state.is50moveDraw()))
		return draw_score;

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

	nodes++;
	const bool incheck = isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn);

	// Null Move Pruning 
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

	node[ply].initNodeData(prev_move);
	bool is_pruned = false;

	for (int fail_low_count = 0, i = 0; i < node[ply].mcount; i++, is_pruned = false) {
		// move ordering
		node[ply].m_score = move_order.pickBest(node[ply].ml, i, ply, depth, prev_move);
		const auto& move = node[ply].ml[i];

		// futility pruning and razoring routine
		if (i >= 1 and ply != ROOT and node[ply].mcount >= 8 and !incheck and move.getPromo() != QUEEN
			and (!move.isCapture() or node[ply].m_score < mOrder::FIRST_KILLER_SCORE)
			and alpha > mate_comp and alpha < -mate_comp and beta > mate_comp and beta < -mate_comp) {

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
		} 
		// recapture extra time - although it looks strange, it makes engine a little bit stronger
		else if (node[ply].prev_to == (move.getTarget())
			and time_data.is_time and time_data.this_move > 300_ms
			and time_data.this_move < time_data.left / 12) {
			time_data.this_move += 50_ms;
		}

		rep_tt.posRegister();
		MovePerform::makeMove(move);
		prev_move = move;
		node[ply].checking_move = isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn);
			
		// late move pruning using previous fail-low moves in nullwindow search and non-late moves count
		// based on an assumption that probability of finding a good move after processing many good moves before
		// decreases significantly.
		if (fail_low_count > 7 and node[ply].mcount >= 10 and depth >= 4 and !node[ply].checking_move
			and (!move.isCapture() or node[ply].m_score < mOrder::FIRST_KILLER_SCORE) and move.getPromo() != QUEEN)
			is_pruned = true;
		else {
			// if PV move (hash move) is already processed, save time by checking uninteresting moves 
			// using null window and late move reduction (PV Search) -
			// however, if such 'late' node fails low, it's a sign we are offered a good move (score > alpha)
			if (node[ply].m_score < mOrder::HASH_SCORE and depth >= 3 and !node[ply].checking_move and !incheck
				and (!move.isCapture() or node[ply].m_score < mOrder::FIRST_KILLER_SCORE)
				and move.getPromo() != QUEEN) {
				// late move reduction in null move search
				//const int depth_reduction = dynamicReductionLMR(i, move);
				const int depth_reduction = 1 + (i >= 6);
				node[ply].score = -alphaBeta(-alpha - 1, -alpha, depth - 1 - depth_reduction, ply + 1);
			}
			else node[ply].score = alpha + 1;

			if (node[ply].score > alpha) {
				node[ply].score = -alphaBeta(-beta, -alpha, depth - 1, ply + 1);
				fail_low_count++;
			}
		}

		MovePerform::unmakeMove(node[ply].bbs_cpy, node[ply].gstate_cpy);
		rep_tt.count--;
		hash.key = node[ply].hash_cpy;
		prev_move = node[ply].my_prev;

		if (time_data.stop) {
			MoveItem::iMove tt_move = node[ply].node_best_move;

			if (ply == ROOT and node[ply].node_best_move == MoveItem::iMove::no_move) {
				node[ply].node_best_move = node[ply].ml[0];
				tt_move = MoveItem::iMove::no_move;
			}

			tt.write(depth, alpha, node[ply].hash_flag, ply, tt_move);
			return time_stop_sign;
		}
		else if (is_pruned)
			break;

		// register move appearance in butterfly board
		node[ply].to = move.getTarget();
		node[ply].pc = move.getPiece();
		move_order.butterfly[node[ply].pc][node[ply].to] += depth;

		if (node[ply].score > alpha) {
			node[ply].node_best_move = move;

			// fail hard beta-cutoff
			if (node[ply].score >= beta) {
				if (!move.isCapture()) {
					// store killer move
					move_order.killer[1][ply] = move_order.killer[0][ply];
					move_order.killer[0][ply] = move;

					// store move as a history move
					move_order.history_moves[node[ply].pc][node[ply].to] += depth * depth;

					// store a countermove
					move_order.countermove[node[ply].prev_pc][node[ply].prev_to] = move.raw();
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

template int mSearch::alphaBeta<false>(int alpha, int beta, int depth, const int ply);
template int mSearch::alphaBeta<true>(int alpha, int beta, int depth, const int ply);

// quiescence search - protect from dangerous consequences of horizon effect
int mSearch::qSearch(int alpha, int beta, const int ply) {
	if (time_data.is_time and !(nodes & time_check_modulo) and !time_data.checkTimeLeft()) {
		time_data.stop = true;
		return time_stop_sign;
	} 

	const int eval = Eval::evaluate(alpha - Eval::params.piece_material[QUEEN] - 1, beta);
	nodes++;

	if (eval >= beta) 
		return beta;

	const bool incheck = isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn),
		is_endgame = game_state.gamePhase() == gState::ENDGAME;

	// delta pruning
	if (!is_endgame and !incheck and eval + Eval::params.piece_material[QUEEN] <= alpha)
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
		node[ply].m_score = move_order.pickBestTactical(node[ply].ml, i);
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

inline void mSearch::clearSearchHistory() {
	nodes = 0;
	move_order.clearButterfly();
	move_order.clearHistory();
	move_order.clearKiller();
}

// display best move according to search algorithm
MoveItem::iMove mSearch::bestMove(const int depth) {
	assert(depth > 0 && "Unvalid depth");

	// cleaning
	clearSearchHistory();
	tt.increaseAge();
		
	int lbound = low_bound, hbound = high_bound,
		curr_dpt = 1, score, prev_score = HashEntry::no_score;
	long long time = 0;
	MoveItem::iMove ponder;

	time_data.start = now();

	// aspiration window reduction size
	const int asp_margin = static_cast<int>(0.45 * Eval::params.piece_material[PAWN]);

	// aspiration window search
	while (curr_dpt <= depth) {
		ponder = MoveItem::iMove::no_move;
		score = alphaBeta<false>(lbound, hbound, curr_dpt, 0);

		if (score == time_stop_sign)
			score = prev_score;
		// failed aspiration window search
		else if (score <= lbound or score >= hbound) {
			lbound = low_bound, hbound = high_bound;
			continue;
		}

		// success - expand bounds for next search
		lbound = score - asp_margin;
		hbound = score + asp_margin;
			
		// (-) search result - opponent checkmating
		if (score >= mate_score and score < mate_comp)
			OS << "info score mate " << (mate_score - score) / 2 - 1;
		// (+) search result - engine checkmating
		else if (score > -mate_comp and score <= -mate_score)
			OS << "info score mate " << (-score - mate_score) / 2 + 1;
		// no checkmate
		else OS << "info score cp " << score;

		OS  << " depth " << curr_dpt
			<< " nodes " << nodes
			<< " time " << (time = sinceStart_ms(time_data.start))
			<< " nps " << static_cast<int>(nodes / (1. * (time + 1) / 1000))
			<< " pv ";

		tt.recreatePV(curr_dpt++, node[ROOT].node_best_move, ponder);

		if (time_data.stop 
			or (time_data.is_time and 5 * sinceStart_ms(time_data.start) / 2 > time_data.this_move))
			break;

		prev_score = score;
	}

	OS << "bestmove ";
	node[ROOT].node_best_move.print() << ' ';

	if (ponder != MoveItem::iMove::no_move) {
		OS << "ponder ";
		ponder.print() << ' ';
	}

	OS << '\n';

#if COLLECT_POSITION_DATA
	if (score > mSearch::mate_comp and score < -mSearch::mate_comp)
		game_collector.registerPosition(BBs.getFEN());
#endif

	return node[ROOT].node_best_move;
}
