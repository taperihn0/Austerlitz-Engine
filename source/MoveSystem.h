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

#define __CEXPR inline constexpr

// generalized shift
__CEXPR U64 genShift(U64 bb, int shift) noexcept {
	return (shift > 0) ? (bb << shift) : (bb >> -shift);
}


namespace {

	//  ONE STEP ONLY functions
	__CEXPR U64 nortOne(U64 bb) { return genShift(bb, Compass::nort); }
	__CEXPR U64 noEaOne(U64 bb) { return genShift((bb & Constans::not_h_file), Compass::noEa); }
	__CEXPR U64 eastOne(U64 bb) { return genShift((bb & Constans::not_h_file), Compass::east); }
	__CEXPR U64 soEaOne(U64 bb) { return genShift((bb & Constans::not_h_file), Compass::soEa); }
	__CEXPR U64 soutOne(U64 bb) { return genShift(bb, Compass::sout); }
	__CEXPR U64 soWeOne(U64 bb) { return genShift((bb & Constans::not_a_file), Compass::soWe); }
	__CEXPR U64 westOne(U64 bb) { return genShift((bb & Constans::not_a_file), Compass::west); }
	__CEXPR U64 noWeOne(U64 bb) { return genShift((bb & Constans::not_a_file), Compass::noWe); }

	__CEXPR U64 noNoEa(U64 b) { return genShift((b & Constans::not_h_file),  Compass::noNoEa); }
	__CEXPR U64 noEaEa(U64 b) { return genShift((b & Constans::not_gh_file), Compass::noEaEa); }
	__CEXPR U64 soEaEa(U64 b) { return genShift((b & Constans::not_gh_file), Compass::soEaEa); }
	__CEXPR U64 soSoEa(U64 b) { return genShift((b & Constans::not_h_file),  Compass::soSoEa); }
	__CEXPR U64 noNoWe(U64 b) { return genShift((b & Constans::not_a_file),  Compass::noNoWe); }
	__CEXPR U64 noWeWe(U64 b) { return genShift((b & Constans::not_ab_file), Compass::noWeWe); }
	__CEXPR U64 soWeWe(U64 b) { return genShift((b & Constans::not_ab_file), Compass::soWeWe); }
	__CEXPR U64 soSoWe(U64 b) { return genShift((b & Constans::not_a_file),  Compass::soSoWe); }

#undef __CEXPR

	// function template checking move validity:
	template <int Direct>
	bool oneStep(int sq);

	template <>
	bool oneStep<Compass::noWe>(int sq) {
		return noWeOne(bitU64(sq));
	}

	template <>
	bool oneStep<Compass::soWe>(int sq) {
		return soWeOne(bitU64(sq));
	}

	template <>
	bool oneStep<Compass::noEa>(int sq) {
		return noEaOne(bitU64(sq));
	}

	template <>
	bool oneStep<Compass::soEa>(int sq) {
		return soEaOne(bitU64(sq));
	}

	template <>
	bool oneStep<Compass::west>(int sq) {
		return westOne(bitU64(sq));
	}

	template <>
	bool oneStep<Compass::east>(int sq) {
		return eastOne(bitU64(sq));
	}

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

	} // namespace PawnPushes

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

		template <enumSide SIDE>
		inline U64 bothAttackPawn(U64 pawns, U64 opp) {
			return PawnAttacks::eastAttackPawn<SIDE>(pawns, opp)
				& PawnAttacks::westAttackPawn<SIDE>(pawns, opp);
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

	} // namespace PawnAttacks


	inline U64 noNeightbourEast(U64 bb) {
		return bb & ~westOne(fileFill(bb));
	}

	inline U64 noNeightbourWest(U64 bb) {
		return bb & ~eastOne(fileFill(bb));
	}

	inline U64 isolanis(U64 bb) {
		return noNeightbourEast(bb) & noNeightbourWest(bb);
	}

	inline U64 halfIsolanis(U64 bb) {
		return noNeightbourEast(bb) ^ noNeightbourWest(bb);
	}

	template <enumSide SIDE>
	inline U64 unfreePawns(U64 own, U64 opp) {
		if constexpr (SIDE) return own & nortFill(opp);
		return own & soutFill(opp);
	}

	template <enumSide SIDE>
	inline U64 overlyAdvancedPawns(U64 own, U64 opp) {
		const U64 unfree = unfreePawns<SIDE>(own, opp);
		const U64 unfree_att = PawnAttacks::anyAttackPawn<SIDE>(unfree, UINT64_MAX);

		if constexpr (SIDE) return unfree & (soutFill(unfree_att) ^ unfree_att);
		return unfree & (nortFill(unfree_att) ^ unfree_att);
	}

} // namespace
