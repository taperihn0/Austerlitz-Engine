﻿#include "BitBoardsSet.h"
#include "AttackTables.h"
#include "UCI.h"
#include "MagicBitBoards.h"
#include "Search.h"
#include "MoveOrder.h"
#include "SearchBenchmark.h"
#include "Zobrist.h"
#include "Evaluation.h"

// global variables storing universal piece data
Zobrist hash;

BitBoardsSet BBs(BitBoardsSet::start_pos);

CSinglePieceAttacks<PAWN> cpawn_attacks;
CSinglePieceAttacks<KNIGHT> cknight_attacks;
CSinglePieceAttacks<KING> cking_attacks;

mData mdata;
gState game_state;

Search::Results Search::search_results;
Order::killerLookUp Order::killer;
Order::historyLookUp Order::history_moves;
Order::butterflyLookUp Order::butterfly, Order::countermove;

UCI UCI_o;
SearchBenchmark bench;

PV::lenghtPV PV::pv_len;
PV::linePV PV::pv_line;
MoveItem::iMove Search::prev_move;

TranspositionTable tt;
RepetitionTable rep_tt;

Time time_data;

int main(int argc, char* argv[]) {
	InitState::initMAttacksTables();
	tt.clear();
	UCI_o.goLoop(argc, argv);
}