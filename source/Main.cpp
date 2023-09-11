﻿#include "AttackTables.h"
#include "MoveGeneration.h"
#include "Search.h"

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
	const auto bm = Search::bestMove();

	bm.print();
}