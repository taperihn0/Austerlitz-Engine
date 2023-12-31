#include "Evaluation.h"
#include "MoveGeneration.h"
#include "Search.h"
#include "MoveOrder.h"
#include "Zobrist.h"
#include "Timer.h"


namespace Eval {

	static struct commonEvalData {

		inline U64 kingNearby(bool side) {
			U64 k_att = k_zone[side];
			k_att |= westOne(k_att);
			k_att |= eastOne(k_att);
			k_att |= nortOne(k_att);
			k_att |= soutOne(k_att);
			return k_att & ~k_zone[side];
		}

		// reset shared evaluation data for opening, middlegame and endgame
		void openingDataReset() {
			// king square
			k_sq[WHITE] = getLS1BIndex(BBs[nWhiteKing]);
			k_sq[BLACK] = getLS1BIndex(BBs[nBlackKing]);

			s_pawn_count[WHITE] = bitCount(BBs[nWhitePawn]);
			s_pawn_count[BLACK] = bitCount(BBs[nBlackPawn]);

			pawn_count = s_pawn_count[WHITE] + s_pawn_count[BLACK];

			// king zone, used in king safety evaluation
			k_zone[WHITE] = attack<KING>(UINT64_MAX, k_sq[WHITE]) | bitU64(k_sq[WHITE]);
			k_zone[BLACK] = attack<KING>(UINT64_MAX, k_sq[BLACK]) | bitU64(k_sq[BLACK]);

			k_nearby[WHITE] = kingNearby(WHITE);
			k_nearby[BLACK] = kingNearby(BLACK);

			// attackers data as used in king safety evaluation
			att_count = { 0, 0 }, att_value = { 0, 0 };

			open_files[WHITE] = ~fileFill(BBs[nWhitePawn]);
			open_files[BLACK] = ~fileFill(BBs[nBlackPawn]);
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

		bothSideLookUp<U64> k_zone, k_nearby, tarrasch_passed_msk, open_files;
		bothSideLookUp<std::array<U64, 5>> pt_att;

	} eval_vector;

	// bonus for distance from promotion square
	template <enumSide SIDE>
	inline int promotionDistanceBonus(U64 bb) {
		if constexpr (SIDE) return (7 - (getLS1BIndex(bb) / 8)) * 4;
		return (getMS1BIndex(bb) / 8) * 4;
	}

	// connectivity bonus - squares controlled by at least two pieces
	template <enumSide SIDE, gState::gPhase Phase>
	int connectivity() {
		U64 conn = eval_vector.pt_att[SIDE][PAWN];
		int eval = 0;

		static constexpr int pawn_phase_scale = []() constexpr {
			if constexpr (Phase == gState::ENDGAME)
				return Value::ENDGAME_PAWN_ATTACK;
			return Value::PAWN_ATTACK;
		}();
		
		for (int pc = KNIGHT; pc <= QUEEN; pc++) {
			eval += pawn_phase_scale * bitCount(conn & eval_vector.pt_att[SIDE][pc] & BBs[nBlackPawn - SIDE])
				+ Value::MINOR_ATTACK * bitCount(conn & eval_vector.pt_att[SIDE][pc] & (BBs[nBlackKnight - SIDE] | BBs[nBlackBishop - SIDE]))
				+ Value::ROOK_ATTACK * bitCount(conn & eval_vector.pt_att[SIDE][pc] & BBs[nBlackRook - SIDE])
				+ Value::QUEEN_ATTACK * bitCount(conn & eval_vector.pt_att[SIDE][pc] & BBs[nBlackQueen - SIDE]);

			conn |= eval_vector.pt_att[SIDE][pc];
		}

		return eval;
	}

	// penalty for no pawns, especially in endgame
	template <enumSide SIDE>
	inline int noPawnsPenalty() noexcept {
		return (eval_vector.s_pawn_count[SIDE] == 0) * (-30);
	}

	// king pawn tropism, considering pawns distance to own king
	template <enumSide SIDE>
	inline int kingPawnTropism() {
		static constexpr int scale = 16;
		const int other_count = eval_vector.pawn_count
			- eval_vector.passed_count
			- eval_vector.backward_count;

		return scale * (
				eval_vector.t_passed_dist[SIDE] * Value::PASSER_WEIGHT
				+ eval_vector.t_backw_dist[SIDE] * Value::BACKWARD_WEIGHT
				+ eval_vector.t_o_dist[SIDE] * Value::OTHER_WEIGHT
			) / (
				eval_vector.passed_count * Value::PASSER_WEIGHT
				+ eval_vector.backward_count * Value::BACKWARD_WEIGHT
				+ other_count * Value::OTHER_WEIGHT + 1
			);
	}

	// simplified castle checking
	template <enumSide SIDE>
	bool isCastle() {
		if (game_state.castle.checkLegalCastle<SIDE & QUEEN>()
			or game_state.castle.checkLegalCastle<SIDE & ROOK>())
			return false;
		return !(BBs[nWhiteKing + SIDE] & Constans::king_center[SIDE]);
	}

	template <enumSide SIDE>
	inline int flipSquare(int sq) {
		if constexpr (SIDE) return sq;
		return ver_flip_square.get(sq);
	}

	template <enumSide SIDE>
	inline int flipRank(int sq) {
		if constexpr (SIDE) return 7 - (sq / 8);
		return sq / 8;
	}

	// evaluation of pawn structure of given side
	template <enumSide SIDE, gState::gPhase Phase>
	int pawnStructureEval() {
		static constexpr auto vertical_pawn_shift = std::make_tuple(nortOne, soutOne);
		U64 pawns = BBs[nWhitePawn + SIDE], passed = eU64;
		int sq, eval = 0;

		const U64 p_att = PawnAttacks::anyAttackPawn<SIDE>(BBs[nWhitePawn + SIDE], UINT64_MAX);
		eval_vector.pt_att[SIDE][PAWN] = p_att;

		const bool is_pawn_endgame = game_state.isPawnEndgame();

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
					eval_vector.t_backw_dist[SIDE] += Value::distance_score.get(sq, eval_vector.k_sq[SIDE]);
					eval_vector.t_backw_dist[!SIDE] += Value::distance_score.get(sq, eval_vector.k_sq[!SIDE]);
					eval_vector.backward_count++;
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
					eval_vector.t_passed_dist[SIDE] += Value::distance_score.get(sq, eval_vector.k_sq[SIDE]);
					eval_vector.t_passed_dist[!SIDE] += Value::distance_score.get(sq, eval_vector.k_sq[!SIDE]);
					eval_vector.passed_count++;
					dist_updated = true;
				}

				// clear passer square bonus
				if (!(LookUp::passer_square.get(SIDE, sq) & eval_vector.k_sq[!SIDE])
					and game_state.turn == SIDE)
					eval += Value::passed_score[flipRank<SIDE>(sq)] / 2 + is_pawn_endgame * 60;

				eval += Value::passed_score[flipRank<SIDE>(sq)];
			}

			// if protected...
			if (bitU64(sq) & p_att or std::get<SIDE>(vertical_pawn_shift)(bitU64(sq)) & p_att)
				eval += 6;

			if constexpr (Phase == gState::ENDGAME) {
				if (!dist_updated) {
					eval_vector.t_o_dist[SIDE] += Value::distance_score.get(sq, eval_vector.k_sq[SIDE]);
					eval_vector.t_o_dist[!SIDE] += Value::distance_score.get(sq, eval_vector.k_sq[!SIDE]);
				}
			}
		}

		// both defending another pawn bonus
		eval += 2 * bitCount(PawnAttacks::bothAttackPawn<SIDE>(BBs[nWhitePawn + SIDE], UINT64_MAX));

		// save tarrasch masks
		if constexpr (Phase == gState::ENDGAME) {
			eval_vector.tarrasch_passed_msk[SIDE] |= rearspan<SIDE>(passed);
			eval_vector.tarrasch_passed_msk[!SIDE] |= frontspan<SIDE>(passed);
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
				eval -= 4 * bitCount(eval_vector.open_files[SIDE] & (eval_vector.k_zone[SIDE] | eval_vector.k_nearby[SIDE]));
			}
		}
		else {
			// no pawns in endgame penalty
			eval += !eval_vector.s_pawn_count[SIDE] ? -30 :
				promotionDistanceBonus<SIDE>(BBs[nWhitePawn + SIDE]) * (is_pawn_endgame + 1);
		}

		// overly advanced pawns
		eval -= 2 * bitCount(overlyAdvancedPawns<SIDE>(BBs[nWhitePawn + SIDE], BBs[nBlackPawn - SIDE]));

		return eval;
	}

	// evaluation of knights
	template <enumSide SIDE, enumPiece PC, gState::gPhase Phase>
	auto pcEval() -> std::enable_if_t<PC == KNIGHT, int> {
		int sq, eval = 0, mobility;
		const U64 opp_p_att = eval_vector.pt_att[!SIDE][PAWN];
		U64 k_msk = BBs[nWhiteKnight + SIDE], k_att, safe_att;

		eval_vector.pt_att[SIDE][KNIGHT] = eU64;

		while (k_msk) {
			sq = popLS1B(k_msk);
			eval += Value::position_score[Phase][KNIGHT][flipSquare<SIDE>(sq)];

			// safe mobility - do not consider squares controled by enemy pawns
			k_att = attack<KNIGHT>(UINT64_MAX, sq);
			safe_att = k_att & ~eval_vector.pt_att[!SIDE][PAWN] & BBs[nEmpty];
			mobility = bitCount(safe_att);
			eval += 4 * (mobility - 4);
			// undefended minor pieces 
			eval -= 2 * bitCount(~k_att & (BBs[nWhiteKnight + SIDE] | BBs[nWhiteBishop + SIDE]));

			eval_vector.pt_att[SIDE][KNIGHT] |= k_att;

			if constexpr (Phase != gState::ENDGAME) {
				if (k_att & eval_vector.k_zone[!SIDE]) {
					eval_vector.att_count[SIDE]++;
					eval_vector.att_value[SIDE] += Value::attacker_weight[KNIGHT]
						* bitCount(k_att & eval_vector.k_zone[!SIDE]);
				}
				else if (k_att & eval_vector.k_nearby[!SIDE])
					eval_vector.att_value[SIDE] += Value::attacker_weight[KNIGHT] / 5;
			}

			// outpos check
			if (bitU64(sq) &
				(Constans::board_side[!SIDE] & eval_vector.pt_att[SIDE][PAWN] & ~opp_p_att))
				eval += Value::outpos_score[sq];

			// king tropism bonus
			eval += Value::knight_distance_score.get(sq, eval_vector.k_sq[!SIDE])
				+ eval_vector.pawn_count;
		}

		return eval;
	}

	// evaluation of bishops
	template <enumSide SIDE, enumPiece PC, gState::gPhase Phase>
	auto pcEval() -> std::enable_if_t<PC == BISHOP, int> {
		int sq, eval = 0, b_count = 0, mobility;
		U64 b_msk = BBs[nWhiteBishop + SIDE], b_att;

		eval_vector.pt_att[SIDE][BISHOP] = eU64;

		while (b_msk) {
			sq = popLS1B(b_msk);
			b_count++;
			eval += Value::position_score[Phase][BISHOP][flipSquare<SIDE>(sq)];
			
			// mobility
			b_att = attack<BISHOP>(BBs[nOccupied], sq);
			mobility = bitCount(b_att);
			eval += 3 * (mobility - 7);
			// undefended minor pieces
			eval -= 2 * bitCount(~b_att & (BBs[nWhiteKnight + SIDE] | BBs[nWhiteBishop + SIDE]));

			eval_vector.pt_att[SIDE][BISHOP] |= b_att;

			if constexpr (Phase != gState::ENDGAME) {
				if (b_att & eval_vector.k_zone[!SIDE]) {
					eval_vector.att_count[SIDE]++;
					eval_vector.att_value[SIDE] += Value::attacker_weight[BISHOP]
						* bitCount(b_att & eval_vector.k_zone[!SIDE]);
				}
				else if (b_att & eval_vector.k_nearby[!SIDE])
					eval_vector.att_value[SIDE] += Value::attacker_weight[BISHOP] / 5;
			}

			// king tropism score
			eval += std::max(
				Value::adiag_score.get(sq, eval_vector.k_sq[!SIDE]),
				Value::diag_score.get(sq, eval_vector.k_sq[!SIDE])
			);

			// bishop's value increasing while number of pawns are decreasing
			eval -= eval_vector.pawn_count;
		}

		// bishop pair bonus
		eval += (b_count >= 2) * 50;
		return eval;
	}

	// evaluation of rooks
	template <enumSide SIDE, enumPiece PC, gState::gPhase Phase>
	auto pcEval() -> std::enable_if_t<PC == ROOK, int> {
		int sq, eval = 0, mobility;
		U64 r_msk = BBs[nWhiteRook + SIDE], r_att;
		
		static constexpr int mobility_weight = Phase + 1 + (Phase + 1 / 3);

		eval_vector.pt_att[SIDE][ROOK] = eU64;

		while (r_msk) {
			sq = popLS1B(r_msk);
			eval += Value::position_score[Phase][ROOK][flipSquare<SIDE>(sq)];
			
			// mobility
			r_att = attack<ROOK>(BBs[nOccupied], sq);
			mobility = bitCount(r_att);
			eval += mobility_weight * (mobility - 7);

			eval_vector.pt_att[SIDE][ROOK] |= r_att;

			if constexpr (Phase != gState::ENDGAME) {
				if (r_att & eval_vector.k_zone[!SIDE]) {
					eval_vector.att_count[SIDE]++;
					eval_vector.att_value[SIDE] += Value::attacker_weight[ROOK]
						* bitCount(r_att & eval_vector.k_zone[!SIDE]);
				}
				else if (r_att & eval_vector.k_nearby[!SIDE])
					eval_vector.att_value[SIDE] += Value::attacker_weight[ROOK] / 5;
			}

			// king tropism score
			eval += Value::distance_score.get(sq, eval_vector.k_sq[!SIDE]);

			// open file score
			if (bitU64(sq) & eval_vector.open_files[SIDE])
				eval += 10;
			// queen/rook on the same file
			if (Constans::f_by_index[sq % 8] & (BBs[nBlackQueen - SIDE] | BBs[nWhiteRook + SIDE]))
				eval += 10;
			// tarrasch rule
			if constexpr (Phase == gState::ENDGAME) {
				if (bitU64(sq) & eval_vector.tarrasch_passed_msk[SIDE])
					eval += 10;
			}

			// rook's value increasing as number of pawns is decreasing
			eval -= eval_vector.pawn_count;
		}

		return eval;
	}

	// evaluation of queens
	template <enumSide SIDE, enumPiece PC, gState::gPhase Phase>
	auto pcEval() -> std::enable_if_t<PC == QUEEN, int> {
		int sq, eval = 0, mobility;
		U64 q_msk = BBs[nWhiteQueen + SIDE], q_att;
		
		static constexpr int mobility_weight = (Phase + 2) / 2;

		eval_vector.pt_att[SIDE][QUEEN] = eU64;

		while (q_msk) {
			sq = popLS1B(q_msk);
			eval += Value::position_score[Phase][QUEEN][flipSquare<SIDE>(sq)];
	
			// mobility
			q_att = attack<QUEEN>(BBs[nOccupied], sq);
			mobility = bitCount(q_att);
			eval += mobility_weight * (mobility - 14);

			eval_vector.pt_att[SIDE][QUEEN] |= q_att;

			if constexpr (Phase != gState::ENDGAME) {
				if (q_att & eval_vector.k_zone[!SIDE]) {
					eval_vector.att_count[SIDE]++;
					eval_vector.att_value[SIDE] += Value::attacker_weight[QUEEN]
						* bitCount(q_att & eval_vector.k_zone[!SIDE]);
				}
				else if (q_att & eval_vector.k_nearby[!SIDE])
					eval_vector.att_value[SIDE] += Value::attacker_weight[QUEEN] / 5;
			}

			// king tropism score
			eval += Value::distance_score.get(sq, eval_vector.k_sq[!SIDE])
				+ std::max(
					Value::adiag_score.get(sq, eval_vector.k_sq[!SIDE]),
					Value::diag_score.get(sq, eval_vector.k_sq[!SIDE])
				);

			// penalty for queen development in opening
			if constexpr (Phase == gState::OPENING)
				eval += Value::queen_ban_dev[flipSquare<SIDE>(sq)];
		}

		return eval;
	}

	// king evaluation
	template <enumSide SIDE, gState::gPhase Phase>
	int kingEval(int relative_eval) {
		// king zone control
		const int k_zone_control = eval_vector.att_value[SIDE] * Value::attack_count_weight[eval_vector.att_count[SIDE]] / 120;
		int eval = k_zone_control;

		if constexpr (Phase != gState::ENDGAME) {
			eval += Value::king_score[flipSquare<SIDE>(eval_vector.k_sq[SIDE])];

			// check castling possibility
			if constexpr (Phase == gState::OPENING)
				if (isCastle<SIDE>()) eval += 15;
		}
		else {
			// king distance consideration
			if (relative_eval > 70)
				eval += 2 * Value::distance_score.get(eval_vector.k_sq[SIDE], eval_vector.k_sq[!SIDE]);
			eval += Value::late_king_score[flipSquare<SIDE>(eval_vector.k_sq[SIDE])];
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

		// pawn structure evaluation
		int eval = pawnStructureEval<SIDE, Phase>() - pawnStructureEval<!SIDE, Phase>();

		if constexpr (Phase == gState::ENDGAME)
			eval += kingPawnTropism<SIDE>() - kingPawnTropism<!SIDE>();

		eval += pcEval<SIDE, KNIGHT, Phase>() - pcEval<!SIDE, KNIGHT, Phase>();
		eval += pcEval<SIDE, BISHOP, Phase>() - pcEval<!SIDE, BISHOP, Phase>();
		eval += pcEval<SIDE, ROOK, Phase>() - pcEval<!SIDE, ROOK, Phase>();
		eval += pcEval<SIDE, QUEEN, Phase>() - pcEval<!SIDE, QUEEN, Phase>();

		eval +=
			// consider connectivity (double connected squares)			
			connectivity<SIDE, Phase>() - connectivity<!SIDE, Phase>()
			// material score and mobility
			+ material_sc;

		// king position evaluation
		eval += kingEval<SIDE, Phase>(eval) - kingEval<!SIDE, Phase>(-eval);
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
		eval_vector.openingDataReset();
		
		if (game_state.gamePhase() == gState::OPENING)
			return sideEval<gState::OPENING>(alpha, beta);

		eval_vector.endgameDataReset();

		// middlegame and endgame point of view score interpolation
		const int phase = ((8150 - (game_state.material[0] + game_state.material[1] - Value::DOUBLE_KING_VAL)) * 256 + 4075) / 8150,
			mid_score = sideEval<gState::MIDDLEGAME>(alpha, beta),
			end_score = sideEval<gState::ENDGAME>(alpha, beta);

		return ((mid_score * (256 - phase)) + (end_score * phase)) / 256;
	}

} // namespace Eval