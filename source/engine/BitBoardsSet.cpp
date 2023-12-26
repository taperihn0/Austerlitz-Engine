#include "BitBoardsSet.h"
#include "Zobrist.h"
#include "Search.h"
#include "Evaluation.h"
#include "UCI.h"


BitBoardsSet::BitBoardsSet(const BitBoardsSet& cbbs) noexcept(nothrow_copy_assign) 
: bbs(cbbs.bbs) {}

BitBoardsSet::BitBoardsSet(const std::string& fen) {
	clear();
	parseFEN(fen);
}

void BitBoardsSet::parseFEN(const std::string& fen) {
	clear();

	static constexpr std::array<size_t, 12> bbs_pc = {
		nWhitePawn, nWhiteKnight, nWhiteBishop, nWhiteRook, nWhiteQueen, nWhiteKing, 
		nBlackPawn, nBlackKnight, nBlackBishop, nBlackRook, nBlackQueen, nBlackKing
	};
	static constexpr std::string_view pc_str = "PNBRQKpnbrqk";

	int x = 0, y = 7;
	game_state.material = { 0, 0 };

	for (int i = 0; i < size(fen); i++) {
		const char c = fen[i];

		if (isdigit(c)) {
			x += c - '0';
			continue;
		}
		else if (c == ' ') {
			parseGState(fen, i + 1);
			break;
		}

		int in = y * 8 + x;
		if (c == '/') {
			y--, x = 0;
			continue;
		}

		const auto pc = pc_str.find_first_of(c);
		const bool side = islower(c);
		setBit(bbs[side ? nBlack :nWhite], in);
		setBit(bbs[nOccupied], in);
		setBit(bbs[bbs_pc[pc]], in);
		game_state.material[side] += Eval::params.piece_material[toPieceType(bbs_pc[pc])];
		++x;
	}

	bbs[nEmpty] = ~bbs[nOccupied];
	hash.generateKey();
}

void BitBoardsSet::parseGState(const std::string& fen, int i) {
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
		OS << '\n' << frame << "\n\t|";

		for (int j = 0; j < 8; j++) {
			sq_piece = -1;

			for (int piece = nWhitePawn; piece <= nBlackKing; piece++) {
				if (getBit(bbs[piece], (7 - i) * 8 + j)) {
					sq_piece = piece;
					break;
				}
			}

			OS << ' ' << (sq_piece != -1 ? piece_ascii[sq_piece] : ' ') << " |";
		}

		OS << ' ' << (8 - i);
	}

	OS << std::endl << frame << std::endl
		<< "\t  a   b   c   d   e   f   g   h" << std::endl << std::endl
		<< "  FEN: " << getFEN() << std::endl
		<< "  Castling rights: ";

	for (int i = 3; i >= 0; i--) {
		OS << ((game_state.castle & (1 << i)) >> i);
	}

	OS << std::endl << std::endl;
}

std::string BitBoardsSet::getFEN() {
	std::string fen;
	int distance, sq_piece;

	for (int i = 0; i < 8; i++) {
		distance = 0;

		for (int j = 0; j < 8; j++) {
			sq_piece = -1;
			
			for (int piece = nWhitePawn; piece <= nBlackKing; piece++) {
				if (getBit(bbs[piece], (7 - i) * 8 + j)) {
					sq_piece = piece;
					break;
				}
			}

			if (sq_piece == -1) {
				distance++;
				continue;
			}

			if (distance) {
				fen += '0' + distance;
				distance = 0;
			}

			fen += piece_ascii[sq_piece];
		}

		if (distance) fen += '0' + distance;
		fen += '/';
	}

	fen.pop_back();

	return fen + ' ' + getFEN_States();
}

std::string BitBoardsSet::getFEN_States() {
	std::string states;

	// load current states manually
	states += "wb"[game_state.turn];
	states += ' ';

	if (!game_state.castle.raw())
		states += '-';

	for (int i = 3; i >= 0; i--) {
		if (game_state.castle & (1 << i)) {
			states += "qkQK"[i];
		}
	}

	states += ' ';
	states += game_state.ep_sq == -1 ? "-" : index_to_square[game_state.ep_sq];
	states += ' ';
	states += std::to_string(game_state.halfmove);
	states += ' ';
	states += std::to_string(game_state.fullmove);

	return states;
}
