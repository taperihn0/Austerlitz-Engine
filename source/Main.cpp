#include "HeaderFile.h"
#include <string>

// global variables storing universal piece data
BitBoardsSet BBs;

CSinglePieceAttacks<PAWN> cpawn_attacks;
CSinglePieceAttacks<KNIGHT> cknight_attacks;
CSinglePieceAttacks<KING> cking_attacks;

mData mdata;
gState game_state;

int main() {
	std::ios::sync_with_stdio(false);
	InitState::initMAttacksTables<ROOK>();
	InitState::initMAttacksTables<BISHOP>();
	
	BBs.parseFEN(BitBoardsSet::start_pos);
	//BBs.parseFEN("3Q4/pp2r3/6p1/1p2kP2/1pPnP2r/7p/8/1K5N w - - 0 1");
	BBs.printBoard();

	MoveGenerator::Analisis::perft(6);
}