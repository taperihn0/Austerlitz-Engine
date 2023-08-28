#include "MoveGeneration.h"
#include <string>

namespace {

	template <enumPiece PC>
	U64 test(U64 occ, int sq) {
		return eU64;
	}


	template <>
	U64 test<BISHOP>(U64 occ, int sq) {
		auto res = mdata.mBishopAtt[sq][mIndexHash(occ & mTabs::rBishop[sq], mTabs::mBishop[sq], mTabs::rbBishop[sq])];
		assert(res != eU64 && "HERE");
		return res;
	}

}

namespace MoveGenerator {

	// generator functions based on PINNED piece flag -
	// excluded pieces template parameters don't meet conditions of these functions.

	template <enumPiece PC, enumSide SIDE, class>
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

	template <enumPiece PC, enumSide SIDE, class>
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

		template <bool Capture, int Offset>
		void pawnPromotionGenHelper(int target) {
			MoveList::move_list[MoveList::count++] =
				MoveItem::encodePromotion<QUEEN, Capture>(target + Offset, target);
			MoveList::move_list[MoveList::count++] =
				MoveItem::encodePromotion<ROOK,  Capture>(target + Offset, target);
			MoveList::move_list[MoveList::count++] =
				MoveItem::encodePromotion<BISHOP,Capture>(target + Offset, target);
			MoveList::move_list[MoveList::count++] =
				MoveItem::encodePromotion<KNIGHT,Capture>(target + Offset, target);
		}

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
		static constexpr U64 promote_rank_mask = SIDE == WHITE ? Constans::r8_rank : Constans::r1_rank;

		// pawn moves except pinned pawns
		int target;
		U64 single_push =
				PawnPushes::singlePushPawn<SIDE>(BBs[nWhitePawn + SIDE] & ~pinData::pinned, BBs[nEmpty]),
			double_push =
				PawnPushes::singlePushPawn<SIDE>(single_push, BBs[nEmpty]),
			captures    =
				PawnAttacks::anyAttackPawn<SIDE>(BBs[nWhitePawn + SIDE] & ~pinData::pinned, BBs[nBlack - SIDE]),
			promote_moves =
				single_push & promote_rank_mask;

		// processing pawn promotions by pushes
		while (promote_moves) {
			PawnHelpers::pawnPromotionGenHelper<false, single_off>(popLS1B(promote_moves));
		}

		// processing pawn promotions by captures
		promote_moves = captures & promote_rank_mask;
		while (promote_moves) {
			PawnHelpers::pawnPromotionGenHelper<true, single_off>(popLS1B(promote_moves));
		}

		// check en passant capture possibility and find origin square
		if (gState::ep_sq != -1) {
			PawnHelpers::pawnEPGenHelper<MoveItem::encodeType::EN_PASSANT, SIDE, Compass::west, single_off>();
			PawnHelpers::pawnEPGenHelper<MoveItem::encodeType::EN_PASSANT, SIDE, Compass::east, single_off>();
		}

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

		// calculate offset for origin squares only once for every generated function
		static constexpr int single_off = SIDE == WHITE ? Compass::sout : Compass::nort,
			double_off = single_off * 2,
			west_att_off = SIDE == WHITE ? Compass::soWe : Compass::noWe,
			east_att_off = SIDE == WHITE ? Compass::soEa : Compass::noEa;
		static constexpr U64 promote_rank_mask = SIDE == WHITE ? Constans::r8_rank : Constans::r1_rank;

		int target;
		U64 single_push =
				PawnPushes::singlePushPawn<SIDE>(BBs[nWhitePawn + SIDE] & ~pinData::pinned, BBs[nEmpty]),
			double_push =
				PawnPushes::singlePushPawn<SIDE>(single_push, BBs[nEmpty]),
			captures =
				PawnAttacks::anyAttackPawn<SIDE>(BBs[nWhitePawn + SIDE] & ~pinData::pinned, BBs[nBlack - SIDE]),
			promote_moves =
				single_push & promote_rank_mask & legal_squares;

		// processing pawn promotions by pushes
		while (promote_moves) {
			PawnHelpers::pawnPromotionGenHelper<false, single_off>(popLS1B(promote_moves));
		}

		// processing pawn promotions by captures
		promote_moves = captures & promote_rank_mask & legal_squares;
		while (promote_moves) {
			PawnHelpers::pawnPromotionGenHelper<true, single_off>(popLS1B(promote_moves));
		}

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
		const U64 checkers = attackTo<SIDE, KING>(king_sq);
		const bool check = checkers;

		// double check case skipping - only king moves to non-attacked squares are permitted
		// when there is double check situation
		if (!isDoubleChecked(checkers)) {
			pinData::pinned = pinnedPiece<SIDE>(king_sq);
			int checker_sq = check ? getLS1BIndex(checkers) : -1;
			U64 legal_blocks = check ?
				(inBetween(king_sq, checker_sq) | bitU64(checker_sq)) : UINT64_MAX;

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

		if (check)
			return;
		
		// castling: rook side and queen side
		if (gState::castle.checkLegalCastle<SIDE & ROOK>()) {
			MoveList::move_list[MoveList::count++] =
				MoveItem::encode<MoveItem::encodeType::CASTLE>(king_sq, SIDE == WHITE ? g1 : g8);
		}
		if (gState::castle.checkLegalCastle<SIDE & QUEEN>()) {
			MoveList::move_list[MoveList::count++] =
				MoveItem::encode<MoveItem::encodeType::CASTLE>(king_sq, SIDE == WHITE ? c1 : c8);
		}
	}

	void generateLegalMoves() {
		MoveList::count = 0;
		return gState::turn == WHITE ? generateLegalOf<WHITE>() : generateLegalOf<BLACK>();
	}

	void populateMoveList() {
		auto helper = [](bool mask, std::string text) {
			if (mask) {
				std::cout << text << ' ';
			}
		};

		std::cout << "MoveList.size: " << MoveList::count << std::endl << std::endl << ' ';
		
		for (int tmp, i = 0; i < MoveList::count; i++) {
			std::cout << index_to_square[MoveList::move_list[i].getMask<MoveItem::iMask::ORIGIN>()]
				<< index_to_square[MoveList::move_list[i].getMask<MoveItem::iMask::TARGET>() >> 6];

			tmp = MoveList::move_list[i].getMask<MoveItem::iMask::PROMOTION>();
			helper(tmp, std::to_string(tmp >> 12) + " promotion");
			helper(MoveList::move_list[i].getMask<MoveItem::iMask::DOUBLE_PUSH_F>(), "double push");
			helper(MoveList::move_list[i].getMask<MoveItem::iMask::CAPTURE_F>(), "capture");
			helper(MoveList::move_list[i].getMask<MoveItem::iMask::EN_PASSANT_F>(), "capture");
			helper(MoveList::move_list[i].getMask<MoveItem::iMask::CASTLE_F>(), "capture");

			std::cout << '\n' << ' ';
		}
	}

} // namespace MoveGenerator

