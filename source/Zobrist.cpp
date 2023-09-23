#include "Zobrist.h"
#include "BitBoardsSet.h"


// kind of 'wrapper' funcitions for random U64 generator
inline U64 randomU64wrap_2(int, int) {
	return randomU64();
}

inline U64 randomU64wrap_1(int) {
	return randomU64();
}


// static data members initialization
cexpr::CexprArr<true, U64, 12, 64> Zobrist::piece_keys(randomU64wrap_2);
cexpr::CexprArr<false, U64, 16> Zobrist::castle_keys(randomU64wrap_1);
cexpr::CexprArr<false, U64, 64> Zobrist::enpassant_keys(randomU64wrap_1);
U64 Zobrist::side_key = randomU64();


Zobrist::Zobrist() {
	generateKey();
}


void Zobrist::generateKey() {
	key = eU64;
	U64 tmp;
	int sq;

	// consider piece distribution
	for (int pc = nWhitePawn; pc <= nBlackKing; pc++) {
		tmp = BBs[pc];

		while (tmp) {
			sq = popLS1B(tmp);
			key ^= piece_keys.get(pc, sq);
		}
	}

	// consider en passant, player to turn and castling rights
	if (game_state.ep_sq != -1)
		key ^= enpassant_keys.get(game_state.ep_sq);

	if (game_state.turn)
		key ^= side_key;

	key ^= castle_keys.get(game_state.castle.raw());
}
