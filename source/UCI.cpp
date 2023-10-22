#include "UCI.h"
#include "MoveItem.h"
#include "MoveGeneration.h"
#include "Search.h"
#include "SearchBenchmark.h"
#include "Zobrist.h"
#include "Evaluation.h"
#include <iostream>
#include <string>

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
inline std::string introduce() {
	static constexpr const char* engine_name = "id name Austerlitz@";
	static constexpr const char* author = "id author Simon B.";

	std::stringstream strm;
	strm << engine_name << '\n'
		<< author << "\nuciok\n";
	return strm.str();
}

// serve "go" command
void parseGo(std::istringstream& strm) {
	std::string com;
	int depth;

	strm >> std::skipws >> com;

	if (com == "depth") {
		strm >> std::skipws >> depth;
		time_data.is_time = false, time_data.stop = false;
		Search::bestMove(depth);
	}
	else if (com == "wtime") {
		static int wtime, btime, winc, binc;
		strm >> std::skipws >> wtime
			>> std::skipws >> com >> std::skipws >> btime
			>> std::skipws >> com >> std::skipws >> winc
			>> std::skipws >> com >> std::skipws >> binc;

		time_data.is_time = true, time_data.stop = false;
		time_data.left = game_state.turn ? btime : wtime;
		time_data.inc = game_state.turn ? binc : winc;

		// time for single move
		time_data.this_move = (time_data.left / 42) + (time_data.inc / 2);

		if (time_data.this_move >= time_data.left)
			time_data.this_move = time_data.left / 12;

		if (time_data.this_move < 0_ms)
			time_data.this_move = 5_ms;

		Search::bestMove(Search::max_depth);
	}
	else if (com == "movetime") {
		static int mtime;
		strm >> std::skipws >> mtime;

		time_data.is_time = true, time_data.stop = false;
		time_data.this_move = mtime;

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

		if (token == "isready") OS << "readyok\n";
		else if (token == "position") parsePosition(strm);
		else if (token == "ucinewgame") { tt.clear(), rep_tt.clear(), BBs.parseFEN(BitBoardsSet::start_pos); }
		else if (token == "uci") OS << introduce();
		else if (token == "print") BBs.printBoard();
		else if (token == "go") parseGo(strm);
		else if (token == "benchmark") bench.start();
#if defined(__DEBUG__)
		else if (token == "hashkey") OS << hash.key << '\n';
		else if (token == "eval")    OS << Eval::evaluate(Search::low_bound, Search::high_bound) << '\n';
		else if (token == "see") { 
			std::string sq_str;
			strm >> std::skipws >> sq_str;
			int sq = (sq_str[1] - '1') * 8 + (sq_str[0] - 'a');
			OS << Order::see(sq) << '\n';
		}
#endif
	} while (line != "quit" and argc == 1);
}