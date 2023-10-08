#include "MoveItem.h"


template <enumSide SIDE>
uint32_t MoveItem::toMove(int target, int origin, char promo) {
	const bool is_pawn = bitU64(origin) & BBs[nWhitePawn + SIDE],
		capture = bitU64(target) & BBs[nBlack - SIDE];

	// check en passant capture
	if (is_pawn and target == game_state.ep_sq + (SIDE ? Compass::sout : Compass::nort)) {
		return encodeEnPassant<SIDE>(origin, target);
	} // promotion detection
	else if (is_pawn and promo != '\0') {
		static std::string promo_arr = " nbrq";
		return encodePromotion<SIDE>(origin, target, static_cast<int>(promo_arr.find(promo)), capture);
	} // double push detection
	else if (is_pawn and (SIDE ? Constans::r7_rank : Constans::r2_rank) & bitU64(origin)
		and (SIDE ? Constans::r5_rank : Constans::r4_rank) & bitU64(target)) {
		return encode<encodeType::DOUBLE_PUSH>(origin, target, PAWN, SIDE);
	} // check castle
	else if (getLS1BIndex(BBs[nWhiteKing + SIDE]) == origin and
		((game_state.castle.checkLegalCastle<SIDE & ROOK>() and target == (SIDE ? g8 : g1)) or
			(game_state.castle.checkLegalCastle<SIDE & QUEEN>() and target == (SIDE ? c8 : c1)))) {
		return encodeCastle<SIDE>(origin, target);
	}

	// get piece type
	for (auto piece = nWhitePawn + SIDE; piece <= nBlackKing; piece += 2) {
		if (bitU64(origin) & BBs[piece]) {
			return encodeQuietCapture<SIDE>(origin, target, capture, toPieceType(piece));
		}
	}

	return 0;
}


template uint32_t MoveItem::toMove<WHITE>(int, int, char);
template uint32_t MoveItem::toMove<BLACK>(int, int, char);


void MoveItem::iMove::constructMove(std::string move) {
	int origin, target;
	char promo = '\0';

	origin = (move[1] - '1') * 8 + (move[0] - 'a');
	target = (move[3] - '1') * 8 + (move[2] - 'a');

	if (move.size() == 5) {
		promo = move.back();
	}

	cmove = (game_state.turn ?
		toMove<BLACK>(target, origin, promo) :
		toMove<WHITE>(target, origin, promo)
		);
}