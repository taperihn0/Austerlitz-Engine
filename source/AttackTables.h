#pragma once

#include "MoveSystem.h"
#include <functional>

// table of attack squares bitboards alias
using a2dTable_t = std::array<std::array<U64, 64>, 2>;

//	SINGLE ATTACK MASKS for PAWNS, KNIGHTS and KINGS
namespace {

	// pre-generated mask of avaible attacks for pawn on sq square
	U64 sqMaskPawnAttacks(enumSide side, int sq) {
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
	U64 sqMaskKnightAttacks(enumSide, int sq) {
		U64 piece = eU64;
		setBit(piece, sq);

		// union bitboard of all the single knight attacks
		return noNoEa(piece) | noEaEa(piece) | soEaEa(piece)
			| soSoEa(piece) | noNoWe(piece) | noWeWe(piece)
			| soWeWe(piece) | soSoWe(piece);
	}

	// ... for king on sq square
	U64 sqMaskKingAttacks(enumSide, int sq) {
		U64 piece = eU64;
		setBit(piece, sq);

		// union bitboard of all the king surrounding fields
		return nortOne(piece) | noEaOne(piece) | eastOne(piece)
			| soEaOne(piece) | soutOne(piece) | soWeOne(piece) |
			westOne(piece) | noWeOne(piece);
	}

} // namespace

// class template as a custom table of pre-calculated attack tables
template <enumPiece pT>
class CSinglePieceAttacks {
public:
	CSinglePieceAttacks();

	const std::array<U64, 64>& operator[](enumSide side);
private:
	template <enumSide sT>
	void Init();

	a2dTable_t arrAttacks;
};

// General public initialization for pT piece type attacks tables
template <enumPiece pT>
CSinglePieceAttacks<pT>::CSinglePieceAttacks() {
	Init<WHITE>();
	Init<BLACK>();
}

// return given table of attacks for single pT piece type on sq square
template <enumPiece pT>
inline const std::array<U64, 64>& CSinglePieceAttacks<pT>::operator[](enumSide side) {
	return arrAttacks[side];
}

// Init template function for given as a template parameter piece color and pT piece type
template <enumPiece pT>
template <enumSide sT>
void CSinglePieceAttacks<pT>::Init() {
	std::function<U64(enumSide, int)> mask;

	switch (pT) {
	case PAWN:
		mask = &sqMaskPawnAttacks;
		break;
	case KNIGHT:
		mask = &sqMaskKnightAttacks;
		break;
	case KING:
		mask = &sqMaskKingAttacks;
		break;
	default: return;
	}

	for (int i = 0; i < 64; i++) {
		arrAttacks[sT][i] = mask(sT, i);
	}
}

// declarations of attack look-up tables for simple pieces
extern CSinglePieceAttacks<PAWN> cPawnAttacks;
extern CSinglePieceAttacks<KNIGHT> cKnightAttacks;
extern CSinglePieceAttacks<KING> cKingAttacks;