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

static constexpr U64 eU64 = cU64(0);

// define board size
static constexpr int bSIZE = 8 * 8;

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

// general piece enum
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
		not_r1_rank = ~r1_rank,
		not_r8_rank = ~r8_rank,
		a1h8_diagonal = 0x8040201008040201,
		h1_a8_diagonal = 0x0102040810204080,
		lsquares = 0x55AA55AA55AA55AA,
		dsquares = 0xAA55AA55AA55AA55;
}

// piece enum for their bitboards
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
		return bitboards[piece_get];
	}
private:
	std::array<U64, 16> bitboards;
};

namespace {

	// display single bitboard function
	void printBitBoard(U64 bitboard) {
		int bitcheck = 56;

		std::cout << std::endl << "  ";

		for (int i = 0; i < 8; i++) {
			for (int j = 0; j < 8; j++) {
				std::cout << static_cast<bit>(bitboard & (cU64(1) << (bitcheck + j))) << ' ';

				if (j == 7) {
					std::cout << "  " << 8 - i << "\n  ";
				}
			}

			bitcheck -= 8;
		}

		std::cout << std::endl << "  a b c d e f g h" << std::endl << std::endl
			<< "bb value: " << bitboard << std::endl;
	}

	// Population count for magic bitboards 
	//  - counting bits set to one in a 64-bits number 'x'
	int bitCount(U64 x) {
		int count = 0;

		while (x) {
			count++;
			x &= x - 1;
		}

		return count;
	}

}

// get index of LS1B (least significant bit)
inline int getLS1B_Index(U64 bb) {
	return (bb == 0) ? -1 : bitCount((bb & ~bb) - 1);
}

// pop indexed bit
inline constexpr void popBit(U64& bb, int index) {
	bb ^= (bb & (cU64(1) << index))? (1 << index) : 0;
}

// set given bit
inline constexpr void setBit(U64& bb, int  shift) {
	bb |= (cU64(1) << shift);
}