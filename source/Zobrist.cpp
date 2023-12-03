#include "Zobrist.h"
#include "BitBoardsSet.h"
#include "Search.h"
#include "MoveGeneration.h"

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
  side_key(randomU64()),
  key() {}


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


int TranspositionTable::read(int alpha, int beta, int g_depth, int ply) {
	const HashEntry& entry = htab[hash.key % hash_size];

	// unmatching zobrist key (key collision) or unproper depth of an entry
	if (entry.zobrist != hash.key or entry.depth < g_depth)
		return HashEntry::no_score;

	// 'extract' relative checkmate path from current node
	const int res =
		entry.score < Search::mate_comp ? entry.score + ply :
		entry.score > -Search::mate_comp ? entry.score - ply : entry.score;

	switch (entry.flag) {
	case HashEntry::Flag::HASH_EXACT:
		return res;
	case HashEntry::Flag::HASH_ALPHA:
		return (res <= alpha) ? res : HashEntry::no_score;
	case HashEntry::Flag::HASH_BETA:
		return (res >= beta) ? res : HashEntry::no_score;
	}

	return HashEntry::no_score;
}


void TranspositionTable::write(int g_depth, int g_score, HashEntry::Flag g_flag, int ply, MoveItem::iMove g_move) {
	HashEntry& entry = htab[hash.key % hash_size];

	// depth-preffered replacement scheme, but if an entry is too old, instantly replace it.
	if (entry.age > curr_age - 3 and entry.depth > g_depth)
		return;

	entry.zobrist = hash.key;

	// set original path to checkmate
	if (g_score < Search::mate_comp) g_score -= ply;
	else if (g_score > -Search::mate_comp) g_score += ply;

	entry.score = g_score;
	entry.flag = g_flag;
	entry.depth = g_depth;
	entry.age = curr_age;
	entry.move = g_move;
}


void TranspositionTable::recreatePV(int g_depth, MoveItem::iMove best, MoveItem::iMove& ponder) {
	MoveItem::iMove move;

	const BitBoardsSet bbs_cpy = BBs;
	const gState gstate_cpy = game_state;
	const U64 hash_cpy = hash.key;

	// print found in search root move and ponder move
	best.print() << ' ';
	MovePerform::makeMove(best);

	for (int i = 1; (move = tt.hashMove()) != MoveItem::iMove::no_move and i < g_depth; i++) {
		if (i == 1) ponder = move;
		move.print() << ' ';
		MovePerform::makeMove(move);
	}

	OS << '\n';

	BBs = bbs_cpy;
	game_state = gstate_cpy;
	hash.key = hash_cpy;
}


void TranspositionTable::setSize(size_t g_size) {
	memory_MB_size = 1_MB * g_size;

	// adjust memory to min/max memory tt bound size
	memory_MB_size = std::max(memory_MB_size, min_MB_size);
	memory_MB_size = std::min(memory_MB_size, max_MB_size);

	// actual hash size, as a given memory size divided by entry size
	hash_size = memory_MB_size / sizeof(HashEntry);
	htab.resize(hash_size);
	clear();
}