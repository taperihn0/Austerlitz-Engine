#include "BitBoard.h"
#include "MagicBitBoards.h"
#include <random>

namespace {

	// random U64 bitboard using Mersenne Twister generator
	U64 randomU64() {
		static std::mt19937_64 engine(1);
		// minimum random number set to 1, since 0 can't be magic number
		static std::uniform_int_distribution<U64> dist(1, UINT64_MAX);

		return dist(engine);
	}

	// get subset of attack_mask by using binary representation of index
	U64 indexSubsetU64(int index, U64 attack_mask, int bit_count) {
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
	U64 attackSquaresRook(U64 occ, int sq) {
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
	U64 attackSquaresBishop(U64 occ, int sq) { 
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

	// hash function using magic numbers
	int mIndexHash(U64 occ, U64 magic, int relv_bits) {
		return static_cast<int>((occ * magic) >> (64 - relv_bits));
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

	// print elegant magic numbers for rook or bishop
	template <enumPiece pT>
	void printMagics() {
		std::cout << "std::array<U64, 64> m" 
			<< ((pT == ROOK) ? "Rook" : "Bishop") << "RookTab = {" << std::endl;

		for (int i = 0; i < 8; i++) {
			for (int j = 0; j < 8; j++) {
				std::cout << std::hex << "\t0x" << findSquareMagic<pT>(i * 8 + j) << ",\n";
			}
		}

		std::cout << '}' << std::endl;
	}
}