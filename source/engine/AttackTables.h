#pragma once

#include "MoveSystem.h"
#include <functional>


// single attacks masks for leaper pieces 
namespace {

	// mask of avaible attacks for pawn on sq square
	constexpr U64 sqMaskPawnAttacks(enumSide side, int sq) {
		U64 piece = eU64;
		setBit(piece, sq);

		switch (side) {
		case WHITE:
			return noWeOne(piece) | noEaOne(piece);
		case BLACK:
			return soWeOne(piece) | soEaOne(piece);
		default: break;
		}

		return eU64;
	}

	// ... for knight on sq square
	constexpr U64 sqMaskKnightAttacks(enumSide, int sq) {
		U64 piece = eU64;
		setBit(piece, sq);

		// union bitboard of all the single knight attacks
		return noNoEa(piece) | noEaEa(piece) | soEaEa(piece)
			| soSoEa(piece) | noNoWe(piece) | noWeWe(piece)
			| soWeWe(piece) | soSoWe(piece);
	}

	constexpr U64 bbMaskKnightAttacks(U64 bb) {
		return noNoEa(bb) | noEaEa(bb) | soEaEa(bb)
			| soSoEa(bb) | noNoWe(bb) | noWeWe(bb)
			| soWeWe(bb) | soSoWe(bb);
	}

	// ... for king on sq square
	constexpr U64 sqMaskKingAttacks(enumSide, int sq) {
		U64 piece = eU64;
		setBit(piece, sq);

		// union bitboard of all the king surrounding fields
		return nortOne(piece) | noEaOne(piece) | eastOne(piece)
			| soEaOne(piece) | soutOne(piece) | soWeOne(piece) 
			| westOne(piece) | noWeOne(piece);
	}

} // namespace

// class template as a custom table of pre-calculated attack tables
template <enumPiece pT>
class CSinglePieceAttacks {
	// private class aliases
private: 
	using a2dTable_t =
		std::conditional_t<pT == PAWN, std::array<std::array<U64, 64>, 2>, std::array<U64, 64>>;

	using accessRType =
		std::conditional_t<pT == PAWN, const std::array<U64, 64>&, U64>;
public:
	CSinglePieceAttacks();

	constexpr accessRType operator[](size_t side) const;
private:
	template <enumSide sT>
	void Init();

	a2dTable_t arrAttacks;
};


// 'capsulating' template helper functions - 
// no need to keep them in global namespace scope
namespace attArrHelpers {

	template <enumPiece pT, enumSide sT, typename iT>
	inline auto initHelper(iT& arr) -> std::enable_if_t<pT == PAWN> {
		for (int i = 0; i < 64; i++) {
			arr[sT][i] = sqMaskPawnAttacks(sT, i);
		}
	}

	template <enumPiece pT, enumSide sT, typename iT>
	inline auto initHelper(iT& arr) -> std::enable_if_t<pT != PAWN> {
		const std::function<U64(enumSide, int)> mask =
			(pT == KNIGHT) ? sqMaskKnightAttacks : sqMaskKingAttacks;

		for (int i = 0; i < 64; i++) {
			arr[i] = mask(sT, i);
		}
	}

}


// General public initialization for pT piece type attacks tables
template <enumPiece pT>
CSinglePieceAttacks<pT>::CSinglePieceAttacks() {
	Init<WHITE>();
	Init<BLACK>();
}

// return given table of attacks for single pT piece type on sq square
template <enumPiece pT>
inline constexpr typename CSinglePieceAttacks<pT>::accessRType 
CSinglePieceAttacks<pT>::operator[](size_t side) const {
	return arrAttacks[side];
}

// Init template function for given as a template parameter piece color and pT piece type
template <enumPiece pT>
template <enumSide sT>
void CSinglePieceAttacks<pT>::Init() {
	attArrHelpers::initHelper<pT, sT, a2dTable_t>(arrAttacks);
}

// declarations of attack look-up tables for simple pieces
inline CSinglePieceAttacks<PAWN> cpawn_attacks;
inline CSinglePieceAttacks<KNIGHT> cknight_attacks;
inline CSinglePieceAttacks<KING> cking_attacks;