#include "HeaderFile.h"

// global variables storing universal piece data
BitBoardsSet BBs;

CSinglePieceAttacks<PAWN> cPawnAttacks;
CSinglePieceAttacks<KNIGHT> cKnightAttacks;
CSinglePieceAttacks<KING> cKingAttacks;


int main() {
	InitState::initMAttacksTables<ROOK>();
	InitState::initMAttacksTables<BISHOP>();

	BBs.parseFEN("8/8/2pk4/2bqr3/8/6B1/2R4K/3Q4 w - - 0 1");
	BBs.printBoard();

	std::cout << std::endl;
	//displayLegalMoves<BLACK>();

	//printBitBoard(xRayBishopAttack(BBs[nOccupied], BBs[nBlack], getLS1BIndex(BBs[nBlackKing])));
	//printBitBoard(xRayRookAttack(BBs[nOccupied], BBs[nWhite], getLS1BIndex(BBs[nWhiteKing])));
}