#include "Evaluation.h"


namespace Eval {
	
	template <enumSide SIDE>
	int properSquare(int square);

	template <>
	inline int properSquare<WHITE>(int square) {
		return square;
	}

	template <>
	inline int properSquare<BLACK>(int square) {
		return mirrored_square[square];
	}


	template <enumSide SIDE>
	int templEval() {
		int eval = 0, sq;
		U64 piece_bb;

		for (auto piece = nWhitePawn + SIDE; piece <= nBlackKing; piece += 2) {
			piece_bb = BBs[piece];

			while (piece_bb) {
				sq = popLS1B(piece_bb);

				// consider piece material score and his position score
				eval +=
					Value::piece_material[piece] + Value::position_score[toPieceType(piece)][properSquare<SIDE>(sq)];
			}
		}

		return eval;
	}

	template int templEval<WHITE>();
	template int templEval<BLACK>();
}