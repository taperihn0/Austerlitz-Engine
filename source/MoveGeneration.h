#pragma once

#include "LegalityTest.h"

namespace {

	// check king's surrounding squares and display safe moves for king
	template <enumSide SIDE>
	void displaySafeMovesKing(int king_sq) {
		U64 king_move = cKingAttacks[SIDE][king_sq];
		int sq;

		// loop throught all king possible moves
		while (king_move) {
			sq = getLS1BIndex(king_move);

			// unattacked position after move
			if (!isSquareAttacked<SIDE, KING>(sq)) {
				std::cout << index_to_square[king_sq] << index_to_square[sq] << ',';
			}

			// pop least significant bit
			king_move &= king_move - 1;
		}
	}

	// displays only legal moves of given player
	template <enumSide SIDE>
	void displayLegalMoves() {
		int king_sq = getLS1BIndex(BBs[nWhiteKing + SIDE]);
		U64 king_attackers = attackTo<SIDE>(king_sq);
		bool is_check = king_attackers;

		std::cout << "Possible moves: ";

		// double check case - only king moves to non-attacked squares are permitted
		if (is_check and isDoubleChecked(king_attackers)) {
			displaySafeMovesKing<SIDE>(king_sq);
			return;
		}
		// single check case - 
		// 1) move king to non-attacked squares,
		// 2) capture checking piece by unpinned piece or
		// 3) block check by moving unpinned piece, except king of course
		//    only if checking piece is sliding piece
		else if (is_check) {
			// 1*
			displaySafeMovesKing<SIDE>(king_sq);
			
			// 2* - there is only one attacker in such case
			int attacker_sq = getLS1BIndex(king_attackers);
			U64 pinned = pinnedPiece<SIDE>(king_sq),
				// exclude pinned pieces
				own_capture = attackTo<!SIDE>(attacker_sq) & ~pinned;

			while (own_capture) {
				std::cout << index_to_square[getLS1BIndex(own_capture)] << index_to_square[attacker_sq] << ',';
				own_capture &= own_capture - 1;
			}
			
			// 3* - check for sliding piece attacker and if so, continue checking
			if ((king_attackers & BBs[nBlackPawn - SIDE])
				or (king_attackers & BBs[nBlackKnight - SIDE])) {
				return;
			}

			U64 block_squares = inBetween(attacker_sq, king_sq),
				blockers;
			int block_sq;

			while (block_squares) {
				block_sq = getLS1BIndex(block_squares);
				// search for blockers for given square, skip king blocks since 
				// king can't be a blocker piece
				blockers = attackTo<!SIDE, KING>(block_sq) & ~pinned;

				while (blockers) {
					std::cout << index_to_square[getLS1BIndex(blockers)] << index_to_square[block_sq] << ',';
					blockers &= blockers - 1;
				}

				block_squares &= block_squares - 1;
			}
			
			return;
		}

		// not in check scenario
		//  ...

	}

}