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

	std::string fen = "K7/N3PPB1/1p4p1/P2Q4/2p3R1/1k6/1Pb3p1/q7 b - - 0 1";

	//while (std::getline(std::cin, fen)) {
		BBs.parseFEN(fen);
		MoveGenerator::Analisis::perft<3>();
	//}
}