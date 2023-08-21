#pragma once

#include "LegalityTest.h"
#include "MoveSystem.h"

// whole displaying legal moves systems namespace
namespace displays {
	
	namespace dResources {

		// class template for displaying legal moves of specific piece of given color
		template <enumPiece PC, enumSide SIDE>
		class displayerLegalMoves {
		public:
			void operator()(U64 pinned) = delete;
		};

		template <enumSide SIDE>
		class displayerLegalMoves<KING, SIDE> {
		public:
			// check king's surrounding squares and display safe moves for king
			void operator()(U64) {
				int sq, king_sq = getLS1BIndex(BBs[nWhiteKing + SIDE]);
				U64 king_move = cKingAttacks[SIDE][king_sq];

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
		};

		template <enumSide SIDE>
		class displayerLegalMoves<PAWN, SIDE> {
		public:
			void operator()(U64 pinned) {

				// double pawn pushes
				const U64 double_push_pawns = BBs[nWhitePawn + SIDE] &
					(((SIDE == WHITE) * Constans::r2_rank) | ((SIDE == BLACK) * Constans::r7_rank)) & ~pinned;
				U64 after_double_push = PawnPushes::doublePushPawn<SIDE>(double_push_pawns, BBs[nEmpty]);
				int after_sq, offset = 8 * (2 - (SIDE == WHITE) * 4);

				while (after_double_push) {
					after_sq = getLS1BIndex(after_double_push);
					std::cout << index_to_square[after_sq + offset] << index_to_square[after_sq] << ',';
					after_double_push &= after_double_push - 1;
				}

				// single pawn pushes
				U64 after_push = PawnPushes::singlePushPawn<SIDE>(BBs[nWhitePawn + SIDE] & ~pinned, BBs[nEmpty]);
				offset = 8 * (1 - (SIDE == WHITE) * 2);

				while (after_push) {
					after_sq = getLS1BIndex(after_double_push);
					std::cout << index_to_square[after_sq + offset] << index_to_square[after_sq] << ',';
					after_push &= after_push - 1;
				}
			}
		};

		template <enumSide SIDE>
		class displayerLegalMoves<KNIGHT, SIDE> {
		public:
			void operator()(U64 pinned) {
				U64 knights = BBs[nWhiteKnight + SIDE] & ~pinned,
					attacks;
				int sq;

				// for every knight position loop through his attack squares
				while (knights) {
					sq = getLS1BIndex(knights);
					attacks = cKnightAttacks[SIDE][sq] & ~BBs[nWhite + SIDE];

					while (attacks) {
						std::cout << index_to_square[sq] << index_to_square[getLS1BIndex(attacks)] << ',';
						attacks &= attacks - 1;
					}

					knights &= knights - 1;
				}
			}
		};

	}

	// general function template called every time to display legal moves of proper piece
	template <enumPiece PC, enumSide SIDE>
	void displayLegalMoves(U64 pinned) {
		dResources::displayerLegalMoves<PC, SIDE>()(pinned);
	}

}

// displays only legal moves of given player
template <enumSide SIDE>
void displayLegalMoves() {
	const int king_sq = getLS1BIndex(BBs[nWhiteKing + SIDE]);
	const U64 king_attackers = attackTo<SIDE>(king_sq);
	const bool is_check = king_attackers;

	std::cout << "Possible moves: ";

	// double check case - only king moves to non-attacked squares are permitted
	if (is_check and isDoubleChecked(king_attackers)) {
		displays::displayLegalMoves<KING, SIDE>(eU64);
		return;
	}

	const U64 pinned = pinnedPiece<SIDE>(king_sq);

	// single check case - 
	// 1) move king to non-attacked squares,
	// 2) capture checking piece by unpinned piece or
	// 3) block check by moving unpinned piece, except king of course
	//    only if checking piece is sliding piece
	if (is_check) {
		// 1*
		displays::displayLegalMoves<KING, SIDE>(eU64);
			
		// 2* - there is only one attacker in such case
		const int attacker_sq = getLS1BIndex(king_attackers);
			// exclude pinned pieces
		U64 own_capture = attackTo<!SIDE>(attacker_sq) & ~pinned;

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

	// not in check scenario -
	displays::displayLegalMoves<PAWN, SIDE>(pinned);
	displays::displayLegalMoves<KNIGHT, SIDE>(pinned);
}
