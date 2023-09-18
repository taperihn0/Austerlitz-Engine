#include "BitBoardsSet.h"
#include "AttackTables.h"
#include "UCI.h"
#include "MagicBitBoards.h"
#include "Search.h"
#include "MoveOrder.h"

// global variables storing universal piece data
BitBoardsSet BBs(BitBoardsSet::start_pos);

CSinglePieceAttacks<PAWN> cpawn_attacks;
CSinglePieceAttacks<KNIGHT> cknight_attacks;
CSinglePieceAttacks<KING> cking_attacks;

mData mdata;
gState game_state;

Search::Results Search::search_results;
Order::killerLookUp Order::killer;

int main(int argc, char* argv[]) {
	InitState::initMAttacksTables();
	UCI::goLoop(argc, argv);
}