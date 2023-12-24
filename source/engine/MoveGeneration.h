#pragma once

#include "LegalityTest.h"
#include "MoveSystem.h"
#include "MoveItem.h"


// storage for generated moves
struct MoveList {
public:
	MoveList() noexcept
		: move_list{}, it(move_list.begin()) {}
	MoveList(const MoveList& ml)
		: move_list(ml.move_list), it(move_list.begin() + ml.size()) {}

	inline void operator=(const MoveList& ml) {
		move_list = ml.move_list;
		it = move_list.begin() + ml.size();
	}

	// maximum possible playable moves at one turn
	static constexpr int MAX_PLAY_MOVES = 256;
	using iterator = std::array<MoveItem::iMove, MAX_PLAY_MOVES>::iterator;
	using c_iterator = std::array<MoveItem::iMove, MAX_PLAY_MOVES>::const_iterator;

	// main move list storage array of directly defined size
	std::array<MoveItem::iMove, MAX_PLAY_MOVES> move_list;

	// ending iterator of generated moves
	iterator it;

	inline iterator begin() noexcept { return move_list.begin(); }
	inline iterator end() noexcept { return it; }

	inline c_iterator begin() const noexcept { return move_list.cbegin(); }
	inline c_iterator end() const { return static_cast<c_iterator>(it); }

	inline const MoveItem::iMove& operator[](size_t i) const { return move_list[i]; }
	inline MoveItem::iMove& operator[](size_t i) { return move_list[i]; }

	inline std::size_t size() const { return std::distance(move_list.begin(), static_cast<c_iterator>(it)); }
	inline std::size_t size() { return std::distance(move_list.begin(), it); }
};


// move generation resources
namespace MoveGenerator {

	// generation type for move generator
	enum GenType {
		LEGAL, CAPTURES, TACTICAL
	};

	namespace Analisis {

		// open this perft test passing compile-time depth manually
		template <int Depth>
		void perft();

		// perft function testing whether move generator is bug-free
		void perftDriver(int depth);

	}

	// main generation function, generating all the legal moves for all the turn-to-move pieces
	template <GenType gType>
	void generateLegalMoves(MoveList& ml);
} 


// make move resources
namespace MovePerform {

	// decode move and perform move
	void makeMove(const MoveItem::iMove& move);

	// perform null move
	void makeNull();

	// unmake move - copy-make approach
	void unmakeMove(const BitBoardsSet& bbs_cpy, const gState& states_cpy);

	// unmake move - copy-make approach
	void unmakeNull(U64 hash_cpy, int ep_cpy);
}