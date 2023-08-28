#pragma once

#include "BitBoard.h"
#include "GeneratingMagics.h"
#include "MoveSystem.h"
#include "AttackTables.h"


// helper type checking template
template <enumPiece pT, enumPiece pT1, enumPiece pT2>
using checkEqual = std::enable_if_t<pT == pT1 or pT == pT2>;

namespace {

    // magic data
    namespace mTabs {

        // Magics tables:
        constexpr std::array<U64, 64> mRook = {
            0x188001a080400431,
            0x8140002002401000,
            0x49000852600101c1,
            0x1900100021281500,
            0x9a0022005c0810a0,
            0x300082400020100,
            0xe0002000110c418,
            0x200048a05462104,
            0x4562801140008320,
            0x6489404010002000,
            0x8039002000401101,
            0x1309001000492104,
            0xe109800400080082,
            0x48a000442000950,
            0x720400101c810248,
            0x25620001240881c6,
            0x72298000400088,
            0x824000a008100122,
            0xc4c6020012802048,
            0x81030020700208,
            0xd012020008218410,
            0x10c004002004100,
            0x23c040041081042,
            0x450406000042930c,
            0xb16c0008005208a,
            0x8510181002b4002,
            0x8103118200204601,
            0x9205002500081002,
            0x90a0006000811a0,
            0x102010200100488,
            0x403044000f0258,
            0x6101258200114904,
            0x464830206002140,
            0x1bc8804005002110,
            0x20a00061030041b1,
            0x1b00206b003000,
            0x8830a008600208e,
            0x402018402001008,
            0x900090026c004801,
            0x193304082000c07,
            0xa818400882258000,
            0x3c090200804a0021,
            0x4819c0a201820011,
            0x810020d0050009,
            0x490080100050010,
            0x8200188c620010,
            0x130450860d040008,
            0xa5841400a4460011,
            0x511ac18000a100,
            0x29003140068100,
            0x134582d008600080,
            0xa2c102085000a500,
            0x881000800045300,
            0x116004811041a00,
            0xd228b01801860c00,
            0x10860c11118c4200,
            0x2282042013024382,
            0x4028420085022112,
            0x8008421020010903,
            0x248608a4012003a,
            0x512001c200830aa,
            0x448a00140805501a,
            0x944302802008104,
            0x28811300442c0082,
        };
        constexpr std::array<U64, 64> mBishop = {
            0xd0100150140149c2,
            0x400b4a8fe1010031,
            0x480a1420200000,
            0x12c8204840c98020,
            0x284050484ca00c2,
            0x2006082494118828,
            0x8c02020282410320,
            0x24401208140c2408,
            0x30c060a0220a3845,
            0x5458108248004488,
            0x70980404440048b8,
            0xa41040512093810,
            0x11020c0308012920,
            0x4a02011002114209,
            0x8d0a090a4304090,
            0x9bea06640c0431,
            0xc87106a802500400,
            0xa410090424009419,
            0xe10054800401022,
            0x20f080080823000c,
            0x808210540104120a,
            0x1a2a002b00908c0a,
            0x8c20608481e0850,
            0x3927010043049058,
            0x2d0842ac0040400,
            0x1004224c60880128,
            0x10903004080e4340,
            0x80060080080280a2,
            0x8019001005004021,
            0x80230a010a481401,
            0x74040401dc94840a,
            0xa30906024720841c,
            0x5022824000903014,
            0x8988085440583120,
            0xd03e0022243c0801,
            0x823020082280080,
            0x6410020202082008,
            0x18608401000a9010,
            0xc92020c0f421088,
            0x1034028220408c00,
            0x4080d8202810f000,
            0x602a88089814c218,
            0x95e0051c80c0400,
            0x44400201800c905,
            0x800041810118601,
            0x822408113001a01,
            0x178c752e04003600,
            0x8018c1040a813620,
            0x10610420200c40,
            0x409040104030830,
            0xc40404844103100,
            0x1952d001420a00d1,
            0x63302402d050c10,
            0x4840202092008ca0,
            0x85a2121002009a82,
            0xb101d43090e0006,
            0x803401044000,
            0x4284c2218c2050,
            0x200246270c060911,
            0x9006f16002420882,
            0x115571440283201,
            0xa90084202212ca00,
            0x804a042048123092,
            0x838e00408820010,
        };

        // Relevant occupancy bitboards:
        constexpr std::array<U64, 64> rRook = {
            0x101010101017e,
            0x202020202027c,
            0x404040404047a,
            0x8080808080876,
            0x1010101010106e,
            0x2020202020205e,
            0x4040404040403e,
            0x8080808080807e,
            0x1010101017e00,
            0x2020202027c00,
            0x4040404047a00,
            0x8080808087600,
            0x10101010106e00,
            0x20202020205e00,
            0x40404040403e00,
            0x80808080807e00,
            0x10101017e0100,
            0x20202027c0200,
            0x40404047a0400,
            0x8080808760800,
            0x101010106e1000,
            0x202020205e2000,
            0x404040403e4000,
            0x808080807e8000,
            0x101017e010100,
            0x202027c020200,
            0x404047a040400,
            0x8080876080800,
            0x1010106e101000,
            0x2020205e202000,
            0x4040403e404000,
            0x8080807e808000,
            0x1017e01010100,
            0x2027c02020200,
            0x4047a04040400,
            0x8087608080800,
            0x10106e10101000,
            0x20205e20202000,
            0x40403e40404000,
            0x80807e80808000,
            0x17e0101010100,
            0x27c0202020200,
            0x47a0404040400,
            0x8760808080800,
            0x106e1010101000,
            0x205e2020202000,
            0x403e4040404000,
            0x807e8080808000,
            0x7e010101010100,
            0x7c020202020200,
            0x7a040404040400,
            0x76080808080800,
            0x6e101010101000,
            0x5e202020202000,
            0x3e404040404000,
            0x7e808080808000,
            0x7e01010101010100,
            0x7c02020202020200,
            0x7a04040404040400,
            0x7608080808080800,
            0x6e10101010101000,
            0x5e20202020202000,
            0x3e40404040404000,
            0x7e80808080808000,
        };
        constexpr std::array<U64, 64> rBishop = {
            0x40201008040200,
            0x402010080400,
            0x4020100a00,
            0x40221400,
            0x2442800,
            0x204085000,
            0x20408102000,
            0x2040810204000,
            0x20100804020000,
            0x40201008040000,
            0x4020100a0000,
            0x4022140000,
            0x244280000,
            0x20408500000,
            0x2040810200000,
            0x4081020400000,
            0x10080402000200,
            0x20100804000400,
            0x4020100a000a00,
            0x402214001400,
            0x24428002800,
            0x2040850005000,
            0x4081020002000,
            0x8102040004000,
            0x8040200020400,
            0x10080400040800,
            0x20100a000a1000,
            0x40221400142200,
            0x2442800284400,
            0x4085000500800,
            0x8102000201000,
            0x10204000402000,
            0x4020002040800,
            0x8040004081000,
            0x100a000a102000,
            0x22140014224000,
            0x44280028440200,
            0x8500050080400,
            0x10200020100800,
            0x20400040201000,
            0x2000204081000,
            0x4000408102000,
            0xa000a10204000,
            0x14001422400000,
            0x28002844020000,
            0x50005008040200,
            0x20002010080400,
            0x40004020100800,
            0x20408102000,
            0x40810204000,
            0xa1020400000,
            0x142240000000,
            0x284402000000,
            0x500804020000,
            0x201008040200,
            0x402010080400,
            0x2040810204000,
            0x4081020400000,
            0xa102040000000,
            0x14224000000000,
            0x28440200000000,
            0x50080402000000,
            0x20100804020000,
            0x40201008040200,
        };

        // relevant bits:
        constexpr std::array<int, 64> rbRook = {
            12,11,11,11,11,11,11,12,
            11,10,10,10,10,10,10,11,
            11,10,10,10,10,10,10,11,
            11,10,10,10,10,10,10,11,
            11,10,10,10,10,10,10,11,
            11,10,10,10,10,10,10,11,
            11,10,10,10,10,10,10,11,
            12,11,11,11,11,11,11,12,
        };
        constexpr std::array<int, 64> rbBishop = {
            6,5,5,5,5,5,5,6,
            5,5,5,5,5,5,5,5,
            5,5,7,7,7,7,5,5,
            5,5,7,9,9,7,5,5,
            5,5,7,9,9,7,5,5,
            5,5,7,7,7,7,5,5,
            5,5,5,5,5,5,5,5,
            6,5,5,5,5,5,5,6,
        };

        // look-up tables of rook and bishop attacks in Plain Magic Bitboards implementation
        // 4096 = 2 ^ 12 - maximum number of occupancy subsets for rook (rook at [a1, h8])
        // 512 = 2 ^ 9 - maximum number of occupancy subsets for bishop (bishop at board center [d4, d5, e4, e5])
        std::array<std::array<U64, 4096>, 64> mRookAtt;
        std::array<std::array<U64, 512>, 64> mBishopAtt;

    } // namespace mTabs


    // InitState namespace contains program init functions which needs to be called
    // when the program starts
    namespace InitState {

        // initialize look-up attacks tables for sliders except queen
        // moving definition of this function to implementation .cpp file cause linker error
        template <enumPiece pT, class =
            checkEqual<pT, ROOK, BISHOP>>
        void initMAttacksTables() {
            U64 att, subset;
            int n;

            for (int sq = 0; sq < 64; sq++) {
                switch (pT) {
                case ROOK:
                    att = mTabs::rRook[sq];
                    n = mTabs::rbRook[sq];
                    break;
                case BISHOP:
                    att = mTabs::rBishop[sq];
                    n = mTabs::rbBishop[sq];
                    break;
                default: return;
                }

                // loop throught occupancy subsets: generate each subset from index and fill look-up attacks tables
                for (int i = 0; i < (1 << n); i++) {
                    subset = indexSubsetU64(i, att, n);

                    switch (pT) {
                    case ROOK:
                        mTabs::mRookAtt[sq][mIndexHash(subset, mTabs::mRook[sq], n)]
                            = attackSquaresRook(subset, sq);
                        break;
                    case BISHOP:
                        mTabs::mBishopAtt[sq][mIndexHash(subset, mTabs::mBishop[sq], n)]
                            = attackSquaresBishop(subset, sq);
                        break;
                    default: return;
                    }
                }
            }
        }

    }


    // handy function templates for generating sliding pieces attacks using pre-generated magic bitboards
    // supported pieces: sliders and knight

    template <enumPiece pT>
    U64 attack(U64, int) {
        static_assert(pT == KNIGHT or pT == BISHOP or pT == ROOK or pT == QUEEN,
            "Unsupported piece type by attack function");
        return eU64;
    }

    template <>
    U64 attack<KNIGHT>(U64, int sq) {
        return cknight_attacks[sq];
    }

    template <>
    U64 attack<BISHOP>(U64 occ, int sq) {
        return mTabs::mBishopAtt[sq][mIndexHash(occ & mTabs::rBishop[sq], mTabs::mBishop[sq], mTabs::rbBishop[sq])];
    }

    template <>
    U64 attack<ROOK>(U64 occ, int sq) {
        return mTabs::mRookAtt[sq][mIndexHash(occ & mTabs::rRook[sq], mTabs::mRook[sq], mTabs::rbRook[sq])];
    }

    template <>
    U64 attack<QUEEN>(U64 occ, int sq) {
        return attack<BISHOP>(occ, sq) | attack<ROOK>(occ, sq);
    }

} // namespace


template <enumPiece pT, class =
    checkEqual<pT, BISHOP, ROOK>>
void printMagics();

template <enumPiece pT, class =
    checkEqual<pT, BISHOP, ROOK>>
void printRelevantOccupancy();

template <enumPiece pT, class =
    checkEqual<pT, BISHOP, ROOK>>
void printRelevantBits();
