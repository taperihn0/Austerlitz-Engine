#include "HeaderFile.h"

// global variables storing universal piece data
BitBoardsSet BBs;

CSinglePieceAttacks<PAWN> cPawnAttacks;
CSinglePieceAttacks<KNIGHT> cKnightAttacks;
CSinglePieceAttacks<KING> cKingAttacks;


int main() {
	InitState::initMAttacksTables<ROOK>();
	InitState::initMAttacksTables<BISHOP>();

	BBs.parseFEN("3r4/3q3b/8/5P2/3NN3/3KP3/8/8 w - - 0 1");
	BBs.printBoard();

	std::cout << std::endl;
	displayLegalMoves<WHITE>();

	//printBitBoard(pinnedPiece<WHITE>(getLS1BIndex(BBs[nWhiteKing])));
	//printBitBoard(xRayRookAttack(BBs[nOccupied], BBs[nWhite], getLS1BIndex(BBs[nWhiteKing])));
}