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
	
	//BBs.parseFEN(BitBoardsSet::start_pos);
	//BBs.parseFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
	//BBs.parseFEN("rnbqkbnr/pppp1ppp/8/8/3PpP2/4B3/PPP1P1PP/RN1QKBNR b KQkq f4 0 1");
	//BBs.printBoard();

	std::string fen;

	while (std::getline(std::cin, fen)) {
		BBs.parseFEN(fen);
		MoveGenerator::Analisis::perft<4>();
	}
}