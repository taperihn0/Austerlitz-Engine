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


TranspositionTable::TranspositionTable() {
	clear();
}


template <bool OnlyScore>
inline auto transform(const HashEntry& entry) noexcept -> std::enable_if_t<OnlyScore, int> {
	return entry.score;
}

template <bool OnlyScore>
inline auto transform(const HashEntry& entry) noexcept -> std::enable_if_t<!OnlyScore, MoveItem::iMove> {
	return entry.best_move;
}


template <ReadType rType, bool OnlyScore>
auto TranspositionTable::read(int alpha, int beta, unsigned _depth) -> std::conditional_t<OnlyScore, int, MoveItem::iMove> {
	const auto& entry = htab[hash.key % hash_size];
	HashEntry res;

	// unmatching zobrist key (key collision) or unproper depth of an entry
	if (entry.zobrist != hash.key
		or entry.depth < _depth)
		res = empty_entry;
	else
		switch (entry.flag) {
		case HashEntry::Flag::HASH_ALPHA:
			res = (entry.score <= alpha) ? entry : empty_entry;
			break;
		case HashEntry::Flag::HASH_BETA:
			res = (entry.score >= beta) ? entry : empty_entry;
			break;
		default:
			res = empty_entry;
			break;
		}

	return transform<OnlyScore>(res);
}


template int TranspositionTable::read<ReadType::ONLY_SCORE, true>(int, int, unsigned);
template MoveItem::iMove TranspositionTable::read<ReadType::ONLY_BESTMOVE, false>(int, int, unsigned);


void TranspositionTable::write(unsigned _depth, int _score, HashEntry::Flag _flag, MoveItem::iMove _bestmv) {
	auto& entry = htab[hash.key % hash_size];
	entry.zobrist = hash.key;
	entry.score = _score;
	entry.flag = _flag;
	entry.depth = _depth;
	entry.best_move = _bestmv;
}

void TranspositionTable::clear() {
	for (auto& entry : htab)
		entry = empty_entry;
}
