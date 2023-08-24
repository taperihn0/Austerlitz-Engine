#pragma once

#include "AttackTables.h"
#include "BitBoardsSet.h"
#include "MagicBitBoards.h"
#include "abs.h"
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

	// pre-run-called functions
	namespace InitState {
		
		// resources for initialization compile-time array
		namespace cexprArr {

			struct CArr {
				constexpr CArr()
					: arr{} {
					for (int i = 0; i < 64; i++) {
						for (int j = 0; j < 64; j++) {
							arr[i][j] = inBetweenConstr(i, j);
						}
					}
				}

				U64 arr[64][64];
			};

		}
	}

} // namespace


// pre-initialization of compile-time
namespace RectangularLookUp {
	constexpr auto inBetweenArr = InitState::cexprArr::CArr();
}


// get proper inBetween bitboard element and check for index overflow
inline U64 inBetween(int sq1, int sq2) {
	assert(0 <= sq1 and sq1 < 64 and 0 <= sq2 and sq2 < 64 && "Rectangular look-up array index overflow");
	return RectangularLookUp::inBetweenArr.arr[sq1][sq2];
}

// check whether king is double checked
inline bool isDoubleChecked(U64 king_attackers) noexcept {
	return king_attackers ? (king_attackers & (king_attackers - 1)) : false;
}


namespace {

	U64 xRayBishopAttack(U64 occ, U64 blockers, int sq) {
		U64 default_attacks = bishopAttack(occ, sq);
		blockers &= default_attacks;
		return default_attacks ^ bishopAttack(occ ^ blockers, sq);
	}

	U64 xRayRookAttack(U64 occ, U64 blockers, int sq) {
		U64 default_attacks = rookAttack(occ, sq);
		blockers &= default_attacks;
		return default_attacks ^ rookAttack(occ ^ blockers, sq);
	}

} // namespace


// return true whether given piece is attacked by any of 
// opponent piece, else return false
template <enumSide PC_SIDE, enumPiece PC = ANY>
bool isSquareAttacked(int sq) {

	// check pawn attack
	if (BBs[nBlackPawn - PC_SIDE] & cPawnAttacks[PC_SIDE][sq]) {
		return true;
	}

	// check knight attack
	if (BBs[nBlackKnight - PC_SIDE] & cKnightAttacks[PC_SIDE][sq]) {
		return true;
	}

	// if checked square is occupied by KING, we can skip this statement
	// since king can't attack other king
	if (PC != KING and (BBs[nBlackKing - PC_SIDE] & cKnightAttacks[PC_SIDE][sq])) {
		return true;
	}

	// sliding pieces attack
	U64 queen = BBs[nBlackQueen - PC_SIDE],
		rookQueen = BBs[nBlackRook - PC_SIDE] | queen,
		bishopQueen = BBs[nBlackBishop - PC_SIDE] | queen,
		occ = BBs[nOccupied];

	if (bishopQueen & bishopAttack(occ, sq)) {
		return true;
	}

	// final checking
	return (rookQueen & rookAttack(occ, sq));
}

// return bitboard of black attackers, if pc_side is white,
// else return white attackers, if pc_side is black
template <enumSide PC_SIDE, enumPiece PC = ANY>
U64 attackTo(int sq) {
	U64 queen = BBs[nBlackQueen - PC_SIDE],
		rookQueen = BBs[nBlackRook - PC_SIDE] | queen,
		bishopQueen = BBs[nBlackBishop - PC_SIDE] | queen,
		occ = BBs[nOccupied];

	// skipping king attacks if checked square is occupied by king
	return (BBs[nBlackPawn - PC_SIDE] & cPawnAttacks[PC_SIDE][sq])
		| (BBs[nBlackKnight - PC_SIDE] & cKnightAttacks[PC_SIDE][sq])
		| ((PC != KING) * (BBs[nBlackKing - PC_SIDE] & cKingAttacks[PC_SIDE][sq]))
		| (bishopQueen & bishopAttack(occ, sq))
		| (rookQueen & rookAttack(occ, sq));
}

// return bitboard of pinned piece of given color
template <enumSide SIDE>
U64 pinnedPiece(int own_king_sq) {
	U64 own_side_occ = BBs[nWhite + SIDE],
		opRookQueen = BBs[nBlackRook - SIDE] | BBs[nBlackQueen - SIDE],
		opBishopQueen = BBs[nBlackBishop - SIDE] | BBs[nBlackQueen - SIDE],
		pinner = opRookQueen & xRayRookAttack(BBs[nOccupied], own_side_occ, own_king_sq),
		pinned = eU64;

	// processing pins performed by file and rank lines - rooks and queens
	while (pinner) {
		pinned |= own_side_occ & inBetween(getLS1BIndex(pinner), own_king_sq);

		// get rid of least significant bit
		pinner &= pinner - 1;
	}

	// pins on diagonal lines - bishop and queens
	pinner = opBishopQueen & xRayBishopAttack(BBs[nOccupied], own_side_occ, own_king_sq);
	while (pinner) {
		pinned |= own_side_occ & inBetween(getLS1BIndex(pinner), own_king_sq);

		// get rid of least significant bit
		pinner &= pinner - 1;
	}

	return pinned;
}

// return bitboard of pinners pieces of given color
template <enumSide SIDE>
U64 pinnersPiece(int own_king_sq) {
	U64 own_side_occ = BBs[nWhite + SIDE],
		opRookQueen = BBs[nBlackRook - SIDE] | BBs[nBlackQueen - SIDE],
		opBishopQueen = BBs[nBlackBishop - SIDE] | BBs[nBlackQueen - SIDE];

	return (opRookQueen & xRayRookAttack(BBs[nOccupied], own_side_occ, own_king_sq)) &
		(opBishopQueen & xRayBishopAttack(BBs[nOccupied], own_side_occ, own_king_sq));
}