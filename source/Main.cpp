#include "HeaderFile.h"

int main() {
	/*
	initMAttacksTables<BISHOP>();
	initMAttacksTables<ROOK>();

	U64 bb = eU64;
	setBit(bb, f4);
	setBit(bb, e4);
	setBit(bb, a2);
	setBit(bb, g7);
	setBit(bb, g6);
	setBit(bb, g5);
	setBit(bb, d4);
	setBit(bb, c2);
	setBit(bb, e6);

	printBitBoard(bb);
	printBitBoard(queenAttack(bb, f5));
	*/

	BitBoardsSet BBs("4k2r/6r1/8/8/8/8/3R4/R3K3 w Qk - 0 1");
	BBs.printBoard();
}