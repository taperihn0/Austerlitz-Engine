#pragma once

#include "BitBoard.h"
#include "GeneratingMagics.h"


class Zobrist {
public:
	Zobrist();

	// calculate Zobrist key for current position
	void generateKey();

	U64 key;
private:
	// 12 - number of all pieces
	static cexpr::CexprArr<true, U64, 12, 64> piece_keys;
	static cexpr::CexprArr<false, U64, 16> castle_keys;
	static cexpr::CexprArr<false, U64, 64> enpassant_keys;
	static U64 side_key;
};

// forward declaration
extern Zobrist hash;