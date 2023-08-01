#include "HeaderFile.h"

int main() {
	CSinglePieceAttacks<PAWN> arrSingleKnightAttack;
	arrSingleKnightAttack.Init();

	printBitBoard(arrSingleKnightAttack.get(WHITE, g1));
}