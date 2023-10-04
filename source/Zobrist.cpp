#include "Zobrist.h"
#include "BitBoardsSet.h"

// kind of 'wrapper' funcitions for random U64 generator
inline U64 randomU64wrap_2(int, int) {
	return randomU64();
}

inline U64 randomU64wrap_1(int) {
	return randomU64();
}

Zobrist::Zobrist() 
: piece_keys(randomU64wrap_2), 
  castle_keys(randomU64wrap_1), 
  enpassant_keys(randomU64wrap_1), 
  side_key(randomU64()) {
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


int TranspositionTable::read(int alpha, int beta, int _depth) {
	const auto& entry = htab[hash.key % hash_size];

	// unmatching zobrist key (key collision) or unproper depth of an entry
	if (entry.zobrist != hash.key or entry.depth < _depth)
		return no_score;

	switch (entry.flag) {
	case HashEntry::Flag::HASH_EXACT:
		return entry.score;
	case HashEntry::Flag::HASH_ALPHA:
		return (entry.score <= alpha) ? entry.score : no_score;
	case HashEntry::Flag::HASH_BETA:
		return (entry.score >= beta) ? entry.score : no_score;
	default: break;
	}

	return no_score;
}


void TranspositionTable::write(int _depth, int _score, HashEntry::Flag _flag) {
	auto& entry = htab[hash.key % hash_size];
	entry.zobrist = hash.key;
	entry.score = _score;
	entry.flag = _flag;
	entry.depth = _depth;
}

