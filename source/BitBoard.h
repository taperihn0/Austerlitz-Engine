#pragma once

// system headers
#include <cstdint>
#include <iostream>
#include <array>

// define bitboard data type 
using U64 = uint64_t;
using bit = bool;

template <typename T, typename =
	std::enable_if_t<std::is_enum<T>::value or std::is_constructible_v<T, U64>>>
inline constexpr U64 cU64(T&& s) {
	return static_cast<U64>(std::forward<T>(s));
}

constexpr U64 eU64 = cU64(0);

// define board size
constexpr int8_t BWIDTH = 8, BHEIGHT = 8, BSIZE = 8 * 8;

// enum indexes for little endian rank-file mapping
enum enumSquare {
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8
};

// enum sides color
enum enumSide { WHITE, BLACK };

enum enumPiece {
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING
};

// little endian rank-file mapping constants
namespace Constans {

	// usefull constans for bits-oriented board
	constexpr U64
		a_file = 0x0101010101010101,
		h_file = 0x8080808080808080,
		not_a_file = ~a_file,
		not_h_file = ~h_file,
		not_ab_file = 0xfcfcfcfcfcfcfcfc,
		not_gh_file = 0x3f3f3f3f3f3f3f3f,
		r1_rank = 0x00000000000000FF,
		r8_rank = 0xFF00000000000000,
		a1h8_diagonal = 0x8040201008040201,
		h1_a8_diagonal = 0x0102040810204080,
		lsquares = 0x55AA55AA55AA55AA,
		dsquares = 0xAA55AA55AA55AA55;
}

enum enumPiece_bb {
	nWhite,
	nBlack,
	nEmpty,
	nOccupied,

	nWhitePawn,
	nWhiteKnight,
	nWhiteBishop,
	nWhiteRook,
	nWhiteQueen,
	nWhiteKing,

	nBlackPawn,
	nBlackKnight,
	nBlackBishop,
	nBlackRook,
	nBlackQueen,
	nBlackKing
};

// bitboards set
class BitBoardsSet {
public:
	BitBoardsSet() = default;

	U64 operator[](enumPiece_bb piece_get) {
		return bitboards[static_cast<size_t>(piece_get)];
	}
private:
	std::array<U64, 16> bitboards;
};

// display single bitboard function
void printBitBoard(U64 bitboard);

inline constexpr void setBit(U64& bb, enumSquare shift) {
	bb |= (cU64(1) << shift);
}

template <typename T, typename = 
	std::enable_if_t<std::is_constructible_v<T, int>>>
inline constexpr void setBit(U64& bb, T shift) {
	bb |= (cU64(1) << std::forward<T>(shift));
}