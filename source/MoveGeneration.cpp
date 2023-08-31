#include "MoveGeneration.h"
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
				*it++ = MoveItem::encodeQuietCapture(origin, target, bitU64(target) & BBs[nBlack - SIDE], PC, SIDE);
			}
		}
	}

	// common moves generator for sliding pieces - 
	// just processing proper pinned and unpinned sliders
	template <enumPiece PC, enumSide SIDE>
	inline void generateOf(U64 legal_blocks, MoveList::iterator& it) {
		pinGenerate<PC, SIDE,  true>(eU64, it);
		pinGenerate<PC, SIDE, false>(legal_blocks, it);
	}


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
			if (bitU64(target + Offset) & BBs[nWhitePawn + SIDE]) {
				*it++ = (MoveItem::encode<MoveItem::encodeType::CAPTURE>(target + Offset, target, PAWN, SIDE));
			}
		}

		// special horizontal pin test function template for en passant capture scenario
		template <enumSide SIDE, int Offset, int EP_Offset>
		void pawnEPGenHelper(MoveList::iterator& it) {

			// no legal en passant capture is possible
			if (!(bitU64(game_state.ep_sq + Offset) & BBs[nWhitePawn + SIDE]))
				return;

			// horizontal pin test
			const U64 horizon = attack<ROOK>(BBs[nOccupied] & ~bitU64(game_state.ep_sq), game_state.ep_sq + Offset);
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

				// pinned pawn - only pushes
				U64 single_push =
					PawnPushes::singlePushPawn<SIDE>(BBs[nWhitePawn + SIDE] & pinData::pinned, BBs[nEmpty]),
					double_push =
					PawnPushes::singlePushPawn<SIDE>(single_push, BBs[nEmpty]);
				int target;

				while (single_push) {
					target = popLS1B(single_push);
					PawnHelpers::pPawnMovesGenHelper<MoveItem::encodeType::QUIET, SIDE, SingleOff>(target, it);
				}

				while (double_push) {
					target = popLS1B(double_push);
					PawnHelpers::pPawnMovesGenHelper<MoveItem::encodeType::DOUBLE_PUSH, SIDE, DoubleOff>(target, it);
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

		int target;
		U64 single_push =
			PawnPushes::singlePushPawn<SIDE>(BBs[nWhitePawn + SIDE] & ~pinData::pinned, BBs[nEmpty])
			& legal_squares,
			double_push =
			PawnPushes::singlePushPawn<SIDE>(single_push, BBs[nEmpty]) & double_push_mask
			& legal_squares,
			captures =
			PawnAttacks::anyAttackPawn<SIDE>(BBs[nWhitePawn + SIDE] & ~pinData::pinned, BBs[nBlack - SIDE])
			& legal_squares,
			promote_moves =
			single_push & promote_rank_mask & legal_squares;

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

			check ? generateOfPawns<SIDE, true>(legal_blocks, it) : generateOfPawns<SIDE, false>(legal_blocks, it);
			// exclude pinned knights directly, since pinned knight can't move anywhere
			pinGenerate<KNIGHT, SIDE, false>(legal_blocks, it);
			generateOf <BISHOP, SIDE>(legal_blocks, it);
			generateOf <ROOK,   SIDE>(legal_blocks, it);
			generateOf <QUEEN,  SIDE>(legal_blocks, it);
		}

		
		// generate king legal moves
		U64 king_moves = cking_attacks[king_sq] & ~BBs[nWhite + SIDE];
		int target;

		// loop throught all king possible moves
		while (king_moves) {
			target = popLS1B(king_moves);

			// checking for an unattacked position after a move
			if (!isSquareAttacked<SIDE>(target)) {
				*it++ = MoveItem::encodeQuietCapture(king_sq, target, bitU64(target) & BBs[nBlack - SIDE], KING, SIDE);
			}
		}

		if (check)
			return;
		
		// castling: rook side and queen side
		if (game_state.castle.checkLegalCastle<SIDE & ROOK>()) {
			*it++ = MoveItem::encodeCastle<SIDE>(king_sq, SIDE == WHITE ? g1 : g8);
		}
		if (game_state.castle.checkLegalCastle<SIDE & QUEEN>()) {
			*it++ = MoveItem::encodeCastle<SIDE>(king_sq, SIDE == WHITE ? c1 : c8);
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
	void makeMove(MoveItem::iMove& move) {
		int target = move.getMask<MoveItem::iMask::TARGET>() >> 6,
			origin = move.getMask<MoveItem::iMask::ORIGIN>();
		auto promotion = move.getMask<MoveItem::iMask::PROMOTION>();
		bool side = move.getMask<MoveItem::iMask::SIDE_F>();

		game_state.ep_sq = -1;

		if (promotion) {
			setBit(BBs[side ? bbsIndex<BLACK>(promotion >> 20) : bbsIndex<WHITE>(promotion >> 20)], target);
			popBit(BBs[nWhitePawn + side], origin);
			moveBit(BBs[nWhite + side], origin, target);
			moveBit(BBs[nOccupied], origin, target);
			moveBit(BBs[nEmpty], target, origin);
			return;
		}
		else if (move.getMask<MoveItem::iMask::EN_PASSANT_F>()) {
			moveBit(BBs[nWhitePawn + side], origin, target);
			popBit(BBs[nBlackPawn - side], target + (side ? Compass::nort : Compass::sout));
			moveBit(BBs[nWhite + side], origin, target);
			moveBit(BBs[nOccupied], origin, target);
			moveBit(BBs[nEmpty], target + (side ? Compass::nort : Compass::sout), origin);
			return;
		}
		else if (move.getMask<MoveItem::iMask::CASTLE_F>()) {
			game_state.rook_king_move_block[side] = true;

			moveBit(BBs[nWhiteKing + side], origin, target);
			
			bool rook_side = target == g1 or target == g8;
			int rook_origin = (rook_side ? (side ? h8 : h1) : (side ? a8 : a1)),
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
		auto piece = move.getMask<MoveItem::iMask::PIECE>() >> 12;

		moveBit(BBs[side ? bbsIndex<BLACK>(piece) : bbsIndex<WHITE>(piece)], origin, target);
		moveBit(BBs[nWhite + side], origin, target);
		moveBit(BBs[nOccupied], origin, target);
		moveBit(BBs[nEmpty], target, origin);

		// captured piece updating
		if (move.getMask<MoveItem::iMask::CAPTURE_F>()) {
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
		}

		// change player to turn
		game_state.turn = !game_state.turn;

		// updating castle rights
		game_state.rook_king_move_block[side] |= (piece == KING or piece == ROOK);
		game_state.castle = 0;

		bool flag = true;

		// checking castling rights change at opponent side - white move
		if (!side and !game_state.rook_king_move_block[BLACK]) {
			
			// checking on king side
			if (attack<ROOK>(BBs[nOccupied], h8) & BBs[nBlackKing]) {
				
				// check whether fields between rook and king aren't attacked
				for (int sq = e8; sq < h8; sq++) {
					if (isSquareAttacked<BLACK>(sq)) {
						flag = false;
						break;
					}
				}

				if (flag) game_state.castle |= 1 << 1;

			} 
			// and queen side
			if (attack<ROOK>(BBs[nOccupied], a8) & BBs[nBlackKing]) {

				flag = true;

				for (int sq = e8; sq > a8; sq--) {
					if (isSquareAttacked<BLACK>(sq)) {
						flag = false;
						break;
					}
				}

				if(flag) game_state.castle |= 1;
			}

		} // return, no sense to continue executing
		else if (!side or game_state.rook_king_move_block[WHITE]) {
			return;
		}

		// castle change if black has just moved a piece - rook side
		if (attack<ROOK>(BBs[nOccupied], h1) & BBs[nWhiteKing]) {

			flag = true;

			for (int sq = e1; sq < h1; sq++) {
				if (isSquareAttacked<WHITE>(sq)) {
					flag = false;
					break;
				}
			}

			if (flag) game_state.castle |= 1 << 3;
		} 
		// and queen side
		if (attack<ROOK>(BBs[nOccupied], a1) & BBs[nWhiteKing]) {

			for (int sq = e1; sq > a1; sq--) {
				if (isSquareAttacked<WHITE>(sq)) {
					return;
				}
			}

			game_state.castle |= 1 << 2;
		}
	}

	// unmake move using copy-make approach
	void unmakeMove(BitBoardsSet& bbs_cpy, gState& states_cpy) {
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
				if (tmp) helper(tmp, " promotion", promotion_piece[(tmp >> 20) - 1]);
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

			auto move_list = MoveGenerator::generateLegalMoves();
			auto bbs_cpy = BBs;
			auto gstate_cpy = game_state;

			for (auto& move : move_list) {
				MovePerform::makeMove(move);
				dPerft(depth - 1);
				MovePerform::unmakeMove(bbs_cpy, gstate_cpy);
			}

			return move_list.size();
		}

		void perft(int depth) {
			assert(depth > 0 && "Unvalid depth size");

			auto move_list = MoveGenerator::generateLegalMoves();
			auto bbs_cpy = BBs;
			auto gstate_cpy = game_state;
			std::size_t total = 0, single;
			
			for (auto& move : move_list) {
				MovePerform::makeMove(move);
				total += (single = dPerft(depth - 1));

				std::cout << ' ' << index_to_square[move.getMask<MoveItem::iMask::ORIGIN>()]
					<< index_to_square[move.getMask<MoveItem::iMask::TARGET>() >> 6]
					<< ' ' << single << '\n';

				MovePerform::unmakeMove(bbs_cpy, gstate_cpy);
			}

			std::cout << "Nodes: " << total << std::endl << std::endl;
		}

	}

} // namespace MoveGenerator
