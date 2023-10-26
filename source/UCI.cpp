#include "UCI.h"
#include "MoveItem.h"
#include "MoveGeneration.h"
#include "Search.h"
#include "SearchBenchmark.h"
#include "Zobrist.h"
#include "Evaluation.h"
#include "MoveOrder.h"
#include <iostream>
#include <string>

// do not check fen position appended moves legality -
// some extra performance gain where guaranted to get legal moves
#if defined(__DEBUG__)
#define _CHECK_MOVE_LEGAL true
#else
#define _CHECK_MOVE_LEGAL false
#endif


UCI::UCI() 
	: os(&std::cout), is(&std::cin) {
	std::ios_base::sync_with_stdio(false);
}

// return introducing string
inline void introduce() {
	OS << UCI::engine_name << '\n'
		<< UCI::author << "\nuciok\n";
}

// serve "go" command
void parseGo(std::istringstream& strm) {
	std::string com;
	int depth;

	strm >> std::skipws >> com;

	if (com == "depth") {
		strm >> std::skipws >> depth;
		Search::time_data.setFixedTime(0);
		Search::bestMove(depth);
	}
	else if (com == "wtime") {
		static int wtime, btime, winc, binc;
		strm >> std::skipws >> wtime
			>> std::skipws >> com >> std::skipws >> btime
			>> std::skipws >> com >> std::skipws >> winc
			>> std::skipws >> com >> std::skipws >> binc;

		Search::time_data.calcMoveTime(game_state.turn ? btime : wtime, game_state.turn ? binc : winc);
		Search::bestMove(Search::max_depth);
	}
	else if (com == "movetime") {
		static int mtime;
		strm >> std::skipws >> mtime;

		Search::time_data.setFixedTime(mtime);
		Search::bestMove(Search::max_depth);
	}
	else if (com == "perft") {
		strm >> std::skipws >> depth;
		MoveGenerator::Analisis::perftDriver(depth);
		return;
	}
}

// parse given position and perform moves
void UCI::parsePosition(std::istringstream& strm) {
	std::string com, move;

	strm >> std::skipws >> com;

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

	// scan given moves and perform them on real board
	while (strm >> std::skipws >> move) {
#if _CHECK_MOVE_LEGAL
		casted.constructMove(move);
		bool illegal = true;

		// check move validity
		for (const auto& move : MoveGenerator::generateLegalMoves<MoveGenerator::LEGAL>()) {
			if (casted == move) {
				MovePerform::makeMove(move);
				illegal = false;
				rep_tt.posRegister();
				break;
			}
		}

		if (illegal) {
			OS << "illegal move '" << move << "'\n";
			return;
		}
#else
		casted.constructMove(move);
		MovePerform::makeMove(casted);
		rep_tt.posRegister();
#endif
	}
}

// prepare for new game
inline void newGame() {
	tt.clear();
	rep_tt.clear();
	BBs.parseFEN(BitBoardsSet::start_pos);
}


#if defined(__DEBUG__)
void seePrint(std::istringstream& strm) {
	std::string sq_str;
	strm >> std::skipws >> sq_str;
	int sq = (sq_str[1] - '1') * 8 + (sq_str[0] - 'a');
	OS << Order::see(sq) << '\n';
}
#endif

// main UCI loop
void UCI::goLoop(int argc, char* argv[]) {
	std::string line, token;

	for (int i = 1; i < argc; i++)
		line += std::string(argv[i]) + " ";

	if (is == &std::cin)
		OS << "Polish Chess Engine: Austerlitz@ by Simon B.\n";

	do {
		if (argc == 1 and !std::getline(IS, line))
			line = "quit";

		std::istringstream strm(line);

		strm >> std::skipws >> token;

		if (token == "isready")         OS << "readyok\n";
		else if (token == "position")   parsePosition(strm);
		else if (token == "ucinewgame") newGame();
		else if (token == "uci")        introduce();
		else if (token == "print")      BBs.printBoard();
		else if (token == "go")         parseGo(strm);
		else if (token == "benchmark")  bench.start();
#if defined(__DEBUG__)
		else if (token == "hashkey") OS << hash.key << '\n';
		else if (token == "eval")    OS << Eval::evaluate(Search::low_bound, Search::high_bound) << '\n';
		else if (token == "see")     seePrint(strm);
#endif
	} while (line != "quit" and argc == 1);
}