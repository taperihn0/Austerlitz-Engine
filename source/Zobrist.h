#pragma once

#include "BitBoard.h"
#include "GeneratingMagics.h"


struct Zobrist {
	Zobrist();

	// calculate Zobrist key for current position
	void generateKey();

	// 12 - number of all pieces
	cexpr::CexprArr<true, U64, 12, 64> piece_keys;
	cexpr::CexprArr<false, U64, 16> castle_keys;
	cexpr::CexprArr<false, U64, 64> enpassant_keys;
	U64 side_key;

	// raw Zobrist key
	U64 key;
};

// forward declaration
extern Zobrist hash;