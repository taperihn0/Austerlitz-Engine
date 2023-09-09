#include "MoveGeneration.h"
#include "Timer.h"
#include <string>


namespace pinData {

	// cache table to store inbetween paths of king and his x-ray attackers
	// for pinned pieces, since such pinned piece can only move through their inbetween path
	// mask for diagonal pinmask (simply bishop attack from king square), horizontally or vertically pinned, 
	// logical OR of these two pin types
	U64 diag_pin, hv_pin, pinned;
	// current king square
	int king_sq;
}

namespace checkData {
	U64 legal_squares;
}


namespace MoveGenerator {

	template <enumPiece PC, enumSide SIDE, bool Pin>
	inline auto pinMask(U64) -> std::enable_if_t<!Pin, U64>{
		return checkData::legal_squares;
	}

	template <enumPiece PC, enumSide SIDE, bool Pin>
	inline auto pinMask(U64) -> std::enable_if_t<Pin and PC == QUEEN, U64> {
		return UINT64_MAX;
	}

	template <enumPiece PC, enumSide SIDE, bool Pin>
	inline auto pinMask(U64) -> std::enable_if_t<Pin and PC == BISHOP, U64> {
		return attack<BISHOP>(BBs[nBlack - SIDE], pinData::king_sq);
	}

	template <enumPiece PC, enumSide SIDE, bool Pin>
	inline auto pinMask(bool any_rook) -> std::enable_if_t<Pin and PC == ROOK, U64> {
		return any_rook ? attack<ROOK>(BBs[nBlack - SIDE], pinData::king_sq) : UINT64_MAX;
	}

	template <enumPiece PC, bool Pin>
	inline auto pinType() -> std::enable_if_t<Pin and PC == BISHOP, U64> {
		return pinData::diag_pin;
	}

	template <enumPiece PC, bool Pin>
	inline auto pinType() -> std::enable_if_t<Pin and PC == ROOK, U64> {
		return pinData::hv_pin;
	}

	template <enumPiece PC, bool Pin>
	inline auto pinType() -> std::enable_if_t<Pin and PC == QUEEN, U64> {
		return pinData::pinned;
	}

	template <enumPiece PC, bool Pin>
	inline auto pinType() -> std::enable_if_t<!Pin, U64> {
		return ~pinData::pinned;
	}

	template <enumPiece PC, enumSide SIDE, bool Pin>
	inline auto sqAvaible(int sq) -> std::enable_if_t<Pin and PC == QUEEN, U64> {
		return attack<QUEEN>(BBs[nBlack - SIDE], pinData::king_sq)
			& (xRayQueenAttack(BBs[nBlack - SIDE], bitU64(sq), pinData::king_sq))
			| inBetween(pinData::king_sq, sq);
	}

	template <enumPiece PC, enumSide SIDE, bool Pin>
	inline auto sqAvaible(int) -> std::enable_if_t<!Pin or (Pin and PC != QUEEN), U64> {
		return UINT64_MAX;
	};

	// generator functions based on PINNED piece flag -
	// excluded pieces template parameters don't meet conditions of these functions.
	template <enumPiece PC, enumSide SIDE, bool Pin, class =
		std::enable_if<PC != PAWN and PC != KING and (!Pin and PC == KNIGHT)>>
	void pinGenerate(MoveList::iterator& it) {

		// only pinned or only unpinned processing
		U64 pieces = BBs[bbsIndex<PC>() + SIDE] & pinType<PC, Pin>(),
			attacks;
		int origin, target;

		// avaible fields during attacking
		const U64 avaible = ~BBs[nWhite + SIDE] & pinMask<PC, SIDE, Pin>(pieces);

		while (pieces) {
			origin = popLS1B(pieces);
			attacks = attack<PC>(BBs[nOccupied], origin) & avaible & sqAvaible<PC, SIDE, Pin>(origin);

			while (attacks) {
				target = popLS1B(attacks);
				*it++ = MoveItem::encodeQuietCapture<PC, SIDE>(origin, target, bitU64(target) & BBs[nBlack - SIDE]);
			}
		}
	}

	// common moves generator for sliding pieces - 
	// just processing proper pinned and unpinned sliders
	template <enumPiece PC, enumSide SIDE>
	inline void generateOf(MoveList::iterator& it, bool check) {
		pinGenerate<PC, SIDE, false>(it);
		if (!check) pinGenerate<PC, SIDE, true>(it);
	}

	// some handy functions during generating pawn fully-legal moves
	namespace PawnHelpers {

		// generate pawn possible promotions
		template <enumSide SIDE, bool Capture, int Offset>
		void pawnPromotionGenHelper(int target, MoveList::iterator& it) {
			*it++ = (MoveItem::encodePromotion<SIDE, QUEEN, Capture>(target + Offset, target));
			*it++ = (MoveItem::encodePromotion<SIDE, ROOK, Capture>(target + Offset, target));
			*it++ = (MoveItem::encodePromotion<SIDE, BISHOP, Capture>(target + Offset, target));
			*it++ = (MoveItem::encodePromotion<SIDE, KNIGHT, Capture>(target + Offset, target));
		}

		// special horizontal pin test function template for en passant capture scenario
		template <enumSide SIDE, int Offset, int EP_Offset>
		void pawnEPGenHelper(MoveList::iterator& it) {

			const U64 exclude_ep_cap = BBs[nOccupied] & ~(bitU64(game_state.ep_sq) | bitU64(game_state.ep_sq + Offset))
				| bitU64(game_state.ep_sq - EP_Offset);

			// checking en passant capture legality
			if (attack<ROOK>(exclude_ep_cap, pinData::king_sq)
				& (BBs[nBlackQueen - SIDE] | BBs[nBlackRook - SIDE]) or 
				attack<BISHOP>(exclude_ep_cap, pinData::king_sq)
				& (BBs[nBlackQueen - SIDE] | BBs[nBlackBishop - SIDE]))
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
			static void process(const U64 legal_squares, MoveList::iterator& it) {

				// en passant case - 
				// permitted only when checker is a pawn possible to capture using en passant rule
				if (getLS1BIndex(legal_squares) != game_state.ep_sq)
					return;
				else if (bitU64(game_state.ep_sq + Compass::west) & (BBs[nWhitePawn + SIDE] & ~pinData::pinned)) {
					*it++ = MoveItem::encodeEnPassant<SIDE>(game_state.ep_sq + Compass::west, game_state.ep_sq - SingleOff);
				}
				if (bitU64(game_state.ep_sq + Compass::east) & (BBs[nWhitePawn + SIDE] & ~pinData::pinned)) {
					*it++ = MoveItem::encodeEnPassant<SIDE>(game_state.ep_sq + Compass::east, game_state.ep_sq - SingleOff);
				}
			}
		};

		template <enumSide SIDE, int SingleOff, int DoubleOff>
		struct miscellaneousHelper<SIDE, SingleOff, DoubleOff, false> {
			static void process(U64, MoveList::iterator& it) {
				static constexpr U64 double_push_mask = SIDE == WHITE ? Constans::r4_rank : Constans::r5_rank;
				static constexpr int east_att_off = SIDE == WHITE ? Compass::soEa : Compass::noEa,
					west_att_off = SIDE == WHITE ? Compass::soWe : Compass::noWe;
				static constexpr U64 promote_rank_mask = SIDE == WHITE ? Constans::r8_rank : Constans::r1_rank;

				const U64 hor_pinned_pawns = pinData::pinned & BBs[nWhitePawn + SIDE]
					& Constans::f_by_index[pinData::king_sq % 8],
					possible_captures = BBs[nBlack - SIDE] & attack<BISHOP>(BBs[nBlack - SIDE], pinData::king_sq);

				int target;
				U64 single_push = PawnPushes::singlePushPawn<SIDE>(hor_pinned_pawns, BBs[nEmpty]),
					double_push = PawnPushes::singlePushPawn<SIDE>(single_push, BBs[nEmpty]) & double_push_mask,
					west_captures = PawnAttacks::westAttackPawn<SIDE>(pinData::diag_pin & BBs[nWhitePawn + SIDE],
						possible_captures),
					east_captures = PawnAttacks::eastAttackPawn<SIDE>(pinData::diag_pin & BBs[nWhitePawn + SIDE],
						possible_captures),
					promote_moves = west_captures & promote_rank_mask;

				// processing pawn promotions by captures
				while (promote_moves) {
					PawnHelpers::pawnPromotionGenHelper<SIDE, true, east_att_off>(popLS1B(promote_moves), it);
				}

				promote_moves = east_captures & promote_rank_mask;
				while (promote_moves) {
					PawnHelpers::pawnPromotionGenHelper<SIDE, true, west_att_off>(popLS1B(promote_moves), it);
				}

				while (single_push) {
					target = popLS1B(single_push);
					*it++ = (MoveItem::encode<MoveItem::encodeType::QUIET>(target + SingleOff, target, PAWN, SIDE));
				}

				while (double_push) {
					target = popLS1B(double_push);
					*it++ = (MoveItem::encode<MoveItem::encodeType::DOUBLE_PUSH>(target + DoubleOff, target, PAWN, SIDE));
				}

				west_captures &= ~promote_rank_mask;
				while (west_captures) {
					target = popLS1B(west_captures);
					*it++ = MoveItem::encode<MoveItem::encodeType::CAPTURE>(target + east_att_off, target, PAWN, SIDE);
				}

				east_captures &= ~promote_rank_mask;
				while (east_captures) {
					target = popLS1B(east_captures);
					*it++ = MoveItem::encode<MoveItem::encodeType::CAPTURE>(target + west_att_off, target, PAWN, SIDE);
				}

				if (game_state.ep_sq == -1)
					return;

				if (PawnAttacks::eastAttackPawn<SIDE>(BBs[nWhitePawn + SIDE] & ~pinData::pinned, bitU64(game_state.ep_sq - SingleOff))) {
					PawnHelpers::pawnEPGenHelper<SIDE, Compass::west, SingleOff>(it);
				} 
				if (PawnAttacks::westAttackPawn<SIDE>(BBs[nWhitePawn + SIDE] & ~pinData::pinned, bitU64(game_state.ep_sq - SingleOff))) {
					PawnHelpers::pawnEPGenHelper<SIDE, Compass::east, SingleOff>(it);
					return;
				}
				
				if (!(bitU64(game_state.ep_sq - SingleOff) & attack<BISHOP>(BBs[nBlack - SIDE], pinData::king_sq)))
					return;

				if (PawnAttacks::eastAttackPawn<SIDE>(BBs[nWhitePawn + SIDE] & pinData::diag_pin, bitU64(game_state.ep_sq - SingleOff))) {
					*it++ = MoveItem::encodeEnPassant<SIDE>(game_state.ep_sq + Compass::west, game_state.ep_sq - SingleOff);
				}
				else if (PawnAttacks::westAttackPawn<SIDE>(BBs[nWhitePawn + SIDE] & pinData::diag_pin, bitU64(game_state.ep_sq - SingleOff))) {
					*it++ = MoveItem::encodeEnPassant<SIDE>(game_state.ep_sq + Compass::east, game_state.ep_sq - SingleOff);
				}
			}
		};

	} // namespace PawnHelpers


	// pawn legal moves generator function
	// template parameter Check indicating whether to consider check
	// in the background or not
	template <enumSide SIDE, bool Check>
	void generateOfPawns(MoveList::iterator& it) {

		// calculate offset for origin squares only once for every generated function
		static constexpr int single_off = SIDE == WHITE ? Compass::sout : Compass::nort,
			double_off = single_off * 2,
			west_att_off = SIDE == WHITE ? Compass::soWe : Compass::noWe,
			east_att_off = SIDE == WHITE ? Compass::soEa : Compass::noEa;
		static constexpr U64 promote_rank_mask = SIDE == WHITE ? Constans::r8_rank : Constans::r1_rank,
			double_push_mask = SIDE == WHITE ? Constans::r4_rank : Constans::r5_rank;

		const U64 unpinned = BBs[nWhitePawn + SIDE] & ~pinData::pinned;

		int target;
		U64 single_push = PawnPushes::singlePushPawn<SIDE>(unpinned, BBs[nEmpty]),
			double_push = PawnPushes::singlePushPawn<SIDE>(single_push, BBs[nEmpty]) & double_push_mask & checkData::legal_squares,
			west_captures = PawnAttacks::westAttackPawn<SIDE>(unpinned, BBs[nBlack - SIDE]) & checkData::legal_squares,
			east_captures = PawnAttacks::eastAttackPawn<SIDE>(unpinned, BBs[nBlack - SIDE]) & checkData::legal_squares,
			promote_moves = single_push & checkData::legal_squares & promote_rank_mask;

		// processing pawn promotions by pushes
		while (promote_moves) {
			PawnHelpers::pawnPromotionGenHelper<SIDE, false, single_off>(popLS1B(promote_moves), it);
		}

		// processing pawn promotions by captures
		promote_moves = west_captures & promote_rank_mask;
		while (promote_moves) {
			PawnHelpers::pawnPromotionGenHelper<SIDE, true, east_att_off>(popLS1B(promote_moves), it);
		}

		promote_moves = east_captures & promote_rank_mask;
		while (promote_moves) {
			PawnHelpers::pawnPromotionGenHelper<SIDE, true, west_att_off>(popLS1B(promote_moves), it);
		}

		// processing pawn pushes, then possible captures
		// excluding promotion case - promotion scenario is processed
		single_push &= ~promote_rank_mask & checkData::legal_squares;
		while (single_push) {
			target = popLS1B(single_push);
			*it++ = (MoveItem::encode<MoveItem::encodeType::QUIET>(target + single_off, target, PAWN, SIDE));
		}

		while (double_push) {
			target = popLS1B(double_push);
			*it++ = MoveItem::encode<MoveItem::encodeType::DOUBLE_PUSH>(target + double_off, target, PAWN, SIDE);
		}

		west_captures &= ~promote_rank_mask;
		while (west_captures) {
			target = popLS1B(west_captures);
			*it++ = MoveItem::encode<MoveItem::encodeType::CAPTURE>(target + east_att_off, target, PAWN, SIDE);
		}

		east_captures &= ~promote_rank_mask;
		while (east_captures) {
			target = popLS1B(east_captures);
			*it++ = MoveItem::encode<MoveItem::encodeType::CAPTURE>(target + west_att_off, target, PAWN, SIDE);
		}

		PawnHelpers::miscellaneousHelper<SIDE, single_off, double_off, Check>::process(checkData::legal_squares, it);
	}

	// generate legal moves for pieces of specific color
	template <enumSide SIDE>
	void generateLegalOf(MoveList::iterator& it) {
		pinData::king_sq = getLS1BIndex(BBs[nWhiteKing + SIDE]);
		const U64 checkers = attackTo<SIDE, KING>(pinData::king_sq);
		const bool check = checkers;

		// double check case skipping - only king moves to non-attacked squares are permitted
		// when there is double check situation
		if (!isDoubleChecked(checkers)) {
			pinData::diag_pin = pinnedDiagonal<SIDE>(pinData::king_sq);
			pinData::hv_pin = pinnedHorizonVertic<SIDE>(pinData::king_sq);
			pinData::pinned = pinData::diag_pin | pinData::hv_pin;
			checkData::legal_squares = UINT64_MAX;

			if (check) {
				const int checker_sq = getLS1BIndex(checkers);
				checkData::legal_squares = inBetween(pinData::king_sq, checker_sq) | bitU64(checker_sq);
			}

			// exclude pinned knights directly, since pinned knight can't move anywhere
			pinGenerate<KNIGHT, SIDE, false>(it);
			generateOf <BISHOP, SIDE>(it, check);
			generateOf <ROOK, SIDE>(it, check);
			generateOf <QUEEN, SIDE>(it, check);
			check ? generateOfPawns<SIDE, true>(it) : generateOfPawns<SIDE, false>(it);
		}

		// generate king legal moves
		U64 king_moves = cking_attacks[pinData::king_sq] & ~BBs[nWhite + SIDE];
		int target;

		// loop throught all king possible moves
		while (king_moves) {
			target = popLS1B(king_moves);

			// checking for an unattacked position after a move
			if (!isSquareAttacked<SIDE>(target)) {
				*it++ = MoveItem::encodeQuietCapture<KING, SIDE>(pinData::king_sq, target, bitU64(target) & BBs[nBlack - SIDE]);
			}
		}

		if (check)
			return;

		// prepare king square and proper rook square
		static constexpr int rsq1 = SIDE ? h8 : h1;
		static constexpr int rsq2 = SIDE ? b8 : b1;
		static constexpr int rook_left = SIDE ? a8 : a1;

		// checking castling rights change at opponent side - white move
		if (game_state.castle.checkLegalCastle<SIDE & ROOK>() and
			attack<ROOK>(BBs[nOccupied], rsq1) & BBs[nWhiteKing + SIDE] and
			bitU64(rsq1) & BBs[nWhiteRook + SIDE]) {
				bool ind_f = true;

				// check whether fields between rook and king aren't attacked
				for (int sq = pinData::king_sq + 1; sq < rsq1; sq++) {
					if (isSquareAttacked<SIDE>(sq)) {
						ind_f = false;
						break;
					}
				}

				if (ind_f) *it++ = MoveItem::encodeCastle<SIDE>(pinData::king_sq, SIDE ? g8 : g1);
		}

		// and queen side
		if (game_state.castle.checkLegalCastle<SIDE & QUEEN>() and
			attack<ROOK>(BBs[nOccupied], rook_left) & BBs[nWhiteKing + SIDE] and
			bitU64(rook_left) & BBs[nWhiteRook + SIDE]) {
				bool ind_f = true;

				for (int sq = pinData::king_sq - 1; sq > rsq2; sq--) {
					if (isSquareAttacked<SIDE>(sq)) {
						ind_f = false;
						break;
					}
				}

				if (ind_f) *it++ = MoveItem::encodeCastle<SIDE>(pinData::king_sq, SIDE ? c8 : c1);
		}
	}

	// remember about RVO rule
	MoveList generateLegalMoves() {
		MoveList move_list;
		game_state.turn == WHITE ? generateLegalOf<WHITE>(move_list.it) : generateLegalOf<BLACK>(move_list.it);
		return move_list;
	}

} // namespace MoveGenerator


namespace MovePerform {

	// handy functions helping getting proper piece index in bitboards set
	template <enumSide Side>
	size_t bbsIndex(uint32_t pc) {
		return pc == PAWN ? nWhitePawn + Side :
			pc == KNIGHT ? nWhiteKnight + Side :
			pc == BISHOP ? nWhiteBishop + Side :
			pc == ROOK ? nWhiteRook + Side :
			pc == QUEEN ? nWhiteQueen + Side:
			nWhiteKing + Side;
	}

	inline void captureCase(const MoveItem::iMove& move, bool side, int target) {
		if (move.getMask<MoveItem::iMask::CAPTURE_F>()) {
			popBit(BBs[nBlack - side], target);

			for (auto pc = nBlackPawn - side; pc <= nBlackQueen; pc += 2) {
				if (getBit(BBs[pc], target)) {
					popBit(BBs[pc], target);
					break;
				}
			}
		}
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
			setBit(BBs[nEmpty], origin);
			moveBit(BBs[nEmpty], target, ep_pawn);
			return;
		}
		else if (const auto promotion = move.getMask<MoveItem::iMask::PROMOTION>()) {
			setBit(BBs[bbsIndex<WHITE>(promotion >> 20) + side], target);
			popBit(BBs[nWhitePawn + side], origin);
			moveBit(BBs[nWhite + side], origin, target);
			moveBit(BBs[nOccupied], origin, target);
			moveBit(BBs[nEmpty], target, origin);
			// maybe there is also a capture?
			captureCase(move, side, target);
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
		captureCase(move, side, target);

		// setting new en passant position, if legal and possible
		if (move.getMask<MoveItem::iMask::DOUBLE_PUSH_F>()) {
			game_state.ep_sq = target;
		} // updating castling rights
		else if (piece == ROOK and origin == (side ? a8 : a1))
			game_state.castle &= ~(1 << (!side * 2));
		else if (piece == ROOK and origin == (side ? h8 : h1))
			game_state.castle &= ~(1 << (3 - side * 2));
		else if (piece == KING)
			game_state.castle &= ~(12 - side * 9);
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

		template <int Depth>
		unsigned long long dPerft() {
			const auto bbs_cpy = BBs;
			const auto gstate_cpy = game_state;
			unsigned long long total = 0;

			for (const auto& move : MoveGenerator::generateLegalMoves()) {
				MovePerform::makeMove(move);
				total += dPerft<Depth - 1>();
				MovePerform::unmakeMove(bbs_cpy, gstate_cpy);
			}

			return total;
		}

		template <>
		inline unsigned long long dPerft<0>() noexcept {
			return 1uLL;
		}

		template <>
		inline unsigned long long dPerft<1>() {
			return MoveGenerator::generateLegalMoves().size();
		}

		template <int Depth>
		void perft() {
			static_assert(Depth > 0, "Unvalid depth size");

			Timer::go();

			const auto move_list = MoveGenerator::generateLegalMoves();
			const auto bbs_cpy = BBs;
			const auto gstate_cpy = game_state;
			unsigned long long total = 0, single;

			for (const auto& move : move_list) {
				MovePerform::makeMove(move);
				total += (single = dPerft<Depth - 1>());

#ifdef __DEBUG__
				std::cout << ' ' << index_to_square[move.getMask<MoveItem::iMask::ORIGIN>()]
					<< index_to_square[move.getMask<MoveItem::iMask::TARGET>() >> 6]
					<< ": " << single << '\n';
#endif

				MovePerform::unmakeMove(bbs_cpy, gstate_cpy);
			}

			Timer::stop();
			const auto duration = Timer::duration();

			const auto perform_kn = total * 1. / duration;

			std::cout << std::endl << "Nodes: " << total << std::endl
				<< "Timer: ~" << duration << " ms" << std::endl
				<< "Performance: ~" << perform_kn << " kN/s = ~" << static_cast<int>(perform_kn) / 1000 << " MN/s" << std::endl
				<< "[depth " << Depth << ']' << std::endl;
		}

		template void perft<1>();
		template void perft<2>();
		template void perft<3>();
		template void perft<4>();
		template void perft<5>();
		template void perft<6>();
		template void perft<7>();
		template void perft<8>();
		template void perft<9>();
		template void perft<10>();

	} // namespace Analisis


} // namespace MoveGenerator