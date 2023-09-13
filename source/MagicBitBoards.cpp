#include "MagicBitBoards.h"


void InitState::initMAttacksTables() {
    U64 att_r, att_b, subset, att;
    int n_r, n_b;

    for (int sq = 0; sq < 64; sq++) {
        att_r = mTabs::rRook[sq];
        n_r = mTabs::rbRook[sq];
        att_b = mTabs::rBishop[sq];
        n_b = mTabs::rbBishop[sq];

        // loop throught occupancy subsets: generate each subset from index and fill look-up attacks tables
        for (const auto n : { n_r, n_b }) {
            bool r = n == n_r;
            att = r ? att_r : att_b;

            for (int i = 0; i < (1 << n); i++) {
                subset = indexSubsetU64(i, att, n);

                if (r)
                    mdata.mRookAtt[sq][mIndexHash(subset, mTabs::mRook[sq], n)]
                    = attackSquaresRook(subset, sq);
                else
                    mdata.mBishopAtt[sq][mIndexHash(subset, mTabs::mBishop[sq], n)]
                    = attackSquaresBishop(subset, sq);
            }
        }
    }
}


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
