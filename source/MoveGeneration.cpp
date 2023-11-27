#include "MoveGeneration.h"
#include "Timer.h"
#include "Zobrist.h"
#include "Evaluation.h"
#include <string>
		

namespace pinData {
	// cache table to store inbetween paths of king and his x-ray attackers
	// for pinned pieces, since such pinned piece can only move through their inbetween path
	// mask for diagonal pinmask (simply bishop attack from king square), horizontally or vertically pinned, 
	// logical OR of these two pin types and pinmask for bishop
	U64 diag_pin, hv_pin, pinned, ksq_diag;
	// current king square
	int king_sq;
}

namespace checkData {
	U64 legal_squares;
}


namespace MoveGenerator {

	template <enumPiece PC, enumSide SIDE, bool Pin>
	inline auto pinMask() -> std::enable_if_t<!Pin, U64>{
		return checkData::legal_squares;
	}

	template <enumPiece PC, enumSide SIDE, bool Pin>
	inline auto pinMask() -> std::enable_if_t<Pin and PC == QUEEN, U64> {
		return UINT64_MAX;
	}

	template <enumPiece PC, enumSide SIDE, bool Pin>
	inline auto pinMask() -> std::enable_if_t<Pin and PC == BISHOP, U64> {
		return pinData::ksq_diag;
	}

	template <enumPiece PC, enumSide SIDE, bool Pin>
	inline auto pinMask() -> std::enable_if_t<Pin and PC == ROOK, U64> {
		return attack<ROOK>(BBs[nBlack - SIDE], pinData::king_sq);
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
	template <GenType gType, enumPiece PC, enumSide SIDE, bool Pin, class =
		std::enable_if<PC != PAWN and PC != KING and (!Pin and PC == KNIGHT)>>
	void pinGenerate(MoveList::iterator& it) {

		// only pinned or only unpinned processing
		U64 pieces = BBs[bbsIndex<PC>() + SIDE] & pinType<PC, Pin>(),
			attacks, captures, quiets;
		int origin;

		// avaible fields during attacking
		const U64 avaible = ~BBs[nWhite + SIDE] & pinMask<PC, SIDE, Pin>();

		while (pieces) {
			origin = popLS1B(pieces);

			// perform captures
			attacks =
				attack<PC>(BBs[nOccupied], origin) & avaible & sqAvaible<PC, SIDE, Pin>(origin);

			captures = attacks & BBs[nBlack - SIDE];
			while (captures) {
				*it++ = MoveItem::encodeQuietCapture<PC, SIDE>(origin, popLS1B(captures), true);
			}

			// perform quiets
			if constexpr (gType != CAPTURES and gType != TACTICAL) {
				quiets = attacks & ~BBs[nBlack - SIDE];

				while (quiets) {
					*it++ = MoveItem::encodeQuietCapture<PC, SIDE>(origin, popLS1B(quiets), false);
				}
			}
		}
	}

	// common moves generator for sliding pieces - 
	// just processing proper pinned and unpinned sliders
	template <GenType gType, enumPiece PC, enumSide SIDE>
	inline void generateOf(MoveList::iterator& it, bool check) {
		pinGenerate<gType, PC, SIDE, false>(it);
		if (!check) pinGenerate<gType, PC, SIDE, true>(it);
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

		// pawn legal moves direct helper - separate for check case and no check case
		template <GenType, enumSide SIDE, int SingleOff, int DoubleOff, bool Check>
		auto diffPawnGenerate(MoveList::iterator& it) -> std::enable_if_t<Check> {
			// en passant case - 
			// permitted only when checker is a pawn possible to capture using en passant rule
			if (getLS1BIndex(checkData::legal_squares) != game_state.ep_sq)
				return;
			else if (bitU64(game_state.ep_sq + Compass::west) & (BBs[nWhitePawn + SIDE] & ~pinData::pinned)) {
				*it++ = MoveItem::encodeEnPassant<SIDE>(game_state.ep_sq + Compass::west, game_state.ep_sq - SingleOff);
			}
			if (bitU64(game_state.ep_sq + Compass::east) & (BBs[nWhitePawn + SIDE] & ~pinData::pinned)) {
				*it++ = MoveItem::encodeEnPassant<SIDE>(game_state.ep_sq + Compass::east, game_state.ep_sq - SingleOff);
			}
		}

		template <GenType gType, enumSide SIDE, int SingleOff, int DoubleOff, bool Check>
		auto diffPawnGenerate(MoveList::iterator& it) -> std::enable_if_t<!Check> {
			static constexpr U64 
				double_push_mask = SIDE == WHITE ? Constans::r4_rank : Constans::r5_rank,
				promote_rank_mask = SIDE == WHITE ? Constans::r8_rank : Constans::r1_rank;
			static constexpr int 
				east_att_off = SIDE == WHITE ? Compass::soEa : Compass::noEa,
				west_att_off = SIDE == WHITE ? Compass::soWe : Compass::noWe;

			const U64 
				hor_pinned_pawns = pinData::pinned & BBs[nWhitePawn + SIDE] & Constans::f_by_index[pinData::king_sq % 8],
				possible_captures = BBs[nBlack - SIDE] & pinData::ksq_diag;

			int target;
			U64 west_captures = PawnAttacks::westAttackPawn<SIDE>(pinData::diag_pin & BBs[nWhitePawn + SIDE],
					possible_captures),
				east_captures = PawnAttacks::eastAttackPawn<SIDE>(pinData::diag_pin & BBs[nWhitePawn + SIDE],
					possible_captures);

			if constexpr (gType != CAPTURES and gType != TACTICAL) {
				U64 single_push = PawnPushes::singlePushPawn<SIDE>(hor_pinned_pawns, BBs[nEmpty]),
					double_push = PawnPushes::singlePushPawn<SIDE>(single_push, BBs[nEmpty]) & double_push_mask;

				while (single_push) {
					target = popLS1B(single_push);
					*it++ = (MoveItem::encode<MoveItem::encodeType::QUIET>(target + SingleOff, target, PAWN, SIDE));
				}

				while (double_push) {
					target = popLS1B(double_push);
					*it++ = (MoveItem::encode<MoveItem::encodeType::DOUBLE_PUSH>(target + DoubleOff, target, PAWN, SIDE));
				}
			}

			// processing pawn promotions by captures
			U64 promote_moves = west_captures & promote_rank_mask;
			while (promote_moves) {
				PawnHelpers::pawnPromotionGenHelper<SIDE, true, east_att_off>(popLS1B(promote_moves), it);
			}

			promote_moves = east_captures & promote_rank_mask;
			while (promote_moves) {
				PawnHelpers::pawnPromotionGenHelper<SIDE, true, west_att_off>(popLS1B(promote_moves), it);
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

			// generate en passant capture for unpinned pawns
			if (game_state.ep_sq == -1) {
				return;
			}
			else if (PawnAttacks::eastAttackPawn<SIDE>(BBs[nWhitePawn + SIDE] & ~pinData::pinned, 
				bitU64(game_state.ep_sq - SingleOff))) {
				PawnHelpers::pawnEPGenHelper<SIDE, Compass::west, SingleOff>(it);
			}
			if (PawnAttacks::westAttackPawn<SIDE>(BBs[nWhitePawn + SIDE] & ~pinData::pinned, 
				bitU64(game_state.ep_sq - SingleOff))) {
				PawnHelpers::pawnEPGenHelper<SIDE, Compass::east, SingleOff>(it);
				return;
			}

			// en passant generation for pinned pawns
			if (!(bitU64(game_state.ep_sq - SingleOff) & pinData::ksq_diag)) {
				return;
			}
			else if (PawnAttacks::eastAttackPawn<SIDE>(BBs[nWhitePawn + SIDE] & pinData::diag_pin, 
				bitU64(game_state.ep_sq - SingleOff))) {
				*it++ = MoveItem::encodeEnPassant<SIDE>(game_state.ep_sq + Compass::west, game_state.ep_sq - SingleOff);
			}
			else if (PawnAttacks::westAttackPawn<SIDE>(BBs[nWhitePawn + SIDE] & pinData::diag_pin, 
				bitU64(game_state.ep_sq - SingleOff))) {
				*it++ = MoveItem::encodeEnPassant<SIDE>(game_state.ep_sq + Compass::east, game_state.ep_sq - SingleOff);
			}
		}

	} // namespace PawnHelpers


	// pawn legal moves generator function
	// template parameter Check indicating whether to consider check
	// in the background or not
	template <GenType gType, enumSide SIDE, bool Check>
	void generateOfPawns(MoveList::iterator& it) {
		// calculate offset for origin squares only once for every generated function
		static constexpr int 
			single_off = SIDE == WHITE ? Compass::sout : Compass::nort,
			double_off = single_off * 2,
			west_att_off = SIDE == WHITE ? Compass::soWe : Compass::noWe,
			east_att_off = SIDE == WHITE ? Compass::soEa : Compass::noEa;
		static constexpr U64 
			promote_rank_mask = SIDE == WHITE ? Constans::r8_rank : Constans::r1_rank,
			double_push_mask = SIDE == WHITE ? Constans::r4_rank : Constans::r5_rank;

		const U64 unpinned = BBs[nWhitePawn + SIDE] & ~pinData::pinned;

		int target;
		U64 single_push = 
				PawnPushes::singlePushPawn<SIDE>(unpinned, BBs[nEmpty]),
			west_captures = 
				PawnAttacks::westAttackPawn<SIDE>(unpinned, BBs[nBlack - SIDE]) & checkData::legal_squares,
			east_captures = 
				PawnAttacks::eastAttackPawn<SIDE>(unpinned, BBs[nBlack - SIDE]) & checkData::legal_squares,
			promote_moves = 
				single_push & checkData::legal_squares & promote_rank_mask;
		
		// process quiet moves
		if constexpr (gType != CAPTURES and gType != TACTICAL) {
			// double pushes mask initialization, then process it
			U64 double_push =
				PawnPushes::singlePushPawn<SIDE>(single_push, BBs[nEmpty]) & double_push_mask & checkData::legal_squares;

			// excluding promotion case - quiet moves
			single_push &= ~promote_rank_mask & checkData::legal_squares;
			while (single_push) {
				target = popLS1B(single_push);
				*it++ = (MoveItem::encode<MoveItem::encodeType::QUIET>(target + single_off, target, PAWN, SIDE));
			}

			while (double_push) {
				target = popLS1B(double_push);
				*it++ = MoveItem::encode<MoveItem::encodeType::DOUBLE_PUSH>(target + double_off, target, PAWN, SIDE);
			}
		}

		// processing pawn promotions by pushes
		if constexpr (gType != CAPTURES) {
			while (promote_moves) {
				int target = popLS1B(promote_moves);
					
				// as a tactical move, consider only queen promotion, since other promotion should be skipped in qsearch
				if constexpr (gType != TACTICAL)
					PawnHelpers::pawnPromotionGenHelper<SIDE, false, single_off>(target, it);
				else 
					*it++ = (MoveItem::encodePromotion<SIDE, QUEEN, false>(target + single_off, target));
			}
		}

		// processing all kind of captures - 

		// processing pawn promotions by captures
		promote_moves = west_captures & promote_rank_mask;
		while (promote_moves) {
			PawnHelpers::pawnPromotionGenHelper<SIDE, true, east_att_off>(popLS1B(promote_moves), it);
		}

		promote_moves = east_captures & promote_rank_mask;
		while (promote_moves) {
			PawnHelpers::pawnPromotionGenHelper<SIDE, true, west_att_off>(popLS1B(promote_moves), it);
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

		PawnHelpers::diffPawnGenerate<gType, SIDE, single_off, double_off, Check>(it);
	}

	// generate legal moves for pieces of specific color
	template <GenType gType, enumSide SIDE>
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
			pinData::ksq_diag = attack<BISHOP>(BBs[nBlack - SIDE], pinData::king_sq);
			checkData::legal_squares = UINT64_MAX;

			if (check) {
				const int checker_sq = getLS1BIndex(checkers);
				checkData::legal_squares = inBetween(pinData::king_sq, checker_sq) | bitU64(checker_sq);
			}

			// exclude pinned knights directly, since pinned knight can't move anywhere
			pinGenerate<gType, KNIGHT, SIDE, false>(it);
			generateOf <gType, BISHOP, SIDE>(it, check);
			generateOf <gType, ROOK,   SIDE>(it, check);
			generateOf <gType, QUEEN,  SIDE>(it, check);
			check ? generateOfPawns<gType, SIDE, true>(it) : generateOfPawns<gType, SIDE, false>(it);
		}

		// generate king legal moves
		U64 king_moves = cking_attacks[pinData::king_sq];
		int target;
		bool capture;

		if constexpr (gType == CAPTURES or gType == TACTICAL) king_moves &= BBs[nBlack - SIDE];
		else king_moves &= ~BBs[nWhite + SIDE];

		// loop throught all king possible moves
		while (king_moves) {
			target = popLS1B(king_moves);

			// checking for an unattacked position after a move
			if (!isSquareAttacked<SIDE>(target)) {
				if constexpr (gType == CAPTURES or gType == TACTICAL) capture = true;
				else capture = bitU64(target) & BBs[nBlack - SIDE];
				*it++ = MoveItem::encodeQuietCapture<KING, SIDE>(pinData::king_sq, target, capture);
			}
		}

		// if only captures generated - no need to consider castling
		if constexpr (gType == CAPTURES or gType == TACTICAL)
			return;
		else if (check)
			return;

		// prepare king square and proper rook square
		static constexpr int 
			rsq1 = SIDE ? h8 : h1, 
			rsq2 = SIDE ? b8 : b1, 
			rook_left = SIDE ? a8 : a1;

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

	template <GenType gType>
	void generateLegalMoves(MoveList& ml) {
		ml.it = ml.begin();

		game_state.turn == WHITE ?
			generateLegalOf<gType, WHITE>(ml.it) :
			generateLegalOf<gType, BLACK>(ml.it);
	}

	template void generateLegalMoves<LEGAL>(MoveList&);
	template void generateLegalMoves<CAPTURES>(MoveList&);
	template void generateLegalMoves<TACTICAL>(MoveList&);

} // namespace MoveGenerator


namespace MovePerform {

	// handy functions helping getting proper piece index in bitboards set
	template <enumSide Side>
	size_t bbsIndex(uint32_t pc) {
		return pc == PAWN ? nWhitePawn   + Side :
			pc == KNIGHT  ? nWhiteKnight + Side :
			pc == BISHOP  ? nWhiteBishop + Side :
			pc == ROOK    ? nWhiteRook   + Side :
			pc == QUEEN   ? nWhiteQueen  + Side :
			nWhiteKing + Side;
	}

	inline void captureCase(const MoveItem::iMove& move, bool side, int target) {
		if (move.isCapture()) {
			game_state.halfmove = 0;
			popBit(BBs[nBlack - side], target);
			
			// look for captured piece
			for (auto pc = nBlackPawn - side; pc <= nBlackQueen; pc += 2) {
				if (getBit(BBs[pc], target)) {
					hash.key ^= hash.piece_keys.get(pc, target);

					game_state.material[!side] -= Eval::Value::piece_material[toPieceType(pc)];
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

		// update player to turn and en passant state in Zobrist key
		hash.key ^= hash.side_key;

		if (game_state.ep_sq != -1)
			hash.key ^= hash.enpassant_keys.get(game_state.ep_sq);

		// change player to turn and update en passant square
		game_state.turn = !game_state.turn;
		game_state.ep_sq = -1;
		game_state.fullmove += side;

		if (move.isEnPassant()) {
			const int ep_pawn = target + (side ? Compass::nort : Compass::sout);
			
			hash.key ^= hash.piece_keys.get(nWhitePawn + side, origin);
			hash.key ^= hash.piece_keys.get(nWhitePawn + side, target);
			hash.key ^= hash.piece_keys.get(nBlackPawn - side, ep_pawn);
			game_state.halfmove = 0;
			game_state.material[!side] -= Eval::Value::PAWN_VALUE;

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
		else if (const int promotion = move.getPromo()) {
			const int promo_pc = static_cast<int>(bbsIndex<WHITE>(promotion)) + side;

			hash.key ^= hash.piece_keys.get(nWhitePawn + side, origin);
			hash.key ^= hash.piece_keys.get(promo_pc, target);
			game_state.halfmove = 0;
			game_state.material[side] += Eval::Value::piece_material[toPieceType(promo_pc)] - Eval::Value::PAWN_VALUE;

			setBit(BBs[promo_pc], target);
			popBit(BBs[nWhitePawn + side], origin);
			moveBit(BBs[nWhite + side], origin, target);
			moveBit(BBs[nOccupied], origin, target);
			moveBit(BBs[nEmpty], target, origin);
			// maybe there is also a capture?
			captureCase(move, side, target);
			return;
		}
		else if (move.isCastling()) {
			// update castling rights - exclude old castle state and set new castle state then
			hash.key ^= hash.castle_keys.get(game_state.castle.raw());
			game_state.castle &= ~(side ? 3 : 12);
			hash.key ^= hash.castle_keys.get(game_state.castle.raw());
			game_state.halfmove++;

			moveBit(BBs[nWhiteKing + side], origin, target);

			const bool rook_side = target == g1 or target == g8;
			const int 
				rook_origin = (rook_side ? (side ? h8 : h1) : (side ? a8 : a1)),
				rook_target = origin + (rook_side ? 1 : -1);

			hash.key ^= hash.piece_keys.get(nWhiteKing + side, origin);
			hash.key ^= hash.piece_keys.get(nWhiteKing + side, target);
			hash.key ^= hash.piece_keys.get(nWhiteRook + side, rook_origin);
			hash.key ^= hash.piece_keys.get(nWhiteRook + side, rook_target);

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
		const int bbs_pc = static_cast<int>(bbsIndex<WHITE>(piece)) + side;

		hash.key ^= hash.piece_keys.get(bbs_pc, origin);
		hash.key ^= hash.piece_keys.get(bbs_pc, target);
		game_state.halfmove = piece == PAWN ? 0 : game_state.halfmove + 1;

		moveBit(BBs[bbs_pc], origin, target);
		moveBit(BBs[nWhite + side], origin, target);
		moveBit(BBs[nOccupied], origin, target);
		moveBit(BBs[nEmpty], target, origin);

		// captured piece updating
		captureCase(move, side, target);

		// exclude old castle state
		hash.key ^= hash.castle_keys.get(game_state.castle.raw());

		// setting new en passant position, if legal and possible (also in Zobrist notation)
		if (move.isDoublePush()) {
			game_state.ep_sq = target;
			hash.key ^= hash.enpassant_keys.get(game_state.ep_sq);
			game_state.halfmove = 0;
		} // updating castling rights
		else if (piece == ROOK and origin == (side ? a8 : a1))
			game_state.castle &= ~(1 << (!side * 2));
		else if (piece == ROOK and origin == (side ? h8 : h1))
			game_state.castle &= ~(1 << (3 - side * 2));
		else if (piece == KING)
			game_state.castle &= ~(12 - side * 9);

		// update castle state in hash key
		hash.key ^= hash.castle_keys.get(game_state.castle.raw());
	}

	// update hash key and player to turn
	void makeNull() {
		game_state.turn = !game_state.turn;
		hash.key ^= hash.side_key;

		if (game_state.ep_sq != -1)
			hash.key ^= hash.enpassant_keys.get(game_state.ep_sq);

		game_state.ep_sq = -1;
		game_state.halfmove++;
	}

	// unmake move using copy-make approach
	void unmakeMove(const BitBoardsSet& bbs_cpy, const gState& states_cpy) {
		BBs = bbs_cpy;
		game_state = states_cpy;
	}

	// copy-make approach
	void unmakeNull(U64 hash_cpy, int ep_cpy) {
		game_state.turn = !game_state.turn;
		hash.key = hash_cpy;
		game_state.ep_sq = ep_cpy;
		game_state.halfmove--;
	}

} // namespace MovePerform


namespace MoveGenerator {

	namespace Analisis {

		static constexpr GenType perft_gentype = LEGAL;

		template <int Depth>
		unsigned long long dPerft() {
			const auto bbs_cpy = BBs;
			const auto gstate_cpy = game_state;
			unsigned long long total = 0;

			MoveList ml;
			MoveGenerator::generateLegalMoves<perft_gentype>(ml);

			for (const auto& move : ml) {
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
			MoveList ml;
			MoveGenerator::generateLegalMoves<perft_gentype>(ml);
			return ml.size();
		}

		template <int Depth>
		void perft() {
			static Timer timer;
			timer.go();

			MoveList move_list;
			MoveGenerator::generateLegalMoves<perft_gentype>(move_list);

			const BitBoardsSet bbs_cpy = BBs;
			const gState gstate_cpy = game_state;
			ULL total = 0, single;

			for (const auto& move : move_list) {
				MovePerform::makeMove(move);
				total += (single = dPerft<Depth - 1>());
				move.print() << ": " << single << '\n';
				MovePerform::unmakeMove(bbs_cpy, gstate_cpy);
			}

			timer.stop();
			const auto duration = timer.duration();

			const auto perform_kn = static_cast<int>(total / (1. * (duration + 1) / 1000)) / 1000;

			std::cout << std::endl << "Nodes: " << total << std::endl
				<< "Timer: ~" << duration << " ms" << std::endl
				<< "Performance: ~" << perform_kn << " kN/s = ~" << static_cast<int>(perform_kn) / 1000 << " MN/s" << std::endl
				<< "[depth " << Depth << ']' << std::endl;
		}

		// explicit template instantion
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

		// helper macro
#define PERFT(d) perft<d>(); break;

		void perftDriver(int depth) {
			assert(depth > 0 && "Unvalid depth size");

			switch (depth) {
			case 1:  PERFT(1);
			case 2:  PERFT(2);
			case 3:  PERFT(3);
			case 4:  PERFT(4);
			case 5:  PERFT(5);
			case 6:  PERFT(6);
			case 7:  PERFT(7);
			case 8:  PERFT(8);
			case 9:  PERFT(9);
			case 10: PERFT(10);
			default: 
				OS << "Depth not supported";
				break;
			}
		}

#undef PERFT

	} // namespace Analisis


} // namespace MoveGenerator