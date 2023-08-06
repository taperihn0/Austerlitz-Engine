#include "BitBoard.h"
#include "MoveSystem.h"

namespace {

	// relevant occupancy mask of bishop for magic bitboards system
	U64 relevantOccupancyBishop(int sq) {

		// return occupancy bitboard object
		U64 occ = eU64;
		U64 one = cU64(1);

		// piece square coordinates as numbers
		int tr = sq / 8, tf = sq % 8;

		// mask relevant bishop occupancy bits
		for (int r = tr + 1, f = tf + 1; r <= 6 and f <= 6; r++, f++)
			setBit(occ, (r * 8 + f));
		for (int r = tr + 1, f = tf - 1; r <= 6 and f >= 1; r++, f--)
			setBit(occ, (r * 8 + f));
		for (int r = tr - 1, f = tf - 1; r >= 1 and f >= 1; r--, f--)
			setBit(occ, (r * 8 + f));
		for (int r = tr - 1, f = tf + 1; r >= 1 and f <= 6; r--, f++)
			setBit(occ, (r * 8 + f));

		return occ;
	}

	// same as above for rook as an entry point of magic bitboards system
	U64 relevantOccupancyRook(int sq) {

		// return occupancy bitboard object
		U64 occ = eU64;
		U64 one = cU64(1);

		// piece square coordinates as numbers
		int tr = sq / 8, tf = sq % 8;

		// mask relevant rook occupancy bits
		for (int r = tr + 1; r <= 6; r++) setBit(occ, (r * 8 + tf));
		for (int r = tr - 1; r >= 1; r--) setBit(occ, (r * 8 + tf));
		for (int f = tf + 1; f <= 6; f++) setBit(one, (tr * 8 + f));
		for (int f = tf - 1; f >= 1; f--) setBit(one, (tr * 8 + f));

		return occ;
	}

	struct SMagic {
		U64 mask;  // relevant occupancy mask
		U64 magic; // magic number needed to be found earlier
	};

	// arrays of magic structure containing relevant occupancy mask and magic number for each square
	// separately for bishop and rook
	std::array<SMagic, 64>
		mBishopTab,
		mRookTab;

	// tables of rook and bishop attacks in Plain Magic Bitboards implementation
	// 4096 = 2 ^ 12 - maximum number of occupancy subsets for rook (rook at [a1, h8])
	// 512 = 2 ^ 9 - maximum number of occupancy subsets for bishop (bishop at board center [d4, d5, e4, e5])
	std::array<std::array<U64, 4096>, 64> mRookAttacks;
	std::array<std::array<U64, 512>, 64> mBishopAttacks;

} // anonymous namespace preventing violating the One Definition Rule