#pragma once

#include "LegalityTest.h"
#include "MoveSystem.h"
#include "MoveItem.h"


// storage for generated moves
struct MoveList {
public:
	MoveList()
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

	iterator begin() {
		return move_list.begin();
	}

	iterator end() {
		return it;
	}

	c_iterator begin() const {
		return move_list.cbegin();
	}

	c_iterator end() const {
		return static_cast<c_iterator>(it);
	}

	inline std::size_t size() const {
		return std::distance(move_list.begin(), static_cast<c_iterator>(it));
	}

	inline std::size_t size() {
		return std::distance(move_list.begin(), it);
	}
};


// move generation resources
namespace MoveGenerator {

	// generation type for move generator
	enum GenType {
		LEGAL,
		CAPTURES,
	};

	namespace Analisis {

		// loop through move list and display newly generated moves
		void populateMoveList(const MoveList& move_list);

		// open this perft test passing compile-time depth manually
		template <int Depth>
		void perft();

		// perft function testing whether move generator is bug-free
		void perftDriver(int depth);

	}

	// main generation function, generating all the legal moves for all the turn-to-move pieces
	template <GenType gType>
	MoveList generateLegalMoves();

} 


// make move resources
namespace MovePerform {

	// decode move and perform move
	void makeMove(const MoveItem::iMove& move);

	// unmake move - copy-make approach
	void unmakeMove(const BitBoardsSet& bbs_cpy, const gState& states_cpy);

}