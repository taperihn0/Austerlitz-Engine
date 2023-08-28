#pragma once

#include "BitBoard.h"
#include <random>


// hash function using magic numbers
inline constexpr int mIndexHash(U64 occ, U64 magic, int relv_bits) {
	return static_cast<int>((occ * magic) >> (64 - relv_bits));
}


namespace {

	// relevant occupancy mask of bishop for magic bitboards system
	U64 relevantOccupancyBishop(int sq) {

		// return occupancy bitboard object
		U64 occ = eU64;

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

		// piece square coordinates as numbers
		int tr = sq / 8, tf = sq % 8;

		// mask relevant rook occupancy bits
		for (int r = tr + 1; r <= 6; r++) setBit(occ, (r * 8 + tf));
		for (int r = tr - 1; r >= 1; r--) setBit(occ, (r * 8 + tf));
		for (int f = tf + 1; f <= 6; f++) setBit(occ, (tr * 8 + f));
		for (int f = tf - 1; f >= 1; f--) setBit(occ, (tr * 8 + f));

		return occ;
	}

	// random U64 bitboard using Mersenne Twister generator
	U64 randomU64() {
		static std::mt19937_64 engine(1);
		// minimum random number set to 1, since 0 can't be magic number
		static std::uniform_int_distribution<U64> dist(1, UINT64_MAX);

		return dist(engine) & dist(engine);
	}

	// get subset of attack_mask by using binary representation of index
	constexpr U64 indexSubsetU64(int index, U64 attack_mask, int bit_count) {
		U64 subset = eU64;

		for (int count = 0; count < bit_count; count++) {
			int lsb_index = getLS1BIndex(attack_mask);
			popBit(attack_mask, lsb_index);

			if (index & (1 << count)) {
				setBit(subset, lsb_index);
			}
		}

		return subset;
	}

	// construct bitboard of attacked squares by rook 
	// (including friendly pieces which are attacked - thats illegal)
	constexpr U64 attackSquaresRook(U64 occ, int sq) {
		U64 attacks = eU64;

		int tr = sq / 8, tf = sq % 8;

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
	
	// same as above but now for bishop
	constexpr U64 attackSquaresBishop(U64 occ, int sq) { 
		U64 attacks = eU64;
		int tr = sq / 8, tf = sq % 8;

		for (int r = tr + 1, f = tf + 1; r <= 7 and f <= 7; r++, f++) {
			setBit(attacks, r * 8 + f);
			if (occ & (cU64(1) << (r * 8 + f))) break;
		}

		for (int r = tr + 1, f = tf - 1; r <= 7 and f >= 0; r++, f--) {
			setBit(attacks, r * 8 + f);
			if (occ & (cU64(1) << (r * 8 + f))) break;
		}

		for (int r = tr - 1, f = tf - 1; r >= 0 and f >= 0; r--, f--) {
			setBit(attacks, r * 8 + f);
			if (occ & (cU64(1) << (r * 8 + f))) break;
		}

		for (int r = tr - 1, f = tf + 1; r >= 0 and f <= 7; r--, f++) {
			setBit(attacks, r * 8 + f);
			if (occ & (cU64(1) << (r * 8 + f))) break;
		}

		return attacks;
	}

	// find magic number for given square for rook or bishop via trial and error method
	template <enumPiece pT>
	U64 findSquareMagic(int sq, unsigned int tries = 100000000) {
		std::array<U64, 4096> att, occup, check;
		U64 magic, 
			relv_mask = (pT == ROOK) ? relevantOccupancyRook(sq) : relevantOccupancyBishop(sq);
		int new_index,
			n = bitCount(relv_mask);
		bool unvalid = false;

		// looping through all the subsets of relevant occupancy - 
		// remembering results in an array and generating appropiate attack bitboards for such occupancy subsets
		for (int i = 0; i < (1 << n); i++) {
			occup[i] = indexSubsetU64(i, relv_mask, n);
			att[i] = (pT == ROOK) ? attackSquaresRook(occup[i], sq) : attackSquaresBishop(occup[i], sq);
		}

		// trying to generate random U64 intiger meeting conditions - 
		// that will be magic number of given square
		for (unsigned int t = 0; t < tries; t++) {
			magic = randomU64();

			// number has to consist of many bits after shift in magic hash function
			if (bitCount((relv_mask * magic) & 0xFF00000000000000) < 6) continue;

			// prepare array for the next try with new generated number
			check.fill(eU64);
			unvalid = false;

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
				}
			}

			// passed all the tests - magic number found
			if (!unvalid) {
				return magic;
			}
		}

		std::cout << "No magic number found - ";
		return eU64;
	}

} // namespace