#include "AttackTables.h"
#include "UCI.h"


// global variables storing universal piece data
BitBoardsSet BBs;

CSinglePieceAttacks<PAWN> cpawn_attacks;
CSinglePieceAttacks<KNIGHT> cknight_attacks;
CSinglePieceAttacks<KING> cking_attacks;

mData mdata;
gState game_state;

int main() {
	InitState::initMAttacksTables<ROOK>();
	InitState::initMAttacksTables<BISHOP>();

	//BBs.parseFEN(BitBoardsSet::start_pos);
	//const auto bm = Search::bestMove();

	//bm.print();

	UCI::goLoop();
}