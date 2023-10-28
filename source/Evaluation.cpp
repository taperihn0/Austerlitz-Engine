#include "Evaluation.h"
#include "MoveGeneration.h"
#include "Search.h"
#include "MoveOrder.h"
#include "Zobrist.h"
#include "Timer.h"


namespace Eval {

	static struct commonEvalData {

		void openingDataReset() {
			k_sq[WHITE] = getLS1BIndex(BBs[nWhiteKing]);
			k_sq[BLACK] = getLS1BIndex(BBs[nBlackKing]);

			s_pawn_count[WHITE] = bitCount(BBs[nWhitePawn]);
			s_pawn_count[BLACK] = bitCount(BBs[nBlackPawn]);

			pawn_count = s_pawn_count[WHITE] + s_pawn_count[BLACK];

			k_zone[WHITE] = attack<KING>(MAX_U64, k_sq[WHITE]);
			k_zone[BLACK] = attack<KING>(MAX_U64, k_sq[BLACK]);

			att_count = { 0, 0 }, att_value = { 0, 0 };
		}

		inline void endgameDataReset() noexcept {
			passed_count = 0;
			backward_count = 0;
			t_passed_dist = { 0, 0 };
			t_backw_dist = { 0, 0 };
			t_o_dist = { 0, 0 };

			tarrasch_passed_msk = { eU64, eU64 };
		}

		template <typename T>
		using bothSideLookUp = std::array<T, 2>;

		int passed_count, backward_count, pawn_count;
		bothSideLookUp<int>
			t_passed_dist, t_backw_dist, t_o_dist, s_pawn_count,
			k_sq, att_count, att_value;

		int phase, mid_score;
		bothSideLookUp<U64> k_zone, tarrasch_passed_msk;
		bothSideLookUp<std::array<U64, 5>> pt_att;

	} eval_lookup;

	template <enumSide SIDE>
	int connectivity() {
		U64 conn = eval_lookup.pt_att[SIDE][PAWN];
		int eval = 0;

		for (int pc = KNIGHT; pc <= QUEEN; pc++) {
			eval += 3 * bitCount(conn & eval_lookup.pt_att[SIDE][pc]);
			conn |= eval_lookup.pt_att[SIDE][pc];
		}

		return eval;
	}

	template <enumSide SIDE>
	inline int noPawnsPenalty() noexcept {
		return (!eval_lookup.s_pawn_count[SIDE]) * (-30);
	}

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
			)
		/ (
			eval_lookup.passed_count * Value::PASSER_WEIGHT 
			+ eval_lookup.backward_count * Value::BACKWARD_WEIGHT 
			+ other_count * Value::OTHER_WEIGHT + 1
			);
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

		// both attacking pawns bonus
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
				eval -= 3 * bitCount(open_f & ~pshield);
			}
		}

		// overly advanced pawns
		eval -= 2 * bitCount(overlyAdvancedPawns<SIDE>(BBs[nWhitePawn + SIDE], BBs[nBlackPawn - SIDE]));

		// no pawns in endgame penalty
		if constexpr (Phase == gState::ENDGAME)
			eval += noPawnsPenalty<SIDE>();

		return eval;
	}

	template <enumSide SIDE, enumPiece PC, gState::gPhase Phase>
	auto spcScore(int& c_sq) -> std::enable_if_t<PC == KNIGHT, int> {
		int sq, eval = 0;
		const U64 opp_p_att = eval_lookup.pt_att[!SIDE][PAWN];
		U64 k_msk = BBs[nWhiteKnight + SIDE], opp_contr, k_att, t_att = eU64;

		while (k_msk) {
			sq = popLS1B(k_msk);
			eval += Value::position_score[Phase][KNIGHT][flipSquare<SIDE>(sq)];

			// penalty for squares controled by enemy pawns
			t_att |= (k_att = attack<KNIGHT>(UINT64_MAX, sq));
			opp_contr = k_att & opp_p_att;
			eval -= 20 * bitCount(opp_contr) / bitCount(k_att);

			if constexpr (Phase != gState::ENDGAME) {
				if (k_att & Value::ext_king_zone.get(eval_lookup.k_sq[!SIDE])) {
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

			eval -= eval_lookup.pawn_count;
		}

		eval_lookup.pt_att[SIDE][BISHOP] = t_att;

		eval -= 2 * bitCount(~t_att & (BBs[nWhiteKnight + SIDE] | BBs[nWhiteBishop + SIDE]));
		c_sq += bitCount(t_att);
		return eval;
	}

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
			if (bitU64(sq) & open_f) eval += 10;
			// queen/rook on the same file
			if (Constans::f_by_index[sq % 8] & (BBs[nBlackQueen - SIDE] | BBs[nWhiteRook + SIDE])) eval += 10;
			// tarrasch rule
			if constexpr (Phase == gState::ENDGAME) {
				if (bitU64(sq) & eval_lookup.tarrasch_passed_msk[SIDE]) eval += 10;
			}

			eval -= eval_lookup.pawn_count;
		}

		return eval;
	}

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

			if constexpr (Phase == gState::OPENING)
				eval += Value::queen_ban_dev[sq];
		}

		return eval;
	}

	template <enumSide SIDE, gState::gPhase Phase>
	int kingScore() {
		int eval = // king zone control
			eval_lookup.att_value[SIDE] * Value::attack_count_weight[eval_lookup.att_count[SIDE]] / 450;

		if constexpr (Phase != gState::ENDGAME) {
			eval += Value::king_score[flipSquare<SIDE>(eval_lookup.k_sq[SIDE])];

			// check castling possibility
			if constexpr (Phase == gState::OPENING) {
				if (isCastle<SIDE>()) eval += 15;
			}
		}
		else eval += Value::late_king_score[flipSquare<SIDE>(eval_lookup.k_sq[SIDE])];

		return eval;
	}

	template <enumSide SIDE, gState::gPhase Phase>
	int templEval(int alpha, int beta) {
		const int material_sc = game_state.material[SIDE] - game_state.material[!SIDE];

		if constexpr (Phase == gState::OPENING) {
			static constexpr int lazy_margin_op = 274;

			if (material_sc - lazy_margin_op >= beta)
				return beta;
			else if (material_sc + lazy_margin_op <= alpha)
				return alpha;
		}
		else if constexpr (Phase == gState::ENDGAME) {
			static constexpr int lazy_margin_ed = 292;

			if (((eval_lookup.mid_score * (256 - eval_lookup.phase))
				+ ((material_sc - lazy_margin_ed) * eval_lookup.phase)) / 256 >= beta)
				return beta;
			else if (((eval_lookup.mid_score * (256 - eval_lookup.phase))
				+ ((material_sc + lazy_margin_ed) * eval_lookup.phase)) / 256 <= alpha)
				return alpha;
		}

		int eval = 0, c_sq1 = 0, c_sq2 = 0;

		eval += spawnScore<SIDE, Phase>() - spawnScore<!SIDE, Phase>();

		if constexpr (Phase == gState::ENDGAME)
			eval += kingPawnTropism<SIDE>() - kingPawnTropism<!SIDE>();

		eval += spcScore<SIDE, KNIGHT, Phase>(c_sq1) - spcScore<!SIDE, KNIGHT, Phase>(c_sq2);
		eval += spcScore<SIDE, BISHOP, Phase>(c_sq1) - spcScore<!SIDE, BISHOP, Phase>(c_sq2);
		eval += spcScore<SIDE, ROOK, Phase>(c_sq1) - spcScore<!SIDE, ROOK, Phase>(c_sq2);
		eval += spcScore<SIDE, QUEEN, Phase>(c_sq1) - spcScore<!SIDE, QUEEN, Phase>(c_sq2);

		eval += kingScore<SIDE, Phase>() - kingScore<!SIDE, Phase>();

		// consider connectivity (double connected squares)
		eval += connectivity<SIDE>() - connectivity<!SIDE>();

		return material_sc + eval + 2 * (c_sq1 / (c_sq2 + 1));
	}

	template <gState::gPhase Phase>
	inline int sideEval(enumSide side, int alpha, int beta) {
		return game_state.turn == WHITE ? 
			templEval<WHITE, Phase>(alpha, beta) : 
			templEval<BLACK, Phase>(alpha, beta);
	}

	// main evaluation system
	int evaluate(int alpha, int beta) {
		eval_lookup.openingDataReset();

		if (game_state.gamePhase() == gState::OPENING)
			return sideEval<gState::OPENING>(game_state.turn, alpha, beta);

		// score interpolation
		eval_lookup.phase = ((8150 - (game_state.material[0] + game_state.material[1] - Value::DOUBLE_KING_VAL)) * 256 + 4075) / 8150;

		eval_lookup.mid_score = sideEval<gState::MIDDLEGAME>(game_state.turn, alpha, beta);
		
		eval_lookup.endgameDataReset();
		const int end_score = sideEval<gState::ENDGAME>(game_state.turn, alpha, beta);

		if (end_score >= beta) return beta;
		else if (end_score <= alpha) return alpha;
		return ((eval_lookup.mid_score * (256 - eval_lookup.phase)) + (end_score * eval_lookup.phase)) / 256;
	}

} // namespace Eval