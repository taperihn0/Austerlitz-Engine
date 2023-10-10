#pragma once

#include "BitBoardsSet.h"
#include "StaticLookup.h"
#include "LegalityTest.h"


namespace Eval {
	
	namespace LookUp {
		// backward pawn lookup table of masks
		constexpr auto back_file = cexpr::CexprArr<true, U64, 2, 64>([](bool side, int sq) constexpr -> U64 {
			const U64 same_bfile = inBetween(sq, (side ? 56 : 0) + sq % 8) | bitU64(sq);
			return eastOne(same_bfile) | westOne(same_bfile);
		});

		// masks for isolated pawn (neighbour ranks)
		constexpr auto n_file = cexpr::CexprArr<false, U64, 64>([](int sq) constexpr -> U64 {
			const U64 same_wfile = Constans::f_by_index[sq % 8];
			return eastOne(same_wfile) | westOne(same_wfile);
		});

		// same forward rank
		constexpr auto sf_file = cexpr::CexprArr<true, U64, 2, 64>([](bool side, int sq) constexpr -> U64 {
			return inBetween(sq, (side ? 0 : 56) + sq % 8);
		});

		// forward file masks for passed pawns
		constexpr auto nf_file = cexpr::CexprArr<true, U64, 2, 64>([](bool side, int sq) constexpr -> U64 {
			const U64 same_forward_rank = sf_file.get(side, sq);
			return eastOne(same_forward_rank) | westOne(same_forward_rank);
		});
	}

	// resources for evaluating a position
	namespace Value {

		/*
		* Pawn = 1 Point
		* Knight = 3.25 Points
		* Bishop = 3.25 Points
		* Rook = 5 Points
		* Queen = 9.75 Points
		* (accurate pieces values according to GM Larry Kaufman)
		*/

		// enum of relevant values used in evaluation functions
		enum PieceValue {
			PAWN_VALUE = 100,
			KNIGHT_VALUE = 325,
			BISHOP_VALUE = 325,
			ROOK_VALUE = 500,
			QUEEN_VALUE = 975,
			KING_VALUE = 10000
		};

		constexpr std::array<int, 12> piece_material = {
			PAWN_VALUE, KNIGHT_VALUE, BISHOP_VALUE, ROOK_VALUE, QUEEN_VALUE, KING_VALUE,
		};

		// single lookup table for score of each square for evary single piece
		using posScoreTab = std::array<int, 64>;

		constexpr posScoreTab pawn_score = {
			 0,  0,  0,  0,  0,  0,  0,  0,
			50, 50, 50, 50, 50, 50, 50, 50,
			10, 10, 20, 30, 30, 20, 10, 10,
			 5,  5, 10, 25, 25, 10,  5,  5,
			 0,  0,  0, 20, 20,  0,  0,  0,
			 5, -5,-10,  0,  0,-10, -5,  5,
			 5, 10, 10,-20,-20, 10, 10,  5,
			 0,  0,  0,  0,  0,  0,  0,  0
		};

		constexpr posScoreTab knight_score = {
			-50,-40,-30,-30,-30,-30,-40,-50,
			-40,-20,  0,  0,  0,  0,-20,-40,
			-30,  0, 10, 15, 15, 10,  0,-30,
			-30,  5, 15, 20, 20, 15,  5,-30,
			-30,  0, 15, 20, 20, 15,  0,-30,
			-30,  5, 10, 15, 15, 10,  5,-30,
			-40,-20,  0,  5,  5,  0,-20,-40,
			-50,-40,-30,-30,-30,-30,-40,-50,
		};

		constexpr posScoreTab bishop_score = {
			-20,-10,-10,-10,-10,-10,-10,-20,
			-10,  0,  0,  0,  0,  0,  0,-10,
			-10,  0,  5, 10, 10,  5,  0,-10,
			-10,  5,  5, 10, 10,  5,  5,-10,
			-10,  0, 10, 10, 10, 10,  0,-10,
			-10, 10, 10, 10, 10, 10, 10,-10,
			-10,  5,  0,  0,  0,  0,  5,-10,
			-20,-10,-10,-10,-10,-10,-10,-20,
		};

		constexpr posScoreTab rook_score = {
			  0,  0,  0,  0,  0,  0,  0,  0,
			  5, 10, 10, 10, 10, 10, 10,  5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			  0,  0,  0,  5,  5,  0,  0,  0
		};

		constexpr posScoreTab queen_score = {
			-20,-10,-10, -5, -5,-10,-10,-20,
			-10,  0,  0,  0,  0,  0,  0,-10,
			-10,  0,  5,  5,  5,  5,  0,-10,
			 -5,  0,  5,  5,  5,  5,  0, -5,
			  0,  0,  5,  5,  5,  5,  0, -5,
			-10,  5,  5,  5,  5,  5,  0,-10,
			-10,  0,  5,  0,  0,  0,  0,-10,
			-20,-10,-10, -5, -5,-10,-10,-20
		};

		constexpr posScoreTab king_score = {
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-20,-30,-30,-40,-40,-30,-30,-20,
			-10,-20,-20,-20,-20,-20,-20,-10,
			 20, 20,  0,  0,  0,  0, 20, 20,
			 20, 30, 10,  0,  0, 10, 30, 20
		};

		// easily aggregated lookups
		using aggregateScoreTab = std::array<posScoreTab, 6>;
		constexpr aggregateScoreTab position_score = {
			pawn_score,
			knight_score,
			bishop_score,
			rook_score,
			queen_score,
			king_score
		};

		using passedPawnTab = std::array<int, 7>;
		constexpr passedPawnTab passed_score = { 0, 10, 20, 30, 55, 90, 105 };
	};

	// simple version of evaluation funcion
	inline int simple_evaluation() {
		return Value::PAWN_VALUE * (BBs.count(nWhitePawn + game_state.turn) - BBs.count(nBlackPawn - game_state.turn));
	}
	
	template <enumSide SIDE>
	int templEval();

	// main evaluation system
	inline int evaluate() {
		// setting template parameter so then I won't have to
		return game_state.turn == WHITE ? templEval<WHITE>() : templEval<BLACK>();
	}

	// quiescence search - protect from dangerous consequences of horizon effect
	int qSearch(int alpha, int beta, int Ply);

}