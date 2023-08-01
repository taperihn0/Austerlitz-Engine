#include "MoveSystem.h"
#include <functional>


// table of attack squares bitboards alias
using a2dTable_t = std::array<std::array<U64, BSIZE>, 2>;

//	SINGLE ATTACK MASKS for PAWNS, KNIGHTS and KINGS
U64 sqMaskPawnAttacks(enumSide side, uint8_t sq);
U64 sqMaskKnightAttacks(enumSide, uint8_t sq);
U64 sqMaskKingAttacks(enumSide, uint8_t sq);

// class template as a custom table of pre-calculated attack tables
template <enumPiece pT>
class CSinglePieceAttacks {
public:
	CSinglePieceAttacks() = default;
	void Init();

	U64 get(enumSide side, enumSquare sq);
private:
	template <enumSide sT>
	void Init();

	a2dTable_t arrAttacks;
};

// General public initialization template method for pT piece type attacks tables
template <enumPiece pT>
void CSinglePieceAttacks<pT>::Init() {
	Init<WHITE>();
	Init<BLACK>();
}

// return given table of attacks for single pT piece type on sq square
template <enumPiece pT>
U64 CSinglePieceAttacks<pT>::get(enumSide side, enumSquare sq) {
	return arrAttacks[side][sq];
}

// Init template function for given as a template parameter piece color and pT piece type
template <enumPiece pT>
template <enumSide sT>
void CSinglePieceAttacks<pT>::Init() {
	std::function<U64(enumSide, uint8_t)> mask;

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

	for (uint8_t i = 0; i < BSIZE; i++) {
		arrAttacks[sT][i] = mask(sT, i);
	}
}