#include "BitBoardsSet.h"


template <enumPiece PC>
enumPiece_bbs bbsIndex() {
	return nEmpty;
}

template <>
enumPiece_bbs bbsIndex<KNIGHT>() {
	return nWhiteKnight;
}

template <>
enumPiece_bbs bbsIndex<BISHOP>() {
	return nWhiteBishop;
}

template <>
enumPiece_bbs bbsIndex<ROOK>() {
	return nWhiteRook;
}

template <>
enumPiece_bbs bbsIndex<QUEEN>() {
	return nWhiteQueen;
}

// explicit template's instantiation definition
template enumPiece_bbs bbsIndex<KNIGHT>();
template enumPiece_bbs bbsIndex<ROOK>();
template enumPiece_bbs bbsIndex<BISHOP>();
template enumPiece_bbs bbsIndex<QUEEN>();

BitBoardsSet::BitBoardsSet()
	: piece_char{'P', 'p', 'N', 'n', 'B', 'b', 'R', 'r', 'Q', 'q', 'K', 'k'} {
	bbs.fill(eU64);
}

BitBoardsSet::BitBoardsSet(const std::string& _fen)
	: BitBoardsSet() {
	bbs.fill(eU64);
	parseFEN(_fen);
}

void BitBoardsSet::parseFEN(const std::string& _fen) {
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
		gState::turn = WHITE;
		break;
	case 'b':
		gState::turn = BLACK;
		break;
	default: break;
	}

	i += 2;
	gState::castle = 0;
	for (; fen[i] != ' '; i++) {
		switch (fen[i]) {
		case 'K':
			gState::castle |= (1 << 3);
			break;
		case 'Q':
			gState::castle |= (1 << 2);
			break;
		case 'k':
			gState::castle |= (1 << 1);
			break;
		case 'q':
			gState::castle |= 1;
			break;
		default:
			break;
		}
	}

	gState::ep_sq = -1;
	if (fen[++i] != '-') {
		gState::ep_sq = (fen[i] - 'a') * 8 + (fen[++i] - '1');
	} 

	i += 2;
	
	gState::halfmove = 0;
	for (; fen[i] != ' '; i++) {
		gState::halfmove *= 10;
		gState::halfmove += fen[i] - '0';
	}
	
	i++;
	gState::fullmove = 0;
	for (; i < size(fen) and fen[i] != ' '; i++) {
		gState::fullmove *= 10;
		gState::fullmove += fen[i] - '0';
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
		std::cout << ((gState::castle & (1 << i)) >> i);
	}

	std::cout << std::endl;
}