#pragma once

#include "AttackTables.h"
#include "BitBoardsSet.h"
#include "MagicBitBoards.h"
#include "static.h"
#include <cassert>


namespace {

	// construct bitboard of set bits between squares excluding that squares
	// rectangular lookup method
	constexpr U64 inBetweenConstr(int sq1, int sq2) {
		U64 res = eU64;

		const auto sq_m = std::min(sq1, sq2),
			sq_x = std::max(sq1, sq2);

		// loop approach for each case: through file, rank and diagonal
		if (sq1 % 8 == sq2 % 8) {
			for (int i = 1; i < cexpr::abs(sq2 / 8 - sq1 / 8); i++) {
				setBit(res, (sq_m / 8 + i) * 8 + sq1 % 8);
			}
		}
		else if (sq1 / 8 == sq2 / 8) {
			for (int i = 1; i < cexpr::abs(sq2 % 8 - sq1 % 8); i++) {
				setBit(res, sq_m + i);
			}
		}
		else if (cexpr::abs(sq1 % 8 - sq2 % 8) == cexpr::abs(sq1 / 8 - sq2 / 8)) {
			for (int i = 1; i < cexpr::abs(sq1 % 8 - sq2 % 8); i++) {
				setBit(res, sq_m + i * (sq_m % 8 < sq_x % 8 ? 9 : 7));
			}
		}
		return res;
	}

} // namespace

// pre-initialization of compile-time
namespace RectangularLookUp {
	constexpr auto inBetweenArr = cexpr::CexprArr<true, U64, 64, 64>(inBetweenConstr);
}


// get proper inBetween bitboard element and check for index overflow
inline constexpr U64 inBetween(int sq1, int sq2) noexcept {
	assert(0 <= sq1 and sq1 < 64 and 0 <= sq2 and sq2 < 64 && "Rectangular look-up array index overflow");
	return RectangularLookUp::inBetweenArr.get(sq1, sq2);
}

// check whether king is double checked
inline bool isDoubleChecked(U64 king_attackers) noexcept {
	return king_attackers ? (king_attackers & (king_attackers - 1)) : false;
}


U64 xRayBishopAttack(U64 occ, U64 blockers, int sq);
U64 xRayRookAttack(U64 occ, U64 blockers, int sq);
U64 xRayQueenAttack(U64 occ, U64 blockers, int sq);


// is-square-attacked funtions returing true whether square
// is attacked by opponent piece

template <enumSide PC_SIDE>
bool isSquareAttacked(int sq);

inline bool isSquareAttacked(int sq, bool side) {
	return side ? 
		isSquareAttacked<BLACK>(sq) : 
		isSquareAttacked<WHITE>(sq);
}

bool isBySliderAttacked(int sq, bool side, U64 occ);

template <enumSide PC_SIDE, enumPiece PC = ANY>
U64 attackTo(int sq);
U64 attackTo(int sq, bool side);
U64 attackTo(int sq, bool side, U64 occ);


template <enumSide SIDE>
U64 pinnedPiece(int own_king_sq);
U64 pinnedPiece(int own_king_sq, bool side);


template <enumSide SIDE>
U64 pinnedHorizonVertic(int own_king_sq);
U64 pinnedHorizonVertic(int own_king_sq, bool side);

template <enumSide SIDE>
U64 pinnedDiagonal(int own_king_sq);
U64 pinnedDiagonal(int own_king_sq, bool side);

template <enumSide SIDE>
U64 pinnersPiece(int own_king_sq);
U64 pinnersPiece(int own_king_sq, U64 occ, U64 blockers, bool side);