#include "BitBoardsSet.h"
#include "AttackTables.h"
#include "UCI.h"
#include "MagicBitBoards.h"


// global variables storing universal piece data
BitBoardsSet BBs(BitBoardsSet::start_pos);

CSinglePieceAttacks<PAWN> cpawn_attacks;
CSinglePieceAttacks<KNIGHT> cknight_attacks;
CSinglePieceAttacks<KING> cking_attacks;

mData mdata;
gState game_state;

// position startpos moves e2e3 b8c6 c2c3 e7e5 d2d4 d8h4 g1f3 h4f2 e1f2
// > position current e1f2 - engine thinks it's illegal!
int main(int argc, char* argv[]) {
	InitState::initMAttacksTables();
	UCI::goLoop(argc, argv);
}