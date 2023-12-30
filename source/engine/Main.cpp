#include "BitBoardsSet.h"
#include "AttackTables.h"
#include "UCI.h"
#include "MagicBitBoards.h"
#include "Search.h"
#include "MoveOrder.h"
#include "SearchBenchmark.h"
#include "Zobrist.h"
#include "Evaluation.h"
#include "../tuning/TexelTuning.h"

Zobrist hash;
BitBoardsSet BBs(BitBoardsSet::start_pos);

mData mdata;
gState game_state;

UCI UCI_o;
SearchBenchmark bench;

TranspositionTable tt;
RepetitionTable rep_tt;

mSearch m_search;

Eval::Value::EvaluationParameters Eval::params;

#if COLLECT_POSITION_DATA
tGameCollector game_collector;
#endif

#if ENABLE_TUNING
tTuning tuning;
#endif

int main(int argc, const char* argv[]) {
#if COLLECT_POSITION_DATA
	assert(game_collector.openFile());
#endif

#if ENABLE_TUNING
	tuning.initTuningData();
	assert(tuning.openSessionFile());
#endif

	InitState::initMAttacksTables();
	UCI_o.goLoop(argc, argv);
}