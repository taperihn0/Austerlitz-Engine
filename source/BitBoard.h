#pragma once

// system headers
#include <cstdint>
#include <iostream>
#include <array>

// define bitboard data type 
using U64 = uint64_t;
using bit = bool;

// custom U64 bitboard of given intiger
template <typename T, typename =
	std::enable_if_t<std::is_enum<T>::value or std::is_integral_v<T>>>
inline constexpr U64 cU64(T&& s) {
	return static_cast<U64>(std::forward<T>(s));
}

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

constexpr std::array<const char*, 64> index_to_square = {
	"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
	"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
	"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
	"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
	"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
	"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
	"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
	"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
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

} // namespace Constans


namespace {

	// empty U64 bitboard pre-compile constans
	constexpr U64 eU64 = cU64(0);

	// pre-compile board size
	constexpr int bSIZE = 8 * 8;

	// LS1B index purpose
	constexpr std::array<int, 64> index64 = {
		0, 47,  1, 56, 48, 27,  2, 60,
		57, 49, 41, 37, 28, 16,  3, 61,
		54, 58, 35, 52, 50, 42, 21, 44,
		38, 32, 29, 23, 17, 11,  4, 62,
		46, 55, 26, 59, 40, 36, 15, 53,
		34, 51, 20, 43, 31, 22, 10, 45,
		25, 39, 14, 33, 19, 30,  9, 24,
		13, 18,  8, 12,  7,  6,  5, 63
	};
	constexpr U64 debruijn64 = cU64(0x03f79d71b4cb0a89);

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
		int c;
		for (c = 0; x; x &= x - 1, c++);
		return c;
	}

} // namespace


inline int getLS1BIndex(U64 bb) {
	return (bb != 0) ? index64[((bb ^ (bb - 1)) * debruijn64) >> 58] : -1;
}

inline constexpr void popBit(U64& bb, int shift) {
	bb &= ~(cU64(1) << shift);
}

inline constexpr void setBit(U64& bb, int shift) {
	bb |= (cU64(1) << shift);
}

inline constexpr U64 getBit(U64 bb, int shift) {
	return bb & (cU64(1) << shift);
}