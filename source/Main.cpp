#include "HeaderFile.h"

// global variables storing universal piece data
BitBoardsSet BBs;

CSinglePieceAttacks<PAWN> cPawnAttacks;
CSinglePieceAttacks<KNIGHT> cKnightAttacks;
CSinglePieceAttacks<KING> cKingAttacks;


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

	InitState::initMAttacksTables<ROOK>();
	InitState::initMAttacksTables<BISHOP>();

	BBs.parseFEN("2r4k/8/8/Q7/3N4/8/8/2K5 w - - 0 1");
	BBs.printBoard();

	std::cout << std::endl;
	displayLegalMoves<WHITE>();
}