#pragma once

// system headers
#include <cstdint>
#include <iostream>
#include <array>
#include <cassert>
#include <nmmintrin.h>
#include <intrin.h>
#include "staticLookup.h"
#include "UCI.h"

// define bitboard data type:
// U64 type is used for declaring 64-bit masks as bitboards,
// while ULL type is preferred when declaring object in a sense of a number, where bits aren't relevant
using U64 = uint64_t;
using ULL = unsigned long long;

// custom U64 bitboard of given integer
template <typename T, class =
	std::enable_if_t<std::is_enum_v<T> or std::is_constructible_v<U64, T>>>
inline constexpr auto cU64(T s) noexcept {
	return static_cast<U64>(s);
}


namespace BitU64LookUp {
	constexpr auto u64bit = cexpr::CexprArr<false, U64, 64>([](int i) {
		return cU64(1) << i;
	});
}

// set single bit on a bitboard and return such result
template <typename T, class =
	std::enable_if_t<std::is_enum<T>::value or std::is_integral_v<T>>>
inline constexpr auto bitU64(T s) noexcept {
	assert(s >= 0 and s < 64 && "Index overflow");
	return BitU64LookUp::u64bit.get(s);
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

// vertical flipped squares
constexpr auto ver_flip_square = cexpr::CexprArr<false, int, 64>([](int i) constexpr { return i ^ 56; });

// translate index to string square in little endian mapping
constexpr std::array<const char*, 64> index_to_square = {
	"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
	"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
	"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
	"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
	"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
	"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
	"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
	"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
};

// enum sides color
enum enumSide : bool { WHITE, BLACK };

// flip player's turn
inline constexpr enumSide operator!(enumSide s) noexcept {
	return static_cast<enumSide>(!static_cast<bool>(s));
}


// general piece enum
enum enumPiece {
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING,
	ANY
};

inline constexpr bool operator==(uint32_t iarg, enumPiece pc) noexcept {
	return iarg == static_cast<uint32_t>(pc);
}


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
		r2_rank = 0x000000000000FF00,
		r3_rank = 0x0000000000FF0000,
		r4_rank = 0x00000000FF000000,
		r5_rank = 0x000000FF00000000,
		r6_rank = 0x0000FF0000000000,
		r7_rank = 0x00FF000000000000,
		r8_rank = 0xFF00000000000000,
		not_r1_rank = ~r1_rank,
		not_r8_rank = ~r8_rank,
		a1h8_diagonal = 0x8040201008040201,
		h1_a8_diagonal = 0x0102040810204080,
		lsquares = 0x55AA55AA55AA55AA,
		dsquares = 0xAA55AA55AA55AA55,
		center = 0x0000001818000000,
		board_side[2] = { 0x00000000FFFFFFFF, 0xFFFFFFFF00000000 },
		king_center[2] = { 0x000000000000FF1C, 0x1CFF000000000000 },
		castle_sides[2][2] = { 0x0000000000000003, 0x00000000000000E0, 0x0300000000000000, 0xE000000000000000 },
		ban_dev_sq[2] = { 0xFFFFFFFFFFFFFF00, 0x00FFFFFFFFFFFFFF };

	constexpr std::array<U64, 8> r_by_index = {
		r1_rank, r2_rank, r3_rank, r4_rank, r5_rank, r6_rank, r7_rank, r8_rank
	};

	constexpr std::array<U64, 8> f_by_index = {
		a_file, a_file << 1, a_file << 2, a_file << 3, a_file << 4, a_file << 5, a_file << 6, a_file << 7
	};

} // namespace Constans


namespace {

	// empty U64 bitboard pre-compile constans
	constexpr U64 eU64 = cU64(0);

#if not defined(_MSC_VER) and not defined(__INTEL_COMPILER) and not defined(__GNUC__)

	// LS1B index finding purpose
	namespace lsbResource {

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
	}

#endif
	
	// display single U64 mask
	void printBitBoard(U64 bitboard) {
		int bitcheck = 56;

		OS << std::endl << "  ";

		for (int i = 0; i < 8; i++) {
			for (int j = 0; j < 8; j++) {
				OS << static_cast<bool>(bitboard & (cU64(1) << (bitcheck + j))) << ' ';

				if (j == 7) {
					OS << "  " << 8 - i << "\n  ";
				}
			}

			bitcheck -= 8;
		}

		OS << std::endl << "  a b c d e f g h" << std::endl << std::endl
			<< "bb value: " << bitboard << std::endl;
	}

	// Population count for magic bitboards 
	//  - counting bits set to one in a 64-bits number 'x'
	int bitCount(U64 x) {
#if defined(__INTEL_COMPILER) or defined(_MSC_VER)
		return static_cast<int>(_mm_popcnt_u64(x));
#elif defined(__GNUC__)
		return __builtin_popcount(x);
#else
		int c;
		for (c = 0; x; x &= x - 1, c++);
		return c;
#endif
	}

} // namespace


#if defined(_MSC_VER) or defined(__INTEL_COMPILER)
inline int getLS1BIndex(U64 bb) {
	assert(bb != eU64);
	unsigned long s;
	_BitScanForward64(&s, bb);
	return s;
}

inline int getMS1BIndex(U64 bb) {
	assert(bb != eU64);
	unsigned long s;
	_BitScanReverse64(&s, bb);
	return s;
}
#elif defined(__GNUC__)
inline int getLS1BIndex(U64 bb) {
	assert(bb != eU64);
	return __builtin_ctzll(bb);
}

inline int getMS1BIndex(U64 bb) {
	assert(bb != eU64);
	return __builtin_clzll(bb);
}
#else
inline constexpr int getLS1BIndex(U64 bb) noexcept {
	assert(bb != eU64);
	return lsbResource::index64[((bb ^ (bb - 1)) * lsbResource::debruijn64) >> 58];
}

inline constexpr U64 verticalFlip(U64 bb) noexcept {
	const U64 k1 = C64(0x00FF00FF00FF00FF);
	const U64 k2 = C64(0x0000FFFF0000FFFF);
	x = ((x >> 8) & k1) | ((x & k1) << 8);
	x = ((x >> 16) & k2) | ((x & k2) << 16);
	x = (x >> 32) | (x << 32);
	return x;
}

inline constexpr int getMS1BIndex(U64 bb) noexcept {
	assert(bb != eU64);
	return getLS1BIndex(verticalFlip(bb));
}
#endif

inline int popLS1B(U64& bb) {
	assert(bb != eU64);
	int res = getLS1BIndex(bb);
	bb &= bb - 1;
	return res;
}

inline constexpr void popBit(U64& bb, int shift) noexcept {
	bb &= ~bitU64(shift);
}

inline constexpr void setBit(U64& bb, int shift) noexcept {
	bb |= bitU64(shift);
}

inline constexpr bool getBit(U64 bb, int shift) noexcept {
	return bb & bitU64(shift);
}

inline constexpr void moveBit(U64& bb, int origin, int target) noexcept {
	setBit(bb, target);
	popBit(bb, origin);
}

inline constexpr U64 soutFill(U64 bb) noexcept {
	bb |= (bb >> 8);
	bb |= (bb >> 16);
	bb |= (bb >> 32);
	return bb;
}

inline constexpr U64 nortFill(U64 bb) noexcept {
	bb |= (bb << 8);
	bb |= (bb << 16);
	bb |= (bb << 32);
	return bb;
}

inline constexpr U64 fileFill(U64 bb) noexcept {
	return nortFill(bb) | soutFill(bb);
}

template <enumSide SIDE>
inline constexpr U64 rearspan(U64 bb) noexcept {
	if constexpr (SIDE) return nortFill(bb);
	return soutFill(bb);
}

template <enumSide SIDE>
inline constexpr U64 frontspan(U64 bb) noexcept {
	if constexpr (SIDE) return soutFill(bb);
	return nortFill(bb);
}

inline constexpr U64 islandsEastFile(U64 fileset) noexcept {
	return fileset ^ (fileset & (fileset >> 1));
}


namespace islandCountLookUp {
	auto lookup = cexpr::CexprArr<false, int, 256>([](U64 bb) {
		int isl_count = 0, i = 0;

		while (i < 8) {
			if (getBit(bb, i)) {
				while (getBit(bb, i++));
				isl_count++;
			} else i++;
		}

		return isl_count;
	});
}

inline constexpr int islandCount(U64 bb) {
	return islandCountLookUp::lookup.get(static_cast<uint8_t>(bb));
}