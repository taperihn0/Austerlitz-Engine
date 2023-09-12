#include "MoveItem.h"
#include "MoveGeneration.h"
#include <iostream>
#include <string>
#include <sstream>


#define TOGUI_S std::cout
#define FROMGUI_S std::cin

#define OPTIMIZE() \
		std::ios::sync_with_stdio(false); \
		TOGUI_S.tie(NULL); \
		FROMGUI_S.tie(NULL); \


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
		static constexpr const char* engine_name = "id name Austerlitz*";
		static constexpr const char* author = "id author Szymon BS";

		std::stringstream strm;
		strm << engine_name << '\n'
			<< author << "\nuciok\n";
		return strm.str();
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

		strm >> std::skipws >> com;
		if (com != "moves")
			return;

		// scan given moves and perform them on real board
		while (strm >> std::skipws >> move) {
			casted = constructMove(move, game_state.turn);

			// check move validity
			for (const auto& move : MoveGenerator::generateLegalMoves()) {
				if (casted == move) {
					MovePerform::makeMove(move);
					break;
				}
			}
		}
	}


	void goLoop() {
		// optimization flags
		OPTIMIZE();
		std::string line, token;

		// introduce yourself
		TOGUI_S << introduce();

		// main UCI loop
		while (true) {
			if (!std::getline(FROMGUI_S, line))
				break;

			std::istringstream strm(line);

			strm >> std::skipws >> token;

			if (token == "isready") TOGUI_S << "readyok\n";
			else if (token == "position") parsePosition(strm);
			else if (token == "ucinewgame") BBs.parseFEN(BitBoardsSet::start_pos);
			else if (token == "uci") TOGUI_S << introduce();
			else if (token == "print") BBs.printBoard();
			else if (token == "quit") break;
		}
	}

}

#undef TOGUI_S
#undef FROMGUI_S
#undef OPTIMIZE