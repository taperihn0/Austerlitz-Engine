#include "Evaluation.h"
#include "MoveGeneration.h"
#include "Search.h"
#include "MoveOrder.h"
#include "Zobrist.h"
#include "Timer.h"
#include "../tuning/TexelTuning.h"


namespace Eval {

	static struct commonEvalData {

		inline U64 grid5x5(U64 square) {
			square |= westOne(square);
			square |= eastOne(square);
			square |= nortOne(square);
			square |= soutOne(square);
			return square;
		}

		// reset shared evaluation data for opening, middlegame and endgame
		void reset() {
			// king square
			k_sq[WHITE] = getLS1BIndex(BBs[nWhiteKing]);
			k_sq[BLACK] = getLS1BIndex(BBs[nBlackKing]);

			s_pawn_count[WHITE] = bitCount(BBs[nWhitePawn]);
			s_pawn_count[BLACK] = bitCount(BBs[nBlackPawn]);

			pawn_count = s_pawn_count[WHITE] + s_pawn_count[BLACK];

			// king zone, used in king safety evaluation
			king_zone[WHITE] = grid5x5(attack<KING>(UINT64_MAX, k_sq[WHITE]));
			king_zone[BLACK] = grid5x5(attack<KING>(UINT64_MAX, k_sq[BLACK]));

			open_files[WHITE] = ~fileFill(BBs[nWhitePawn]);
			open_files[BLACK] = ~fileFill(BBs[nBlackPawn]);

			att_count = { 0, 0 }, att_value = { 0, 0 };
		}

		// reset shared evaluation data for endgame phase
		inline void clear() noexcept {
			att_count = { 0, 0 }, att_value = { 0, 0 };
			tarrasch_passed_msk = { eU64, eU64 };
		}

		/* common evaluation data for every game phase evaluation */

		template <typename T>
		using bothSideLookUp = std::array<T, 2>;

		int pawn_count;
		bothSideLookUp<int>
			s_pawn_count,
			k_sq, att_count, 
			att_value;

		bothSideLookUp<U64> king_zone, tarrasch_passed_msk, open_files;
		bothSideLookUp<std::array<U64, 5>> pt_att;

	} eval_vector;

	// simplified castle checking
	template <enumSide SIDE>
	inline bool isCastle() {
		if (game_state.castle.checkLegalCastle<SIDE & QUEEN>()
			or game_state.castle.checkLegalCastle<SIDE & ROOK>())
			return false;
		return !(BBs[nWhiteKing + SIDE] & Constans::king_center[SIDE]);
	}

	template <enumSide SIDE>
	inline int flipSquare(int sq) {
		if constexpr (SIDE) return hor_flip_square.get(sq);
		return ver_flip_square.get(sq);
	}

	template <enumSide SIDE>
	inline int flipRank(int sq) {
		if constexpr (SIDE) return 7 - (sq / 8);
		return sq / 8;
	}

	template <enumSide SIDE>
	U64 backwardPawnsMask() {
		U64 stop;

		if constexpr (SIDE) stop = soutOne(BBs[nBlackPawn]);
		else stop = nortOne(BBs[nWhitePawn]);

		const U64 defend_spans = attackspan<SIDE>(BBs[nWhitePawn + SIDE]),
			attack_spans = attackspan<!SIDE>(BBs[nBlackPawn - SIDE]);

		const U64 res = stop & attack_spans & ~defend_spans;

		if constexpr (SIDE) return nortOne(res);
		return soutOne(res);
	}

	template <enumSide SIDE>
	inline U64 doublePawnsMask() {
		const U64 rear_span = rearspan<SIDE>(BBs[nWhitePawn + SIDE]);
		return BBs[nWhitePawn + SIDE] & rear_span;
	}

	template <enumSide SIDE>
	inline U64 protectedPawnsMask() {
		return BBs[nWhitePawn + SIDE] & eval_vector.pt_att[SIDE][PAWN];
	}

	template <enumSide SIDE>
	inline U64 doubleProtectedPawnsMask() {
		return PawnAttacks::bothAttackPawn<SIDE>(BBs[nWhitePawn + SIDE], BBs[nWhitePawn + SIDE]);
	}

	inline U64 noNeightbourEast(U64 bb) {
		return bb & ~westOne(fileFill(bb));
	}

	inline U64 noNeightbourWest(U64 bb) {
		return bb & ~eastOne(fileFill(bb));
	}

	template <enumSide SIDE>
	inline U64 isolanisMask() {
		return noNeightbourEast(BBs[nWhitePawn + SIDE]) & noNeightbourWest(BBs[nWhitePawn + SIDE]);
	}

	template <enumSide SIDE>
	inline int pawnIslandCount() {
		return bitCount(soutFill(BBs[nWhitePawn + SIDE]) & Constans::r1_rank);
	}

	/*
	template <enumSide SIDE>
	int pawnShieldBonus() {
		if (eval_vector.k_sq[SIDE] % 8 == 0 or eval_vector.k_sq[SIDE] % 8 == 7)
			return 0;
		else if (eval_vector.k_sq[WHITE] / 8 >= 6 or eval_vector.k_sq[BLACK] / 8 <= 1)
			return 0;

		U64 wall = bitU64(eval_vector.k_sq[SIDE]);
		wall |= westOne(wall) | eastOne(wall);

		if constexpr (SIDE) wall = soutOne(wall);
		else wall = nortOne(wall);

		U64 key = wall & BBs[nWhitePawn + SIDE];
		
		if constexpr (SIDE) wall = soutOne(wall);
		else wall = nortOne(wall);

		if constexpr (SIDE) {
			key |= (wall & BBs[nWhitePawn + SIDE]) << 5;
			key >>= (getLS1BIndex(wall) + 5);
		}
		else {
			key |= (wall & BBs[nWhitePawn + SIDE]) >> 5;
			key >>= (getLS1BIndex(wall) - 8);

		}

		return params.pawn_shield_weight[SIDE][key];
	}
	*/

	// evaluation of pawn structure of given side
	template <enumSide SIDE, gState::gPhase Phase>
	int pawnStructureEval() {
		U64 pawns = BBs[nWhitePawn + SIDE], passed = eU64;
		int sq, eval = 0;

		eval_vector.pt_att[SIDE][PAWN] = PawnAttacks::anyAttackPawn<SIDE>(BBs[nWhitePawn + SIDE], UINT64_MAX);

		eval -= bitCount(backwardPawnsMask<SIDE>()) * params.backward_penalty
			+ bitCount(doublePawnsMask<SIDE>()) * params.double_pawn_penalty
			+ bitCount(isolanisMask<SIDE>()) * params.isolanis_penalty;

		eval += bitCount(protectedPawnsMask<SIDE>()) * params.protected_pawn_bonus
			+ bitCount(doubleProtectedPawnsMask<SIDE>()) * params.doubleprot_pawn_bonus;

		while (pawns) {
			sq = popLS1B(pawns);
			eval += params.position_score[Phase][PAWN][flipSquare<SIDE>(sq)];

			// if passed pawn...
			if constexpr (Phase == gState::ENDGAME) {
				if ((LookUp::sf_file.get(SIDE, sq) & (BBs[nWhitePawn] | BBs[nBlackPawn])) == 0ull
					and (LookUp::nf_file.get(SIDE, sq) & BBs[nBlackPawn - SIDE]) == 0ull) {
					passed |= bitU64(sq);
					eval += params.passed_score[flipRank<SIDE>(sq)];
				}
			}
		}

		// save tarrasch masks
		if constexpr (Phase == gState::ENDGAME) {
			eval_vector.tarrasch_passed_msk[SIDE] |= rearspan<SIDE>(passed);
		
			if (!eval_vector.s_pawn_count[SIDE])
				eval -= params.no_pawns_penalty;
		}

		return eval;
	}

	template <enumSide SIDE>
	inline U64 outposMask() {
		return BBs[nWhiteKnight + SIDE] & eval_vector.pt_att[SIDE][PAWN] & ~eval_vector.pt_att[!SIDE][PAWN];
	}

	// evaluation of knights
	template <enumSide SIDE, gState::gPhase Phase>
	int knightEval() {
		int sq, eval = 0, mobility;
		U64 knights = BBs[nWhiteKnight + SIDE], k_att;

		eval_vector.pt_att[SIDE][KNIGHT] = eU64;

		eval += bitCount(outposMask<SIDE>()) * params.k_outpos_bonus;

		while (knights) {
			sq = popLS1B(knights);
			eval += params.position_score[Phase][KNIGHT][flipSquare<SIDE>(sq)];

			// mobily bonus
			k_att = attack<KNIGHT>(UINT64_MAX, sq);
			mobility = bitCount(k_att & ~BBs[nWhite + SIDE]);
			eval += 6 * (mobility - 4);

			eval_vector.pt_att[SIDE][KNIGHT] |= k_att;

			if (k_att & eval_vector.king_zone[!SIDE]) {
				eval_vector.att_count[SIDE]++;
				eval_vector.att_value[SIDE] += params.attacker_weight[KNIGHT]
					* bitCount(k_att & eval_vector.king_zone[!SIDE]);
			}

			eval += params.k_pawns_onboard_bonus[eval_vector.s_pawn_count[SIDE]];
		}

		return eval;
	}

	// evaluation of bishops
	template <enumSide SIDE, gState::gPhase Phase>
	int bishopEval() {
		int sq, eval = 0, b_count = 0, mobility;
		U64 bishops = BBs[nWhiteBishop + SIDE], b_att;

		eval_vector.pt_att[SIDE][BISHOP] = eU64;

		while (bishops) {
			sq = popLS1B(bishops);
			b_count++;
			eval += params.position_score[Phase][BISHOP][flipSquare<SIDE>(sq)];
			
			// mobility credit
			b_att = attack<BISHOP>(BBs[nOccupied], sq);
			mobility = bitCount(b_att & ~BBs[nWhite + SIDE]);
			eval += 5 * (mobility - 7);

			eval_vector.pt_att[SIDE][BISHOP] |= b_att;

			if (b_att & eval_vector.king_zone[!SIDE]) {
				eval_vector.att_count[SIDE]++;
				eval_vector.att_value[SIDE] += params.attacker_weight[BISHOP]
					* bitCount(b_att & eval_vector.king_zone[!SIDE]);
			}

			// bishop's value increasing while number of pawns is decreasing
			eval += params.b_pawns_onboard_bonus[eval_vector.s_pawn_count[SIDE]];
		}

		// bishop pair bonus
		eval += (b_count >= 2) * params.b_pair_bonus;
		return eval;
	}

	// evaluation of rooks
	template <enumSide SIDE, gState::gPhase Phase>
	int rookEval() {
		int sq, eval = 0, mobility;
		U64 rooks = BBs[nWhiteRook + SIDE], r_att;
		
		int mobility_weight = 0;

		if constexpr (Phase == gState::OPENING) mobility_weight = 1;
		else mobility_weight = 2;

		eval_vector.pt_att[SIDE][ROOK] = eU64;

		eval += bitCount(eval_vector.open_files[SIDE] & BBs[nWhiteRook + SIDE])
			* params.r_open_file_bonus
			+ bitCount(fileFill(BBs[nWhiteRook + SIDE]) & BBs[nWhiteQueen + SIDE])
			* params.r_queen_file_bonus;

		if constexpr (Phase == gState::ENDGAME)
			eval += bitCount(BBs[nWhiteRook + SIDE] & eval_vector.tarrasch_passed_msk[SIDE])
				* params.r_tarrasch_file_bonus;

		while (rooks) {
			sq = popLS1B(rooks);
			eval += params.position_score[Phase][ROOK][flipSquare<SIDE>(sq)];
			
			// mobility credit
			r_att = attack<ROOK>(BBs[nOccupied], sq);
			mobility = bitCount(r_att & ~BBs[nWhite + SIDE]);
			eval += mobility_weight * 3 * (mobility - 7);

			eval_vector.pt_att[SIDE][ROOK] |= r_att;

			if (r_att & eval_vector.king_zone[!SIDE]) {
				eval_vector.att_count[SIDE]++;
				eval_vector.att_value[SIDE] += params.attacker_weight[ROOK]
					* bitCount(r_att & eval_vector.king_zone[!SIDE]);
			}

			// rook's value increasing as number of pawns is decreasing
			eval += params.r_pawns_onboard_bonus[eval_vector.s_pawn_count[SIDE]];
		}

		return eval;
	}

	// evaluation of queens
	template <enumSide SIDE, gState::gPhase Phase>
	int queenEval() {
		int sq, eval = 0, mobility;
		U64 queens = BBs[nWhiteQueen + SIDE], q_att;
		
		int mobility_weight;

		if constexpr (Phase == gState::OPENING) mobility_weight = 2;
		else mobility_weight = 3;

		eval_vector.pt_att[SIDE][QUEEN] = eU64;

		if constexpr (Phase == gState::OPENING)
			eval -= bitCount(BBs[nWhiteQueen + SIDE] & Constans::develop_squares[SIDE])
			* params.q_develop_penalty;

		while (queens) {
			sq = popLS1B(queens);
			eval += params.position_score[Phase][QUEEN][flipSquare<SIDE>(sq)];
	
			// mobility
			q_att = attack<QUEEN>(BBs[nOccupied], sq);
			mobility = bitCount(q_att & ~BBs[nWhite + SIDE]);
			eval += mobility_weight * (mobility - 14);

			eval_vector.pt_att[SIDE][QUEEN] |= q_att;

			if (q_att & eval_vector.king_zone[!SIDE]) {
				eval_vector.att_count[SIDE]++;
				eval_vector.att_value[SIDE] += params.attacker_weight[QUEEN]
					* bitCount(q_att & eval_vector.king_zone[!SIDE]);
			}

			eval += params.b_pawns_onboard_bonus[eval_vector.s_pawn_count[SIDE]] 
				+ params.r_pawns_onboard_bonus[eval_vector.s_pawn_count[SIDE]];
		}

		return eval;
	}

	// king evaluation
	template <enumSide SIDE, gState::gPhase Phase>
	int kingEval() {

		// king zone control
		int eval = -eval_vector.att_value[!SIDE] * params.attack_count_weight[eval_vector.att_count[!SIDE]] / 100;
		eval += params.position_score[Phase][KING][flipSquare<SIDE>(eval_vector.k_sq[SIDE])];

		if constexpr (Phase != gState::ENDGAME) {

			// check castling possibility
			if constexpr (Phase == gState::OPENING)
				if (isCastle<SIDE>()) 
					eval += params.k_castle_bonus;
		}

		return eval;
	}

	template <enumSide SIDE, gState::gPhase Phase>
	int templEval(int alpha, int beta) {
		
		// material difference and pawn structure evaluation
		int eval = game_state.material[SIDE] - game_state.material[!SIDE]
			+ pawnStructureEval<SIDE, Phase>() - pawnStructureEval<!SIDE, Phase>()
			+ knightEval<SIDE, Phase>() - knightEval<!SIDE, Phase>()
			+ bishopEval<SIDE, Phase>() - bishopEval<!SIDE, Phase>()
			+ rookEval<SIDE, Phase>() - rookEval<!SIDE, Phase>()
			+ queenEval<SIDE, Phase>() - queenEval<!SIDE, Phase>()
			+ kingEval<SIDE, Phase>() - kingEval<!SIDE, Phase>();

		if (SIDE == game_state.turn) eval += params.tempo_bonus;

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
		eval_vector.reset();

		const int total_mat_no_kings = 16 * params.piece_material[PAWN]
									+ 4 * params.piece_material[KNIGHT] 
									+ 4 * params.piece_material[BISHOP]
									+ 4 * params.piece_material[ROOK] 
									+ 2 * params.piece_material[QUEEN];

		game_state.material[WHITE] = material<WHITE>();
		game_state.material[BLACK] = material<BLACK>();

		// compute evaluation values
		// as phase is: (100% opening, 0% endgame)
		const int op_eval = sideEval<gState::OPENING>(alpha, beta);

		eval_vector.clear();

		// and as phase is: (0% opening, 100% endgame)
		const int eg_eval = sideEval<gState::ENDGAME>(alpha, beta);

		// compute current phase of the game,
		// where biggest possible material is opening phase and 0 - endgame phase
		const int phase = game_state.material[WHITE] + game_state.material[BLACK];

		// final tapered eval result as linear approximation/interpolation
		return eg_eval + phase * (op_eval - eg_eval) / total_mat_no_kings;
	}

} // namespace Eval