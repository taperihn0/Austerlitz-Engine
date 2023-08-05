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
}

//	GENERALIZED SHIFT
U64 genShift(U64 bb, int shift);
 
//	ONE STEP ONLY functions
U64 nortOne(U64 bb);
U64 noEaOne(U64 bb);
U64 eastOne(U64 bb);
U64 soEaOne(U64 bb);
U64 soutOne(U64 bb);
U64 soWeOne(U64 bb);
U64 westOne(U64 bb);
U64 noWeOne(U64 bb);

U64 noNoEa(U64 b);
U64 noEaEa(U64 b);
U64 soEaEa(U64 b);
U64 soSoEa(U64 b);
U64 noNoWe(U64 b);
U64 noWeWe(U64 b);
U64 soWeWe(U64 b);
U64 soSoWe(U64 b);
 
//	PAWN PUSHES functions in bitboard
U64 wSinglePushPawn(U64 wpawns, U64 empty);
U64 wDoublePushPawn(U64 wpawns, U64 empty);

U64 bSinglePushPawn(U64 bpawns, U64 empty);
U64 bDoublePushPawn(U64 bpawns, U64 empty);
	
// PAWN ATTACKS functions - 
//  for white pawns: 
U64 wEastAttackPawn(U64 wpawns, U64 black_occ);
U64 wWestAttackPawn(U64 wpawns, U64 black_occ);
U64 wAnyAttackPawn(U64 wpawns, U64 black_occ);

// PAWN ATTACKS functions - 
//  for black pawns:
U64 bEastAttackPawn(U64 bpawns, U64 white_occ);
U64 bWestAttackPawn(U64 bpawns, U64 white_occ);
U64 bAnyAttackPawn(U64 bpawns, U64 white_occ);
