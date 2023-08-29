﻿#include "HeaderFile.h"
#include <string>

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
	
	std::string fen;

	while (std::getline(std::cin, fen)) {
		BBs.clear();

		BBs.parseFEN(fen);
		BBs.printBoard();

		MoveGenerator::generateLegalMoves();
		MoveGenerator::populateMoveList();
	}
}