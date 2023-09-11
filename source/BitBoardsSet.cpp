#include "BitBoardsSet.h"


std::array<char, 12> BitBoardsSet::piece_char = {
	'P', 'p', 'N', 'n', 'B', 'b', 'R', 'r', 'Q', 'q', 'K', 'k'
};

BitBoardsSet::BitBoardsSet(const BitBoardsSet& cbbs) noexcept(nothrow_copy_assign) {
	bbs = cbbs.bbs;
}

BitBoardsSet::BitBoardsSet(const std::string& _fen) {
	clear();
	parseFEN(_fen);
}

void BitBoardsSet::parseFEN(const std::string& _fen) {
	clear();

	fen = _fen;
	int x = 0, y = 7;

	for (int i = 0; i < size(fen); i++) {
		const char c = fen[i];

		if (isdigit(c)) {
			x += c - '0';
			continue;
		}
		else if (c == ' ') {
			parseGState(i + 1);
			break;
		}

		int in = y * 8 + x;
		switch (c) {
		case 'p':
			setBit(bbs[nBlackPawn], in);
			setBit(bbs[nBlack], in);
			setBit(bbs[nOccupied], in);
			break;
		case 'n':
			setBit(bbs[nBlackKnight], in);
			setBit(bbs[nBlack], in);
			setBit(bbs[nOccupied], in);
			break;
		case 'b':
			setBit(bbs[nBlackBishop], in);
			setBit(bbs[nBlack], in);
			setBit(bbs[nOccupied], in);
			break;
		case 'r':
			setBit(bbs[nBlackRook], in);
			setBit(bbs[nBlack], in);
			setBit(bbs[nOccupied], in);
			break;
		case 'q':
			setBit(bbs[nBlackQueen], in);
			setBit(bbs[nBlack], in);
			setBit(bbs[nOccupied], in);
			break;
		case 'k':
			setBit(bbs[nBlackKing], in);
			setBit(bbs[nBlack], in);
			setBit(bbs[nOccupied], in);
			break;
		case 'P':
			setBit(bbs[nWhitePawn], in);
			setBit(bbs[nWhite], in);
			setBit(bbs[nOccupied], in);
			break;
		case 'N':
			setBit(bbs[nWhiteKnight], in);
			setBit(bbs[nWhite], in);
			setBit(bbs[nOccupied], in);
			break;
		case 'B':
			setBit(bbs[nWhiteBishop], in);
			setBit(bbs[nWhite], in);
			setBit(bbs[nOccupied], in);
			break;
		case 'R':
			setBit(bbs[nWhiteRook], in);
			setBit(bbs[nWhite], in);
			setBit(bbs[nOccupied], in);
			break;
		case 'Q':
			setBit(bbs[nWhiteQueen], in);
			setBit(bbs[nWhite], in);
			setBit(bbs[nOccupied], in);
			break;
		case 'K':
			setBit(bbs[nWhiteKing], in);
			setBit(bbs[nWhite], in);
			setBit(bbs[nOccupied], in);
			break;
		case '/':
			y--, x = 0;
			continue;
		default: return;
		}

		++x;
	}

	bbs[nEmpty] = ~bbs[nOccupied];
}

void BitBoardsSet::parseGState(int i) {
	switch (fen[i]) {
	case 'w':
		game_state.turn = WHITE;
		break;
	case 'b':
		game_state.turn = BLACK;
		break;
	default: break;
	}

	i += 2;
	game_state.castle = 0;
	for (; fen[i] != ' '; i++) {
		switch (fen[i]) {
		case 'K':
			game_state.castle |= (1 << 3);
			break;
		case 'Q':
			game_state.castle |= (1 << 2);
			break;
		case 'k':
			game_state.castle |= (1 << 1);
			break;
		case 'q':
			game_state.castle |= 1;
			break;
		default:
			break;
		}
	}

	game_state.ep_sq = -1;
	if (fen[++i] != '-') {
		game_state.ep_sq = (fen[i] - 'a') + (fen[i + 1] - '1') * 8;
	} 

	i += 2;
	
	game_state.halfmove = 0;
	for (; fen[i] != ' '; i++) {
		game_state.halfmove *= 10;
		game_state.halfmove += fen[i] - '0';
	}
	
	i++;
	game_state.fullmove = 0;
	for (; i < size(fen) and fen[i] != ' '; i++) {
		game_state.fullmove *= 10;
		game_state.fullmove += fen[i] - '0';
	}
}

void BitBoardsSet::printBoard() {
	int sq_piece;

	std::string frame = "\t+";
	for (int r = 0; r < 8; r++) {
		frame += "---+";
	}

	for (int i = 0; i < 8; i++) {
		std::cout << '\n' << frame << "\n\t|";

		for (int j = 0; j < 8; j++) {
			sq_piece = -1;

			for (int piece = nWhitePawn; piece <= nBlackKing; piece++) {
				if (getBit(bbs[piece], (7 - i) * 8 + j)) {
					sq_piece = piece;
					break;
				}
			}

			std::cout << ' ' << (sq_piece != -1 ? piece_char[sq_piece] : ' ') << " |";
		}

		std::cout << ' ' << (8 - i);
	}

	std::cout << std::endl << frame << std::endl
		<< "\t  a   b   c   d   e   f   g   h" << std::endl << std::endl
		<< "  FEN Notation: " << fen << std::endl
		<< "  Castling rights: ";

	for (int i = 3; i >= 0; i--) {
		std::cout << ((game_state.castle & (1 << i)) >> i);
	}

	std::cout << std::endl << std::endl;
}