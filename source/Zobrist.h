#pragma once

#include "BitBoard.h"
#include "GeneratingMagics.h"


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

	U64 zobrist;
	int depth;
	Flag flag;
	int score;
};


// main transposition table class
class TranspositionTable {
public:
	static constexpr size_t hash_size = 0x400000;
	static constexpr int no_score = std::numeric_limits<int>::min();
	static constexpr HashEntry empty_entry = { 0, 0, HashEntry::Flag::HASH_EXACT, no_score };

	TranspositionTable() = default;

	int read(int alpha, int beta, int _depth);

	void write(int _depth, int _score, HashEntry::Flag _flag);
	void clear();
private:
	std::array<HashEntry, hash_size> htab;
};

extern TranspositionTable tt;

inline bool HashEntry::isValid(int _score) noexcept {
	return _score != TranspositionTable::no_score;
}