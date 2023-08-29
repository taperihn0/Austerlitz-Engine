#pragma once

#include "BitBoard.h"

class castleRights {
public:
	castleRights() = default;

	enum class cSide {
		wQUEEN,
		wROOK,
		bQUEEN,
		bROOK,
	};

	void operator=(uint8_t d) {
		decoded = d;
	}

	void operator|=(int s) {
		decoded |= s;
	}

	auto operator&(int s) {
		return decoded & s;
	}

	template <cSide SIDE>
	bool checkLegalCastle() {
		return decoded & (1 << static_cast<int>(SIDE));
	}
private:

	/*
	 * decoding castle rights:
	 * KQkq in FEN Notation
	 * K stands for 4th bit in decoded variable, Q stands for 3th bit
	 * k stands for 2nd bit and q stands for 1st bit
	 * decoded variable bits:	0	 0	  0	    0
	 * FEN Notation:			K	 Q	  k     q
	 */
	 
	uint8_t decoded;
};


inline constexpr castleRights::cSide operator&(enumSide SIDE, enumPiece PC) {
	return (SIDE == WHITE) ? (PC == ROOK ? castleRights::cSide::wROOK : castleRights::cSide::wQUEEN)
		: (PC == ROOK ? castleRights::cSide::bROOK : castleRights::cSide::bQUEEN);
}


// game state variables
struct gState {
	int ep_sq;
	enumSide turn;
	castleRights castle;
	int halfmove, fullmove;
};

extern gState game_state;


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
enumPiece_bbs bbsIndex();


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

	void clear();
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