#pragma once

#include "BitBoard.h"

// game state variables
namespace gState {
	namespace {
		int ep_sq;
		enumSide turn;
		int castle, halfmove, fullmove;
	}
}

// piece enum for their bitboards
enum enumPiece_bbs {
	nWhitePawn,
	nBlackPawn,

	nWhiteKnight,
	nBlackKnight,

	nWhiteBishop,
	nBlackBishop,

	nWhiteRook,
	nBlackRook,

	nWhiteQueen,
	nBlackQueen,

	nWhiteKing,
	nBlackKing,

	nWhite,
	nBlack,
	nEmpty,
	nOccupied,
};


// custom operators returning proper type of index of std::array
inline constexpr size_t operator-(enumPiece_bbs pc, enumSide s) {
	return static_cast<size_t>(pc) - static_cast<size_t>(s);
}

inline constexpr size_t operator+(enumPiece_bbs pc, enumSide s) {
	return static_cast<size_t>(pc) + static_cast<size_t>(s);
}


template <enumPiece PC>
enumPiece_bbs  bbsIndex();


class BitBoardsSet {
public:
	BitBoardsSet();
	BitBoardsSet(const std::string& _fen);

	// pieces bitboard initialization using FEN method
	void parseFEN(const std::string& fen);
	void parseGState(int i);

	U64& operator[](size_t piece_get) noexcept;

	const std::string& getFEN() noexcept;

	// display entire board of all pieces
	void printBoard();
private:
	// board display purpose
	std::array<char, 12> piece_char;

	// central storage of actual piece structure of every type
	std::array<U64, 16> bbs;
	std::string fen;
};

inline U64& BitBoardsSet::operator[](size_t piece_get) noexcept {
	return bbs[piece_get];
}

inline const std::string& BitBoardsSet::getFEN() noexcept {
	return fen;
}

// declare global BitBoardsSet containing piece structure data
extern BitBoardsSet BBs;