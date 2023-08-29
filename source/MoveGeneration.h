#pragma once

#include "LegalityTest.h"
#include "MoveSystem.h"
#include "MoveItem.h"


namespace {

	namespace pinData {

		// cache table to store inbetween paths of king and his x-ray attackers
		// for pinned pieces, since such pinned piece can only move through their inbetween path
		std::array<U64, 64> inbetween_blockers;
		U64 pinned;
	}


	namespace MoveList {

		// maximum possible playable moves at one turn
		static constexpr int MAX_PLAY_MOVES = 256;

		// main move list storage array of directly defined size
		std::array<MoveItem::iMove, MAX_PLAY_MOVES> move_list;

		// count of generated moves
		int count;
	}

} // namespace


// move generation resources
namespace MoveGenerator {

	// main generation function, generating all the legal moves for all the turn-to-move pieces
	void generateLegalMoves();

	void populateMoveList();

} 