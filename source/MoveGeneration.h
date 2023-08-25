#pragma once

#include "LegalityTest.h"
#include "MoveSystem.h"


namespace {
	namespace pinData {
		// cache table to store inbetween paths of king and his x-ray attackers
		// for pinned pieces, since such pinned piece can only move through their inbetween path
		std::array<U64, 64> inbetween_blockers;
		U64 pinned;
	}
}


/* move list resources -
 * move item consists of:
 * 0000 0000 0000 0011 1111 origin square   - 6 bits (0..63)
 * 0000 0000 1111 1100 0000 target square   - 6 bits (0..63)
 * 0000 0111 0000 0000 0000 promoted piece  - 3 bits (0..5)
 * 0000 1000 0000 0000 0000 double push flag - 1 bit
 * 0001 0000 0000 0000 0000 capture flag     - 1 bit
 * 0010 0000 0000 0000 0000 en passant flag  - 1 bit
 * 0100 0000 0000 0000 0000 castle flag      - 1 bit
 *						    19 bits of total:
 *						    uint32_t is enought for storing move data
 */


namespace {
	namespace MoveItem {

		// enum classes used for separating bit masks and encoding type
		// masks for extracting relevant bits from 32-bit int data
		enum class iMask {
			ORIGIN = 0x3F,
			TARGET = 0xFC0,
			PROMOTED = 0x7000,
			DOUBLE_PUSH_F = 0x8000,
			CAPTURE_F = 0x10000,
			EN_PASSANT_F = 0x20000,
			CASTLE_F = 0x40000
		};

		// encoding mode
		enum class encodeType {
			QUIET, 
			CAPTURE,
			DOUBLE_PUSH,
		};


		class iMove {
		public:
			void operator=(uint32_t data) noexcept {
				cmove = data;
			}

			template <iMask MASK>
			static inline uint32_t getMask() noexcept {
				return (cmove & MASK);
			}
		private:
			uint32_t cmove;
		};


		// claim move item states and return encoded move item ready to save in move list

		// decide about capture flag relevancy
		inline uint32_t encodeQuietCapture(int origin, int target, bool capture) noexcept {
			return (capture << 16) | (target << 6) | origin;
		}

		// direct function template as a parameter of specific encode mode
		template <encodeType eT>
		uint32_t encode(int origin, int target);

		template <>
		inline uint32_t encode<encodeType::CAPTURE>(int origin, int target) noexcept {
			return (1 << 16) | (target << 6) | origin;
		}

		template <>
		inline uint32_t encode<encodeType::QUIET>(int origin, int target) noexcept {
			return (target << 6) | origin;
		}

		template <>
		inline uint32_t encode<encodeType::DOUBLE_PUSH>(int origin, int target) noexcept {
			return (1 << 15) | (target << 6) | origin;
		}
	}


	namespace MoveList {

		// maximum possible playable moves 
		static constexpr int MAX_PLAY_MOVES = 256;

		// main move list storage array of directly defined size
		std::array<MoveItem::iMove, MAX_PLAY_MOVES> move_list;

		// count of generated moves
		int count;
	}

} // namespace


// move generation resources
namespace MoveGenerator {

	// generator functions based on PINNED piece flag -
	// excluded pieces template parameters don't meet conditions of these functions.

	template <enumPiece PC, enumSide SIDE, class =
		std::enable_if<PC != PAWN and PC != KING>>
	void unPinnedGenerate(U64 legal_blocks) {

		// unpinned pieces only
		U64 pieces = BBs[pieceToPieceIndex<PC>() + SIDE] & ~pinData::pinned,
			attacks;
		int origin, target;

		while (pieces) {
			origin = popLS1B(pieces);
			attacks = (attack<PC>(BBs[nOccupied], origin) & ~BBs[nWhite + SIDE])
				& legal_blocks;

			while (attacks) {
				target = popLS1B(attacks);
				MoveList::move_list[MoveList::count++] =
					MoveItem::encodeQuietCapture(origin, target, bitU64(target) & BBs[nBlack - SIDE]);
			}
		}
	}

	template <enumPiece PC, enumSide SIDE, class =
		std::enable_if<PC != PAWN and PC != KING and PC != KNIGHT>>
	void pinnedGenerate() {

		// pinned pieces only
		U64 pieces = BBs[pieceToPieceIndex<PC>() + SIDE] & pinData::pinned,
			attacks;
		int origin, target;

		while (pieces) {
			origin = popLS1B(pieces);
			attacks = (attack<PC>(BBs[nOccupied], origin) & ~BBs[nWhite + SIDE])
				& pinData::inbetween_blockers[origin];

			while (attacks) {
				target = popLS1B(attacks);
				MoveList::move_list[MoveList::count++] =
					MoveItem::encodeQuietCapture(origin, target, bitU64(target) & BBs[nBlack - SIDE]);
			}
		}
	}


	// common moves generator for sliding pieces - 
	// just processing proper pinned and unpinned sliders
	template <enumPiece PC, enumSide SIDE>
	void generateOf(U64 legal_blocks) {
		unPinnedGenerate<PC, SIDE>(legal_blocks);
		pinnedGenerate  <PC, SIDE>();
	}


	// pawn moves generation helper function template
	template <MoveItem::encodeType eT>
	void pawnMovesGenerateHelper(int bit_shift, U64 check_mask, int origin, int target) {
		if (bitU64(bit_shift) & check_mask) {
			MoveList::move_list[MoveList::count++] =
				MoveItem::encode<eT>(origin, target);
		}
	}

	// pawn legal moves generator function:
	// assuming there is no check
	template <enumSide SIDE>
	void nGenerateOfPawns() {

		// calculate offset for origin squares
		constexpr int single_off = SIDE == WHITE ? Compass::sout : Compass::nort,
			double_off = single_off * 2,
			west_att_off = SIDE == WHITE ? Compass::soWe : Compass::noWe,
			east_att_off = SIDE == WHITE ? Compass::soEa : Compass::noEa;

		// pawn moves except pinned pawns
		int target;
		U64 single_push =
			PawnPushes::singlePushPawn<SIDE>(BBs[nWhitePawn + SIDE] & ~pinData::pinned, BBs[nEmpty]),
			double_push =
			PawnPushes::singlePushPawn<SIDE>(single_push, BBs[nEmpty]),
			captures =
			PawnAttacks::anyAttackPawn<SIDE>(BBs[nWhitePawn + SIDE] & ~pinData::pinned, BBs[nBlack - SIDE]);

		while (single_push) {
			target = popLS1B(single_push);
			MoveList::move_list[MoveList::count++] =
				MoveItem::encode<MoveItem::encodeType::QUIET>(target + single_off, target);
		}

		while (double_push) {
			target = popLS1B(double_push);
			MoveList::move_list[MoveList::count++] =
				MoveItem::encode<MoveItem::encodeType::DOUBLE_PUSH>(target + double_off, target);
		}

		while (captures) {
			target = popLS1B(captures);

			// separate function template help with checking occupied squared by pawn belonging
			pawnMovesGenerateHelper<MoveItem::encodeType::CAPTURE>(
				target + west_att_off, BBs[nWhite + SIDE], target + west_att_off, target
			);
			pawnMovesGenerateHelper<MoveItem::encodeType::CAPTURE>(
				target + east_att_off, BBs[nWhite + SIDE], target + east_att_off, target
			);
		}

		// pinned pawn - only pushes
		single_push =
			PawnPushes::singlePushPawn<SIDE>(BBs[nWhitePawn + SIDE] & pinData::pinned, BBs[nEmpty]);
		double_push =
			PawnPushes::singlePushPawn<SIDE>(single_push, BBs[nEmpty]);

		while (single_push) {
			target = popLS1B(single_push);
			pawnMovesGenerateHelper<MoveItem::encodeType::QUIET>(
				target, pinData::inbetween_blockers[target + single_off], target + single_off, target
			);
		}

		while (double_push) {
			target = popLS1B(double_push);
			pawnMovesGenerateHelper<MoveItem::encodeType::DOUBLE_PUSH>(
				target, pinData::inbetween_blockers[target + double_off], target + double_off, target
			);
		}
	}

	// pawn moves generator function while there is a check in the background
	template <enumSide SIDE>
	void cGenerateOfPawns(U64 legal_squares) {

		// calculate offset for origin squares
		constexpr int single_off = SIDE == WHITE ? Compass::sout : Compass::nort,
			double_off = single_off * 2,
			west_att_off = SIDE == WHITE ? Compass::soWe : Compass::noWe,
			east_att_off = SIDE == WHITE ? Compass::soEa : Compass::noEa;

		int target;
		U64 single_push =
				PawnPushes::singlePushPawn<SIDE>(BBs[nWhitePawn + SIDE] & ~pinData::pinned, BBs[nEmpty]),
			double_push =
				PawnPushes::singlePushPawn<SIDE>(single_push, BBs[nEmpty]),
			captures =
				PawnAttacks::anyAttackPawn<SIDE>(BBs[nWhitePawn + SIDE] & ~pinData::pinned, BBs[nBlack - SIDE]);

		// processing pawn pushes, then possible captures
		single_push &= legal_squares;
		while (single_push) {
			target = popLS1B(single_push);
			MoveList::move_list[MoveList::count++] =
				MoveItem::encode<MoveItem::encodeType::QUIET>(target + single_off, target);
		}

		double_push &= legal_squares;
		while (double_push) {
			target = popLS1B(double_push);
			MoveList::move_list[MoveList::count++] =
				MoveItem::encode<MoveItem::encodeType::DOUBLE_PUSH>(target + double_off, target);
		}

		captures &= legal_squares;
		while (captures) {
			target = popLS1B(captures);

			// checking occupied squared by pawn belonging
			pawnMovesGenerateHelper<MoveItem::encodeType::CAPTURE>(
				target + west_att_off, BBs[nWhite + SIDE], target + west_att_off, target
			);
			pawnMovesGenerateHelper<MoveItem::encodeType::CAPTURE>(
				target + east_att_off, BBs[nWhite + SIDE], target + east_att_off, target
			);
		}
	}


	// generate legal moves for pieces of specific color
	template <enumSide SIDE>
	void generateLegalOf() {
		const int king_sq = getLS1BIndex(BBs[nWhiteKing + SIDE]);
		const U64 king_att = attackTo<SIDE>(king_sq),
			checkers = attackTo<SIDE>(king_sq);

		// double check case skipping - only king moves to non-attacked squares are permitted
		// when there is double check situation
		if (!isDoubleChecked(king_att)) {
			const bool check = king_att;
			pinData::pinned = pinnedPiece<SIDE>(king_sq);
			int checker_sq = popLS1B(checkers);
			U64 legal_blocks = check ?
				(inBetween(king_sq, checker_sq) | bitU64(checker_sq)) : ~eU64;

			// use bit index as an access index for inbetween cache
			U64 pinners = pinnersPiece<SIDE>(king_sq), ray;
			int pinner_sq;

			while (pinners) {
				pinner_sq = popLS1B(pinners);
				ray = inBetween(king_sq, pinner_sq) | bitU64(pinner_sq);
				pinData::inbetween_blockers[getLS1BIndex(ray & pinData::pinned)] = ray;
			}

			check ? cGenerateOfPawns<SIDE>(legal_blocks) : nGenerateOfPawns<SIDE>();
			// exclude pinned knights directly, since pinned knight can't move anywhere
			unPinnedGenerate<KNIGHT, SIDE>(legal_blocks);
			generateOf<BISHOP, SIDE>(legal_blocks);
			generateOf<ROOK, SIDE>(legal_blocks);
			generateOf<QUEEN, SIDE>(legal_blocks);
		}

		// generate king legal moves
		U64 king_moves = cKingAttacks[king_sq] & ~BBs[nWhite + SIDE];
		int target;

		// loop throught all king possible moves
		while (king_moves) {
			target = popLS1B(king_moves);

			// checking for an unattacked position after a move
			if (!isSquareAttacked<SIDE, KING>(king_sq)) {
				MoveList::move_list[MoveList::count++] =
					MoveItem::encodeQuietCapture(king_sq, target, bitU64(target) & BBs[nBlack - SIDE]);
			}
		}
	}

	// main generation function, generating all the legal moves for all the turn-to-move pieces
	inline void generateLegalMoves() {
		MoveList::count = 0;
		return gState::turn == WHITE ? generateLegalOf<WHITE>() : generateLegalOf<BLACK>();
	}

} // namespace MoveGenerator