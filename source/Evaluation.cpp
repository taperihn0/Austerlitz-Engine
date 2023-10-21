#include "Evaluation.h"
#include "MoveGeneration.h"
#include "Search.h"
#include "MoveOrder.h"
#include "Zobrist.h"
#include "Timer.h"


namespace Eval {

	static struct commonEvalData {
		std::array<int, 2> k_sq, att_count, att_value;
		int pawn_count, phase, mid_score;
		std::array<U64, 2> k_zone;
	} common_data;

	template <enumSide SIDE>
	constexpr U64 pshield(U64 bb) {
		int shift;

		if constexpr (SIDE) shift = Compass::nort;
		else shift = Compass::sout;

		bb |= (genShift(bb, shift));
		bb |= (genShift(bb, shift));
		return bb;
	}

	// simple castle checking
	template <enumSide SIDE>
	bool isCastle() {
		if (game_state.castle.checkLegalCastle<SIDE & QUEEN>()
			or game_state.castle.checkLegalCastle<SIDE & ROOK>())
			return false;

		const int c_side = (BBs[nWhiteKing + SIDE] & Constans::castle_sides[SIDE][ROOK]) ? ROOK :
			(BBs[nWhiteKing + SIDE] & Constans::castle_sides[SIDE][QUEEN]) ? QUEEN : -1;

		if (c_side == -1) return false;
		return !(BBs[nWhiteRook + SIDE] & Constans::castle_sides[SIDE][c_side])
			and (BBs[nWhiteRook + SIDE] & Constans::king_center[SIDE]);
	}

	template <enumSide SIDE>
	inline int flipSquare(int square) {
		if constexpr (SIDE) return square;
		return mirrored_square[square];
	}

	template <enumSide SIDE>
	inline int flipRank(int sq) {
		if constexpr (SIDE) return 7 - (sq / 8);
		return sq / 8;
	}

	template <enumSide SIDE, gState::gPhase Phase>
	int spawnScore() {
		static constexpr auto pawn_shift = std::make_tuple(nortOne, soutOne);
		U64 pawns = BBs[nWhitePawn + SIDE];
		int sq, eval = 0;
		const U64 p_att = PawnAttacks::anyAttackPawn<SIDE>(BBs[nWhitePawn + SIDE], UINT64_MAX);

		// pawn islands
		const U64 fileset = soutFill(BBs[nWhitePawn + SIDE]) & Constans::r1_rank;
		eval -= 3 * islandCount(fileset);

		while (pawns) {
			sq = popLS1B(pawns);
			eval += Value::position_score[Phase][PAWN][flipSquare<SIDE>(sq)];

			// if backward pawn...
			if (!(LookUp::back_file.get(SIDE, sq) & BBs[nWhitePawn + SIDE]))
				eval -= 7;

			// if double pawn...
			if (!(LookUp::sf_file.get(SIDE, sq) & BBs[nWhitePawn + SIDE]))
				eval -= 10;
			// if passed pawn...
			else if (!((LookUp::nf_file.get(SIDE, sq) | LookUp::sf_file.get(SIDE, sq)) & BBs[nBlackPawn - SIDE]))
				eval += Value::passed_score[flipRank<SIDE>(sq)];

			// if protected...
			if (bitU64(sq) & p_att or std::get<SIDE>(pawn_shift)(bitU64(sq)) & p_att)
				eval += 6;

			if constexpr (Phase != gState::ENDGAME)
				eval += static_cast<int>(0.5 * Value::distance_score.get(sq, common_data.k_sq[!SIDE]));
		}

		// candidate pawns
		const U64
			att = PawnAttacks::bothAttackPawn<!SIDE>(BBs[nBlackPawn - SIDE], BBs[nWhitePawn + SIDE]
				| std::get<SIDE>(pawn_shift)(BBs[nWhitePawn + SIDE])),
			helper = PawnAttacks::bothAttackPawn<SIDE>(BBs[nWhitePawn + SIDE], BBs[nWhitePawn + SIDE]
				| std::get<SIDE>(pawn_shift)(BBs[nWhitePawn + SIDE]));
		eval += 5 * bitCount(helper & ~att);

		// isolanis and half-isolanis
		eval -= 8 * bitCount(isolanis(BBs[nWhitePawn + SIDE]))
			+ 2 * bitCount(halfIsolanis(BBs[nWhitePawn + SIDE]));

		// pawn shield
		if constexpr (Phase != gState::ENDGAME) {
			const U64 pshield = std::get<SIDE>(pawn_shift)(
				BBs[nWhiteKing + SIDE] | eastOne(BBs[nWhiteKing + SIDE]) | westOne(BBs[nWhiteKing + SIDE])
				) & BBs[nWhitePawn + SIDE],
				pshield_front = std::get<SIDE>(pawn_shift)(pshield) & BBs[nWhitePawn + SIDE];
			const int pshield_count = bitCount(pshield);

			if (pshield_count == 3) eval += 12;
			else if (pshield_count == 2 and ((pshield << 1) & (pshield >> 1)) == pshield_front) eval += 8;
			else {
				// penalty for open file near the king
				const U64 open_f = ~fileFill(BBs[nWhitePawn + SIDE]);
				eval -= 3 * bitCount(open_f & ~pshield);
			}
		}

		// overly advanced pawns
		eval -= 5 * bitCount(overlyAdvancedPawns<SIDE>(BBs[nWhitePawn + SIDE], BBs[nBlackPawn - SIDE]));
		return eval;
	}

	template <enumSide SIDE, enumPiece PC, gState::gPhase Phase>
	auto spcScore(int& c_sq) -> std::enable_if_t<PC == KNIGHT, int> {
		int sq, eval = 0;
		const U64 opp_p_att = PawnAttacks::anyAttackPawn<!SIDE>(BBs[nBlackPawn - SIDE], UINT64_MAX);
		U64 k_msk = BBs[nWhiteKnight + SIDE], opp_contr, k_att, t_att = eU64;

		while (k_msk) {
			sq = popLS1B(k_msk);
			eval += Value::position_score[Phase][KNIGHT][flipSquare<SIDE>(sq)];

			// penalty for squares controled by enemy pawns
			t_att |= (k_att = attack<KNIGHT>(BBs[nOccupied], sq));
			opp_contr = k_att & opp_p_att;
			//eval -= 2 * bitCount(opp_contr);
			eval -= 20 * bitCount(opp_contr) / bitCount(k_att);

			if (k_att & Value::ext_king_zone.get(common_data.k_sq[!SIDE])) {
				common_data.att_count[SIDE]++;
				common_data.att_value[SIDE] += Value::attacker_weight[KNIGHT]
					* bitCount(k_att & common_data.k_zone[!SIDE]);
			}

			// outpos check
			if (bitU64(sq) &
				(Constans::board_side[!SIDE] & PawnAttacks::anyAttackPawn<SIDE>(BBs[nWhitePawn + SIDE], UINT64_MAX
					& ~opp_p_att)))
				eval += Value::outpos_score[sq];

			eval += common_data.pawn_count;
		}

		// mobility: undefended minor pieces and legal moves squares
		eval -= 2 * bitCount(~t_att & (BBs[nWhiteKnight + SIDE] | BBs[nWhiteBishop + SIDE]));
		c_sq += bitCount(t_att);
		return eval;
	}

	template <enumSide SIDE, enumPiece PC, gState::gPhase Phase>
	auto spcScore(int& c_sq) -> std::enable_if_t<PC == BISHOP, int> {
		int sq, eval = 0;
		U64 b_msk = BBs[nWhiteBishop + SIDE], b_att, t_att = eU64;

		// bishop pair bonus
		eval += (bitCount(b_msk) == 2) * 50;

		while (b_msk) {
			sq = popLS1B(b_msk);
			eval += Value::position_score[Phase][BISHOP][flipSquare<SIDE>(sq)];
			b_att = attack<BISHOP>(BBs[nOccupied], sq);
			t_att |= b_att;

			if (b_att & common_data.k_zone[!SIDE]) {
				common_data.att_count[SIDE]++;
				common_data.att_value[SIDE] += Value::attacker_weight[BISHOP]
					* bitCount(b_att & common_data.k_zone[!SIDE]);
			}

			// king tropism score
			eval += std::max(
						Value::adiag_score.get(sq, common_data.k_sq[!SIDE]),
						Value::diag_score.get(sq, common_data.k_sq[!SIDE])
					);

			eval -= common_data.pawn_count;
		}

		eval -= 2 * bitCount(~t_att & (BBs[nWhiteKnight + SIDE] | BBs[nWhiteBishop + SIDE]));
		c_sq += bitCount(t_att & BBs[nEmpty]);
		return eval;
	}

	template <enumSide SIDE, enumPiece PC, gState::gPhase Phase>
	auto spcScore(int& c_sq) -> std::enable_if_t<PC == ROOK, int> {
		int sq, eval = 0;
		U64 r_msk = BBs[nWhiteRook + SIDE], r_att;
		const U64 open_f = ~fileFill(BBs[nWhitePawn + SIDE]);

		while (r_msk) {
			sq = popLS1B(r_msk);
			eval += Value::position_score[Phase][ROOK][flipSquare<SIDE>(sq)];
			r_att = attack<ROOK>(BBs[nOccupied], sq);
			c_sq += bitCount(r_att & BBs[nEmpty]);

			if (r_att & common_data.k_zone[!SIDE]) {
				common_data.att_count[SIDE]++;
				common_data.att_value[SIDE] += Value::attacker_weight[ROOK]
					* bitCount(r_att & common_data.k_zone[!SIDE]);
			}

			// king tropism score
			eval += Value::distance_score.get(sq, common_data.k_sq[!SIDE]);

			// open file score
			if (bitU64(sq) & open_f) eval += 10;
			// queen/rook on the same file
			if (Constans::f_by_index[sq % 8] & (BBs[nBlackQueen - SIDE] | BBs[nWhiteRook + SIDE])) eval += 10;
			// looking at king zone
			//if (Constans::f_by_index[sq % 8] & common_data.k_zone[!SIDE]) eval += 7;

			eval -= common_data.pawn_count;
		}

		return eval;
	}

	template <enumSide SIDE, enumPiece PC, gState::gPhase Phase>
	auto spcScore(int& c_sq) -> std::enable_if_t<PC == QUEEN, int> {
		int sq, eval = 0;
		U64 q_msk = BBs[nWhiteQueen + SIDE], q_att;

		while (q_msk) {
			sq = popLS1B(q_msk);
			eval += Value::position_score[Phase][QUEEN][flipSquare<SIDE>(sq)];
			q_att = attack<QUEEN>(BBs[nOccupied], sq);
			c_sq += bitCount(q_att & BBs[nEmpty]);

			if (q_att & common_data.k_zone[!SIDE]) {
				common_data.att_count[SIDE]++;
				common_data.att_value[SIDE] += Value::attacker_weight[QUEEN]
					* bitCount(q_att & common_data.k_zone[!SIDE]);
			}

			// king tropism score
			eval += Value::distance_score.get(sq, common_data.k_sq[!SIDE]) 
				+ std::max(
					Value::adiag_score.get(sq, common_data.k_sq[!SIDE]),
					Value::diag_score.get(sq, common_data.k_sq[!SIDE])
				);

			if constexpr (Phase == gState::OPENING)
				eval += Value::queen_ban_dev[sq];
		}

		return eval;
	}

	template <enumSide SIDE, gState::gPhase Phase>
	int kingScore() {
		int eval = 0;

		// king zone control
		eval += common_data.att_value[SIDE] * Value::attack_count_weight[common_data.att_count[SIDE]] / 100;

		if constexpr (Phase != gState::ENDGAME) {
			eval = Value::king_score[flipSquare<SIDE>(common_data.k_sq[SIDE])];

			// check castling possibility
			if constexpr (Phase == gState::OPENING) {
				if (isCastle<SIDE>())
					eval += 15;
			}
		}
		else eval = Value::late_king_score[flipSquare<SIDE>(common_data.k_sq[SIDE])];

		return eval;
	}

	template <enumSide SIDE, gState::gPhase Phase>
	int templEval(int alpha, int beta) {
		const int material_sc = game_state.material[SIDE] - game_state.material[!SIDE];

		if constexpr (Phase == gState::OPENING) {
			static constexpr int lazy_margin_op = 265; //265 255

			if (material_sc - lazy_margin_op >= beta)
				return beta;
			else if (material_sc + lazy_margin_op <= alpha)
				return alpha;
		}
		else if constexpr (Phase == gState::ENDGAME) {
			static constexpr int lazy_margin_ed = 255; //255 240

			if (((common_data.mid_score * (256 - common_data.phase))
				+ ((material_sc - lazy_margin_ed) * common_data.phase)) / 256 >= beta)
				return beta;
			else if (((common_data.mid_score * (256 - common_data.phase))
				+ ((material_sc + lazy_margin_ed) * common_data.phase)) / 256 <= alpha)
				return alpha;
		}

		int eval = 0, c_sq1 = 0, c_sq2 = 0;

		eval += spawnScore<SIDE, Phase>() - spawnScore<!SIDE, Phase>();
		eval += kingScore<SIDE, Phase>() - kingScore<!SIDE, Phase>();
		eval += spcScore<SIDE, KNIGHT, Phase>(c_sq1) - spcScore<!SIDE, KNIGHT, Phase>(c_sq2);
		eval += spcScore<SIDE, BISHOP, Phase>(c_sq1) - spcScore<!SIDE, BISHOP, Phase>(c_sq2);
		eval += spcScore<SIDE, ROOK, Phase>(c_sq1) - spcScore<!SIDE, ROOK, Phase>(c_sq2);
		eval += spcScore<SIDE, QUEEN, Phase>(c_sq1) - spcScore<!SIDE, QUEEN, Phase>(c_sq2);

		return material_sc + eval + 2 * (c_sq1 / (c_sq2 + 1));
	}

	template <gState::gPhase Phase>
	inline int sideEval(enumSide side, int alpha, int beta) {
		return game_state.turn == WHITE ? templEval<WHITE, Phase>(alpha, beta) : templEval<BLACK, Phase>(alpha, beta);
	}

	// main evaluation system
	int evaluate(int alpha, int beta) {
		common_data.k_sq[game_state.turn] = getLS1BIndex(BBs[nWhiteKing + game_state.turn]);
		common_data.k_sq[!game_state.turn] = getLS1BIndex(BBs[nBlackKing - game_state.turn]);
		common_data.pawn_count = bitCount(BBs[nWhitePawn] | BBs[nBlackPawn]);
		common_data.att_count = { 0, 0 }, common_data.att_value = { 0, 0 };
		common_data.k_zone[WHITE] = attack<KING>(UINT64_MAX, common_data.k_sq[WHITE]);
		common_data.k_zone[BLACK] = attack<KING>(UINT64_MAX, common_data.k_sq[BLACK]);

		if (game_state.gamePhase() == gState::OPENING)
			return sideEval<gState::OPENING>(game_state.turn, alpha, beta);

		// score interpolation
		static constexpr int double_k_val = 2 * Value::KING_VALUE;
		common_data.phase = ((8150 - (game_state.material[0] + game_state.material[1] - double_k_val)) * 256 + 4075) / 8150;

		common_data.mid_score = sideEval<gState::MIDDLEGAME>(game_state.turn, alpha, beta);
		const int end_score = sideEval<gState::ENDGAME>(game_state.turn, alpha, beta);

		if (end_score >= beta) return beta;
		else if (end_score <= alpha) return alpha;
		return ((common_data.mid_score * (256 - common_data.phase)) + (end_score * common_data.phase)) / 256;
	}

	int qSearch(int alpha, int beta, int ply) {
		if (time_data.is_time and !(Search::search_results.nodes & Search::time_check_modulo) and !time_data.checkTimeLeft()) {
			time_data.stop = true;
			return Search::time_stop_sign;
		}

		const int eval = evaluate(Search::low_bound, beta);
		Search::search_results.nodes++;

		if (eval >= beta) return beta;
		else if (game_state.gamePhase() != game_state.ENDGAME
			and !isSquareAttacked(getLS1BIndex(BBs[nWhiteKing + game_state.turn]), game_state.turn)
			and eval + Eval::Value::QUEEN_VALUE <= alpha)
			return alpha;
		alpha = std::max(alpha, eval);

		// generate opponent capture moves
		auto capt_list = MoveGenerator::generateLegalMoves<MoveGenerator::CAPTURES>();
		const auto bbs_cpy = BBs;
		const auto gstate_cpy = game_state;
		const auto hash_cpy = hash.key;
		int score;

		for (int i = 0; i < capt_list.size(); i++) {

			// capture ordering
			Order::pickBest(capt_list, i, ply);
			const auto& move = capt_list[i];

			MovePerform::makeMove(move);

			score = -qSearch(-beta, -alpha, ply + 1);

			MovePerform::unmakeMove(bbs_cpy, gstate_cpy);
			hash.key = hash_cpy;

			if (time_data.stop)
				return Search::time_stop_sign;
			else if (score > alpha) {
				if (score >= beta)
					return beta;
				alpha = score;
			}
		}

		return alpha;
	}

} // namespace Eval