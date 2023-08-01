#include "BitBoard.h"

// display single bitboard function
void printBitBoard(U64 bitboard) {
	int bitcheck = 56;

	std::cout << std::endl << "  ";

	for (int i = 0; i < BHEIGHT; i++) {
		for (int j = 0; j < BWIDTH; j++) {
			std::cout << static_cast<bit>(bitboard & (cU64(1) << (bitcheck + j))) << ' ';

			if (j == 7) {
				std::cout << "  " << 8 - i << "\n  ";
			}
		}

		bitcheck -= 8;
	}

	std::cout << std::endl << "  a b c d e f g h" << std::endl << std::endl
		<< "BitBoard: " << bitboard << std::endl;
}