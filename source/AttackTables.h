#include "MoveSystem.h"

// attack table alias
using a2dTable_t = std::array<std::array<U64, BSIZE>, 2>;

//	SINGLE PAWN ATTACK TABLES
class classSinglePawnAttacks {
public:
	classSinglePawnAttacks() = default;
	void Init();

	U64 get(enumSide side, enumSquare sq);
	static U64 sqMaskPawnAttack(enumSide side, uint8_t sq);
private:
	template <enumSide sT>
	void Init();

	a2dTable_t tabSinglePawnAttack;
};

// classSinglePawnAttacks class method template implementation:
template <enumSide sT>
void classSinglePawnAttacks::Init() {
	for (uint8_t i = 0; i < BSIZE; i++) {
		tabSinglePawnAttack[sT][i] = sqMaskPawnAttack(sT, i);
	}
}


//	SINGLE KNIGHT ATTACK TABLES
class classSingleKnightAttacks {
public:
	classSingleKnightAttacks() = default;
	void Init();

	U64 get(enumSide side, enumSquare sq);
	static U64 spMaskKnightAttack(uint8_t sq);
private:
	template <enumSide sT>
	void Init();

	a2dTable_t tabSingleKnightAttack;
};

// classSingleKnightAttacks class method template implementation:
template <enumSide sT>
void classSingleKnightAttacks::Init() {
	for (uint8_t i = 0; i < BSIZE; i++) {
		tabSingleKnightAttack[sT][i] = spMaskKnightAttack(i);
	}
}
