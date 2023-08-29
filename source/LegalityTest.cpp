#include "LegalityTest.h"


U64 xRayBishopAttack(U64 occ, U64 blockers, int sq) {
	U64 default_attacks = attack<BISHOP>(occ, sq);
	blockers &= default_attacks;
	return default_attacks ^ attack<BISHOP>(occ ^ blockers, sq);
}

U64 xRayRookAttack(U64 occ, U64 blockers, int sq) {
	U64 default_attacks = attack<ROOK>(occ, sq);
	blockers &= default_attacks;
	return default_attacks ^ attack<ROOK>(occ ^ blockers, sq);
}


// return true whether given piece is attacked by any of 
// opponent piece, else return false
template <enumSide PC_SIDE>
bool isSquareAttacked(int sq) {

	// check pawn attack
	if (BBs[nBlackPawn - PC_SIDE] & cpawn_attacks[PC_SIDE][sq]) {
		return true;
	}

	// check knight attack
	if (BBs[nBlackKnight - PC_SIDE] & cknight_attacks[sq]) {
		return true;
	}

	// same for king attacks
	if (BBs[nBlackKing - PC_SIDE] & cking_attacks[sq]) {
		return true;
	}

	// sliding pieces attack
	U64 queen = BBs[nBlackQueen - PC_SIDE],
		rookQueen = BBs[nBlackRook - PC_SIDE] | queen,
		bishopQueen = BBs[nBlackBishop - PC_SIDE] | queen,
		occ = BBs[nOccupied];

	if (bishopQueen & attack<BISHOP>(occ & ~BBs[nWhiteKing + PC_SIDE], sq)) {
		return true;
	}

	// final checking
	return (rookQueen & attack<ROOK>(occ & ~BBs[nWhiteKing + PC_SIDE], sq));
}


template bool isSquareAttacked<WHITE>(int);
template bool isSquareAttacked<BLACK>(int);


// return bitboard of black attackers, if pc_side is white,
// else return white attackers, if pc_side is black
// if PC is KING, we don't have to check opposite king attacks
template <enumSide PC_SIDE, enumPiece PC>
U64 attackTo(int sq) {
	U64 queen = BBs[nBlackQueen - PC_SIDE],
		rookQueen = BBs[nBlackRook - PC_SIDE] | queen,
		bishopQueen = BBs[nBlackBishop - PC_SIDE] | queen,
		occ = BBs[nOccupied];

	// skipping king attacks if checked square is occupied by king
	return (BBs[nBlackPawn - PC_SIDE] & cpawn_attacks[PC_SIDE][sq])
		| (BBs[nBlackKnight - PC_SIDE] & cknight_attacks[sq])
		| ((PC != KING) * (BBs[nBlackKing - PC_SIDE] & cking_attacks[sq]))
		| (bishopQueen & attack<BISHOP>(occ, sq))
		| (rookQueen & attack<ROOK>(occ, sq));
}

template U64 attackTo<WHITE, KING>(int);
template U64 attackTo<BLACK, KING>(int);
template U64 attackTo<WHITE>(int);
template U64 attackTo<BLACK>(int);


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

template U64 pinnedPiece<WHITE>(int);
template U64 pinnedPiece<BLACK>(int);


// return bitboard of pinners pieces of given color
template <enumSide SIDE>
U64 pinnersPiece(int own_king_sq) {
	U64 own_side_occ = BBs[nWhite + SIDE],
		opRookQueen = BBs[nBlackRook - SIDE] | BBs[nBlackQueen - SIDE],
		opBishopQueen = BBs[nBlackBishop - SIDE] | BBs[nBlackQueen - SIDE];

	return (opRookQueen & xRayRookAttack(BBs[nOccupied], own_side_occ, own_king_sq)) &
		(opBishopQueen & xRayBishopAttack(BBs[nOccupied], own_side_occ, own_king_sq));
}

template U64 pinnersPiece<WHITE>(int);
template U64 pinnersPiece<BLACK>(int);