#include "BitBoardsSet.h"
#include "AttackTables.h"
#include "UCI.h"
#include "MagicBitBoards.h"
#include "Search.h"
#include "MoveOrder.h"
#include "SearchBenchmark.h"
#include "Zobrist.h"
#include "Evaluation.h"

Zobrist hash;
BitBoardsSet BBs(BitBoardsSet::start_pos);

mData mdata;
gState game_state;

UCI UCI_o;
SearchBenchmark bench;

TranspositionTable tt;
RepetitionTable rep_tt;

mSearch m_search;

int main(int argc, char* argv[]) {
	InitState::initMAttacksTables();
	UCI_o.goLoop(argc, argv);
}