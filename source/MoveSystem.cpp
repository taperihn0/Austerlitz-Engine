#include "MoveSystem.h"

//	GENERALIZED SHIFT
U64 genShift(U64 bb, int shift) {
	return (shift > 0) ? (bb << shift) : (bb >> -shift);
}

//	ONE STEP ONLY functions definitions
U64 nortOne(U64 bb) { return genShift(bb, Compass::nort); };
U64 noEaOne(U64 bb) { return genShift((bb & Constans::not_h_file), Compass::noEa); }
U64 eastOne(U64 bb) { return genShift((bb & Constans::not_h_file), Compass::east); }
U64 soEaOne(U64 bb) { return genShift((bb & Constans::not_h_file), Compass::soEa); }
U64 soutOne(U64 bb) { return genShift(bb, Compass::sout); }
U64 soWeOne(U64 bb) { return genShift((bb & Constans::not_a_file), Compass::soWe); }
U64 westOne(U64 bb) { return genShift((bb & Constans::not_a_file), Compass::west); }
U64 noWeOne(U64 bb) { return genShift((bb & Constans::not_a_file), Compass::noWe); }

U64 noNoEa(U64 b) { return genShift((b & Constans::not_h_file),  Compass::noNoEa); }
U64 noEaEa(U64 b) { return genShift((b & Constans::not_gh_file), Compass::noEaEa); }
U64 soEaEa(U64 b) { return genShift((b & Constans::not_gh_file), Compass::soEaEa); }
U64 soSoEa(U64 b) { return genShift((b & Constans::not_h_file),  Compass::soSoEa); }
U64 noNoWe(U64 b) { return genShift((b & Constans::not_a_file),  Compass::noNoWe); }
U64 noWeWe(U64 b) { return genShift((b & Constans::not_ab_file), Compass::noWeWe); }
U64 soWeWe(U64 b) { return genShift((b & Constans::not_ab_file), Compass::soWeWe); }
U64 soSoWe(U64 b) { return genShift((b & Constans::not_a_file),  Compass::soSoWe); }

//	PAWN PUSHES functions in bitboard definitions
U64 wSinglePushPawn(U64 wpawns, U64 empty) {
	return nortOne(wpawns) & empty;
}

U64 wDoublePushPawn(U64 wpawns, U64 empty) {
	return nortOne(wSinglePushPawn(wpawns, empty)) & empty;
}

U64 bSinglePushPawn(U64 bpawns, U64 empty) {
	return soutOne(bpawns) & empty;
}

U64 bDoublePushPawn(U64 bpawns, U64 empty) {
	return soutOne(bSinglePushPawn(bpawns, empty)) & empty;
}

//	PAWN ATTACKS functions -
//	 for white pawns:
U64 wEastAttackPawn(U64 wpawns, U64 black_occ) {
	return noEaOne(wpawns) & black_occ;
}

U64 wWestAttackPawn(U64 wpawns, U64 black_occ) {
	return noWeOne(wpawns) & black_occ;
}

U64 wAnyAttackPawn(U64 wpawns, U64 black_occ) {
	return wEastAttackPawn(wpawns, black_occ) | wWestAttackPawn(wpawns, black_occ);
}

//	PAWN ATTACKS functions -
//   for black pawns:
U64 bEastAttackPawn(U64 bpawns, U64 white_occ) {
	return soEaOne(bpawns) & white_occ;
}

U64 bWestAttackPawn(U64 bpawns, U64 white_occ) {
	return soWeOne(bpawns) & white_occ;
}

U64 bAnyAttackPawn(U64 bpawns, U64 white_occ) {
	return bEastAttackPawn(bpawns, white_occ) | bWestAttackPawn(bpawns, white_occ);
}