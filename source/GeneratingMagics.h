#include "BitBoard.h"
#include "MagicBitBoards.h"
#include <random>

namespace {

	// random U64 bitboard
	U64 randomU64() {

		// Mersenne Twister generator
		static std::mt19937_64 engine(1);

		// minimum random number set to 1, since 0 can't be magic number
		static std::uniform_int_distribution<U64> dist(1, UINT64_MAX);

		return dist(engine) & dist(engine) & dist(engine);
	}

	// get subset of attack_mask by using binary representation of index
	U64 indexSubsetU64(int index, U64 attack_mask, int bit_count) {
		U64 subset = eU64;

		for (int count = 0; count < bit_count; count++) {
			int lsb_index = getLS1B_Index(attack_mask);
			popBit(attack_mask, lsb_index);

			if (index & (1 << count)) {
				setBit(subset, lsb_index);
			}
		}

		return subset;
	}

	// construct bitboard of attacked squares by rook 
	// (including friendly pieces which are attacked - thats illegal)
	U64 attackSquaresRook(U64 occ, int sq) {
		U64 attacks = eU64;

		int tr = sq / 8;
		int tf = sq % 8;

		for (int f = tf + 1; f <= 7; f++) {
			setBit(attacks, tr * 8 + f);
			if (occ & (cU64(1) << (tr * 8 + f))) break;
		}

		for (int r = tr + 1; r <= 7; r++) {
			setBit(attacks, r * 8 + tf);
			if (occ & (cU64(1) << (r * 8 + tf))) break;
		}

		for (int f = tf - 1; f >= 0; f--) {
			setBit(attacks, tr * 8 + f);
			if (occ & (cU64(1) << (tr * 8 + f))) break;
		}

		for (int r = tr - 1; r >= 0; r--) {
			setBit(attacks, r * 8 + tf);
			if (occ & (cU64(1) << (r * 8 + tf))) break;
		}

		return attacks;
	}

	// TO DO!!
	// same as above but now for bishop
	U64 attackSquaresBishop(U64 occ, int sq) { return eU64; }

	// hash function using magic numbers
	int mIndexHash(U64 occ, U64 magic, int relv_bits) {
		return static_cast<int>(occ * magic) << (64 - relv_bits);
	}

	// find magic number for given square for rook or bishop via trial and error method
	template <enumPiece pT, std::enable_if_t<pT == BISHOP or pT == ROOK>>
	U64 findSquareMagic(int sq, int n, unsigned int tries = 100000000) {
		std::array<U64, 4096> att, occup, check;
		U64 magic, 
			relv_mask = (pT == ROOK) ? relevantOccupancyRook(sq) : relevantOccupancyBishop(sq);
		int new_index;
		bool unvalid = true;

		// looping through all the subsets of relevant occupancy - 
		// remembering results in an array and generating appropiate attack bitboards for such occupancy subsets
		for (int i = 0; i < (1 << n); i++) {
			occup[i] = indexSubsetU64(i, relv_mask, n);
			att[i] = (pT == ROOK) ? attackSquaresRook(occup[i], sq) : attackSquaresBishop(occup[i], sq);
		}

		// trying to generate random U64 intiger meeting conditions - 
		// that will be magic number of given square
		for (int t = 0; t < tries; t++) {
			magic = randomU64();

			// skip generated numbers containing too little relevant bits
			if (bitCount((relv_mask * magic) & 0xff00000000000000))
				continue;

			// prepare array for the next try with new generated number
			check.fill(eU64);

			// check hashing using generated number for every occupancy subset
			for (int i = 0; i < (1 << n) and !unvalid; i++) {
				new_index = mIndexHash(occup[i], magic, n);

				// checking whether new index was not taken earlier
				if (check[new_index] == eU64) {
					check[new_index] = att[i];
				} // and if it was taken, maybe new index 'covers' same attack bitboard - it's still valid
				  // if the attack was different then, generated number is unvalid
				else if (check[new_index] != att[i]) {
					unvalid = true;
					break;
				}
			}

			// passed all the tests - magic number found
			if (!unvalid) {
				return magic;
			}
		}

		std::cout << "No magic number found" << std::endl;
		return eU64;
	}
}