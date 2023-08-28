#pragma once

#include "MoveGeneration.h"

/* DEPRECATED HEADER FILE */

// whole displaying legal moves systems namespace
namespace displays {

	namespace dResources {

		// class template for displaying legal moves of specific piece of given color
		// condition: functions assume there is no check on the board but support pin system
		template <enumPiece PC, enumSide SIDE>
		class displayerLegalMoves {
		public:
			// common move displayer for sliding pieces
			void operator()() {

				// unpinned pieces
				U64 pieces = BBs[pieceToPieceIndex<PC>() + SIDE] & ~pinData::pinned,
					attacks;
				int sq;

				while (pieces) {
					sq = getLS1BIndex(pieces);
					attacks = attack<PC>(BBs[nOccupied], sq) & ~BBs[nWhite + SIDE];

					while (attacks) {
						std::cout << index_to_square[sq] << index_to_square[getLS1BIndex(attacks)] << ',';
						attacks &= attacks - 1;
					}

					pieces &= pieces - 1;
				}

				// pinned pieces
				pieces = BBs[pieceToPieceIndex<PC>() + SIDE] & pinData::pinned;

				while (pieces) {
					sq = getLS1BIndex(pieces);
					attacks = (attack<PC>(BBs[nOccupied], sq) & ~BBs[nWhite + SIDE]) & pinData::inbetween_blockers[sq];

					while (attacks) {
						std::cout << index_to_square[sq] << index_to_square[getLS1BIndex(attacks)] << ',';
						attacks &= attacks - 1;
					}

					pieces &= pieces - 1;
				}
			}
		};

		// manualy defined displayers for leaper pieces:

		// displayer class for legal king moves
		template <enumSide SIDE>
		class displayerLegalMoves<KING, SIDE> {
		public:
			// check king's surrounding squares and display safe moves for king
			void operator()() {
				int sq, king_sq = getLS1BIndex(BBs[nWhiteKing + SIDE]);
				U64 king_move = cKingAttacks[king_sq] & ~BBs[nWhite + SIDE];

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

		// ... legal pawn moves
		template <enumSide SIDE>
		class displayerLegalMoves<PAWN, SIDE> {
		public:
			void operator()() {

				// calculate offset for origin squares
				constexpr int double_offset = 8 * (2 - (SIDE == WHITE) * 4),
					single_offset = 8 * (1 - (SIDE == WHITE) * 2);

				// double pawn pushes
				U64 double_push_pawns = BBs[nWhitePawn + SIDE] &
					(((SIDE == WHITE) * Constans::r2_rank) | ((SIDE == BLACK) * Constans::r7_rank));
				U64 after_double_push = PawnPushes::doublePushPawn<SIDE>(double_push_pawns & ~pinData::pinned, BBs[nEmpty]);
				int after_sq;

				while (after_double_push) {
					after_sq = getLS1BIndex(after_double_push);
					std::cout << index_to_square[after_sq + double_offset] << index_to_square[after_sq] << ',';
					after_double_push &= after_double_push - 1;
				}

				// single pawn pushes
				U64 after_push = PawnPushes::singlePushPawn<SIDE>(BBs[nWhitePawn + SIDE] & ~pinData::pinned, BBs[nEmpty]);

				while (after_push) {
					after_sq = getLS1BIndex(after_push);
					std::cout << index_to_square[after_sq + single_offset] << index_to_square[after_sq] << ',';
					after_push &= after_push - 1;
				}

				// double pushes for pinned pawns
				double_push_pawns = double_push_pawns & pinData::pinned;
				after_double_push = PawnPushes::doublePushPawn<SIDE>(double_push_pawns, BBs[nEmpty]);

				while (after_double_push) {
					after_sq = getLS1BIndex(after_double_push);
					if (bitU64(after_sq) & pinData::inbetween_blockers[after_sq + double_offset])
						std::cout << index_to_square[after_sq + double_offset] << index_to_square[after_sq] << ',';
					after_double_push &= after_double_push - 1;
				}

				// single pushes for pinned pawns
				after_push = PawnPushes::singlePushPawn<SIDE>(BBs[nWhitePawn + SIDE] & pinData::pinned, BBs[nEmpty]);

				while (after_push) {
					after_sq = getLS1BIndex(after_push);
					if (bitU64(after_sq) & pinData::inbetween_blockers[after_sq + single_offset])
						std::cout << index_to_square[after_sq + single_offset] << index_to_square[after_sq] << ',';
					after_push &= after_push - 1;
				}
			}
		};

		// ... legal knight moves
		template <enumSide SIDE>
		class displayerLegalMoves<KNIGHT, SIDE> {
		public:
			void operator()() {

				// exclude pinned knights directly, since pinned knight can move
				U64 knights = BBs[nWhiteKnight + SIDE] & ~pinData::pinned,
					attacks;
				int sq;

				// for every knight position loop through his attack squares
				// check whether attacked square is empty or occupied by opponent piece
				while (knights) {
					sq = getLS1BIndex(knights);
					attacks = cKnightAttacks[sq] & ~BBs[nWhite + SIDE];

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
	void displayLegalMoves() {
		dResources::displayerLegalMoves<PC, SIDE>()();
	}

} // namespace displays


// displays only legal moves of given player
template <enumSide SIDE>
void displayLegalMoves() {
	const int king_sq = getLS1BIndex(BBs[nWhiteKing + SIDE]);
	const U64 king_attackers = attackTo<SIDE>(king_sq);
	const bool is_check = king_attackers;

	std::cout << "Possible moves:  " << std::endl;

	// double check case - only king moves to non-attacked squares are permitted
	if (is_check and isDoubleChecked(king_attackers)) {
		displays::displayLegalMoves<KING, SIDE>();
		return;
	}

	pinData::pinned = pinnedPiece<SIDE>(king_sq);
	const int attacker_sq = getLS1BIndex(king_attackers);

	// single check case - 
	// 1) move king to non-attacked squares,
	// 2) capture checking piece by unpinned piece or
	// 3) block check by moving unpinned piece, except king of course
	//    only if checking piece is a sliding piece
	if (is_check) {
		// 1*
		displays::displayLegalMoves<KING, SIDE>();

		// 2* - there is only one attacker in such case
		// exclude pinned pieces
		U64 own_capture = attackTo<!SIDE>(attacker_sq) & ~pinData::pinned;

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
			blockers = attackTo<!SIDE, KING>(block_sq) & ~pinData::pinned;

			while (blockers) {
				std::cout << index_to_square[getLS1BIndex(blockers)] << index_to_square[block_sq] << ',';
				blockers &= blockers - 1;
			}

			block_squares &= block_squares - 1;
		}

		return;
	}

	// not in check scenario -
	int pinner_sq;
	U64 ray,
		pinners = pinnersPiece<SIDE>(king_sq);

	// use bit index as an access index for inbetween cache
	while (pinners) {
		pinner_sq = getLS1BIndex(pinners);
		ray = inBetween(king_sq, pinner_sq) | bitU64(pinner_sq);
		pinData::inbetween_blockers[getLS1BIndex(ray & pinData::pinned)] = ray;
		pinners &= pinners - 1;
	}

	// display legal move beyond check of leaper pieces:
	displays::displayLegalMoves<KING, SIDE>();
	displays::displayLegalMoves<PAWN, SIDE>();
	displays::displayLegalMoves<KNIGHT, SIDE>();

	// sliding pieces left
	displays::displayLegalMoves<BISHOP, SIDE>();
	displays::displayLegalMoves<ROOK, SIDE>();
	displays::displayLegalMoves<QUEEN, SIDE>();
}