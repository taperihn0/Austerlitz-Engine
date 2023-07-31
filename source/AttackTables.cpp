#include "AttackTables.h"

// General public initialization function for pawns attacks tables
void classSinglePawnAttacks::Init() {
	Init<WHITE>();
	Init<BLACK>();
}

// return given table of avaible attacks for single pawn on sq square
U64 classSinglePawnAttacks::get(enumSide side, enumSquare sq) {
	return tabSinglePawnAttack[sCast<size_t>(side)][sCast<size_t>(sq)];
}

// pre-generated mask of avaible attacks for pawn on sq square
U64 classSinglePawnAttacks::sqMaskPawnAttack(enumSide side, uint8_t sq) {
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


// General public initialization function for knights attacks tables
void classSingleKnightAttacks::Init() {
	Init<WHITE>();
	Init<BLACK>();
}

// return given table of avaible attacks for single knight on sq square
U64 classSingleKnightAttacks::get(enumSide side, enumSquare sq) {
	return tabSingleKnightAttack[sCast<size_t>(side)][sCast<size_t>(sq)];
}

// pre-generated mask of avaible attacks for pawn on sq square
U64 classSingleKnightAttacks::spMaskKnightAttack(uint8_t sq) {
	U64 piece = eU64;
	setBit(piece, sq);

	// union bitboard of all the single knight attacks
	return noNoEa(piece) | noEaEa(piece) | soEaEa(piece) 
			| soSoEa(piece) | noNoWe(piece) | noWeWe(piece) 
			 | soWeWe(piece) | soSoWe(piece);
}