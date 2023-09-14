#include "UCI.h"
#include "MoveItem.h"
#include "MoveGeneration.h"
#include "Search.h"
#include <iostream>
#include <string>


namespace UCI {

	// construct a move based on a normal string notation
	MoveItem::iMove constructMove(std::string move, bool side) {
		int origin, target;
		char promo = '\0';

		origin = (move[1] - '1') * 8 + (move[0] - 'a');
		target = (move[3] - '1') * 8 + (move[2] - 'a');

		if (move.size() == 5) {
			promo = move.back();
		}

		return side ? MoveItem::toMove<BLACK>(target, origin, promo)
			: MoveItem::toMove<WHITE>(target, origin, promo);
	}

	// return introducing string
	inline std::string introduce() {
		static constexpr const char* engine_name = "id name Austerlitz@";
		static constexpr const char* author = "id author Szymon BSdie";

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
			const auto move = Search::bestMove(depth);
			TOGUI_S << "bestmove ";
			move.print() << '\n';
		}
		else if (com == "perft") {
			strm >> std::skipws >> depth;
			MoveGenerator::Analisis::perftDriver(depth);
			return;
		}
		
		const auto move = Search::bestMove(6);
		TOGUI_S << "bestmove ";
		move.print() << '\n';
	}

	// parse given position and perform moves
	void parsePosition(std::istringstream& strm) {
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

			// load additional board states (en passant, turn color)
			for (int i = 0; i < 5; i++) {
				strm >> std::skipws >> states;
				fen += ' ' + states;
			}

			BBs.parseFEN(fen);
		}
		else if (com != "current") {
			TOGUI_S << "position [fen | startpos | current] moves[optionally] ...\n";
			return;
		}

		strm >> std::skipws >> com;
		if (com != "moves")
			return;

		// scan given moves and perform them on real board
		while (strm >> std::skipws >> move) {
			casted = constructMove(move, game_state.turn);
			bool illegal = true;

			// check move validity
			for (const auto& move : MoveGenerator::generateLegalMoves()) {
				if (casted == move) {
					MovePerform::makeMove(move);
					illegal = false;
					break;
				}
			}

			if (illegal) {
				TOGUI_S << "illegal move '" << move << "'\nAll previous moves was performed\n";
				return;
			}
		}
	}

	// main UCI loop
	void goLoop(int argc, char* argv[]) {
		std::ios_base::sync_with_stdio(false);
		std::string line, token;

		for (int i = 1; i < argc; i++)
			line += std::string(argv[i]) + " ";

		TOGUI_S << "Polish Chess Engine: Austerlitz by Simon BSdie\n";

		do {
			if (argc == 1 and !std::getline(FROMGUI_S, line))
				line = "quit";

			std::istringstream strm(line);

			strm >> std::skipws >> token;

			if (token == "isready") TOGUI_S << "readyok\n";
			else if (token == "position") parsePosition(strm);
			else if (token == "ucinewgame") BBs.parseFEN(BitBoardsSet::start_pos);
			else if (token == "uci") TOGUI_S << introduce();
			else if (token == "print") BBs.printBoard();
			else if (token == "go") parseGo(strm);
		} while (line != "quit" and argc == 1);
	}

}

#undef OPTIMIZE