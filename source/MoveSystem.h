#pragma once

#include "BitBoard.h"

//	compass rose for bit shifting while moving in bitboard 
namespace Compass {

	// constants usefull especially for one step only concept
	// or for pawn movement system
	constexpr int
		nort = 8,
		noEa = 9,
		east = 1,
		soEa = -7,
		sout = -8,
		soWe = -9,
		west = -1,
		noWe = 7;

	// constants used for knight movement
	constexpr int
		noNoEa = 17,
		noEaEa = 10,
		soEaEa = -6,
		soSoEa = -15,
		soSoWe = -17,
		soWeWe = -10,
		noWeWe = 6,
		noNoWe = 15;

} // namespace Compass

// generalized shift
inline U64 genShift(U64 bb, int shift) noexcept {
	return (shift > 0) ? (bb << shift) : (bb >> -shift);
}


namespace {

	//  ONE STEP ONLY functions
	U64 nortOne(U64 bb) { return genShift(bb, Compass::nort); }
	U64 noEaOne(U64 bb) { return genShift((bb & Constans::not_h_file), Compass::noEa); }
	U64 eastOne(U64 bb) { return genShift((bb & Constans::not_h_file), Compass::east); }
	U64 soEaOne(U64 bb) { return genShift((bb & Constans::not_h_file), Compass::soEa); }
	U64 soutOne(U64 bb) { return genShift(bb, Compass::sout); }
	U64 soWeOne(U64 bb) { return genShift((bb & Constans::not_a_file), Compass::soWe); }
	U64 westOne(U64 bb) { return genShift((bb & Constans::not_a_file), Compass::west); }
	U64 noWeOne(U64 bb) { return genShift((bb & Constans::not_a_file), Compass::noWe); }

	U64 noNoEa(U64 b) { return genShift((b & Constans::not_h_file), Compass::noNoEa); }
	U64 noEaEa(U64 b) { return genShift((b & Constans::not_gh_file), Compass::noEaEa); }
	U64 soEaEa(U64 b) { return genShift((b & Constans::not_gh_file), Compass::soEaEa); }
	U64 soSoEa(U64 b) { return genShift((b & Constans::not_h_file), Compass::soSoEa); }
	U64 noNoWe(U64 b) { return genShift((b & Constans::not_a_file), Compass::noNoWe); }
	U64 noWeWe(U64 b) { return genShift((b & Constans::not_ab_file), Compass::noWeWe); }
	U64 soWeWe(U64 b) { return genShift((b & Constans::not_ab_file), Compass::soWeWe); }
	U64 soSoWe(U64 b) { return genShift((b & Constans::not_a_file), Compass::soSoWe); }

	// some resources of pawn pushing system
	namespace PawnPushes {

		template <enumSide SIDE>
		U64 singlePushPawn(U64 pawns, U64 empty);

		template <enumSide SIDE>
		U64 doublePushPawn(U64 pawns, U64 empty);

		template <>
		U64 singlePushPawn<WHITE>(U64 pawns, U64 empty) {
			return nortOne(pawns) & empty;
		}

		template <>
		U64 singlePushPawn<BLACK>(U64 pawns, U64 empty) {
			return soutOne(pawns) & empty;
		}

		template <>
		U64 doublePushPawn<WHITE>(U64 pawns, U64 empty) {
			return nortOne(singlePushPawn<WHITE>(pawns, empty)) & empty;
		}

		template <>
		U64 doublePushPawn<BLACK>(U64 pawns, U64 empty) {
			return soutOne(singlePushPawn<BLACK>(pawns, empty)) & empty;
		}

	}

	// pawn attacks functions - 
	//  for white and black pawns 
	namespace PawnAttacks {

		template <enumSide SIDE>
		U64 eastAttackPawn(U64 pawns, U64 opp_occ);

		template <enumSide SIDE>
		U64 westAttackPawn(U64 pawns, U64 opp_occ);

		template <enumSide SIDE>
		inline U64 anyAttackPawn(U64 pawns, U64 opp_occ) {
			return PawnAttacks::eastAttackPawn<SIDE>(pawns, opp_occ)
				| PawnAttacks::westAttackPawn<SIDE>(pawns, opp_occ);
		}

		template <>
		inline U64 eastAttackPawn<WHITE>(U64 pawns, U64 opp_occ) {
			return noEaOne(pawns) & opp_occ;
		}

		template <>
		inline U64 eastAttackPawn<BLACK>(U64 pawns, U64 opp_occ) {
			return soEaOne(pawns) & opp_occ;
		}

		template <>
		inline U64 westAttackPawn<WHITE>(U64 pawns, U64 opp_occ) {
			return noWeOne(pawns) & opp_occ;
		}

		template <>
		inline U64 westAttackPawn<BLACK>(U64 pawns, U64 opp_occ) {
			return soWeOne(pawns) & opp_occ;
		}

		template <>
		inline U64 anyAttackPawn<WHITE>(U64 pawns, U64 opp_occ) {
			return PawnAttacks::eastAttackPawn<WHITE>(pawns, opp_occ)
				| PawnAttacks::westAttackPawn<WHITE>(pawns, opp_occ);
		}
	}

} // namespace
