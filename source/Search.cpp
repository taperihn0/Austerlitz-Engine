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

	// break recursion and return evalutation of current position
	template <bool Root, int Depth, int Ply>
	inline auto alphaBeta(int alpha, int beta) -> std::enable_if_t<!Depth, int> {
		// init PV table lenght
		std::get<Ply>(PV::pv_len) = 0;
		return Eval::qSearch(alpha, beta, Ply);
	}

	// negamax algorithm as an extension of minimax algorithm with alpha-beta pruning framework
	// only root node sets best move
	template <bool Root, int Depth, int Ply>
	auto alphaBeta(int alpha, int beta) -> std::enable_if_t<Depth, int> {
		// use fully-legal moves generator
		auto move_list = MoveGenerator::generateLegalMoves<MoveGenerator::LEGAL>();
		int tt_score;

		if constexpr (Ply)
			if (HashEntry::isValid(tt_score = tt.read<ReadType::ONLY_SCORE>(alpha, beta, Depth)))
				return tt_score;

		search_results.nodes++;

		// no legal moves detected
		if (!move_list.size()) {
			// is this situation a checkmate?
			if (isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn))
				return low_bound + 1000 - Depth;
			// stalemate case
			return 0;
		}

		const auto bbs_cpy = BBs;
		const auto gstate_cpy = game_state;
		const auto hash_cpy = hash.key;
		int score, to, pc;
		HashEntry::Flag hash_flag = HashEntry::Flag::HASH_ALPHA;

		// move ordering
		Order::sort(move_list, Ply);

		for (const auto& move : move_list) {
#if _SEARCH_DEBUG
			move.print() << '\n';
#endif

			MovePerform::makeMove(move);
			score = -alphaBeta<false, Depth - 1, Ply + 1>(-beta, -alpha);
			MovePerform::unmakeMove(bbs_cpy, gstate_cpy);
			hash.key = hash_cpy;

			// register move appearance in butterfly board
			to = move.getMask<MoveItem::iMask::TARGET>() >> 6;
			pc = move.getMask<MoveItem::iMask::PIECE>() >> 12;
			Order::butterfly[pc][to] += Depth;

			if (score > alpha) {
				// fail hard beta-cutoff
				if (score >= beta) {
					if (!move.getMask<MoveItem::iMask::CAPTURE_F>()) {
						// store killer move
						Order::killer[1][Ply] = Order::killer[0][Ply];
						Order::killer[0][Ply] = move;
						// store move as a history move
						Order::history_moves[pc][to] += Depth * Depth;
					}

					tt.write(Depth, beta, HashEntry::Flag::HASH_BETA, move);
					return beta;
				}

				hash_flag = HashEntry::Flag::HASH_EXACT;
				alpha = score;

				PV::pv_line[Ply][0] = move;
				std::memcpy(&PV::pv_line[Ply][1], &PV::pv_line[Ply + 1], PV::pv_len[Ply + 1] * sizeof(move));
				PV::pv_len[Ply] = PV::pv_len[Ply + 1] + 1;
			}
		}

		// save current position in tt
		tt.write(Depth, alpha, hash_flag, PV::pv_line[Ply][0]);

		// fail low cutoff (return best option)
		return alpha;
	}

#define CALL(d) search_results.score = -alphaBeta<true, d, 0>(low_bound, high_bound); \
				break;

	// display best move according to search algorithm
	void bestMove(int depth) {
		// cleaning
		search_results.nodes = 0;
		Search::killerClear();
		InitState::initButterfly();
		PV::clear();

		for (auto& pc : Order::history_moves)
			pc.fill(0);


		for (int d = 1; d <= depth; d++) {
			switch (d) {
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
				OS << "Unvalid depth size";
				return;
			}

			// proper static evaluation value
			if (!game_state.turn ^ (depth % 2 == 0))
				search_results.score = -search_results.score;

			OS << "info score cp " << Search::search_results.score << " depth " << d
				<< " nodes " << Search::search_results.nodes
				<< " pv ";

			for (int cnt = 0; cnt < PV::pv_len[0]; cnt++)
				PV::pv_line[0][cnt].print() << ' ';
			OS << '\n';
		}

		OS << "bestmove ";
		PV::pv_line[0][0].print() << '\n';
	}

#undef CALL
}