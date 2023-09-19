#include "UCI.h"
#include "MoveItem.h"
#include "MoveGeneration.h"
#include "Search.h"
#include "SearchBenchmark.h"
#include <iostream>
#include <string>


UCI::UCI() 
	: os(&std::cout), is(&std::cin) {
	std::ios_base::sync_with_stdio(false);
}


// construct a move based on a normal string notation
MoveItem::iMove constructMove(std::string move) {
	int origin, target;
	char promo = '\0';

	origin = (move[1] - '1') * 8 + (move[0] - 'a');
	target = (move[3] - '1') * 8 + (move[2] - 'a');

	if (move.size() == 5) {
		promo = move.back();
	}

	return game_state.turn ? MoveItem::toMove<BLACK>(target, origin, promo)
		: MoveItem::toMove<WHITE>(target, origin, promo);
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
	int depth = 6;

	strm >> std::skipws >> com;

	if (com == "depth") {
		strm >> std::skipws >> depth;
	}
	else if (com == "perft") {
		strm >> std::skipws >> depth;
		MoveGenerator::Analisis::perftDriver(depth);
		return;
	}

	const auto move = Search::bestMove(depth);
	*UCI_o.os << "info score cp " << Search::search_results.score << " depth " << depth 
		<< " nodes " << Search::search_results.nodes << "\nbestmove ";
	move.print() << '\n';
}

// parse given position and perform moves
void UCI::parsePosition(std::istringstream& strm) {
	std::string com, move;
	MoveItem::iMove casted;

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
		*os << "position [fen | startpos | current] moves[optionally] ...\n";
		return;
	}

	strm >> std::skipws >> com;
	if (com != "moves")
		return;

	// scan given moves and perform them on real board
	while (strm >> std::skipws >> move) {
		casted = constructMove(move);
		bool illegal = true;

		// check move validity
		for (const auto& move : MoveGenerator::generateLegalMoves<MoveGenerator::LEGAL>()) {
			if (casted == move) {
				MovePerform::makeMove(move);
				illegal = false;
				break;
			}
		}

		if (illegal) {
			*os << "illegal move '" << move << "'\n";
			return;
		}
	}
}

// main UCI loop
void UCI::goLoop(int argc, char* argv[]) {
	std::string line, token;

	for (int i = 1; i < argc; i++)
		line += std::string(argv[i]) + " ";

	if (is == &std::cin)
		*os << "Polish Chess Engine: Austerlitz@ by Simon B.\n";

	do {
		if (argc == 1 and !std::getline(*is, line))
			line = "quit";

		std::istringstream strm(line);

		strm >> std::skipws >> token;

		if (token == "isready") *os << "readyok\n";
		else if (token == "position") parsePosition(strm);
		else if (token == "ucinewgame") BBs.parseFEN(BitBoardsSet::start_pos);
		else if (token == "uci") *os << introduce();
		else if (token == "print") BBs.printBoard();
		else if (token == "go") parseGo(strm);
		else if (token == "benchmark") bench.start();
#if defined(__DEBUG__)
		else if (token == "killer_reset") Search::killerReset();
#endif
	} while (line != "quit" and argc == 1);
}

#undef OPTIMIZE