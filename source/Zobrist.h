#pragma once

#include "BitBoard.h"
#include "GeneratingMagics.h"
#include "BitBoardsSet.h"
#include "string"


struct Zobrist {
	Zobrist();

	// calculate Zobrist key for current position
	void generateKey();

	// 12 - number of all pieces (nWhitePawn..nBlackKing)
	cexpr::CexprArr<true, U64, 12, 64> piece_keys;
	cexpr::CexprArr<false, U64, 16> castle_keys;
	cexpr::CexprArr<false, U64, 64> enpassant_keys;
	U64 side_key;

	// raw Zobrist keys
	U64 key;
};

// forward declaration
extern Zobrist hash;

// single entry in hash table structure
struct HashEntry {
	static constexpr int no_score = std::numeric_limits<int>::min();

	// hashing flags
	enum class Flag {
		HASH_EXACT,
		HASH_ALPHA,
		HASH_BETA
	};

	static inline bool isValid(int g_score) noexcept;

	U64 zobrist;
	uint8_t depth, age;
	Flag flag;
	int score;
};

inline constexpr size_t operator"" _MB(ULL mb) noexcept {
	return mb * 0x100000;
}


// main transposition table class
class TranspositionTable {
public:
	static constexpr size_t default_size = 4_MB,
		min_size = 1_MB, max_size = 128_MB;
	static constexpr HashEntry empty_entry = { 0, 0, 0, HashEntry::Flag::HASH_EXACT, HashEntry::no_score };
	
	inline TranspositionTable() {
		setSize(default_size / 1_MB);
	};

	static inline std::string hashInfo() {
		return "option name Hash type spin default "
			+ std::to_string(default_size / 1_MB)
			+ " min " + std::to_string(min_size / 1_MB) 
			+ " max " + std::to_string(max_size / 1_MB);
	}

	inline std::string currSizeInfo() {
		return "hash size " + std::to_string(hash_size / 1_MB);
	}

	int read(int alpha, int beta, int g_depth, int ply);
	void write(int g_depth, int g_score, HashEntry::Flag g_flag, int ply);

	inline void clear() {
		std::fill(htab.begin(), htab.end(), empty_entry);
	}

	inline void setSize(size_t mb_size) {
		hash_size = 1_MB * mb_size;

		hash_size = std::max(hash_size, min_size);
		hash_size = std::min(hash_size, max_size);

		htab.resize(hash_size);
		clear();
	}

	// decrease entries age and delete too old entries 
	inline void decreaseAge() {
		for (auto& entry : htab) {
			if (HashEntry::isValid(entry.score) and ++entry.age >= 3) 
				entry = empty_entry;
		}
	}

private:
	size_t hash_size;
	std::vector<HashEntry> htab;
};

extern TranspositionTable tt;

inline bool HashEntry::isValid(int g_score) noexcept {
	return g_score != no_score;
}


// list of keys of last positions
class RepetitionTable {
public:
	static constexpr size_t tab_size = 0x200;
	RepetitionTable() = default;

	bool isRepetition();
	void posRegister() noexcept;

	inline void clear() noexcept {
		count = 0;
	}

	int count;
private:
	std::array<U64, tab_size> tab;
};

extern RepetitionTable rep_tt;

inline void RepetitionTable::posRegister() noexcept {
	assert(count < tab_size && "Repetition table index overflow");
	tab[count++] = hash.key;
}


inline bool RepetitionTable::isRepetition() {
	for (int i = count - 1; i >= 0; i--)
		if (tab[i] == hash.key) return true;
	return false;
}