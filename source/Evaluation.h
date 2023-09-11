#pragma once

#include "BitBoardsSet.h"


namespace Eval {
	
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
			-PAWN_VALUE, -KNIGHT_VALUE, -BISHOP_VALUE, -ROOK_VALUE, -QUEEN_VALUE, -KING_VALUE,
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

}