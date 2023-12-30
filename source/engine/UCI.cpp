#include "UCI.h"
#include "MoveItem.h"
#include "MoveGeneration.h"
#include "Search.h"
#include "SearchBenchmark.h"
#include "Zobrist.h"
#include "Evaluation.h"
#include "MoveOrder.h"
#include "../tuning/TexelTuning.h"
#include <iostream>
#include <string>


UCI::UCI() 
	: o_stream(&std::cout), i_stream(&std::cin) {
	std::ios_base::sync_with_stdio(false);
}

// return introducing string
inline void parseUCI() {
	OS << UCI::engine_name << '\n'
		<< UCI::author << '\n'
		<< TranspositionTable::hashInfo() << '\n'
		<< "uciok\n";
}

// serve "go" command
void parseGo(std::istringstream& strm) {
	std::string com;
	int depth;

	strm >> std::skipws >> com;

	if (com == "depth") {
		strm >> std::skipws >> depth;
		m_search.time_data.setFixedTime(0);
		m_search.bestMove(depth);
	}
	else if (com == "wtime") {
		static int wtime, btime, winc, binc;
		strm >> std::skipws >> wtime
			>> std::skipws >> com >> std::skipws >> btime
			>> std::skipws >> com >> std::skipws >> winc
			>> std::skipws >> com >> std::skipws >> binc;

		m_search.time_data.calcMoveTime(game_state.turn ? btime : wtime, game_state.turn ? binc : winc);
		m_search.bestMove(mSearch::max_depth);
	}
	else if (com == "movetime") {
		static int mtime;
		strm >> std::skipws >> mtime;

		m_search.time_data.setFixedTime(mtime);
		m_search.bestMove(mSearch::max_depth);
	}
	else if (com == "perft") {
		strm >> std::skipws >> depth;
		MoveGenerator::Analisis::perftDriver(depth);
		return;
	}
#if ENABLE_TUNING
	else if (com == "tuner")
		tuning.runWeightTuning();
#endif
}

// parse given position and perform moves
void UCI::parsePosition(std::istringstream& strm) {
	std::string com, move;

	strm >> std::skipws >> com;

	m_search.prev_move = MoveItem::iMove::no_move;

	// set starting or given fen
	if (com == "startpos") {
		BBs.parseFEN(BitBoardsSet::start_pos);
	}
	else if (com == "fen") {
		std::string states, fen;
		strm >> std::skipws >> fen;

		if (fen == "startpos") {
			BBs.parseFEN(BitBoardsSet::start_pos);
			return;
		}

		// load additional board states (en passant, turn color)
		for (int i = 0; i < 5; i++) {
			strm >> std::skipws >> states;
			fen += ' ' + states;
		}

		BBs.parseFEN(fen);
	}
	else if (com != "current") {
		OS << "position [fen | startpos | current] moves[optionally] ...\n";
		return;
	}

	strm >> std::skipws >> com;
	if (com != "moves")
		return;

	rep_tt.clear();

	MoveItem::iMove casted;
	MoveList ml;

	// scan given moves and perform them on real board
	while (strm >> std::skipws >> move) {
		casted.constructMove(move);
		bool illegal = true;

		// check move validity - generate all the legal moves and then 
		// compare given move with all legal moves in move list
		MoveGenerator::generateLegalMoves<MoveGenerator::LEGAL>(ml);

		for (const auto& move : ml) {
			if (casted == move) {
				MovePerform::makeMove(move);
				illegal = false;
				rep_tt.posRegister();
				m_search.prev_move = move;
				break;
			}
		}

		if (illegal) {
			OS << "illegal move '" << move << "'\n";
			return;
		}
	}
}

// prepare for new game
inline void newGame() {
#if COLLECT_POSITION_DATA
	game_collector.flushBufor();
#endif

	tt.clear();
	rep_tt.clear();
	m_search.move_order.clearCountermove();
	BBs.parseFEN(BitBoardsSet::start_pos);
}


void setOption(std::istringstream& strm) {
	std::string com;

	strm >> std::skipws >> com >> std::skipws >> com;

	if (com == "Hash") {
		strm >> std::skipws >> com >> std::skipws >> com;
		tt.setSize(std::stoi(com));
	}
}


#if defined(__DEBUG__)
void seePrint(std::istringstream& strm) {
	std::string sq_str;
	strm >> std::skipws >> sq_str;
	int sq = (sq_str[1] - '1') * 8 + (sq_str[0] - 'a');
	OS << mOrder::see(sq) << '\n';
}

void evalInfo() {
	OS << "white material: " << game_state.material[0] << '\n'
		<< "black material: " << game_state.material[1] << '\n'
		<< "pawn endgame: " << game_state.isPawnEndgame() << '\n'
		<< Eval::evaluate(mSearch::low_bound, mSearch::high_bound) << "\n\n";
}
#endif

#if ENABLE_TUNING
void parseTuning(std::istringstream& strm) {
	std::string opt;
	strm >> std::skipws >> opt;

	if (opt == "k") tuning.updateK();
	else if (isdigit(opt.back())) tuning.printIndexInfo(stoi(opt));
}
#endif

// main UCI loop
void UCI::goLoop(int argc, const char* argv[]) {
	std::string line, token;
	bool skip_getline = argc > 1;

	for (int i = 1; i < argc; i++)
		line += " " + std::string(argv[i]);

	if (i_stream == &std::cin)
		OS << introduction;

	do {
		if (!skip_getline and !std::getline(IS, line))
			line = "quit";

		std::istringstream strm(line);
		skip_getline = false;
		strm >> std::skipws >> token;

		if (token == "isready")         OS << "readyok\n";
		else if (token == "position")   parsePosition(strm);
		else if (token == "ucinewgame") newGame();
		else if (token == "uci")        parseUCI();
		else if (token == "go")         parseGo(strm);
		else if (token == "setoption")  setOption(strm);
		else if (token == "print")      BBs.printBoard();
		else if (token == "benchmark")  bench.start();
		else if (token == "hashinfo")   OS << tt.currSizeInfo() << '\n';
#if defined(__DEBUG__)
		else if (token == "hashkey")    OS << hash.key << '\n';
		else if (token == "eval")       evalInfo();
		else if (token == "see")        seePrint(strm);
#endif

#if ENABLE_TUNING
		else if (token == "tuner")      parseTuning(strm);
#endif
	} while (line != "quit");

#if COLLECT_POSITION_DATA
	game_collector.flushBufor();
#endif
}