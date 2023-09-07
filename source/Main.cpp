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

	std::string fen = "n6Q/4Pkp1/K1Pp4/2pP3P/4B1q1/P1p2p2/8/8 w - - 0 1";

	//while (std::getline(std::cin, fen)) {
		BBs.parseFEN(fen);
		MoveGenerator::Analisis::perft<6>();
	//}
}