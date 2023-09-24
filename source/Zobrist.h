#pragma once

#include "BitBoard.h"
#include "GeneratingMagics.h"
#include "MoveItem.h"


struct Zobrist {
	Zobrist();

	// calculate Zobrist key for current position
	void generateKey();

	// 12 - number of all pieces (nWhitePawn..nBlackKing)
	cexpr::CexprArr<true, U64, 12, 64> piece_keys;
	cexpr::CexprArr<false, U64, 16> castle_keys;
	cexpr::CexprArr<false, U64, 64> enpassant_keys;
	U64 side_key;

	// raw Zobrist key
	U64 key;
};

// forward declaration
extern Zobrist hash;

// single entry in hash table structure
struct HashEntry {

	// hashing flags
	enum class Flag {
		HASH_EXACT,
		HASH_ALPHA,
		HASH_BETA
	};

	static inline bool isValid(int _score) noexcept;
	static inline bool isValid(MoveItem::iMove _move) noexcept;

	U64 zobrist;
	unsigned depth;
	Flag flag;
	int score;
	MoveItem::iMove best_move;
};

// type for read function in transposition table
enum class ReadType {
	ONLY_SCORE, ONLY_BESTMOVE,
};

// main transposition table class
class TranspositionTable {
public:
	static constexpr size_t hash_size = 0x400000;
	static constexpr int no_score = std::numeric_limits<int>::min();
	static constexpr MoveItem::iMove no_move = 0;
	static constexpr HashEntry empty_entry = { 0, 0, HashEntry::Flag::HASH_EXACT, no_score, no_move };

	TranspositionTable();

	template <ReadType rType, bool OnlyScore = rType == ReadType::ONLY_SCORE>
	auto read(int alpha, int beta, unsigned _depth) -> std::conditional_t<OnlyScore, int, MoveItem::iMove>;

	void write(unsigned _depth, int _score, HashEntry::Flag _flag, MoveItem::iMove _bestmv);
	void clear();
private:
	std::array<HashEntry, hash_size> htab;
};


inline bool HashEntry::isValid(int _score) noexcept {
	return _score != TranspositionTable::no_score;
}

inline bool HashEntry::isValid(MoveItem::iMove _move) noexcept {
	return _move.raw() != TranspositionTable::no_move.raw();
}