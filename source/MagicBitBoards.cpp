#include "MagicBitBoards.h"


template <enumPiece pT, class>
void printMagics() {
    std::cout << "std::array<U64, 64> m"
        << ((pT == ROOK) ? "Rook" : "Bishop") << "Tab = {" << std::endl;

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            std::cout << std::hex << "\t0x" << findSquareMagic<pT>(i * 8 + j) << ",\n";
        }
    }

    std::cout << "};" << std::endl;
}

template <enumPiece pT, class>
void printRelevantOccupancy() {
    std::cout << "std::array<U64, 64> r"
        << ((pT == ROOK) ? "Rook" : "Bishop") << "Tab = {" << std::endl;

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            std::cout << std::hex << "\t0x"
                << ((pT == ROOK) ? relevantOccupancyRook(i * 8 + j) : relevantOccupancyBishop(i * 8 + j)) << ",\n";
        }
    }

    std::cout << "};" << std::endl;
}

template <enumPiece pT, class>
void printRelevantBits() {
    std::cout << "std::array<U64, 64> revbits"
        << ((pT == ROOK) ? "Rook" : "Bishop") << "Tab = {" << std::endl;

    for (int i = 0; i < 8; i++) {
        std::cout << '\t';
        for (int j = 0; j < 8; j++) {
            std::cout << ((pT == ROOK) ? bitCount(mTabs::rRook[i * 8 + j]) : bitCount(mTabs::rBishop[i * 8 + j])) << ",";
        }
        std::cout << '\n';
    }

    std::cout << "};" << std::endl;
}
