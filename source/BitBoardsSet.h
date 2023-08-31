#pragma once

#include "BitBoard.h"

class castleRights {
public:
	castleRights() = default;
	
	enum class cSide {
		bQUEEN,
		bROOK,
		wQUEEN,
		wROOK,
	};

	void operator=(const castleRights& cr);

	inline void operator=(uint8_t d) noexcept {
		decoded = d;
	}

	inline void operator|=(int s) noexcept {
		decoded |= s;
	}

	inline auto operator&(int s) noexcept {
		return decoded & s;
	}

	template <cSide SIDE>
	inline bool checkLegalCastle() noexcept {
		return decoded & (1 << static_cast<int>(SIDE));
	}

	inline uint32_t raw() {
		return decoded;
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

inline void castleRights::operator=(const castleRights& cr) {
	decoded = cr.decoded;
}

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
	// for each side
	std::array<bool, 2> rook_king_move_block;
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

template <enumPiece PC>
inline enumPiece_bbs bbsIndex() {
	return nEmpty;
}

template <>
inline enumPiece_bbs bbsIndex<KNIGHT>() {
	return nWhiteKnight;
}

template <>
inline enumPiece_bbs bbsIndex<BISHOP>() {
	return nWhiteBishop;
}

template <>
inline enumPiece_bbs bbsIndex<ROOK>() {
	return nWhiteRook;
}

template <>
inline enumPiece_bbs bbsIndex<QUEEN>() {
	return nWhiteQueen;
}


class BitBoardsSet {
public:
	BitBoardsSet() = default;
	BitBoardsSet(const BitBoardsSet& bbs) noexcept(nothrow_copy_assign);
	BitBoardsSet(const std::string& _fen);

	// pieces bitboard initialization using FEN method
	void parseFEN(const std::string& fen);
	void parseGState(int i);

	U64& operator[](size_t piece_get);
	void operator=(const BitBoardsSet& cbbs) noexcept(nothrow_copy_assign);

	const std::string& getFEN() noexcept(nothrow_str_cpy_constr);

	// display entire board of all pieces
	void printBoard();

	void clear();
private:
	// board display purpose
	static std::array<char, 12> piece_char;
	static constexpr bool nothrow_copy_assign = std::is_nothrow_copy_assignable_v<std::array<U64, 16>>,
		nothrow_str_cpy_constr = std::is_nothrow_copy_constructible_v<std::string>;

	// central storage of actual piece structure of every type
	std::array<U64, 16> bbs;
	std::string fen;
};

inline U64& BitBoardsSet::operator[](size_t piece_get) {
	return bbs[piece_get];
}

inline void BitBoardsSet::operator=(const BitBoardsSet& cbbs) noexcept(nothrow_copy_assign) {
	bbs = cbbs.bbs;
}

inline const std::string& BitBoardsSet::getFEN() noexcept(nothrow_str_cpy_constr) {
	return fen;
}

// declare global BitBoardsSet containing piece structure data
extern BitBoardsSet BBs;