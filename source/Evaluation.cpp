#include "Evaluation.h"
#include "MoveGeneration.h"
#include "Search.h"
#include "MoveOrder.h"
#include "Zobrist.h"
#include "Timer.h"


namespace Eval {

	static struct commonEvalData {

		// reset shared evaluation data for opening, middlegame and endgame
		void openingDataReset() {
			// king square
			k_sq[WHITE] = getLS1BIndex(BBs[nWhiteKing]);
			k_sq[BLACK] = getLS1BIndex(BBs[nBlackKing]);

			s_pawn_count[WHITE] = bitCount(BBs[nWhitePawn]);
			s_pawn_count[BLACK] = bitCount(BBs[nBlackPawn]);

			pawn_count = s_pawn_count[WHITE] + s_pawn_count[BLACK];

			// king zone, used in king safety evaluation
			k_zone[WHITE] = attack<KING>(UINT64_MAX, k_sq[WHITE]);
			k_zone[BLACK] = attack<KING>(UINT64_MAX, k_sq[BLACK]);
			
			// attackers data as used in king safety evaluation
			att_count = { 0, 0 }, att_value = { 0, 0 };
		}

		// reset shared evaluation data for endgame phase
		inline void endgameDataReset() noexcept {
			passed_count = 0;
			backward_count = 0;
			t_passed_dist = { 0, 0 };
			t_backw_dist = { 0, 0 };
			t_o_dist = { 0, 0 };

			tarrasch_passed_msk = { eU64, eU64 };
		}

		/* common evaluation data for every game phase evaluation */

		template <typename T>
		using bothSideLookUp = std::array<T, 2>;

		int passed_count, backward_count, pawn_count;
		bothSideLookUp<int>
			t_passed_dist, t_backw_dist, t_o_dist, s_pawn_count,
			k_sq, att_count, att_value;

		bothSideLookUp<U64> k_zone, tarrasch_passed_msk;
		bothSideLookUp<std::array<U64, 5>> pt_att;

	} eval_lookup;

	// bonus for distance from promotion square
	template <enumSide SIDE>
	inline int promotionDistanceBonus(U64 bb) {
		if constexpr (SIDE) return (7 - (getLS1BIndex(bb) / 8)) * 2;
		return (getMS1BIndex(bb) / 8) * 2;
	}

	// connectivity bonus - squares controlled by at least two pieces
	template <enumSide SIDE>
	int connectivity() {
		U64 conn = eval_lookup.pt_att[SIDE][PAWN];
		int eval = 0;

		for (int pc = KNIGHT; pc <= QUEEN; pc++) {
			eval += 2 * bitCount(conn & eval_lookup.pt_att[SIDE][pc] & ~BBs[nBlack - SIDE])
				+ 4 * bitCount(conn & eval_lookup.pt_att[SIDE][pc] & BBs[nBlack - SIDE]);
			conn |= eval_lookup.pt_att[SIDE][pc];
		}

		return eval;
	}

	// penalty for no pawns, especially in endgame
	template <enumSide SIDE>
	inline int noPawnsPenalty() noexcept {
		return (!eval_lookup.s_pawn_count[SIDE]) * (-30);
	}

	// king pawn tropism, considering pawns distance to own king
	template <enumSide SIDE>
	inline int kingPawnTropism() {
		static constexpr int scale = 16;
		const int other_count = eval_lookup.pawn_count 
			- eval_lookup.passed_count 
			- eval_lookup.backward_count;

		return scale * (
			eval_lookup.t_passed_dist[SIDE] * Value::PASSER_WEIGHT
			+ eval_lookup.t_backw_dist[SIDE] * Value::BACKWARD_WEIGHT
			+ eval_lookup.t_o_dist[SIDE] * Value::OTHER_WEIGHT
		) / (
			eval_lookup.passed_count * Value::PASSER_WEIGHT 
			+ eval_lookup.backward_count * Value::BACKWARD_WEIGHT 
			+ other_count * Value::OTHER_WEIGHT + 1
		);
	} 

	// simplified castle checking
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
		return ver_flip_square[square];
	}

	template <enumSide SIDE>
	inline int flipRank(int sq) {
		if constexpr (SIDE) return 7 - (sq / 8);
		return sq / 8;
	}

	// evaluation of pawn structure of given side
	template <enumSide SIDE, gState::gPhase Phase>
	int spawnScore() {
		static constexpr auto vertical_pawn_shift = std::make_tuple(nortOne, soutOne);
		U64 pawns = BBs[nWhitePawn + SIDE], passed = eU64;
		int sq, eval = 0;
		const U64 p_att = PawnAttacks::anyAttackPawn<SIDE>(BBs[nWhitePawn + SIDE], UINT64_MAX);
		eval_lookup.pt_att[SIDE][PAWN] = p_att;

		// pawn islands
		const U64 fileset = soutFill(BBs[nWhitePawn + SIDE]) & Constans::r1_rank;
		eval -= 3 * islandCount(fileset);
		
		bool dist_updated;

		while (pawns) {
			sq = popLS1B(pawns);
			eval += Value::position_score[Phase][PAWN][flipSquare<SIDE>(sq)];
			dist_updated = false;

			// if backward pawn...
			if (!(LookUp::back_file.get(SIDE, sq) & BBs[nWhitePawn + SIDE])) {
				if constexpr (Phase == gState::ENDGAME) {
					eval_lookup.t_backw_dist[SIDE] += Value::distance_score.get(sq, eval_lookup.k_sq[SIDE]);
					eval_lookup.t_backw_dist[!SIDE] += Value::distance_score.get(sq, eval_lookup.k_sq[!SIDE]);
					eval_lookup.backward_count++;
					dist_updated = true;
				}

				eval -= 7;
			}

			// if double pawn...
			if (!(LookUp::sf_file.get(SIDE, sq) & BBs[nWhitePawn + SIDE]))
				eval -= 10;
			// if passed pawn...
			else if (!((LookUp::nf_file.get(SIDE, sq) | LookUp::sf_file.get(SIDE, sq)) & BBs[nBlackPawn - SIDE])) {
				if constexpr (Phase == gState::ENDGAME) {
					passed |= bitU64(sq);
					eval_lookup.t_passed_dist[SIDE] += Value::distance_score.get(sq, eval_lookup.k_sq[SIDE]);
					eval_lookup.t_passed_dist[!SIDE] += Value::distance_score.get(sq, eval_lookup.k_sq[!SIDE]);
					eval_lookup.passed_count++;
					dist_updated = true;

					// clear passer square bonus
					if (!(LookUp::passer_square.get(SIDE, sq) & eval_lookup.k_sq[!SIDE])
						and game_state.turn == SIDE)
						eval += 25 + game_state.isPawnEndgame() * 15;
				}

				eval += Value::passed_score[flipRank<SIDE>(sq)];
			}

			// if protected...
			if (bitU64(sq) & p_att or std::get<SIDE>(vertical_pawn_shift)(bitU64(sq)) & p_att)
				eval += 6;

			if constexpr (Phase == gState::ENDGAME) {
				if (!dist_updated) {
					eval_lookup.t_o_dist[SIDE] += Value::distance_score.get(sq, eval_lookup.k_sq[SIDE]);
					eval_lookup.t_o_dist[!SIDE] += Value::distance_score.get(sq, eval_lookup.k_sq[!SIDE]);
				}
			}
		}

		// both defending another pawn bonus
		eval += 2 * bitCount(PawnAttacks::bothAttackPawn<SIDE>(BBs[nWhitePawn + SIDE], UINT64_MAX));

		// save tarrasch masks
		if constexpr (Phase == gState::ENDGAME) {
			eval_lookup.tarrasch_passed_msk[SIDE] |= rearspan<SIDE>(passed);
			eval_lookup.tarrasch_passed_msk[!SIDE] |= frontspan<SIDE>(passed);
		}

		// isolanis and half-isolanis
		eval -= 8 * bitCount(isolanis(BBs[nWhitePawn + SIDE]))
			+ 2 * bitCount(halfIsolanis(BBs[nWhitePawn + SIDE]));

		// pawn shield
		if constexpr (Phase != gState::ENDGAME) {
			const U64 pshield = std::get<SIDE>(vertical_pawn_shift)(
				BBs[nWhiteKing + SIDE] | eastOne(BBs[nWhiteKing + SIDE]) | westOne(BBs[nWhiteKing + SIDE])
				) & BBs[nWhitePawn + SIDE],
				pshield_front = std::get<SIDE>(vertical_pawn_shift)(pshield) & BBs[nWhitePawn + SIDE];
			const int pshield_count = bitCount(pshield);

			if (pshield_count == 3) 
				eval += 12;
			else if (pshield_count == 2 and
				((pshield << 1) & (pshield >> 1)) == std::get<!SIDE>(vertical_pawn_shift)(pshield_front)) 
				eval += 8;
			else {
				// penalty for open file near the king
				const U64 open_f = ~fileFill(BBs[nWhitePawn + SIDE]);
				eval -= 4 * bitCount(open_f & LookUp::ext_king_zone.get(eval_lookup.k_sq[SIDE]));
			}
		}
		else {
			// no pawns in endgame penalty
			eval += !eval_lookup.s_pawn_count[SIDE] ? -30 : promotionDistanceBonus<SIDE>(BBs[nWhitePawn + SIDE]);
		}

		// overly advanced pawns
		eval -= 2 * bitCount(overlyAdvancedPawns<SIDE>(BBs[nWhitePawn + SIDE], BBs[nBlackPawn - SIDE]));

		return eval;
	}

	// evaluation of knights
	template <enumSide SIDE, enumPiece PC, gState::gPhase Phase>
	auto spcScore(int& c_sq) -> std::enable_if_t<PC == KNIGHT, int> {
		int sq, eval = 0, att_count;
		const U64 opp_p_att = eval_lookup.pt_att[!SIDE][PAWN];
		U64 k_msk = BBs[nWhiteKnight + SIDE], opp_contr, k_att, t_att = eU64;

		while (k_msk) {
			sq = popLS1B(k_msk);
			eval += Value::position_score[Phase][KNIGHT][flipSquare<SIDE>(sq)];

			// penalty for squares controled by enemy pawns
			t_att |= (k_att = attack<KNIGHT>(UINT64_MAX, sq));
			opp_contr = k_att & opp_p_att;
			att_count = bitCount(k_att);
			eval -= 20 * bitCount(opp_contr) / att_count;
			c_sq += att_count;

			if constexpr (Phase != gState::ENDGAME) {
				if (k_att & LookUp::ext_king_zone.get(eval_lookup.k_sq[!SIDE])) {
					eval_lookup.att_count[SIDE]++;
					eval_lookup.att_value[SIDE] += Value::attacker_weight[KNIGHT]
						* bitCount(k_att & eval_lookup.k_zone[!SIDE]);
				}
			}

			// outpos check
			if (bitU64(sq) &
				(Constans::board_side[!SIDE] & eval_lookup.pt_att[SIDE][PAWN] & ~opp_p_att))
				eval += Value::outpos_score[sq];

			// king tropism bonus
			eval += Value::knight_distance_score.get(sq, eval_lookup.k_sq[!SIDE])
				+ eval_lookup.pawn_count;
		}

		eval_lookup.pt_att[SIDE][KNIGHT] = t_att;

		// mobility: undefended minor pieces 
		eval -= 2 * bitCount(~t_att & (BBs[nWhiteKnight + SIDE] | BBs[nWhiteBishop + SIDE]));
		return eval;
	}

	// evaluation of bishops
	template <enumSide SIDE, enumPiece PC, gState::gPhase Phase>
	auto spcScore(int& c_sq) -> std::enable_if_t<PC == BISHOP, int> {
		int sq, eval = 0, b_count = 0;
		U64 b_msk = BBs[nWhiteBishop + SIDE], b_att, t_att = eU64;

		while (b_msk) {
			sq = popLS1B(b_msk);
			b_count++;
			eval += Value::position_score[Phase][BISHOP][flipSquare<SIDE>(sq)];
			b_att = attack<BISHOP>(BBs[nOccupied], sq);
			t_att |= b_att;

			if constexpr (Phase != gState::ENDGAME) {
				if (b_att & eval_lookup.k_zone[!SIDE]) {
					eval_lookup.att_count[SIDE]++;
					eval_lookup.att_value[SIDE] += Value::attacker_weight[BISHOP]
						* bitCount(b_att & eval_lookup.k_zone[!SIDE]);
				}
			}

			// king tropism score
			eval += std::max(
						Value::adiag_score.get(sq, eval_lookup.k_sq[!SIDE]),
						Value::diag_score.get(sq, eval_lookup.k_sq[!SIDE])
					);

			// bishop's value increasing while number of pawns are decreasing
			eval -= eval_lookup.pawn_count;
		}

		// bishop pair bonus
		eval += (b_count >= 2) * 50;

		eval_lookup.pt_att[SIDE][BISHOP] = t_att;

		// mobility: undefended minor pieces and legal moves squares
		eval -= 2 * bitCount(~t_att & (BBs[nWhiteKnight + SIDE] | BBs[nWhiteBishop + SIDE]));
		c_sq += bitCount(t_att);
		return eval;
	}

	// evaluation of rooks
	template <enumSide SIDE, enumPiece PC, gState::gPhase Phase>
	auto spcScore(int& c_sq) -> std::enable_if_t<PC == ROOK, int> {
		int sq, eval = 0;
		U64 r_msk = BBs[nWhiteRook + SIDE], r_att;
		const U64 open_f = ~fileFill(BBs[nWhitePawn + SIDE]);
		eval_lookup.pt_att[SIDE][ROOK] = eU64;

		while (r_msk) {
			sq = popLS1B(r_msk);
			eval += Value::position_score[Phase][ROOK][flipSquare<SIDE>(sq)];
			eval_lookup.pt_att[SIDE][ROOK] |= (r_att = attack<ROOK>(BBs[nOccupied], sq));
			c_sq += bitCount(r_att);

			if constexpr (Phase != gState::ENDGAME) {
				if (r_att & eval_lookup.k_zone[!SIDE]) {
					eval_lookup.att_count[SIDE]++;
					eval_lookup.att_value[SIDE] += Value::attacker_weight[ROOK]
						* bitCount(r_att & eval_lookup.k_zone[!SIDE]);
				}
			}

			// king tropism score
			eval += Value::distance_score.get(sq, eval_lookup.k_sq[!SIDE]);

			// open file score
			if (bitU64(sq) & open_f) 
				eval += 10;
			// queen/rook on the same file
			if (Constans::f_by_index[sq % 8] & (BBs[nBlackQueen - SIDE] | BBs[nWhiteRook + SIDE])) 
				eval += 10;
			// tarrasch rule
			if constexpr (Phase == gState::ENDGAME) {
				if (bitU64(sq) & eval_lookup.tarrasch_passed_msk[SIDE]) 
					eval += 10;
			}

			// rook's value increasing as number of pawns is decreasing
			eval -= eval_lookup.pawn_count;
		}

		return eval;
	}

	// evaluation of queens
	template <enumSide SIDE, enumPiece PC, gState::gPhase Phase>
	auto spcScore(int& c_sq) -> std::enable_if_t<PC == QUEEN, int> {
		int sq, eval = 0;
		U64 q_msk = BBs[nWhiteQueen + SIDE], q_att;
		eval_lookup.pt_att[SIDE][QUEEN] = eU64;

		while (q_msk) {
			sq = popLS1B(q_msk);
			eval += Value::position_score[Phase][QUEEN][flipSquare<SIDE>(sq)];
			eval_lookup.pt_att[SIDE][QUEEN] |= (q_att = attack<QUEEN>(BBs[nOccupied], sq));
			c_sq += bitCount(q_att);

			if constexpr (Phase != gState::ENDGAME) {
				if (q_att & eval_lookup.k_zone[!SIDE]) {
					eval_lookup.att_count[SIDE]++;
					eval_lookup.att_value[SIDE] += Value::attacker_weight[QUEEN]
						* bitCount(q_att & eval_lookup.k_zone[!SIDE]);
				}
			}

			// king tropism score
			eval += Value::distance_score.get(sq, eval_lookup.k_sq[!SIDE]) 
				+ std::max(
					Value::adiag_score.get(sq, eval_lookup.k_sq[!SIDE]),
					Value::diag_score.get(sq, eval_lookup.k_sq[!SIDE])
				);

			// penalty for queen development in opening
			if constexpr (Phase == gState::OPENING)
				eval += Value::queen_ban_dev[sq];
		}

		return eval;
	}

	// king evaluation
	template <enumSide SIDE, gState::gPhase Phase>
	int kingScore(int relative_eval) {
		int eval = // king zone control
			eval_lookup.att_value[SIDE] * Value::attack_count_weight[eval_lookup.att_count[SIDE]] / 110;

		if constexpr (Phase != gState::ENDGAME) {
			eval += Value::king_score[flipSquare<SIDE>(eval_lookup.k_sq[SIDE])];

			// check castling possibility
			if constexpr (Phase == gState::OPENING)
				if (isCastle<SIDE>()) eval += 15;
		}
		else {
			// king distance consideration
			if (relative_eval > 0) 
				eval += Value::distance_score.get(eval_lookup.k_sq[SIDE], eval_lookup.k_sq[!SIDE]);
			eval += Value::late_king_score[flipSquare<SIDE>(eval_lookup.k_sq[SIDE])];
		}

		return eval;
	}

	template <enumSide SIDE, gState::gPhase Phase>
	int templEval(int alpha, int beta) {
		const int material_sc = game_state.material[SIDE] - game_state.material[!SIDE];

		if constexpr (Phase == gState::OPENING) {
			static constexpr int lazy_margin_op = 450;

			if (material_sc - lazy_margin_op >= beta)
				return beta;
			else if (material_sc + lazy_margin_op <= alpha)
				return alpha;
		}

		// controlled squared by each sides
		int c_sq1 = 0, c_sq2 = 0;

		// pawn structure evaluation
		int eval = spawnScore<SIDE, Phase>() - spawnScore<!SIDE, Phase>();

		if constexpr (Phase == gState::ENDGAME)
			eval += kingPawnTropism<SIDE>() - kingPawnTropism<!SIDE>();

		eval += spcScore<SIDE, KNIGHT, Phase>(c_sq1) - spcScore<!SIDE, KNIGHT, Phase>(c_sq2);
		eval += spcScore<SIDE, BISHOP, Phase>(c_sq1) - spcScore<!SIDE, BISHOP, Phase>(c_sq2);
		eval += spcScore<SIDE, ROOK,   Phase>(c_sq1) - spcScore<!SIDE, ROOK,   Phase>(c_sq2);
		eval += spcScore<SIDE, QUEEN,  Phase>(c_sq1) - spcScore<!SIDE, QUEEN,  Phase>(c_sq2);

		eval += 
			// consider connectivity (double connected squares)			
			connectivity<SIDE>() - connectivity<!SIDE>()
			// material score and mobility
			+ material_sc + 2 * (c_sq1 / (c_sq2 + 1));

		// king position evaluation
		eval += kingScore<SIDE, Phase>(eval) - kingScore<!SIDE, Phase>(-eval);
		return eval;
	}

	template <gState::gPhase Phase>
	inline int sideEval(int alpha, int beta) {
		return game_state.turn == WHITE ? 
			templEval<WHITE, Phase>(alpha, beta) : 
			templEval<BLACK, Phase>(alpha, beta);
	}

	// main evaluation system
	int evaluate(int alpha, int beta) {
		eval_lookup.openingDataReset();

		if (game_state.gamePhase() == gState::OPENING)
			return sideEval<gState::OPENING>(alpha, beta);

		eval_lookup.endgameDataReset();

		// middlegame and endgame point of view score interpolation
		const int phase = ((8150 - (game_state.material[0] + game_state.material[1] - Value::DOUBLE_KING_VAL)) * 256 + 4075) / 8150,
			mid_score = sideEval<gState::MIDDLEGAME>(alpha, beta),
			end_score = sideEval<gState::ENDGAME>(alpha, beta);
		
		return ((mid_score * (256 - phase)) + (end_score * phase)) / 256;
	}

} // namespace Eval