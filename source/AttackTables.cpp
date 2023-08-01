#include "AttackTables.h"

// pre-generated mask of avaible attacks for pawn on sq square
U64 sqMaskPawnAttacks(enumSide side, uint8_t sq) {
	U64 piece = eU64;
	setBit(piece, sq);

	switch (side) {
	case WHITE:
		return noWeOne(piece) | noEaOne(piece);
		break;
	case BLACK:
		return soWeOne(piece) | soEaOne(piece);
		break;
	default:
		break;
	}

	return eU64;
}

// ... for pawn on sq square
U64 sqMaskKnightAttacks(enumSide, uint8_t sq) {
	U64 piece = eU64;
	setBit(piece, sq);

	// union bitboard of all the single knight attacks
	return noNoEa(piece) | noEaEa(piece) | soEaEa(piece) 
			| soSoEa(piece) | noNoWe(piece) | noWeWe(piece) 
			 | soWeWe(piece) | soSoWe(piece);
}

// ... for king on sq square
U64 sqMaskKingAttacks(enumSide, uint8_t sq) {
	U64 piece = eU64;
	setBit(piece, sq);

	// union bitboard of all the king surrounding fields
	return nortOne(piece) | noEaOne(piece) | eastOne(piece)
		| soEaOne(piece) | soutOne(piece) | soWeOne(piece) |
		westOne(piece) | noWeOne(piece);
}