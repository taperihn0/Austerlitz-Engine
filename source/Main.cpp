#include "HeaderFile.h"

int main() {
	classSingleKnightAttacks arrSingleKnightAttack;
	arrSingleKnightAttack.Init();

	printBitBoard(arrSingleKnightAttack.get(BLACK, g1));
}