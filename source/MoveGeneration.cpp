#include "MoveGeneration.h"
#include "Timer.h"
#include <string>


namespace MoveGenerator {

	// generator functions based on PINNED piece flag -
	// excluded pieces template parameters don't meet conditions of these functions.
	template <enumPiece PC, enumSide SIDE, bool Pin, class =
		std::enable_if<PC != PAWN and PC != KING and (!Pin and PC == KNIGHT)>>
	void pinGenerate(U64 legal_blocks, MoveList::iterator& it) {

		// only pinned or only unpinned processing
		U64 pieces = BBs[bbsIndex<PC>() + SIDE] & (Pin ? pinData::pinned : ~pinData::pinned),
			attacks;
		int origin, target;

		while (pieces) {
			origin = popLS1B(pieces);
			attacks = (attack<PC>(BBs[nOccupied], origin) & ~BBs[nWhite + SIDE])
				& (Pin ? pinData::inbetween_blockers[origin] : legal_blocks);

			while (attacks) {
				target = popLS1B(attacks);
				*it++ = MoveItem::encodeQuietCapture<PC, SIDE>(origin, target, bitU64(target) & BBs[nBlack - SIDE]);
			}
		}
	}

	// common moves generator for sliding pieces - 
	// just processing proper pinned and unpinned sliders
	template <enumPiece PC, enumSide SIDE>
	inline void generateOf(U64 legal_blocks, MoveList::iterator& it, bool check) {
		pinGenerate<PC, SIDE, false>(legal_blocks, it);
		if (!check) pinGenerate<PC, SIDE, true>(eU64, it);
	}

	// some handy functions during generating pawn fully-legal moves
	namespace PawnHelpers {

		template <enumSide SIDE, bool Capture, int Offset>
		inline void pawnPromotionGenHelper(int target, MoveList::iterator& it) {
			*it++ = (MoveItem::encodePromotion<SIDE, QUEEN, Capture>(target + Offset, target));
			*it++ = (MoveItem::encodePromotion<SIDE, ROOK,  Capture>(target + Offset, target));
			*it++ = (MoveItem::encodePromotion<SIDE, BISHOP,Capture>(target + Offset, target));
			*it++ = (MoveItem::encodePromotion<SIDE, KNIGHT,  Capture>(target + Offset, target));
		}

		// pinned pawn moves generation helper function template
		template <MoveItem::encodeType eT, enumSide SIDE, int Offset>
		inline void pPawnMovesGenHelper(int target, MoveList::iterator& it) {
			if (bitU64(target) & pinData::inbetween_blockers[target + Offset]) {
				*it++ = (MoveItem::encode<eT>(target + Offset, target, PAWN, SIDE));
			}
		}

		// pawn captures helper function template
		template <enumSide SIDE, int Offset>
		inline void pawnCapturesGenHelper(int target, MoveList::iterator& it) {
			if ((target % 8 == 0 and (Offset == Compass::noWe or Offset == Compass::soWe)) or 
			    (target % 8 == 7 and (Offset == Compass::noEa or Offset == Compass::soEa)) or
				!(bitU64(target + Offset) & (BBs[nWhitePawn + SIDE] & ~pinData::pinned)))
				return;

			*it++ = (MoveItem::encode<MoveItem::encodeType::CAPTURE>(target + Offset, target, PAWN, SIDE));
		}

		// special horizontal pin test function template for en passant capture scenario
		template <enumSide SIDE, int Offset, int EP_Offset>
		void pawnEPGenHelper(MoveList::iterator& it) {

			// no legal en passant capture is possible
			if ((!(game_state.ep_sq % 8) and Offset == Compass::west) or 
				(game_state.ep_sq % 8 == 7 and Offset == Compass::east) or 
				!(bitU64(game_state.ep_sq + Offset) & BBs[nWhitePawn + SIDE]) or 
				bitU64(game_state.ep_sq + Offset) & pinData::pinned)
				return;

			// horizontal pin test
			const U64 horizon = attack<ROOK>(BBs[nOccupied] & ~bitU64(game_state.ep_sq), game_state.ep_sq + Offset) 
				& Constans::r_by_index[getLS1BIndex(BBs[nWhiteKing + SIDE]) / 8];
			if ((horizon & BBs[nWhiteKing + SIDE]) and (horizon & (BBs[nBlackRook - SIDE] | BBs[nBlackQueen - SIDE])))
				return;

			*it++ = MoveItem::encodeEnPassant<SIDE>(game_state.ep_sq + Offset, game_state.ep_sq - EP_Offset);
		}

		// pawn legal moves direct helper
		template <enumSide SIDE, int SingleOff, int DoubleOff, bool Check>
		struct miscellaneousHelper {
			static void process(U64, MoveList::iterator&);
		};

		template <enumSide SIDE, int SingleOff, int DoubleOff>
		struct miscellaneousHelper<SIDE, SingleOff, DoubleOff, true> {
			static void process(U64 legal_squares, MoveList::iterator& it) {

				// en passant case - 
				// permitted only when checker is a pawn possible to capture using en passant rule
				if (getLS1BIndex(legal_squares) != game_state.ep_sq)
					return;

				if (bitU64(game_state.ep_sq + Compass::west) & BBs[nWhitePawn + SIDE]) {
					*it++ = MoveItem::encodeEnPassant<SIDE>(game_state.ep_sq + Compass::west, game_state.ep_sq - SingleOff);
				}
				if (bitU64(game_state.ep_sq + Compass::east) & BBs[nWhitePawn + SIDE]) {
					*it++ = MoveItem::encodeEnPassant<SIDE>(game_state.ep_sq + Compass::east, game_state.ep_sq - SingleOff);
				}
			}
		};

		template <enumSide SIDE, int SingleOff, int DoubleOff>
		struct miscellaneousHelper<SIDE, SingleOff, DoubleOff, false> {
			static void process(U64, MoveList::iterator& it) {

				static constexpr int west_att_off = SIDE ? Compass::soWe : Compass::noWe,
					east_att_off = SIDE ? Compass::soEa : Compass::noEa;
				static constexpr U64 double_push_mask = SIDE == WHITE ? Constans::r4_rank : Constans::r5_rank;

				// pinned pawn - only pushes
				U64 single_push =
					PawnPushes::singlePushPawn<SIDE>(BBs[nWhitePawn + SIDE] & pinData::pinned, BBs[nEmpty]),
					double_push =
					PawnPushes::singlePushPawn<SIDE>(single_push, BBs[nEmpty]) & double_push_mask;
				int target;

				while (single_push) {
					target = popLS1B(single_push);
					PawnHelpers::pPawnMovesGenHelper<MoveItem::encodeType::QUIET, SIDE, SingleOff>(target, it);
				}

				while (double_push) {
					target = popLS1B(double_push);
					PawnHelpers::pPawnMovesGenHelper<MoveItem::encodeType::DOUBLE_PUSH, SIDE, DoubleOff>(target, it);
				}

				// captures
				U64 pawns = BBs[nWhitePawn + SIDE] & pinData::pinned;
				int origin;

				while (pawns) {
					origin = popLS1B(pawns);

					if (bitU64(origin + west_att_off) & BBs[nBlack - SIDE] & pinData::inbetween_blockers[origin]) {
						*it++ = MoveItem::encode<MoveItem::encodeType::CAPTURE>(origin, origin + west_att_off, PAWN, SIDE);
					}
					else if (bitU64(origin + east_att_off) & BBs[nBlack - SIDE] & pinData::inbetween_blockers[origin]) {
						*it++ = MoveItem::encode<MoveItem::encodeType::CAPTURE>(origin, origin + east_att_off, PAWN, SIDE);
					}

				}

				// check en passant capture possibility and find origin square
				if (game_state.ep_sq != -1) {
					PawnHelpers::pawnEPGenHelper<SIDE, Compass::west, SingleOff>(it);
					PawnHelpers::pawnEPGenHelper<SIDE, Compass::east, SingleOff>(it);
				}
			}
		};

	} // namespace PawnHelpers


	// pawn legal moves generator function
	// template parameter Check indicating whether to consider check
	// in the background or not
	template <enumSide SIDE, bool Check>
	void generateOfPawns(U64 legal_squares, MoveList::iterator& it) {

		// calculate offset for origin squares only once for every generated function
		static constexpr int single_off = SIDE == WHITE ? Compass::sout : Compass::nort,
			double_off = single_off * 2,
			west_att_off = SIDE == WHITE ? Compass::soWe : Compass::noWe,
			east_att_off = SIDE == WHITE ? Compass::soEa : Compass::noEa;
		static constexpr U64 promote_rank_mask = SIDE == WHITE ? Constans::r8_rank : Constans::r1_rank,
			double_push_mask = SIDE == WHITE ? Constans::r4_rank : Constans::r5_rank;

		const U64 unpinned = BBs[nWhitePawn + SIDE] & ~pinData::pinned;

		int target;
		U64 single_push =
			PawnPushes::singlePushPawn<SIDE>(unpinned, BBs[nEmpty]),
			double_push =
			PawnPushes::singlePushPawn<SIDE>(single_push, BBs[nEmpty]) & double_push_mask
			& legal_squares,
			captures =
			PawnAttacks::anyAttackPawn<SIDE>(unpinned, BBs[nBlack - SIDE])
			& legal_squares,
			promote_moves =
			single_push & legal_squares & promote_rank_mask;

		// processing pawn promotions by pushes
		while (promote_moves) {
			PawnHelpers::pawnPromotionGenHelper<SIDE, false, single_off>(popLS1B(promote_moves), it);
		}

		// processing pawn promotions by captures
		promote_moves = captures & promote_rank_mask & legal_squares;
		while (promote_moves) {
			target = popLS1B(promote_moves);

			if (bitU64(target + west_att_off) & BBs[nWhitePawn + SIDE]) {
				PawnHelpers::pawnPromotionGenHelper<SIDE, true, west_att_off>(target, it);
			}
			if (bitU64(target + east_att_off) & BBs[nWhitePawn + SIDE]) {
				PawnHelpers::pawnPromotionGenHelper<SIDE, true, east_att_off>(target, it);
			}
		}

		// processing pawn pushes, then possible captures
		// excluding promotion case - promotion scenario is processed
		single_push &= ~promote_rank_mask;
		single_push &= legal_squares;
		while (single_push) {
			target = popLS1B(single_push);
			*it++ = (MoveItem::encode<MoveItem::encodeType::QUIET>(target + single_off, target, PAWN, SIDE));
		}

		while (double_push) {
			target = popLS1B(double_push);
			*it++ = MoveItem::encode<MoveItem::encodeType::DOUBLE_PUSH>(target + double_off, target, PAWN, SIDE);
		}

		captures &= ~promote_rank_mask;
		while (captures) {
			target = popLS1B(captures);

			// checking occupied squared by pawn belonging
			PawnHelpers::pawnCapturesGenHelper<SIDE, west_att_off>(target, it);
			PawnHelpers::pawnCapturesGenHelper<SIDE, east_att_off>(target, it);
		}

		PawnHelpers::miscellaneousHelper<SIDE, single_off, double_off, Check>::process(legal_squares, it);
	}

	// generate legal moves for pieces of specific color
	template <enumSide SIDE>
	void generateLegalOf(MoveList::iterator& it) {
		const int king_sq = getLS1BIndex(BBs[nWhiteKing + SIDE]);
		const U64 checkers = attackTo<SIDE, KING>(king_sq);
		const bool check = checkers;

		// double check case skipping - only king moves to non-attacked squares are permitted
		// when there is double check situation
		if (!isDoubleChecked(checkers)) {
			pinData::pinned = pinnedPiece<SIDE>(king_sq);
			const int checker_sq = check ? getLS1BIndex(checkers) : -1;
			const U64 legal_blocks = check ?
				(inBetween(king_sq, checker_sq) | bitU64(checker_sq)) : UINT64_MAX;

			// use bit index as an access index for inbetween cache
			U64 pinners = pinnersPiece<SIDE>(king_sq), ray;
			int pinner_sq;

			while (pinners) {
				pinner_sq = popLS1B(pinners);
				ray = inBetween(king_sq, pinner_sq) | bitU64(pinner_sq);
				pinData::inbetween_blockers[getLS1BIndex(ray & pinData::pinned)] = ray;
			}

			check ? generateOfPawns<SIDE, true>(legal_blocks, it) : generateOfPawns<SIDE, false>(legal_blocks, it);
			// exclude pinned knights directly, since pinned knight can't move anywhere
			pinGenerate<KNIGHT, SIDE, false>(legal_blocks, it);
			generateOf <BISHOP, SIDE>(legal_blocks, it, check);
			generateOf <ROOK,   SIDE>(legal_blocks, it, check);
			generateOf <QUEEN,  SIDE>(legal_blocks, it, check);
		}

		
		// generate king legal moves
		U64 king_moves = cking_attacks[king_sq] & ~BBs[nWhite + SIDE];
		int target;

		// loop throught all king possible moves
		while (king_moves) {
			target = popLS1B(king_moves);

			// checking for an unattacked position after a move
			if (!isSquareAttacked<SIDE>(target)) {
				*it++ = MoveItem::encodeQuietCapture<KING, SIDE>(king_sq, target, bitU64(target) & BBs[nBlack - SIDE]);
			}
		}

		if (check)
			return;
		
		const int ksq = SIDE ? e8 : e1;

		// checking castling rights change at opponent side - white move
		if (game_state.castle.checkLegalCastle<SIDE & ROOK>()) {

			// prepare king square and proper rook square
			const int rsq = SIDE ? h8 : h1;

			// checking on king side
			if (attack<ROOK>(BBs[nOccupied], rsq) & BBs[nWhiteKing + SIDE]) {

				bool ind_f = true;

				// check whether fields between rook and king aren't attacked
				for (int sq = ksq; sq < rsq; sq++) {
					if (isSquareAttacked<SIDE>(sq)) {
						ind_f = false;
						break;
					}
				}

				if (ind_f) *it++ = MoveItem::encodeCastle<SIDE>(ksq, SIDE ? g8 : g1);
			}
		}


		// and queen side
		if (game_state.castle.checkLegalCastle<SIDE & QUEEN>()) {
			const int rsq = SIDE ? a8 : a1;

			if (attack<ROOK>(BBs[nOccupied], rsq) & BBs[nWhiteKing + SIDE]) {

				bool ind_f = true;

				for (int sq = ksq; sq > rsq; sq--) {
					if (isSquareAttacked<SIDE>(sq)) {
						ind_f = false;
						break;
					}
				}

				if (ind_f) *it++ = MoveItem::encodeCastle<SIDE>(ksq, SIDE ? c8 : c1);
			}
		}
	}

	MoveList generateLegalMoves() {
		MoveList move_list;
		game_state.turn == WHITE ? generateLegalOf<WHITE>(move_list.it) : generateLegalOf<BLACK>(move_list.it);
		return move_list;
	}

} // namespace MoveGenerator


namespace MovePerform {

	// handy functions helping getting proper piece index in bitboards set
	template <bool Side>
	enumPiece_bbs bbsIndex(uint32_t pc);

	template <>
	enumPiece_bbs bbsIndex<WHITE>(uint32_t pc) {
		return pc == PAWN ? nWhitePawn :
			pc == KNIGHT  ? nWhiteKnight :
			pc == BISHOP  ? nWhiteBishop :
			pc == ROOK    ? nWhiteRook :
			pc == QUEEN   ? nWhiteQueen :
			                nWhiteKing;
	}

	template <>
	enumPiece_bbs bbsIndex<BLACK>(uint32_t pc) {
		return pc == PAWN ? nBlackPawn :
			pc == KNIGHT  ? nBlackKnight :
			pc == BISHOP  ? nBlackBishop :
			pc == ROOK    ? nBlackRook :
			pc == QUEEN   ? nBlackQueen :
						    nBlackKing;
	}

	// perform given move
	void makeMove(const MoveItem::iMove& move) {
		int target = move.getMask<MoveItem::iMask::TARGET>() >> 6,
			origin = move.getMask<MoveItem::iMask::ORIGIN>();
		const bool side = move.getMask<MoveItem::iMask::SIDE_F>();

		// change player to turn and update en passant square
		game_state.turn = !game_state.turn;
		game_state.ep_sq = -1;

		if (move.getMask<MoveItem::iMask::EN_PASSANT_F>()) {
			const int ep_pawn = target + (side ? Compass::nort : Compass::sout);
			moveBit(BBs[nWhitePawn + side], origin, target);
			popBit(BBs[nBlackPawn - side], ep_pawn);
			moveBit(BBs[nWhite + side], origin, target);
			popBit(BBs[nBlack - side], ep_pawn);
			popBit(BBs[nOccupied], origin);
			moveBit(BBs[nOccupied], ep_pawn, target);
			moveBit(BBs[nEmpty], ep_pawn, origin);
			return;
		}
		else if (const auto promotion = move.getMask<MoveItem::iMask::PROMOTION>()) {
			setBit(BBs[bbsIndex<WHITE>(promotion >> 20) + side], target);
			popBit(BBs[nWhitePawn + side], origin);
			moveBit(BBs[nWhite + side], origin, target);
			moveBit(BBs[nOccupied], origin, target);
			moveBit(BBs[nEmpty], target, origin);
			return;
		}
		else if (move.getMask<MoveItem::iMask::CASTLE_F>()) {
			game_state.castle &= ~(side ? 3 : 12);

			moveBit(BBs[nWhiteKing + side], origin, target);

			const bool rook_side = target == g1 or target == g8;
			const int rook_origin = (rook_side ? (side ? h8 : h1) : (side ? a8 : a1)),
				rook_target = origin + (rook_side ? 1 : -1);

			moveBit(BBs[nWhiteRook + side], rook_origin, rook_target);
			moveBit(BBs[nWhite + side], origin, target);
			moveBit(BBs[nWhite + side], rook_origin, rook_target);
			moveBit(BBs[nOccupied], origin, target);
			moveBit(BBs[nOccupied], rook_origin, rook_target);
			moveBit(BBs[nEmpty], target, origin);
			moveBit(BBs[nEmpty], rook_target, rook_origin);
			return;
		}

		// quiet moves or captures
		const auto piece = move.getMask<MoveItem::iMask::PIECE>() >> 12;

		moveBit(BBs[bbsIndex<WHITE>(piece) + side], origin, target);
		moveBit(BBs[nWhite + side], origin, target);
		moveBit(BBs[nOccupied], origin, target);
		moveBit(BBs[nEmpty], target, origin);

		// captured piece updating
		if (move.getMask<MoveItem::iMask::CAPTURE_F>()) {
			popBit(BBs[nBlack - side], target);

			for (auto pc = nBlackPawn - side; pc <= nBlackQueen; pc += 2) {
				if (getBit(BBs[pc], target)) {
					popBit(BBs[pc], target);
					break;
				}
			}
		}

		// setting new en passant position, if legal and possible
		if (move.getMask<MoveItem::iMask::DOUBLE_PUSH_F>()) {
			game_state.ep_sq = target;
			return;
		}

		// updating castle rights
		if (piece == ROOK and origin == (side ? a8 : a1))
			game_state.castle &= ~(side ? (1 << 1) : (1 << 2));
		else if (piece == ROOK and origin == (side ? h8 : h1))
			game_state.castle &= ~(1 << 3);
		else if (piece == KING)
			game_state.castle &= ~(side ? 3 : 12);
	}

	// unmake move using copy-make approach
	void unmakeMove(const BitBoardsSet& bbs_cpy, const gState& states_cpy) {
		BBs = bbs_cpy;
		game_state = states_cpy;
	}

} // namespace MovePerform


namespace MoveGenerator {

	namespace Analisis {

		void populateMoveList(MoveList& move_list) {
			auto helper = [](bool mask, std::string text, char s = ' ') {
				if (mask) {
					std::cout << s << text;
				}
			};
			static constexpr std::array<char, 4> promotion_piece = { 'n', 'b', 'r', 'q' };

			std::cout << "MoveList.size: " << move_list.size() << std::endl << std::endl << ' ';

			for (auto move : move_list) {
				std::cout << index_to_square[move.getMask<MoveItem::iMask::ORIGIN>()]
					<< index_to_square[move.getMask<MoveItem::iMask::TARGET>() >> 6];

				auto tmp = move.getMask<MoveItem::iMask::PROMOTION>();
				if (tmp) std::cout << promotion_piece[(tmp >> 20) - 1] << " promotion";
				helper(move.getMask<MoveItem::iMask::DOUBLE_PUSH_F>(), "double push");
				helper(move.getMask<MoveItem::iMask::CAPTURE_F>(), "capture");
				helper(move.getMask<MoveItem::iMask::EN_PASSANT_F>(), "en passant");
				helper(move.getMask<MoveItem::iMask::CASTLE_F>(), "castle");

				std::cout << '\n' << ' ';
			}

			std::cout << std::endl;
		}

		std::size_t dPerft(int depth) {
			if (!depth)
				return 1;

			const auto move_list = MoveGenerator::generateLegalMoves();
			const auto bbs_cpy = BBs;
			const auto gstate_cpy = game_state;
			std::size_t total = 0;

			for (const auto& move : move_list) {
				MovePerform::makeMove(move);
				total += dPerft(depth - 1);
				MovePerform::unmakeMove(bbs_cpy, gstate_cpy);
			}

			return total;
		}

		void perft(int depth) {
			assert(depth > 0 && "Unvalid depth size");
			
			Timer::go();

			const auto move_list = MoveGenerator::generateLegalMoves();
			const auto bbs_cpy = BBs;
			const auto gstate_cpy = game_state;
			std::size_t total = 0, single;
			
			for (const auto& move : move_list) {
				MovePerform::makeMove(move);
				total += (single = dPerft(depth - 1));
				
#ifdef __DEBUG__
				std::cout << ' ' << index_to_square[move.getMask<MoveItem::iMask::ORIGIN>()]
					<< index_to_square[move.getMask<MoveItem::iMask::TARGET>() >> 6]
					<< ": " << single << '\n';
#endif

				MovePerform::unmakeMove(bbs_cpy, gstate_cpy);
			}

			Timer::stop();
			const auto duration = Timer::duration();

			std::cout << std::endl << "Nodes: " << total << std::endl
				<< "Timer: ~" << duration << " ms" << std::endl
				<< "Performance: ~" << static_cast<float>(total / duration) << " kN/s" << std::endl
				<< "[depth " << depth << ']' << std::endl;
		}

	}

} // namespace MoveGenerator
