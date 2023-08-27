#include "MoveGeneration.h"


namespace MoveGenerator {

	// generator functions based on PINNED piece flag -
	// excluded pieces template parameters don't meet conditions of these functions.

	template <enumPiece PC, enumSide SIDE, class =
		std::enable_if<PC != PAWN and PC != KING>>
	void unPinnedGenerate(U64 legal_blocks) {

		// unpinned pieces only
		U64 pieces = BBs[bbsIndex<PC>() + SIDE] & ~pinData::pinned,
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
		U64 pieces = BBs[bbsIndex<PC>() + SIDE] & pinData::pinned,
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


	namespace PawnHelpers {

		// pinned pawn moves generation helper function template
		template <MoveItem::encodeType eT, int Offset>
		inline void pPawnMovesGenHelper(int target) {
			if (bitU64(target) & pinData::inbetween_blockers[target + Offset]) {
				MoveList::move_list[MoveList::count++] =
					MoveItem::encode<eT>(target + Offset, target);
			}
		}

		// pawn captures helper function template
		template <MoveItem::encodeType eT, enumSide SIDE, int Offset, class =
			std::enable_if_t<eT == MoveItem::encodeType::CAPTURE>>
		inline void pawnCapturesGenHelper(int target) {
			if (bitU64(target + Offset) & BBs[nBlack - SIDE]) {
				MoveList::move_list[MoveList::count++] =
					MoveItem::encode<eT>(target + Offset, target);
			}
		}

		// special horizontal pin test function template for en passant capture scenario
		template <MoveItem::encodeType eT, enumSide SIDE, int Offset, int EP_Offset, class =
			std::enable_if_t<eT == MoveItem::encodeType::EN_PASSANT>>
		void pawnEPGenHelper() {

			// no legal en passant capture is possible
			if (!(bitU64(gState::ep_sq + Offset) & BBs[nWhitePawn + SIDE]))
				return;

			// horizontal pin test
			const U64 horizon = attack<ROOK>(BBs[nOccupied] & ~bitU64(gState::ep_sq), gState::ep_sq + Offset);
			if ((horizon & BBs[nWhiteKing + SIDE]) and (horizon & (BBs[nBlackRook - SIDE] | BBs[nBlackQueen - SIDE])))
				return;

			MoveList::move_list[MoveList::count++] =
				MoveItem::encode<eT>(gState::ep_sq + Offset, gState::ep_sq + EP_Offset);
		}

	}


	// pawn legal moves generator function:
	// assuming there is no check
	template <enumSide SIDE>
	void nGenerateOfPawns() {

		// calculate offset for origin squares
		static constexpr int single_off = SIDE == WHITE ? Compass::sout : Compass::nort,
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

		// following square of en passant pawn, which is occupied after perfoming a en passant capture move
		const int capture_ep_sq = gState::ep_sq + single_off;

		// check en passant capture possibility and find origin square
		PawnHelpers::pawnEPGenHelper<MoveItem::encodeType::EN_PASSANT, SIDE, Compass::west, single_off>();
		PawnHelpers::pawnEPGenHelper<MoveItem::encodeType::EN_PASSANT, SIDE, Compass::east, single_off>();

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

		// pawn captures
		while (captures) {
			target = popLS1B(captures);

			// separate function template help with checking occupied squared by pawn belonging
			PawnHelpers::pawnCapturesGenHelper<MoveItem::encodeType::CAPTURE, SIDE, west_att_off>(target);
			PawnHelpers::pawnCapturesGenHelper<MoveItem::encodeType::CAPTURE, SIDE, east_att_off>(target);
		}

		// pinned pawn - only pushes
		single_push =
			PawnPushes::singlePushPawn<SIDE>(BBs[nWhitePawn + SIDE] & pinData::pinned, BBs[nEmpty]);
		double_push =
			PawnPushes::singlePushPawn<SIDE>(single_push, BBs[nEmpty]);

		while (single_push) {
			target = popLS1B(single_push);
			PawnHelpers::pPawnMovesGenHelper<MoveItem::encodeType::QUIET, single_off>(target);
		}

		while (double_push) {
			target = popLS1B(double_push);
			PawnHelpers::pPawnMovesGenHelper<MoveItem::encodeType::DOUBLE_PUSH, double_off>(target);
		}
	}

	// pawn moves generator function while there is a check in the background
	template <enumSide SIDE>
	void cGenerateOfPawns(U64 legal_squares) {

		// calculate offset for origin squares
		static constexpr int single_off = SIDE == WHITE ? Compass::sout : Compass::nort,
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
			PawnHelpers::pawnCapturesGenHelper<MoveItem::encodeType::CAPTURE, SIDE, west_att_off>(target);
			PawnHelpers::pawnCapturesGenHelper<MoveItem::encodeType::CAPTURE, SIDE, east_att_off>(target);
		}

		// en passant case - 
		// permitted only when checker is a pawn possible to capture using en passant rule
		if (getLS1BIndex(legal_squares) != gState::ep_sq)
			return;

		if (bitU64(gState::ep_sq + Compass::west) & BBs[nWhitePawn + SIDE]) {
			MoveList::move_list[MoveList::count++] =
				MoveItem::encode<MoveItem::encodeType::EN_PASSANT>(gState::ep_sq + Compass::west, gState::ep_sq + single_off);
		}
		if (bitU64(gState::ep_sq + Compass::east) & BBs[nWhitePawn + SIDE]) {
			MoveList::move_list[MoveList::count++] =
				MoveItem::encode<MoveItem::encodeType::EN_PASSANT>(gState::ep_sq + Compass::east, gState::ep_sq + single_off);
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
		U64 king_moves = cking_attacks[king_sq] & ~BBs[nWhite + SIDE];
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

	void generateLegalMoves() {
		MoveList::count = 0;
		return gState::turn == WHITE ? generateLegalOf<WHITE>() : generateLegalOf<BLACK>();
	}

} // namespace MoveGenerator

