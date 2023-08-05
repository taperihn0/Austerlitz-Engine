#include "BitBoard.h"
#include "MoveSystem.h"

namespace {

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

	// relevant occupancy mask of bishop for magic bitboards system
	U64 relevantOccupancyBishop(int sq) {

		// return occupancy bitboard object
		U64 occ = eU64;
		U64 one = cU64(1);

		// piece square coordinates as numbers
		int tr = sq / 8;
		int tf = sq % 8;

		// mask relevant bishop occupancy bits
		for (int r = tr + 1, f = tf + 1; r <= 6 and f <= 6; r++, f++)
			occ |= (one << (r * 8 + f));
		for (int r = tr + 1, f = tf - 1; r <= 6 and f >= 1; r++, f--)
			occ |= (one << (r * 8 + f));
		for (int r = tr - 1, f = tf - 1; r >= 1 and f >= 1; r--, f--)
			occ |= (one << (r * 8 + f));
		for (int r = tr - 1, f = tf + 1; r >= 1 and f <= 6; r--, f++)
			occ |= (one << (r * 8 + f));

		return occ;
	}

	// same for rook as an entry point of magic bitboards system
	U64 relevantOccupancyRook(int sq) {

		// return occupancy bitboard object
		U64 occ = eU64;
		U64 one = cU64(1);

		// piece square coordinates as numbers
		int tr = sq / 8;
		int tf = sq % 8;

		// mask relevant rook occupancy bits
		for (int r = tr + 1; r <= 6; r++) occ |= (one << (r * 8 + tf));
		for (int r = tr - 1; r >= 1; r--) occ |= (one << (r * 8 + tf));
		for (int f = tf + 1; f <= 6; f++) occ |= (one << (tr * 8 + f));
		for (int f = tf - 1; f >= 1; f--) occ |= (one << (tr * 8 + f));

		return occ;
	}

} // anonymous namespace preventing violating the One Definition Rule