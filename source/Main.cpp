#include "HeaderFile.h"

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
	//BBs.parseFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
	//BBs.parseFEN("rnb1kbnr/pppp1ppp/8/4p3/5P1q/8/PPPPPKPP/RNBQ1BNR w kq - 0 1");
	BBs.printBoard();

	MoveGenerator::Analisis::perft(6);
}