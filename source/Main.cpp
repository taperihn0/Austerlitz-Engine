#include "HeaderFile.h"

int main() {
	initMAttacksTables<BISHOP>();

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
	printBitBoard(bishopAttack(bb, c3));

	//int n = bitCount(bb);

	//for (int i = 0; i < (1 << n); i++) {
	//	printBitBoard(indexSubsetU64(i, bb, n));
	//}
}